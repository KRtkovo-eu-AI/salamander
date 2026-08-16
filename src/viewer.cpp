// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"
#include "common/winlibdpi.h"

#include <algorithm>
#include <vector>
#include <string>
#include <usp10.h>

#include "viewer.h"
#include "common/widepath.h"

#include "cfgdlg.h"
#include "mainwnd.h"
#include "codetbl.h"
#include "codetbl_utils.h"
#include "usermenu.h"
#include "execute.h"
#include "gui.h"

const char* CVIEWERWINDOW_CLASSNAME = "Salamander's Viewer Window";
const wchar_t* CVIEWERWINDOW_CLASSNAMEW = L"Salamander's Viewer Window";

std::wstring ViewerTextToWide(const char* text)
{
    if (text == NULL)
        text = "";
    int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0);
    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (len == 0)
    {
        codePage = CP_ACP;
        flags = 0;
        len = MultiByteToWideChar(codePage, flags, text, -1, NULL, 0);
    }
    if (len <= 0)
        return std::wstring();

    std::wstring wide(len, L'\0');
    int written = MultiByteToWideChar(codePage, flags, text, -1, &wide[0], len);
    if (written <= 0)
        return std::wstring();
    wide.resize(written - 1);
    return wide;
}

void SetViewerWindowText(HWND hWindow, const char* text)
{
    std::wstring wide = ViewerTextToWide(text);
    SetWindowTextW(hWindow, wide.c_str());
}


static std::wstring ViewerPathToWide(const char* path)
{
    std::wstring wide = SalMultiByteToWidePath(path, CP_UTF8);
    if (wide.empty() && GetACP() != CP_UTF8)
        wide = SalMultiByteToWidePath(path, CP_ACP);
    return wide;
}

char* ViewerHistory[VIEWER_HISTORY_SIZE];

HACCEL ViewerTable = NULL;
BOOL UseCustomViewerFont = FALSE;
LOGFONT ViewerLogFont;
HMENU ViewerMenu = NULL;
int CharWidth = 1,  // character width (in points); we divide by this value, so we will never set it to zero
    CharHeight = 1; // character height (in points); we divide by this value, so we will never set it to zero

CRITICAL_SECTION ViewerFontMeasureCS;
BOOL ViewerFontMeasured = FALSE;
BOOL ViewerFontNeedsMapping = FALSE;
char ViewerFontMapping[256];

static HWND ResolveHistoryComboEditControl(HWND ctrl)
{
    char className[16];
    if (GetClassName(ctrl, className, (int)ARRAYSIZE(className)) > 0 &&
        _stricmp(className, "ComboBox") == 0)
    {
        COMBOBOXINFO info;
        info.cbSize = sizeof(info);
        if (GetComboBoxInfo(ctrl, &info) && info.hwndItem != NULL)
            return info.hwndItem;
    }
    return ctrl;
}

static bool GetHistoryControlTextUtf8(HWND ctrl, char* buffer, int bufferSize)
{
    if (buffer == NULL || bufferSize <= 0)
        return false;
    buffer[0] = 0;

    HWND source = ResolveHistoryComboEditControl(ctrl);
    if (GetACP() != CP_UTF8 && !IsWindowUnicode(source))
    {
        SendMessage(ctrl, WM_GETTEXT, bufferSize, (LPARAM)buffer);
        return true;
    }

    int length = GetWindowTextLengthW(source);
    if (length < 0)
        length = 0;
    std::vector<WCHAR> wide(length + 1);
    int copied = GetWindowTextW(source, wide.data(), length + 1);
    if (copied < 0)
        copied = 0;
    wide[copied] = 0;

    int written = WideCharToMultiByte(CP_UTF8, 0, wide.data(), copied, buffer, bufferSize - 1, NULL, NULL);
    if (written < 0)
        written = 0;
    buffer[written] = 0;
    return written < bufferSize - 1;
}

static void SetHistoryControlTextUtf8(HWND ctrl, const char* text)
{
    if (text == NULL)
        text = "";
    HWND target = ResolveHistoryComboEditControl(ctrl);
    if (GetACP() != CP_UTF8 && !IsWindowUnicode(target))
    {
        SendMessage(ctrl, WM_SETTEXT, 0, (LPARAM)text);
        return;
    }

    std::wstring wide = SalMultiByteToWidePath(text, CP_UTF8);
    if (IsWindowUnicode(target))
        SetWindowTextW(target, wide.c_str());
    else
        SendMessage(target, WM_SETTEXT, 0, (LPARAM)text);
}

void GetDefaultViewerLogFont(LOGFONT* lf)
{
    const int VIEWER_FONT_PTS = 10;
    memset(lf, 0, sizeof(*lf));
    lf->lfHeight = -(VIEWER_FONT_PTS * SystemDPI) / 72;
    lf->lfWeight = FW_NORMAL;
    lf->lfCharSet = UserCharset;
    lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf->lfQuality = DEFAULT_QUALITY;
    lf->lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    strcpy(lf->lfFaceName, "Consolas");
}

//
//*****************************************************************************

void HistoryComboBox(HWND hWindow, CTransferInfo& ti, int ctrlID, char* Text,
                     int textLen, BOOL hexMode, int historySize, char* history[],
                     BOOL changeOnlyHistory)
{
    CALL_STACK_MESSAGE6("HistoryComboBox(, , %d, , %d, %d, %d, , %d)",
                        ctrlID, textLen, hexMode, historySize, changeOnlyHistory);
    HWND hwnd;
    if (changeOnlyHistory || ti.GetControl(hwnd, ctrlID))
    {
        if (!changeOnlyHistory && ti.Type == ttDataToWindow)
        {
            SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
            SendMessage(hwnd, CB_LIMITTEXT, textLen - 1, 0);
            SetHistoryControlTextUtf8(hwnd, Text);
        }
        else
        {
            if (!changeOnlyHistory)
            {
                GetHistoryControlTextUtf8(hwnd, Text, textLen);
                SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
                SendMessage(hwnd, CB_LIMITTEXT, textLen - 1, 0);
                SetHistoryControlTextUtf8(hwnd, Text);
            }

            // hex mode handling
            if (hexMode)
            {
                char* s = Text;
                BOOL openedQuotes = FALSE;
                char* lastQuotes = NULL;
                while (*s != 0 && (openedQuotes || *s == ' ' || *s >= '0' && *s <= '9' ||
                                   LowerCase[*s] >= 'a' && LowerCase[*s] <= 'f' ||
                                   *s == '"'))
                {
                    if (*s == '"')
                    {
                        openedQuotes = !openedQuotes;
                        lastQuotes = s;
                    }
                    s++;
                }
                if (openedQuotes)
                    s = lastQuotes;
                if (*s != 0) // contains a non-hex character
                {
                    if (!changeOnlyHistory)
                    {
                        SalMessageBox(hWindow, LoadStr(IDS_STRINGISNOTHEX), LoadStr(IDS_ERRORTITLE),
                                      MB_OK | MB_ICONEXCLAMATION);
                        SetFocus(hwnd);
                        SendMessage(hwnd, CB_SETEDITSEL, 0, MAKELPARAM(s - Text, 1 + (s - Text)));
                    }
                    ti.ErrorOn(ctrlID);
                }
            }
            // everything is fine; store it in the history
            if (ti.IsGood())
            {
                if (Text[0] != 0)
                {
                    BOOL insert = TRUE;
                    int i;
                    for (i = 0; i < historySize; i++)
                    {
                        if (history[i] != NULL)
                        {
                            if (strcmp(history[i], Text) == 0) // already in the history
                            {                                  // move it to position 0
                                if (i > 0)
                                {
                                    char* swap = history[i];
                                    memmove(history + 1, history, i * sizeof(char*));
                                    history[0] = swap;
                                }
                                insert = FALSE;
                                break;
                            }
                        }
                        else
                            break;
                    }

                    if (insert)
                    {
                        char* newText = (char*)malloc(strlen(Text) + 1);
                        if (newText != NULL)
                        {
                            strcpy(newText, Text);
                            if (history[historySize - 1] != NULL)
                                free(history[historySize - 1]);
                            memmove(history + 1, history,
                                    (historySize - 1) * sizeof(char*));
                            history[0] = newText;
                        }
                        else
                            TRACE_E(LOW_MEMORY);
                    }
                }
            }
        }

        if (!changeOnlyHistory)
        {
            int i;
            for (i = 0; i < historySize; i++) // fill the combo-box list
                if (history[i] != NULL)
                    SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)history[i]);
                else
                    break;
        }
    }
}

//
//*****************************************************************************

void DoHexValidation(HWND edit, const int textLen)
{
    CALL_STACK_MESSAGE2("DoHexValidation(, %d)", textLen);
    int start, end;
    SendMessage(edit, CB_GETEDITSEL, (WPARAM)&start, (LPARAM)&end);
    char* text = new char[textLen];
    if (text == NULL)
    {
        TRACE_E(LOW_MEMORY);
        return;
    }
    SendMessage(edit, WM_GETTEXT, textLen, (LPARAM)text);
    char* s = text;
    while (*s != 0 && *s == ' ')
        s++;
    if (s != text)
    {
        start -= (int)(s - text);
        end -= (int)(s - text);
        if (start < 0)
            start = 0;
        if (end < 0)
            end = 0;
        memmove(text, s, strlen(s) + 1);
    }
    s = text;
    BOOL openedQuotes = FALSE;
    char *st = s, *strEnd = text + strlen(text);
    while (*s != 0)
    {
        if (*s == '"')
        {
            if (!openedQuotes && s > text && *(s - 1) != ' ' && strEnd - text < textLen - 1)
            {
                if (start > s - text)
                    start++;
                if (end > s - text)
                    end++;
                memmove(s + 1, s, (strEnd - s) + 1);
                *s++ = ' ';
                strEnd++;
            }
            else
            {
                if (openedQuotes && s + 1 < strEnd && *(s + 1) != ' ' &&
                    strEnd - text < textLen - 1)
                {
                    if (start >= (s - text) + 1)
                        start++;
                    if (end >= (s - text) + 1)
                        end++;
                    memmove(s + 2, s + 1, strEnd - s);
                    *(s + 1) = ' ';
                    strEnd++;
                }
                if (openedQuotes && s + 1 < strEnd)
                    s++;
                st = s + 1;
            }
            openedQuotes = !openedQuotes;
        }
        else
        {
            if (!openedQuotes)
            {
                if (*s == ' ')
                {
                    if (st == s) // '  ' -> ' '
                    {
                        s--;
                        if (start >= st - text)
                            start--;
                        if (end >= st - text)
                            end--;
                        memmove(s, st, (strEnd - st) + 1);
                        strEnd--;
                    }
                    else
                        st = s + 1;
                }
                else
                {
                    if ((s - st) == 2) // 'ABC' -> 'AB C'
                    {
                        if (strEnd - text < textLen - 1)
                        {
                            if (start >= s - text)
                                start++;
                            if (end >= s - text)
                                end++;
                            memmove(s + 1, s, (strEnd - s) + 1);
                            *s = ' ';
                            st = s + 1;
                            strEnd++;
                        }
                    }
                }
            }
        }
        s++;
    }
    SendMessage(edit, WM_SETTEXT, 0, (LPARAM)text);
    SendMessage(edit, CB_SETEDITSEL, 0, MAKELPARAM(start, end));
    delete[] (text);
}

//
//*****************************************************************************

void ConvertHexToString(char* text, char* hex, int& len)
{
    CALL_STACK_MESSAGE2("ConvertHexToString(%s, ,)", text);
    len = 0;
    char *s = text, *st = text;
    BYTE value = 0;
    BOOL openedQuotes = FALSE;
    while (1)
    {
        if (*s == '"')
        {
            s++;
            openedQuotes = !openedQuotes;
            continue;
        }
        if (openedQuotes)
        {
            if (*s == 0)
                break;
            else
                hex[len++] = *s++;
        }
        else
        {
            if (*s != ' ')
            {
                if (*s == 0)
                    break; // end of string
                else
                {
                    if (*s >= '0' && *s <= '9')
                        value = (BYTE)(*s - '0'); // first digit
                    else
                        value = (BYTE)(10 + (LowerCase[*s] - 'a'));
                    s++;
                    if (*s != ' ' && *s != 0 && *s != '"') // second digit
                    {
                        value <<= 4;
                        if (*s >= '0' && *s <= '9')
                            value |= (BYTE)(*s - '0');
                        else
                            value |= (BYTE)(10 + (LowerCase[*s] - 'a'));
                        s++;
                    }
                    hex[len++] = value;
                }
            }
            else
                s++; // skip the space
        }
    }
}

//
//*****************************************************************************
// CFindSetDialog
//

void CFindSetDialog::Transfer(CTransferInfo& ti)
{
    ti.CheckBox(IDC_FINDHEX, HexMode);
    ti.CheckBox(IDC_VIEWREGEXP, Regular);
    HistoryComboBox(HWindow, ti, IDC_FINDTEXT, Text, FIND_TEXT_LEN, !Regular && HexMode,
                    VIEWER_HISTORY_SIZE, ViewerHistory);
    if (ti.Type == ttDataToWindow)
    { // initialize the search text based on the selection in the viewer (the parent of this dialog)
        CWindowsObject* win = WindowsManager.GetWindowPtr(Parent);
        if (win != NULL && win->Is(otViewerWindow)) // just to be sure, check that it is a viewer window
        {
            CViewerWindow* view = (CViewerWindow*)win;
            char buf[FIND_TEXT_LEN];
            char hexBuf[FIND_TEXT_LEN];
            int len;
            if (view->GetFindText(buf, len))
            {
                if (HexMode)
                {
                    if (len * 3 > FIND_TEXT_LEN)
                        len = (FIND_TEXT_LEN - 1) / 3;
                    int i;
                    for (i = 0; i < len; i++)
                    {
                        sprintf(hexBuf + i * 3, i == len - 1 ? "%02X" : "%02X ", (unsigned)buf[i]);
                    }
                    strcpy(buf, hexBuf);
                }
                SendMessage(GetDlgItem(HWindow, IDC_FINDTEXT), WM_SETTEXT, 0, (LPARAM)buf);
            }
        }
    }
    ti.RadioButton(IDC_SBACKWARD, 0, Forward);
    ti.RadioButton(IDC_SFORWARD, 1, Forward);
    ti.CheckBox(IDC_WHOLEWORDS, WholeWords);
    ti.CheckBox(IDC_CASESENSITIVE, CaseSensitive);
}

INT_PTR
CFindSetDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CFindSetDialog::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        CancelHexMode = HexMode;
        CancelRegular = Regular;
        EnableWindow(GetDlgItem(HWindow, IDC_FINDHEX), !Regular);
        if (Regular)
            CheckDlgButton(HWindow, IDC_FINDHEX, BST_UNCHECKED);
        ChangeToArrowButton(HWindow, IDC_REGEXP_BROWSE);

        CComboboxEdit* edit = new CComboboxEdit();
        if (edit != NULL)
        {
            HWND hCombo = GetDlgItem(HWindow, IDC_FINDTEXT);
            edit->AttachToWindow(GetWindow(hCombo, GW_CHILD));
        }

        break;
    }

    case WM_USER_CLEARHISTORY:
    {
        // we should clear the histories
        ClearComboboxListbox(GetDlgItem(HWindow, IDC_FINDTEXT));
        return 0;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
        {
            HexMode = CancelHexMode; // keep Cancel correct
            Regular = CancelRegular;
            break;
        }

        case IDC_REGEXP_BROWSE:
        {
            const CExecuteItem* item = TrackExecuteMenu(HWindow, IDC_REGEXP_BROWSE, IDC_FINDTEXT,
                                                        TRUE, RegularExpressionItems);
            if (item != NULL)
            {
                BOOL regular = (IsDlgButtonChecked(HWindow, IDC_VIEWREGEXP) == BST_CHECKED);
                if (item->Keyword == EXECUTE_HELP)
                {
                    // open the help page dedicated to regular expressions
                    OpenHtmlHelp(NULL, HWindow, HHCDisplayContext, IDH_REGEXP, FALSE);
                }
                if (item->Keyword != EXECUTE_HELP && !regular)
                {
                    // the user selected an expression, so tick the checkbox for regular search
                    CheckDlgButton(HWindow, IDC_VIEWREGEXP, BST_CHECKED);
                    PostMessage(HWindow, WM_COMMAND, MAKELPARAM(IDC_VIEWREGEXP, BN_CLICKED), 0);
                }
            }
            return 0;
        }

        case IDC_VIEWREGEXP:
        {
            Regular = (IsDlgButtonChecked(HWindow, IDC_VIEWREGEXP) != BST_UNCHECKED);
            EnableWindow(GetDlgItem(HWindow, IDC_FINDHEX), !Regular);
            if (Regular)
                CheckDlgButton(HWindow, IDC_FINDHEX, BST_UNCHECKED);
            break;
        }

        case IDC_FINDHEX:
        {
            if (HIWORD(wParam) == BN_CLICKED)
            {
                HexMode = (IsDlgButtonChecked(HWindow, IDC_FINDHEX) != BST_UNCHECKED);
                if (HexMode)
                    CheckDlgButton(HWindow, IDC_CASESENSITIVE, BST_CHECKED);
                return TRUE;
            }
            break;
        }

        case IDC_FINDTEXT:
        {
            if (!Regular && HexMode && HIWORD(wParam) == CBN_EDITUPDATE)
            {
                DoHexValidation((HWND)lParam, FIND_TEXT_LEN);
                return TRUE;
            }
            break;
        }
        }
        break;
    }
    }
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}

//
//*****************************************************************************
// CViewerGoToOffsetDialog
//

void CViewerGoToOffsetDialog::Validate(CTransferInfo& ti)
{
    int h;
    ti.CheckBox(IDC_VGTO_HEX, h);
    __int64 dummy;
    ti.EditLine(IDE_VGTO_OFFSET, dummy, TRUE, TRUE, h);
}

void CViewerGoToOffsetDialog::Transfer(CTransferInfo& ti)
{
    ti.CheckBox(IDC_VGTO_HEX, Configuration.GoToOffsetIsHex);
    ti.EditLine(IDE_VGTO_OFFSET, *Offset, TRUE, TRUE, Configuration.GoToOffsetIsHex);
}

INT_PTR
CViewerGoToOffsetDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CViewerGoToOffsetDialog::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDC_VGTO_HEX && HIWORD(wParam) == BN_CLICKED)
        { // switching HEX = change the offset from decimal to hex and back
            BOOL h = IsDlgButtonChecked(HWindow, IDC_VGTO_HEX) != BST_UNCHECKED;
            CTransferInfo ti(HWindow, ttDataFromWindow);
            __int64 off;
            ti.EditLine(IDE_VGTO_OFFSET, off, TRUE, TRUE, !h, FALSE, TRUE); // do not show an error; just skip conversion
            if (ti.IsGood())
            {
                CTransferInfo ti2(HWindow, ttDataToWindow);
                ti2.EditLine(IDE_VGTO_OFFSET, off, FALSE, TRUE, h);
            }
        }
        break;
    }
    }
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}

//
//*****************************************************************************
// CViewerWindow
//

CViewerWindow::CViewerWindow(const char* fileName, CViewType type, const char* caption,
                             BOOL wholeCaption, CObjectOrigin origin,
                             int enumFileNamesSourceUID, int enumFileNamesLastFileIndex)
    : CWindow(origin), LineOffset(300, 100),
      FindDialog(HLanguage, IDD_FINDSET, IDD_FINDSET)
{
    // GDI variables
    BkgndBrush = NULL;
    BkgndBrushSel = NULL;
    LineNumberBrush = NULL;
    ViewerFont = NULL;
    StatusFont = NULL;
    HStatusBar = HScrollBar = VScrollBar = HZoomReset = HZoomOut = HZoomEdit = HZoomIn = NULL;
    StatusBarHeight = 0;
    ZoomPercent = Configuration.ViewerZoomPercent;
    StatusOffset = -1;
    CachedTotalLines = -1;
    CachedMaxLineLen = 0;
    VisibleFirstDocumentLine = -1;
    CachedSelectionStart = CachedSelectionEnd = CachedSelectionLineCount = CachedSelectionCharacterCount = -1;
    CachedVerticalPageSize = -1;
    LayoutNeeded = TRUE;

    Width = Height = 0;

    // dummy bitmap -- the correct size will be set in WM_SIZE
    if (!Bitmap.CreateBmp(NULL, 1, 1))
        TRACE_E("Unable to create bitmap or memory DC for viewer.");

    CreateViewerBrushs();
    SetViewerFont(); // uses Bitmap (must already be allocated to at least 1x1) and Width (must already be initialized to at least 0)

    // other variables
    HexOffsetLength = 0;
    CanSwitchToHex = TRUE;
    CanSwitchQuietlyToHex = FALSE;
    FindingSoDonotSwitchToHex = FALSE;
    WaitForViewerRefresh = FALSE;
    LastSeekY = 0;
    LastOriginX = 0;
    RepeatCmdAfterRefresh = -1;
    CurrentDir[0] = 0;
    ExitTextMode = FALSE;
    ForceTextMode = FALSE;
    CodeType = 0;
    CodeTables.Init(MainWindow->HWindow);
    UseCodeTable = FALSE;
    TextEncoding = Salamander::Unicode::BomEncoding::LegacyBytes;
    TextContentOffset = 0;
    if (fileName == NULL)
        FileName = NULL; // error
    else
    {
        char name[SAL_MAX_PATH];
        lstrcpyn(name, fileName, SAL_MAX_PATH);
        if (SalGetFullName(name, NULL, NULL, NULL, NULL, SAL_MAX_PATH))
        {
            FileName = (char*)malloc(strlen(name) + 1);
            if (FileName != NULL)
            {
                strcpy(FileName, name);
                FileNameW = ViewerPathToWide(name);
            }
        }
        else
            FileName = NULL;
    }
    Buffer = (unsigned char*)malloc(VIEW_BUFFER_SIZE);
    Seek = 0;
    Loaded = 0;
    DefViewMode = Configuration.DefViewMode;
    Type = type;
    OriginX = SeekY = 0;
    MaxSeekY = -1;
    ViewSize = FileSize = 0;
    LastLineSize = FirstLineSize = 0;
    EnablePaint = TRUE;
    StartSelection = -1; // no selection yet
    EndSelection = -1;   // no selection yet
    TooBigSelAction = 0;
    EndSelectionRow = -1;
    EndSelectionPrefX = -1;
    WrapIsBeforeFirstLine = FALSE;
    MouseDrag = FALSE;
    ChangingSelWithShiftKey = FALSE;
    FindOffset = 0;
    ResetFindOffsetOnNextPaint = TRUE;
    SelectionIsFindResult = FALSE;
    ScrollScaleX = ScrollScaleY = 0;
    EnableSetScroll = TRUE;
    ScrollToSelection = FALSE;
    ToolTipOffset = -1;
    HToolTip = NULL;
    Lock = NULL;
    WrapText = Configuration.WrapText;
    ShowLineNumbers = Configuration.ViewerShowLineNumbers;
    ShowStatusBar = Configuration.ViewerShowStatusBar;
    LogViewMode = FALSE;
    LineNumberDigits = 1;
    CodePageAutoSelect = Configuration.CodePageAutoSelect;
    strcpy(DefaultConvert, Configuration.DefaultConvert);
    LastFindSeekY = -1;
    LastFindOffset = -1;

    if (caption != NULL)
    {
        Caption = DupStr(caption);
        WholeCaption = wholeCaption;
    }
    else
    {
        Caption = NULL;
        WholeCaption = FALSE;
    }
    EnumFileNamesSourceUID = enumFileNamesSourceUID;
    EnumFileNamesLastFileIndex = enumFileNamesLastFileIndex;
    VScrollWParam = -1;

    ResetMouseWheelAccumulator();
}

CViewerWindow::~CViewerWindow()
{
    if (StatusFont != NULL)
        HANDLES(DeleteObject(StatusFont));
    if (ViewerFont != NULL)
        HANDLES(DeleteObject(ViewerFont));
    ReleaseViewerBrushs();
    if (Lock != NULL)
    {
        SetEvent(Lock);
        Lock = NULL; // now it is up to the disk cache
    }
    if (Buffer != NULL)
        free(Buffer);
    if (FileName != NULL)
        free(FileName);
    FileNameW.erase();
    if (Caption != NULL)
        free(Caption);
}

HANDLE
CViewerWindow::GetLockObject()
{
    if (Lock == NULL)
        Lock = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    return Lock;
}

void CViewerWindow::CloseLockObject()
{
    if (Lock != NULL)
    {
        HANDLES(CloseHandle(Lock));
        Lock = NULL;
    }
}

void CViewerWindow::FindNewSeekY(__int64 newSeekY, BOOL& fatalErr)
{
    CALL_STACK_MESSAGE2("CViewerWindow::FindNewSeekY(%g,)", (double)newSeekY);
    fatalErr = FALSE;
    if (newSeekY >= MaxSeekY)
        SeekY = MaxSeekY;
    else
    {
        if (newSeekY == 0)
            SeekY = 0;
        else
        {
            newSeekY = FindBegin(newSeekY, fatalErr);
            if (!fatalErr && !ExitTextMode)
                SeekY = newSeekY;
        }
    }
}

int TranslateU2T(int u, BOOL left, int hexOffsetLength)
{
    int i = u - (62 - 8 + hexOffsetLength);
    return left ? (10 - 8 + hexOffsetLength + i * 3 + (i / 4)) : (9 - 8 + hexOffsetLength + i * 3 + ((i - 1) / 4));
}

int GetHexOffsetMode(unsigned __int64 fileSize, int& hexOffsetLength)
{
    if (fileSize == 0)
    {
        hexOffsetLength = 4;
        return 1; // at least 4 characters
    }
    --fileSize;                                              // the largest possible offset in the file is one less than the file size
    if (fileSize <= CQuadWord(0x0000FFFF, 0x00000000).Value) // 4 characters are enough
    {
        hexOffsetLength = 4;
        return 1;
    }
    else
    {
        if (fileSize <= CQuadWord(0xFFFFFFFF, 0x00000000).Value) // 8 characters are enough
        {
            hexOffsetLength = 9;
            return 2;
        }
        else
        {
            if (fileSize <= CQuadWord(0xFFFFFFFF, 0x0000FFFF).Value) // 12 characters are enough
            {
                hexOffsetLength = 14;
                return 3;
            }
            else // 16 characters are necessary
            {
                hexOffsetLength = 19;
                return 4;
            }
        }
    }
}

#define LOWORD64(qw) ((WORD)((qw) & 0xffff))

void PrintHexOffset(char* s, unsigned __int64 offset, int mode)
{
    switch (mode)
    {
    case 1:
        sprintf(s, "%04X", LOWORD64(offset));
        return; // 4 characters are enough
    case 2:
        sprintf(s, "%04X %04X", LOWORD64(offset >> 16), LOWORD64(offset));
        return; // 8 characters are enough
    case 3:
        sprintf(s, "%04X %04X %04X", LOWORD64(offset >> 32), LOWORD64(offset >> 16), LOWORD64(offset));
        return; // 12 characters are enough
    case 4:
        sprintf(s, "%04X %04X %04X %04X", LOWORD64(offset >> 48), LOWORD64(offset >> 32),
                LOWORD64(offset >> 16), LOWORD64(offset));
        return; // 16 characters are necessary
    }
    TRACE_E("Unexpected situation in PrintHexOffset().");
}

void MyTextOut(HDC hdc, int nXStart, int nYStart, LPCTSTR lpString, int cbString)
{
#ifdef _DEBUG
    if (!ViewerFontMeasured)
        TRACE_E("MyTextOut(): ViewerFontMeasured is FALSE!");
#endif // _DEBUG
    // The UTF-8 activeCodePage manifest changes TextOutA semantics process-wide.
    // Legacy viewer rows (including conversion-table output) are still bytes in
    // the regional Windows code page, so invalid CP1250/CP1251 bytes otherwise
    // disappear. Keep the historic ANSI path on non-UTF-8 systems and use
    // Unicode GDI only where the manifest makes it necessary.
    if (GetACP() == CP_UTF8)
    {
        std::wstring wide;
        if (ConvertLegacyViewerTextToWide(lpString, cbString,
                                          GetEffectiveConversionCodePage(), &wide))
        {
            TextOutW(hdc, nXStart, nYStart, wide.data(), (int)wide.size());
            return;
        }
    }
    if (ViewerFontNeedsMapping)
    {
        const char* s = lpString;
        if (cbString >= 2001)
        {
            cbString = 2000;
            TRACE_E("MyTextOut(): too long string! Truncating to 2000 characters!");
        }
        const char* end = s + cbString;
        char buf[2001];
        char* d = buf;
        while (s < end)
            *d++ = ViewerFontMapping[(unsigned char)*s++];
        *d = 0;
        TextOut(hdc, nXStart, nYStart, buf, cbString);
    }
    else
        TextOut(hdc, nXStart, nYStart, lpString, cbString);
}

void MyTextOutW(HDC hdc, int nXStart, int nYStart, const wchar_t* lpString, int cchString)
{
#ifdef _DEBUG
    if (!ViewerFontMeasured)
        TRACE_E("MyTextOutW(): ViewerFontMeasured is FALSE!");
#endif // _DEBUG
    TextOutW(hdc, nXStart, nYStart, lpString, cchString);
}

namespace
{

bool IsViewerDecodedEOL(std::uint32_t scalar)
{
    return scalar == L'\r' || scalar == L'\n' || scalar == 0;
}

void AppendVisualCell(Salamander::Unicode::DecodedRun& visual, std::uint32_t scalar,
                      __int64 rawStart, __int64 rawEnd, int tabSize)
{
    if (scalar == L'\t')
    {
        std::size_t column = Salamander::Unicode::BuildTextElementMap(visual).Count();
        int tab = (int)(tabSize - (column % tabSize));
        if (tab <= 0)
            tab = 1;
        while (tab-- > 0)
            visual.AppendCell(L' ', rawStart, rawEnd);
    }
    else
        visual.AppendCell(scalar, rawStart, rawEnd);
}

std::size_t DecodedSelectionStartElement(const Salamander::Unicode::DecodedRun& visual,
                                         const Salamander::Unicode::TextElementMap& elements,
                                         __int64 offset)
{
    for (std::size_t i = 0; i < elements.Count(); ++i)
    {
        std::size_t first = elements.CellStart(i);
        std::size_t last = elements.CellEnd(i) - 1;
        if (offset <= visual.RawStart[first])
            return i;
        if (offset < visual.RawEnd[last])
            return i;
    }
    return elements.Count();
}

std::size_t DecodedSelectionEndElement(const Salamander::Unicode::DecodedRun& visual,
                                       const Salamander::Unicode::TextElementMap& elements,
                                       __int64 offset)
{
    for (std::size_t i = 0; i < elements.Count(); ++i)
    {
        std::size_t first = elements.CellStart(i);
        std::size_t last = elements.CellEnd(i) - 1;
        if (offset <= visual.RawStart[first])
            return i;
        if (offset <= visual.RawEnd[last])
            return i + 1;
    }
    return elements.Count();
}

void DrawDecodedElements(HDC dc, const Salamander::Unicode::DecodedRun& visual,
                         const Salamander::Unicode::TextElementMap& elements,
                         std::size_t elementStart, std::size_t elementEnd, int xCell)
{
    if (elementStart >= elementEnd)
        return;
    std::size_t cellStart = elements.CellStart(elementStart);
    std::size_t cellEnd = elements.CellStart(elementEnd);
    std::size_t textStart = visual.TextIndexForCellEnd(cellStart);
    std::size_t textEnd = visual.TextIndexForCellEnd(cellEnd);
    if (textEnd > textStart)
    {
        int textLength = (int)(textEnd - textStart);
        SCRIPT_STRING_ANALYSIS analysis = nullptr;
        int glyphCapacity = textLength + textLength / 2 + 16;
        if (SUCCEEDED(ScriptStringAnalyse(dc, visual.Text.c_str() + textStart, textLength,
                                          glyphCapacity, -1, SSA_GLYPHS | SSA_FALLBACK | SSA_LINK,
                                          0, nullptr, nullptr, nullptr, nullptr, nullptr, &analysis)))
        {
            ScriptStringOut(analysis, xCell * CharWidth, 0, 0, nullptr, 0, 0, FALSE);
            ScriptStringFree(&analysis);
        }
        else
            MyTextOutW(dc, xCell * CharWidth, 0, visual.Text.c_str() + textStart, textLength);
    }
}

int DecodedElementsPixelWidth(HDC dc, const Salamander::Unicode::DecodedRun& visual,
                              const Salamander::Unicode::TextElementMap& elements,
                              std::size_t elementStart, std::size_t elementEnd)
{
    if (elementStart >= elementEnd)
        return 0;
    std::size_t cellStart = elements.CellStart(elementStart);
    std::size_t cellEnd = elements.CellStart(elementEnd);
    std::size_t textStart = visual.TextIndexForCellEnd(cellStart);
    std::size_t textEnd = visual.TextIndexForCellEnd(cellEnd);
    if (textEnd <= textStart)
        return 0;

    int textLength = (int)(textEnd - textStart);
    SCRIPT_STRING_ANALYSIS analysis = nullptr;
    int glyphCapacity = textLength + textLength / 2 + 16;
    if (SUCCEEDED(ScriptStringAnalyse(dc, visual.Text.c_str() + textStart, textLength,
                                      glyphCapacity, -1, SSA_GLYPHS | SSA_FALLBACK | SSA_LINK,
                                      0, nullptr, nullptr, nullptr, nullptr, nullptr, &analysis)))
    {
        const SIZE* size = ScriptString_pSize(analysis);
        int width = size != nullptr ? size->cx : 0;
        ScriptStringFree(&analysis);
        return width;
    }

    SIZE size = {0, 0};
    if (GetTextExtentPoint32W(dc, visual.Text.c_str() + textStart, textLength, &size))
        return size.cx;
    return (int)(elementEnd - elementStart) * CharWidth;
}

struct DecodedPixelInterval
{
    int Left;
    int Right;
};

std::vector<DecodedPixelInterval> DecodedSelectionPixelIntervals(
    HDC dc, const Salamander::Unicode::DecodedRun& visual,
    const Salamander::Unicode::TextElementMap& elements,
    std::size_t visibleStart, std::size_t visibleEnd,
    std::size_t selectedStart, std::size_t selectedEnd)
{
    std::vector<DecodedPixelInterval> intervals;
    selectedStart = max(selectedStart, visibleStart);
    selectedEnd = min(selectedEnd, visibleEnd);
    if (selectedStart >= selectedEnd)
        return intervals;

    std::size_t baseText = visual.TextIndexForCellEnd(elements.CellStart(visibleStart));
    std::size_t textEnd = visual.TextIndexForCellEnd(elements.CellStart(visibleEnd));
    int textLength = (int)(textEnd - baseText);
    SCRIPT_STRING_ANALYSIS analysis = nullptr;
    int glyphCapacity = textLength + textLength / 2 + 16;
    if (textLength > 0 &&
        SUCCEEDED(ScriptStringAnalyse(dc, visual.Text.c_str() + baseText, textLength,
                                      glyphCapacity, -1, SSA_GLYPHS | SSA_FALLBACK | SSA_LINK,
                                      0, nullptr, nullptr, nullptr, nullptr, nullptr, &analysis)))
    {
        for (std::size_t element = selectedStart; element < selectedEnd; ++element)
        {
            int first = (int)(visual.TextIndexForCellEnd(elements.CellStart(element)) - baseText);
            int last = (int)(visual.TextIndexForCellEnd(elements.CellEnd(element)) - baseText) - 1;
            int leading = 0;
            int trailing = 0;
            if (last >= first && SUCCEEDED(ScriptStringCPtoX(analysis, first, FALSE, &leading)) &&
                SUCCEEDED(ScriptStringCPtoX(analysis, last, TRUE, &trailing)))
                intervals.push_back({min(leading, trailing), max(leading, trailing)});
        }
        ScriptStringFree(&analysis);
    }

    if (intervals.empty())
    {
        int left = DecodedElementsPixelWidth(dc, visual, elements, visibleStart, selectedStart);
        int right = DecodedElementsPixelWidth(dc, visual, elements, visibleStart, selectedEnd);
        intervals.push_back({min(left, right), max(left, right)});
        return intervals;
    }

    std::sort(intervals.begin(), intervals.end(), [](const DecodedPixelInterval& left, const DecodedPixelInterval& right)
              { return left.Left < right.Left; });
    std::vector<DecodedPixelInterval> merged;
    for (const DecodedPixelInterval& interval : intervals)
    {
        if (!merged.empty() && interval.Left <= merged.back().Right + 1)
            merged.back().Right = max(merged.back().Right, interval.Right);
        else
            merged.push_back(interval);
    }
    return merged;
}

} // namespace

BOOL CViewerWindow::DecodeTextRange(HANDLE* hFile, __int64 start, __int64 end,
                                    Salamander::Unicode::DecodedRun& run, BOOL& fatalErr, bool flush)
{
    run.Clear();
    fatalErr = FALSE;
    if (!HasDecodedTextEncoding())
        return FALSE;

    start = max(start, TextContentOffset);
    end = min(end, FileSize);
    start = Salamander::Unicode::AlignToCodeUnit(TextEncoding, start, TextContentOffset);
    if (end <= start)
        return TRUE;

    __int64 off = start;
    while (off < end)
    {
        __int64 want = min((__int64)APROX_LINE_LEN + 8, end - off);
        __int64 len = Prepare(hFile, off, want, fatalErr);
        if (fatalErr)
            return FALSE;
        if (len <= 0)
            break;

        bool finalChunk = flush && off + len >= end;
        Salamander::Unicode::DecodedRun part = Salamander::Unicode::DecodeBytes(TextEncoding, Buffer + (off - Seek), (std::size_t)len, off, finalChunk);
        if (part.RawBytesConsumed == 0 && part.CellCount() == 0)
        {
            if (finalChunk)
                break;
            len = min((__int64)APROX_LINE_LEN + 16, FileSize - off);
            len = Prepare(hFile, off, len, fatalErr);
            if (fatalErr || len <= 0)
                return !fatalErr;
            part = Salamander::Unicode::DecodeBytes(TextEncoding, Buffer + (off - Seek), (std::size_t)len, off, off + len >= FileSize);
            if (part.RawBytesConsumed == 0)
                break;
        }
        run.AppendRun(part);
        off += (__int64)part.RawBytesConsumed;
    }
    return TRUE;
}

BOOL CViewerWindow::ReadDecodedScalar(HANDLE* hFile, __int64 offset, Salamander::Unicode::DecodedRun& scalar, BOOL& fatalErr)
{
    scalar.Clear();
    fatalErr = FALSE;
    if (!HasDecodedTextEncoding())
        return FALSE;
    offset = max(offset, TextContentOffset);
    offset = Salamander::Unicode::AlignToCodeUnit(TextEncoding, offset, TextContentOffset);
    if (offset >= FileSize)
        return TRUE;

    __int64 len = Prepare(hFile, offset, min((__int64)8, FileSize - offset), fatalErr);
    if (fatalErr || len <= 0)
        return !fatalErr;
    scalar = Salamander::Unicode::DecodeBytes(TextEncoding, Buffer + (offset - Seek), (std::size_t)len, offset, TRUE);
    return TRUE;
}

__int64 CViewerWindow::PreviousTextOffset(__int64 offset, BOOL& fatalErr)
{
    fatalErr = FALSE;
    if (!HasDecodedTextMode())
        return max((__int64)0, offset - 1);

    __int64 minSeek = TextStartOffset();
    offset = max(min(offset, FileSize), minSeek);
    if (offset <= minSeek)
        return minSeek;

    __int64 windowStart = max(minSeek, offset - 4096);
    windowStart = Salamander::Unicode::AlignToCodeUnit(TextEncoding, windowStart, TextContentOffset);
    if (TextEncoding == Salamander::Unicode::BomEncoding::Utf8 && windowStart > minSeek)
    {
        while (windowStart < offset)
        {
            __int64 len = Prepare(NULL, windowStart, 1, fatalErr);
            if (fatalErr || len != 1)
                return minSeek;
            if ((*(Buffer + (windowStart - Seek)) & 0xC0) != 0x80)
                break;
            windowStart++;
        }
    }

    Salamander::Unicode::DecodedRun run;
    if (!DecodeTextRange(NULL, windowStart, offset, run, fatalErr, TRUE) || fatalErr)
        return minSeek;
    Salamander::Unicode::TextElementMap elements = Salamander::Unicode::BuildTextElementMap(run);
    for (std::size_t element = elements.Count(); element-- > 0;)
    {
        std::size_t first = elements.CellStart(element);
        if (run.RawStart[first] < offset)
            return run.RawStart[first];
    }
    return windowStart;
}

__int64 CViewerWindow::NextTextOffset(__int64 offset, BOOL& fatalErr)
{
    fatalErr = FALSE;
    if (!HasDecodedTextMode())
        return min(FileSize, offset + 1);

    offset = max(min(offset, FileSize), TextStartOffset());
    if (offset >= FileSize)
        return FileSize;

    __int64 minSeek = TextStartOffset();
    __int64 windowStart = max(minSeek, offset - 4096);
    windowStart = Salamander::Unicode::AlignToCodeUnit(TextEncoding, windowStart, TextContentOffset);
    if (TextEncoding == Salamander::Unicode::BomEncoding::Utf8 && windowStart > minSeek)
    {
        while (windowStart < offset)
        {
            __int64 len = Prepare(NULL, windowStart, 1, fatalErr);
            if (fatalErr || len != 1)
                return offset;
            if ((*(Buffer + (windowStart - Seek)) & 0xC0) != 0x80)
                break;
            windowStart++;
        }
    }

    Salamander::Unicode::DecodedRun run;
    __int64 windowEnd = min(FileSize, offset + 4096);
    if (!DecodeTextRange(NULL, windowStart, windowEnd, run, fatalErr, windowEnd >= FileSize) || fatalErr)
        return offset;
    Salamander::Unicode::TextElementMap elements = Salamander::Unicode::BuildTextElementMap(run);
    for (std::size_t element = 0; element < elements.Count(); ++element)
    {
        std::size_t first = elements.CellStart(element);
        std::size_t last = elements.CellEnd(element) - 1;
        if (offset <= run.RawStart[first] || offset < run.RawEnd[last])
            return min(FileSize, run.RawEnd[last]);
    }
    return min(FileSize, windowEnd);
}

BOOL CViewerWindow::ReadDecodedTextLine(HANDLE* hFile, __int64 lineOffset, __int64 maxCells,
                                        Salamander::Unicode::DecodedRun& visualLine, __int64& lineEnd,
                                        __int64& nextLineBegin, BOOL& eol, BOOL& wrapped,
                                        int& eolBytes, BOOL& fatalErr)
{
    visualLine.Clear();
    fatalErr = FALSE;
    eol = FALSE;
    wrapped = FALSE;
    eolBytes = 0;

    if (!HasDecodedTextEncoding())
        return FALSE;

    __int64 off = max(lineOffset, TextContentOffset);
    off = Salamander::Unicode::AlignToCodeUnit(TextEncoding, off, TextContentOffset);
    lineEnd = off;
    nextLineBegin = off;
    if (off >= FileSize)
        return TRUE;

    while (off < FileSize)
    {
        __int64 readEnd = min(FileSize, off + APROX_LINE_LEN + 8);
        Salamander::Unicode::DecodedRun decoded;
        if (!DecodeTextRange(hFile, off, readEnd, decoded, fatalErr, readEnd >= FileSize))
            return FALSE;
        if (fatalErr)
            return FALSE;
        if (decoded.CellCount() == 0)
        {
            lineEnd = nextLineBegin = off;
            return TRUE;
        }

        bool foundEol = false;
        for (std::size_t i = 0; i < decoded.CellCount(); ++i)
        {
            std::uint32_t scalar = decoded.Scalars[i];
            if (IsViewerDecodedEOL(scalar))
            {
                if (scalar == L'\r')
                {
                    if (Configuration.EOL_CRLF)
                    {
                        Salamander::Unicode::DecodedRun nextScalar;
                        bool haveNext = false;
                        if (i + 1 < decoded.CellCount())
                        {
                            nextScalar.AppendCell(decoded.Scalars[i + 1], decoded.RawStart[i + 1], decoded.RawEnd[i + 1]);
                            haveNext = true;
                        }
                        else if (ReadDecodedScalar(hFile, decoded.RawEnd[i], nextScalar, fatalErr) && !fatalErr &&
                                 nextScalar.CellCount() > 0)
                            haveNext = true;
                        if (fatalErr)
                            return FALSE;
                        if (haveNext && nextScalar.Scalars[0] == L'\n')
                        {
                            lineEnd = decoded.RawStart[i];
                            nextLineBegin = nextScalar.RawEnd[0];
                            eol = TRUE;
                            eolBytes = (int)(nextLineBegin - lineEnd);
                            foundEol = true;
                            break;
                        }
                    }
                    if (Configuration.EOL_CR)
                    {
                        lineEnd = decoded.RawStart[i];
                        nextLineBegin = decoded.RawEnd[i];
                        eol = TRUE;
                        eolBytes = (int)(nextLineBegin - lineEnd);
                        foundEol = true;
                        break;
                    }
                }
                else if (scalar == L'\n')
                {
                    if (Configuration.EOL_LF)
                    {
                        lineEnd = decoded.RawStart[i];
                        nextLineBegin = decoded.RawEnd[i];
                        eol = TRUE;
                        eolBytes = (int)(nextLineBegin - lineEnd);
                        foundEol = true;
                        break;
                    }
                }
                else if (Configuration.EOL_NULL)
                {
                    lineEnd = decoded.RawStart[i];
                    nextLineBegin = decoded.RawEnd[i];
                    eol = TRUE;
                    eolBytes = (int)(nextLineBegin - lineEnd);
                    foundEol = true;
                    break;
                }
            }

            AppendVisualCell(visualLine, scalar, decoded.RawStart[i], decoded.RawEnd[i], Configuration.TabSize);
            lineEnd = decoded.RawEnd[i];
            nextLineBegin = lineEnd;
        }

        Salamander::Unicode::TextElementMap elements = Salamander::Unicode::BuildTextElementMap(visualLine);
        if (WrapText && maxCells > 0 && (__int64)elements.Count() > maxCells)
        {
            std::size_t cellLimit = elements.CellStart((std::size_t)maxCells);
            // Expanded tab cells share one raw range.  Never split that range
            // between visual rows; doing so would display part of the tab and
            // decode the same source byte again on the following row.
            while (cellLimit > 0 && cellLimit < visualLine.CellCount() &&
                   visualLine.RawStart[cellLimit] == visualLine.RawStart[cellLimit - 1] &&
                   visualLine.RawEnd[cellLimit] == visualLine.RawEnd[cellLimit - 1])
                cellLimit++;
            if (cellLimit == visualLine.CellCount() && foundEol)
                return TRUE;
            __int64 wrapOffset = cellLimit < visualLine.CellCount() ? visualLine.RawStart[cellLimit] :
                                                                     visualLine.RawEnd[cellLimit - 1];
            Salamander::Unicode::DecodedRun prefix;
            for (std::size_t cell = 0; cell < cellLimit; ++cell)
                prefix.AppendCell(visualLine.Scalars[cell], visualLine.RawStart[cell], visualLine.RawEnd[cell]);
            visualLine = std::move(prefix);
            lineEnd = nextLineBegin = wrapOffset;
            eol = FALSE;
            eolBytes = 0;
            wrapped = TRUE;
            return TRUE;
        }
        if (foundEol)
            return TRUE;

        off += (__int64)decoded.RawBytesConsumed;
        if (decoded.RawBytesConsumed == 0)
            break;
    }
    lineEnd = nextLineBegin = max(lineEnd, off);
    return TRUE;
}

void CViewerWindow::PaintDecodedText(HDC dc, const RECT& fullLine, int lines, int columns,
                                     int clipFirstRow, int clipLastRow, BOOL& fatalErr,
                                     BOOL& setFindOffset)
{
    __int64 xRollLimit = (Width - GetTextLeft()) / CharWidth / 6;
    FirstLineSize = LastLineSize = 0;
    WrapIsBeforeFirstLine = FALSE;

    RECT r;
    RECT endRect = fullLine;
    __int64 lineOffset = max(SeekY, TextContentOffset);
    BOOL previousEOL = FALSE;
    for (int i = 0; i < lines; i++)
    {
        Salamander::Unicode::DecodedRun visual;
        __int64 lineEnd = lineOffset;
        __int64 nextLineBegin = lineOffset;
        BOOL EOL = FALSE;
        BOOL lineEndIsWrapped = FALSE;
        int lineEOLSize = 0;

        if (lineOffset >= FileSize)
        {
            int redrI = i;
            if (redrI < clipFirstRow)
                redrI = clipFirstRow;
            r.left = GetTextLeft();
            r.right = Width;
            r.top = CharHeight * redrI;
            r.bottom = CharHeight * clipLastRow;
            if (r.bottom > Height)
                r.bottom = Height;
            if (r.top <= r.bottom)
                FillRect(dc, &r, BkgndBrush);

            if (previousEOL)
            {
                LineOffset.Add(lineOffset);
                LineOffset.Add(lineOffset);
                LineOffset.Add(0);
            }
            break;
        }

        __int64 maxCells = WrapText ? max(1, columns) : TEXT_MAX_LINE_LEN + 1;
        if (!ReadDecodedTextLine(NULL, lineOffset, maxCells, visual, lineEnd, nextLineBegin,
                                 EOL, lineEndIsWrapped, lineEOLSize, fatalErr))
            break;
        if (fatalErr)
            break;

        Salamander::Unicode::TextElementMap elements = Salamander::Unicode::BuildTextElementMap(visual);
        __int64 fullLineLen = max((__int64)0, nextLineBegin - lineOffset);
        __int64 lineLen = (__int64)elements.Count();
        LineOffset.Add(lineOffset);
        LineOffset.Add(lineEnd);
        LineOffset.Add(lineLen);

        __int64 startSel = min(StartSelection, EndSelection);
        if (startSel == -1)
            startSel = 0;
        __int64 endSel = max(StartSelection, EndSelection);
        if (endSel == -1)
            endSel = 0;
        if (startSel == endSel)
            startSel = endSel = 0;

        std::size_t selStartCell = DecodedSelectionStartElement(visual, elements, startSel);
        std::size_t selEndCell = DecodedSelectionEndElement(visual, elements, endSel);

        if (ScrollToSelection)
        {
            int len2 = (Width - GetTextLeft()) / CharWidth;
            if (len2 - 2 * xRollLimit < (__int64)selEndCell - (__int64)selStartCell)
            {
                xRollLimit = (len2 - ((__int64)selEndCell - (__int64)selStartCell)) / 2;
                if (xRollLimit < 0)
                    xRollLimit = 0;
            }
            __int64 left = OriginX;
            __int64 right = OriginX + len2;
            if ((__int64)selStartCell < lineLen)
            {
                __int64 originX = OriginX;
                if ((__int64)selStartCell < left)
                {
                    originX = (__int64)selStartCell - xRollLimit;
                    if (originX < 0)
                        originX = 0;
                }
                else if ((__int64)selStartCell >= right ||
                         ((__int64)selEndCell < lineLen && (__int64)selEndCell >= right))
                {
                    originX = (__int64)selStartCell - xRollLimit;
                    if (originX < 0)
                        originX = 0;
                    __int64 originX2 = (__int64)selEndCell - len2 + 1 + xRollLimit;
                    if (originX2 < 0)
                        originX2 = 0;
                    originX = min(originX, originX2);
                }
                if (originX != OriginX)
                {
                    setFindOffset = FALSE;
                    OriginX = originX;
                    InvalidateRect(HWindow, NULL, FALSE);
                    break;
                }
                else
                    ScrollToSelection = FALSE;
            }
        }

        if (i == 0)
            FirstLineSize = fullLineLen;
        if (i + 1 < lines)
        {
            ViewSize += fullLineLen;
            if (i + 2 == lines)
                LastLineSize = fullLineLen;
        }

        BOOL blackEnd = (lineEndIsWrapped ? startSel < lineEnd : startSel <= lineEnd) && endSel > lineEnd;
        if (OriginX < lineLen)
        {
            __int64 len2 = min((Width - GetTextLeft()) / CharWidth + 1, lineLen - OriginX);
            std::size_t left = (std::size_t)OriginX;
            std::size_t right = (std::size_t)(OriginX + len2);
            std::size_t u1 = (std::size_t)len2, u2 = 0, u3 = 0;
            if (selStartCell <= left)
            {
                if (selEndCell > left)
                {
                    u1 = 0;
                    u2 = min(right, selEndCell) - left;
                    u3 = (std::size_t)len2 - u2;
                }
            }
            else if (selStartCell < right)
            {
                if (selEndCell > selStartCell)
                {
                    u1 = selStartCell - left;
                    u2 = min(right, selEndCell) - left - u1;
                    u3 = (std::size_t)len2 - u2 - u1;
                }
            }

            if (i >= clipFirstRow && i <= clipLastRow)
            {
                RECT myLine = fullLine;
                // Decoded text can contain glyphs that are wider than one
                // average character cell (for example CJK ideographs).  Do
                // not clip the blit to the scalar count; otherwise the last
                // visible glyph can be cut in half or the tail of the line can
                // disappear even though it was drawn into the memory bitmap.
                myLine.right = fullLine.right;

                std::size_t visibleEnd = left + u1 + u2 + u3;
                std::vector<DecodedPixelInterval> selectedIntervals = DecodedSelectionPixelIntervals(
                    Bitmap.HMemDC, visual, elements, left, visibleEnd, left + u1, left + u1 + u2);
                FillRect(Bitmap.HMemDC, &myLine, BkgndBrush);
                for (const DecodedPixelInterval& interval : selectedIntervals)
                {
                    RECT selectedRect = fullLine;
                    selectedRect.left = interval.Left;
                    selectedRect.right = interval.Right;
                    FillRect(Bitmap.HMemDC, &selectedRect, BkgndBrushSel);
                }
                if (blackEnd)
                {
                    // Selection continues past the end of this visual row.
                    // Highlight the empty row tail after the shaped text.  The
                    // selected glyphs themselves are handled as visual bidi
                    // intervals above.
                    endRect.left = DecodedElementsPixelWidth(Bitmap.HMemDC, visual, elements, left, visibleEnd);
                    endRect.right = Width - GetTextLeft();
                    if (endRect.left < endRect.right)
                        FillRect(Bitmap.HMemDC, &endRect, BkgndBrushSel);
                }

                // Draw the complete visible decoded text run in one piece.
                // Splitting Unicode text into selected/non-selected substrings
                // can change shaping or fallback rendering for CJK, Indic, RTL,
                // emoji sequences, etc.  For the selected part, clip a second
                // full-run draw to the selection rectangle so glyph context is
                // preserved while colors still differ.
                if (visibleEnd > left)
                    DrawDecodedElements(Bitmap.HMemDC, visual, elements, left, visibleEnd, 0);
                if (!selectedIntervals.empty())
                {
                    for (const DecodedPixelInterval& interval : selectedIntervals)
                    {
                        int savedDC = SaveDC(Bitmap.HMemDC);
                        IntersectClipRect(Bitmap.HMemDC, interval.Left, fullLine.top, interval.Right, fullLine.bottom);
                        SetTextColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_FG_SELECTED]));
                        DrawDecodedElements(Bitmap.HMemDC, visual, elements, left, visibleEnd, 0);
                        RestoreDC(Bitmap.HMemDC, savedDC);
                    }
                    SetTextColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_FG_NORMAL]));
                }

                BitBlt(dc, GetTextLeft(), CharHeight * i, myLine.right,
                       CharHeight, Bitmap.HMemDC, 0, 0, SRCCOPY);

                if (myLine.right < fullLine.right)
                {
                    myLine.top = CharHeight * i;
                    myLine.bottom = myLine.top + CharHeight;
                    myLine.left = GetTextLeft() + myLine.right;
                    myLine.right = GetTextLeft() + fullLine.right;
                    FillRect(dc, &myLine, blackEnd ? BkgndBrushSel : BkgndBrush);
                }
            }
        }
        else if (i >= clipFirstRow && i <= clipLastRow)
        {
            r.left = GetTextLeft();
            r.top = CharHeight * i;
            r.right = Width;
            r.bottom = r.top + CharHeight;
            FillRect(dc, &r, blackEnd ? BkgndBrushSel : BkgndBrush);
        }

        previousEOL = EOL;
        if (nextLineBegin <= lineOffset)
            break;
        lineOffset = nextLineBegin;
    }
}


void CViewerWindow::Paint(HDC dc)
{
    CALL_STACK_MESSAGE1("CViewerWindow::Paint()");
    if (EnablePaint && !ExitTextMode && FileName != NULL && Width > 0 && Height > 0)
    {
        //    HCURSOR oldCursor = GetCursor();
        //    SetCursor(LoadCursor(NULL, IDC_WAIT));
        //---
        HFONT oldFont = (HFONT)SelectObject(dc, ViewerFont);
        SetTextColor(dc, GetCOLORREF(ViewerColors[VIEWER_FG_NORMAL]));
        SetBkColor(dc, GetCOLORREF(ViewerColors[VIEWER_BK_NORMAL]));
        //---
        HFONT oldFont2 = (HFONT)SelectObject(Bitmap.HMemDC, ViewerFont);
        SetTextColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_FG_NORMAL]));
        SetBkColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_BK_NORMAL]));
        //---
        int oldMode = SetBkMode(Bitmap.HMemDC, TRANSPARENT);

        EnablePaint = FALSE;
        // Calculate digit count from the actual total line count, cached
        // per file.  For hex mode the count is trivial; for text mode
        // GetDocumentLineNumber scans the file once and the result is
        // stored in CachedTotalLines so subsequent paints are free.
        // When line numbers are not shown, skip the expensive scan since
        // GetTextLeft() does not depend on LineNumberDigits.
        if (ShowLineNumbers && CachedTotalLines < 0)
        {
            if (Type == vtHex)
                CachedTotalLines = max((__int64)1, (FileSize + 15) / 16);
            else if (FileName != NULL && FileSize > TextStartOffset())
                CachedTotalLines = GetDocumentLineNumber(FileSize);
            else
                CachedTotalLines = 1;
        }
        int prevLineNumberDigits = LineNumberDigits;
        if (ShowLineNumbers)
        {
            LineNumberDigits = 1;
            for (__int64 n = max((__int64)1, CachedTotalLines); n >= 10; n /= 10)
                ++LineNumberDigits;
        }
        else
        {
            LineNumberDigits = 1; // GetTextLeft() ignores this when !ShowLineNumbers
        }
        // When the real gutter width differs from what HeightChanged()
        // used during wrapping (which assumed LineNumberDigits == 1 at
        // file-open time), recompute the layout so wrapped lines and
        // the scrollbar range reflect the correct text area width.
        if (ShowLineNumbers && FileName != NULL && Type == vtText && WrapText &&
            LineNumberDigits != prevLineNumberDigits)
        {
            BOOL fatalErr = FALSE;
            HeightChanged(fatalErr);
            if (!fatalErr)
            {
                FindNewSeekY(SeekY, fatalErr);
                // HeightChanged() invalidated CachedTotalLines; recompute
                // so the rest of this paint pass uses the correct value.
                if (CachedTotalLines < 0)
                {
                    if (FileSize > TextStartOffset())
                        CachedTotalLines = GetDocumentLineNumber(FileSize);
                    else
                        CachedTotalLines = 1;
                }
                LineNumberDigits = 1;
                for (__int64 n = max((__int64)1, CachedTotalLines); n >= 10; n /= 10)
                    ++LineNumberDigits;
            }
        }
        LineOffset.DestroyMembers();
        RECT r;
        r.left = 0;
        r.right = GetTextLeft();
        r.top = 0;
        r.bottom = Height;
        FillRect(dc, &r, ShowLineNumbers ? LineNumberBrush : BkgndBrush);
        RECT fullLine;
        fullLine.left = 0;
        fullLine.top = 0;
        fullLine.right = Width - GetTextLeft();
        fullLine.bottom = CharHeight /*Height*/; // j.r. W2K slowness patch

        r.right = Width;
        ViewSize = 0;
        int lines = Height / CharHeight + 1;
        int columns = (Width - GetTextLeft()) / CharWidth;
        char line[2001]; // holds at most 2000 fully visible characters per line plus 1 partially visible character
        char* s;
        BOOL fatalErr = FALSE;
        if (columns <= 2000) // only when this maximum is not exceeded
        {
            // determine which rows need to be repainted
            RECT clipRect;
            int clipRet = GetClipBox(dc, &clipRect);
            int clipFirstRow = 0;
            int clipLastRow = lines;
            if (clipRet == SIMPLEREGION || clipRet == COMPLEXREGION)
            {
                clipFirstRow = clipRect.top / CharHeight;
                clipLastRow = clipRect.bottom / CharHeight + 1;
            }

            BOOL setFindOffset = ResetFindOffsetOnNextPaint;
            switch (Type)
            {
            case vtHex:
            {
                FirstLineSize = LastLineSize = 16;
                int hexOffsetMode = GetHexOffsetMode(FileSize, HexOffsetLength);

                __int64 startSel = min(StartSelection, EndSelection);
                if (startSel == -1)
                    startSel = 0;
                __int64 endSel = max(StartSelection, EndSelection);
                if (endSel == -1)
                    endSel = 0;
                if (startSel == endSel)
                    startSel = endSel = 0;
                __int64 lineOffset = SeekY;
                int i;
                for (i = 0; i < lines; i++)
                {
                    __int64 len = Prepare(NULL, lineOffset, 16, fatalErr);
                    // if (fatalErr) FatalFileErrorOccured(); // see below
                    if (fatalErr)
                        break;
                    if (len == 0 && i + 1 != lines && SeekY != 0)
                    {
                        __int64 size = FileSize;
                        FileChanged(NULL, TRUE, fatalErr, FALSE);
                        // if (fatalErr) FatalFileErrorOccured(); // see below
                        if (fatalErr || ExitTextMode)
                            break;
                        if (size != FileSize)
                        {
                            setFindOffset = FALSE; // leave it for the next drawing pass
                            ViewSize = 0;
                            FindNewSeekY(SeekY, fatalErr);
                            if (fatalErr || ExitTextMode)
                                break;
                            FirstLineSize = LastLineSize = 0;
                            // when viewing a growing text file whose last line ended with a line break,
                            // scrolling down at the end of the file caused incorrect repainting;
                            // we expect the same in HEX view, so the following invalidate ensures a full repaint
                            InvalidateRect(HWindow, NULL, FALSE);
                            break;
                        }
                    }
                    if (i + 1 != lines)
                        ViewSize += len; // count only fully visible lines

                    s = line;
                    if (len != 0)
                    {
                        PrintHexOffset(s, lineOffset, hexOffsetMode); // line offset
                        s += HexOffsetLength;
                        *s++ = ':';
                        *s++ = ' ';

                        int j;
                        for (j = 0; j < 16; j++)
                        {
                            if (j < len)
                                if ((j % 4) == 3)
                                    s += sprintf(s, "%02X  ", (unsigned int)Buffer[lineOffset - Seek + j]);
                                else
                                    s += sprintf(s, "%02X ", (unsigned int)Buffer[lineOffset - Seek + j]);
                            else if ((j % 4) == 3)
                                s += sprintf(s, "    ");
                            else
                                s += sprintf(s, "   ");
                        }
                        memmove(s, Buffer + (lineOffset - Seek), (int)len);
                        s += len;
                    }

                    int lineLen = (int)(s - line); // length of the line to print
                    if (OriginX < lineLen)
                    {
                        int u1, u2;
                        if (startSel < lineOffset + len && endSel > lineOffset)
                        {
                            u1 = (int)(lineLen - len);
                            if (startSel > lineOffset)
                                u1 += (int)(startSel - lineOffset);
                            if (endSel >= lineOffset + len)
                                u2 = lineLen;
                            else
                                u2 = (int)(lineLen - (lineOffset + len - endSel));
                        }
                        else
                        {
                            u1 = lineLen;
                            u2 = lineLen;
                        }
                        int t1, t2;
                        t1 = TranslateU2T(u1, TRUE, HexOffsetLength);
                        t2 = TranslateU2T(u2, FALSE, HexOffsetLength);
                        if (t2 < t1)
                            t2 = t1;

                        if (i >= clipFirstRow && i <= clipLastRow)
                        {
                            RECT myLine = fullLine; // myLine is the text area that we will draw with the slower BitBlt path
                            myLine.right = min(myLine.right, (lineLen + 1) * CharWidth);
                            FillRect(Bitmap.HMemDC, &myLine, BkgndBrush);
                            if (lineLen > OriginX)
                            {
                                char* text = line + OriginX; // shift the text buffer according to OriginX
                                u1 -= (int)OriginX;
                                if (u1 < 0)
                                    u1 = 0;
                                u2 -= (int)OriginX;
                                if (u2 < 0)
                                    u2 = 0;
                                t1 -= (int)OriginX;
                                if (t1 < 0)
                                    t1 = 0;
                                t2 -= (int)OriginX;
                                if (t2 < 0)
                                    t2 = 0;
                                // render text backwards because of italics
                                // u2, lineLen - OriginX norm
                                if (u2 < lineLen - OriginX)
                                {
                                    MyTextOut(Bitmap.HMemDC, u2 * CharWidth, 0, text + u2,
                                              (int)(lineLen - OriginX - u2));
                                }
                                // u1, u2 sel
                                if (u1 < u2)
                                {
                                    SetBkColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_BK_SELECTED]));
                                    SetTextColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_FG_SELECTED]));
                                    SetBkMode(Bitmap.HMemDC, OPAQUE);
                                    MyTextOut(Bitmap.HMemDC, u1 * CharWidth, 0, text + u1, u2 - u1);
                                    SetBkMode(Bitmap.HMemDC, TRANSPARENT);
                                    SetTextColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_FG_NORMAL]));
                                    SetBkColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_BK_NORMAL]));
                                }
                                // t2, u1 norm
                                if (t2 < u1)
                                {
                                    MyTextOut(Bitmap.HMemDC, t2 * CharWidth, 0, text + t2, u1 - t2);
                                }
                                // t1, t2 select
                                if (t1 < t2)
                                {
                                    SetBkColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_BK_SELECTED]));
                                    SetTextColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_FG_SELECTED]));
                                    SetBkMode(Bitmap.HMemDC, OPAQUE);
                                    MyTextOut(Bitmap.HMemDC, t1 * CharWidth, 0, text + t1, t2 - t1);
                                    SetBkMode(Bitmap.HMemDC, TRANSPARENT);
                                    SetTextColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_FG_NORMAL]));
                                    SetBkColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_BK_NORMAL]));
                                }
                                // 0, t1 norm
                                if (t1 > 0)
                                    MyTextOut(Bitmap.HMemDC, 0, 0, text, t1);
                            }

                            // bitblt the entire row to the screen
                            BitBlt(dc, GetTextLeft(), CharHeight * i, (lineLen + 1) * CharWidth, // extend the line by one character so italics fit
                                   CharHeight, Bitmap.HMemDC, 0, 0, SRCCOPY);
                            // if needed, clear the space on the right
                            if (myLine.right < fullLine.right)
                            {
                                myLine.top = CharHeight * i;
                                myLine.bottom = myLine.top + CharHeight;
                                myLine.left = GetTextLeft() + myLine.right;
                                myLine.right = GetTextLeft() + fullLine.right;
                                FillRect(dc, &myLine, BkgndBrush);
                            }
                        }
                    }

                    lineOffset += len;
                    if (lineOffset == FileSize)
                    {
                        r.left = GetTextLeft();
                        if (FileSize > 0) // JR: up to AS2.52b1 (inclusive) we rendered a 0-byte file in hex without clearing the first line (it appeared when resizing the window)
                            r.top = CharHeight * (i + 1);
                        else
                            r.top = 0;
                        r.bottom = Height;
                        if (r.top <= r.bottom)
                            FillRect(dc, &r, BkgndBrush);
                        break;
                    }
                }
                break;
            }

            case vtText:
            {
                if (HasDecodedTextMode())
                {
                    PaintDecodedText(dc, fullLine, lines, columns, clipFirstRow, clipLastRow, fatalErr, setFindOffset);
                    break;
                }
                __int64 xRollLimit = (Width - GetTextLeft()) / CharWidth / 6;
                FirstLineSize = LastLineSize = 0;

                WrapIsBeforeFirstLine = FALSE;
                if (WrapText && SeekY > 0)
                {
                    __int64 len = Prepare(NULL, SeekY - (SeekY > 1 ? 2 : 1), SeekY > 1 ? 2 : 1, fatalErr);
                    // if (fatalErr) FatalFileErrorOccured(); // see below
                    if (fatalErr)
                        break;

                    unsigned char* s = Buffer + (SeekY - Seek) - 1;
                    if (!(*s == '\n' && Configuration.EOL_LF ||
                          *s == '\r' && Configuration.EOL_CR ||
                          *s == 0 && Configuration.EOL_NULL ||
                          SeekY > 1 && *(s - 1) == '\r' && *s == '\n' && Configuration.EOL_CRLF))
                    {
                        WrapIsBeforeFirstLine = TRUE;
                    }
                }

                RECT endRect = fullLine;
                __int64 lineOffset = SeekY; // offset of the beginning of the line in bytes
                BOOL EOL = FALSE;           // TRUE if the previous line ended with an EOL (the next line exists, it may be empty)
                int lineEOLSize = 0;        // length of the EOL (CR=1, LF=1, CRLF=2, NULL=1)
                for (int i = 0; i < lines; i++)
                {
                    __int64 len = Prepare(NULL, lineOffset, APROX_LINE_LEN, fatalErr);
                    // if (fatalErr) FatalFileErrorOccured(); // see below
                    if (fatalErr)
                        break;
                    if (len == 0 && i + 1 != lines && SeekY != 0)
                    {
                        __int64 size = FileSize;
                        FileChanged(NULL, TRUE, fatalErr, FALSE);
                        // if (fatalErr) FatalFileErrorOccured(); // see below
                        if (fatalErr || ExitTextMode)
                            break;
                        if (size != FileSize)
                        {
                            setFindOffset = FALSE; // leave it for the next drawing pass
                            LineOffset.DestroyMembers();
                            ViewSize = 0;
                            FindNewSeekY(SeekY, fatalErr);
                            if (fatalErr || ExitTextMode)
                                break;
                            FirstLineSize = LastLineSize = 0;
                            // when viewing a growing text file whose last line ended with a line break,
                            // scrolling down at the end of the file caused incorrect repainting;
                            // this invalidate call ensures a full repaint
                            InvalidateRect(HWindow, NULL, FALSE);
                            break;
                        }
                    }

                    if (len == 0)
                    {
                        int redrI = i;
                        if (redrI < clipFirstRow)
                            redrI = clipFirstRow; // avoid repainting unnecessarily
                        r.left = GetTextLeft();
                        r.top = CharHeight * redrI;
                        r.bottom = CharHeight * clipLastRow;
                        if (r.bottom > Height)
                            r.bottom = Height;
                        if (r.top <= r.bottom)
                            FillRect(dc, &r, BkgndBrush);

                        if (EOL) // add the last empty line to the LineOffset array -> the line cannot be ignored
                        {
                            LineOffset.Add(lineOffset);
                            LineOffset.Add(lineOffset);
                            LineOffset.Add(0);
                        }
                        break;
                    }

                    unsigned char* st;                                               // start of the buffer with the line content
                    unsigned char* s2;                                               // processed character from the buffer with the line content
                    __int64 lineLen = 0;                                             // line length in characters (tab != 1 character)
                    BOOL lineEndIsWrapped = FALSE;                                   // is the end of the line wrapped because wrap mode is enabled?
                    __int64 fullLineLen = 0;                                         // line length in bytes
                    __int64 endX = OriginX + (Width - GetTextLeft()) / CharWidth + 1; // screen edge offset in characters
                    BOOL onlyOne = (len == 1);                                       // last character of the file?
                    __int64 startSel = min(StartSelection, EndSelection);            // selection start - offset in bytes
                    if (startSel == -1)
                        startSel = 0;
                    __int64 endSel = max(StartSelection, EndSelection); // selection end - offset in bytes
                    if (endSel == -1)
                        endSel = 0;
                    if (startSel == endSel)
                        startSel = endSel = 0;
                    BOOL startSelDone = startSel <= lineOffset; // the selection start has already been drawn
                    BOOL endSelDone = endSel < lineOffset;      // the selection end has already been drawn

                    __int64 tabSpaces = 0; // shift caused by tabs
                    EOL = FALSE;
                    lineEOLSize = 0;
                    while (len != 0)
                    {
                        st = s2 = Buffer + (lineOffset + fullLineLen - Seek);
                        while (len--)
                        {
                            if (*s2 == '\r') // 'end of line \r' or '\r\n'
                            {
                                BOOL ok = FALSE;
                                if (len > 0)
                                {
                                    if (*(s2 + 1) == '\n' && Configuration.EOL_CRLF)
                                    {
                                        s2 += 2; // '\r\n'
                                        len--;
                                        ok = TRUE;
                                        EOL = TRUE;
                                        lineEOLSize = 2;
                                    }
                                    else if (Configuration.EOL_CR)
                                    {
                                        ok = TRUE;
                                        s2++; // '\r'
                                        EOL = TRUE;
                                        lineEOLSize = 1;
                                    }
                                }
                                else
                                {
                                    if (!onlyOne)
                                    { // the line has not ended yet; there may be '\n' beyond the boundary
                                        if (Configuration.EOL_CRLF)
                                        {
                                            len = -1;
                                            break;
                                        }
                                        else
                                        {
                                            if (Configuration.EOL_CR)
                                            {
                                                ok = TRUE;
                                                s2++; // '\r'
                                                EOL = TRUE;
                                                lineEOLSize = 1;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if (Configuration.EOL_CR)
                                        {
                                            ok = TRUE;
                                            s2++; // the last character of the file is '\r'
                                            EOL = TRUE;
                                            lineEOLSize = 1;
                                        }
                                    }
                                }

                                if (ok)
                                    break;
                                else
                                    goto COMMON_CHAR;
                            }
                            else
                            {
                                if (*s2 == '\n' || *s2 == 0) // end of line '\n' or 0
                                {
                                    if ((*s2 == '\n') ? Configuration.EOL_LF : Configuration.EOL_NULL)
                                    {
                                        s2++;
                                        EOL = TRUE;
                                        lineEOLSize = 1;
                                        break;
                                    }
                                    else
                                        goto COMMON_CHAR;
                                }
                                else
                                {
                                    if (*s2 == '\t')
                                    {
                                        __int64 curOff = lineOffset + fullLineLen + (s2 - st);
                                        if (!startSelDone && startSel <= curOff)
                                        {
                                            startSelDone = TRUE;
                                            startSel += tabSpaces;
                                        }
                                        if (!endSelDone && endSel <= curOff)
                                        {
                                            endSelDone = TRUE;
                                            endSel += tabSpaces;
                                        }
                                        int tab = (int)(Configuration.TabSize - (lineLen % Configuration.TabSize));
                                        if (WrapText && lineLen + tab - 1 >= columns)
                                        {
                                            tab = (int)max(1, columns - lineLen); // at most to the edge, at least 1 character
                                        }
                                        tabSpaces += tab - 1;
                                        if (lineLen > OriginX - tab && endX > lineLen)
                                            memset(line + max(0, lineLen - OriginX), ' ', min(tab, (int)(endX - lineLen)));

                                        lineLen += tab - 1;
                                    }
                                    else
                                    {
                                    COMMON_CHAR:

                                        if (lineLen >= OriginX && lineLen < endX)
                                            line[lineLen - OriginX] = *s2;
                                    }
                                }
                            }
                            if (WrapText && lineLen >= columns)
                            {
#ifdef _DEBUG
                                if (lineLen > columns)
                                    TRACE_E("something's wrong");
#endif // _DEBUG
                                lineEndIsWrapped = TRUE;
                                break; // premature end of line
                            }
                            s2++;
                            lineLen++;
                        }
                        fullLineLen += s2 - st;

                        // test the text line length (over 10000 characters we offer HEX mode)
                        if (CanSwitchToHex && !ForceTextMode && fullLineLen > TEXT_MAX_LINE_LEN)
                        {
                            if (!CanSwitchQuietlyToHex)
                                CanSwitchToHex = FALSE;
                            if (CanSwitchQuietlyToHex ||
                                SalMessageBox(HWindow, LoadStr(IDS_VIEWER_BINFILE), LoadStr(IDS_VIEWERTITLE),
                                              MB_YESNO | MB_ICONQUESTION) == IDYES)
                            {
                                CanSwitchQuietlyToHex = FALSE;
                                ExitTextMode = TRUE;
                                PostMessage(HWindow, WM_COMMAND, CM_TO_HEX, 0);
                                break;
                            }
                            else
                            {
                                ForceTextMode = TRUE;
                            }
                        }

                        if (len == -1) // the line continues into a yet unread section
                        {
                            len = Prepare(NULL, lineOffset + fullLineLen, APROX_LINE_LEN, fatalErr);
                            // if (fatalErr) FatalFileErrorOccured(); // see below
                            if (fatalErr)
                                break;
                            onlyOne = (len == 1);
                        }
                        else
                            break; // the entire line has been loaded
                    }
                    if (fatalErr || ExitTextMode)
                        break;
                    LineOffset.Add(lineOffset);                             // line start offset
                    LineOffset.Add(lineOffset + fullLineLen - lineEOLSize); // line end offset (before EOL)
                    LineOffset.Add(lineLen);                                // line length in displayed characters (TAB is more characters)
                    if (!startSelDone)
                        startSel += tabSpaces;
                    if (!endSelDone)
                        endSel += tabSpaces;

                    if (ScrollToSelection)
                    {
                        int len2 = (Width - GetTextLeft()) / CharWidth;
                        if (len2 - 2 * xRollLimit < endSel - startSel)
                        { // try to show as much of long strings as possible, so set xRollLimit -> 0
                            xRollLimit = (len2 - (endSel - startSel)) / 2;
                            if (xRollLimit < 0)
                                xRollLimit = 0;
                        }
                        __int64 left = lineOffset + OriginX;
                        __int64 right = lineOffset + OriginX + len2;
                        if (startSel < lineOffset + lineLen)
                        {
                            __int64 originX = OriginX;
                            if (startSel < left) // the view is too far to the right
                            {
                                originX = startSel - lineOffset - xRollLimit;
                                if (originX < 0)
                                    originX = 0;
                            }
                            else
                            {
                                if (startSel >= right ||
                                    endSel < lineOffset + lineLen && endSel >= right)
                                { // the view is too far to the left
                                    originX = startSel - lineOffset - xRollLimit;
                                    if (originX < 0)
                                        originX = 0;
                                    __int64 originX2 = endSel - lineOffset - len2 + 1 + xRollLimit;
                                    if (originX2 < 0)
                                        originX2 = 0;
                                    originX = min(originX, originX2);
                                }
                            }
                            if (originX != OriginX) // there was a change
                            {
                                setFindOffset = FALSE; // resetting FindOffset probably is not needed here, but if it is, we will do it during the next drawing pass
                                OriginX = originX;
                                InvalidateRect(HWindow, NULL, FALSE);
                                break; // we need to start over
                            }
                            else
                                ScrollToSelection = FALSE; // not necessary
                        }
                    }

                    if (i == 0)
                        FirstLineSize = fullLineLen;
                    if (i + 1 < lines)
                    {
                        ViewSize += fullLineLen; // count only fully visible lines
                        if (i + 2 == lines)
                            LastLineSize = fullLineLen;
                    }

                    BOOL blackEnd;
                    if (OriginX < lineLen)
                    {
                        __int64 len2 = min((Width - GetTextLeft()) / CharWidth + 1, lineLen - OriginX);
                        __int64 left = lineOffset + OriginX;
                        __int64 right = lineOffset + OriginX + len2;
                        __int64 u1 = len2, u2 = 0, u3 = 0;
                        blackEnd = (lineEndIsWrapped ? startSel < right : startSel <= right) && endSel > right;
                        if (startSel <= left)
                        {
                            if (endSel > left)
                            {
                                u1 = 0;
                                u2 = min(right, endSel) - left;
                                u3 = len2 - u2;
                            }
                        }
                        else if (startSel < right)
                        {
                            if (endSel > startSel)
                            {
                                u1 = startSel - left;
                                u2 = min(right, endSel) - left - u1;
                                u3 = len2 - u2 - u1;
                            }
                        }

                        if (i >= clipFirstRow && i <= clipLastRow)
                        {
                            RECT myLine = fullLine;                                        // myLine is the text area that we will draw with the slower BitBlt path
                            myLine.right = min(myLine.right, (int)(len2 + 1) * CharWidth); // allow one extra character so italics fit

                            if (blackEnd)
                            {
                                endRect.left = 0;
                                endRect.right = (int)((u1 + u2 + u3) * CharWidth);
                                FillRect(Bitmap.HMemDC, &endRect, BkgndBrush);
                                endRect.left = endRect.right;
                                endRect.right = Width - GetTextLeft();
                                FillRect(Bitmap.HMemDC, &endRect, BkgndBrushSel);
                            }
                            else
                                FillRect(Bitmap.HMemDC, &myLine, BkgndBrush);

                            if (lineLen > OriginX)
                            { // output text to Bitmap.HMemDC
                                if (u3 > 0)
                                    MyTextOut(Bitmap.HMemDC, (int)((u1 + u2) * CharWidth), 0, line + u1 + u2, (int)u3);
                                if (u2 > 0)
                                {
                                    SetBkColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_BK_SELECTED]));
                                    SetTextColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_FG_SELECTED]));
                                    SetBkMode(Bitmap.HMemDC, OPAQUE);
                                    MyTextOut(Bitmap.HMemDC, (int)(u1 * CharWidth), 0, line + u1, (int)u2);
                                    SetBkMode(Bitmap.HMemDC, TRANSPARENT);
                                    SetTextColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_FG_NORMAL]));
                                    SetBkColor(Bitmap.HMemDC, GetCOLORREF(ViewerColors[VIEWER_BK_NORMAL]));
                                }
                                if (u1 > 0)
                                    MyTextOut(Bitmap.HMemDC, 0, 0, line, (int)u1);
                            }

                            // bitblt the entire row to the screen
                            BitBlt(dc, GetTextLeft(), CharHeight * i, myLine.right,
                                   CharHeight, Bitmap.HMemDC, 0, 0, SRCCOPY);

                            // if needed, clear the space on the right
                            if (myLine.right < fullLine.right)
                            {
                                myLine.top = CharHeight * i;
                                myLine.bottom = myLine.top + CharHeight;
                                myLine.left = GetTextLeft() + myLine.right;
                                myLine.right = GetTextLeft() + fullLine.right;
                                FillRect(dc, &myLine, blackEnd ? BkgndBrushSel : BkgndBrush);
                            }
                        }
                    }
                    else
                    {
                        if (i >= clipFirstRow && i <= clipLastRow)
                        {
                            blackEnd = (lineEndIsWrapped ? startSel < lineOffset + lineLen : startSel <= lineOffset + lineLen) &&
                                       endSel > lineOffset + lineLen;
                            r.left = GetTextLeft();
                            r.top = CharHeight * i;
                            r.right = Width;
                            r.bottom = r.top + CharHeight;
                            FillRect(dc, &r, blackEnd ? BkgndBrushSel : BkgndBrush);
                        }
                    }
                    lineOffset += fullLineLen;
                }
                break;
            }
            }
            if (setFindOffset && !fatalErr && !ExitTextMode)
            {
                ResetFindOffsetOnNextPaint = FALSE;
                FindOffset = SeekY;
                if (!FindDialog.Forward)
                    FindOffset += ViewSize;
            }
        }
        EnablePaint = TRUE;
        ScrollToSelection = FALSE;
        SetBkMode(Bitmap.HMemDC, oldMode);
        __int64 documentLine = 1;
        if ((ShowLineNumbers || ShowStatusBar) && LineOffset.Count >= 3)
            documentLine = GetDocumentLineNumber(LineOffset[0]);
        VisibleFirstDocumentLine = documentLine;
        if (ShowLineNumbers)
        {
            // Keep the gutter visibly distinct from document text in both
            // standard light schemes and Windows Dark Mode.
            SetTextColor(dc, DarkModeShouldUseDarkColors() ? RGB(160, 160, 160) : RGB(96, 96, 96));
            int gutterBkMode = SetBkMode(dc, TRANSPARENT);
            for (int i = 0; i < LineOffset.Count / 3; i++)
            {
                BOOL isWrap = (i > 0 && LineOffset[i * 3] <= LineOffset[i * 3 - 2]);
                if (!isWrap && i > 0)
                    ++documentLine;
                RECT numberRect = {0, i * CharHeight, GetTextLeft() - BORDER_WIDTH, (i + 1) * CharHeight};
                if (isWrap)
                {
                    // Show a rightwards arrow with hook for wrapped lines
                    // instead of repeating the line number.
                    DrawTextW(dc, L"\x21AA", -1, &numberRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                }
                else
                {
                    char number[32];
                    sprintf(number, "%I64d", documentLine);
                    DrawText(dc, number, -1, &numberRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                }
            }
            SetBkMode(dc, gutterBkMode);
        }
        //---
        SelectObject(dc, oldFont);
        SelectObject(Bitmap.HMemDC, oldFont2);
        //---
        if (fatalErr)
            FatalFileErrorOccured();
        if (fatalErr || ExitTextMode)
            return;
        SetScrollBar();
        //    SetCursor(oldCursor);
    }
    else // at least clear the screen
    {
        RECT r;
        r.left = 0;
        r.right = Width;
        r.top = 0;
        r.bottom = Height;
        FillRect(dc, &r, BkgndBrush); // clear the column to the left of the text
        SetScrollBar();
    }
}

//
// ****************************************************************************

BOOL CViewerWindow::CreateViewerBrushs()
{
    BkgndBrush = HANDLES(CreateSolidBrush(GetCOLORREF(ViewerColors[VIEWER_BK_NORMAL])));
    if (BkgndBrush == NULL)
    {
        TRACE_E("Unable to create window background brush.");
        return FALSE;
    }
    BkgndBrushSel = HANDLES(CreateSolidBrush(GetCOLORREF(ViewerColors[VIEWER_BK_SELECTED])));
    if (BkgndBrushSel == NULL)
    {
        TRACE_E("Unable to create window selected text background brush.");
        return FALSE;
    }
    const COLORREF gutterBackground = DarkModeShouldUseDarkColors() ? RGB(38, 38, 38) : RGB(245, 245, 245);
    LineNumberBrush = HANDLES(CreateSolidBrush(gutterBackground));
    if (LineNumberBrush == NULL)
    {
        TRACE_E("Unable to create line-number gutter background brush.");
        return FALSE;
    }
    return TRUE;
}

void UpdateViewerColors(SALCOLOR* colors)
{
    if (GetFValue(colors[VIEWER_FG_NORMAL]) & SCF_DEFAULT)
        SetRGBPart(&colors[VIEWER_FG_NORMAL], GetSysColor(COLOR_WINDOWTEXT));
    if (GetFValue(colors[VIEWER_BK_NORMAL]) & SCF_DEFAULT)
        SetRGBPart(&colors[VIEWER_BK_NORMAL], GetSysColor(COLOR_WINDOW));
    if (GetFValue(colors[VIEWER_FG_SELECTED]) & SCF_DEFAULT)
        SetRGBPart(&colors[VIEWER_FG_SELECTED], GetSysColor(COLOR_WINDOW));
    if (GetFValue(colors[VIEWER_BK_SELECTED]) & SCF_DEFAULT)
        SetRGBPart(&colors[VIEWER_BK_SELECTED], GetSysColor(COLOR_WINDOWTEXT));
}

BOOL InitializeViewer()
{
    int i;
    for (i = 0; i < VIEWER_HISTORY_SIZE; i++)
        ViewerHistory[i] = NULL;

    GetDefaultViewerLogFont(&ViewerLogFont);

    HANDLES(InitializeCriticalSection(&ViewerFontMeasureCS));

    UpdateViewerColors(ViewerColors);
    ViewerMenu = LoadMenu(HLanguage, MAKEINTRESOURCE(IDM_VIEWERMENU));
    if (ViewerMenu == NULL)
    {
        TRACE_E("Unable to load menu for viewer.");
        return FALSE;
    }
    MENUITEMINFO mi;
    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_TYPE | MIIM_SUBMENU;
    mi.fType = MFT_STRING;
    mi.hSubMenu = CreatePopupMenu();
    mi.dwTypeData = LoadStr(IDS_VIEWERCODINGMENU);
    InsertMenuItem(ViewerMenu, CODING_MENU_INDEX, TRUE, &mi);

    ViewerTable = HANDLES(LoadAccelerators(HInstance, MAKEINTRESOURCE(IDA_VIEWERACCELS)));
    if (ViewerTable == NULL)
    {
        TRACE_E("Unable to load accelerators for viewer.");
        return FALSE;
    }

#ifndef _UNICODE
    if (!CViewerWindow::RegisterUniversalClassW(CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
                                                0,
                                                0,
                                                HANDLES(LoadIcon(HInstance,
                                                                 MAKEINTRESOURCE(IDI_VIEWER))),
                                                LoadCursor(NULL, IDC_ARROW),
                                                (HBRUSH)(COLOR_WINDOW + 1),
                                                NULL,
                                                CVIEWERWINDOW_CLASSNAMEW,
                                                NULL))
#else  // _UNICODE
    if (!CViewerWindow::RegisterUniversalClass(CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
                                               0,
                                               0,
                                               HANDLES(LoadIcon(HInstance,
                                                                MAKEINTRESOURCE(IDI_VIEWER))),
                                               LoadCursor(NULL, IDC_ARROW),
                                               (HBRUSH)(COLOR_WINDOW + 1),
                                               NULL,
                                               CVIEWERWINDOW_CLASSNAMEW,
                                               NULL))
#endif // _UNICODE
    {
        TRACE_E("Unable to register window class for viewer.");
        return FALSE;
    }

    ViewerContinue = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    if (ViewerContinue == NULL)
    {
        TRACE_E("Unable to create ViewerContinue event.");
        return FALSE;
    }

    return TRUE;
}

void CViewerWindow::ReleaseViewerBrushs()
{
    if (BkgndBrush != NULL)
    {
        HANDLES(DeleteObject(BkgndBrush));
        BkgndBrush = NULL;
    }
    if (BkgndBrushSel != NULL)
    {
        HANDLES(DeleteObject(BkgndBrushSel));
        BkgndBrushSel = NULL;
    }
    if (LineNumberBrush != NULL)
    {
        HANDLES(DeleteObject(LineNumberBrush));
        LineNumberBrush = NULL;
    }
}

void ClearViewerHistory(BOOL dataOnly)
{
    int i;
    for (i = 0; i < VIEWER_HISTORY_SIZE; i++)
    {
        if (ViewerHistory[i] != NULL)
        {
            free(ViewerHistory[i]);
            ViewerHistory[i] = NULL;
        }
    }

    if (!dataOnly)
    {
        // also clear the combobox in any open Find windows
        ViewerWindowQueue.BroadcastMessage(WM_USER_CLEARHISTORY, 0, 0);
    }
}

void ReleaseViewer()
{
    if (ViewerMenu != NULL)
        DestroyMenu(ViewerMenu);
    ClearViewerHistory(TRUE); // we only want to clear the data
    if (ViewerContinue != NULL)
        HANDLES(CloseHandle(ViewerContinue));
    HANDLES(DeleteCriticalSection(&ViewerFontMeasureCS));
}

void CViewerWindow::SetViewerFont()
{
    if (ViewerFont != NULL)
        HANDLES(DeleteObject(ViewerFont));
    LOGFONT lf;
    if (UseCustomViewerFont)
        lf = ViewerLogFont;
    else
        GetDefaultViewerLogFont(&lf);
    lf.lfHeight = MulDiv(lf.lfHeight, ZoomPercent, 100);
    ViewerFont = HANDLES(CreateFontIndirect(&lf));
    if (ViewerFont == NULL)
    {
        TRACE_E("Unable to create ViewerFont.");
        return;
    }
    else
    {
        HDC dc = HANDLES(GetDC(NULL));
        HFONT old = (HFONT)SelectObject(dc, ViewerFont);
        TEXTMETRIC tm;
        BOOL ok = GetTextMetrics(dc, &tm);
        CharHeight = max(1, tm.tmHeight);
        CharWidth = max(1, tm.tmAveCharWidth);
        SelectObject(dc, old);
        HANDLES(ReleaseDC(NULL, dc));

        if (!ok)
        {
            TRACE_E("Unable to get text metrics for ViewerFont.");
            HANDLES(DeleteObject(ViewerFont));
            ViewerFont = NULL;
            return;
        }

        if (Bitmap.HBmp != NULL)
            Bitmap.ReCreateForScreenDC(Width, CharHeight);

        HANDLES(EnterCriticalSection(&ViewerFontMeasureCS));

        // Vista: the fixedsys font contains characters that do not have the expected width (even though it is a fixed-width font), therefore
        // we measure individual characters and map those with an incorrect width to a replacement character with the correct width
        if (!WindowsXP64AndLater && !ViewerFontMeasured) // before XP64 and Vista we did not run into this mess, so we will not even test it (on XP, W2K, NT4, etc.)
        {
            ViewerFontMeasured = TRUE;
            ViewerFontNeedsMapping = FALSE;
        }
        if (!ViewerFontMeasured)
        {
            HFONT oldFont = (HFONT)SelectObject(Bitmap.HMemDC, ViewerFont);
            int oldMode = SetBkMode(Bitmap.HMemDC, TRANSPARENT);

            ViewerFontNeedsMapping = FALSE;
            char ch[2];
            ch[1] = 0;
            RECT rect;
            char substChar = (char)0xB7 /* middle dot */;
            int x;
            for (x = 0; x < 256; x++)
            {
                ViewerFontMapping[x] = x;
                ch[0] = x;
                rect.left = 0;
                rect.right = Width;
                rect.top = 0;
                rect.bottom = CharHeight;
                if (DrawTextEx(Bitmap.HMemDC, ch, 1, &rect, DT_LEFT | DT_TOP | DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE, NULL))
                {
                    if (rect.right - rect.left != CharWidth)
                    {
                        if (x == 0xB7 /* middle dot */) // if the 'middle dot' is also incorrectly wide, substitute a space, which should be OK
                        {
                            substChar = ' ';
                            int z;
                            for (z = 0; z < x; z++)
                                if (ViewerFontMapping[z] == (char)0xB7 /* middle dot */)
                                    ViewerFontMapping[z] = substChar;
                        }
                        ViewerFontMapping[x] = substChar;
                        ViewerFontNeedsMapping = TRUE;
                    }
                }
                else
                    TRACE_I("CViewerWindow::SetViewerFont(): DrawTextEx: error for: " << x);
            }

            SetBkMode(Bitmap.HMemDC, oldMode);
            SelectObject(Bitmap.HMemDC, oldFont);

            ViewerFontMeasured = TRUE;
        }

        HANDLES(LeaveCriticalSection(&ViewerFontMeasureCS));
    }
}

void CViewerWindow::SetViewerZoom(int percent)
{
    percent = max(25, min(500, percent));
    if (ZoomPercent == percent)
        return;
    ZoomPercent = percent;
    Configuration.ViewerZoomPercent = ZoomPercent;
    // Font metrics are shared by the legacy viewer drawing code.  Reflow all
    // open viewers at the new shared zoom so none retains an old font.
    ViewerWindowQueue.BroadcastMessage(WM_USER_VIEWERZOOMCHANGED, 0, 0);
}

int CViewerWindow::GetTextLeft() const
{
    if (!ShowLineNumbers)
        return BORDER_WIDTH;
    return BORDER_WIDTH + (LineNumberDigits + 1) * CharWidth;
}

__int64 CViewerWindow::GetDocumentLineNumber(__int64 offset, __int64* lineStart)
{
    if (offset <= TextStartOffset())
    {
        if (lineStart)
            *lineStart = TextStartOffset();
        return 1;
    }
    if (Type == vtHex)
    {
        if (lineStart)
            *lineStart = offset - (offset % 16);
        return offset / 16 + 1;
    }

    if (HasDecodedTextMode())
    {
        const __int64 savedSeek = Seek;
        const __int64 savedLoaded = Loaded;
        __int64 line = 1;
        __int64 lastNewline = TextStartOffset();
        __int64 pos = TextStartOffset();
        __int64 pendingCREnd = -1;
        BOOL fatalErr = FALSE;
        while (pos < offset)
        {
            __int64 read = Prepare(NULL, pos, min((__int64)VIEW_BUFFER_SIZE, offset - pos), fatalErr);
            if (fatalErr || read <= 0)
                break;
            Salamander::Unicode::DecodedRun decoded = Salamander::Unicode::DecodeBytes(
                TextEncoding, Buffer + pos - Seek, (std::size_t)read, pos, pos + read >= offset);
            for (std::size_t i = 0; i < decoded.Scalars.size(); ++i)
            {
                if (decoded.RawStart[i] >= offset)
                    break;
                std::uint32_t scalar = decoded.Scalars[i];
                __int64 rawEnd = decoded.RawEnd[i];
                if (pendingCREnd != -1)
                {
                    if (scalar == L'\n' && Configuration.EOL_CRLF)
                    {
                        lastNewline = rawEnd;
                        ++line;
                        pendingCREnd = -1;
                        continue;
                    }
                    if (Configuration.EOL_CR)
                    {
                        lastNewline = pendingCREnd;
                        ++line;
                    }
                    pendingCREnd = -1;
                }
                if (scalar == L'\r')
                    pendingCREnd = rawEnd;
                else if (scalar == L'\n' && Configuration.EOL_LF)
                {
                    lastNewline = rawEnd;
                    ++line;
                }
                else if (scalar == 0 && Configuration.EOL_NULL)
                {
                    lastNewline = rawEnd;
                    ++line;
                }
            }
            pos += read;
        }
        if (pendingCREnd != -1 && Configuration.EOL_CR)
        {
            lastNewline = pendingCREnd;
            ++line;
        }
        if (lineStart)
            *lineStart = lastNewline;
        if (savedLoaded > 0)
        {
            BOOL restoreFatalErr = FALSE;
            Prepare(NULL, savedSeek, savedLoaded, restoreFatalErr);
        }
        return line;
    }

    // Prepare() reuses the rendering buffer.  Restore the visible buffer
    // afterwards so obtaining a label/status position cannot disturb an
    // incremental scroll repaint.
    const __int64 savedSeek = Seek;
    const __int64 savedLoaded = Loaded;
    __int64 line = 1;
    __int64 lastNewline = TextStartOffset() - 1; // offset of last EOL seen (-1 = none yet)
    __int64 pendingCREnd = -1;
    __int64 pos = TextStartOffset();
    BOOL fatalErr = FALSE;
    while (pos < offset)
    {
        __int64 read = Prepare(NULL, pos, min((__int64)VIEW_BUFFER_SIZE, offset - pos), fatalErr);
        if (fatalErr || read <= 0)
            break;
        unsigned char* text = Buffer + pos - Seek;
        for (__int64 i = 0; i < read; ++i)
        {
            unsigned char ch = text[i];
            if (pendingCREnd != -1)
            {
                if (ch == '\n' && Configuration.EOL_CRLF)
                {
                    lastNewline = pos + i;
                    ++line;
                    pendingCREnd = -1;
                    continue;
                }
                if (Configuration.EOL_CR)
                {
                    lastNewline = pendingCREnd;
                    ++line;
                }
                pendingCREnd = -1;
            }
            if (ch == '\r')
            {
                pendingCREnd = pos + i;
            }
            else if (ch == '\n')
            {
                if (Configuration.EOL_LF)
                {
                    lastNewline = pos + i;
                    ++line;
                }
            }
            else if (ch == 0)
            {
                if (Configuration.EOL_NULL)
                {
                    lastNewline = pos + i;
                    ++line;
                }
            }
        }
        pos += read;
    }
    if (pendingCREnd != -1 && Configuration.EOL_CR)
    {
        lastNewline = pendingCREnd;
        ++line;
    }
    if (lineStart)
        *lineStart = lastNewline + 1; // character after the last EOL is the start of the current logical line
    if (savedLoaded > 0)
    {
        BOOL restoreFatalErr = FALSE;
        Prepare(NULL, savedSeek, savedLoaded, restoreFatalErr);
    }
    return line;
}

void CViewerWindow::RefreshStatusBarDPI()
{
    LOGFONT logFont;
    GetEffectiveDefaultUILogFont(&logFont, HWindow);
    HFONT newFont = HANDLES(CreateFontIndirect(&logFont));
    if (newFont == NULL)
        return;

    if (HStatusBar != NULL)
        SendMessage(HStatusBar, WM_SETFONT, (WPARAM)newFont, TRUE);
    if (HZoomReset != NULL)
        SendMessage(HZoomReset, WM_SETFONT, (WPARAM)newFont, TRUE);
    if (HZoomOut != NULL)
        SendMessage(HZoomOut, WM_SETFONT, (WPARAM)newFont, TRUE);
    if (HZoomEdit != NULL)
        SendMessage(HZoomEdit, WM_SETFONT, (WPARAM)newFont, TRUE);
    if (HZoomIn != NULL)
        SendMessage(HZoomIn, WM_SETFONT, (WPARAM)newFont, TRUE);

    HFONT oldFont = StatusFont;
    StatusFont = newFont;
    if (oldFont != NULL)
        HANDLES(DeleteObject(oldFont));
}

void CViewerWindow::LayoutStatusBar()
{
    if (HStatusBar == NULL)
        return;
    RECT rc;
    GetClientRect(HWindow, &rc);
    // The status bar and its controls are window chrome.  Their dimensions
    // must not follow the document font zoom.
    int fontHeight = 0;
    if (StatusFont != NULL)
    {
        HDC dc = HANDLES(GetDC(HWindow));
        HFONT oldFont = (HFONT)SelectObject(dc, StatusFont);
        TEXTMETRIC metrics;
        if (GetTextMetrics(dc, &metrics))
            fontHeight = metrics.tmHeight + metrics.tmExternalLeading;
        SelectObject(dc, oldFont);
        HANDLES(ReleaseDC(HWindow, dc));
    }
    StatusBarHeight = ShowStatusBar
                          ? max(WinLibDPIFromLogical(HWindow, 20),
                                max(WinLibDPIGetSystemMetric(HWindow, SM_CYSMICON) +
                                        WinLibDPIFromLogical(HWindow, 4),
                                    fontHeight + WinLibDPIFromLogical(HWindow, 6)))
                          : 0;
    int scrollWidth = WinLibDPIGetSystemMetric(HWindow, SM_CXVSCROLL);
    int scrollHeight = WinLibDPIGetSystemMetric(HWindow, SM_CYHSCROLL);
    int gripWidth = (GetWindowLong(HStatusBar, GWL_STYLE) & SBARS_SIZEGRIP) ? scrollWidth : 0;
    ShowWindow(HStatusBar, ShowStatusBar ? SW_SHOW : SW_HIDE);
    ShowWindow(HZoomReset, ShowStatusBar ? SW_SHOW : SW_HIDE);
    ShowWindow(HZoomOut, ShowStatusBar ? SW_SHOW : SW_HIDE);
    ShowWindow(HZoomEdit, ShowStatusBar ? SW_SHOW : SW_HIDE);
    ShowWindow(HZoomIn, ShowStatusBar ? SW_SHOW : SW_HIDE);
    // Horizontal scrollbar (child control) spans the full client width, positioned
    // above the status bar — same layout as Notepad's Edit control.
    SetWindowPos(HScrollBar, NULL, 0, rc.bottom - StatusBarHeight - scrollHeight,
                 rc.right - scrollWidth, scrollHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    SetWindowPos(VScrollBar, NULL, rc.right - scrollWidth, 0, scrollWidth,
                 rc.bottom - StatusBarHeight - scrollHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    // Keep the native status bar behind its interactive children.
    SetWindowPos(HStatusBar, HWND_BOTTOM, 0, rc.bottom - StatusBarHeight, rc.right, StatusBarHeight, SWP_NOACTIVATE);
    // Do not reserve the size-grip width here.  The status bar owns that
    // grip at the window edge, while the zoom controls must extend right up
    // to it instead of leaving an unused scrollbar-width gap.
    int x = rc.right - gripWidth;
    int buttonWidth = WinLibDPIFromLogical(HWindow, 22);
    int editWidth = WinLibDPIFromLogical(HWindow, 54);
    int resetWidth = WinLibDPIFromLogical(HWindow, 42);
    int edge2 = WinLibDPIFromLogical(HWindow, 2);
    int edge3 = WinLibDPIFromLogical(HWindow, 3);
    int edge4 = WinLibDPIFromLogical(HWindow, 4);
    int edge6 = WinLibDPIFromLogical(HWindow, 6);
    x -= buttonWidth;
    SetWindowPos(HZoomIn, HWND_TOP, x, rc.bottom - StatusBarHeight + edge2,
                 buttonWidth, StatusBarHeight - edge4, SWP_NOACTIVATE);
    x -= editWidth;
    SetWindowPos(HZoomEdit, HWND_TOP, x, rc.bottom - StatusBarHeight + edge3,
                 editWidth, StatusBarHeight - edge6, SWP_NOACTIVATE);
    x -= buttonWidth;
    SetWindowPos(HZoomOut, HWND_TOP, x, rc.bottom - StatusBarHeight + edge2,
                 buttonWidth, StatusBarHeight - edge4, SWP_NOACTIVATE);
    x -= resetWidth;
    SetWindowPos(HZoomReset, HWND_TOP, x, rc.bottom - StatusBarHeight + edge2,
                 resetWidth, StatusBarHeight - edge4, SWP_NOACTIVATE);
    InvalidateRect(HWindow, NULL, FALSE);
    InvalidateRect(HScrollBar, NULL, FALSE);
    InvalidateRect(VScrollBar, NULL, FALSE);
    InvalidateRect(HStatusBar, NULL, FALSE);
}

void CViewerWindow::UpdateStatusBar(__int64 offset)
{
    if (HStatusBar == NULL || !ShowStatusBar)
        return;
    if (offset != -1)
        StatusOffset = offset;
    __int64 line = 0;
    __int64 column = 0;
    if (StatusOffset >= 0)
    {
        if (Type == vtHex)
        {
            line = StatusOffset / 16 + 1;
            column = StatusOffset % 16 + 1;
        }
        else if (VisibleFirstDocumentLine > 0)
        {
            for (int i = 0; i + 2 < LineOffset.Count; i += 3)
                if (StatusOffset >= LineOffset[i] && StatusOffset <= LineOffset[i + 1])
                {
                    __int64 logicalLineStart = 0;
                    line = GetDocumentLineNumber(StatusOffset, &logicalLineStart);
                    // LineOffset[i] is the start of the displayed row.  In wrap mode
                    // continuation rows start after the previous visual segment, but
                    // the status column must stay relative to the original logical
                    // line, not restart at the visual wrap.
                    if (HasDecodedTextMode())
                    {
                        const __int64 savedSeek = Seek;
                        const __int64 savedLoaded = Loaded;
                        Salamander::Unicode::DecodedRun visual;
                        BOOL fatalErr = FALSE;
                        if (DecodeTextRange(NULL, logicalLineStart, StatusOffset, visual, fatalErr, FALSE) && !fatalErr)
                            column = (__int64)Salamander::Unicode::BuildTextElementMap(visual).Count() + 1;
                        if (savedLoaded > 0)
                        {
                            BOOL restoreFatalErr = FALSE;
                            Prepare(NULL, savedSeek, savedLoaded, restoreFatalErr);
                        }
                    }
                    else
                        column = StatusOffset - logicalLineStart + 1;
                    break;
                }
        }
    }
    char text[256];
    if (line == 0)
        strcpy(text, "Line -, Column -");
    else
        sprintf(text, "Line %I64d, Column %I64d", line, column);
    if (StartSelection != -1 && EndSelection != -1 && StartSelection != EndSelection)
    {
        __int64 first = min(StartSelection, EndSelection);
        __int64 last = max(StartSelection, EndSelection);
        // LineOffset covers only the current paint.  Use document offsets so
        // multi-page selections are counted in their entirety.  'last' is
        // exclusive, hence the final selected byte is last - 1.
        if (CachedSelectionStart != first || CachedSelectionEnd != last)
        {
            CachedSelectionStart = first;
            CachedSelectionEnd = last;
            CachedSelectionLineCount = GetDocumentLineNumber(last - 1) - GetDocumentLineNumber(first) + 1;
            CachedSelectionCharacterCount = last - first;
            if (HasDecodedTextMode())
            {
                const __int64 savedSeek = Seek;
                const __int64 savedLoaded = Loaded;
                Salamander::Unicode::DecodedRun selected;
                BOOL fatalErr = FALSE;
                if (DecodeTextRange(NULL, first, last, selected, fatalErr) && !fatalErr)
                    CachedSelectionCharacterCount = (__int64)Salamander::Unicode::BuildTextElementMap(selected).Count();
                if (savedLoaded > 0)
                {
                    BOOL restoreFatalErr = FALSE;
                    Prepare(NULL, savedSeek, savedLoaded, restoreFatalErr);
                }
            }
        }
        char selection[96];
        sprintf(selection, "  |  %I64d characters, %I64d lines selected", CachedSelectionCharacterCount,
                max((__int64)1, CachedSelectionLineCount));
        strcat(text, selection);
    }
    SendMessage(HStatusBar, SB_SETTEXT, 0, (LPARAM)text);
}
