// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"

#include "mainwnd.h"
#include "usermenu.h"
#include "edtlbwnd.h"
#include "cfgdlg.h"
#include "dialogs.h"
#include "configstorage.h"
#include "plugins.h"
#include "fileswnd.h"
#include "shellib.h"
#include "editwnd.h"
#include "codetbl.h"
#include "execute.h"
#include "viewer.h"
#include "find.h"
#include "gui.h"
#include "darkmode.h"
#include "svg.h"
#include <uxtheme.h>

#ifndef DARKMODE_TRACE_CTLFLOW
#define DARKMODE_TRACE_CTLFLOW 0
#endif

#if DARKMODE_TRACE_CTLFLOW
static void DarkModeTracePageThemeEvent(const char* pageName, UINT uMsg)
{
    const char* msg = uMsg == WM_THEMECHANGED ? "WM_THEMECHANGED" : "WM_SETTINGCHANGE";
    TRACE_I("[DARKMODE_TRACE] page=%s event=%s", pageName, msg);
}
#endif

static void FlushDWMForInteractiveMove()
{
    typedef HRESULT(WINAPI * FDwmFlush)();
    static FDwmFlush dwmFlush = NULL;
    static BOOL loaded = FALSE;
    if (!loaded)
    {
        HMODULE dwmApi = GetModuleHandleW(L"dwmapi.dll");
        if (dwmApi != NULL)
            dwmFlush = reinterpret_cast<FDwmFlush>(GetProcAddress(dwmApi, "DwmFlush"));
        loaded = TRUE;
    }
    if (dwmFlush != NULL)
        dwmFlush();
}

static void UpdateConfigListViewColors(HWND listView)
{
    if (DarkModeShouldUseDarkColors())
        DarkModeUpdateListViewColors(listView);
    else
        DarkModeUpdateListViewColors(listView, GetSysColor(COLOR_WINDOWTEXT), GetSysColor(COLOR_WINDOW), false);
}

static bool DarkModeTryHandleCtlColorForDialogPage(UINT uMsg, WPARAM wParam, LPARAM lParam, INT_PTR& outResult)
{
    if (uMsg != WM_CTLCOLORSTATIC && uMsg != WM_CTLCOLORBTN)
        return false;

    LRESULT brush = 0;
    if (DarkModeHandleCtlColor(uMsg, wParam, lParam, brush))
    {
        outResult = static_cast<INT_PTR>(brush);
        return true;
    }

    if (!DarkModeShouldUseDarkColors())
        return false;

    HDC dc = reinterpret_cast<HDC>(wParam);
    if (dc != NULL)
    {
        const DarkModeColors& colors = DarkModeGetColors();
        SetTextColor(dc, colors.readableText);
        SetBkColor(dc, colors.background);
        SetBkMode(dc, TRANSPARENT);
    }
    outResult = reinterpret_cast<INT_PTR>(HDialogBrush != NULL ? HDialogBrush : GetSysColorBrush(COLOR_BTNFACE));
    return true;
}


static void FillRectWithSysColor(HDC hdc, const RECT& rect, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    if (brush != NULL)
    {
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);
    }
}

static void SetViewsAvailableColumnsColumnWidth(HWND listView)
{
    if (listView == NULL)
        return;

    RECT rc;
    GetClientRect(listView, &rc);
    // This list has a single column. Let it cover the full client area so the
    // report-view background is painted by that column instead of leaving a
    // separate native strip on the right side.
    int width = rc.right - rc.left;
    if (width < 20)
        width = 20;
    ListView_SetColumnWidth(listView, 0, width);
}

void RemoveListViewWhiteClientEdge(HWND listView)
{
    if (listView == NULL || !DarkModeShouldUseDarkColors())
        return;

    DWORD exStyle = (DWORD)GetWindowLongPtr(listView, GWL_EXSTYLE);
    DWORD style = (DWORD)GetWindowLongPtr(listView, GWL_STYLE);
    DWORD newExStyle = exStyle & ~WS_EX_CLIENTEDGE;
    DWORD newStyle = style & ~WS_BORDER;
    if (newExStyle == exStyle && newStyle == style)
        return;

    if (newExStyle != exStyle)
        SetWindowLongPtr(listView, GWL_EXSTYLE, newExStyle);
    if (newStyle != style)
        SetWindowLongPtr(listView, GWL_STYLE, newStyle);
    SetWindowPos(listView, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

static void RemoveListViewsWhiteClientEdge(HWND listView, HWND listView2)
{
    RemoveListViewWhiteClientEdge(listView);
    RemoveListViewWhiteClientEdge(listView2);
}

bool ShouldCustomDrawListViewCheckboxes()
{
    // Keep the post-paint path for checkbox list views whenever dark colors are
    // active: darkmodelib themes the list-view/header chrome, but the native
    // state-image background can still remain white.
    return DarkModeShouldUseDarkColors();
}

// Custom-draw handler for dark-mode checkboxes in a ListView.
// Called from CDDS_ITEMPOSTPAINT so the native/default item draw stays intact
// and this pass overlays the problematic state-image area (plus the row
// background/text) with dark colors.
void DrawDarkModeListViewCheckboxes(HWND listView, NMLVCUSTOMDRAW* customDraw, int columnCount)
{
    if (!ShouldCustomDrawListViewCheckboxes() || listView == NULL || customDraw == NULL || columnCount <= 0)
        return;

    const int item = static_cast<int>(customDraw->nmcd.dwItemSpec);
    RECT boundsRect;
    RECT labelRect;
    if (!ListView_GetItemRect(listView, item, &boundsRect, LVIR_BOUNDS) ||
        !ListView_GetItemRect(listView, item, &labelRect, LVIR_LABEL))
    {
        return;
    }

    HDC hdc = customDraw->nmcd.hdc;
    if (hdc == NULL)
        return;

    // Fill the entire visual row. LVIR_BOUNDS does not cover the state-image
    // gutter nor the unused space to the right of the text, and those are
    // exactly the areas where native list-view painting leaves white pixels.
    RECT clientRect;
    GetClientRect(listView, &clientRect);
    RECT rowRect = boundsRect;
    rowRect.left = clientRect.left;
    rowRect.right = clientRect.right;

    const bool selected = (ListView_GetItemState(listView, item, LVIS_SELECTED) & LVIS_SELECTED) != 0;
    const COLORREF rowBackground = selected ? DarkModeGetColors().background : DarkModeGetDialogBackgroundColor();
    FillRectWithSysColor(hdc, rowRect, rowBackground);

    // Calculate checkbox area (same region the native state image occupies)
    RECT stateRect = rowRect;
    stateRect.right = labelRect.left;
    if (stateRect.right <= stateRect.left)
        return;

    int checkSize = stateRect.bottom - stateRect.top - 2;
    if (checkSize < 9)
        checkSize = 9;
    if (checkSize > 13)
        checkSize = 13;
    RECT checkRect;
    checkRect.left = stateRect.left + ((stateRect.right - stateRect.left) - checkSize) / 2;
    checkRect.top = stateRect.top + ((stateRect.bottom - stateRect.top) - checkSize) / 2;
    checkRect.right = checkRect.left + checkSize;
    checkRect.bottom = checkRect.top + checkSize;

    const bool checked = (ListView_GetItemState(listView, item, LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2));
    const COLORREF fill = checked ? RGB(0x4C, 0xC2, 0xF0) : RGB(0x24, 0x24, 0x24);
    const COLORREF border = checked ? RGB(0x7A, 0xD7, 0xF7) : RGB(0x78, 0x78, 0x78);

    HBRUSH fillBrush = CreateSolidBrush(fill);
    HPEN borderPen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBrush = fillBrush != NULL ? SelectObject(hdc, fillBrush) : NULL;
    HGDIOBJ oldPen = borderPen != NULL ? SelectObject(hdc, borderPen) : NULL;
    Rectangle(hdc, checkRect.left, checkRect.top, checkRect.right, checkRect.bottom);

    if (checked)
    {
        HPEN checkPen = CreatePen(PS_SOLID, 2, RGB(0x10, 0x10, 0x10));
        HGDIOBJ oldCheckPen = checkPen != NULL ? SelectObject(hdc, checkPen) : NULL;
        MoveToEx(hdc, checkRect.left + 3, checkRect.top + checkSize / 2, NULL);
        LineTo(hdc, checkRect.left + checkSize / 2 - 1, checkRect.bottom - 4);
        LineTo(hdc, checkRect.right - 3, checkRect.top + 3);
        if (oldCheckPen != NULL)
            SelectObject(hdc, oldCheckPen);
        if (checkPen != NULL)
            DeleteObject(checkPen);
    }

    int oldBkMode = SetBkMode(hdc, TRANSPARENT);
    COLORREF oldTextColor = SetTextColor(hdc, DarkModeGetDialogTextColor());
    for (int column = 0; column < columnCount; ++column)
    {
        char text[256];
        text[0] = 0;
        ListView_GetItemText(listView, item, column, text, _countof(text));

        RECT textRect;
        if (ListView_GetSubItemRect(listView, item, column, LVIR_LABEL, &textRect))
        {
            textRect.left += 2;
            DrawText(hdc, text, -1, &textRect,
                     DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX | DT_END_ELLIPSIS);
        }
    }
    SetTextColor(hdc, oldTextColor);
    SetBkMode(hdc, oldBkMode);

    if (oldPen != NULL)
        SelectObject(hdc, oldPen);
    if (oldBrush != NULL)
        SelectObject(hdc, oldBrush);
    if (borderPen != NULL)
        DeleteObject(borderPen);
    if (fillBrush != NULL)
        DeleteObject(fillBrush);
}

//****************************************************************************
//
// CHighlightMasksItem
//

CHighlightMasksItem::CHighlightMasksItem()
{
    CALL_STACK_MESSAGE1("CHighlightMasksItem()");
    Masks = NULL;
    Attr = 0;
    ValidAttr = 0;
    NormalFg = RGBF(0, 0, 0, SCF_DEFAULT);
    NormalBk = RGBF(0, 0, 0, SCF_DEFAULT);
    FocusedFg = RGBF(0, 0, 0, SCF_DEFAULT);
    FocusedBk = RGBF(0, 0, 0, SCF_DEFAULT);
    SelectedFg = RGBF(0, 0, 0, SCF_DEFAULT);
    SelectedBk = RGBF(0, 0, 0, SCF_DEFAULT);
    FocSelFg = RGBF(0, 0, 0, SCF_DEFAULT);
    FocSelBk = RGBF(0, 0, 0, SCF_DEFAULT);
    HighlightFg = RGBF(0, 0, 0, SCF_DEFAULT);
    HighlightBk = RGBF(0, 0, 0, SCF_DEFAULT);
    Set("");
}

CHighlightMasksItem::CHighlightMasksItem(CHighlightMasksItem& item)
{
    CALL_STACK_MESSAGE1("CHighlightMasksItem(&)");
    Masks = NULL;
    Attr = item.Attr;
    ValidAttr = item.ValidAttr;
    NormalFg = item.NormalFg;
    NormalBk = item.NormalBk;
    FocusedFg = item.FocusedFg;
    FocusedBk = item.FocusedBk;
    SelectedFg = item.SelectedFg;
    SelectedBk = item.SelectedBk;
    FocSelFg = item.FocSelFg;
    FocSelBk = item.FocSelBk;
    HighlightFg = item.HighlightFg;
    HighlightBk = item.HighlightBk;
    Set(item.Masks->GetMasksString());
}

CHighlightMasksItem::~CHighlightMasksItem()
{
    if (Masks != NULL)
        delete Masks;
}

BOOL CHighlightMasksItem::Set(const char* masks)
{
    if (Masks == NULL)
        Masks = new CMaskGroup;
    if (Masks == NULL)
    {
        TRACE_E(LOW_MEMORY);
        return FALSE;
    }
    Masks->SetMasksString(masks);
    return TRUE;
}

BOOL CHighlightMasksItem::IsGood()
{
    return Masks != NULL;
}

BOOL CHighlightMasks::Load(CHighlightMasks& source)
{
    CALL_STACK_MESSAGE1("CHighlightMasks::Load()");
    CHighlightMasksItem* item;
    DestroyMembers();
    int i;
    for (i = 0; i < source.Count; i++)
    {
        item = new CHighlightMasksItem(*source[i]);
        if (!item->IsGood())
        {
            delete item;
            TRACE_E(LOW_MEMORY);
            return FALSE;
        }
        Add(item);
        if (!IsGood())
        {
            delete item;
            TRACE_E(LOW_MEMORY);
            return FALSE;
        }
    }
    return TRUE;
}

//****************************************************************************
//
// ValidatePathIsNotEmpty
//
// returns FALSE when the path is empty, otherwise TRUE (the path contains 'something')

BOOL ValidatePathIsNotEmpty(HWND hParent, const char* path)
{
    const char* iterator = path;
    BOOL empty = TRUE;
    while (*iterator != 0)
    {
        if (*iterator != ' ')
        {
            empty = FALSE;
            break;
        }
        else
            iterator++;
    }

    // an empty string is not allowed
    if (empty)
    {
        SalMessageBox(hParent, LoadStr(IDS_THEPATHISINVALID), LoadStr(IDS_ERRORTITLE),
                      MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }
    /* originally this function was called ValidateCommandFile and checked file existence, but we abandoned that idea (maybe because of network issues?)
  if (testFileExist)
  {
    // try to access the file
    if (!FileExists(myPath))
    {
      // the file was not found - it might not exist or the path may be unreachable;
      // let the user force it
      char buff[1000];
      sprintf(buff, LoadStr(IDS_THECOMMANDISINVALID), myPath, itemName);
      int ret = SalMessageBox(hParent, buff, LoadStr(IDS_ERRORTITLE),
                              MB_YESNO | MB_ICONQUESTION);
      if (ret == IDYES)
        return FALSE;    // the user wants to modify the item
    }
  }
*/
    return TRUE;
}

static BOOL CanWriteRegStorageFilePath(const char* path)
{
    if (path == NULL || path[0] == 0)
        return FALSE;

    DWORD attrs = GetFileAttributes(path);
    if (attrs != INVALID_FILE_ATTRIBUTES)
    {
        if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
            return FALSE;

        HANDLE file = HANDLES_Q(CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL, NULL));
        if (file == INVALID_HANDLE_VALUE)
            return FALSE;

        HANDLES(CloseHandle(file));
        return TRUE;
    }

    char tmpPath[SAL_MAX_PATH];
    _snprintf_s(tmpPath, _TRUNCATE, "%s.%lu.test", path, GetCurrentProcessId());
    HANDLE file = HANDLES_Q(CreateFile(tmpPath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, NULL));
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    HANDLES(CloseHandle(file));
    DeleteFile(tmpPath);
    return TRUE;
}

//
// ****************************************************************************
// CLoadSaveToRegistryMutex
//

CLoadSaveToRegistryMutex::CLoadSaveToRegistryMutex()
{
    Mutex = NULL;
    DebugCheck = 0;
}

CLoadSaveToRegistryMutex::~CLoadSaveToRegistryMutex()
{
    if (DebugCheck != 0)
        TRACE_E("CLoadSaveToRegistryMutex(): fatal error: incorrect use of mutex for Load/Save to Registry: " << DebugCheck);
    if (Mutex != NULL)
        HANDLES(CloseHandle(Mutex));
    Mutex = NULL;
}

extern const char* LOADSAVE_REGISTRY_MUTEX_NAME;

void CLoadSaveToRegistryMutex::Init()
{
    // 2.52b1: adding support for FastUserSwitching/Terminal Services
    // Up to this version the mutex was created in the local namespace under the name SalamanderLoadSaveToRegistryMutex.
    // Now we want to provide interoperability across all sessions, so we place it in the Global namespace.
    // We also append the SID (from W2K on) so that mutexes of Salamander instances running under different users 
    // do not collide -- they work with their own Registry trees, no synchronization needed there.
    // We could add the Salamander version into the mutex name as well (every version has its own Registry tree).
    // But new versions can remove obsolete configurations, so we won't do it.
    LPTSTR sid = NULL;
    if (!GetStringSid(&sid))
        sid = NULL;

    char buff[1000];
    if (sid == NULL)
    {
        // failed to obtain SID -- continue in fallback mode
        _snprintf_s(buff, _TRUNCATE, "%s", LOADSAVE_REGISTRY_MUTEX_NAME);
    }
    else
    {
        _snprintf_s(buff, _TRUNCATE, "Global\\%s_%s", LOADSAVE_REGISTRY_MUTEX_NAME, sid);
        LocalFree(sid);
    }

    PSID psidEveryone;
    PACL paclNewDacl;
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES* saPtr = CreateAccessableSecurityAttributes(&sa, &sd, SYNCHRONIZE /*| MUTEX_MODIFY_STATE*/, &psidEveryone, &paclNewDacl);

    Mutex = HANDLES_Q(CreateMutex(saPtr, FALSE, buff));
    if (Mutex == NULL)
    {
        Mutex = HANDLES_Q(OpenMutex(SYNCHRONIZE, FALSE, buff));
        if (Mutex == NULL)
        {
            DWORD err = GetLastError();
            TRACE_I("CLoadSaveToRegistryMutex::Init(): Unable to create/open mutex for Load/Save to Registry synchronization! Error: " << GetErrorText(err));
        }
    }

    if (psidEveryone != NULL)
        FreeSid(psidEveryone);
    if (paclNewDacl != NULL)
        LocalFree(paclNewDacl);

    DebugCheck = 0;
}

void CLoadSaveToRegistryMutex::Enter()
{
    DebugCheck++;
    if (Mutex != NULL)
    {
        if (WaitForSingleObject(Mutex, INFINITE) == WAIT_FAILED)
            TRACE_E("CLoadSaveToRegistryMutex::Enter(): WaitForSingleObject() failed!");
    }
    else
        TRACE_E("CLoadSaveToRegistryMutex::Enter(): the Mutex==NULL! Not initialized?");
}

void CLoadSaveToRegistryMutex::Leave()
{
    DebugCheck--;
    if (Mutex != NULL)
    {
        if (!ReleaseMutex(Mutex))
            TRACE_E("CLoadSaveToRegistryMutex::Enter(): ReleaseMutex() failed!");
    }
    else
        TRACE_E("CLoadSaveToRegistryMutex::Leave(): the Mutex==NULL! Not initialized?");
}

//
// ****************************************************************************
// CConfiguration
//

const char* DefTopToolBar = "11,14,15,70,-1,40,56,-1,30,31,32,-1,41,42,18,27,55,33,-1,3,34,35,20,19,-1,43,46,49";
const char* DefMiddleToolBar = "2,3,17,21,22,23,72,26,24,25,27,55,28,29,30,31,32,33";
const char* DefLeftToolBar = "36";
const char* DefRightToolBar = "51";

CConfiguration::CConfiguration()
{
    ConfigVersion = 0;
    IncludeDirs = FALSE;
    AutoSave = TRUE;
    StorageType = cstRegistry;
    CloseShell = FALSE;
    CommandLineApplication[0] = 0; // empty means use COMSPEC
    CommandLineArguments[0] = 0;   // empty keeps the default cmd.exe /C or /K handling
    ShowGrepErrors = FALSE; // other search tools (FAR/WinCmd/PowerDesk/Windows Find) do not show errors
    FindFullRowSelect = FALSE;
    MinBeepWhenDone = TRUE;
    UseRecycleBin = 1;
    FileNameFormat = 4; // as on the disk
    SizeFormat = SIZE_FORMAT_BYTES;
    RecycleMasks.SetMasksString("*.txt;*.doc");
    LastFocusedPage = 0;
    ConfigurationHeight = 0; // the dialog logic will not allow a smaller dialog than the largest page it contains
    ConfigurationWidth = 0;
    ConfigurationTreeWidth = 0;
    ConfigurationViewsRightWidth = 0;
    PluginsManagerWidth = 0;
    PluginsManagerHeight = 0;
    ViewersAndEditorsExpanded = 0;
    PackersAndUnpackersExpanded = 0;
    ClearReadOnly = TRUE;
    PrimaryContextMenu = TRUE;
    NotHiddenSystemFiles = FALSE;
    AlwaysOnTop = FALSE;
    //  FastDirectoryMove = TRUE;
    SortUsesLocale = TRUE;
    SortDetectNumbers = TRUE;
    SortNewerOnTop = FALSE; // by default sort like Explorer on XP, newer items at the bottom
    SortDirsByName = FALSE; // so people do not report it as a bug like they did to Ghisler
    SortDirsByExt = FALSE;  // directories have no extensions, an option kept for companies/users relying on the old directory ordering
    SaveHistory = TRUE;
    SaveWorkDirs = FALSE; // by default save space in the registry, the list is large
    WorkDirsHistoryScope = wdhsShared;
    EnableCmdLineHistory = TRUE;
    SaveCmdLineHistory = TRUE;
    //  LantasticCheck = FALSE;
    UseSalOpen = FALSE;
    NetwareFastDirMove = FALSE; // choose the slower but 100% working mode; power users can switch it
    UseAsyncCopyAlg = TRUE;
    ReloadEnvVariables = TRUE;
    PathAutoComplete = TRUE;
    CreateDirAutoComplete = FALSE;
    QuickRenameSelectAll = FALSE;
    EditNewSelectAll = TRUE;
    ShiftForHotPaths = TRUE;
    BackspaceAction = 0;
    OnlyOneInstance = FALSE;
    ForceOnlyOneInstance = FALSE;
    StatusArea = FALSE;
    SingleClick = FALSE;
    TopToolBarVisible = TRUE;
    PluginsBarVisible = FALSE;
    ExtensionBarVisible = FALSE;
    MiddleToolBarVisible = FALSE;
    BottomToolBarVisible = TRUE;
    UserMenuToolBarVisible = FALSE;
    HotPathsBarVisible = FALSE;
    DriveBarVisible = TRUE;
    DriveBar2Visible = FALSE;
    TreeViewVisible = FALSE;
    PanelTooltips = FALSE;
    IconSpacingVert = 43;
    IconSpacingHorz = 43;
    TileSpacingVert = 8;
    ThumbnailSpacingHorz = 19; // 29 on Windows XP
    ThumbnailSize = THUMBNAIL_SIZE_DEFAULT;

    // options for Compare Directories
    CompareByTime = TRUE;
    CompareBySize = TRUE;
    CompareByContent = FALSE;
    CompareByAttr = FALSE;
    CompareSubdirs = FALSE;
    CompareSubdirsAttr = FALSE;
    CompareOnePanelDirs = FALSE;
    CompareMoreOptions = FALSE;
    CompareIgnoreFiles = FALSE;
    CompareIgnoreDirs = FALSE;
    int errPos;
    CompareIgnoreFilesMasks.SetMasksString("");
    CompareIgnoreFilesMasks.PrepareMasks(errPos);
    CompareIgnoreDirsMasks.SetMasksString("");
    CompareIgnoreDirsMasks.PrepareMasks(errPos);

    // Confirmation
    CnfrmFileDirDel = TRUE;
    CnfrmNEDirDel = FALSE;
    CnfrmFileOver = TRUE;
    CnfrmDirOver = FALSE;
    CnfrmSHFileDel = TRUE;
    CnfrmSHDirDel = FALSE;
    CnfrmSHFileOver = TRUE;
    CnfrmNTFSPress = TRUE;
    CnfrmNTFSCrypt = TRUE;
    CnfrmDragDrop = FALSE;
    CnfrmCloseArchive = TRUE;
    CnfrmCreateDir = TRUE;
    CnfrmCloseFind = TRUE;
    CnfrmStopFind = TRUE;
    CnfrmCreatePath = TRUE;
    CnfrmAlwaysOnTop = TRUE;
    CnfrmOnSalClose = FALSE;
    CnfrmDetachClose = TRUE;
    CnfrmDetachTabClose = TRUE;
    CnfrmSendEmail = TRUE;
    CnfrmAddToArchive = TRUE;
    CnfrmChangeDirTC = TRUE;
    CnfrmShowNamesToCompare = TRUE;
    CnfrmDSTShiftsIgnored = TRUE;
    CnfrmDSTShiftsOccured = TRUE;
    CnfrmCopyMoveOptionsNS = TRUE;
    CnfrmChangeDirHistoryErr = TRUE;
    CnfrmConfirmDeleteExtInfo = FALSE;
    //  PanelTooltip = TRUE;
    KeepPluginsSorted = TRUE;
    ShowSLGIncomplete = TRUE;

    LastUsedSpeedLimit = 1024 * 1024; // default 1 MB/s

    QuickSearchEnterAlt = FALSE;

    // for displaying items in the panel
    FullRowSelect = FALSE;
    FullRowHighlight = TRUE;
    UseIconTincture = TRUE;
    UsePanelTabs = TRUE;
    ShowPanelCaption = TRUE;
    ShowPanelZoom = TRUE;
    UseWindowsDarkMode = FALSE;
    strcpy(InfoLineContent, "$(FileName): $(FileSize), $(FileDate), $(FileTime), $(FileAttributes), $(FileDOSName)");

    HotPathAutoConfig = TRUE;

    DrvSpecFloppyMon = TRUE;
    DrvSpecFloppySimple = TRUE;
    DrvSpecRemovableMon = TRUE;
    DrvSpecRemovableSimple = FALSE; // 2.5b11: since floppies have their own category, we can afford to read icons
    DrvSpecFixedMon = TRUE;
    DrvSpecFixedSimple = FALSE;
    DrvSpecRemoteMon = TRUE;
    DrvSpecRemoteSimple = FALSE;
    DrvSpecRemoteDoNotRefreshOnAct = FALSE;
    DrvSpecCDROMMon = TRUE;
    DrvSpecCDROMSimple = FALSE;

    IfPathIsInaccessibleGoToIsMyDocs = TRUE;
    IfPathIsInaccessibleGoTo[0] = 0;

    SpaceSelCalcSpace = TRUE;
    CountSizeStayOnFileSystem = FALSE;

    UseTimeResolution = TRUE;
    TimeResolution = 2;
    IgnoreDSTShifts = FALSE;

    UseDragDropMinTime = TRUE;
    DragDropMinTime = 500;

    strcpy(TopToolBar, DefTopToolBar);
    strcpy(MiddleToolBar, DefMiddleToolBar);
    strcpy(LeftToolBar, DefLeftToolBar);
    strcpy(RightToolBar, DefRightToolBar);

    SkillLevel = SKILL_LEVEL_ADVANCED; // try to present as many options as possible

    int i;
    for (i = 0; i < SELECT_HISTORY_SIZE; i++)
        SelectHistory[i] = NULL;
    for (i = 0; i < COPY_HISTORY_SIZE; i++)
        CopyHistory[i] = NULL;
    for (i = 0; i < EDIT_HISTORY_SIZE; i++)
        EditHistory[i] = NULL;
    for (i = 0; i < CHANGEDIR_HISTORY_SIZE; i++)
        ChangeDirHistory[i] = NULL;
    for (i = 0; i < FILELIST_HISTORY_SIZE; i++)
        FileListHistory[i] = NULL;
    for (i = 0; i < CREATEDIR_HISTORY_SIZE; i++)
        CreateDirHistory[i] = NULL;
    for (i = 0; i < QUICKRENAME_HISTORY_SIZE; i++)
        QuickRenameHistory[i] = NULL;
    for (i = 0; i < EDITNEW_HISTORY_SIZE; i++)
        EditNewHistory[i] = NULL;
    for (i = 0; i < CONVERT_HISTORY_SIZE; i++)
        ConvertHistory[i] = NULL;
    for (i = 0; i < FILTER_HISTORY_SIZE; i++)
        FilterHistory[i] = NULL;

    FileListHistory[0] = DupStr("$(FileName)$(CRLF)"); // default for MakeFileList

    FileListName[0] = 0;
    FileListAppend = FALSE;
    FileListRecursive = FALSE;
    FileListDestination = 0;
    // Internal Viewer:
    CopyFindText = TRUE;
    EOL_CRLF = TRUE;
    EOL_CR = TRUE;
    EOL_LF = TRUE;
    EOL_NULL = TRUE;
    DefViewMode = 0; // Auto-Select
    TabSize = 8;
    SavePosition = FALSE;
    TextModeMasks.SetMasksString("*.txt;*.602;*.xml");
    TextModeMasks.PrepareMasks(errPos);
    HexModeMasks.SetMasksString("");
    HexModeMasks.PrepareMasks(errPos);
    WindowPlacement.length = 0;
    WrapText = TRUE;
    ViewerShowLineNumbers = FALSE;
    ViewerShowStatusBar = TRUE;
    ViewerZoomPercent = 100;
    CodePageAutoSelect = TRUE;
    DefaultConvert[0] = 0;
    AutoCopySelection = FALSE;
    GoToOffsetIsHex = TRUE;

    // Change drive
    ChangeDriveShowMountFolders = TRUE;
    ChangeDriveMountFoldersMode = TITLE_BAR_MODE_DIRECTORY;
    ChangeDriveMountFoldersName = TRUE;
    ChangeDriveMountFoldersDriveBar = FALSE;
    ChangeDriveShowWindowsSandbox = FALSE;
    ChangeDriveShowMyDoc = TRUE;
    ChangeDriveShow3DObjects = FALSE;
    ChangeDriveShowDesktop = FALSE;
    ChangeDriveShowDownloads = FALSE;
    ChangeDriveShowMusic = FALSE;
    ChangeDriveShowPictures = FALSE;
    ChangeDriveShowVideos = FALSE;
    ChangeDriveShowAnother = TRUE;
    ChangeDriveShowNet = TRUE;
    ChangeDriveCloudStorage = TRUE;

    MenuIndex = 0;
    MenuBreak = TRUE;
    MenuWidth = 1; // dummy

    TopToolbarIndex = 1;
    TopToolbarBreak = TRUE;
    TopToolbarWidth = 1; // dummy

    PluginsBarIndex = 2;
    PluginsBarBreak = TRUE;
    PluginsBarWidth = 1; // dummy

    ExtensionBarIndex = 3;
    ExtensionBarBreak = TRUE;
    ExtensionBarWidth = 1; // dummy

    UserMenuToolbarIndex = 4;
    UserMenuToolbarBreak = TRUE;
    UserMenuToolbarWidth = 1; // dummy
    UserMenuToolbarLabels = 1;

    HotPathsBarIndex = 5;
    HotPathsBarBreak = TRUE;
    HotPathsBarWidth = 1; // dummy

    DriveBarIndex = 6;
    DriveBarBreak = TRUE;
    DriveBarWidth = 1; // dummy
    TreeViewWidth = 200;
    TreeViewAutoHide = FALSE;
    DetachedTreeViewWidth = TreeViewWidth;
    DetachedTreeViewAutoHide = TreeViewAutoHide;

    GripsVisible = TRUE;
    DetachedPanels = FALSE;
    DetachedWindowPlacement.length = 0;
    DetachedTab = FALSE;
    DetachedTabWindowPlacement.length = 0;

    // Packers / Unpackers
    UseAnotherPanelForPack = FALSE;   // like WinZip folks - let's annoy them
    UseAnotherPanelForUnpack = FALSE; // or rather strip them bare :-)
    UseSubdirNameByArchiveForUnpack = FALSE;
    UseSimpleIconsInArchives = FALSE;

    UseEditNewFileDefault = FALSE;
    EditNewFileDefault[0] = 0;

    // Tip of the Day
    //  ShowTipOfTheDay = TRUE;
    //  LastTipOfTheDay = 0;

    LastPluginVer = 0;
    LastPluginVerOP = 0;

    ConfigWasImported = FALSE;

    // Find dialog
    SearchFileContent = FALSE;
    FindDialogWindowPlacement.length = 0; // not valid yet
    // column width of the Find dialog
    FindColNameWidth = -1; // let it be set according to the window size

    // Language
    LoadedSLGName[0] = 0;
    SLGName[0] = 0;
    DoNotDispCantLoadPluginSLG = FALSE;
    DoNotDispCantLoadPluginSLG2 = FALSE;
    UseAsAltSLGInOtherPlugins = FALSE;
    AltPluginSLGName[0] = 0;

    // the ConversionTable variable is not loaded from the configuration
    strcpy(ConversionTable, "*");

    TitleBarShowPath = TRUE;
    TitleBarMode = TITLE_BAR_MODE_DIRECTORY; // like Explorer
    TabCaptionMode = TITLE_BAR_MODE_DIRECTORY;
    TabButtonMinWidth = 0;
    TabButtonMaxWidth = 0;
    TabCaptionAlignment = TAB_CAPTION_ALIGN_CENTER;
    TabActiveBorder = TRUE;
    TabActiveBorderColor = CLR_INVALID;
    TabCloseButtonActive = FALSE;
    TabCloseButtonAll = FALSE;
    UseTitleBarPrefix = FALSE;
    strcpy(TitleBarPrefix, "ADMIN");
    UseTitleBarPrefixForced = FALSE;
    TitleBarPrefixForced[0] = 0;

    MainWindowIconIndex = MAINWINDOWICON_DEFAULT_INDEX; // default Samandarin icon
    MainWindowIconIndexForced = -1;

    ClickQuickRename = TRUE;

    VisibleDrives = DRIVES_MASK; // by default show all drives
    SeparatedDrives = 0;         // insert no separators by default, we do not know where

    ShowSplashScreen = TRUE;

    EnableCustomIconOverlays = TRUE;
    DisabledCustomIconOverlays = NULL;

#ifndef _WIN64
    AddX86OnlyPlugins = FALSE; // FIXME_X64_WINSCP
#endif                         // _WIN64
}

CConfiguration::~CConfiguration()
{
    ClearHistory();
    ClearListOfDisabledCustomIconOverlays();
}

void CConfiguration::ClearHistory()
{
    int i;
    for (i = 0; i < SELECT_HISTORY_SIZE; i++)
    {
        if (SelectHistory[i] != NULL)
        {
            free(SelectHistory[i]);
            SelectHistory[i] = NULL;
        }
    }

    for (i = 0; i < COPY_HISTORY_SIZE; i++)
    {
        if (CopyHistory[i] != NULL)
        {
            free(CopyHistory[i]);
            CopyHistory[i] = NULL;
        }
    }

    for (i = 0; i < EDIT_HISTORY_SIZE; i++)
    {
        if (EditHistory[i] != NULL)
        {
            free(EditHistory[i]);
            EditHistory[i] = NULL;
        }
    }

    for (i = 0; i < CHANGEDIR_HISTORY_SIZE; i++)
    {
        if (ChangeDirHistory[i] != NULL)
        {
            free(ChangeDirHistory[i]);
            ChangeDirHistory[i] = NULL;
        }
    }

    for (i = 0; i < FILELIST_HISTORY_SIZE; i++)
    {
        if (FileListHistory[i] != NULL)
        {
            free(FileListHistory[i]);
            FileListHistory[i] = NULL;
        }
    }

    for (i = 0; i < CREATEDIR_HISTORY_SIZE; i++)
    {
        if (CreateDirHistory[i] != NULL)
        {
            free(CreateDirHistory[i]);
            CreateDirHistory[i] = NULL;
        }
    }

    for (i = 0; i < QUICKRENAME_HISTORY_SIZE; i++)
    {
        if (QuickRenameHistory[i] != NULL)
        {
            free(QuickRenameHistory[i]);
            QuickRenameHistory[i] = NULL;
        }
    }

    for (i = 0; i < EDITNEW_HISTORY_SIZE; i++)
    {
        if (EditNewHistory[i] != NULL)
        {
            free(EditNewHistory[i]);
            EditNewHistory[i] = NULL;
        }
    }

    for (i = 0; i < CONVERT_HISTORY_SIZE; i++)
    {
        if (ConvertHistory[i] != NULL)
        {
            free(ConvertHistory[i]);
            ConvertHistory[i] = NULL;
        }
    }

    for (i = 0; i < FILTER_HISTORY_SIZE; i++)
    {
        if (FilterHistory[i] != NULL)
        {
            free(FilterHistory[i]);
            FilterHistory[i] = NULL;
        }
    }
}

BOOL CConfiguration::PrepareRecycleMasks(int& errorPos)
{
    return RecycleMasks.PrepareMasks(errorPos);
}

BOOL CConfiguration::AgreeRecycleMasks(const char* fileName, const char* fileExt)
{
    return RecycleMasks.AgreeMasks(fileName, fileExt);
}

int CConfiguration::GetMainWindowIconIndex()
{
    int index = MainWindowIconIndexForced != -1 ? MainWindowIconIndexForced : MainWindowIconIndex;
    if (index >= 0 && index < MAINWINDOWICONS_COUNT)
        return index;
    else
        return MAINWINDOWICON_DEFAULT_INDEX; // default Samandarin icon
}

//
// ****************************************************************************
// CConfigurationDlg
//

const int ConfigurationDialogDefaultWidthExtra = 50;
const int ConfigurationViewsDefaultAvailableColumnsWidthExtra = 50;

CConfigurationDlg::CConfigurationDlg(HWND parent, CUserMenuItems* userMenuItems,
                                     int mode, int param)
    : CTreePropDialog(parent, HLanguage, LoadStr(IDS_CONFIGURATION),
                      mode == 0 ? Configuration.LastFocusedPage : mode == 1 ? 15
                                                              : mode == 2   ? 14
                                                              : mode == 3   ? 22
                                                              : mode == 4   ? 13
                                                              : mode == 5   ? 1
                                                                            : 12 /* mode == 6 */,
                      PSH_NOAPPLYNOW | PSH_HASHELP,
                      &Configuration.LastFocusedPage,
                      &Configuration.ConfigurationHeight,
                      &Configuration.ConfigurationWidth,
                      &Configuration.ConfigurationTreeWidth,
                      ConfigurationDialogDefaultWidthExtra),
      TabsPageVisible(TRUE),
      PageView(mode == 4 ? param : -1), //-1 = active panel
      PageUserMenu(userMenuItems),
      PageHotPath(mode == 1 ? TRUE : FALSE, param),
      PageDrives(mode == 6 ? TRUE : FALSE),
      Page14(TRUE)
{
    HOldPluginMsgBoxParent = NULL;
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // when the page order changes, update the constructor
    // mode == 0 ? Configuration.LastFocusedPage : 4
    // in 1.6b2 this fooled me
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    /*00*/ Add(&PageGeneral);       // General
    /*01*/ Add(&PagePanels);        // Panels
    /*02*/ Add(&PageTabs);          // Tabs
    /*03*/ Add(&PageHistory);       // History
    /*04*/ Add(&PageSystem);        // System
    /*05*/ Add(&PageRegional);      // Regional
    /*06*/ Add(&PageMainWindow);    // MainWindow
    /*07*/ Add(&PageAppear);        // Appearance
    /*08*/ Add(&PageColors);        // Colors
    /*09*/ Add(&PageKeyboard);      // Keyboard
    /*10*/ Add(&PageConfirmations); // Confirmations
    /*11*/ Add(&PageChangeDrive);   // Change Drive Menu
    /*12*/ Add(&PageDrives);        // Drives
    /*13*/ Add(&PageView);          // Views
    /*14*/ Add(&PageUserMenu);      // User Menu
    /*15*/ Add(&PageHotPath);       // Hot Paths
    /*16*/ Add(&PageSecurity);      // Security
    /*17*/ Add(&PageIconOvrls);     // Icon Overlays
    /*18*/ Add(&PageViewEdit, NULL, &Configuration.ViewersAndEditorsExpanded);
    /*19*/ Add(&Page13, &PageViewEdit);
    /*20*/ Add(&Page14, &PageViewEdit);
    /*21*/ Add(&Page15, &PageViewEdit);
    /*22*/ Add(&PageViewer, &PageViewEdit);
    /*23*/ Add(&PagePP, NULL, &Configuration.PackersAndUnpackersExpanded);
    /*24*/ Add(&PageP4, &PagePP);
    /*25*/ Add(&PageP3, &PagePP);
    /*26*/ Add(&PageP1, &PagePP);
    /*27*/ Add(&PageP2, &PagePP);
    /*27*/ //  Add(&PageShellExtensions);
           // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
           // when the page order changes, update the constructor
           // mode == 0 ? Configuration.LastFocusedPage : 4
           // in 1.6b2 this fooled me
           // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

void CConfigurationDlg::EnsureTabsPageVisibility(BOOL showTabs)
{
    HWND tree = GetTreeViewHandle();
    if (tree == NULL)
        return;

    if (showTabs)
    {
        if (TabsPageVisible && GetPageTreeItem(&PageTabs) != NULL)
            return;

        TVINSERTSTRUCT tvis;
        ZeroMemory(&tvis, sizeof(tvis));
        tvis.hParent = NULL;
        HTREEITEM panelsItem = GetPageTreeItem(&PagePanels);
        tvis.hInsertAfter = panelsItem != NULL ? panelsItem : TVI_LAST;
        tvis.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
        const TCHAR* tabsTitle = GetPageTitle(&PageTabs);
        tvis.item.pszText = (TCHAR*)tabsTitle;
        tvis.item.cchTextMax = (tabsTitle != NULL) ? (int)_tcslen(tabsTitle) : 0;
        tvis.item.lParam = (LPARAM)&PageTabs;

        HTREEITEM item = TreeView_InsertItem(tree, &tvis);
        if (item != NULL)
        {
            SetPageTreeItem(&PageTabs, item);
            TabsPageVisible = TRUE;
        }
        return;
    }

    if (!TabsPageVisible && GetPageTreeItem(&PageTabs) == NULL)
        return;

    HTREEITEM tabsItem = GetPageTreeItem(&PageTabs);
    if (tabsItem != NULL)
    {
        HTREEITEM selection = TreeView_GetSelection(tree);
        if (selection == tabsItem)
        {
            HTREEITEM fallback = GetPageTreeItem(&PagePanels);
            if (fallback == tabsItem || fallback == NULL)
                fallback = TreeView_GetRoot(tree);
            if (fallback != NULL && fallback != tabsItem)
                TreeView_SelectItem(tree, fallback);
        }
        TreeView_DeleteItem(tree, tabsItem);
        SetPageTreeItem(&PageTabs, NULL);
    }
    TabsPageVisible = FALSE;
}

void CConfigurationDlg::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // ColorsChanged() calls a plug-in method (when colors change PLUGINEVENT_COLORSCHANGED 
        // is called) -> we must set the parent for their message boxes
        HOldPluginMsgBoxParent = PluginMsgBoxParent;
        PluginMsgBoxParent = Dialog.HWindow;
        MultiMonCenterWindow(Dialog.HWindow, Parent, TRUE);
        EnsureTabsPageVisibility(Configuration.UsePanelTabs != 0);
        break;
    }

    case WM_DESTROY:
    {
        if (GetKeyState(VK_ESCAPE) & 0x8000) // prevent listing interruption in the panel after each ESC
            WaitForESCReleaseBeforeTestingESC = TRUE;

        PluginMsgBoxParent = HOldPluginMsgBoxParent;
        break;
    }

    case WM_WINDOWPOSCHANGED:
    {
        const WINDOWPOS* windowPos = reinterpret_cast<const WINDOWPOS*>(lParam);
        if (windowPos != NULL && (windowPos->flags & SWP_NOSIZE) != 0)
            FlushDWMForInteractiveMove();
        break;
    }

    case WM_SYSCOLORCHANGE:
    {
        int i;
        for (i = 0; i < Count; i++)
        {
            HWND w = At(i)->HWindow;
            if (w != NULL)
                PostMessage(w, WM_SYSCOLORCHANGE, 0, 0);
        }
        break;
    }

    case WM_CFG_UPDATE_TABS_VISIBILITY:
    {
        EnsureTabsPageVisibility(wParam != 0);
        break;
    }
    }
}

static BOOL BrowseConfigurationStorageFile(HWND hParent, char* path, int pathSize)
{
    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));
    char filter[] = "Registration Files (*.reg)\0*.reg\0All Files (*.*)\0*.*\0\0";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hParent;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = path;
    ofn.nMaxFile = pathSize;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "reg";
    return SafeGetSaveFileName(&ofn);
}

//
// ****************************************************************************
// CCfgPageGeneral
//

CCfgPageGeneral::CCfgPageGeneral()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_GENERAL, IDD_CFGPAGE_GENERAL, PSP_USETITLE, NULL)
{
}

void CCfgPageGeneral::Validate(CTransferInfo& ti)
{
    BOOL useTimeRes;
    ti.CheckBox(IDC_TIMERESOLUTION, useTimeRes);
    if (useTimeRes)
    {
        int timerRes;
        ti.EditLine(IDE_TIMERESOLUTION, timerRes);
        if (timerRes < 0 || timerRes > 3600)
        {
            SalMessageBox(HWindow, LoadStr(IDS_BADTIMERESOLUTION), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
            ti.ErrorOn(IDE_TIMERESOLUTION);
        }
    }

    int storageType = Configuration.StorageType;
    ti.RadioButton(IDC_SAVE_TO_REGISTRY, cstRegistry, storageType);
    ti.RadioButton(IDC_SAVE_TO_FILE, cstRegFile, storageType);
    if (storageType == cstRegFile)
    {
        char configPath[SAL_MAX_PATH];
        ti.EditLine(IDC_SAVE_TO_FILE_PATH, configPath, SizeOf(configPath));
        if (configPath[0] == 0)
        {
            SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_FILEPATHERR), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
            ti.ErrorOn(IDC_SAVE_TO_FILE);
            return;
        }

        if (!CanWriteRegStorageFilePath(configPath))
        {
            SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_FILEWRITEERR), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
            ti.ErrorOn(IDC_SAVE_TO_FILE);
            return;
        }
    }
}

void CCfgPageGeneral::Transfer(CTransferInfo& ti)
{
    ti.CheckBox(IDC_AUTOSAVE, Configuration.AutoSave);
    if (ti.Type == ttDataToWindow)
    {
        char configPath[SAL_MAX_PATH];
        configPath[0] = 0;
        CConfigurationStorageType bootstrapType = (CConfigurationStorageType)Configuration.StorageType;
        ConfigurationStorage.LoadStorageTypeBootstrap(bootstrapType, configPath, SizeOf(configPath));
        if (configPath[0] == 0)
            ConfigurationStorage.GetRegFilePath(configPath, SizeOf(configPath));
        ti.EditLine(IDC_SAVE_TO_FILE_PATH, configPath, SizeOf(configPath));
    }
    int oldStorageType = Configuration.StorageType;
    ti.RadioButton(IDC_SAVE_TO_REGISTRY, cstRegistry, Configuration.StorageType);
    ti.RadioButton(IDC_SAVE_TO_FILE, cstRegFile, Configuration.StorageType);
    if (ti.Type == ttDataFromWindow && oldStorageType == Configuration.StorageType && Configuration.StorageType == cstRegFile)
    {
        char configPath[SAL_MAX_PATH];
        ti.EditLine(IDC_SAVE_TO_FILE_PATH, configPath, SizeOf(configPath));
        if (!ConfigurationStorage.SwitchStorageType(cstRegFile, FALSE, configPath))
            SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_MIGRATIONERR), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
    }
    if (ti.Type == ttDataFromWindow && oldStorageType != Configuration.StorageType)
    {
        if (SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_SWITCHCONFIRM), LoadStr(IDS_QUESTION),
                          MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            char configPath[SAL_MAX_PATH];
            ti.EditLine(IDC_SAVE_TO_FILE_PATH, configPath, SizeOf(configPath));
            ConfigurationStorage.Flush();
            if (!ConfigurationStorage.SwitchStorageType((CConfigurationStorageType)Configuration.StorageType, TRUE, configPath))
            {
                Configuration.StorageType = oldStorageType;
                SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_MIGRATIONERR), LoadStr(IDS_ERRORTITLE),
                              MB_OK | MB_ICONEXCLAMATION);
            }
        }
        else
            Configuration.StorageType = oldStorageType;
    }
    ti.CheckBox(IDC_CLOSESHELL, Configuration.CloseShell);
    ti.CheckBox(IDC_CLEARREADONLY, Configuration.ClearReadOnly);
    //  ti.CheckBox(IDC_FASTDIRMOVE, Configuration.FastDirectoryMove);
    ti.CheckBox(IDC_TIMERESOLUTION, Configuration.UseTimeResolution);
    ti.CheckBox(IDC_IGNOREDSTSHIFTS, Configuration.IgnoreDSTShifts);
    ti.EditLine(IDE_TIMERESOLUTION, Configuration.TimeResolution);
    //  ti.CheckBox(IDC_LANTASTICCHECK, Configuration.LantasticCheck);
    ti.CheckBox(IDC_ONLYONEINSTANCE, Configuration.OnlyOneInstance);
    ti.CheckBox(IDC_MINBEEPWHENDONE, Configuration.MinBeepWhenDone);
    ti.CheckBox(IDC_USESALOPEN, Configuration.UseSalOpen);
    ti.CheckBox(IDC_QUICKRENAME_SELALL, Configuration.QuickRenameSelectAll);
    ti.CheckBox(IDC_EDITNEW_SELALL, Configuration.EditNewSelectAll);
    ti.CheckBox(IDC_NETWAREFASTDIRMOVE, Configuration.NetwareFastDirMove);
    int dummy = 0;
    ti.CheckBox(IDC_ASYNCCOPYALG, Windows7AndLater ? Configuration.UseAsyncCopyAlg : dummy);
    int oldReloadEnvVariables = Configuration.ReloadEnvVariables;
    ti.CheckBox(IDC_RELOADENVVARS, Configuration.ReloadEnvVariables);
    ti.CheckBox(IDC_PATHAUTOCOMPLETE, Configuration.PathAutoComplete);
    ti.CheckBox(IDC_CREATEDIR_AUTOCOMPLETE, Configuration.CreateDirAutoComplete);
    if (ti.Type == ttDataFromWindow && Configuration.ReloadEnvVariables && oldReloadEnvVariables != Configuration.ReloadEnvVariables)
    {
        InitEnvironmentVariablesDifferences();
    }

    if (ti.Type == ttDataToWindow)
        EnableControls();
}

BOOL CCfgPageGeneral::IsDefaultCommandShellApplication()
{
    char commandLineApplication[SAL_MAX_PATH];
    lstrcpyn(commandLineApplication, Configuration.CommandLineApplication, SAL_MAX_PATH);

    if (ParentDialog != NULL)
    {
        int i;
        for (i = 0; i < ParentDialog->Count; i++)
        {
            HWND hPage = ParentDialog->At(i)->HWindow;
            if (hPage != NULL)
            {
                HWND hCommandLineApplication = GetDlgItem(hPage, IDC_CMDLINEAPP_PATH);
                if (hCommandLineApplication != NULL)
                {
                    GetWindowText(hCommandLineApplication, commandLineApplication, SizeOf(commandLineApplication));
                    break;
                }
            }
        }
    }

    return commandLineApplication[0] == 0;
}

void CCfgPageGeneral::EnableControls()
{
    BOOL useTimeRes = IsDlgButtonChecked(HWindow, IDC_TIMERESOLUTION);
    EnableWindow(GetDlgItem(HWindow, IDE_TIMERESOLUTION), useTimeRes);
    EnableWindow(GetDlgItem(HWindow, IDC_ASYNCCOPYALG), Windows7AndLater);
    char configPath[SAL_MAX_PATH];
    BOOL canSaveStorageTypeBootstrap = ConfigurationStorage.CanSaveStorageTypeBootstrap();
    BOOL canUseFileStorage = canSaveStorageTypeBootstrap && ConfigurationStorage.GetPortableConfigFilePath(configPath, SizeOf(configPath));
    BOOL fileStorageSelected = IsDlgButtonChecked(HWindow, IDC_SAVE_TO_FILE) == BST_CHECKED;
    EnableWindow(GetDlgItem(HWindow, IDC_SAVE_TO_REGISTRY), canSaveStorageTypeBootstrap);
    EnableWindow(GetDlgItem(HWindow, IDC_SAVE_TO_FILE), canUseFileStorage);
    EnableWindow(GetDlgItem(HWindow, IDC_SAVE_TO_FILE_PATH), canUseFileStorage && fileStorageSelected);
    EnableWindow(GetDlgItem(HWindow, IDC_SAVE_TO_FILE_BROWSE), canUseFileStorage && fileStorageSelected);

    BOOL defaultCommandShell = IsDefaultCommandShellApplication();
    HWND hCloseShell = GetDlgItem(HWindow, IDC_CLOSESHELL);
    SetWindowText(hCloseShell, LoadStr(defaultCommandShell ? IDS_CLOSESHELL_COMMANDLINE : IDS_CLOSESHELL_DEFAULTCMDONLY));
    EnableWindow(hCloseShell, defaultCommandShell);
}

INT_PTR
CCfgPageGeneral::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            DarkModeApplyStaticTextColors(HWindow, NULL);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }
        break;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDC_SAVE_TO_FILE_BROWSE)
        {
            char path[SAL_MAX_PATH];
            GetDlgItemText(HWindow, IDC_SAVE_TO_FILE_PATH, path, SizeOf(path));
            if (BrowseConfigurationStorageFile(HWindow, path, SizeOf(path)))
            {
                if (CanWriteRegStorageFilePath(path))
                    SetDlgItemText(HWindow, IDC_SAVE_TO_FILE_PATH, path);
                else
                    SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_FILEWRITEERR), LoadStr(IDS_ERRORTITLE),
                                  MB_OK | MB_ICONEXCLAMATION);
            }
        }
        else if (HIWORD(wParam) == BN_CLICKED)
            EnableControls();
        break;
    }

    case WM_NOTIFY:
    {
        if (((NMHDR*)lParam)->code == PSN_SETACTIVE)
            EnableControls();
        break;
    }
    }
    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CCfgPageRegional
//

CCfgPageRegional::CCfgPageRegional()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_REGIONAL, IDD_CFGPAGE_REGIONAL, PSP_USETITLE, NULL)
{
    lstrcpy(SLGName, Configuration.SLGName);
    lstrcpy(DirName, Configuration.ConversionTable);
}

void CCfgPageRegional::LoadControls()
{
    CLanguage language;
    if (language.Init(SLGName, NULL))
    {
        char buff[200];
        language.GetLanguageName(buff, 200);
        SetDlgItemText(HWindow, IDE_LANGUAGE, buff);
        language.Free();
    }
}

void CCfgPageRegional::Transfer(CTransferInfo& ti)
{
    if (ti.Type == ttDataToWindow)
    {
        LoadControls();
    }
    else
    {
        if (stricmp(Configuration.SLGName, SLGName) != 0)
        {
            SalMessageBox(HWindow, LoadStr(IDS_LANGUAGE_CHANGE), LoadStr(IDS_INFOTITLE),
                          MB_OK | MB_ICONINFORMATION);
            lstrcpy(Configuration.SLGName, SLGName);
            Configuration.ShowSLGIncomplete = TRUE; // if the language is not complete, show a message at startup (recruit translators)
        }
        // if the table has changed and the old one is already loaded we need to restart Salamander
        if (stricmp(Configuration.ConversionTable, DirName) != 0)
        {
            lstrcpy(Configuration.ConversionTable, DirName);
            if (CodeTables.IsLoaded())
            {
                SalMessageBox(HWindow, LoadStr(IDS_CONVERSION_CHANGE), LoadStr(IDS_INFOTITLE),
                              MB_OK | MB_ICONINFORMATION);
            }
        }
    }
}

INT_PTR
CCfgPageRegional::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        INT_PTR result = 0;
        if (DarkModeTryHandleCtlColorForDialogPage(uMsg, wParam, lParam, result))
            return result;
        break;
    }

    case WM_INITDIALOG:
    {
        if (IsSLGIncomplete[0] != 0)
        {
            new CStaticText(HWindow, IDC_CFGREG_INCOMPLETE_TITLE, STF_BOLD);
            SetDlgItemText(HWindow, IDC_CFGREG_INCOMPLETE_TITLE, LoadStr(IDS_SLGINCOMPLETE_TITLE));
            SetDlgItemText(HWindow, IDC_CFGREG_INCOMPLETE_TEXT, LoadStr(IDS_SLGINCOMPLETE_TEXT));
            SetDlgItemText(HWindow, IDC_CFGREG_INCOMPLETE_URL, IsSLGIncomplete);
            CHyperLink* hl = new CHyperLink(HWindow, IDC_CFGREG_INCOMPLETE_URL);
            hl->SetActionOpen(IsSLGIncomplete);
        }
        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            DarkModeApplyStaticTextColors(HWindow, NULL);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDB_LANGUAGE:
        {
            CLanguageSelectorDialog dlg(HWindow, SLGName, NULL);
            dlg.Initialize();
            if (dlg.Execute() == IDOK)
            {
                LoadControls();
                PostMessage(GetParent(), WM_NEXTDLGCTL, (WPARAM)GetDlgItem(GetParent(), 5 /* _TPD_IDC_OK */), TRUE);
            }
            return 0;
        }

        case IDB_CONVERSION:
        {
            CConversionTablesDialog dlg(HWindow, DirName);
            dlg.Execute();
            return 0;
        }
        }
        break;
    }
    }
    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CCfgPageView
//

// used only to suppress quick search in the list view
class CMyListView : public CWindow
{
public:
    CMyListView(HWND hDlg, int ctrlID) : CWindow(hDlg, ctrlID) {}

protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_CHAR)
        {
            PostMessage(GetParent(HWindow), WM_USER_CHAR, wParam, lParam);
            return 0;
        }

        if (uMsg == WM_LBUTTONDBLCLK)
        {
            int index = ListView_GetNextItem(HWindow, -1, LVNI_SELECTED);
            ListView_EditLabel(HWindow, index);
            return 0;
        }

        return CWindow::WindowProc(uMsg, wParam, lParam);
    }
};

CCfgPageView::CCfgPageView(int index)
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_VIEWS, IDD_CFGPAGE_VIEWS, PSP_USETITLE, NULL)
{
    Dirty = FALSE;
    Header = NULL;
    HListView = NULL;
    Header2 = NULL;
    HListView2 = NULL;
    HAvailableColumnsImageList = NULL;
    HAvailableColumnsFilter = NULL;
    AvailableColumnsFilterVisible = FALSE;
    AvailableColumnsFilterText[0] = 0;
    DisableNotification = FALSE;
    LabelEdit = FALSE;
    AvailableColumnsWidth = 0;
    AvailableColumnsRightMargin = 0;
    ViewsListGap = 0;
    ViewsSplitterDrag = FALSE;
    if (index == -1)
        index = MainWindow->GetActivePanel()->GetViewTemplateIndex();
    SelectIndex = index;
}

BOOL CCfgPageView::IsDirty()
{
    return Dirty;
}

void CCfgPageView::Transfer(CTransferInfo& ti)
{
    if (ti.Type == ttDataToWindow)
    {
        Config.Load(MainWindow->ViewTemplates);

        DisableNotification = TRUE;
        char hotKey[20];
        int i;
        for (i = 0; i < VIEW_TEMPLATES_COUNT; i++)
        {
            LVITEM lvi;
            lvi.mask = LVIF_TEXT | LVIF_STATE;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            lvi.state = 0;
            lvi.pszText = Config.Items[i].Name;
            ListView_InsertItem(HListView, &lvi);

            char* modeName = NULL;
            switch (Config.Items[i].Mode)
            {
            case VIEW_MODE_TREE:
                modeName = LoadStr(IDS_TREE_VIEW_NAME);
                break;
            case VIEW_MODE_BRIEF:
                modeName = LoadStr(IDS_BRIEF_VIEW_NAME);
                break;
            case VIEW_MODE_DETAILED:
                modeName = LoadStr(IDS_DETAILED_VIEW_NAME);
                break;
            case VIEW_MODE_ICONS:
                modeName = LoadStr(IDS_ICONS_VIEW_NAME);
                break;
            case VIEW_MODE_THUMBNAILS:
                modeName = LoadStr(IDS_THUMBNAILS_VIEW_NAME);
                break;
            case VIEW_MODE_TILES:
                modeName = LoadStr(IDS_TILES_VIEW_NAME);
                break;
            }
            ListView_SetItemText(HListView, i, 1, modeName != NULL ? modeName : (char*)"");

            sprintf(hotKey, "Alt+%d", i < VIEW_TEMPLATES_COUNT - 1 ? i + 1 : 0);
            ListView_SetItemText(HListView, i, 2, hotKey);
        }
        // set column widths
        ListView_SetColumnWidth(HListView, 0, LVSCW_AUTOSIZE_USEHEADER);
        int w = ListView_GetColumnWidth(HListView, 0);
        w += 30;
        ListView_SetColumnWidth(HListView, 0, w);
        ListView_SetColumnWidth(HListView, 1, LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnWidth(HListView, 2, LVSCW_AUTOSIZE_USEHEADER);

        DWORD state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(HListView, SelectIndex, state, state);
        DisableNotification = FALSE;
        LoadControls();
        EnableHeader();
    }
    else
    {
        MainWindow->ViewTemplates.Load(Config);
    }
}

void CCfgPageView::Validate(CTransferInfo& ti)
{
}

const int CFGP2ItemsCount = 9;
const int CFGP2Flags[CFGP2ItemsCount] = {0, VIEW_SHOW_EXTENSION, VIEW_SHOW_DOSNAME, VIEW_SHOW_SIZE, VIEW_SHOW_TYPE, VIEW_SHOW_DATE, VIEW_SHOW_TIME, VIEW_SHOW_ATTRIBUTES, VIEW_SHOW_DESCRIPTION};
const int CFGP2ResID[CFGP2ItemsCount] = {IDS_COLUMN_CFG_NAME, IDS_COLUMN_CFG_EXT, IDS_COLUMN_CFG_DOSNAME, IDS_COLUMN_CFG_SIZE, IDS_COLUMN_CFG_TYPE, IDS_COLUMN_CFG_DATE, IDS_COLUMN_CFG_TIME, IDS_COLUMN_CFG_ATTR, IDS_COLUMN_CFG_DESC};
const int CFGP2ExplorerColumnsStart = CFGP2ItemsCount;
const int CFGP2MinDefinedViewsWidth = 160;
const int CFGP2MinAvailableColumnsWidth = 120;

static int GetAvailableColumnIndex(HWND listView, int item)
{
    LVITEM lvi;
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_PARAM;
    lvi.iItem = item;
    return ListView_GetItem(listView, &lvi) ? (int)lvi.lParam : -1;
}

static void InitColumnOrder(BYTE* order)
{
    BOOL used[CFGP2ItemsCount];
    ZeroMemory(used, sizeof(used));
    for (int i = 0; i < CFGP2ItemsCount; i++)
    {
        if (order[i] < CFGP2ItemsCount && !used[order[i]])
            used[order[i]] = TRUE;
        else
            order[i] = 0xff;
    }
    int next = 0;
    for (int i = 0; i < CFGP2ItemsCount; i++)
    {
        if (order[i] == 0xff)
        {
            while (next < CFGP2ItemsCount && used[next])
                next++;
            order[i] = (BYTE)next;
            used[next] = TRUE;
        }
    }
}

static void InitExplorerColumnOrder(WORD* order)
{
    BOOL used[EXPLORER_COLUMNS_COUNT];
    ZeroMemory(used, sizeof(used));
    for (int i = 0; i < EXPLORER_COLUMNS_COUNT; i++)
    {
        if (order[i] < EXPLORER_COLUMNS_COUNT && !used[order[i]])
            used[order[i]] = TRUE;
        else
            order[i] = 0xffff;
    }
    int next = 0;
    for (int i = 0; i < EXPLORER_COLUMNS_COUNT; i++)
    {
        if (order[i] == 0xffff)
        {
            while (next < EXPLORER_COLUMNS_COUNT && used[next])
                next++;
            if (next < EXPLORER_COLUMNS_COUNT)
            {
                order[i] = (WORD)next;
                used[next] = TRUE;
            }
        }
    }
}


void CCfgPageView::LayoutViewsListControls()
{
    if (HListView == NULL || HListView2 == NULL || Header == NULL || Header2 == NULL ||
        AvailableColumnsWidth <= 0)
    {
        return;
    }

    RECT client;
    GetClientRect(HWindow, &client);

    RECT leftRect;
    RECT rightRect;
    RECT headerRect;
    RECT header2Rect;
    GetWindowRect(HListView, &leftRect);
    GetWindowRect(HListView2, &rightRect);
    GetWindowRect(Header->HWindow, &headerRect);
    GetWindowRect(Header2->HWindow, &header2Rect);
    POINT leftTop = {leftRect.left, leftRect.top};
    POINT rightTop = {rightRect.left, rightRect.top};
    ScreenToClient(HWindow, &leftTop);
    ScreenToClient(HWindow, &rightTop);

    int rightLeft = client.right - AvailableColumnsRightMargin - AvailableColumnsWidth;
    if (rightLeft < CFGP2MinDefinedViewsWidth)
        rightLeft = CFGP2MinDefinedViewsWidth;
    if (rightLeft > client.right - AvailableColumnsRightMargin - CFGP2MinAvailableColumnsWidth)
        rightLeft = client.right - AvailableColumnsRightMargin - CFGP2MinAvailableColumnsWidth;
    AvailableColumnsWidth = client.right - AvailableColumnsRightMargin - rightLeft;

    int leftWidth = rightLeft - ViewsListGap - leftTop.x;
    if (leftWidth < 20)
        leftWidth = 20;
    const int listHeight = rightRect.bottom - rightRect.top;
    const int leftHeight = leftRect.bottom - leftRect.top;
    const int headerHeight = headerRect.bottom - headerRect.top;
    const int header2Height = header2Rect.bottom - header2Rect.top;

    HDWP hdwp = HANDLES(BeginDeferWindowPos(5));
    if (hdwp != NULL)
    {
        hdwp = HANDLES(DeferWindowPos(hdwp, HListView, NULL,
                                      0, 0, leftWidth, leftHeight,
                                      SWP_NOMOVE | SWP_NOZORDER));
        hdwp = HANDLES(DeferWindowPos(hdwp, HListView2, NULL,
                                      rightLeft, rightTop.y, AvailableColumnsWidth, listHeight,
                                      SWP_NOZORDER));
        hdwp = HANDLES(DeferWindowPos(hdwp, Header->HWindow, NULL,
                                      0, 0, leftWidth, headerHeight,
                                      SWP_NOMOVE | SWP_NOZORDER));
        hdwp = HANDLES(DeferWindowPos(hdwp, Header2->HWindow, NULL,
                                      rightLeft, rightTop.y - header2Height,
                                      AvailableColumnsWidth, header2Height,
                                      SWP_NOZORDER));
        if (HAvailableColumnsFilter != NULL)
        {
            int listHeaderHeight = header2Height;
            HWND listHeader = ListView_GetHeader(HListView2);
            if (listHeader != NULL)
            {
                RECT listHeaderRect;
                GetWindowRect(listHeader, &listHeaderRect);
                listHeaderHeight = listHeaderRect.bottom - listHeaderRect.top;
            }

            const int filterLeftOffset = 100;
            const int filterMargin = 4;
            int filterWidth = AvailableColumnsWidth - filterLeftOffset - GetSystemMetrics(SM_CXVSCROLL) - filterMargin;
            if (filterWidth < 20)
                filterWidth = 20;
            int filterHeight = listHeaderHeight - 4;
            if (filterHeight < 10)
                filterHeight = 10;
            hdwp = HANDLES(DeferWindowPos(hdwp, HAvailableColumnsFilter, HWND_TOP,
                                          rightLeft + filterLeftOffset, rightTop.y + 2,
                                          filterWidth, filterHeight,
                                          AvailableColumnsFilterVisible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
        }
        HANDLES(EndDeferWindowPos(hdwp));
    }

    SetViewsAvailableColumnsColumnWidth(HListView2);
    InvalidateRect(HListView, NULL, TRUE);
    InvalidateRect(HListView2, NULL, TRUE);
}

BOOL CCfgPageView::IsAvailableColumnsFilterActive()
{
    return AvailableColumnsFilterVisible && AvailableColumnsFilterText[0] != 0;
}

BOOL CCfgPageView::AvailableColumnMatchesFilter(const char* text)
{
    if (!IsAvailableColumnsFilterActive())
        return TRUE;
    if (text == NULL)
        return FALSE;

    char filter[100];
    lstrcpyn(filter, AvailableColumnsFilterText, _countof(filter));
    CharLowerBuffA(filter, lstrlen(filter));

    char name[COLUMN_DESCRIPTION_MAX];
    lstrcpyn(name, text, _countof(name));
    CharLowerBuffA(name, lstrlen(name));
    return strstr(name, filter) != NULL;
}

void CCfgPageView::ApplyAvailableColumnsFilter()
{
    if (HAvailableColumnsFilter != NULL)
        GetWindowText(HAvailableColumnsFilter, AvailableColumnsFilterText, _countof(AvailableColumnsFilterText));
    LoadControls();
    EnableHeader();
}

void CCfgPageView::ToggleAvailableColumnsFilter()
{
    AvailableColumnsFilterVisible = !AvailableColumnsFilterVisible;
    if (!AvailableColumnsFilterVisible)
        AvailableColumnsFilterText[0] = 0;

    if (HAvailableColumnsFilter != NULL)
    {
        DisableNotification = TRUE;
        SetWindowText(HAvailableColumnsFilter, AvailableColumnsFilterText);
        ShowWindow(HAvailableColumnsFilter, AvailableColumnsFilterVisible ? SW_SHOW : SW_HIDE);
        DisableNotification = FALSE;
        if (AvailableColumnsFilterVisible)
            SetFocus(HAvailableColumnsFilter);
    }
    ApplyAvailableColumnsFilter();
    LayoutViewsListControls();
}

void CCfgPageView::LoadControls()
{
    DisableNotification = TRUE;
    int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);

    BOOL checked[CFGP2ItemsCount];

    BOOL empty = TRUE;
    if (index >= 2 && index != 3 && index != 4 && index != 5) // tree a brief ma disabled checkboxy
    {
        checked[0] = TRUE;
        int i;
        for (i = 1; i < CFGP2ItemsCount; i++)
            checked[i] = (Config.Items[index].Flags & CFGP2Flags[i]) ? TRUE : FALSE;
        empty = FALSE;
    }
    DWORD count = ListView_GetItemCount(HListView2);
    if (count > 0)
        ListView_DeleteAllItems(HListView2);
    if (!empty)
    {
        InitColumnOrder(Config.Items[index].ColumnOrder);
        int i;
        for (i = 0; i < CFGP2ItemsCount; i++)
        {
            int columnIndex = Config.Items[index].ColumnOrder[i];
            const char* columnName = LoadStr(CFGP2ResID[columnIndex]);
            if (!AvailableColumnMatchesFilter(columnName))
                continue;
            LVITEM lvi;
            lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM | LVIF_IMAGE;
            lvi.iImage = I_IMAGENONE;
            lvi.iItem = ListView_GetItemCount(HListView2);
            lvi.iSubItem = 0;
            lvi.state = 0;
            lvi.lParam = columnIndex;
            lvi.pszText = (char*)columnName;
            ListView_InsertItem(HListView2, &lvi);
        }
        InitExplorerColumnOrder(Config.Items[index].ExplorerColumnOrder);
        int explorerColumnsCount = GetExplorerColumnCount();
        for (i = 0; i < explorerColumnsCount; i++)
        {
            int explorerIndex = Config.Items[index].ExplorerColumnOrder[i];
            LVITEM lvi;
            lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM | LVIF_IMAGE;
            lvi.iImage = 0;
            if (!AvailableColumnMatchesFilter(GetExplorerColumnName(explorerIndex)))
                continue;
            lvi.iItem = ListView_GetItemCount(HListView2);
            lvi.iSubItem = 0;
            lvi.state = 0;
            lvi.lParam = -(explorerIndex + 1);
            lvi.pszText = (char*)GetExplorerColumnName(explorerIndex);
            ListView_InsertItem(HListView2, &lvi);
        }
        ListView_SetItemState(HListView2, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        SetViewsAvailableColumnsColumnWidth(HListView2);
    }
    int index2 = -1;
    if (!empty)
    {
        int i;
        for (i = 0; i < ListView_GetItemCount(HListView2); i++)
        {
            int columnIndex = GetAvailableColumnIndex(HListView2, i);
            BOOL isChecked = FALSE;
            if (columnIndex >= 0)
                isChecked = checked[columnIndex];
            else
            {
                int explorerIndex = -columnIndex - 1;
                isChecked = explorerIndex >= 0 && explorerIndex < EXPLORER_COLUMNS_COUNT && Config.Items[index].ExplorerColumnVisible[explorerIndex];
            }
            UINT state = INDEXTOSTATEIMAGEMASK((isChecked ? 2 : 1));
            ListView_SetItemState(HListView2, i, state, LVIS_STATEIMAGEMASK);
        }
        CTransferInfo ti(HWindow, ttDataToWindow);
        int tmp = Config.Items[index].LeftSmartMode;
        ti.CheckBox(IDC_VIEW_LEFT_SMARTMODE, tmp);
        tmp = Config.Items[index].RightSmartMode;
        ti.CheckBox(IDC_VIEW_RIGHT_SMARTMODE, tmp);
        index2 = ListView_GetNextItem(HListView2, -1, LVNI_SELECTED);
        if (index2 != -1)
        {
            int columnIndex = GetAvailableColumnIndex(HListView2, index2);
            CColumnConfig* columnConfig = columnIndex >= 0 ? &Config.Items[index].Columns[columnIndex] : &Config.Items[index].ExplorerColumns[-columnIndex - 1];
            tmp = columnConfig->LeftFixedWidth;
            ti.CheckBox(IDC_VIEW_LEFT_FIXED, tmp);
            tmp = columnConfig->RightFixedWidth;
            ti.CheckBox(IDC_VIEW_RIGHT_FIXED, tmp);
            tmp = columnConfig->LeftWidth;
            ti.EditLine(IDC_VIEW_LEFT_WIDTH, tmp);
            tmp = columnConfig->RightWidth;
            ti.EditLine(IDC_VIEW_RIGHT_WIDTH, tmp);
        }
    }

    EnableControls();
    DisableNotification = FALSE;
}

void CCfgPageView::StoreControls()
{
    int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
    if (IsAvailableColumnsFilterActive())
        return;
    if (index >= 2)
    {
        DWORD flags = 0;
        int i;
        ZeroMemory(Config.Items[index].ExplorerColumnVisible, sizeof(Config.Items[index].ExplorerColumnVisible));
        for (i = 0; i < CFGP2ItemsCount; i++)
        {
            int columnIndex = GetAvailableColumnIndex(HListView2, i);
            Config.Items[index].ColumnOrder[i] = (BYTE)columnIndex;
            if (columnIndex > 0)
            {
                DWORD state = ListView_GetItemState(HListView2, i, LVIS_STATEIMAGEMASK);
                if (state == INDEXTOSTATEIMAGEMASK(2))
                    flags |= CFGP2Flags[columnIndex];
            }
        }
        int count = ListView_GetItemCount(HListView2);
        for (i = CFGP2ExplorerColumnsStart; i < count; i++)
        {
            int columnIndex = GetAvailableColumnIndex(HListView2, i);
            if (columnIndex < 0)
            {
                int explorerIndex = -columnIndex - 1;
                if (explorerIndex >= 0 && explorerIndex < EXPLORER_COLUMNS_COUNT)
                {
                    DWORD state = ListView_GetItemState(HListView2, i, LVIS_STATEIMAGEMASK);
                    Config.Items[index].ExplorerColumnVisible[explorerIndex] = state == INDEXTOSTATEIMAGEMASK(2);
                    Config.Items[index].ExplorerColumnOrder[i - CFGP2ExplorerColumnsStart] = (WORD)explorerIndex;
                }
            }
        }
        Config.Items[index].Flags = flags;
        Dirty = TRUE;
    }
}

void CCfgPageView::EnableControls()
{
    int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
    BOOL enable = TRUE;
    if (index == -1 || index < 2 || index == 3 || index == 4 || index == 5 || strlen(Config.Items[index].Name) == 0)
        enable = FALSE;

    int index2 = ListView_GetNextItem(HListView2, -1, LVNI_SELECTED);
    BOOL supportFixedWidth = (index2 != -1 && enable);

    EnableWindow(HListView2, enable);
    CTransferInfo ti(HWindow, ttDataToWindow);
    int tmp = 0;
    if (!supportFixedWidth)
    {
        ti.CheckBox(IDC_VIEW_LEFT_FIXED, tmp);
        ti.CheckBox(IDC_VIEW_RIGHT_FIXED, tmp);
        char buff[] = "";
        ti.EditLine(IDC_VIEW_LEFT_WIDTH, buff, 1);
        ti.EditLine(IDC_VIEW_RIGHT_WIDTH, buff, 1);
    }
    if (!enable)
    {
        ti.CheckBox(IDC_VIEW_LEFT_SMARTMODE, tmp);
        ti.CheckBox(IDC_VIEW_RIGHT_SMARTMODE, tmp);
    }
    BOOL enableLeftEdit = supportFixedWidth && IsDlgButtonChecked(HWindow, IDC_VIEW_LEFT_FIXED) == BST_CHECKED;
    BOOL enableRightEdit = supportFixedWidth && IsDlgButtonChecked(HWindow, IDC_VIEW_RIGHT_FIXED) == BST_CHECKED;

    const BOOL keepStaticTextReadable = DarkModeShouldUseDarkColors();
    EnableWindow(GetDlgItem(HWindow, IDC_VIEW_TEXT4), keepStaticTextReadable || enable);
    EnableWindow(GetDlgItem(HWindow, IDC_VIEW_TEXT6), keepStaticTextReadable || enable);
    EnableWindow(GetDlgItem(HWindow, IDC_VIEW_LEFT_FIXED), supportFixedWidth);
    EnableWindow(GetDlgItem(HWindow, IDC_VIEW_RIGHT_FIXED), supportFixedWidth);
    EnableWindow(GetDlgItem(HWindow, IDC_VIEW_LEFT_WIDTH), enableLeftEdit);
    EnableWindow(GetDlgItem(HWindow, IDC_VIEW_RIGHT_WIDTH), enableRightEdit);
    EnableWindow(GetDlgItem(HWindow, IDC_VIEW_TEXT2), keepStaticTextReadable || supportFixedWidth);
    EnableWindow(GetDlgItem(HWindow, IDC_VIEW_TEXT5), keepStaticTextReadable || supportFixedWidth);
    EnableWindow(GetDlgItem(HWindow, IDC_VIEW_TEXT7), keepStaticTextReadable || enable);
    EnableWindow(GetDlgItem(HWindow, IDC_VIEW_LEFT_SMARTMODE), enable);
    EnableWindow(GetDlgItem(HWindow, IDC_VIEW_RIGHT_SMARTMODE), enable);
}

DWORD
CCfgPageView::GetEnabledFunctions()
{
    DWORD mask = 0;
    if (!LabelEdit)
    {
        int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
        if (index > 6)
        {
            mask |= TLBHDRMASK_MODIFY;
            if (lstrlen(Config.Items[index].Name) > 0)
            {
                mask |= TLBHDRMASK_DELETE;
                /* view up/down buttons are intentionally hidden */
            }
        }
    }
    return mask;
}

void CCfgPageView::EnableHeader()
{
    Header->EnableToolbar(GetEnabledFunctions());
    int viewIndex = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
    int colIndex = ListView_GetNextItem(HListView2, -1, LVNI_SELECTED);
    DWORD mask = TLBHDRMASK_FILTER;
    if (!LabelEdit && !IsAvailableColumnsFilterActive() && viewIndex >= 2 && viewIndex != 3 && viewIndex != 4 && viewIndex != 5 && colIndex != -1)
    {
        int selectedColumnIndex = GetAvailableColumnIndex(HListView2, colIndex);
        if (selectedColumnIndex >= 0)
        {
            if (colIndex > 1 && GetAvailableColumnIndex(HListView2, colIndex - 1) >= 0)
                mask |= TLBHDRMASK_UP | TLBHDRMASK_TOP;
            if (colIndex > 0 && colIndex < CFGP2ItemsCount - 1 && GetAvailableColumnIndex(HListView2, colIndex + 1) >= 0)
                mask |= TLBHDRMASK_DOWN | TLBHDRMASK_BOTTOM;
        }
        else
        {
            if (colIndex > CFGP2ExplorerColumnsStart && GetAvailableColumnIndex(HListView2, colIndex - 1) < 0)
                mask |= TLBHDRMASK_UP | TLBHDRMASK_TOP;
            if (colIndex < ListView_GetItemCount(HListView2) - 1 && GetAvailableColumnIndex(HListView2, colIndex + 1) < 0)
                mask |= TLBHDRMASK_DOWN | TLBHDRMASK_BOTTOM;
        }
    }
    Header2->EnableToolbar(mask);
    Header2->CheckToolbar(AvailableColumnsFilterVisible ? TLBHDRMASK_FILTER : 0);
}

void CCfgPageView::OnModify()
{
    if ((GetEnabledFunctions() & TLBHDRMASK_MODIFY) == 0)
        return;
    Dirty = TRUE;
    int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
    if (index != -1)
        PostMessage(HListView, LVM_EDITLABEL, index, 0);
}

void CCfgPageView::OnDelete()
{
    if ((GetEnabledFunctions() & TLBHDRMASK_DELETE) == 0)
        return;
    Dirty = TRUE;
    int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
    if (index != -1)
    {
        Config.Items[index].Name[0] = 0;
        char buff[] = "";
        ListView_SetItemText(HListView, index, 0, buff);
        LoadControls();
    }
    EnableHeader();
}

void CCfgPageView::OnMove(BOOL up)
{
    CALL_STACK_MESSAGE2("CCfgPageView::OnMove(%d)", up);
    DWORD mask = GetEnabledFunctions();
    if (up && (mask & TLBHDRMASK_UP) == 0 ||
        !up && (mask & TLBHDRMASK_DOWN) == 0)
        return;
    Dirty = TRUE;
    DisableNotification = TRUE;
    int index1 = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
    int index2 = index1;
    if (index1 == -1)
        return;
    if (up && index1 > 0)
        index2 = index1 - 1;
    if (!up && index1 < VIEW_TEMPLATES_COUNT - 1)
        index2 = index1 + 1;
    if (index2 != index1)
    {
        ListView_SetItemText(HListView, index1, 0, Config.Items[index2].Name);
        ListView_SetItemText(HListView, index2, 0, Config.Items[index1].Name);
        DWORD state1 = ListView_GetItemState(HListView, index1, LVIS_STATEIMAGEMASK);
        DWORD state2 = ListView_GetItemState(HListView, index2, LVIS_STATEIMAGEMASK);
        ListView_SetItemState(HListView, index1, state2, LVIS_STATEIMAGEMASK);
        state1 |= LVIS_FOCUSED | LVIS_SELECTED;
        ListView_SetItemState(HListView, index2, state1, LVIS_STATEIMAGEMASK | LVIS_FOCUSED | LVIS_SELECTED);
        Config.SwapItems(index1, index2);
        LoadControls();
    }
    DisableNotification = FALSE;
    EnableHeader();
}

INT_PTR
CCfgPageView::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CCfgPageView::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        INT_PTR result = 0;
        if (DarkModeTryHandleCtlColorForDialogPage(uMsg, wParam, lParam, result))
            return result;
        break;
    }

    case WM_INITDIALOG:
    {
        CMyListView* listView = new CMyListView(HWindow, IDC_VIEW_LIST);
        HListView = listView->HWindow;
        HListView2 = GetDlgItem(HWindow, IDC_VIEW_LIST2);
        Header = new CToolbarHeader(HWindow, IDC_VIEWLIST_HEADER, HListView,
                                    TLBHDRMASK_MODIFY | TLBHDRMASK_DELETE);
        Header2 = new CToolbarHeader(HWindow, IDC_VIEWLIST_HEADER2, HListView2, TLBHDRMASK_TOP | TLBHDRMASK_UP | TLBHDRMASK_DOWN | TLBHDRMASK_BOTTOM | TLBHDRMASK_FILTER);
        HAvailableColumnsFilter = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
                                                 WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                                 0, 0, 0, 0, HWindow,
                                                 (HMENU)IDC_VIEW_COLUMNS_FILTER, HInstance, NULL);
        if (HAvailableColumnsFilter != NULL)
        {
            SendMessage(HAvailableColumnsFilter, WM_SETFONT, SendMessage(HListView2, WM_GETFONT, 0, 0), TRUE);
            ShowWindow(HAvailableColumnsFilter, SW_HIDE);
        }

        DWORD exFlags = LVS_EX_FULLROWSELECT /*| LVS_EX_CHECKBOXES*/;
        DWORD origFlags = ListView_GetExtendedListViewStyle(HListView);
        ListView_SetExtendedListViewStyle(HListView, origFlags | exFlags); // 4.71

        exFlags = LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
        origFlags = ListView_GetExtendedListViewStyle(HListView2);
        ListView_SetExtendedListViewStyle(HListView2, origFlags | exFlags); // 4.71

        // determine the list view size
        RECT r;
        GetClientRect(HListView, &r);

        // fill the list view with Name, Mode and HotKey columns
        LVCOLUMN lvc;
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        lvc.pszText = LoadStr(IDS_HOTPATH_NAME);
        lvc.cx = 1; // dummy
        lvc.fmt = LVCFMT_LEFT;
        lvc.iSubItem = 0;
        ListView_InsertColumn(HListView, 0, &lvc);

        lvc.mask |= LVCF_SUBITEM;
        lvc.pszText = LoadStr(IDS_VIEW_MODE);
        lvc.iSubItem = 1;
        ListView_InsertColumn(HListView, 1, &lvc);
        ListView_SetColumnWidth(HListView, 1, LVSCW_AUTOSIZE_USEHEADER);

        lvc.mask |= LVCF_SUBITEM;
        lvc.pszText = LoadStr(IDS_HOTPATH_HOTKEY);
        lvc.iSubItem = 2;
        ListView_InsertColumn(HListView, 2, &lvc);
        ListView_SetColumnWidth(HListView, 2, LVSCW_AUTOSIZE_USEHEADER);

        int iconSize = GetIconSizeForSystemDPI(ICONSIZE_16);
        HAvailableColumnsImageList = ImageList_Create(iconSize, iconSize, ILC_COLOR32, 1, 1);
        if (HAvailableColumnsImageList != NULL)
        {
            HBITMAP hBitmap = NULL;
            if (RenderSVGIconBitmap("Windows", iconSize, TRUE, &hBitmap))
            {
                ImageList_Add(HAvailableColumnsImageList, hBitmap, NULL);
                DeleteObject(hBitmap);
                ListView_SetImageList(HListView2, HAvailableColumnsImageList, LVSIL_SMALL);
            }
        }

        // insert the Name column into the list view with columns
        GetClientRect(HListView2, &r);
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        lvc.pszText = LoadStr(IDS_COLUMN_NAME);
        lvc.fmt = LVCFMT_LEFT;
        lvc.iSubItem = 0;
        ListView_InsertColumn(HListView2, 0, &lvc);
        SetViewsAvailableColumnsColumnWidth(HListView2);

        RECT rightWindowRect;
        RECT leftWindowRect;
        RECT clientRect;
        GetWindowRect(HListView2, &rightWindowRect);
        GetWindowRect(HListView, &leftWindowRect);
        GetClientRect(HWindow, &clientRect);
        POINT rightBottom = {rightWindowRect.right, rightWindowRect.bottom};
        POINT rightTop = {rightWindowRect.left, rightWindowRect.top};
        POINT leftBottom = {leftWindowRect.right, leftWindowRect.bottom};
        ScreenToClient(HWindow, &rightBottom);
        ScreenToClient(HWindow, &rightTop);
        ScreenToClient(HWindow, &leftBottom);
        AvailableColumnsWidth = rightBottom.x - rightTop.x;
        AvailableColumnsRightMargin = clientRect.right - rightBottom.x;
        ViewsListGap = rightTop.x - leftBottom.x;
        if (Configuration.ConfigurationViewsRightWidth != 0)
            AvailableColumnsWidth = Configuration.ConfigurationViewsRightWidth;
        else
            AvailableColumnsWidth += ConfigurationViewsDefaultAvailableColumnsWidthExtra;

        // dialog elements should stretch with the dialog size, set split controls
        ElasticVerticalLayout(2, IDC_VIEW_LIST, IDC_VIEW_LIST2);
        LayoutViewsListControls();

        UpdateConfigListViewColors(HListView);
        UpdateConfigListViewColors(HListView2);
        RemoveListViewsWhiteClientEdge(HListView, HListView2);
        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            // darkmodelib's replaceClientEdgeWithBorderSafeEx replaces WS_BORDER with
            // WS_EX_CLIENTEDGE for enabled windows. On Win10, WS_EX_CLIENTEDGE creates
            // a white/light 3D border that bleeds into the listview client area and shows
            // as white backgrounds behind the native checkbox state images. The custom
            // NM_CUSTOMDRAW handler fills the item rect with dark background and draws dark
            // checkboxes, but the WS_EX_CLIENTEDGE border pixels remain white and are
            // visible at the edges. Remove WS_EX_CLIENTEDGE so only the CToolbarHeader's
            // dark sunken border remains. On Win11+, darkmodelib's setDarkCheckboxes
            // replaces the native state images entirely, so the border isn't an issue.
            RemoveListViewsWhiteClientEdge(HListView, HListView2);
            DarkModeApplyStaticTextColors(HWindow, NULL);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }

        break;
    }

    case WM_SIZE:
    {
        INT_PTR result = CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
        LayoutViewsListControls();
        return result;
    }

    case WM_SETCURSOR:
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(HWindow, &pt);
        RECT client;
        GetClientRect(HWindow, &client);
        int splitterX = client.right - AvailableColumnsRightMargin - AvailableColumnsWidth - ViewsListGap / 2;
        if (abs(pt.x - splitterX) <= 4)
        {
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
            return TRUE;
        }
        break;
    }

    case WM_LBUTTONDOWN:
    {
        int x = GET_X_LPARAM(lParam);
        RECT client;
        GetClientRect(HWindow, &client);
        int splitterX = client.right - AvailableColumnsRightMargin - AvailableColumnsWidth - ViewsListGap / 2;
        if (abs(x - splitterX) <= 4)
        {
            ViewsSplitterDrag = TRUE;
            SetCapture(HWindow);
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            return TRUE;
        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        if (ViewsSplitterDrag)
        {
            RECT client;
            GetClientRect(HWindow, &client);
            int x = GET_X_LPARAM(lParam);
            int rightLeft = x + ViewsListGap / 2;
            if (rightLeft < CFGP2MinDefinedViewsWidth)
                rightLeft = CFGP2MinDefinedViewsWidth;
            if (rightLeft > client.right - AvailableColumnsRightMargin - CFGP2MinAvailableColumnsWidth)
                rightLeft = client.right - AvailableColumnsRightMargin - CFGP2MinAvailableColumnsWidth;
            AvailableColumnsWidth = client.right - AvailableColumnsRightMargin - rightLeft;
            Configuration.ConfigurationViewsRightWidth = AvailableColumnsWidth;
            LayoutViewsListControls();
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            return TRUE;
        }
        break;
    }

    case WM_LBUTTONUP:
    {
        if (ViewsSplitterDrag)
        {
            ViewsSplitterDrag = FALSE;
            ReleaseCapture();
            return TRUE;
        }
        break;
    }

    case WM_DESTROY:
    {
        if (HAvailableColumnsImageList != NULL)
        {
            ListView_SetImageList(HListView2, NULL, LVSIL_SMALL);
            ImageList_Destroy(HAvailableColumnsImageList);
            HAvailableColumnsImageList = NULL;
        }
        break;
    }

    case WM_SYSCOLORCHANGE:
    {
        UpdateConfigListViewColors(HListView);
        UpdateConfigListViewColors(HListView2);
        RemoveListViewsWhiteClientEdge(HListView, HListView2);
        break;
    }

    case WM_USER_CHAR:
    {
        int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
        if (wParam != ' ' && !LabelEdit)
        {
            ListView_EditLabel(HListView, index);
            HWND hEdit = ListView_GetEditControl(HListView);
            SendMessage(hEdit, WM_CHAR, wParam, 0);
        }
        return 0;
    }

    case WM_NOTIFY:
    {
        if (DisableNotification)
            break;

        if (wParam == IDC_VIEW_LIST2)
        {
            LPNMHDR nmh = (LPNMHDR)lParam;
            switch (nmh->code)
            {
            case NM_CUSTOMDRAW:
            {
                LPNMLVCUSTOMDRAW customDraw = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);
                LRESULT customDrawResult = CDRF_DODEFAULT;
                if (DarkModeShouldUseDarkColors())
                {
                    if (customDraw->nmcd.dwDrawStage == CDDS_PREPAINT)
                        customDrawResult = CDRF_NOTIFYITEMDRAW;
                    else if (customDraw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
                    {
                        customDraw->clrTextBk = DarkModeGetDialogBackgroundColor();
                        customDraw->clrText = DarkModeGetDialogTextColor();
                        if (ShouldCustomDrawListViewCheckboxes())
                            customDrawResult = CDRF_NOTIFYPOSTPAINT;
                        else
                            customDrawResult = CDRF_DODEFAULT;
                    }
                    else if (customDraw->nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT)
                    {
                        DrawDarkModeListViewCheckboxes(HListView2, customDraw, 1);
                        customDrawResult = CDRF_DODEFAULT;
                    }
                }
                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, customDrawResult);
                return TRUE;
            }

            case NM_DBLCLK:
            {
                LVHITTESTINFO ht;
                DWORD pos = GetMessagePos();
                ht.pt.x = GET_X_LPARAM(pos);
                ht.pt.y = GET_Y_LPARAM(pos);
                ScreenToClient(HListView2, &ht.pt);
                ListView_HitTest(HListView2, &ht);
                int index = ListView_GetNextItem(HListView2, -1, LVNI_SELECTED);
                if (index != -1 && ht.iItem == index && ht.flags != LVHT_ONITEMICON)
                {
                    UINT state = ListView_GetItemState(HListView2, index, LVIS_STATEIMAGEMASK);
                    if (state == INDEXTOSTATEIMAGEMASK(2))
                        state = INDEXTOSTATEIMAGEMASK(1);
                    else
                        state = INDEXTOSTATEIMAGEMASK(2);
                    ListView_SetItemState(HListView2, index, state, LVIS_STATEIMAGEMASK);
                }
                break;
            }

            case LVN_ITEMCHANGING:
            {
                LPNMLISTVIEW nmhi = (LPNMLISTVIEW)nmh;
                // disagree :-) beep when attempting to hide the Name column
                int columnIndex = GetAvailableColumnIndex(HListView2, nmhi->iItem);
                if (columnIndex == 0 && (nmhi->uOldState & 0xF000) != (nmhi->uNewState & 0xF000))
                {
                    MessageBeep(MB_ICONASTERISK);
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
                }
                else
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);

                return TRUE;
            }

            case LVN_ITEMCHANGED:
            {
                LPNMLISTVIEW nmhi = (LPNMLISTVIEW)nmh;
                if ((nmhi->uOldState & 0xF000) != (nmhi->uNewState & 0xF000))
                {
                    if (IsAvailableColumnsFilterActive())
                    {
                        int viewIndex = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
                        int columnIndex = GetAvailableColumnIndex(HListView2, nmhi->iItem);
                        BOOL checked = (nmhi->uNewState & 0xF000) == INDEXTOSTATEIMAGEMASK(2);
                        if (viewIndex >= 2)
                        {
                            if (columnIndex > 0)
                            {
                                if (checked)
                                    Config.Items[viewIndex].Flags |= CFGP2Flags[columnIndex];
                                else
                                    Config.Items[viewIndex].Flags &= ~CFGP2Flags[columnIndex];
                            }
                            else if (columnIndex < 0)
                            {
                                int explorerIndex = -columnIndex - 1;
                                if (explorerIndex >= 0 && explorerIndex < EXPLORER_COLUMNS_COUNT)
                                    Config.Items[viewIndex].ExplorerColumnVisible[explorerIndex] = checked;
                            }
                            Dirty = TRUE;
                        }
                    }
                    else
                        StoreControls(); // save data when the checkbox is clicked
                    EnableControls();
                }
                else if (!(nmhi->uOldState & LVIS_SELECTED) && nmhi->uNewState & LVIS_SELECTED)
                {
                    int viewIndex = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
                    int columnIndex = GetAvailableColumnIndex(HListView2, nmhi->iItem);
                    if (viewIndex >= 2)
                    {
                        CColumnConfig* columnConfig = columnIndex >= 0 ? &Config.Items[viewIndex].Columns[columnIndex] : &Config.Items[viewIndex].ExplorerColumns[-columnIndex - 1];
                        DisableNotification = TRUE;
                        CTransferInfo ti(HWindow, ttDataToWindow);
                        int tmp = columnConfig->LeftFixedWidth;
                        ti.CheckBox(IDC_VIEW_LEFT_FIXED, tmp);
                        tmp = columnConfig->RightFixedWidth;
                        ti.CheckBox(IDC_VIEW_RIGHT_FIXED, tmp);
                        tmp = columnConfig->LeftWidth;
                        ti.EditLine(IDC_VIEW_LEFT_WIDTH, tmp);
                        tmp = columnConfig->RightWidth;
                        ti.EditLine(IDC_VIEW_RIGHT_WIDTH, tmp);
                        DisableNotification = FALSE;
                    }
                    EnableControls();
                    EnableHeader();
                }
                break;
            }
            }
            break;
        }

        if (wParam == IDC_VIEW_LIST)
        {
            LPNMHDR nmh = (LPNMHDR)lParam;
            switch (nmh->code)
            {
            case LVN_KEYDOWN:
            {
                LPNMLVKEYDOWN nmhk = (LPNMLVKEYDOWN)nmh;
                int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
                if (nmhk->wVKey == VK_F2)
                {
                    OnModify();
                }
                if (nmhk->wVKey == VK_DELETE)
                {
                    OnDelete();
                }
                if ((GetKeyState(VK_MENU) & 0x8000) &&
                    (nmhk->wVKey == VK_UP || nmhk->wVKey == VK_DOWN))
                {
                    OnMove(nmhk->wVKey == VK_UP);
                }
                if (!LabelEdit && nmhk->wVKey == VK_INSERT)
                    ListView_EditLabel(HListView, index);
                break;
            }

            case LVN_ITEMCHANGED:
            {
                LPNMLISTVIEW nmhi = (LPNMLISTVIEW)nmh;
                if (!(nmhi->uOldState & LVIS_SELECTED) && nmhi->uNewState & LVIS_SELECTED)
                {
                    LoadControls();
                }

                //            if (nmhi->uOldState & 0x2000 && nmhi->uNewState & 0x1000 ||
                //                nmhi->uOldState & 0x1000 && nmhi->uNewState  & 0x2000)
                //            {
                //              BOOL checked = nmhi->uNewState & 0x2000;
                //              Config->SetVisible(nmhi->iItem, checked);
                //            }

                EnableHeader();
                break;
            }

            case LVN_BEGINLABELEDIT:
            {
                if (GetEnabledFunctions() & TLBHDRMASK_MODIFY)
                {
                    LabelEdit = TRUE;
                    EnableHeader();
                }
                else
                {
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
                break;
            }

            case LVN_ENDLABELEDIT:
            {
                NMLVDISPINFO* nmhd = (NMLVDISPINFO*)nmh;
                LabelEdit = FALSE;
                EnableHeader();
                if (nmhd->item.pszText != NULL)
                {
                    char name[VIEW_NAME_MAX];
                    CopyStringTruncateUtf8(name, VIEW_NAME_MAX, nmhd->item.pszText);
                    Config.CleanName(name);
                    int index = nmhd->item.iItem;
                    if (lstrlen(Config.Items[index].Name) == 0)
                        Config.Items[index].Flags = 0;
                    lstrcpy(Config.Items[index].Name, name);
                    LoadControls();
                    ListView_SetItemText(HListView, index, 0, name);
                    Dirty = TRUE;
                    break;
                }
                break;
            }
            }
        }
        break;
    }

    case WM_COMMAND:
    {
        if (!DisableNotification && HIWORD(wParam) == BN_CLICKED)
        {
            if (LOWORD(wParam) == IDC_VIEW_LEFT_FIXED || LOWORD(wParam) == IDC_VIEW_RIGHT_FIXED ||
                LOWORD(wParam) == IDC_VIEW_LEFT_SMARTMODE || LOWORD(wParam) == IDC_VIEW_RIGHT_SMARTMODE)
            {
                int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
                int index2 = ListView_GetNextItem(HListView2, -1, LVNI_SELECTED);
                if (index >= 2 && index2 != -1)
                {
                    int columnIndex = GetAvailableColumnIndex(HListView2, index2);
                    CColumnConfig* columnConfig = columnIndex >= 0 ? &Config.Items[index].Columns[columnIndex] : &Config.Items[index].ExplorerColumns[-columnIndex - 1];
                    BOOL checked = IsDlgButtonChecked(HWindow, LOWORD(wParam)) == BST_CHECKED;
                    switch (LOWORD(wParam))
                    {
                    case IDC_VIEW_LEFT_FIXED:
                        if (columnConfig != NULL)
                            columnConfig->LeftFixedWidth = checked ? 1 : 0;
                        break;
                    case IDC_VIEW_RIGHT_FIXED:
                        if (columnConfig != NULL)
                            columnConfig->RightFixedWidth = checked ? 1 : 0;
                        break;
                    case IDC_VIEW_LEFT_SMARTMODE:
                        Config.Items[index].LeftSmartMode = checked;
                        break;
                    case IDC_VIEW_RIGHT_SMARTMODE:
                        Config.Items[index].RightSmartMode = checked;
                        break;
                    }
                    Dirty = TRUE;
                }
                EnableControls();
            }
            break;
        }

        if (!DisableNotification && HIWORD(wParam) == EN_CHANGE)
        {
            if (LOWORD(wParam) == IDC_VIEW_COLUMNS_FILTER)
            {
                ApplyAvailableColumnsFilter();
                break;
            }
            if (LOWORD(wParam) == IDC_VIEW_LEFT_WIDTH || LOWORD(wParam) == IDC_VIEW_RIGHT_WIDTH)
            {
                int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
                int index2 = ListView_GetNextItem(HListView2, -1, LVNI_SELECTED);
                if (index >= 2 && index2 != -1)
                {
                    int columnIndex = GetAvailableColumnIndex(HListView2, index2);
                    CColumnConfig* columnConfig = columnIndex >= 0 ? &Config.Items[index].Columns[columnIndex] : &Config.Items[index].ExplorerColumns[-columnIndex - 1];
                    CTransferInfo ti(HWindow, ttDataFromWindow);
                    int tmp;
                    if (LOWORD(wParam) == IDC_VIEW_LEFT_WIDTH)
                    {
                        ti.EditLine(IDC_VIEW_LEFT_WIDTH, tmp);
                        columnConfig->LeftWidth = tmp;
                    }
                    else
                    {
                        ti.EditLine(IDC_VIEW_RIGHT_WIDTH, tmp);
                        columnConfig->RightWidth = tmp;
                    }
                    Dirty = TRUE;
                }
            }
            break;
        }

        if (LOWORD(wParam) == IDC_VIEWLIST_HEADER2)
        {
            if (GetFocus() != HListView2)
                SetFocus(HListView2);
            if (HIWORD(wParam) == TLBHDR_FILTER)
            {
                ToggleAvailableColumnsFilter();
            }
            else if (HIWORD(wParam) == TLBHDR_TOP || HIWORD(wParam) == TLBHDR_UP || HIWORD(wParam) == TLBHDR_DOWN || HIWORD(wParam) == TLBHDR_BOTTOM)
            {
                int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
                int item = ListView_GetNextItem(HListView2, -1, LVNI_SELECTED);
                int item2 = HIWORD(wParam) == TLBHDR_UP ? item - 1 : item + 1;
                int columnIndex = GetAvailableColumnIndex(HListView2, item);
                int columnIndex2 = GetAvailableColumnIndex(HListView2, item2);
                if (HIWORD(wParam) == TLBHDR_TOP)
                {
                    int topItem = columnIndex >= 0 ? 1 : CFGP2ExplorerColumnsStart;
                    if (index >= 2 && item > topItem &&
                        ((columnIndex >= 0 && item < CFGP2ItemsCount) ||
                         (columnIndex < 0 && item >= CFGP2ExplorerColumnsStart)))
                    {
                        StoreControls();
                        if (columnIndex >= 0)
                        {
                            BYTE tmp = Config.Items[index].ColumnOrder[item];
                            memmove(Config.Items[index].ColumnOrder + topItem + 1,
                                    Config.Items[index].ColumnOrder + topItem,
                                    item - topItem);
                            Config.Items[index].ColumnOrder[topItem] = tmp;
                        }
                        else
                        {
                            int explorerItem = item - CFGP2ExplorerColumnsStart;
                            WORD tmp = Config.Items[index].ExplorerColumnOrder[explorerItem];
                            memmove(Config.Items[index].ExplorerColumnOrder + 1,
                                    Config.Items[index].ExplorerColumnOrder,
                                    explorerItem * sizeof(WORD));
                            Config.Items[index].ExplorerColumnOrder[0] = tmp;
                        }
                        ListView_DeleteAllItems(HListView2);
                        LoadControls();
                        ListView_SetItemState(HListView2, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
                        ListView_SetItemState(HListView2, topItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                        ListView_EnsureVisible(HListView2, topItem, FALSE);
                        Dirty = TRUE;
                    }
                }
                else if (HIWORD(wParam) == TLBHDR_BOTTOM)
                {
                    int bottomItem = columnIndex >= 0 ? CFGP2ItemsCount - 1 : ListView_GetItemCount(HListView2) - 1;
                    if (index >= 2 && item >= 0 && item < bottomItem &&
                        ((columnIndex >= 0 && item > 0 && item < CFGP2ItemsCount) ||
                         (columnIndex < 0 && item >= CFGP2ExplorerColumnsStart)))
                    {
                        StoreControls();
                        if (columnIndex >= 0)
                        {
                            BYTE tmp = Config.Items[index].ColumnOrder[item];
                            memmove(Config.Items[index].ColumnOrder + item,
                                    Config.Items[index].ColumnOrder + item + 1,
                                    bottomItem - item);
                            Config.Items[index].ColumnOrder[bottomItem] = tmp;
                        }
                        else
                        {
                            int explorerItem = item - CFGP2ExplorerColumnsStart;
                            int bottomExplorerItem = bottomItem - CFGP2ExplorerColumnsStart;
                            WORD tmp = Config.Items[index].ExplorerColumnOrder[explorerItem];
                            memmove(Config.Items[index].ExplorerColumnOrder + explorerItem,
                                    Config.Items[index].ExplorerColumnOrder + explorerItem + 1,
                                    (bottomExplorerItem - explorerItem) * sizeof(WORD));
                            Config.Items[index].ExplorerColumnOrder[bottomExplorerItem] = tmp;
                        }
                        ListView_DeleteAllItems(HListView2);
                        LoadControls();
                        ListView_SetItemState(HListView2, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
                        ListView_SetItemState(HListView2, bottomItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                        ListView_EnsureVisible(HListView2, bottomItem, FALSE);
                        Dirty = TRUE;
                    }
                }
                else if (index >= 2 && item > 0 && item2 > 0 &&
                         ((columnIndex >= 0 && columnIndex2 >= 0 && item2 < CFGP2ItemsCount) ||
                          (columnIndex < 0 && columnIndex2 < 0 && item >= CFGP2ExplorerColumnsStart && item2 >= CFGP2ExplorerColumnsStart)))
                {
                    StoreControls();
                    if (columnIndex >= 0)
                    {
                        BYTE tmp = Config.Items[index].ColumnOrder[item];
                        Config.Items[index].ColumnOrder[item] = Config.Items[index].ColumnOrder[item2];
                        Config.Items[index].ColumnOrder[item2] = tmp;
                    }
                    else
                    {
                        int explorerItem = item - CFGP2ExplorerColumnsStart;
                        int explorerItem2 = item2 - CFGP2ExplorerColumnsStart;
                        WORD tmp = Config.Items[index].ExplorerColumnOrder[explorerItem];
                        Config.Items[index].ExplorerColumnOrder[explorerItem] = Config.Items[index].ExplorerColumnOrder[explorerItem2];
                        Config.Items[index].ExplorerColumnOrder[explorerItem2] = tmp;
                    }
                    ListView_DeleteAllItems(HListView2);
                    LoadControls();
                    ListView_SetItemState(HListView2, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
                    ListView_SetItemState(HListView2, item2, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                    ListView_EnsureVisible(HListView2, item2, FALSE);
                    Dirty = TRUE;
                }
            }
            EnableHeader();
            break;
        }

        if (LOWORD(wParam) == IDC_VIEWLIST_HEADER)
        {
            if (GetFocus() != HListView)
                SetFocus(HListView);
            switch (HIWORD(wParam))
            {
            case TLBHDR_MODIFY:
                OnModify();
                break;
            case TLBHDR_DELETE:
                OnDelete();
                break;
            case TLBHDR_UP:
                OnMove(TRUE);
                break;
            case TLBHDR_DOWN:
                OnMove(FALSE);
                break;
            }
        }
        break;
    }
    }

    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CCfgPageViewer
//

static void AlignDlgCtrlLeftOfCtrl(HWND hWindow, int labelID, int ctrlID);

CCfgPageViewer::CCfgPageViewer()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_VIEWER, IDD_CFGPAGE_VIEWER, PSP_USETITLE, NULL)
{
    HFont = NULL;
    LocalUseCustomViewerFont = UseCustomViewerFont;
    memcpy(&LocalViewerLogFont, &ViewerLogFont, sizeof(LocalViewerLogFont));
    NormalText = NULL;
    SelectedText = NULL;
}

CCfgPageViewer::~CCfgPageViewer()
{
    if (HFont != NULL)
        HANDLES(DeleteObject(HFont));
}

void CCfgPageViewer::Validate(CTransferInfo& ti)
{
    int dummy;
    ti.EditLine(IDC_TABSIZE, dummy);
    if (dummy <= 0 || dummy > 30)
    {
        SalMessageBox(HWindow, LoadStr(IDS_BADTABSIZE), LoadStr(IDS_ERRORTITLE),
                      MB_OK | MB_ICONEXCLAMATION);
        ti.ErrorOn(IDC_TABSIZE);
    }

    char buf[MAX_PATH];
    if (ti.IsGood())
    {
        lstrcpyn(buf, Configuration.TextModeMasks.GetMasksString(), MAX_PATH); // backup of TextModeMasks
        // provide MasksString, there is range checking, nothing serious
        ti.EditLine(IDC_VIEW_INTEXT, Configuration.TextModeMasks.GetWritableMasksString(), MAX_PATH);
        int errorPos;
        if (!Configuration.TextModeMasks.PrepareMasks(errorPos))
        {
            SalMessageBox(HWindow, LoadStr(IDS_INCORRECTSYNTAX), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
            SetFocus(GetDlgItem(HWindow, IDC_VIEW_INTEXT));
            SendMessage(GetDlgItem(HWindow, IDC_VIEW_INTEXT), EM_SETSEL, errorPos, errorPos + 1);
            ti.ErrorOn(IDC_VIEW_INTEXT);
        }
        Configuration.TextModeMasks.SetMasksString(buf); // TextModeMasks restoration
        Configuration.TextModeMasks.PrepareMasks(errorPos);
    }

    if (ti.IsGood())
    {
        lstrcpyn(buf, Configuration.HexModeMasks.GetMasksString(), MAX_PATH); // backup of HexModeMasks
        // provide MasksString, there is range checking, nothing serious
        ti.EditLine(IDC_VIEW_INHEX, (char*)Configuration.HexModeMasks.GetWritableMasksString(), MAX_PATH);
        int errorPos;
        if (!Configuration.HexModeMasks.PrepareMasks(errorPos))
        {
            SalMessageBox(HWindow, LoadStr(IDS_INCORRECTSYNTAX), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
            SetFocus(GetDlgItem(HWindow, IDC_VIEW_INHEX));
            SendMessage(GetDlgItem(HWindow, IDC_VIEW_INHEX), EM_SETSEL, errorPos, errorPos + 1);
            ti.ErrorOn(IDC_VIEW_INHEX);
        }
        Configuration.HexModeMasks.SetMasksString(buf); // HexModeMasks restoration
        Configuration.HexModeMasks.PrepareMasks(errorPos);
    }
}

void CCfgPageViewer::Transfer(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageViewer::Transfer()");
    if (ti.Type == ttDataToWindow)
        LoadControls();
    ti.CheckBox(IDC_COPYFINDTEXT, Configuration.CopyFindText);
    ti.CheckBox(IDC_CRLFEOL, Configuration.EOL_CRLF);
    ti.CheckBox(IDC_CREOL, Configuration.EOL_CR);
    ti.CheckBox(IDC_LFEOL, Configuration.EOL_LF);
    ti.CheckBox(IDC_NULLEOL, Configuration.EOL_NULL);
    ti.EditLine(IDC_TABSIZE, Configuration.TabSize);
    ti.RadioButton(IDC_SAVEONCLOSE, 1, Configuration.SavePosition);
    ti.RadioButton(IDC_SETBYMAINWINDOW, 0, Configuration.SavePosition);
    if (Configuration.SavePosition)
        Configuration.WindowPlacement.length = 0;

    // provide MasksString, there is range checking, nothing serious
    ti.EditLine(IDC_VIEW_INHEX, (char*)Configuration.HexModeMasks.GetWritableMasksString(), MAX_PATH);
    // provide MasksString, there is range checking, nothing serious
    ti.EditLine(IDC_VIEW_INTEXT, (char*)Configuration.TextModeMasks.GetWritableMasksString(), MAX_PATH);
    int errPos;
    Configuration.TextModeMasks.PrepareMasks(errPos);
    Configuration.HexModeMasks.PrepareMasks(errPos);

    if (ti.Type == ttDataToWindow)
    {
        int i;
        for (i = 0; i < NUMBER_OF_VIEWERCOLORS; i++)
            TmpColors[i] = ViewerColors[i];
        NormalText->SetColor(GetCOLORREF(TmpColors[VIEWER_FG_NORMAL]), GetCOLORREF(TmpColors[VIEWER_BK_NORMAL]));
        SelectedText->SetColor(GetCOLORREF(TmpColors[VIEWER_FG_SELECTED]), GetCOLORREF(TmpColors[VIEWER_BK_SELECTED]));
    }
    else
    {
        UseCustomViewerFont = LocalUseCustomViewerFont;
        if (memcmp(&ViewerLogFont, &LocalViewerLogFont, sizeof(LocalViewerLogFont)) != 0)
        { // when the font changes we must measure the new one
            HANDLES(EnterCriticalSection(&ViewerFontMeasureCS));
            memcpy(&ViewerLogFont, &LocalViewerLogFont, sizeof(LocalViewerLogFont));
            ViewerFontMeasured = FALSE;
            HANDLES(LeaveCriticalSection(&ViewerFontMeasureCS));
        }
        if (!Configuration.UseWindowsDarkMode && WindowsDarkModeIsViewerPalette(TmpColors))
        {
            WindowsLightModeBuildViewerPalette(TmpColors);
            NormalText->SetColor(GetCOLORREF(TmpColors[VIEWER_FG_NORMAL]),
                                 GetCOLORREF(TmpColors[VIEWER_BK_NORMAL]));
            SelectedText->SetColor(GetCOLORREF(TmpColors[VIEWER_FG_SELECTED]),
                                   GetCOLORREF(TmpColors[VIEWER_BK_SELECTED]));
        }

        BOOL colorChanged = FALSE;
        int i;
        for (i = 0; i < NUMBER_OF_VIEWERCOLORS; i++)
        {
            if (ViewerColors[i] != TmpColors[i])
            {
                ViewerColors[i] = TmpColors[i];
                colorChanged = TRUE;
            }
        }
        UpdateViewerColors(ViewerColors);

        // broadcast this change to the plug-ins as well
        if (colorChanged)
            Plugins.Event(PLUGINEVENT_COLORSCHANGED, 0);

        // after closing the dialog BroadcastConfigChanged will be called
    }
}

void CCfgPageViewer::LoadControls()
{
    CALL_STACK_MESSAGE1("CCfgPageViewer::LoadControls()");

    LOGFONT logFont;
    if (LocalUseCustomViewerFont)
        logFont = LocalViewerLogFont;
    else
        GetDefaultViewerLogFont(&logFont);

    HWND hEdit = GetDlgItem(HWindow, IDE_VIEWERFONT);
    int origHeight = logFont.lfHeight;
    logFont.lfHeight = GetWindowFontHeight(hEdit); // use the edit line font size for font presentation
    if (HFont != NULL)
        HANDLES(DeleteObject(HFont));
    HFont = HANDLES(CreateFontIndirect(&logFont));
    HDC hDC = HANDLES(GetDC(HWindow));
    SendMessage(hEdit, WM_SETFONT, (WPARAM)HFont, MAKELPARAM(TRUE, 0));
    char buf[LF_FACESIZE + 200];
    _snprintf_s(buf, _TRUNCATE, LoadStr(IDS_FONTDESCRIPTION),
                MulDiv(-origHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY)),
                LocalViewerLogFont.lfFaceName,
                LoadStr(LocalUseCustomViewerFont ? IDS_FONTDESCRIPTION_CST : IDS_FONTDESCRIPTION_DEF));
    SetWindowText(hEdit, buf);

    HANDLES(ReleaseDC(HWindow, hDC));
}

INT_PTR
CCfgPageViewer::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CCfgPageViewer::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        INT_PTR result = 0;
        if (DarkModeTryHandleCtlColorForDialogPage(uMsg, wParam, lParam, result))
            return result;
        break;
    }

    case WM_INITDIALOG:
    {
        new CButton(HWindow, IDB_VIEWERFONT, BTF_RIGHTARROW);

        NormalText = new CColorArrowButton(HWindow, IDC_IV_NORMAL_TEXT, TRUE);
        SelectedText = new CColorArrowButton(HWindow, IDC_IV_SELECTED_TEXT, TRUE);

        CHyperLink* hl = new CHyperLink(HWindow, IDC_FILEMASK_HINT, STF_DOTUNDERLINE);
        if (hl != NULL)
            hl->SetActionShowHint(LoadStr(IDS_MASKS_HINT));
        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            DarkModeApplyStaticTextColors(HWindow, NULL);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }

        break;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED)
        {
            switch (LOWORD(wParam))
            {
            case IDB_VIEWERFONT:
            {
                /* used by the export_mnu.py script that generates salmenu.mnu for the Translator
    keep synchronized with the InsertMenu() call below...
MENU_TEMPLATE_ITEM CfgPageViewerMenu[] = 
{
  {MNTT_PB, 0
  {MNTT_IT, IDS_USEDEFAULTFONT
  {MNTT_IT, IDS_USECUSTOMFONT
  {MNTT_PE, 0
};
*/
                HMENU hMenu = CreatePopupMenu();
                BOOL cstFont = LocalUseCustomViewerFont;
                InsertMenu(hMenu, 0xFFFFFFFF, cstFont ? 0 : MF_CHECKED | MF_BYCOMMAND | MF_STRING, 1, LoadStr(IDS_USEDEFAULTFONT));
                InsertMenu(hMenu, 0xFFFFFFFF, cstFont ? MF_CHECKED : 0 | MF_BYCOMMAND | MF_STRING, 2, LoadStr(IDS_USECUSTOMFONT));

                TPMPARAMS tpmPar;
                tpmPar.cbSize = sizeof(tpmPar);
                GetWindowRect((HWND)lParam, &tpmPar.rcExclude);
                DWORD cmd = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, tpmPar.rcExclude.right, tpmPar.rcExclude.top,
                                             HWindow, &tpmPar);
                if (cmd == 1)
                {
                    // default font
                    LocalUseCustomViewerFont = FALSE;
                    LoadControls();
                }
                if (cmd == 2)
                {
                    // custom font
                    CHOOSEFONT cf;
                    LOGFONT logFont = LocalViewerLogFont;
                    memset(&cf, 0, sizeof(CHOOSEFONT));
                    cf.lStructSize = sizeof(CHOOSEFONT);
                    cf.hwndOwner = HWindow;
                    cf.hDC = NULL;
                    cf.lpLogFont = &logFont;
                    cf.iPointSize = 10;
                    cf.Flags = CF_NOVERTFONTS | CF_FIXEDPITCHONLY | CF_SCREENFONTS |
                               CF_INITTOLOGFONTSTRUCT;
                    DarkModePrepareChooseFont(&cf);
                    if (ChooseFont(&cf) != 0)
                    {
                        LocalViewerLogFont = logFont;
                        LocalUseCustomViewerFont = TRUE;
                        LoadControls();
                    }
                }
                return 0;
            }

            case IDC_IV_NORMAL_TEXT:
            case IDC_IV_SELECTED_TEXT:
            {
                CColorArrowButton* button = (CColorArrowButton*)WindowsManager.GetWindowPtr((HWND)lParam);
                if (button != NULL)
                {
                    /* used by the export_mnu.py script that generates salmenu.mnu for the Translator
    keep synchronized with the InsertMenu() call below...
MENU_TEMPLATE_ITEM CfgPageViewerMenu[] = 
{
  {MNTT_PB, 0
  {MNTT_IT, IDS_SETCOLOR_CUSTOM_FG
  {MNTT_IT, IDS_SETCOLOR_SYSTEM_FG
  {MNTT_IT, IDS_SETCOLOR_CUSTOM_BK
  {MNTT_IT, IDS_SETCOLOR_SYSTEM_BK
  {MNTT_PE, 0
};
*/
                    HMENU hMenu = CreatePopupMenu();
                    BOOL normal = button == NormalText;
                    BOOL checkedDefaultFg = GetFValue(TmpColors[normal ? VIEWER_FG_NORMAL : VIEWER_FG_SELECTED]) & SCF_DEFAULT;
                    BOOL checkedDefaultBk = GetFValue(TmpColors[normal ? VIEWER_BK_NORMAL : VIEWER_BK_SELECTED]) & SCF_DEFAULT;
                    InsertMenu(hMenu, 0xFFFFFFFF, checkedDefaultFg ? 0 : MF_CHECKED | MF_BYCOMMAND | MF_STRING, 1, LoadStr(IDS_SETCOLOR_CUSTOM_FG));
                    InsertMenu(hMenu, 0xFFFFFFFF, checkedDefaultFg ? MF_CHECKED : 0 | MF_BYCOMMAND | MF_STRING, 2, LoadStr(IDS_SETCOLOR_SYSTEM_FG));
                    InsertMenu(hMenu, 0xFFFFFFFF, MF_BYCOMMAND | MF_SEPARATOR, 0, NULL);
                    InsertMenu(hMenu, 0xFFFFFFFF, checkedDefaultBk ? 0 : MF_CHECKED | MF_BYCOMMAND | MF_STRING, 3, LoadStr(IDS_SETCOLOR_CUSTOM_BK));
                    InsertMenu(hMenu, 0xFFFFFFFF, checkedDefaultBk ? MF_CHECKED : 0 | MF_BYCOMMAND | MF_STRING, 4, LoadStr(IDS_SETCOLOR_SYSTEM_BK));

                    //            int i;
                    //            for (i = 0; i < 4; i++)
                    //              SetMenuItemBitmaps(hMenu, i + 1, MF_BYCOMMAND, NULL, HMenuCheckDot);

                    TPMPARAMS tpmPar;
                    tpmPar.cbSize = sizeof(tpmPar);
                    GetWindowRect(button->HWindow, &tpmPar.rcExclude);
                    DWORD cmd = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, tpmPar.rcExclude.right, tpmPar.rcExclude.top,
                                                 HWindow, &tpmPar);
                    if (cmd != 0)
                    {
                        int colorIndex;
                        if (normal)
                            colorIndex = (cmd == 1 || cmd == 2) ? VIEWER_FG_NORMAL : VIEWER_BK_NORMAL;
                        else
                            colorIndex = (cmd == 1 || cmd == 2) ? VIEWER_FG_SELECTED : VIEWER_BK_SELECTED;
                        if (cmd == 1 || cmd == 3)
                        {
                            CHOOSECOLOR cc;
                            cc.lStructSize = sizeof(cc);
                            cc.hwndOwner = HWindow;
                            cc.lpCustColors = (LPDWORD)CustomColors;
                            if (cmd == 1)
                                cc.rgbResult = button->GetTextColor();
                            else
                                cc.rgbResult = button->GetBkgndColor();
                            cc.Flags = CC_RGBINIT | CC_FULLOPEN;
                            DarkModePrepareChooseColor(&cc);
                            if (ChooseColor(&cc) == TRUE)
                            {
                                if (cmd == 1)
                                    button->SetTextColor(cc.rgbResult);
                                else
                                    button->SetBkgndColor(cc.rgbResult);

                                BYTE flags = GetFValue(TmpColors[colorIndex]);
                                flags &= ~SCF_DEFAULT;
                                TmpColors[colorIndex] = cc.rgbResult & 0x00ffffff | (((DWORD)flags) << 24);
                            }
                        }
                        else
                        {
                            BYTE flags = GetFValue(TmpColors[colorIndex]);
                            flags |= SCF_DEFAULT;
                            TmpColors[colorIndex] = TmpColors[colorIndex] & 0x00ffffff | (((DWORD)flags) << 24);

                            UpdateViewerColors(TmpColors);
                            button->SetColor(GetCOLORREF(TmpColors[normal ? VIEWER_FG_NORMAL : VIEWER_FG_SELECTED]),
                                             GetCOLORREF(TmpColors[normal ? VIEWER_BK_NORMAL : VIEWER_BK_SELECTED]));
                        }
                    }
                    DestroyMenu(hMenu);
                }
                return 0;
            }
            }
        }
        break;
    }

    }
    INT_PTR result = CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
    if (uMsg == WM_INITDIALOG || uMsg == WM_SIZE)
        AlignDlgCtrlLeftOfCtrl(HWindow, IDC_STATIC_11, IDC_IV_SELECTED_TEXT);
    return result;
}
//
// ****************************************************************************
// CCfgPageUserMenu
//
/*
void 
CSmallIconWindow::SetIcon(HICON hIcon)
{
  HIcon = hIcon;
  InvalidateRect(HWindow, NULL, TRUE);
  UpdateWindow(HWindow);
}

LRESULT 
CSmallIconWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc = HANDLES(BeginPaint(HWindow, &ps));

      RECT r;
      GetClientRect(HWindow, &r);

      FillRect(hdc, &r, HDialogBrush);

      if (IsWindowEnabled(HWindow))
        DrawIconEx(hdc, (r.right - 16) / 2, (r.bottom - 16) / 2,
                   HIcon, 0, 0, 0, NULL, DI_NORMAL);
      HANDLES(EndPaint(HWindow, &ps));
      return 0;
    }
  }
  return CWindow::WindowProc(uMsg, wParam, lParam);
}
*/

CCfgPageUserMenu::CCfgPageUserMenu(CUserMenuItems* userMenuItems)
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_USERMENU, IDD_CFGPAGE_USERMENU, PSP_USETITLE, NULL)
{
    SourceUserMenuItems = userMenuItems;
    UserMenuItems = new CUserMenuItems(10, 5);
    UserMenuItems->LoadUMI(*SourceUserMenuItems, FALSE);
    EditLB = NULL;
    //  SmallIcon = NULL;
    DisableNotification = FALSE;
}

CCfgPageUserMenu::~CCfgPageUserMenu()
{
    if (UserMenuItems != NULL)
        delete UserMenuItems;
}

void CCfgPageUserMenu::Validate(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageUserMenu::Validate()");
    int i;
    for (i = 0; i < UserMenuItems->Count; i++)
    {
        int errorPos1, errorPos2;
        CUserMenuItem* item = UserMenuItems->At(i);
        if (item->Type == umitItem)
        {
            if (!ValidateCommandFile(HWindow, item->UMCommand, errorPos1, errorPos2))
            {
                EditLB->SetCurSel(i);
                ti.ErrorOn(IDE_COMMAND);
                PostMessage(GetDlgItem(HWindow, IDE_COMMAND), EM_SETSEL,
                            errorPos1, errorPos2);
                return;
            }
            if (!ValidateUserMenuArguments(HWindow, item->Arguments, errorPos1, errorPos2, NULL))
            {
                EditLB->SetCurSel(i);
                ti.ErrorOn(IDE_ARGUMENTS);
                PostMessage(GetDlgItem(HWindow, IDE_ARGUMENTS), EM_SETSEL,
                            errorPos1, errorPos2);
                return;
            }
            if (!ValidateInitDir(HWindow, item->InitDir, errorPos1, errorPos2))
            {
                EditLB->SetCurSel(i);
                ti.ErrorOn(IDE_INITDIR);
                PostMessage(GetDlgItem(HWindow, IDE_INITDIR), EM_SETSEL,
                            errorPos1, errorPos2);
                return;
            }
        }
    }
}

void CCfgPageUserMenu::Transfer(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageUserMenu::Transfer()");
    if (ti.Type == ttDataToWindow)
    {
        int i;
        for (i = 0; i < UserMenuItems->Count; i++)
            EditLB->AddItem((INT_PTR)UserMenuItems->At(i));
        EditLB->SetCurSel(0);
        LoadControls();
        EnableButtons();
    }
    else
    {
        SourceUserMenuItems->LoadUMI(*UserMenuItems, UserMenuIconBkgndReader.IsReadingIcons() ||
                                                         UserMenuIconBkgndReader.HasSysColorsChanged()); // after a system color change copying icons is not enough, they must be reloaded (the dialog does not handle icon color changes, icons might appear "broken")
    }
}

// pulls a child from the dialog and enables or disables it based on the 'enable' parameter;
// if 'enable' is FALSE and 'clear' is FALSE, it can also clear the edit line or the checkbox

void EnableDlgWindow(HWND hDialog, int resID, BOOL enable, BOOL clear = FALSE)
{
    HWND hChild = GetDlgItem(hDialog, resID);
    if (hChild == NULL)
    {
        TRACE_E("EnableDlgWindow: cannot find child window resID=" << resID);
        return;
    }
    EnableWindow(hChild, enable);
    if (!enable && clear)
    {
        char className[31];
        className[0] = 0;
        GetClassName(hChild, className, 30);
        if (StrICmp(className, "edit") == 0)
            SetWindowText(hChild, "");
        else if (StrICmp(className, "button") == 0)
            SendMessage(hChild, BM_SETCHECK, BST_UNCHECKED, 0);
    }
}

void CCfgPageUserMenu::EnableButtons()
{
    BOOL oldDisableNotification = DisableNotification;
    DisableNotification = TRUE;

    BOOL validItem = TRUE;
    INT_PTR itemID;
    EditLB->GetCurSelItemID(itemID);
    if (itemID != -1)
    {
        CUserMenuItem* item = (CUserMenuItem*)itemID;
        if (item->Type == umitSubmenuEnd)
            validItem = FALSE;
    }
    if (itemID == -1)
        validItem = FALSE;

    BOOL separator = IsDlgButtonChecked(HWindow, IDC_UM_SEPARATOR);
    BOOL submenu = IsDlgButtonChecked(HWindow, IDC_UM_SUBMENU);

    EnableDlgWindow(HWindow, IDC_UM_SUBMENU, validItem & !separator, TRUE);
    EnableDlgWindow(HWindow, IDC_UM_SEPARATOR, validItem & !submenu, TRUE);

    EnableWindow(GetDlgItem(HWindow, IDB_UM_CHANGEICON), validItem & !separator & !submenu);
    /*
  EnableWindow(SmallIcon->HWindow, validItem & !separator & !submenu);
  InvalidateRect(SmallIcon->HWindow, NULL, TRUE);
  UpdateWindow(SmallIcon->HWindow);
*/
    EnableDlgWindow(HWindow, IDE_COMMAND, validItem & !separator & !submenu, TRUE);
    EnableDlgWindow(HWindow, IDE_ARGUMENTS, validItem & !separator & !submenu, TRUE);
    EnableDlgWindow(HWindow, IDE_INITDIR, validItem & !separator & !submenu, TRUE);

    EnableWindow(GetDlgItem(HWindow, IDB_BROWSECOMMAND), validItem & !separator & !submenu);
    EnableWindow(GetDlgItem(HWindow, IDB_BROWSEARGUMENTS), validItem & !separator & !submenu);
    EnableWindow(GetDlgItem(HWindow, IDB_BROWSEINITDIR), validItem & !separator & !submenu);

    EnableDlgWindow(HWindow, IDC_THROUGHSHELL, validItem & !separator & !submenu, TRUE);
    BOOL throughShell = IsDlgButtonChecked(HWindow, IDC_THROUGHSHELL);
    EnableDlgWindow(HWindow, IDC_OUTPUTWND,
                    validItem & throughShell & !separator & !submenu, TRUE);
    BOOL openShell = IsDlgButtonChecked(HWindow, IDC_OUTPUTWND);
    EnableDlgWindow(HWindow, IDC_CLOSESHELL,
                    validItem & throughShell & openShell & !separator & !submenu, TRUE);

    EnableWindow(GetDlgItem(HWindow, IDC_UM_TOOLBAR), validItem);
    DisableNotification = oldDisableNotification;
}

void CCfgPageUserMenu::LoadControls()
{
    CALL_STACK_MESSAGE1("CCfgPageUserMenu::LoadControls()");
    INT_PTR itemID;
    EditLB->GetCurSelItemID(itemID);
    BOOL empty = FALSE;
    if (itemID == -1)
        empty = TRUE;

    CUserMenuItem* item = NULL;
    if (!empty)
        item = (CUserMenuItem*)itemID;
    DisableNotification = TRUE;

    CheckDlgButton(HWindow, IDC_UM_SUBMENU, (!empty && (item->Type == umitSubmenuBegin)) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(HWindow, IDC_UM_SEPARATOR, (!empty && (item->Type == umitSeparator)) ? BST_CHECKED : BST_UNCHECKED);

    SendMessage(GetDlgItem(HWindow, IDE_COMMAND), EM_LIMITTEXT, MAX_PATH - 1, 0);
    SendMessage(GetDlgItem(HWindow, IDE_ARGUMENTS), EM_LIMITTEXT, USRMNUARGS_MAXLEN - 1, 0);
    SendMessage(GetDlgItem(HWindow, IDE_INITDIR), EM_LIMITTEXT, MAX_PATH - 1, 0);
    SendMessage(GetDlgItem(HWindow, IDE_COMMAND), WM_SETTEXT, 0,
                (LPARAM)(empty ? "" : item->UMCommand));
    SendMessage(GetDlgItem(HWindow, IDE_ARGUMENTS), WM_SETTEXT, 0,
                (LPARAM)(empty ? "" : item->Arguments));
    SendMessage(GetDlgItem(HWindow, IDE_ARGUMENTS), EM_SETSEL, 0, -1); // so that Browse overwrites the content
    SendMessage(GetDlgItem(HWindow, IDE_INITDIR), WM_SETTEXT, 0,
                (LPARAM)(empty ? "" : item->InitDir));
    SendMessage(GetDlgItem(HWindow, IDE_INITDIR), EM_SETSEL, 0, -1); // so that Browse overwrites the content

    //  SmallIcon->SetIcon(!empty ? item->HIcon : NULL);

    CheckDlgButton(HWindow, IDC_OUTPUTWND, (!empty && (item->UseWindow)) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(HWindow, IDC_THROUGHSHELL, (!empty && (item->ThroughShell)) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(HWindow, IDC_CLOSESHELL, (!empty && (item->CloseShell)) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(HWindow, IDC_UM_TOOLBAR, (!empty && (item->ShowInToolbar)) ? BST_CHECKED : BST_UNCHECKED);
    DisableNotification = FALSE;
}

void CCfgPageUserMenu::StoreControls()
{
    CALL_STACK_MESSAGE1("CCfgPageUserMenu::StoreControls()");
    int index;
    EditLB->GetCurSel(index);
    if (!DisableNotification && index >= 0 && index < EditLB->GetCount())
    {
        CUserMenuItem* item = UserMenuItems->At(index);

        char command[MAX_PATH];
        char arguments[USRMNUARGS_MAXLEN];
        char initdir[MAX_PATH];
        SendMessage(GetDlgItem(HWindow, IDE_COMMAND), WM_GETTEXT,
                    MAX_PATH, (LPARAM)command);
        SendMessage(GetDlgItem(HWindow, IDE_ARGUMENTS), WM_GETTEXT,
                    USRMNUARGS_MAXLEN, (LPARAM)arguments);
        SendMessage(GetDlgItem(HWindow, IDE_INITDIR), WM_GETTEXT,
                    MAX_PATH, (LPARAM)initdir);

        item->Set(item->ItemName, command, arguments, initdir, item->Icon);

        BOOL submenu = (IsDlgButtonChecked(HWindow, IDC_UM_SUBMENU) == BST_CHECKED);
        BOOL separator = (IsDlgButtonChecked(HWindow, IDC_UM_SEPARATOR) == BST_CHECKED);

        CUserMenuItemType type;
        if (submenu)
            type = umitSubmenuBegin;
        else if (separator)
            type = umitSeparator;
        else
            type = umitItem;
        if (item->Type != type)
        {
            item->SetType(type);
            item->GetIconHandle(NULL, FALSE);
            EditLB->RedrawFocusedItem();
        }

        item->UseWindow = (IsDlgButtonChecked(HWindow, IDC_OUTPUTWND) == BST_CHECKED);
        item->ThroughShell = (IsDlgButtonChecked(HWindow, IDC_THROUGHSHELL) == BST_CHECKED);
        item->CloseShell = (IsDlgButtonChecked(HWindow, IDC_CLOSESHELL) == BST_CHECKED);
        item->ShowInToolbar = (IsDlgButtonChecked(HWindow, IDC_UM_TOOLBAR) == BST_CHECKED);
    }
}

void CCfgPageUserMenu::DeleteSubmenuEnd(int index)
{
    int endIndex = UserMenuItems->GetSubmenuEndIndex(index);
    if (endIndex != -1)
    {
        // a closing item exists, delete it
        EditLB->DeleteItem(endIndex);
        UserMenuItems->Delete(endIndex);
    }
}

void CCfgPageUserMenu::RefreshGroupIconInUMItems()
{
    for (int i = 0; i < UserMenuItems->Count; i++)
    {
        CUserMenuItem* item = UserMenuItems->At(i);
        if (item->Type == umitSubmenuBegin)
            item->UMIcon = HGroupIcon;
    }
}

INT_PTR
CCfgPageUserMenu::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CCfgPageUserMenu::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        INT_PTR result = 0;
        if (DarkModeTryHandleCtlColorForDialogPage(uMsg, wParam, lParam, result))
            return result;
        break;
    }

    case WM_INITDIALOG:
    {
        RefreshGroupIconInUMItems();                                                          // if colors changed before first visiting the User Menu page and we did not receive WM_SYSCOLORCHANGE, handle it here
        EditLB = new CEditListBox(HWindow, IDL_MENUITEMS, ELB_ENABLECOMMANDS | ELB_SHOWICON); // we need the enabler and icons
        if (EditLB == NULL)
            TRACE_E(LOW_MEMORY);
        ChangeToArrowButton(HWindow, IDB_BROWSECOMMAND);
        ChangeToArrowButton(HWindow, IDB_BROWSEARGUMENTS);
        ChangeToArrowButton(HWindow, IDB_BROWSEINITDIR);
        EditLB->MakeHeader(IDS_USHEADER);
        //      SmallIcon = new CSmallIconWindow(HWindow, IDC_UM_ICON);
        //      if (SmallIcon == NULL) TRACE_E(LOW_MEMORY);

        // dialog elements should stretch with the dialog size, set split controls
        ElasticVerticalLayout(1, IDL_MENUITEMS);
        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            DarkModeApplyStaticTextColors(HWindow, NULL);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }

        break;
    }

    case WM_COMMAND:
    {
        if (DisableNotification)
            return 0;

        if (HIWORD(wParam) == BN_CLICKED)
        {
            if (LOWORD(wParam) == IDC_UM_SUBMENU)
            {
                int index;
                EditLB->GetCurSel(index);
                if (index >= 0 && index < EditLB->GetCount())
                {
                    BOOL submenu = IsDlgButtonChecked(HWindow, IDC_UM_SUBMENU) == BST_CHECKED;
                    if (submenu)
                    {
                        // user checked the Submenu checkbox; add a closing item
                        static char emptyBuffer[] = "";
                        CUserMenuItem* item = new CUserMenuItem(LoadStr(IDS_ENDUSERSUBMENU), emptyBuffer, emptyBuffer, emptyBuffer, emptyBuffer,
                                                                FALSE, FALSE, FALSE, FALSE, umitSubmenuEnd, NULL);
                        if (item == NULL)
                            return 0;
                        UserMenuItems->Insert(index + 1, item);
                        EditLB->InsertItem((INT_PTR)item, index + 1);

                        // the icon will change
                        //              EditLB->RedrawFocusedItem();
                    }
                    else
                    {
                        // user cleared the Submenu checkbox; remove the closing item
                        DeleteSubmenuEnd(index);
                    }
                }
            }

            EnableButtons();
            StoreControls();
        }

        if (HIWORD(wParam) == EN_CHANGE)
        {
            StoreControls();
            EnableButtons();
        }

        if (LOWORD(wParam) == IDL_MENUITEMS && HIWORD(wParam) == LBN_SELCHANGE)
        {
            EditLB->OnSelChanged();
            LoadControls();
            EnableButtons();
        }

        switch (LOWORD(wParam))
        {
        case IDB_BROWSECOMMAND:
        {
            const CExecuteItem* item;
            item = TrackExecuteMenu(HWindow, IDB_BROWSECOMMAND, IDE_COMMAND, FALSE,
                                    CommandExecutes, IDS_EXEFILTER);
            if (item != NULL)
            {
                // update icons
                int index;
                EditLB->GetCurSel(index);
                if (index >= 0 && index < EditLB->GetCount())
                {
                    CUserMenuItem* item2 = UserMenuItems->At(index);
                    item2->GetIconHandle(NULL, FALSE);
                    EditLB->RedrawFocusedItem();
                }
            }
            return 0;
        }

        case IDB_BROWSEARGUMENTS:
        {
            TrackExecuteMenu(HWindow, IDB_BROWSEARGUMENTS, IDE_ARGUMENTS, FALSE,
                             UserMenuArgsExecutes);
            return 0;
        }

        case IDB_BROWSEINITDIR:
        {
            TrackExecuteMenu(HWindow, IDB_BROWSEINITDIR, IDE_INITDIR, FALSE,
                             InitDirExecutes);
            return 0;
        }

        case IDB_UM_CHANGEICON:
        {
            char fileName[MAX_PATH + 10];
            fileName[0] = 0;
            int resID = 0;

            int index;
            EditLB->GetCurSel(index);
            if (index >= 0 && index < EditLB->GetCount())
            {
                CUserMenuItem* item = UserMenuItems->At(index);
                if (item->Icon != NULL && item->Icon[0] != 0)
                {
                    // Icon is in the format "file name,resID"
                    // perform decomposition
                    char* iterator = item->Icon + strlen(item->Icon) - 1;
                    while (iterator > item->Icon && *iterator != ',')
                        iterator--;
                    if (iterator > item->Icon && *iterator == ',')
                    {
                        strncpy(fileName, item->Icon, iterator - item->Icon);
                        fileName[iterator - item->Icon] = 0;
                        iterator++;
                        resID = atoi(iterator);
                    }
                }
                BOOL error = FALSE;
                if (fileName[0] == 0 && item->UMCommand != NULL)
                {
                    if (!ExpandCommand(MainWindow->HWindow, item->UMCommand, fileName, MAX_PATH, FALSE))
                        error = TRUE;
                    else
                    {
                        while (strlen(fileName) > 2 && CutDoubleQuotesFromBothSides(fileName))
                            ;
                    }
                }

                if (!error)
                {
                    CChangeIconDialog dlg(HWindow, fileName, &resID);
                    if (dlg.Execute() == IDOK)
                    {
                        sprintf(fileName + strlen(fileName), ",%d", resID);
                        item->Set(item->ItemName, item->UMCommand, item->Arguments, item->InitDir, fileName);
                        item->GetIconHandle(NULL, FALSE);
                        EditLB->RedrawFocusedItem();
                    }
                }
            }
            return 0;
        }
        }
        break;
    }

    case WM_SYSCOLORCHANGE:
    {
        RefreshGroupIconInUMItems();
        return 0;
    }

    case WM_NOTIFY:
    {
        /*
      LRESULT result;
      if (EditLB->OnWMNotify(lParam, result))
      {
        SetWindowLongPtr(HWindow, DWLP_MSGRESULT, result);
        return 0;
      }
*/
        NMHDR* nmhdr = (NMHDR*)lParam;
        switch (nmhdr->idFrom)
        {
        case IDL_MENUITEMS:
        {
            switch (nmhdr->code)
            {
            case EDTLBN_GETDISPINFO:
            {
                EDTLB_DISPINFO* dispInfo = (EDTLB_DISPINFO*)lParam;
                if (dispInfo->ToDo == edtlbGetData)
                {
                    CUserMenuItem* item = (CUserMenuItem*)dispInfo->ItemID;
                    strcpy(dispInfo->Buffer, item->ItemName);
                    dispInfo->HIcon = item->UMIcon;
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);
                    return TRUE;
                }
                else
                {
                    CUserMenuItem* item;
                    if (dispInfo->ItemID == -1)
                    {
                        item = new CUserMenuItem();
                        if (item == NULL)
                        {
                            TRACE_E(LOW_MEMORY);
                            SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
                            return TRUE;
                        }
                        UserMenuItems->Add(item);
                        item->Set(dispInfo->Buffer, item->UMCommand, item->Arguments, item->InitDir, item->Icon);
                        EditLB->SetItemData((INT_PTR)item);
                    }
                    else
                    {
                        item = (CUserMenuItem*)dispInfo->ItemID;
                        item->Set(dispInfo->Buffer, item->UMCommand, item->Arguments, item->InitDir, item->Icon);
                    }

                    LoadControls();
                    EnableButtons();
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
                break;
            }

            case EDTLBN_ENABLECOMMANDS:
            {
                EDTLB_DISPINFO* dispInfo = (EDTLB_DISPINFO*)lParam;
                int index;
                EditLB->GetCurSel(index);
                dispInfo->Enable = TLBHDRMASK_NEW;
                if (index < 0 || index >= UserMenuItems->Count)
                {
                    dispInfo->Enable |= TLBHDRMASK_MODIFY;
                    return TRUE;
                }

                if (UserMenuItems->At(index)->Type != umitSubmenuEnd)
                    dispInfo->Enable |= TLBHDRMASK_DELETE | TLBHDRMASK_MODIFY;

                if (UserMenuItems->At(index)->Type == umitSubmenuBegin ||
                    UserMenuItems->At(index)->Type == umitSubmenuEnd)
                {
                    if (UserMenuItems->At(index)->Type == umitSubmenuBegin)
                    {
                        // umitSubmenuBegin - the start can always move up, down until its matching end hits the list end
                        dispInfo->Enable |= TLBHDRMASK_UP;
                        int endIndex = UserMenuItems->GetSubmenuEndIndex(index);
                        if (endIndex != -1 && endIndex + 1 < UserMenuItems->Count)
                        {
                            dispInfo->Enable |= TLBHDRMASK_DOWN;
                        }
                    }
                    else
                    {
                        // umitSubmenuEnd - do not allow the end to move over a different end or start
                        if (index > 0)
                        {
                            if (UserMenuItems->At(index - 1)->Type != umitSubmenuBegin &&
                                UserMenuItems->At(index - 1)->Type != umitSubmenuEnd)
                                dispInfo->Enable |= TLBHDRMASK_UP;
                        }
                        if (index >= 0 && index < UserMenuItems->Count - 1)
                        {
                            if (UserMenuItems->At(index + 1)->Type != umitSubmenuBegin &&
                                UserMenuItems->At(index + 1)->Type != umitSubmenuEnd)
                                dispInfo->Enable |= TLBHDRMASK_DOWN;
                        }
                    }
                }
                else
                {
                    // !umitSubmenuBegin && !umitSubmenuEnd
                    dispInfo->Enable |= TLBHDRMASK_UP | TLBHDRMASK_DOWN;
                }
                return TRUE;
            }
                /*
            case EDTLBN_MOVEITEM:
            {
              EDTLB_DISPINFO *dispInfo = (EDTLB_DISPINFO *)lParam;
              int index;
              EditLB->GetCurSel(index);

              CUserMenuItem *item = UserMenuItems->At(index);

              if (item->Type == umitSubmenuBegin)
              {
                int endIndex = UserMenuItems->GetSubmenuEndIndex(index);
                if (endIndex != -1)
                {
                  // found SubmenuEnd
                  BYTE buf[sizeof(CUserMenuItem)];
                  if (dispInfo->Up)
                  {
                    // move the whole block up
                    if (index > 0)
                    {
                      memcpy(buf, UserMenuItems->At(index - 1), sizeof(CUserMenuItem));
                      int i;
                      for (i = index; i <= endIndex; i++)
                        memcpy(UserMenuItems->At(i - 1), UserMenuItems->At(i), sizeof(CUserMenuItem));
                      memcpy(UserMenuItems->At(endIndex), buf, sizeof(CUserMenuItem));
                      EditLB->SetCurSel(index - 1);
                    }
                  }
                  else
                  {
                    // move the whole block down
                    if (endIndex > 0 && endIndex < EditLB->GetCount() - 1)
                    {
                      memcpy(buf, UserMenuItems->At(endIndex + 1), sizeof(CUserMenuItem));
                      int i;
                      for (i = endIndex; i >= index; i--)
                        memcpy(UserMenuItems->At(i + 1), UserMenuItems->At(i), sizeof(CUserMenuItem));
                      memcpy(UserMenuItems->At(index), buf, sizeof(CUserMenuItem));
                      EditLB->SetCurSel(index + 1);
                    }
                  }
                  InvalidateRect(EditLB->HWindow, NULL, FALSE);
                }

                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);  // disallow the change
                return TRUE;
              }
              else
              {
                int srcIndex = index;
                int dstIndex = index + (dispInfo->Up ? -1 : 1);

                BYTE buf[sizeof(CUserMenuItem)];
                memcpy(buf, UserMenuItems->At(srcIndex), sizeof(CUserMenuItem));
                memcpy(UserMenuItems->At(srcIndex), UserMenuItems->At(dstIndex), sizeof(CUserMenuItem));
                memcpy(UserMenuItems->At(dstIndex), buf, sizeof(CUserMenuItem));

                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);  // allow the change
                return TRUE;
              }
            }
*/
            case EDTLBN_MOVEITEM2:
            {
                EDTLB_DISPINFO* dispInfo = (EDTLB_DISPINFO*)lParam;
                int index;
                EditLB->GetCurSel(index);
                int srcIndex = index;
                int dstIndex = dispInfo->NewIndex;

                CUserMenuItem* item = UserMenuItems->At(index);

                if (item->Type == umitSubmenuBegin)
                {
                    int endIndex = UserMenuItems->GetSubmenuEndIndex(index);
                    if (endIndex != -1)
                    {
                        // found SubmenuEnd
                        BYTE buf[sizeof(CUserMenuItem)];
                        if (srcIndex > dstIndex)
                        {
                            // move the whole block up
                            if (index > 0)
                            {
                                memcpy(buf, UserMenuItems->At(index - 1), sizeof(CUserMenuItem));
                                int i;
                                for (i = index; i <= endIndex; i++)
                                    memcpy(UserMenuItems->At(i - 1), UserMenuItems->At(i), sizeof(CUserMenuItem));
                                memcpy(UserMenuItems->At(endIndex), buf, sizeof(CUserMenuItem));
                                EditLB->SetCurSel(index - 1);
                            }
                        }
                        else
                        {
                            // move the whole block down
                            if (endIndex > 0 && endIndex < EditLB->GetCount() - 1)
                            {
                                memcpy(buf, UserMenuItems->At(endIndex + 1), sizeof(CUserMenuItem));
                                int i;
                                for (i = endIndex; i >= index; i--)
                                    memcpy(UserMenuItems->At(i + 1), UserMenuItems->At(i), sizeof(CUserMenuItem));
                                memcpy(UserMenuItems->At(index), buf, sizeof(CUserMenuItem));
                                EditLB->SetCurSel(index + 1);
                            }
                        }
                        InvalidateRect(EditLB->HWindow, NULL, FALSE);
                    }

                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE); // disallow the changes
                    return TRUE;
                }
                else
                {
                    dstIndex = index + (srcIndex > dstIndex ? -1 : 1);

                    BYTE buf[sizeof(CUserMenuItem)];
                    memcpy(buf, UserMenuItems->At(srcIndex), sizeof(CUserMenuItem));
                    memcpy(UserMenuItems->At(srcIndex), UserMenuItems->At(dstIndex), sizeof(CUserMenuItem));
                    memcpy(UserMenuItems->At(dstIndex), buf, sizeof(CUserMenuItem));

                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE); // allow the change
                    return TRUE;
                }
            }

            case EDTLBN_DELETEITEM:
            {
                int index;
                EditLB->GetCurSel(index);

                // when the user deletes a popup we must remove the closing item
                if (UserMenuItems->At(index)->Type == umitSubmenuBegin)
                    DeleteSubmenuEnd(index);

                UserMenuItems->Delete(index);
                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE); // allow deletion
                return TRUE;
            }
            }
            break;
        }
        }
        break;
    }

    case WM_DRAWITEM:
    {
        int idCtrl = (int)wParam;
        if (idCtrl == IDL_MENUITEMS)
        {
            EditLB->OnDrawItem(lParam);
            return TRUE;
        }
        break;
    }
    }
    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CCfgPageHotPath
//

CCfgPageHotPath::CCfgPageHotPath(BOOL editMode, int editIndex)
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_HOTPATH, IDD_CFGPAGE_HOTPATH, PSP_USETITLE, NULL)
{
    Dirty = FALSE;
    Header = NULL;
    HListView = NULL;
    DisableNotification = FALSE;
    EditMode = editMode;
    EditIndex = editIndex;
    if (editIndex < 0 || editIndex >= HOT_PATHS_COUNT)
    {
        EditMode = FALSE;
        EditIndex = 0;
    }
    LabelEdit = FALSE;
    Config = new CHotPathItems();
    if (Config == NULL)
    {
        TRACE_E(LOW_MEMORY);
        return;
    }
}

CCfgPageHotPath::~CCfgPageHotPath()
{
    if (Config != NULL)
        delete Config;
}

void CCfgPageHotPath::Validate(CTransferInfo& ti)
{
    if (Dirty)
    {
        HCURSOR hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
        int i;
        for (i = 0; i < HOT_PATHS_COUNT; i++)
        {
            int errorPos1, errorPos2;
            if (Config->GetNameLen(i) > 0)
            {
                char path[HOTPATHITEM_MAXPATH];
                Config->GetPath(i, path, HOTPATHITEM_MAXPATH);
                if (!ValidateHotPath(HWindow, path, errorPos1, errorPos2))
                {
                    ListView_SetItemState(HListView, i, LVIS_SELECTED, LVIS_SELECTED);
                    ListView_EnsureVisible(HListView, i, FALSE);
                    ti.ErrorOn(IDC_HOTPATH_PATH);
                    PostMessage(GetDlgItem(HWindow, IDC_HOTPATH_PATH), EM_SETSEL,
                                errorPos1, errorPos2);
                    return;
                }
            }
        }
        SetCursor(hOldCur);
    }
}

extern char HotPathSetBufferName[MAX_PATH];
extern char HotPathSetBufferPath[HOTPATHITEM_MAXPATH];

void CCfgPageHotPath::Transfer(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageHotPath::Transfer()");
    ti.CheckBox(IDC_HOTPATH_AUTOCONFIG, Configuration.HotPathAutoConfig);
    if (ti.Type == ttDataToWindow)
    {
        Config->Load(MainWindow->HotPaths);

        if (EditMode)
            Config->Set(EditIndex, HotPathSetBufferName, HotPathSetBufferPath);

        int index = 0;
        DisableNotification = TRUE;
        char buff[20];
        char name[MAX_PATH];
        int i;
        for (i = 0; i < HOT_PATHS_COUNT; i++)
        {
            LVITEM lvi;
            lvi.mask = LVIF_TEXT | LVIF_STATE;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            lvi.state = 0;
            name[0] = 0;
            Config->GetName(i, name, MAX_PATH);
            lvi.pszText = name;
            ListView_InsertItem(HListView, &lvi);

            UINT state = INDEXTOSTATEIMAGEMASK((Config->GetVisible(i) ? 2 : 1));
            ListView_SetItemState(HListView, i, state, LVIS_STATEIMAGEMASK);

            if (i < 10)
            {
                sprintf(buff, "%s+%d", LoadStr(IDS_CTRL), i < 9 ? i + 1 : 0);
                ListView_SetItemText(HListView, i, 1, buff);
            }
        }

        RECT r;
        GetClientRect(HListView, &r);
        ListView_SetColumnWidth(HListView, 0, r.right - r.left);
        ListView_SetColumnWidth(HListView, 1, LVSCW_AUTOSIZE_USEHEADER);
        int w = ListView_GetColumnWidth(HListView, 1);
        ListView_SetColumnWidth(HListView, 0, r.right - r.left - w);

        DWORD state = LVIS_SELECTED | LVIS_FOCUSED;
        if (EditMode)
            index = EditIndex;
        ListView_SetItemState(HListView, index, state, state);
        DisableNotification = FALSE;
        LoadControls();
        EnableHeader();
    }
    else
    {
        MainWindow->HotPaths.Load(*Config);
    }
    Dirty = FALSE;
}

void CCfgPageHotPath::LoadControls()
{
    int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
    char path[HOTPATHITEM_MAXPATH];
    path[0] = 0;
    BOOL visible = FALSE;
    if (index != -1)
    {
        Config->GetPath(index, path, HOTPATHITEM_MAXPATH);
        visible = Config->GetVisible(index);
    }

    DisableNotification = TRUE;
    SendDlgItemMessage(HWindow, IDC_HOTPATH_PATH, EM_LIMITTEXT, HOTPATHITEM_MAXPATH - 1, 0);
    SetDlgItemText(HWindow, IDC_HOTPATH_PATH, path);

    DisableNotification = FALSE;
    EnableControls();
}

void CCfgPageHotPath::StoreControls()
{
    int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
    if (index != -1)
    {
        char buff[HOTPATHITEM_MAXPATH];
        GetDlgItemText(HWindow, IDC_HOTPATH_PATH, buff, HOTPATHITEM_MAXPATH);
        Config->SetPath(index, buff);
    }
}

void CCfgPageHotPath::EnableControls()
{
    int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
    BOOL enable = TRUE;
    if (index == -1 || Config->GetNameLen(index) == 0)
        enable = FALSE;
    EnableWindow(GetDlgItem(HWindow, IDC_HOTPATH_PATH), enable);
    EnableWindow(GetDlgItem(HWindow, IDC_HOTPATH_BROWSE), enable);
}

void CCfgPageHotPath::EnableHeader()
{
    DWORD mask = 0;
    if (!LabelEdit)
    {
        mask |= TLBHDRMASK_MODIFY | TLBHDRMASK_DELETE;
        int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
        if (index > 0)
            mask |= TLBHDRMASK_UP;
        if (index < HOT_PATHS_COUNT - 1)
            mask |= TLBHDRMASK_DOWN;
    }
    Header->EnableToolbar(mask);
}

void CCfgPageHotPath::OnModify()
{
    int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
    if (index != -1)
        PostMessage(HListView, LVM_EDITLABEL, index, 0);
}

void CCfgPageHotPath::OnDelete()
{
    int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
    if (index != -1)
    {
        Config->Set(index, "", "");
        char buffEmpty[] = "";
        ListView_SetItemText(HListView, index, 0, buffEmpty);
        LoadControls();
    }
    EnableHeader();
}

void CCfgPageHotPath::OnMove(BOOL up)
{
    CALL_STACK_MESSAGE2("CCfgPageHotPath::OnMove(%d)", up);
    DisableNotification = TRUE;
    int index1 = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
    int index2 = index1;
    if (index1 == -1)
        return;
    if (up && index1 > 0)
        index2 = index1 - 1;
    if (!up && index1 < HOT_PATHS_COUNT - 1)
        index2 = index1 + 1;
    if (index2 != index1)
    {
        char name1[MAX_PATH];
        char name2[MAX_PATH];
        Config->GetName(index1, name1, MAX_PATH);
        Config->GetName(index2, name2, MAX_PATH);
        ListView_SetItemText(HListView, index1, 0, name2);
        ListView_SetItemText(HListView, index2, 0, name1);
        DWORD state1 = ListView_GetItemState(HListView, index1, LVIS_STATEIMAGEMASK);
        DWORD state2 = ListView_GetItemState(HListView, index2, LVIS_STATEIMAGEMASK);
        ListView_SetItemState(HListView, index1, state2, LVIS_STATEIMAGEMASK);
        state1 |= LVIS_FOCUSED | LVIS_SELECTED;
        ListView_SetItemState(HListView, index2, state1, LVIS_STATEIMAGEMASK | LVIS_FOCUSED | LVIS_SELECTED);
        ListView_EnsureVisible(HListView, index2, FALSE);
        Config->SwapItems(index1, index2);
        LoadControls();
    }
    DisableNotification = FALSE;
    EnableHeader();
}

INT_PTR
CCfgPageHotPath::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CCfgPageHotPath::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        INT_PTR result = 0;
        if (DarkModeTryHandleCtlColorForDialogPage(uMsg, wParam, lParam, result))
            return result;
        break;
    }

    case WM_PAINT:
    {
        // Horrible mess - I need a message that arrives
        // after WM_INITDIALOG so we can set the focus
        // hopefully this will survive until the next Salamander version :-)
        if (EditMode)
        {
            SetFocus(HListView);
            SendMessage(HListView, LVM_EDITLABEL, EditIndex, 0);
            EditMode = FALSE;
        }
        break;
    }

    case WM_INITDIALOG:
    {
        CMyListView* listView = new CMyListView(HWindow, IDC_HOTPATH_LIST);
        HListView = listView->HWindow;
        //      HListView = GetDlgItem(HWindow, IDC_HOTPATH_LIST);
        Header = new CToolbarHeader(HWindow, IDC_HOTPATH_HEADER, HListView,
                                    TLBHDRMASK_MODIFY | TLBHDRMASK_DELETE |
                                        TLBHDRMASK_UP | TLBHDRMASK_DOWN);
        if (Header == NULL)
            TRACE_E(LOW_MEMORY);

        DWORD exFlags = LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
        //      ListView_SetExtendedListViewStyleEx(HListView, exFlags, exFlags);  // 4.71

        DWORD origFlags = ListView_GetExtendedListViewStyle(HListView);
        ListView_SetExtendedListViewStyle(HListView, origFlags | exFlags);

        // determine the list view size
        RECT r;
        GetClientRect(HListView, &r);
        int nameWidth = r.right - (int)(r.right / 7);

        // fill the list view with the Name and HotKey columns
        LVCOLUMN lvc;
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        lvc.pszText = LoadStr(IDS_HOTPATH_NAME);
        lvc.cx = 1;
        lvc.fmt = LVCFMT_LEFT;
        ListView_InsertColumn(HListView, 0, &lvc);

        lvc.mask |= LVCF_SUBITEM;
        lvc.pszText = LoadStr(IDS_HOTPATH_HOTKEY);
        lvc.cx = 1;
        lvc.iSubItem = 1;
        ListView_InsertColumn(HListView, 1, &lvc);

        ChangeToArrowButton(HWindow, IDC_HOTPATH_BROWSE);

        // dialog elements should stretch with the dialog size, set split controls
        ElasticLayoutControls(1, 2, 1, IDC_HOTPATH_LIST, IDC_HOTPATH_HEADER, IDC_HOTPATH_PATH,
                              IDC_HOTPATH_BROWSE);

        DarkModeUpdateListViewColors(HListView);
        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            DarkModeApplyStaticTextColors(HWindow, NULL);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }

        break;
    }

    case WM_USER_CHAR:
    {
        int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
        if (wParam != ' ' && !LabelEdit)
        {
            ListView_EditLabel(HListView, index);
            HWND hEdit = ListView_GetEditControl(HListView);
            SendMessage(hEdit, WM_CHAR, wParam, 0);
        }
        return 0;
    }

    case WM_NOTIFY:
    {
        if (DisableNotification)
            break;

        if (wParam == IDC_HOTPATH_LIST)
        {
            LPNMHDR nmh = (LPNMHDR)lParam;
            switch (nmh->code)
            {
            case LVN_KEYDOWN:
            {
                LPNMLVKEYDOWN nmhk = (LPNMLVKEYDOWN)nmh;
                int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
                if (nmhk->wVKey == VK_F2)
                {
                    OnModify();
                }
                if (nmhk->wVKey == VK_DELETE)
                {
                    OnDelete();
                }
                if ((GetKeyState(VK_MENU) & 0x8000) &&
                    (nmhk->wVKey == VK_UP || nmhk->wVKey == VK_DOWN))
                {
                    OnMove(nmhk->wVKey == VK_UP);
                }
                if (!LabelEdit && nmhk->wVKey == VK_INSERT)
                    ListView_EditLabel(HListView, index);
                break;
            }

            case LVN_ITEMCHANGED:
            {
                LPNMLISTVIEW nmhi = (LPNMLISTVIEW)nmh;
                if (!(nmhi->uOldState & LVIS_SELECTED) && nmhi->uNewState & LVIS_SELECTED)
                {
                    LoadControls();
                }
                if (nmhi->uOldState & 0x2000 && nmhi->uNewState & 0x1000 ||
                    nmhi->uOldState & 0x1000 && nmhi->uNewState & 0x2000)
                {
                    BOOL checked = nmhi->uNewState & 0x2000;
                    Config->SetVisible(nmhi->iItem, checked);
                }
                EnableHeader();
                break;
            }

            case LVN_BEGINLABELEDIT:
            {
                LabelEdit = TRUE;
                EnableHeader();
                break;
            }

            case LVN_ENDLABELEDIT:
#ifndef _UNICODE
            case LVN_ENDLABELEDITW:
#endif // _UNICODE
            {
                LabelEdit = FALSE;
                EnableHeader();
                int index = -1;
                BOOL hasLabel = FALSE;
                char name[MAX_PATH];
                name[0] = 0;
#ifdef _UNICODE
                NMLVDISPINFO* nmhd = (NMLVDISPINFO*)nmh;
                index = nmhd->item.iItem;
                if (nmhd->item.pszText != NULL)
                {
                    lstrcpyn(name, nmhd->item.pszText, MAX_PATH);
                    hasLabel = TRUE;
                }
#else  // _UNICODE
                if (nmh->code == LVN_ENDLABELEDITW)
                {
                    NMLVDISPINFOW* nmhd = (NMLVDISPINFOW*)nmh;
                    index = nmhd->item.iItem;
                    if (nmhd->item.pszText != NULL)
                    {
                        WideCharToMultiByte(CP_ACP, 0, nmhd->item.pszText, -1, name, MAX_PATH, NULL, NULL);
                        hasLabel = TRUE;
                    }
                }
                else
                {
                    NMLVDISPINFOA* nmhd = (NMLVDISPINFOA*)nmh;
                    index = nmhd->item.iItem;
                    if (nmhd->item.pszText != NULL)
                    {
                        lstrcpyn(name, nmhd->item.pszText, MAX_PATH);
                        hasLabel = TRUE;
                    }
                }
#endif // _UNICODE
                if (index != -1 && hasLabel)
                {
                    Config->CleanName(name);
                    char path[HOTPATHITEM_MAXPATH];
                    path[0] = 0;
                    if (strlen(name) != 0)
                    {
                        if (ListView_GetNextItem(HListView, -1, LVNI_SELECTED) == index)
                            GetDlgItemText(HWindow, IDC_HOTPATH_PATH, path, HOTPATHITEM_MAXPATH);
                        else
                            Config->GetPath(index, path, HOTPATHITEM_MAXPATH);
                    }
                    Config->Set(index, name, path);
                    Dirty = TRUE;
                    if (name[0] == 0)
                        LoadControls(); // discard the removed item's path before it can be reused
                    else
                        EnableControls();

                    // The list view applies the label after this notification
                    // returns.  Report that the edit was accepted without
                    // changing the list control while it is finishing it.
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
                break;
            }
            }
        }
        break;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_HOTPATH_PATH)
        {
            if (!DisableNotification)
            {
                StoreControls();
                Dirty = TRUE;
            }
            return TRUE;
        }

        if (LOWORD(wParam) == IDC_HOTPATH_BROWSE)
        {
            TrackExecuteMenu(HWindow, IDC_HOTPATH_BROWSE, IDC_HOTPATH_PATH, FALSE,
                             HotPathItems);
            return TRUE;
        }

        if (LOWORD(wParam) == IDC_HOTPATH_HEADER)
        {
            if (GetFocus() != HListView)
                SetFocus(HListView);
            switch (HIWORD(wParam))
            {
            case TLBHDR_MODIFY:
                OnModify();
                break;
            case TLBHDR_DELETE:
                OnDelete();
                break;
            case TLBHDR_UP:
                OnMove(TRUE);
                break;
            case TLBHDR_DOWN:
                OnMove(FALSE);
                break;
            }
        }
        break;
    }

    case WM_THEMECHANGED:
    {
#if DARKMODE_TRACE_CTLFLOW
        DarkModeTracePageThemeEvent("CCfgPageViewer", uMsg);
#endif
        DarkModeUpdateListViewColors(HListView);
        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyStaticTextColors(HWindow, NULL);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }
        break;
    }

    case WM_SETTINGCHANGE:
    {
#if DARKMODE_TRACE_CTLFLOW
        DarkModeTracePageThemeEvent("CCfgPageViewer", uMsg);
#endif
        if (DarkModeHandleSettingChange(uMsg, lParam))
            DarkModeUpdateListViewColors(HListView);
        break;
    }
    }
    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CCfgPageSystem
//

CCfgPageSystem::CCfgPageSystem()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_SYSTEM, IDD_CFGPAGE_SYSTEM, PSP_USETITLE, NULL)
{
}

void CCfgPageSystem::Validate(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageSystem::Validate()");
    int useRecycle;
    ti.RadioButton(IDR_RECYCLE1, 0, useRecycle);
    ti.RadioButton(IDR_RECYCLE2, 1, useRecycle);
    ti.RadioButton(IDR_RECYCLE3, 2, useRecycle);
    if (useRecycle == 2)
    {
        char buf[MAX_PATH];
        lstrcpyn(buf, Configuration.RecycleMasks.GetMasksString(), MAX_PATH); // backup of RecycleBinMasks
        // provide MasksString, there is range checking, nothing serious
        ti.EditLine(IDE_RECYCLEMASKS, Configuration.RecycleMasks.GetWritableMasksString(), MAX_PATH);
        int errorPos;
        if (!Configuration.RecycleMasks.PrepareMasks(errorPos))
        {
            SalMessageBox(HWindow, LoadStr(IDS_INCORRECTSYNTAX), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
            SetFocus(GetDlgItem(HWindow, IDE_RECYCLEMASKS));
            SendMessage(GetDlgItem(HWindow, IDE_RECYCLEMASKS), EM_SETSEL, errorPos, errorPos + 1);
            ti.ErrorOn(IDE_RECYCLEMASKS);
        }
        Configuration.RecycleMasks.SetMasksString(buf); // RecycleBinMasks restoration
    }
}

void CCfgPageSystem::Transfer(CTransferInfo& ti)
{
    ti.RadioButton(IDR_RECYCLE1, 0, Configuration.UseRecycleBin);
    ti.RadioButton(IDR_RECYCLE2, 1, Configuration.UseRecycleBin);
    ti.RadioButton(IDR_RECYCLE3, 2, Configuration.UseRecycleBin);
    // provide MasksString, there is range checking, nothing serious
    ti.EditLine(IDE_RECYCLEMASKS, Configuration.RecycleMasks.GetWritableMasksString(), MAX_PATH);

    if (ti.Type == ttDataToWindow)
        EnableControls();
}

void CCfgPageSystem::EnableControls()
{
}

INT_PTR
CCfgPageSystem::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static const int kRecycleBinLabelIds[] = {
        IDC_STATIC_1,
        IDC_STATIC_2,
        IDC_STATIC_3,
        IDC_STATIC_4,
        IDC_STATIC_5,
        IDC_FILEMASK_HINT,
        0};

    auto applyRecycleBinLabelColors = [this]() {
        for (int i = 0; kRecycleBinLabelIds[i] != 0; ++i)
        {
            HWND hCtrl = GetDlgItem(HWindow, kRecycleBinLabelIds[i]);
            if (hCtrl != NULL)
                DarkModeApplyStaticTextColors(HWindow, hCtrl);
        }
    };

    switch (uMsg)
    {
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        INT_PTR result = 0;
        if (DarkModeTryHandleCtlColorForDialogPage(uMsg, wParam, lParam, result))
            return result;
        break;
    }

    case WM_INITDIALOG:
    {
        CHyperLink* hl = new CHyperLink(HWindow, IDC_FILEMASK_HINT, STF_DOTUNDERLINE);
        if (hl != NULL)
            hl->SetActionShowHint(LoadStr(IDS_MASKS_HINT));
        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            applyRecycleBinLabelColors();
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }
        break;
    }

    case WM_THEMECHANGED:
    {
#if DARKMODE_TRACE_CTLFLOW
        DarkModeTracePageThemeEvent("CCfgPageSystem", uMsg);
#endif
        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            applyRecycleBinLabelColors();
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }
        break;
    }

    case WM_SETTINGCHANGE:
    {
#if DARKMODE_TRACE_CTLFLOW
        DarkModeTracePageThemeEvent("CCfgPageSystem", uMsg);
#endif
        if (DarkModeHandleSettingChange(uMsg, lParam))
        {
            if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
            {
                applyRecycleBinLabelColors();
                WinLib_DarkMode_PostDeferredRedraw(HWindow);
            }
        }
        break;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED)
            EnableControls();
        break;
    }
    }
    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CCfgPageColors
//

#define CFG7F_SINGLECOLOR 0x01 // only one color is valid (foreground)
#define CFG7F_DEFFG 0x02       // foreground can take the default value
#define CFG7F_DEFBK 0x04       // background can take the default value

struct CConfigurationPage7SubData
{
    int Label;    // resID of the string with the first color name [static before the button]
    BYTE ColorFg; // index of the foreground color (text) to affect
    BYTE ColorBk; // index of the background color to affect
    BYTE Flags;   // options for the item
};

struct CConfigurationPage7Data
{
    int ItemLabel; // resID of the string with the item name [combobox]
    CConfigurationPage7SubData Items[CFG_COLORS_BUTTONS];
};

#define PAGE7DATA_COUNT 8

CConfigurationPage7Data Page7Data[PAGE7DATA_COUNT] =
    {
        // panel item colors
        {
            IDS_COLORITEM_PANELITEM,
            {{IDS_COLORLABEL_NORMAL, ITEM_FG_NORMAL, ITEM_BK_NORMAL, CFG7F_DEFFG | CFG7F_DEFBK},
             {IDS_COLORLABEL_FOCUSED, ITEM_FG_FOCUSED, ITEM_BK_FOCUSED, CFG7F_DEFFG},
             {IDS_COLORLABEL_SELECTED, ITEM_FG_SELECTED, ITEM_BK_SELECTED, CFG7F_DEFBK},
             {IDS_COLORLABEL_FOCUSEDSELECTED, ITEM_FG_FOCSEL, ITEM_BK_FOCSEL, 0},
             {IDS_COLORLABEL_HIGHLIGHTED, ITEM_FG_HIGHLIGHT, ITEM_BK_HIGHLIGHT, CFG7F_DEFFG | CFG7F_DEFBK}},
        },
        // pen colors for the frame around an item
        {
            IDS_COLORITEM_FOCUSEDFRAME,
            {{IDS_COLORLABEL_ACTIVENORMAL, FOCUS_ACTIVE_NORMAL, 0, CFG7F_SINGLECOLOR | CFG7F_DEFFG},
             {IDS_COLORLABEL_ACTIVESELECTED, FOCUS_ACTIVE_SELECTED, 0, CFG7F_SINGLECOLOR | CFG7F_DEFFG},
             {IDS_COLORLABEL_INACTIVENORMAL, FOCUS_FG_INACTIVE_NORMAL, FOCUS_BK_INACTIVE_NORMAL, CFG7F_DEFBK},
             {IDS_COLORLABEL_INACTIVESELECTED, FOCUS_FG_INACTIVE_SELECTED, FOCUS_BK_INACTIVE_SELECTED, CFG7F_DEFBK},
             {0, 0, 0, 0}}},
        // pen colors for the frame around thumbnails
        {
            IDS_COLORITEM_THUMBNAILFRAME,
            {{IDS_COLORLABEL_NORMAL, THUMBNAIL_FRAME_NORMAL, 0, CFG7F_SINGLECOLOR | CFG7F_DEFFG},
             {IDS_COLORLABEL_FOCUSED, THUMBNAIL_FRAME_FOCUSED, 0, CFG7F_SINGLECOLOR | CFG7F_DEFFG},
             {IDS_COLORLABEL_SELECTED, THUMBNAIL_FRAME_SELECTED, 0, CFG7F_SINGLECOLOR},
             {IDS_COLORLABEL_FOCUSEDSELECTED, THUMBNAIL_FRAME_FOCSEL, 0, CFG7F_SINGLECOLOR},
             {0, 0, 0, 0}}},
        // colors for icon blending
        {
            IDS_COLORITEM_BLENDEDICONS,
            {{IDS_COLORLABEL_SELECTED, ICON_BLEND_SELECTED, 0, CFG7F_SINGLECOLOR | CFG7F_DEFFG},
             {IDS_COLORLABEL_FOCUSED, ICON_BLEND_FOCUSED, 0, CFG7F_SINGLECOLOR},
             {IDS_COLORLABEL_FOCUSEDSELECTED, ICON_BLEND_FOCSEL, 0, CFG7F_SINGLECOLOR},
             {0, 0, 0, 0},
             {0, 0, 0, 0}}},
        // progress bar colors
        {
            IDS_COLORITEM_PROGRESS,
            {{IDS_COLORLABEL_LEFTPART, PROGRESS_FG_SELECTED, PROGRESS_BK_SELECTED, CFG7F_DEFFG | CFG7F_DEFBK},
             {IDS_COLORLABEL_RIGHTPART, PROGRESS_FG_NORMAL, PROGRESS_BK_NORMAL, CFG7F_DEFFG | CFG7F_DEFBK},
             {0, 0, 0, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0}}},
        // panel caption colors
        {
            IDS_COLORITEM_CAPTION,
            {{IDS_COLORLABEL_ACTIVE, ACTIVE_CAPTION_FG, ACTIVE_CAPTION_BK, CFG7F_DEFFG | CFG7F_DEFBK},
             {IDS_COLORLABEL_INACTIVE, INACTIVE_CAPTION_FG, INACTIVE_CAPTION_BK, CFG7F_DEFFG | CFG7F_DEFBK},
             {0, 0, 0, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0}}},
        // hot item colors
        {
            IDS_COLORITEM_HOT,
            {{IDS_COLORLABEL_HOTPANEL, HOT_PANEL, 0, CFG7F_SINGLECOLOR | CFG7F_DEFFG},
             {IDS_COLORLABEL_HOTACTIVE, HOT_ACTIVE, 0, CFG7F_SINGLECOLOR | CFG7F_DEFFG},
             {IDS_COLORLABEL_HOTINACTIVE, HOT_INACTIVE, 0, CFG7F_SINGLECOLOR | CFG7F_DEFFG},
             {0, 0, 0, 0},
             {0, 0, 0, 0}}},
        // autocomplete suggestion colors
        {
            IDS_COLORITEM_AUTOCOMPLETE,
            {{IDS_COLORLABEL_AUTOCOMPLETE_LIST, AUTOCOMPLETE_LIST_FG, AUTOCOMPLETE_LIST_BK, CFG7F_DEFFG | CFG7F_DEFBK},
             {0, 0, 0, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0}}},
};

int CConfigurationPage7Items[CFG_COLORS_BUTTONS] = {IDC_C_ITEM1_L, IDC_C_ITEM2_L, IDC_C_ITEM3_L, IDC_C_ITEM4_L, IDC_C_ITEM5_L};
int CConfigurationPage7Masks[CFG_COLORS_BUTTONS] = {IDC_C_MASK1_L, IDC_C_MASK2_L, IDC_C_MASK3_L, IDC_C_MASK4_L, IDC_C_MASK5_L};
int CConfigurationPage7ItemsBut[CFG_COLORS_BUTTONS] = {IDC_C_ITEM1_C, IDC_C_ITEM2_C, IDC_C_ITEM3_C, IDC_C_ITEM4_C, IDC_C_ITEM5_C};
int CConfigurationPage7MasksBut[CFG_COLORS_BUTTONS] = {IDC_C_MASK1_C, IDC_C_MASK2_C, IDC_C_MASK3_C, IDC_C_MASK4_C, IDC_C_MASK5_C};

CCfgPageColors::CCfgPageColors()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_COLORS, IDD_CFGPAGE_COLORS, PSP_USETITLE, NULL), HighlightMasks(10, 5)
{
    HScheme = NULL;
    HItem = NULL;
    int i;
    for (i = 0; i < CFG_COLORS_BUTTONS; i++)
    {
        Items[i] = NULL;
        Masks[i] = NULL;
    }

    EditLB = NULL;
    DisableNotification = FALSE;
    SourceHighlightMasks = MainWindow->HighlightMasks;
    HighlightMasks.Load(*SourceHighlightMasks);

    Dirty = FALSE;
}

void CCfgPageColors::Transfer(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageColors::Transfer()");
    if (ti.Type == ttDataToWindow)
    {
        int i;
        for (i = 0; i < NUMBER_OF_COLORS; i++)
            TmpColors[i] = UserColors[i];

        struct SchemeEntry
        {
            int id;
            int labelResId;
        };
        static const SchemeEntry schemes[] = {
            {0, IDS_COLORSCHEME_SALAMANDER},
            {1, IDS_COLORSCHEME_EXPLORER},
            {2, IDS_COLORSCHEME_NORTON},
            {3, IDS_COLORSCHEME_NAVIGATOR},
            {4, IDS_COLORSCHEME_CUSTOM},
            {5, IDS_COLORSCHEME_WINDARK},
        };
        for (i = 0; i < (int)_countof(schemes); i++)
        {
            int idx = (int)SendMessage(HScheme, CB_ADDSTRING, 0, (LPARAM)LoadStr(schemes[i].labelResId));
            if (idx != CB_ERR)
                SendMessage(HScheme, CB_SETITEMDATA, idx, schemes[i].id);
        }

        for (i = 0; i < PAGE7DATA_COUNT; i++)
            SendMessage(HItem, CB_ADDSTRING, 0, (LPARAM)LoadStr(Page7Data[i].ItemLabel));

        int labels[CFG_COLORS_BUTTONS] = {IDS_COLORLABEL_NORMAL, IDS_COLORLABEL_FOCUSED, IDS_COLORLABEL_SELECTED, IDS_COLORLABEL_FOCUSEDSELECTED, IDS_COLORLABEL_HIGHLIGHTED};
        for (i = 0; i < CFG_COLORS_BUTTONS; i++)
            SetDlgItemText(HWindow, CConfigurationPage7Masks[i], LoadStr(labels[i]));

        int schemeId = 4; // custom
        if (Configuration.UseWindowsDarkMode)
            schemeId = 5;
        else if (CurrentColors == SalamanderColors)
            schemeId = 0;
        else if (CurrentColors == ExplorerColors)
            schemeId = 1;
        else if (CurrentColors == NortonColors)
            schemeId = 2;
        else if (CurrentColors == NavigatorColors)
            schemeId = 3;
        int sel = 0;
        int count = (int)SendMessage(HScheme, CB_GETCOUNT, 0, 0);
        for (int j = 0; j < count; j++)
        {
            if ((int)SendMessage(HScheme, CB_GETITEMDATA, j, 0) == schemeId)
            {
                sel = j;
                break;
            }
        }
        SendMessage(HScheme, CB_SETCURSEL, sel, 0);
        SendMessage(HItem, CB_SETCURSEL, 0, 0);

        // populate the highlight items list
        for (i = 0; i < HighlightMasks.Count; i++)
            EditLB->AddItem((INT_PTR)HighlightMasks[i]);

        DisableNotification = TRUE;
        EditLB->SetCurSel(0);
        DisableNotification = FALSE;
        LoadColors();
        LoadMasks();
        EnableControls();
    }
    else
    {
        int index = (int)SendMessage(HScheme, CB_GETCURSEL, 0, 0);
        int schemeId = (int)SendMessage(HScheme, CB_GETITEMDATA, index, 0);
        if (schemeId == CB_ERR)
            schemeId = 4;
        if (schemeId == 0)
            CurrentColors = SalamanderColors;
        else if (schemeId == 1)
            CurrentColors = ExplorerColors;
        else if (schemeId == 2)
            CurrentColors = NortonColors;
        else if (schemeId == 3)
            CurrentColors = NavigatorColors;
        else
        {
            CurrentColors = UserColors;
            int i;
            for (i = 0; i < NUMBER_OF_COLORS; i++)
                UserColors[i] = TmpColors[i];
        }

        BOOL oldUseWindowsDarkMode = Configuration.UseWindowsDarkMode;
        Configuration.UseWindowsDarkMode = (schemeId == 5);
        BOOL windowsDarkModeChanged = oldUseWindowsDarkMode != Configuration.UseWindowsDarkMode;
        ColorsChanged(TRUE, !windowsDarkModeChanged, FALSE); // switching Windows Dark Mode scheme changes toolbar SVGs/backgrounds, so reload icons

        SourceHighlightMasks->Load(HighlightMasks);
        int errPos;
        int i;
        for (i = 0; i < SourceHighlightMasks->Count; i++)
            SourceHighlightMasks->At(i)->Masks->PrepareMasks(errPos);
    }
}

void CCfgPageColors::LoadColors()
{
    CALL_STACK_MESSAGE1("CCfgPageColors::LoadColors()");

    COLORREF* tmpColors;
    int index = (int)SendMessage(HScheme, CB_GETCURSEL, 0, 0);
    int schemeId = (int)SendMessage(HScheme, CB_GETITEMDATA, index, 0);
    if (schemeId == CB_ERR)
        schemeId = 4;
    if (schemeId == 0)
        tmpColors = SalamanderColors;
    else if (schemeId == 1)
        tmpColors = ExplorerColors;
    else if (schemeId == 2)
        tmpColors = NortonColors;
    else if (schemeId == 3)
        tmpColors = NavigatorColors;
    else
        tmpColors = TmpColors;

    // let default values be retrieved
    UpdateDefaultColors(tmpColors, &HighlightMasks, TRUE, TRUE);

    index = (int)SendMessage(HItem, CB_GETCURSEL, 0, 0);
    CConfigurationPage7Data* data = &Page7Data[index];

    int i;
    for (i = 0; i < CFG_COLORS_BUTTONS; i++)
    {
        CConfigurationPage7SubData* subData = &data->Items[i];
        const char* label;
        if (subData->Label != 0)
            label = LoadStr(subData->Label);
        else
            label = "";
        SetDlgItemText(HWindow, CConfigurationPage7Items[i], label);
        if (subData->Label != 0)
        {
            if (subData->Flags & CFG7F_SINGLECOLOR)
                Items[i]->SetColor(GetCOLORREF(tmpColors[subData->ColorFg]), GetCOLORREF(tmpColors[subData->ColorFg]));
            else
                Items[i]->SetColor(GetCOLORREF(tmpColors[subData->ColorFg]), GetCOLORREF(tmpColors[subData->ColorBk]));
        }
        ShowWindow(Items[i]->HWindow, subData->Label != 0 ? SW_SHOW : SW_HIDE);
    }

    INT_PTR itemID;
    EditLB->GetCurSelItemID(itemID);
    if (itemID != -1)
    {
        CHighlightMasksItem* item = (CHighlightMasksItem*)itemID;
        Masks[0]->SetColor(GetCOLORREF(item->NormalFg), GetCOLORREF(item->NormalBk));
        Masks[1]->SetColor(GetCOLORREF(item->FocusedFg), GetCOLORREF(item->FocusedBk));
        Masks[2]->SetColor(GetCOLORREF(item->SelectedFg), GetCOLORREF(item->SelectedBk));
        Masks[3]->SetColor(GetCOLORREF(item->FocSelFg), GetCOLORREF(item->FocSelBk));
        Masks[4]->SetColor(GetCOLORREF(item->HighlightFg), GetCOLORREF(item->HighlightBk));
    }
}

#define PAGE7_CTRLCOUNT 7
DWORD Page7Attributes[PAGE7_CTRLCOUNT] = {FILE_ATTRIBUTE_ARCHIVE, FILE_ATTRIBUTE_READONLY, FILE_ATTRIBUTE_HIDDEN, FILE_ATTRIBUTE_SYSTEM, FILE_ATTRIBUTE_COMPRESSED, FILE_ATTRIBUTE_ENCRYPTED, FILE_ATTRIBUTE_DIRECTORY};
DWORD Page7Controls[PAGE7_CTRLCOUNT] = {IDC_C_ARCHIVE, IDC_C_READONLY, IDC_C_HIDDEN, IDC_C_SYSTEM, IDC_C_COMPRESSED, IDC_C_ENCRYPTED, IDC_C_DIRECTORY};

void CCfgPageColors::LoadMasks()
{
    CALL_STACK_MESSAGE1("CCfgPageColors::LoadMask()");
    INT_PTR itemID;
    EditLB->GetCurSelItemID(itemID);
    BOOL empty = FALSE;
    if (itemID == -1)
        empty = TRUE;

    CHighlightMasksItem* item = NULL;
    if (!empty)
        item = (CHighlightMasksItem*)itemID;

    DisableNotification = TRUE;
    DWORD state = BST_UNCHECKED;
    int i;
    for (i = 0; i < PAGE7_CTRLCOUNT; i++)
    {
        if (!empty)
            state = item->ValidAttr & Page7Attributes[i] ? (item->Attr & Page7Attributes[i] ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE;
        CheckDlgButton(HWindow, Page7Controls[i], state);
    }
    DisableNotification = FALSE;
}

void CCfgPageColors::StoreMasks()
{
    CALL_STACK_MESSAGE1("CCfgPageColors::StoreMask()");
    INT_PTR itemID;
    EditLB->GetCurSelItemID(itemID);
    if (!DisableNotification && itemID != -1)
    {
        CHighlightMasksItem* item = (CHighlightMasksItem*)itemID;
        item->ValidAttr = 0;
        item->Attr = 0;
        int i;
        for (i = 0; i < PAGE7_CTRLCOUNT; i++)
        {
            DWORD state = IsDlgButtonChecked(HWindow, Page7Controls[i]);
            if (state == BST_CHECKED || state == BST_UNCHECKED)
            {
                item->ValidAttr |= Page7Attributes[i];
                if (state == BST_CHECKED)
                    item->Attr |= Page7Attributes[i];
            }
        }
    }
}


static int CfgPageColorsDluY(HWND hWindow, int dluY)
{
    RECT r = {0, 0, 0, dluY};
    MapDialogRect(hWindow, &r);
    return r.bottom;
}

static void MoveDlgCtrl(HWND hWindow, HDWP& hdwp, int resID, int x, int y)
{
    HWND hCtrl = GetDlgItem(hWindow, resID);
    if (hCtrl == NULL || hdwp == NULL)
        return;

    hdwp = HANDLES(DeferWindowPos(hdwp, hCtrl, NULL,
                                  x, y, 0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER));
}


static void AlignDlgCtrlLeftOfCtrl(HWND hWindow, int labelID, int ctrlID)
{
    HWND hLabel = GetDlgItem(hWindow, labelID);
    HWND hCtrl = GetDlgItem(hWindow, ctrlID);
    if (hLabel == NULL || hCtrl == NULL)
        return;

    RECT labelR;
    RECT ctrlR;
    GetWindowRect(hLabel, &labelR);
    GetWindowRect(hCtrl, &ctrlR);
    POINT labelP = {labelR.left, labelR.top};
    POINT ctrlP = {ctrlR.left, ctrlR.top};
    ScreenToClient(hWindow, &labelP);
    ScreenToClient(hWindow, &ctrlP);

    const int gap = 5;
    SetWindowPos(hLabel, NULL, ctrlP.x - (labelR.right - labelR.left) - gap, labelP.y,
                 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

static void MoveDlgCtrlVertically(HWND hWindow, HDWP& hdwp, int resID, int y)
{
    HWND hCtrl = GetDlgItem(hWindow, resID);
    if (hCtrl == NULL || hdwp == NULL)
        return;

    RECT r;
    GetWindowRect(hCtrl, &r);
    POINT p = {r.left, r.top};
    ScreenToClient(hWindow, &p);
    MoveDlgCtrl(hWindow, hdwp, resID, p.x, y);
}

void CCfgPageColors::LayoutMaskControls()
{
    HWND hList = GetDlgItem(HWindow, IDC_C_LIST);
    if (hList == NULL)
        return;

    RECT listRect;
    GetWindowRect(hList, &listRect);
    RECT firstMaskButtonRect = {};
    GetWindowRect(GetDlgItem(HWindow, IDC_C_MASK1_C), &firstMaskButtonRect);
    RECT clientRect;
    GetClientRect(HWindow, &clientRect);

    POINT listTop = {listRect.left, listRect.top};
    ScreenToClient(HWindow, &listTop);
    const int maskButtonHeight = firstMaskButtonRect.bottom - firstMaskButtonRect.top;
    const int requiredBelowList = CfgPageColorsDluY(HWindow, 4 + 1 + 4 * 14) + maskButtonHeight;
    const int bottomPadding = CfgPageColorsDluY(HWindow, 2);
    const int maxListBottom = clientRect.bottom - bottomPadding - requiredBelowList;
    const int minListHeight = CfgPageColorsDluY(HWindow, 24);
    if (listRect.bottom - listRect.top > minListHeight)
    {
        const int currentListBottom = listTop.y + (listRect.bottom - listRect.top);
        if (currentListBottom > maxListBottom)
        {
            const int newListHeight = maxListBottom - listTop.y > minListHeight ? maxListBottom - listTop.y : minListHeight;
            SetWindowPos(hList, NULL, 0, 0, listRect.right - listRect.left, newListHeight,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            GetWindowRect(hList, &listRect);
        }
    }

    POINT listBottom = {listRect.left, listRect.bottom};
    ScreenToClient(HWindow, &listBottom);

    // Keep the attribute block anchored below the resized mask list.  The generic
    // elastic layout moves these controls as one bottom-aligned envelope, which
    // can collapse the gaps between the list, the attribute label/checkboxes and
    // the mask color buttons after the page is resized horizontally.  Reapply the
    // original vertical spacing from the dialog resource after the generic layout
    // has finished.  Convert the resource DLU offsets to pixels first; using raw
    // DLU values as pixels makes the controls look vertically glued together.
    const int attrLabelY = listBottom.y + CfgPageColorsDluY(HWindow, 4);
    const int checkColY = attrLabelY + CfgPageColorsDluY(HWindow, 9);
    const int maskLabelY = attrLabelY + CfgPageColorsDluY(HWindow, 3);
    const int maskButtonY = attrLabelY + CfgPageColorsDluY(HWindow, 1);
    const int noteY = attrLabelY + CfgPageColorsDluY(HWindow, 59);
    const int rowStep = CfgPageColorsDluY(HWindow, 14);
    const int checkStep = CfgPageColorsDluY(HWindow, 12);

    HDWP hdwp = HANDLES(BeginDeferWindowPos(1 + PAGE7_CTRLCOUNT + 1 + 2 * CFG_COLORS_BUTTONS));
    if (hdwp == NULL)
        return;

    MoveDlgCtrlVertically(HWindow, hdwp, IDC_STATIC_3, attrLabelY);
    MoveDlgCtrlVertically(HWindow, hdwp, IDC_C_ARCHIVE, checkColY);
    MoveDlgCtrlVertically(HWindow, hdwp, IDC_C_READONLY, checkColY + checkStep);
    MoveDlgCtrlVertically(HWindow, hdwp, IDC_C_HIDDEN, checkColY + 2 * checkStep);
    MoveDlgCtrlVertically(HWindow, hdwp, IDC_C_SYSTEM, checkColY + 3 * checkStep);
    MoveDlgCtrlVertically(HWindow, hdwp, IDC_C_COMPRESSED, checkColY);
    MoveDlgCtrlVertically(HWindow, hdwp, IDC_C_ENCRYPTED, checkColY + checkStep);
    MoveDlgCtrlVertically(HWindow, hdwp, IDC_C_DIRECTORY, checkColY + 2 * checkStep);
    MoveDlgCtrlVertically(HWindow, hdwp, IDC_STATIC_6, noteY);

    HWND hTopButton = GetDlgItem(HWindow, IDC_C_ITEM1_C);
    int maskButtonX = -1;
    if (hTopButton != NULL)
    {
        RECT r;
        GetWindowRect(hTopButton, &r);
        POINT p = {r.left, r.top};
        ScreenToClient(HWindow, &p);
        maskButtonX = p.x;
    }

    for (int i = 0; i < CFG_COLORS_BUTTONS; i++)
    {
        MoveDlgCtrlVertically(HWindow, hdwp, CConfigurationPage7Masks[i], maskLabelY + i * rowStep);
        if (maskButtonX >= 0)
            MoveDlgCtrl(HWindow, hdwp, CConfigurationPage7MasksBut[i], maskButtonX, maskButtonY + i * rowStep);
        else
            MoveDlgCtrlVertically(HWindow, hdwp, CConfigurationPage7MasksBut[i], maskButtonY + i * rowStep);
    }

    HANDLES(EndDeferWindowPos(hdwp));

    for (int i = 0; i < CFG_COLORS_BUTTONS; i++)
    {
        AlignDlgCtrlLeftOfCtrl(HWindow, CConfigurationPage7Items[i], CConfigurationPage7ItemsBut[i]);
        AlignDlgCtrlLeftOfCtrl(HWindow, CConfigurationPage7Masks[i], CConfigurationPage7MasksBut[i]);
    }
}

void CCfgPageColors::EnableControls()
{
    CALL_STACK_MESSAGE1("CCfgPageColors::EnableControls()");
    BOOL validItem = TRUE;
    INT_PTR itemID;
    EditLB->GetCurSelItemID(itemID);
    if (itemID == -1)
        validItem = FALSE;

    EnableWindow(GetDlgItem(HWindow, IDC_C_ARCHIVE), validItem);
    EnableWindow(GetDlgItem(HWindow, IDC_C_READONLY), validItem);
    EnableWindow(GetDlgItem(HWindow, IDC_C_HIDDEN), validItem);
    EnableWindow(GetDlgItem(HWindow, IDC_C_SYSTEM), validItem);
    EnableWindow(GetDlgItem(HWindow, IDC_C_COMPRESSED), validItem);
    EnableWindow(GetDlgItem(HWindow, IDC_C_ENCRYPTED), validItem);
    EnableWindow(GetDlgItem(HWindow, IDC_C_DIRECTORY), validItem);

    if (!validItem)
        Masks[0]->SetColor(RGB(255, 255, 255), RGB(255, 255, 255));
    EnableWindow(GetDlgItem(HWindow, IDC_C_MASK1_C), validItem);
    if (!validItem)
        Masks[1]->SetColor(RGB(255, 255, 255), RGB(255, 255, 255));
    EnableWindow(GetDlgItem(HWindow, IDC_C_MASK2_C), validItem);
    if (!validItem)
        Masks[2]->SetColor(RGB(255, 255, 255), RGB(255, 255, 255));
    EnableWindow(GetDlgItem(HWindow, IDC_C_MASK3_C), validItem);
    if (!validItem)
        Masks[3]->SetColor(RGB(255, 255, 255), RGB(255, 255, 255));
    EnableWindow(GetDlgItem(HWindow, IDC_C_MASK4_C), validItem);
    if (!validItem)
        Masks[4]->SetColor(RGB(255, 255, 255), RGB(255, 255, 255));
    EnableWindow(GetDlgItem(HWindow, IDC_C_MASK5_C), validItem);
}

void CCfgPageColors::Validate(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageColors::Validate()");
    if (Dirty)
    {
        int i;
        for (i = 0; i < HighlightMasks.Count; i++)
        {
            CMaskGroup masks(HighlightMasks[i]->Masks->GetMasksString());
            int errorPos1;
            if (!masks.PrepareMasks(errorPos1))
            {
                EditLB->SetCurSel(i);
                SalMessageBox(HWindow, LoadStr(IDS_INCORRECTSYNTAX), LoadStr(IDS_ERRORTITLE),
                              MB_OK | MB_ICONEXCLAMATION);
                ti.ErrorOn(IDC_C_LIST);
                PostMessage(HWindow, WM_USER_EDIT, errorPos1, errorPos1 + 1);
                return;
            }
        }
    }
}

INT_PTR
CCfgPageColors::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CCfgPageColors::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_CTLCOLORSTATIC:
    {
        if (DarkModeShouldUseDarkColors())
        {
            HWND ctrl = (HWND)lParam;
            int ctrlID = ctrl != NULL ? GetDlgCtrlID(ctrl) : 0;
            bool isTargetLabel = false;
            for (int i = 0; i < CFG_COLORS_BUTTONS; i++)
            {
                if (ctrlID == CConfigurationPage7Items[i] || ctrlID == CConfigurationPage7Masks[i])
                {
                    isTargetLabel = true;
                    break;
                }
            }
            if (isTargetLabel)
            {
                HDC dc = reinterpret_cast<HDC>(wParam);
                if (dc != NULL)
                {
                    const DarkModeColors& colors = DarkModeGetColors();
                    SetTextColor(dc, colors.readableText);
                    SetBkColor(dc, colors.background);
                    SetBkMode(dc, TRANSPARENT);
                }
                return reinterpret_cast<INT_PTR>(HDialogBrush != NULL ? HDialogBrush : GetSysColorBrush(COLOR_BTNFACE));
            }
        }
        INT_PTR result = 0;
        if (DarkModeTryHandleCtlColorForDialogPage(uMsg, wParam, lParam, result))
            return result;
        break;
    }

    case WM_INITDIALOG:
    {
        HScheme = GetDlgItem(HWindow, IDC_C_SCHEME);
        HItem = GetDlgItem(HWindow, IDC_C_ITEM);

        int i;
        for (i = 0; i < 5; i++)
        {
            Items[i] = new CColorArrowButton(HWindow, CConfigurationPage7ItemsBut[i], TRUE);
            Masks[i] = new CColorArrowButton(HWindow, CConfigurationPage7MasksBut[i], TRUE);
        }

        EditLB = new CEditListBox(HWindow, IDC_C_LIST);
        if (EditLB == NULL)
            TRACE_E(LOW_MEMORY);
        EditLB->MakeHeader(IDC_C_LIST_HEADER);
        EditLB->EnableDrag(::GetParent(HWindow));

        // dialog elements should stretch with the dialog size, set split controls
        ElasticVerticalLayout(1, IDC_C_LIST);

        break;
    }

    case WM_USER_EDIT:
    {
        SetFocus(GetDlgItem(HWindow, IDC_C_LIST));
        EditLB->OnBeginEdit((int)wParam, (int)lParam);
        return 0;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED)
        {
            StoreMasks();
        }

        if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_C_SCHEME || LOWORD(wParam) == IDC_C_ITEM)
        {
            if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_C_SCHEME)
            {
                int index = (int)SendMessage(HScheme, CB_GETCURSEL, 0, 0);
                int schemeId = (int)SendMessage(HScheme, CB_GETITEMDATA, index, 0);
                if (schemeId == 5)
                {
                    EditLB->DeleteAllItems();
                    WindowsDarkModeBuildPalette(TmpColors, NULL);
                    WindowsDarkModeBuildHighlightMasks(&HighlightMasks);
                    for (int i = 0; i < HighlightMasks.Count; i++)
                        EditLB->AddItem((INT_PTR)HighlightMasks[i]);
                    EditLB->SetCurSel(0);
                    LoadMasks();
                }
            }
            LoadColors();
            break;
        }
        if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == IDC_C_LIST)
        {
            EditLB->OnSelChanged();
            LoadMasks();
            LoadColors();
            EnableControls();
            break;
        }

        WORD id = LOWORD(wParam);
        if (id == IDC_C_ITEM1_C || id == IDC_C_ITEM2_C || id == IDC_C_ITEM3_C || id == IDC_C_ITEM4_C || id == IDC_C_ITEM5_C ||
            id == IDC_C_MASK1_C || id == IDC_C_MASK2_C || id == IDC_C_MASK3_C || id == IDC_C_MASK4_C || id == IDC_C_MASK5_C)
        {
            CColorArrowButton* button = (CColorArrowButton*)WindowsManager.GetWindowPtr((HWND)lParam);
            HMENU hMenu = CreatePopupMenu();

            BOOL item = (id == IDC_C_ITEM1_C || id == IDC_C_ITEM2_C || id == IDC_C_ITEM3_C || id == IDC_C_ITEM4_C || id == IDC_C_ITEM5_C);

            CHighlightMasksItem* highlightItem = NULL;
            if (!item)
            {
                INT_PTR itemID;
                EditLB->GetCurSelItemID(itemID);
                if (itemID == -1)
                {
                    TRACE_E("This should never happen!");
                    break;
                }
                highlightItem = (CHighlightMasksItem*)itemID;
            }
            BOOL singleColor = FALSE;
            BOOL enabledFg = FALSE;
            BOOL enabledBk = FALSE;
            BOOL checkedDefaultFg = FALSE;
            BOOL checkedDefaultBk = FALSE;
            COLORREF* tmpColors = NULL;
            CConfigurationPage7Data* data = NULL;
            CConfigurationPage7SubData* subData = NULL;
            SALCOLOR* fgColor = NULL;
            SALCOLOR* bkColor = NULL;
            if (item)
            {
                data = &Page7Data[SendMessage(HItem, CB_GETCURSEL, 0, 0)];
                int index;

                index = (int)SendMessage(HScheme, CB_GETCURSEL, 0, 0);
                int schemeId = (int)SendMessage(HScheme, CB_GETITEMDATA, index, 0);
                if (schemeId == CB_ERR)
                    schemeId = 4;
                if (schemeId == 0)
                    tmpColors = SalamanderColors;
                else if (schemeId == 1)
                    tmpColors = ExplorerColors;
                else if (schemeId == 2)
                    tmpColors = NortonColors;
                else if (schemeId == 3)
                    tmpColors = NavigatorColors;
                else
                    tmpColors = TmpColors;

                if (id == IDC_C_ITEM1_C)
                    index = 0;
                else if (id == IDC_C_ITEM2_C)
                    index = 1;
                else if (id == IDC_C_ITEM3_C)
                    index = 2;
                else if (id == IDC_C_ITEM4_C)
                    index = 3;
                else if (id == IDC_C_ITEM5_C)
                    index = 4;

                subData = &data->Items[index];

                if (GetFValue(tmpColors[subData->ColorFg]) & SCF_DEFAULT)
                    checkedDefaultFg = TRUE;
                if (subData->Flags & CFG7F_SINGLECOLOR)
                    singleColor = TRUE;
                else if (GetFValue(tmpColors[subData->ColorBk]) & SCF_DEFAULT)
                    checkedDefaultBk = TRUE;

                if (subData->Flags & CFG7F_DEFFG)
                    enabledFg = TRUE;
                if (subData->Flags & CFG7F_DEFBK)
                    enabledBk = TRUE;
            }
            else
            {
                enabledFg = TRUE;
                enabledBk = TRUE;

                switch (id)
                {
                case IDC_C_MASK1_C:
                {
                    fgColor = &highlightItem->NormalFg;
                    bkColor = &highlightItem->NormalBk;
                    break;
                }

                case IDC_C_MASK2_C:
                {
                    fgColor = &highlightItem->FocusedFg;
                    bkColor = &highlightItem->FocusedBk;
                    break;
                }

                case IDC_C_MASK3_C:
                {
                    fgColor = &highlightItem->SelectedFg;
                    bkColor = &highlightItem->SelectedBk;
                    break;
                }

                case IDC_C_MASK4_C:
                {
                    fgColor = &highlightItem->FocSelFg;
                    bkColor = &highlightItem->FocSelBk;
                    break;
                }

                case IDC_C_MASK5_C:
                {
                    fgColor = &highlightItem->HighlightFg;
                    bkColor = &highlightItem->HighlightBk;
                    break;
                }
                }

                if (GetFValue(*fgColor) & SCF_DEFAULT)
                    checkedDefaultFg = TRUE;
                if (GetFValue(*bkColor) & SCF_DEFAULT)
                    checkedDefaultBk = TRUE;
            }
            int maxCmd;
            if (singleColor)
            {
                /* used by the export_mnu.py script that generates salmenu.mnu for the Translator,
    keep synchronized with the InsertMenu() call below...
MENU_TEMPLATE_ITEM CfgPageColorsMenu1[] = 
{
  {MNTT_PB, 0
  {MNTT_IT, IDS_SETCOLOR_CUSTOM
  {MNTT_IT, IDS_SETCOLOR_SYSTEM
  {MNTT_PE, 0
};
*/
                InsertMenu(hMenu, 0xFFFFFFFF, checkedDefaultFg ? 0 : MF_CHECKED | MF_BYCOMMAND | MF_STRING, 1, LoadStr(IDS_SETCOLOR_CUSTOM));
                InsertMenu(hMenu, 0xFFFFFFFF, checkedDefaultFg ? MF_CHECKED : 0 | MF_BYCOMMAND | MF_STRING | enabledFg ? 0
                                                                                                                       : MF_GRAYED,
                           2, LoadStr(IDS_SETCOLOR_SYSTEM));
                maxCmd = 2;
            }
            else
            {
                /* used by the export_mnu.py script that generates salmenu.mnu for the Translator,
    keep synchronized with the InsertMenu() call below...
MENU_TEMPLATE_ITEM CfgPageColorsMenu2[] = 
{
  {MNTT_PB, 0
  {MNTT_IT, IDS_SETCOLOR_CUSTOM_FG
  {MNTT_IT, IDS_SETCOLOR_SYSTEM_FG
  {MNTT_IT, IDS_SETCOLOR_CUSTOM_BK
  {MNTT_IT, IDS_SETCOLOR_SYSTEM_BK
  {MNTT_PE, 0
};
MENU_TEMPLATE_ITEM CfgPageColorsMenu3[] = 
{
  {MNTT_PB, 0
  {MNTT_IT, IDS_SETCOLOR_CUSTOM_FG
  {MNTT_IT, IDS_SETCOLOR_DEFAULT_FG
  {MNTT_IT, IDS_SETCOLOR_CUSTOM_BK
  {MNTT_IT, IDS_SETCOLOR_DEFAULT_BK
  {MNTT_PE, 0
};
*/
                InsertMenu(hMenu, 0xFFFFFFFF, checkedDefaultFg ? 0 : MF_CHECKED | MF_BYCOMMAND | MF_STRING, 1, LoadStr(IDS_SETCOLOR_CUSTOM_FG));
                int textResID = (item || id == IDC_C_MASK5_C) ? IDS_SETCOLOR_SYSTEM_FG : IDS_SETCOLOR_DEFAULT_FG;
                InsertMenu(hMenu, 0xFFFFFFFF, checkedDefaultFg ? MF_CHECKED : 0 | MF_BYCOMMAND | MF_STRING | enabledFg ? 0
                                                                                                                       : MF_GRAYED,
                           2, LoadStr(textResID));
                InsertMenu(hMenu, 0xFFFFFFFF, MF_BYCOMMAND | MF_SEPARATOR, 0, NULL);
                InsertMenu(hMenu, 0xFFFFFFFF, checkedDefaultBk ? 0 : MF_CHECKED | MF_BYCOMMAND | MF_STRING, 3, LoadStr(IDS_SETCOLOR_CUSTOM_BK));
                textResID = (item || id == IDC_C_MASK5_C) ? IDS_SETCOLOR_SYSTEM_BK : IDS_SETCOLOR_DEFAULT_BK;
                InsertMenu(hMenu, 0xFFFFFFFF, checkedDefaultBk ? MF_CHECKED : 0 | MF_BYCOMMAND | MF_STRING | enabledBk ? 0
                                                                                                                       : MF_GRAYED,
                           4, LoadStr(textResID));
                maxCmd = 4;
            }

            //        int i;
            //        for (i = 0; i < maxCmd; i++)
            //          SetMenuItemBitmaps(hMenu, i + 1, MF_BYCOMMAND, NULL, HMenuCheckDot);

            TPMPARAMS tpmPar;
            tpmPar.cbSize = sizeof(tpmPar);
            GetWindowRect(button->HWindow, &tpmPar.rcExclude);
            DWORD cmd = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, tpmPar.rcExclude.right, tpmPar.rcExclude.top,
                                         HWindow, &tpmPar);
            if (cmd != 0)
            {
                int index = (int)SendMessage(HScheme, CB_GETCURSEL, 0, 0);
                int schemeId = (int)SendMessage(HScheme, CB_GETITEMDATA, index, 0);
                const bool forceDarkCommonDialog = (schemeId == 5);
                if (schemeId >= 0 && schemeId <= 3)
                {
                    COLORREF* colors;
                    if (schemeId == 0)
                        colors = SalamanderColors;
                    else if (schemeId == 1)
                        colors = ExplorerColors;
                    else if (schemeId == 2)
                        colors = NortonColors;
                    else if (schemeId == 3)
                        colors = NavigatorColors;

                    int i;
                    for (i = 0; i < NUMBER_OF_COLORS; i++)
                        TmpColors[i] = colors[i];
                    int count = (int)SendMessage(HScheme, CB_GETCOUNT, 0, 0);
                    for (int j = 0; j < count; j++)
                    {
                        if ((int)SendMessage(HScheme, CB_GETITEMDATA, j, 0) == 4)
                        {
                            SendMessage(HScheme, CB_SETCURSEL, j, 0);
                            break;
                        }
                    }
                }

                if (cmd == 1 || cmd == 3)
                {
                    CHOOSECOLOR cc;
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = HWindow;
                    cc.lpCustColors = (LPDWORD)CustomColors;

                    if (cmd == 1)
                        cc.rgbResult = button->GetTextColor();
                    else
                        cc.rgbResult = button->GetBkgndColor();
                    cc.Flags = CC_RGBINIT | CC_FULLOPEN;
                    DarkModePrepareChooseColor(&cc, forceDarkCommonDialog);
                    if (ChooseColor(&cc))
                    {
                        if (item)
                        {
                            int colorIndex = (cmd == 1) ? subData->ColorFg : subData->ColorBk;
                            BYTE flags = GetFValue(TmpColors[colorIndex]);
                            flags &= ~SCF_DEFAULT;
                            TmpColors[colorIndex] = cc.rgbResult & 0x00ffffff | (((DWORD)flags) << 24);
                        }
                        else
                        {
                            // masks
                            SALCOLOR* color = (cmd == 1) ? fgColor : bkColor;
                            BYTE flags = GetFValue(*color);
                            flags &= ~SCF_DEFAULT;
                            *color = cc.rgbResult & 0x00ffffff | (((DWORD)flags) << 24);
                        }
                    }
                }
                else
                {
                    // cmd == 2 || cmd == 4
                    if (item)
                    {
                        int colorIndex = (cmd == 2) ? subData->ColorFg : subData->ColorBk;
                        BYTE flags = GetFValue(TmpColors[colorIndex]);
                        flags |= SCF_DEFAULT;
                        TmpColors[colorIndex] = TmpColors[colorIndex] & 0x00ffffff | (((DWORD)flags) << 24);
                    }
                    else
                    {
                        SALCOLOR* color = (cmd == 2) ? fgColor : bkColor;
                        BYTE flags = GetFValue(*color);
                        flags |= SCF_DEFAULT;
                        *color = *color & 0x00ffffff | (((DWORD)flags) << 24);
                    }
                }
                // load the colors into the buttons
                LoadColors();
            }
            DestroyMenu(hMenu);
            return 0;
        }
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR* nmhdr = (NMHDR*)lParam;
        switch (nmhdr->idFrom)
        {
        case IDC_C_LIST:
        {
            switch (nmhdr->code)
            {
            case EDTLBN_GETDISPINFO:
            {
                EDTLB_DISPINFO* dispInfo = (EDTLB_DISPINFO*)lParam;
                if (dispInfo->ToDo == edtlbGetData)
                {
                    lstrcpyn(dispInfo->Buffer, ((CHighlightMasksItem*)dispInfo->ItemID)->Masks->GetMasksString(), MAX_PATH);
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);
                    return TRUE;
                }
                else
                {
                    Dirty = TRUE;
                    CHighlightMasksItem* item;
                    if (dispInfo->ItemID == -1)
                    {
                        item = new CHighlightMasksItem();
                        if (item == NULL)
                        {
                            TRACE_E(LOW_MEMORY);
                            SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
                            return TRUE;
                        }
                        HighlightMasks.Add(item);
                        item->Set(dispInfo->Buffer);
                        EditLB->SetItemData((INT_PTR)item);
                        PostMessage(HWindow, WM_COMMAND, MAKELPARAM(IDC_C_LIST, LBN_SELCHANGE), (LPARAM)EditLB->HWindow);
                    }
                    else
                    {
                        item = (CHighlightMasksItem*)dispInfo->ItemID;
                        item->Set(dispInfo->Buffer);
                    }

                    LoadMasks();
                    LoadColors();
                    EnableControls();
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
                break;
            }
                /*
            case EDTLBN_MOVEITEM:
            {
              EDTLB_DISPINFO *dispInfo = (EDTLB_DISPINFO *)lParam;
              int index;
              EditLB->GetCurSel(index);
              int srcIndex = index;
              int dstIndex = index + (dispInfo->Up ? -1 : 1);

              char buf[sizeof(CHighlightMasksItem)];
              memcpy(buf, HighlightMasks[srcIndex], sizeof(CHighlightMasksItem));
              memcpy(HighlightMasks[srcIndex], HighlightMasks[dstIndex], sizeof(CHighlightMasksItem));
              memcpy(HighlightMasks[dstIndex], buf, sizeof(CHighlightMasksItem));

              SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);  // allow swapping
              return TRUE;
            }
*/
            case EDTLBN_MOVEITEM2:
            {
                EDTLB_DISPINFO* dispInfo = (EDTLB_DISPINFO*)lParam;
                int index;
                EditLB->GetCurSel(index);
                int srcIndex = index;
                int dstIndex = dispInfo->NewIndex;

                char buf[sizeof(CHighlightMasksItem)];
                memcpy(buf, HighlightMasks[srcIndex], sizeof(CHighlightMasksItem));
                if (srcIndex < dstIndex)
                {
                    int i;
                    for (i = srcIndex; i < dstIndex; i++)
                        memcpy(HighlightMasks[i], HighlightMasks[i + 1], sizeof(CHighlightMasksItem));
                }
                else
                {
                    int i;
                    for (i = srcIndex; i > dstIndex; i--)
                        memcpy(HighlightMasks[i], HighlightMasks[i - 1], sizeof(CHighlightMasksItem));
                }
                memcpy(HighlightMasks[dstIndex], buf, sizeof(CHighlightMasksItem));

                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE); // allow change
                return TRUE;
            }

            case EDTLBN_DELETEITEM:
            {
                int index;
                EditLB->GetCurSel(index);
                HighlightMasks.Delete(index);
                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE); // allow deletion
                return TRUE;
            }
            }
            break;
        }
        }
        break;
    }

    case WM_DRAWITEM:
    {
        int idCtrl = (int)wParam;
        if (idCtrl == IDC_C_LIST)
        {
            EditLB->OnDrawItem(lParam);
            return TRUE;
        }
        break;
    }
    }

    INT_PTR result = CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
    if (uMsg == WM_INITDIALOG || uMsg == WM_SIZE)
        LayoutMaskControls();
    return result;
}

//
// ****************************************************************************
// CCfgPageHistory
//

CCfgPageHistory::CCfgPageHistory()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_HISTORY, IDD_CFGPAGE_HISTORY, PSP_USETITLE, NULL)
{
}

void CCfgPageHistory::Transfer(CTransferInfo& ti)
{
    ti.CheckBox(IDC_HISTORY_SAVEHISTORY, Configuration.SaveHistory);
    ti.CheckBox(IDC_HISTORY_WORKDIRS, Configuration.SaveWorkDirs);
    ti.CheckBox(IDC_HISTORY_ENABLECMDLINE, Configuration.EnableCmdLineHistory);
    ti.CheckBox(IDC_HISTORY_SAVECMDLINE, Configuration.SaveCmdLineHistory);

    CWorkDirsHistoryScope oldScope = (CWorkDirsHistoryScope)Configuration.WorkDirsHistoryScope;
    ti.RadioButton(IDC_HISTORY_WORKDIRS_SHARED, wdhsShared, Configuration.WorkDirsHistoryScope);
    ti.RadioButton(IDC_HISTORY_WORKDIRS_PER_TAB, wdhsPerTab, Configuration.WorkDirsHistoryScope);

    if (ti.Type == ttDataToWindow)
        EnableControls();
    else
    {
        if (oldScope != (CWorkDirsHistoryScope)Configuration.WorkDirsHistoryScope)
            MainWindow->HandleWorkDirsHistoryScopeChange(oldScope);
        MainWindow->EditWindow->FillHistory();
    }
}

void CCfgPageHistory::EnableControls()
{
    BOOL saveHistory = IsDlgButtonChecked(HWindow, IDC_HISTORY_SAVEHISTORY) == BST_CHECKED;
    if (!saveHistory)
    {
        CheckDlgButton(HWindow, IDC_HISTORY_WORKDIRS, BST_UNCHECKED);
    }
    EnableWindow(GetDlgItem(HWindow, IDC_HISTORY_WORKDIRS), saveHistory);

    BOOL enableCmdLineHistory = IsDlgButtonChecked(HWindow, IDC_HISTORY_ENABLECMDLINE) == BST_CHECKED;
    if (!saveHistory || !enableCmdLineHistory)
        CheckDlgButton(HWindow, IDC_HISTORY_SAVECMDLINE, BST_UNCHECKED);
    EnableWindow(GetDlgItem(HWindow, IDC_HISTORY_SAVECMDLINE), saveHistory && enableCmdLineHistory);
}

void CCfgPageHistory::OnClearHistory()
{
    // set panel paths to fixed (otherwise leaving the path could
    // immediately populate Alt+F12, etc.)
    if (MainWindow->LeftPanel != NULL)
        MainWindow->LeftPanel->ChangeToRescuePathOrFixedDrive(HWindow);
    if (MainWindow->RightPanel != NULL)
        MainWindow->RightPanel->ChangeToRescuePathOrFixedDrive(HWindow);

    // main window history (FileHistory, DirHistory)
    MainWindow->ClearHistory();

    // history from the configuration (SelectHistory, CopyHistory, EditHistory, ChangeDirHistory, FileListHistory)
    Configuration.ClearHistory();
    MainWindow->EditWindow->FillHistory();

    // Find dialog history including combobox of open windows
    ClearFindHistory(FALSE);

    // internal viewer history including combobox of open Find windows
    ClearViewerHistory(FALSE);

    // storage of selected names
    GlobalSelection.Clear();

    // history of both panels (PathHistory, FilterHistory, Selection)
    if (MainWindow->LeftPanel != NULL)
        MainWindow->LeftPanel->ClearHistory();
    if (MainWindow->RightPanel != NULL)
        MainWindow->RightPanel->ClearHistory();

    // clear history in all plugins (they remain loaded so that a subsequent
    // Save can remove data from the Registry)
    Plugins.ClearHistory(HWindow);

    // also clear the memory of recently used directories on individual drives
    InitDefaultDir();
}

INT_PTR
CCfgPageHistory::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_HISTORY_CLEARHISTORY:
        {
            if (SalMessageBox(HWindow, LoadStr(IDS_CLEARHISTORY_CNFRM), LoadStr(IDS_QUESTION),
                              MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
                OnClearHistory();
                SalMessageBox(HWindow, LoadStr(IDS_HISTORY_WAS_CLEARED), LoadStr(IDS_INFOTITLE),
                              MB_OK | MB_ICONINFORMATION);
            }
            return 0;
        }

        case IDC_HISTORY_SAVEHISTORY:
        case IDC_HISTORY_ENABLECMDLINE:
        {
            EnableControls();
            break;
        }
        }
        break;
    }
    }
    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}
