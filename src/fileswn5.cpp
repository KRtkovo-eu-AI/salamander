// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"

#include <new>
#include <string>
#include <vector>

#include "cfgdlg.h"
#include "mainwnd.h"
#include "plugins.h"
#include "fileswnd.h"
#include "filesbox.h"
#include "dialogs.h"
#include "snooper.h"
#include "worker.h"
#include "cache.h"
#include "usermenu.h"
#include "execute.h"
#include "pack.h"
#include "viewer.h"
#include "codetbl.h"
#include "find.h"
#include "menu.h"
#include "edtlbwnd.h"
#include "common/widepath.h"
#include "filetags.h"

static void FormatEditPropertiesLocalizedArguments(char* output, size_t outputSize,
                                                   const char* format, const char* const* arguments,
                                                   size_t argumentCount)
{
    std::string result;
    size_t argument = 0;
    for (const char* p = format != NULL ? format : ""; *p != 0;)
    {
        if (p[0] == '%' && p[1] == '%')
        {
            result.push_back('%');
            p += 2;
        }
        else
        {
            size_t tokenLength = 0;
            if (p[0] == '%' && (p[1] == 'd' || p[1] == 's'))
                tokenLength = 2;
            else if (p[0] == '%' && strncmp(p, "%08X", 4) == 0)
                tokenLength = 4;

            if (tokenLength != 0 && argument < argumentCount)
            {
                const char* value = arguments[argument++];
                result.append(value != NULL ? value : "");
                p += tokenLength;
            }
            else
                result.push_back(*p++);
        }
    }
    _snprintf_s(output, outputSize, _TRUNCATE, "%s", result.c_str());
}

class CEditWindowsPropertiesDialog : public CCommonDialog
{
    enum
    {
        EDPROP_DYNAMIC_CHECK_FIRST = 30000,
        EDPROP_DYNAMIC_EDIT_FIRST = 31000
    };

    struct CTagItem
    {
        std::string Text;
    };

    struct CPropertyRow
    {
        int ExplorerIndex;
        HWND HCheck;
        HWND HEdit;
        BOOL HadValue;
        BOOL Writable;
    };

    const std::vector<std::wstring>& Paths;
    CEditListBox* TagsList;
    std::vector<CTagItem*> Tags;
    std::vector<CPropertyRow> PropertyRows;
    int PropertyScrollPos;
    BOOL TagsHadValue;
    BOOL TagsWritable;

    BOOL IsMultiple() const { return Paths.size() > 1; }

    BOOL IsPropertyWritableForAll(REFPROPERTYKEY key) const
    {
        if (Paths.empty())
            return FALSE;
        for (size_t i = 0; i < Paths.size(); i++)
            if (!IsFilePropertyWritableW(Paths[i].c_str(), key))
                return FALSE;
        return TRUE;
    }

    static void TrimTag(std::string& value)
    {
        size_t first = value.find_first_not_of(" \t\r\n");
        size_t last = value.find_last_not_of(" \t\r\n");
        if (first == std::string::npos)
            value.clear();
        else
            value = value.substr(first, last - first + 1);
    }

    BOOL HasTag(const std::string& value, const CTagItem* except = NULL) const
    {
        for (size_t i = 0; i < Tags.size(); i++)
            if (Tags[i] != except && _stricmp(Tags[i]->Text.c_str(), value.c_str()) == 0)
                return TRUE;
        return FALSE;
    }

    void ClearTags()
    {
        for (size_t i = 0; i < Tags.size(); i++)
            delete Tags[i];
        Tags.clear();
    }

    void UpdateEnabledState()
    {
        BOOL tagsEnabled = TagsWritable &&
                           IsDlgButtonChecked(HWindow, IDC_EDPROP_TAGS_ENABLE) == BST_CHECKED;
        if (TagsList != NULL)
            TagsList->Enable(tagsEnabled);
        for (size_t i = 0; i < PropertyRows.size(); i++)
            EnableWindow(PropertyRows[i].HEdit, PropertyRows[i].Writable &&
                         SendMessage(PropertyRows[i].HCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }

    void LayoutPropertyRows()
    {
        HWND group = GetDlgItem(HWindow, IDC_EDPROP_PROPERTIES_GROUP);
        HWND scroll = GetDlgItem(HWindow, IDC_EDPROP_PROPERTIES_SCROLL);
        RECT groupRect;
        GetWindowRect(group, &groupRect);
        POINT topLeft = {groupRect.left, groupRect.top};
        POINT bottomRight = {groupRect.right, groupRect.bottom};
        ScreenToClient(HWindow, &topLeft);
        ScreenToClient(HWindow, &bottomRight);

        RECT dlu = {0, 0, 135, 18};
        MapDialogRect(HWindow, &dlu);
        int rowHeight = dlu.bottom;
        int checkWidth = dlu.right;
        RECT marginDlu = {0, 0, 8, 14};
        MapDialogRect(HWindow, &marginDlu);
        int left = topLeft.x + marginDlu.right;
        int top = topLeft.y + marginDlu.bottom;
        int bottom = bottomRight.y - marginDlu.right;
        int visibleRows = max(1, (bottom - top) / rowHeight);
        int maxScroll = max(0, (int)PropertyRows.size() - visibleRows);
        if (PropertyScrollPos > maxScroll)
            PropertyScrollPos = maxScroll;

        SCROLLINFO info;
        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);
        info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        info.nMin = 0;
        info.nMax = max(0, (int)PropertyRows.size() - 1);
        info.nPage = visibleRows;
        info.nPos = PropertyScrollPos;
        SetScrollInfo(scroll, SB_CTL, &info, TRUE);
        ShowWindow(scroll, maxScroll > 0 ? SW_SHOW : SW_HIDE);

        RECT scrollRect;
        GetWindowRect(scroll, &scrollRect);
        int right = max(left + checkWidth + 40, bottomRight.x - marginDlu.right -
                                                   (maxScroll > 0 ? scrollRect.right - scrollRect.left + 3 : 0));
        for (size_t i = 0; i < PropertyRows.size(); i++)
        {
            int visibleIndex = (int)i - PropertyScrollPos;
            BOOL visible = visibleIndex >= 0 && visibleIndex < visibleRows;
            if (visible)
            {
                int y = top + visibleIndex * rowHeight;
                MoveWindow(PropertyRows[i].HCheck, left, y, checkWidth, rowHeight - 2, TRUE);
                MoveWindow(PropertyRows[i].HEdit, left + checkWidth + 4, y,
                           max(40, right - left - checkWidth - 4), rowHeight - 2, TRUE);
            }
            ShowWindow(PropertyRows[i].HCheck, visible ? SW_SHOW : SW_HIDE);
            ShowWindow(PropertyRows[i].HEdit, visible ? SW_SHOW : SW_HIDE);
        }
    }

    void AddPropertyRows()
    {
        HFONT font = (HFONT)SendMessage(HWindow, WM_GETFONT, 0, 0);
        int count = min(GetExplorerColumnCount(), EXPLORER_COLUMNS_COUNT);
        for (int i = 0; i < count; i++)
        {
            const PROPERTYKEY* key = GetExplorerColumnPropertyKey(i);
            if (key == NULL || IsEqualPropertyKey(*key, PKEY_Keywords) ||
                !MainWindow->ViewTemplates.IsExplorerColumnAvailable(i))
                continue;

            DWORD checkStyle = WS_CHILD | WS_TABSTOP |
                               (IsMultiple() ? BS_AUTO3STATE : BS_AUTOCHECKBOX);
            HWND check = CreateWindowEx(0, "BUTTON", GetExplorerColumnName(i), checkStyle,
                                        0, 0, 0, 0, HWindow,
                                        (HMENU)(INT_PTR)(EDPROP_DYNAMIC_CHECK_FIRST + PropertyRows.size()),
                                        HInstance, NULL);
            HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                        WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                        0, 0, 0, 0, HWindow,
                                        (HMENU)(INT_PTR)(EDPROP_DYNAMIC_EDIT_FIRST + PropertyRows.size()),
                                        HInstance, NULL);
            if (check == NULL || edit == NULL)
            {
                if (check != NULL)
                    DestroyWindow(check);
                if (edit != NULL)
                    DestroyWindow(edit);
                continue;
            }
            SendMessage(check, WM_SETFONT, (WPARAM)font, TRUE);
            SendMessage(edit, WM_SETFONT, (WPARAM)font, TRUE);
            std::wstring current;
            if (!Paths.empty())
                ReadFilePropertyTextW(Paths[0].c_str(), *key, current);
            SetWindowTextW(edit, current.c_str());
            BOOL hadValue = !current.empty();
            BOOL writable = IsPropertyWritableForAll(*key);
            SendMessage(check, BM_SETCHECK, writable ? (IsMultiple() ? BST_INDETERMINATE
                                                                     : hadValue ? BST_CHECKED : BST_UNCHECKED)
                                                     : BST_UNCHECKED,
                        0);
            EnableWindow(check, writable);
            CPropertyRow row = {i, check, edit, hadValue, writable};
            PropertyRows.push_back(row);
        }
        LayoutPropertyRows();
    }

    void AddInitialTags()
    {
        std::vector<std::wstring> current;
        if (!Paths.empty())
            ReadFileTagsW(Paths[0].c_str(), current);
        TagsHadValue = !current.empty();
        for (size_t i = 0; i < current.size(); i++)
        {
#ifdef new
#undef new
#define RESTORE_INITIAL_TAG_ITEM_DEBUG_NEW_MACRO
#endif
            CTagItem* item = new (std::nothrow) CTagItem;
#ifdef RESTORE_INITIAL_TAG_ITEM_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_INITIAL_TAG_ITEM_DEBUG_NEW_MACRO
#endif
            if (item == NULL)
                break;
            item->Text = SalWideToMultiBytePath(current[i].c_str(), CP_ACP);
            Tags.push_back(item);
            TagsList->AddItem((INT_PTR)item);
        }
        SendMessage(GetDlgItem(HWindow, IDC_EDPROP_TAGS_ENABLE), BM_SETCHECK,
                    IsMultiple() ? BST_INDETERMINATE : TagsHadValue ? BST_CHECKED : BST_UNCHECKED, 0);
        if (!Tags.empty())
            TagsList->SetCurSel(0);
    }

    void ShowRecentTags(HWND edit, POINT point)
    {
        CMenuPopup popup;
        std::vector<std::string> displayTags;
        displayTags.reserve(TAG_HISTORY_SIZE);
        for (int i = 0; i < TAG_HISTORY_SIZE && Configuration.TagHistory[i] != NULL; i++)
        {
            std::wstring wide = SalMultiByteToWidePath(Configuration.TagHistory[i], CP_UTF8);
            displayTags.push_back(SalWideToMultiBytePath(wide.c_str(), CP_ACP));
            MENU_ITEM_INFO item;
            ZeroMemory(&item, sizeof(item));
            item.Mask = MENU_MASK_TYPE | MENU_MASK_ID | MENU_MASK_STRING;
            item.Type = MENU_TYPE_STRING;
            item.ID = i + 1;
            item.String = (char*)displayTags.back().c_str();
            popup.InsertItem(-1, TRUE, &item);
        }
        if (displayTags.empty())
            return;
        DWORD command = popup.Track(MENU_TRACK_RETURNCMD | MENU_TRACK_RIGHTBUTTON,
                                    point.x, point.y, HWindow, NULL);
        if (command > 0 && command <= displayTags.size())
        {
            SetWindowText(edit, displayTags[command - 1].c_str());
            SetFocus(edit);
            SendMessage(edit, EM_SETSEL, 0, -1);
        }
    }

    std::vector<std::wstring> GetTags() const
    {
        std::vector<std::wstring> result;
        for (size_t i = 0; i < Tags.size(); i++)
        {
            std::wstring tag = SalMultiByteToWidePath(Tags[i]->Text.c_str(), CP_ACP);
            if (!tag.empty())
                result.push_back(tag);
        }
        return result;
    }

    BOOL Apply()
    {
        if (TagsList != NULL)
        {
            TagsList->OnSaveEdit();
            TagsList->OnEndEdit();
        }
        int tagsState = IsDlgButtonChecked(HWindow, IDC_EDPROP_TAGS_ENABLE);
        std::vector<std::wstring> tags = GetTags();
        BOOL anyAttempted = FALSE;
        BOOL tagsSucceeded = FALSE;
        int updated = 0;
        int failed = 0;
        std::wstring firstFailedPath;
        std::string firstFailedProperty;
        HRESULT firstFailure = S_OK;
        BOOL firstFailureUnsupported = FALSE;
        HCURSOR oldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        for (size_t pathIndex = 0; pathIndex < Paths.size(); pathIndex++)
        {
            BOOL fileUpdated = FALSE;
            BOOL fileFailed = FALSE;
            if (TagsWritable && tagsState != BST_INDETERMINATE &&
                (tagsState == BST_CHECKED || IsMultiple() || TagsHadValue))
            {
                anyAttempted = TRUE;
                std::vector<std::wstring> values = tagsState == BST_CHECKED
                                                       ? tags
                                                       : std::vector<std::wstring>();
                HRESULT hr = WriteFileTagsW(Paths[pathIndex].c_str(), values);
                if (SUCCEEDED(hr))
                {
                    fileUpdated = TRUE;
                    tagsSucceeded = TRUE;
                }
                else
                {
                    fileFailed = TRUE;
                    if (firstFailedPath.empty())
                    {
                        firstFailedPath = Paths[pathIndex];
                        firstFailedProperty = LoadStr(IDS_FFA_TAGS);
                        firstFailure = hr;
                        firstFailureUnsupported = !IsFilePropertyWritableW(
                            Paths[pathIndex].c_str(), PKEY_Keywords);
                    }
                }
            }
            for (size_t rowIndex = 0; rowIndex < PropertyRows.size(); rowIndex++)
            {
                CPropertyRow& row = PropertyRows[rowIndex];
                if (!row.Writable)
                    continue;
                int state = (int)SendMessage(row.HCheck, BM_GETCHECK, 0, 0);
                if (state == BST_INDETERMINATE ||
                    (state == BST_UNCHECKED && !IsMultiple() && !row.HadValue))
                    continue;
                const PROPERTYKEY* key = GetExplorerColumnPropertyKey(row.ExplorerIndex);
                if (key == NULL)
                    continue;
                anyAttempted = TRUE;
                int length = GetWindowTextLengthW(row.HEdit);
                std::vector<wchar_t> value(length + 1);
                GetWindowTextW(row.HEdit, value.data(), (int)value.size());
                HRESULT hr;
                if (state == BST_CHECKED &&
                    GetExplorerColumnType(row.ExplorerIndex) == (VT_VECTOR | VT_LPWSTR))
                {
                    std::vector<std::wstring> values;
                    ParseFileTagsW(value.data(), values);
                    hr = WriteFileStringVectorPropertyW(Paths[pathIndex].c_str(), *key, values);
                }
                else
                    hr = WriteFilePropertyTextW(Paths[pathIndex].c_str(), *key, value.data(),
                                                state != BST_CHECKED || value[0] == 0);
                if (SUCCEEDED(hr))
                    fileUpdated = TRUE;
                else
                {
                    fileFailed = TRUE;
                    if (firstFailedPath.empty())
                    {
                        firstFailedPath = Paths[pathIndex];
                        firstFailedProperty = GetExplorerColumnName(row.ExplorerIndex);
                        firstFailure = hr;
                        firstFailureUnsupported = !IsFilePropertyWritableW(
                            Paths[pathIndex].c_str(), *key);
                    }
                }
            }
            if (fileUpdated)
                updated++;
            if (fileFailed)
                failed++;
        }
        SetCursor(oldCursor);

        if (tagsSucceeded)
        {
            for (size_t i = tags.size(); i > 0; i--)
            {
                std::string tag = SalWideToMultiBytePath(tags[i - 1].c_str(), CP_UTF8);
                AddValueToStdHistoryValues(Configuration.TagHistory, TAG_HISTORY_SIZE,
                                           tag.c_str(), FALSE);
            }
        }
        if (failed > 0 || (anyAttempted && Paths.size() > 1))
        {
            char result[1200];
            char updatedText[32];
            char failedText[32];
            _snprintf_s(updatedText, _countof(updatedText), _TRUNCATE, "%d", updated);
            _snprintf_s(failedText, _countof(failedText), _TRUNCATE, "%d", failed);
            const char* resultArguments[] = {updatedText, failedText};
            FormatEditPropertiesLocalizedArguments(result, _countof(result),
                                                    LoadStr(IDS_EDPROP_RESULT), resultArguments,
                                                    _countof(resultArguments));
            if (failed > 0 && !firstFailedPath.empty())
            {
                const char* reason = LoadStr(IDS_EDPROP_REASON_OTHER);
                DWORD attributes = GetFileAttributesW(firstFailedPath.c_str());
                if (firstFailure == E_ACCESSDENIED || firstFailure == STG_E_ACCESSDENIED ||
                    firstFailure == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) ||
                    firstFailure == HRESULT_FROM_WIN32(ERROR_WRITE_PROTECT) ||
                    firstFailure == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) ||
                    (attributes != INVALID_FILE_ATTRIBUTES &&
                     (attributes & FILE_ATTRIBUTE_READONLY) != 0))
                    reason = LoadStr(IDS_EDPROP_REASON_READONLY);
                else if (firstFailure == (HRESULT)0x88982F41L || firstFailureUnsupported)
                    reason = LoadStr(IDS_EDPROP_REASON_UNSUPPORTED);

                char detail[800];
                std::string path = SalWideToMultiBytePath(firstFailedPath.c_str(), CP_ACP);
                char failureCode[32];
                _snprintf_s(failureCode, _countof(failureCode), _TRUNCATE, "%08X",
                            (DWORD)firstFailure);
                const char* detailArguments[] = {
                    path.c_str(), firstFailedProperty.c_str(), reason, failureCode};
                FormatEditPropertiesLocalizedArguments(detail, _countof(detail),
                                                        LoadStr(IDS_EDPROP_RESULT_DETAIL),
                                                        detailArguments, _countof(detailArguments));
                strncat_s(result, _countof(result), "\n\n", _TRUNCATE);
                strncat_s(result, _countof(result), detail, _TRUNCATE);
            }
            SalMessageBox(HWindow, result, LoadStr(IDS_EDPROP_TITLE),
                          failed > 0 ? MB_ICONEXCLAMATION | MB_OK : MB_ICONINFORMATION | MB_OK);
        }
        return failed == 0;
    }

public:
    CEditWindowsPropertiesDialog(HWND parent, const std::vector<std::wstring>& paths)
        : CCommonDialog(HLanguage, IDD_EDIT_PROPERTIES, parent), Paths(paths)
    {
        TagsList = NULL;
        PropertyScrollPos = 0;
        TagsHadValue = FALSE;
        TagsWritable = FALSE;
    }

    ~CEditWindowsPropertiesDialog() { ClearTags(); }

protected:
    virtual INT_PTR DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_INITDIALOG:
        {
            SetWindowText(HWindow, LoadStr(IDS_EDPROP_TITLE));
            HWND tagsCheck = GetDlgItem(HWindow, IDC_EDPROP_TAGS_ENABLE);
            LONG_PTR tagsCheckStyle = GetWindowLongPtr(tagsCheck, GWL_STYLE);
            tagsCheckStyle &= ~BS_TYPEMASK;
            tagsCheckStyle |= IsMultiple() ? BS_AUTO3STATE : BS_AUTOCHECKBOX;
            SetWindowLongPtr(tagsCheck, GWL_STYLE, tagsCheckStyle);
            SetDlgItemText(HWindow, IDC_EDPROP_TAGS_ENABLE, LoadStr(IDS_EDPROP_CHANGE_TAGS));
            SetDlgItemText(HWindow, IDC_EDPROP_TAGS_HEADER, LoadStr(IDS_FFA_TAGS));
            SetDlgItemText(HWindow, IDC_EDPROP_PROPERTIES_GROUP, LoadStr(IDS_EDPROP_OTHER_PROPERTIES));
            SetDlgItemText(HWindow, IDC_EDPROP_MULTI_HINT,
                           IsMultiple() ? LoadStr(IDS_EDPROP_MULTI_FORM_HINT) : "");
            TagsWritable = IsPropertyWritableForAll(PKEY_Keywords);
#ifdef new
#undef new
#define RESTORE_TAGS_EDIT_LIST_DEBUG_NEW_MACRO
#endif
            TagsList = new (std::nothrow) CEditListBox(HWindow, IDC_EDPROP_TAGS_LIST,
                                                       ELB_ENABLECOMMANDS | ELB_RIGHTARROW);
#ifdef RESTORE_TAGS_EDIT_LIST_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_TAGS_EDIT_LIST_DEBUG_NEW_MACRO
#endif
            if (TagsList != NULL)
            {
                TagsList->MakeHeader(IDC_EDPROP_TAGS_HEADER);
                AddInitialTags();
            }
            if (!TagsWritable)
                SendMessage(tagsCheck, BM_SETCHECK, BST_UNCHECKED, 0);
            EnableWindow(tagsCheck, TagsWritable);
            AddPropertyRows();
            UpdateEnabledState();
            if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
            {
                DarkModeApplyTree(HWindow);
                DarkModeRefreshTitleBar(HWindow);
                DarkModeApplyStaticTextColors(HWindow, NULL);
            }
            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_EDPROP_TAGS_ENABLE && HIWORD(wParam) == BN_CLICKED)
            {
                UpdateEnabledState();
                return TRUE;
            }
            if (LOWORD(wParam) >= EDPROP_DYNAMIC_CHECK_FIRST &&
                LOWORD(wParam) < EDPROP_DYNAMIC_CHECK_FIRST + (int)PropertyRows.size() &&
                HIWORD(wParam) == BN_CLICKED)
            {
                UpdateEnabledState();
                return TRUE;
            }
            if (LOWORD(wParam) == IDOK)
            {
                if (Apply())
                    EndDialog(HWindow, IDOK);
                return TRUE;
            }
            break;

        case WM_VSCROLL:
            if ((HWND)lParam == GetDlgItem(HWindow, IDC_EDPROP_PROPERTIES_SCROLL))
            {
                SCROLLINFO info;
                ZeroMemory(&info, sizeof(info));
                info.cbSize = sizeof(info);
                info.fMask = SIF_ALL;
                GetScrollInfo((HWND)lParam, SB_CTL, &info);
                int position = info.nPos;
                switch (LOWORD(wParam))
                {
                case SB_LINEUP: position--; break;
                case SB_LINEDOWN: position++; break;
                case SB_PAGEUP: position -= info.nPage; break;
                case SB_PAGEDOWN: position += info.nPage; break;
                case SB_THUMBPOSITION:
                case SB_THUMBTRACK: position = info.nTrackPos; break;
                }
                PropertyScrollPos = max(info.nMin,
                                        min(position, info.nMax - (int)info.nPage + 1));
                LayoutPropertyRows();
                return TRUE;
            }
            break;

        case WM_DRAWITEM:
            if (wParam == IDC_EDPROP_TAGS_LIST && TagsList != NULL)
            {
                TagsList->OnDrawItem(lParam);
                return TRUE;
            }
            break;

        case WM_NOTIFY:
        {
            NMHDR* header = (NMHDR*)lParam;
            if (header->idFrom == IDC_EDPROP_TAGS_LIST && TagsList != NULL)
            {
                EDTLB_DISPINFO* info = (EDTLB_DISPINFO*)lParam;
                switch (header->code)
                {
                case EDTLBN_GETDISPINFO:
                    if (info->ToDo == edtlbGetData)
                    {
                        CTagItem* item = (CTagItem*)info->ItemID;
                        if (item != NULL && item != (CTagItem*)-1)
                            lstrcpyn(info->Buffer, item->Text.c_str(), info->BufferLen);
                        SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);
                    }
                    else
                    {
                        std::string text = info->Buffer;
                        TrimTag(text);
                        CTagItem* item = info->ItemID == -1 ? NULL : (CTagItem*)info->ItemID;
                        if (text.empty() || HasTag(text, item))
                        {
                            SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);
                            return TRUE;
                        }
                        if (item == NULL)
                        {
#ifdef new
#undef new
#define RESTORE_NEW_TAG_ITEM_DEBUG_NEW_MACRO
#endif
                            item = new (std::nothrow) CTagItem;
#ifdef RESTORE_NEW_TAG_ITEM_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_NEW_TAG_ITEM_DEBUG_NEW_MACRO
#endif
                            if (item == NULL)
                            {
                                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);
                                return TRUE;
                            }
                            item->Text = text;
                            Tags.push_back(item);
                            TagsList->SetItemData((INT_PTR)item);
                        }
                        else
                            item->Text = text;
                        InvalidateRect(TagsList->HWindow, NULL, FALSE);
                        SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
                    }
                    return TRUE;

                case EDTLBN_DELETEITEM:
                {
                    CTagItem* item = (CTagItem*)info->ItemID;
                    for (std::vector<CTagItem*>::iterator it = Tags.begin(); it != Tags.end(); ++it)
                        if (*it == item)
                        {
                            delete item;
                            Tags.erase(it);
                            break;
                        }
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);
                    return TRUE;
                }

                case EDTLBN_MOVEITEM2:
                    if (info->Index >= 0 && info->Index < (int)Tags.size() &&
                        info->NewIndex >= 0 && info->NewIndex < (int)Tags.size())
                    {
                        CTagItem* item = Tags[info->Index];
                        Tags.erase(Tags.begin() + info->Index);
                        Tags.insert(Tags.begin() + info->NewIndex, item);
                        for (int i = 0; i < (int)Tags.size(); i++)
                            TagsList->SetItemID(i, (INT_PTR)Tags[i]);
                    }
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);
                    return TRUE;

                case EDTLBN_ENABLECOMMANDS:
                    info->Enable = TLBHDRMASK_NEW | TLBHDRMASK_SEARCH | TLBHDRMASK_FILTER;
                    // CEditListBox starts both Modify and New through
                    // OnBeginEdit(), which requires the MODIFY enabler. The
                    // trailing item at Tags.size() is its editable placeholder.
                    if (info->Index >= 0 && info->Index <= (int)Tags.size())
                        info->Enable |= TLBHDRMASK_MODIFY;
                    if (info->Index >= 0 && info->Index < (int)Tags.size())
                    {
                        info->Enable |= TLBHDRMASK_DELETE;
                        if (info->Index > 0)
                            info->Enable |= TLBHDRMASK_TOP | TLBHDRMASK_UP;
                        if (info->Index + 1 < (int)Tags.size())
                            info->Enable |= TLBHDRMASK_DOWN | TLBHDRMASK_BOTTOM;
                    }
                    return TRUE;

                case EDTLBN_CONTEXTMENU:
                    ShowRecentTags(info->HEdit, info->Point);
                    return TRUE;
                }
            }
            break;
        }

        case WM_DESTROY:
            TagsList = NULL;
            break;
        }
        return CCommonDialog::DialogProc(uMsg, wParam, lParam);
    }
};

//

static DWORD UpdateArchiveCacheHash(DWORD hash, const char* text, BOOL ignoreCase)
{
    const unsigned char* s = (const unsigned char*)text;
    while (s != NULL && *s != 0)
    {
        unsigned char c = *s++;
        if (ignoreCase && c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        hash ^= c;
        hash *= 16777619U;
    }
    return hash;
}

static void BuildArchiveCacheKey(char* key, int keySize, const char* archiveName, const char* nameInArchive, const char* itemName)
{
    DWORD hash = 2166136261U;
    hash = UpdateArchiveCacheHash(hash, archiveName, TRUE);
    hash = UpdateArchiveCacheHash(hash, "\\", FALSE);
    hash = UpdateArchiveCacheHash(hash, nameInArchive, FALSE);
    _snprintf_s(key, keySize, _TRUNCATE, "ArchiveView:%08X:%s", hash, itemName != NULL ? itemName : "");
}


static std::wstring FileActionTextToWide(const char* text)
{
    std::wstring wide = SalMultiByteToWidePath(text, CP_UTF8);
    if (wide.empty() && GetACP() != CP_UTF8)
        wide = SalMultiByteToWidePath(text, CP_ACP);
    return wide;
}

static BOOL CreateProcessForFileAction(const char* cmdLine, const char* currentDir, STARTUPINFO* si, PROCESS_INFORMATION* pi)
{
    std::wstring cmdLineW = FileActionTextToWide(cmdLine);
    if (cmdLineW.empty())
        return HANDLES(CreateProcess(NULL, (char*)cmdLine, NULL, NULL, FALSE,
                                     NORMAL_PRIORITY_CLASS, NULL, currentDir, si, pi));

    std::wstring currentDirW;
    LPCWSTR currentDirParam = NULL;
    if (currentDir != NULL && *currentDir != 0)
    {
        currentDirW = FileActionTextToWide(currentDir);
        if (!currentDirW.empty())
        {
            if (currentDirW.length() >= MAX_PATH && !SalIsExtendedLengthPathW(currentDirW.c_str()))
                currentDirW = SalPathAddExtendedPrefixW(currentDirW.c_str());
            currentDirParam = currentDirW.c_str();
        }
    }

    STARTUPINFOW siW;
    memset(&siW, 0, sizeof(siW));
    siW.cb = sizeof(siW);
    siW.dwX = si->dwX;
    siW.dwY = si->dwY;
    siW.dwXSize = si->dwXSize;
    siW.dwYSize = si->dwYSize;
    siW.dwXCountChars = si->dwXCountChars;
    siW.dwYCountChars = si->dwYCountChars;
    siW.dwFillAttribute = si->dwFillAttribute;
    siW.dwFlags = si->dwFlags;
    siW.wShowWindow = si->wShowWindow;
    siW.cbReserved2 = si->cbReserved2;
    siW.lpReserved2 = si->lpReserved2;
    siW.hStdInput = si->hStdInput;
    siW.hStdOutput = si->hStdOutput;
    siW.hStdError = si->hStdError;

    BOOL created = NOHANDLES(CreateProcessW(NULL, &cmdLineW[0], NULL, NULL, FALSE,
                                            NORMAL_PRIORITY_CLASS, NULL, currentDirParam, &siW, pi));
    if (created)
    {
        HANDLES_ADD(__htProcess, __hoCreateProcess, pi->hProcess);
        HANDLES_ADD(__htThread, __hoCreateProcess, pi->hThread);
    }
    return created;
}

// ****************************************************************************
// CFilesWindow
//

void CFilesWindow::Convert()
{
    CALL_STACK_MESSAGE1("CFilesWindow::Convert()");
    if (Dirs->Count + Files->Count == 0)
        return;
    BeginStopRefresh(); // snooper takes a break

    if (!FilesActionInProgress)
    {
        FilesActionInProgress = TRUE;

        BOOL subDir;
        if (Dirs->Count > 0)
            subDir = (strcmp(Dirs->At(0).Name, "..") == 0);
        else
            subDir = FALSE;

        CConvertFilesDlg convertDlg(HWindow, SelectionContainsDirectory());
        while (1)
        {
            if (convertDlg.Execute() == IDOK)
            {
                UpdateWindow(MainWindow->HWindow);
                if (convertDlg.CodeType == 0 && convertDlg.EOFType == 0)
                    break; // nothing to do

                CCriteriaData filter;
                filter.UseMasks = TRUE;
                filter.Masks.SetMasksString(convertDlg.Mask);
                int errpos = 0;
                if (!filter.Masks.PrepareMasks(errpos))
                    break; // invalid mask

                if (CheckPath(TRUE) != ERROR_SUCCESS) // the path we need to work on failed
                {
                    FilesActionInProgress = FALSE;
                    EndStopRefresh(); // snooper will start again now
                    return;
                }

                CConvertData dlgData;

                dlgData.EOFType = convertDlg.EOFType;

                // the CodeTables object was initialized in the Convert dialog
                if (!CodeTables.GetCode(dlgData.CodeTable, convertDlg.CodeType))
                {
                    // if we fail to obtain the encoding table or no encoding is selected,
                    // perform one-to-one encoding, i.e. no conversion
                    int i;
                    for (i = 0; i < 256; i++)
                        dlgData.CodeTable[i] = i;
                }

                //---  determine which directories and files are selected
                int* indexes = NULL;
                CFileData* f = NULL;
                int count = GetSelCount();
                if (count != 0)
                {
                    indexes = new int[count];
                    if (indexes == NULL)
                    {
                        TRACE_E(LOW_MEMORY);
                        FilesActionInProgress = FALSE;
                        EndStopRefresh(); // snooper will start again now
                        return;
                    }
                    GetSelItems(count, indexes);
                }
                else // nothing is selected
                {
                    int i = GetCaretIndex();
                    if (i < 0 || i >= Dirs->Count + Files->Count)
                    {
                        FilesActionInProgress = FALSE;
                        EndStopRefresh(); // snooper will start again now
                        return;           // invalid index (no files)
                    }
                    if (i == 0 && subDir)
                    {
                        FilesActionInProgress = FALSE;
                        EndStopRefresh(); // snooper will start again now
                        return;           // we do not work with ".."
                    }
                    f = (i < Dirs->Count) ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
                }
                //---
                COperations* script = new COperations(1000, 500, NULL, NULL, NULL);
                if (script == NULL)
                    TRACE_E(LOW_MEMORY);
                else
                {
                    HWND hFocusedWnd = GetFocus();
                    CreateSafeWaitWindow(LoadStr(IDS_ANALYSINGDIRTREEESC), NULL, 1000, TRUE, MainWindow->HWindow);
                    EnableWindow(MainWindow->HWindow, FALSE);

                    HCURSOR oldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

                    BOOL res = BuildScriptMain(script, convertDlg.SubDirs ? atRecursiveConvert : atConvert,
                                               NULL, NULL, count, indexes, f, NULL, NULL, FALSE, &filter);
                    if (script->Count == 0)
                        res = FALSE;
                    // reordered to allow the main window to activate (must not be disabled), otherwise it switches to another app
                    EnableWindow(MainWindow->HWindow, TRUE);
                    DestroySafeWaitWindow();

                    // if Salamander is active, call SetFocus on the stored window (SetFocus fails
                    // if the main window is disabled - after deactivation/activation of the disabled main window the active panel
                    // does not have focus)
                    HWND hwnd = GetForegroundWindow();
                    while (hwnd != NULL && hwnd != MainWindow->HWindow)
                        hwnd = GetParent(hwnd);
                    if (hwnd == MainWindow->HWindow)
                        SetFocus(hFocusedWnd);

                    SetCursor(oldCur);

                    // prepare refresh of manually refreshed directories
                    // change in the directory displayed in the panel and also in subdirectories if work was done there as well
                    script->SetWorkPath1(GetPath(), convertDlg.SubDirs);

                    if (!res || !StartProgressDialog(script, LoadStr(IDS_CONVERTTITLE), NULL, &dlgData))
                    {
                        if (script->IsGood() && script->Count == 0)
                        {
                            SalMessageBox(HWindow, LoadStr(IDS_NOFILE_MATCHEDMASK), LoadStr(IDS_INFOTITLE),
                                          MB_OK | MB_ICONINFORMATION);

                            // none of the files had to pass through the mask filter
                            SetSel(FALSE, -1, TRUE);                        // explicit repaint
                            PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0); // selection change notify
                        }
                        UpdateWindow(MainWindow->HWindow);
                        if (!script->IsGood())
                            script->ResetState();
                        FreeScript(script);
                    }
                    else // removal of selection index (no waiting for operation finish, operation runs in another thread)
                    {
                        SetSel(FALSE, -1, TRUE);                        // explicit repaint
                        PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0); // selection change notify
                        UpdateWindow(MainWindow->HWindow);
                    }
                }
                //---
                if (indexes != NULL)
                    delete[] (indexes);
            }
            UpdateWindow(MainWindow->HWindow);
            break;
        }
        FilesActionInProgress = FALSE;
    }
    EndStopRefresh(); // snooper will start again now
}

void CFilesWindow::EditWindowsProperties()
{
    CALL_STACK_MESSAGE1("CFilesWindow::EditWindowsProperties()");
    if (!Is(ptDisk) || CheckPath(TRUE) != ERROR_SUCCESS)
        return;

    std::vector<int> indexes;
    int selected = GetSelCount();
    if (selected > 0)
    {
        indexes.resize(selected);
        GetSelItems(selected, indexes.data());
    }
    else
        indexes.push_back(GetCaretIndex());

    std::vector<std::wstring> paths;
    std::wstring panelPath = GetPathW() != NULL && GetPathW()[0] != 0
                                 ? std::wstring(GetPathW())
                                 : SalMultiByteToWidePath(GetPath());
    for (size_t i = 0; i < indexes.size(); i++)
    {
        int index = indexes[i];
        if (index < Dirs->Count || index >= Dirs->Count + Files->Count)
            continue;
        CFileData* file = &Files->At(index - Dirs->Count);
        std::wstring path = panelPath;
        std::wstring name = file->UseWideName() ? std::wstring(file->NameW) : SalMultiByteToWidePath(file->Name);
        SalPathAppendW(path, name.c_str());
        paths.push_back(path);
    }
    if (paths.empty())
        return;

    CEditWindowsPropertiesDialog dialog(HWindow, paths);
    if (dialog.Execute() == IDOK)
    {
        RefreshListBox(-1, -1, FocusedIndex, FALSE, FALSE);
        PostMessage(HWindow, WM_USER_REFRESH_DIR, 0, GetTickCount());
    }
}

void CFilesWindow::ChangeAttr(BOOL setCompress, BOOL compressed, BOOL setEncryption, BOOL encrypted)
{
    CALL_STACK_MESSAGE5("CFilesWindow::ChangeAttr(%d, %d, %d, %d)", setCompress, compressed, setEncryption, encrypted);
    if (Dirs->Count + Files->Count == 0)
        return;
    int selectedCount = GetSelCount();
    if (selectedCount == 0 || selectedCount == 1)
    {
        int index;
        if (selectedCount == 0)
            index = GetCaretIndex();
        else
            GetSelItems(1, &index);
        // focus is on UpDir -- nothing to convert
        if (Dirs->Count > 0 && index == 0 && strcmp(Dirs->At(0).Name, "..") == 0)
            return;
    }
    BeginStopRefresh(); // snooper takes a break

    // if no item is selected, select the one under focus and store its name
    char temporarySelected[MAX_PATH];
    temporarySelected[0] = 0;
    if ((!setCompress || Configuration.CnfrmNTFSPress) &&
        (!setEncryption || Configuration.CnfrmNTFSCrypt))
    {
        SelectFocusedItemAndGetName(temporarySelected, MAX_PATH);
    }

    if (Is(ptDisk))
    {
        if (!FilesActionInProgress)
        {
            FilesActionInProgress = TRUE;

            BOOL subDir;
            if (Dirs->Count > 0)
                subDir = (strcmp(Dirs->At(0).Name, "..") == 0);
            else
                subDir = FALSE;

            DWORD attr, attrDiff;
            SYSTEMTIME timeModified;
            SYSTEMTIME timeCreated;
            SYSTEMTIME timeAccessed;
            if (!setCompress && !setEncryption)
            {
                int count = GetSelCount();
                if (count == 1 || count == 0)
                {
                    int index;
                    if (count == 0)
                        index = GetCaretIndex();
                    else
                        GetSelItems(1, &index);
                    if (index >= 0 && index < Files->Count + Dirs->Count)
                    {
                        CFileData* f = (index < Dirs->Count) ? &Dirs->At(index) : &Files->At(index - Dirs->Count);
                        if (strcmp(f->Name, "..") != 0)
                        {
                            BOOL isDir = index < Dirs->Count;

                            BOOL timeObtained = FALSE;

                            // retrieve the file times
                            char fileName[MAX_PATH];
                            strcpy(fileName, GetPath());
                            SalPathAppend(fileName, f->Name, MAX_PATH);

                            WIN32_FIND_DATA find;
                            HANDLE hFind = HANDLES_Q(FindFirstFile(fileName, &find));
                            if (hFind != INVALID_HANDLE_VALUE)
                            {
                                HANDLES(FindClose(hFind));

                                FILETIME ft;
                                if (FileTimeToLocalFileTime(&find.ftCreationTime, &ft) &&
                                    FileTimeToSystemTime(&ft, &timeCreated) &&
                                    FileTimeToLocalFileTime(&find.ftLastAccessTime, &ft) &&
                                    FileTimeToSystemTime(&ft, &timeAccessed) &&
                                    FileTimeToLocalFileTime(&find.ftLastWriteTime, &ft) &&
                                    FileTimeToSystemTime(&ft, &timeModified))
                                {
                                    timeObtained = TRUE;
                                }
                            }
                            if (!timeObtained)
                            {
                                // if we failed to obtain the time from the file, use at least the one we have
                                FILETIME ft;
                                if (!FileTimeToLocalFileTime(&f->LastWrite, &ft) ||
                                    !FileTimeToSystemTime(&ft, &timeModified))
                                {
                                    GetLocalTime(&timeModified); // the time we have is invalid, use the current time
                                }
                                timeCreated = timeModified;
                                timeAccessed = timeModified;
                            }

                            attr = f->Attr;
                            attrDiff = 0;
                            count = -1;
                        }
                    }
                }
                if (count != -1)
                {
                    GetLocalTime(&timeModified);
                    timeAccessed = timeModified;
                    timeCreated = timeModified;
                    attr = 0;
                    attrDiff = 0;
                    BOOL first = TRUE;

                    int totalCount = Dirs->Count + Files->Count;
                    CFileData* f;
                    int i;
                    for (i = 0; i < totalCount; i++)
                    {
                        BOOL isDir = i < Dirs->Count;
                        f = isDir ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
                        if (i == 0 && isDir && strcmp(Dirs->At(0).Name, "..") == 0)
                            continue;
                        if (f->Selected == 1)
                        {
                            if (first)
                            {
                                attr = f->Attr;
                                first = FALSE;
                            }
                            else
                            {
                                if (f->Attr != attr)
                                    attrDiff |= f->Attr ^ attr;
                            }
                        }
                    }
                }
            }

            CChangeAttrDialog chDlg(HWindow, attr, attrDiff,
                                    SelectionContainsDirectory(), FileBasedCompression,
                                    FileBasedEncryption,
                                    &timeModified, &timeCreated, &timeAccessed);
            if (setCompress || setEncryption)
            {
                chDlg.Archive = 2;
                chDlg.ReadOnly = 2;
                chDlg.Hidden = 2;
                chDlg.System = 2;
                if (setCompress)
                {
                    chDlg.Compressed = compressed;
                    chDlg.Encrypted = compressed ? 0 : 2; // compression excludes encryption; without compression encryption may remain as is
                }
                else
                {
                    chDlg.Compressed = encrypted ? 0 : 2; // encryption excludes compression; without encryption compression may remain as is
                    chDlg.Encrypted = encrypted;
                }
                chDlg.ChangeTimeModified = FALSE;
                chDlg.ChangeTimeCreated = FALSE;
                chDlg.ChangeTimeAccessed = FALSE;
                chDlg.RecurseSubDirs = TRUE;

                if (setCompress && Configuration.CnfrmNTFSPress || // ask whether to compress/decompress
                    setEncryption && Configuration.CnfrmNTFSCrypt) // ask whether to encrypt/decrypt
                {
                    char subject[MAX_PATH + 100];
                    char expanded[200];
                    int count = GetSelCount();
                    char path[MAX_PATH];
                    if (count > 1)
                    {
                        int totalCount = Dirs->Count + Files->Count;
                        int files = 0;
                        int dirs = 0;
                        CFileData* f;
                        int i;
                        for (i = 0; i < totalCount; i++)
                        {
                            BOOL isDir = i < Dirs->Count;
                            f = isDir ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
                            if (i == 0 && isDir && strcmp(Dirs->At(0).Name, "..") == 0)
                                continue;
                            if (f->Selected == 1)
                            {
                                if (isDir)
                                    dirs++;
                                else
                                    files++;
                            }
                        }

                        ExpandPluralFilesDirs(expanded, 200, files, dirs, epfdmNormal, FALSE);
                    }
                    else
                    {
                        int index;
                        if (count == 0)
                            index = GetCaretIndex();
                        else
                            GetSelItems(1, &index);

                        BOOL isDir = index < Dirs->Count;
                        CFileData* f = isDir ? &Dirs->At(index) : &Files->At(index - Dirs->Count);
                        AlterFileName(path, f->Name, -1, Configuration.FileNameFormat, 0, index < Dirs->Count);
                        lstrcpy(expanded, LoadStr(isDir ? IDS_QUESTION_DIRECTORY : IDS_QUESTION_FILE));
                    }
                    int resTextID;
                    int resTitleID;
                    if (setCompress)
                    {
                        resTextID = compressed ? IDS_CONFIRM_NTFSCOMPRESS : IDS_CONFIRM_NTFSUNCOMPRESS;
                        resTitleID = compressed ? IDS_CONFIRM_NTFSCOMPRESS_TITLE : IDS_CONFIRM_NTFSUNCOMPRESS_TITLE;
                    }
                    else
                    {
                        resTextID = encrypted ? IDS_CONFIRM_NTFSENCRYPT : IDS_CONFIRM_NTFSDECRYPT;
                        resTitleID = encrypted ? IDS_CONFIRM_NTFSENCRYPT_TITLE : IDS_CONFIRM_NTFSDECRYPT_TITLE;
                    }
                    _snprintf_s(subject, _TRUNCATE, "%s %s", LoadStr(resTextID), expanded);
                    CTruncatedString str;
                    str.Set(subject, count > 1 ? NULL : path);
                    CMessageBox msgBox(HWindow, MSGBOXEX_YESNO | MSGBOXEX_ESCAPEENABLED | MSGBOXEX_ICONQUESTION | MSGBOXEX_SILENT,
                                       LoadStr(resTitleID), &str, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL);
                    if (msgBox.Execute() != IDYES)
                    {
                        // if we selected an item, deselect it again
                        UnselectItemWithName(temporarySelected);
                        FilesActionInProgress = FALSE;
                        EndStopRefresh(); // snooper will start again now
                        return;
                    }
                    UpdateWindow(MainWindow->HWindow);
                }
            }
            if (setCompress || setEncryption || chDlg.Execute() == IDOK)
            {
                UpdateWindow(MainWindow->HWindow);

                if (CheckPath(TRUE) != ERROR_SUCCESS)
                {
                    // if we selected an item, we deselect it again
                    UnselectItemWithName(temporarySelected);
                    FilesActionInProgress = FALSE;
                    EndStopRefresh(); // snooper will start again now
                    return;
                }

                CChangeAttrsData dlgData;
                dlgData.ChangeTimeModified = chDlg.ChangeTimeModified;
                if (dlgData.ChangeTimeModified)
                {
                    FILETIME ft;
                    SystemTimeToFileTime(&chDlg.TimeModified, &ft);
                    LocalFileTimeToFileTime(&ft, &dlgData.TimeModified);
                }
                dlgData.ChangeTimeCreated = chDlg.ChangeTimeCreated;
                if (dlgData.ChangeTimeCreated)
                {
                    FILETIME ft;
                    SystemTimeToFileTime(&chDlg.TimeCreated, &ft);
                    LocalFileTimeToFileTime(&ft, &dlgData.TimeCreated);
                }
                dlgData.ChangeTimeAccessed = chDlg.ChangeTimeAccessed;
                if (dlgData.ChangeTimeAccessed)
                {
                    FILETIME ft;
                    SystemTimeToFileTime(&chDlg.TimeAccessed, &ft);
                    LocalFileTimeToFileTime(&ft, &dlgData.TimeAccessed);
                }
                //---  determine which directories and files are selected
                int* indexes = NULL;
                CFileData* f = NULL;
                int count = GetSelCount();
                if (count != 0)
                {
                    indexes = new int[count];
                    if (indexes == NULL)
                    {
                        TRACE_E(LOW_MEMORY);
                        // if we selected an item, we deselect it again
                        UnselectItemWithName(temporarySelected);
                        FilesActionInProgress = FALSE;
                        EndStopRefresh(); // snooper will start again now
                        return;
                    }
                    GetSelItems(count, indexes);
                }
                else // nothing is selected
                {
                    int i = GetCaretIndex();
                    if (i < 0 || i >= Dirs->Count + Files->Count)
                    {
                        // if we selected an item, we deselect it again
                        UnselectItemWithName(temporarySelected);
                        FilesActionInProgress = FALSE;
                        EndStopRefresh(); // snooper will start again now
                        return;           // invalid index (no files)
                    }
                    if (i == 0 && subDir)
                    {
                        // if we selected an item, we deselect it again
                        UnselectItemWithName(temporarySelected);
                        FilesActionInProgress = FALSE;
                        EndStopRefresh(); // snooper will start again now
                        return;           // we do not work with ".."
                    }
                    f = (i < Dirs->Count) ? &Dirs->At(i) : &Files->At(i - Dirs->Count);
                }
                //---
                COperations* script = new COperations(1000, 500, NULL, NULL, NULL);
                if (script == NULL)
                    TRACE_E(LOW_MEMORY);
                else
                {
                    HWND hFocusedWnd = GetFocus();
                    CreateSafeWaitWindow(LoadStr(IDS_ANALYSINGDIRTREEESC), NULL, 1000, TRUE, MainWindow->HWindow);
                    EnableWindow(MainWindow->HWindow, FALSE);

                    HCURSOR oldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

                    // ensure a correct relationship between Compressed and Encrypted
                    if (chDlg.Encrypted == 1)
                    {
                        if (chDlg.Compressed != 0)
                            TRACE_E("CFilesWindow::ChangeAttr(): unexpected value of chDlg.Compressed!");
                        chDlg.Compressed = 0;
                    }
                    else
                    {
                        if (chDlg.Compressed == 1)
                        {
                            if (chDlg.Encrypted != 0)
                                TRACE_E("CFilesWindow::ChangeAttr(): unexpected value of chDlg.Encrypted!");
                            chDlg.Encrypted = 0;
                        }
                    }

                    CAttrsData attrsData;
                    attrsData.AttrAnd = 0xFFFFFFFF;
                    attrsData.AttrOr = 0;
                    attrsData.SubDirs = chDlg.RecurseSubDirs;
                    attrsData.ChangeCompression = FALSE;
                    attrsData.ChangeEncryption = FALSE;
                    dlgData.ChangeCompression = FALSE;
                    dlgData.ChangeEncryption = FALSE;

                    if (chDlg.Archive == 0)
                        attrsData.AttrAnd &= ~(FILE_ATTRIBUTE_ARCHIVE);
                    if (chDlg.ReadOnly == 0)
                        attrsData.AttrAnd &= ~(FILE_ATTRIBUTE_READONLY);
                    if (chDlg.Hidden == 0)
                        attrsData.AttrAnd &= ~(FILE_ATTRIBUTE_HIDDEN);
                    if (chDlg.System == 0)
                        attrsData.AttrAnd &= ~(FILE_ATTRIBUTE_SYSTEM);
                    if (chDlg.Compressed == 0)
                    {
                        attrsData.AttrAnd &= ~(FILE_ATTRIBUTE_COMPRESSED);
                        attrsData.ChangeCompression = TRUE;
                        dlgData.ChangeCompression = TRUE;
                    }
                    if (chDlg.Encrypted == 0)
                    {
                        attrsData.AttrAnd &= ~(FILE_ATTRIBUTE_ENCRYPTED);
                        attrsData.ChangeEncryption = TRUE;
                        dlgData.ChangeEncryption = TRUE;
                    }

                    if (chDlg.Archive == 1)
                        attrsData.AttrOr |= FILE_ATTRIBUTE_ARCHIVE;
                    if (chDlg.ReadOnly == 1)
                        attrsData.AttrOr |= FILE_ATTRIBUTE_READONLY;
                    if (chDlg.Hidden == 1)
                        attrsData.AttrOr |= FILE_ATTRIBUTE_HIDDEN;
                    if (chDlg.System == 1)
                        attrsData.AttrOr |= FILE_ATTRIBUTE_SYSTEM;
                    if (chDlg.Compressed == 1)
                    {
                        attrsData.AttrOr |= FILE_ATTRIBUTE_COMPRESSED;
                        attrsData.ChangeCompression = TRUE;
                        dlgData.ChangeCompression = TRUE;
                    }
                    if (chDlg.Encrypted == 1)
                    {
                        attrsData.AttrOr |= FILE_ATTRIBUTE_ENCRYPTED;
                        attrsData.ChangeEncryption = TRUE;
                        dlgData.ChangeEncryption = TRUE;
                    }

                    script->ClearReadonlyMask = 0xFFFFFFFF;
                    BOOL res = BuildScriptMain(script, atChangeAttrs, NULL, NULL, count,
                                               indexes, f, &attrsData, NULL, FALSE, NULL);
                    if (script->Count == 0)
                        res = FALSE;
                    // reordered to allow the main window to activate (must not be disabled), otherwise it switches to another app
                    EnableWindow(MainWindow->HWindow, TRUE);
                    DestroySafeWaitWindow();

                    // if Salamander is active, call SetFocus on the stored window (SetFocus fails
                    // if the main window is disabled - after deactivation/activation of the disabled main window the active panel
                    // does not have focus)
                    HWND hwnd = GetForegroundWindow();
                    while (hwnd != NULL && hwnd != MainWindow->HWindow)
                        hwnd = GetParent(hwnd);
                    if (hwnd == MainWindow->HWindow)
                        SetFocus(hFocusedWnd);

                    SetCursor(oldCur);

                    // prepare refresh of manually refreshed directories
                    // change in the directory displayed in the panel and also in subdirectories if work was done there as well
                    script->SetWorkPath1(GetPath(), chDlg.RecurseSubDirs);

                    if (!res || !StartProgressDialog(script, LoadStr(IDS_CHANGEATTRSTITLE), &dlgData, NULL))
                    {
                        UpdateWindow(MainWindow->HWindow);
                        if (!script->IsGood())
                            script->ResetState();
                        FreeScript(script);
                    }
                    else // removal of selection index (no waiting for operation finish, operation runs in another thread)
                    {
                        SetSel(FALSE, -1, TRUE);                        // explicit repaint
                        PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0); // selection change notify
                        UpdateWindow(MainWindow->HWindow);
                    }
                }
                //---
                if (indexes != NULL)
                    delete[] (indexes);
                //---  if a Salamander window is active, end suspend mode
            }
            UpdateWindow(MainWindow->HWindow);
            FilesActionInProgress = FALSE;
        }
    }
    else
    {
        if (Is(ptPluginFS) && GetPluginFS()->NotEmpty() &&
            GetPluginFS()->IsServiceSupported(FS_SERVICE_CHANGEATTRS)) // FS is in the panel
        {
            // lower the thread priority to "normal" (so the operations don't overload the machine)
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

            int panel = IsLeftPanel() ? PANEL_LEFT : PANEL_RIGHT;

            int count = GetSelCount();
            int selectedDirs = 0;
            if (count > 0)
            {
                // count how many directories are selected (the rest of the selected items are files)
                int i;
                for (i = 0; i < Dirs->Count; i++) // ".." cannot be selected, the check would be unnecessary
                {
                    if (Dirs->At(i).Selected)
                        selectedDirs++;
                }
            }
            else
                count = 0;

            BOOL success = GetPluginFS()->ChangeAttributes(GetPluginFS()->GetPluginFSName(), HWindow,
                                                           panel, count - selectedDirs, selectedDirs);

            // raise the thread priority again, the operation has finished
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

            if (success) // success -> unselect
            {
                SetSel(FALSE, -1, TRUE);                        // explicit repaint
                PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0); // selection change notify
                UpdateWindow(MainWindow->HWindow);
            }
        }
    }
    // if we selected an item, we deselect it again
    UnselectItemWithName(temporarySelected);

    EndStopRefresh(); // snooper will start again now
}

void CFilesWindow::FindFile()
{
    CALL_STACK_MESSAGE1("CFilesWindow::FindFile()");
    if (Is(ptDisk) && CheckPath(TRUE) != ERROR_SUCCESS)
        return;

    if (Is(ptPluginFS) && GetPluginFS()->NotEmpty() &&
        GetPluginFS()->IsServiceSupported(FS_SERVICE_OPENFINDDLG))
    { // try to open Find for the FS in the panel; if it succeeds, there is no point in opening the standard Find dialog
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
        BOOL done = GetPluginFS()->OpenFindDialog(GetPluginFS()->GetPluginFSName(),
                                                  IsLeftPanel() ? PANEL_LEFT : PANEL_RIGHT);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
        if (done)
            return;
    }

    if (SystemPolicies.GetNoFind() || SystemPolicies.GetNoShellSearchButton())
    {
        MSGBOXEX_PARAMS params;
        memset(&params, 0, sizeof(params));
        params.HParent = HWindow;
        params.Flags = MSGBOXEX_OK | MSGBOXEX_HELP | MSGBOXEX_ICONEXCLAMATION;
        params.Caption = LoadStr(IDS_POLICIESRESTRICTION_TITLE);
        params.Text = LoadStr(IDS_POLICIESRESTRICTION);
        params.ContextHelpId = IDH_GROUPPOLICY;
        params.HelpCallback = MessageBoxHelpCallback;
        SalMessageBoxEx(&params);
        return;
    }

    OpenFindDialog(MainWindow->HWindow, Is(ptDisk) ? GetPath() : "");
}

void CFilesWindow::ViewFile(char* name, BOOL altView, DWORD handlerID, int enumFileNamesSourceUID,
                            int enumFileNamesLastFileIndex)
{
    CALL_STACK_MESSAGE6("CFilesWindow::ViewFile(%s, %d, %u, %d, %d)", name, altView, handlerID,
                        enumFileNamesSourceUID, enumFileNamesLastFileIndex);
    // verify that the file is on an accessible path
    char path[SAL_MAX_PATH + 10];
    if (name == NULL) // file from the panel
    {
        if (Is(ptDisk) || Is(ptZIPArchive))
        {
            if (CheckPath(TRUE) != ERROR_SUCCESS)
                return;
        }
    }
    else // file from a Windows path (Find + Alt+F11)
    {
        char* backSlash = strrchr(name, '\\');
        if (backSlash != NULL)
        {
            memcpy(path, name, backSlash - name);
            path[backSlash - name] = 0;
            if (CheckPath(TRUE, path) != ERROR_SUCCESS)
                return;
        }
    }

    BOOL addToHistory = name != NULL;
    // if viewing/editing from the panel, obtain the full long name
    BOOL useDiskCache = FALSE;          // TRUE only for ZIP - uses disk-cache
    BOOL arcCacheCacheCopies = TRUE;    // cache copies in disk-cache unless the archiver plugin requests otherwise
    char dcFileName[3 * SAL_MAX_PATH + 50]; // ZIP: name for disk-cache
    std::string unicodeDiskFileName;        // UTF-8 full path for local files with Unicode/long names
    if (name == NULL)
    {
        int i = GetCaretIndex();
        if (i >= Dirs->Count && i < Dirs->Count + Files->Count)
        {
            CFileData* f = &Files->At(i - Dirs->Count);
            if (Is(ptDisk))
            {
                if (enumFileNamesLastFileIndex == -1)
                    enumFileNamesLastFileIndex = i - Dirs->Count;
                std::wstring wideName = GetPathW() != NULL && GetPathW()[0] != 0 ? std::wstring(GetPathW()) : FileActionTextToWide(GetPath());
                SalPathAppendW(wideName, f->UseWideName() ? f->NameW : FileActionTextToWide(f->Name).c_str());
                unicodeDiskFileName = SalWideToMultiBytePath(wideName.c_str(), CP_UTF8);

                lstrcpyn(path, GetPath(), SAL_MAX_PATH);
                if (GetPath()[strlen(GetPath()) - 1] != '\\')
                    strcat(path, "\\");
                char* s = path + strlen(path);
                if ((s - path) + f->NameLen >= SAL_MAX_PATH)
                {
                    if (!unicodeDiskFileName.empty())
                    {
                        name = (char*)unicodeDiskFileName.c_str();
                    }
                    else if (f->DosName != NULL && strlen(f->DosName) + (s - path) < SAL_MAX_PATH)
                        strcpy(s, f->DosName);
                    else
                    {
                        SalMessageBox(HWindow, LoadStr(IDS_TOOLONGNAME), LoadStr(IDS_ERRORTITLE),
                                      MB_OK | MB_ICONEXCLAMATION);
                        return;
                    }
                }
                else
                {
                    strcpy(s, f->Name);
                    name = path;
                }
                // try whether the file name is valid, otherwise try its DOS name
                // (handles files accessible only through Unicode or DOS names)
                if (name == path && SalGetFileAttributes(path) == 0xffffffff)
                {
                    DWORD err = GetLastError();
                    if (err == ERROR_FILE_NOT_FOUND || err == ERROR_INVALID_NAME)
                    {
                        if (!unicodeDiskFileName.empty())
                        {
                            name = (char*)unicodeDiskFileName.c_str();
                        }
                        else if (f->DosName != NULL && strlen(f->DosName) + (s - path) < SAL_MAX_PATH)
                        {
                            strcpy(s, f->DosName);
                            if (SalGetFileAttributes(path) == 0xffffffff) // still error -> revert to the long name
                            {
                                if ((s - path) + f->NameLen < SAL_MAX_PATH)
                                    strcpy(s, f->Name);
                            }
                        }
                    }
                }
                name = !unicodeDiskFileName.empty() ? (char*)unicodeDiskFileName.c_str() : path;
                addToHistory = TRUE;
            }
            else
            {
                if (Is(ptZIPArchive))
                {
                    useDiskCache = TRUE;
                    char nameInArchive[2 * SAL_MAX_PATH];
                    nameInArchive[0] = 0;
                    if (GetZIPPath()[0] != 0)
                        lstrcpyn(nameInArchive, GetZIPPath(), 2 * SAL_MAX_PATH);
                    SalPathAppend(nameInArchive, f->Name, 2 * SAL_MAX_PATH);
                    BuildArchiveCacheKey(dcFileName, 3 * SAL_MAX_PATH + 50, GetZIPArchive(), nameInArchive, f->Name);

                    // setting disk-cache for the plugin (standard values change only for the plugin)
                    char arcCacheTmpPath[MAX_PATH];
                    arcCacheTmpPath[0] = 0;
                    BOOL arcCacheOwnDelete = FALSE;
                    CPluginInterfaceAbstract* plugin = NULL; // != NULL if the plugin handles its own deletion
                    int format = PackerFormatConfig.PackIsArchive(GetZIPArchive());
                    if (format != 0) // a supported archive was found
                    {
                        format--;
                        int index = PackerFormatConfig.GetUnpackerIndex(format);
                        if (index < 0) // view: is the processing internal (plugin)?
                        {
                            CPluginData* data = Plugins.Get(-index - 1);
                            if (data != NULL)
                            {
                                data->GetCacheInfo(arcCacheTmpPath, &arcCacheOwnDelete, &arcCacheCacheCopies);
                                if (arcCacheOwnDelete)
                                    plugin = data->GetPluginInterface()->GetInterface();
                            }
                        }
                    }

                    // besides itself, compare the file with all the others and look for a case-sensitive identical name;
                    // if it exists, these two files must be distinguished in the disk-cache; I chose
                    // an allocated Name address - in opposite panels with the same archive the disk-cache won't be used,
                    // but given the improbability of this case, this approach is more than sufficient
                    int x;
                    for (x = 0; x < Files->Count; x++)
                    {
                        if (i - Dirs->Count != x)
                        {
                            CFileData* f2 = &Files->At(x);
                            if (strcmp(f2->Name, f->Name) == 0)
                            {
                                _snprintf_s(dcFileName + strlen(dcFileName), (3 * SAL_MAX_PATH + 50) - strlen(dcFileName), _TRUNCATE, ":0x%p", f->Name);
                                break;
                            }
                        }
                    }

                    BOOL exists;
                    int errorCode;
                    char validTmpName[MAX_PATH];
                    validTmpName[0] = 0;
                    if (!SalIsValidFileNameComponent(f->Name))
                    {
                        lstrcpyn(validTmpName, f->Name, MAX_PATH);
                        SalMakeValidFileNameComponent(validTmpName);
                    }
                    name = (char*)DiskCache.GetName(dcFileName,
                                                    validTmpName[0] != 0 ? validTmpName : f->Name,
                                                    &exists, FALSE,
                                                    arcCacheTmpPath[0] != 0 ? arcCacheTmpPath : NULL,
                                                    plugin != NULL, plugin, &errorCode);
                    if (name == NULL)
                    {
                        if (errorCode == DCGNE_TOOLONGNAME)
                        {
                            SalMessageBox(HWindow, LoadStr(IDS_UNPACKTOOLONGNAME),
                                          LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
                        }
                        return;
                    }

                    if (!exists) // we must unpack it
                    {
                        char* backSlash = strrchr(name, '\\');
                        char tmpPath[SAL_MAX_PATH];
                        memcpy(tmpPath, name, backSlash - name);
                        tmpPath[backSlash - name] = 0;
                        BeginStopRefresh(); // snooper takes a break
                        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
                        HCURSOR oldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
                        BOOL renamingNotSupported = FALSE;
                        if (PackUnpackOneFile(this, GetZIPArchive(), PluginData.GetInterface(), nameInArchive, f, tmpPath,
                                              validTmpName[0] == 0 ? NULL : validTmpName,
                                              validTmpName[0] == 0 ? NULL : &renamingNotSupported))
                        {
                            SetCursor(oldCur);
                            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
                            CQuadWord size(0, 0);
                            HANDLE file = HANDLES_Q(CreateFile(name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                               NULL, OPEN_EXISTING, 0, NULL));
                            if (file != INVALID_HANDLE_VALUE)
                            {
                                DWORD err;
                                SalGetFileSize(file, size, err); // ignore errors; file size isn't that important
                                HANDLES(CloseHandle(file));
                            }
                            DiskCache.NamePrepared(dcFileName, size);
                        }
                        else
                        {
                            SetCursor(oldCur);
                            if (renamingNotSupported) // to avoid repeating the same message for many plugins, display it here for all of them
                            {
                                SalMessageBox(HWindow, LoadStr(IDS_UNPACKINVNAMERENUNSUP),
                                              LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
                            }
                            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
                            DiskCache.ReleaseName(dcFileName, FALSE); // not unpacked, nothing to cache
                            EndStopRefresh();                         // snooper will start again now
                            return;
                        }
                        EndStopRefresh(); // snooper will start again now
                    }
                }
                else
                {
                    if (Is(ptPluginFS))
                    {
                        if (GetPluginFS()->NotEmpty() && // FS is fine and supports view-file
                            GetPluginFS()->IsServiceSupported(FS_SERVICE_VIEWFILE))
                        {
                            // lower the thread priority to "normal" (so the operations don't overload the machine)
                            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

                            CSalamanderForViewFileOnFS sal(altView, handlerID);
                            GetPluginFS()->ViewFile(GetPluginFS()->GetPluginFSName(), HWindow, &sal, *f);

                            // raise the thread priority again, the operation has finished
                            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
                        }
                        return; // view on the FS is already done
                    }
                    else
                    {
                        TRACE_E("Incorrect call to CFilesWindow::ViewFile()");
                        return;
                    }
                }
            }
        }
        else
        {
            return;
        }
    }

    HANDLE lock = NULL;
    BOOL lockOwner = FALSE;
    ViewFileInt(HWindow, name, altView, handlerID, useDiskCache, lock, lockOwner, addToHistory,
                enumFileNamesSourceUID, enumFileNamesLastFileIndex);

    if (useDiskCache)
    {
        if (lock != NULL) // ensure association between the viewer and disk-cache
        {
            DiskCache.AssignName(dcFileName, lock, lockOwner, arcCacheCacheCopies ? crtCache : crtDirect);
        }
        else // viewer didn't open or has no "lock" object - try leaving the file in disk-cache
        {
            DiskCache.ReleaseName(dcFileName, arcCacheCacheCopies);
        }
    }
}

BOOL ViewFileInt(HWND parent, const char* name, BOOL altView, DWORD handlerID, BOOL returnLock,
                 HANDLE& lock, BOOL& lockOwner, BOOL addToHistory, int enumFileNamesSourceUID,
                 int enumFileNamesLastFileIndex)
{
    BOOL success = FALSE;
    lock = NULL;
    lockOwner = FALSE;

    // obtain the full DOS name
    CPathBuffer dosName(SAL_MAX_PATH);
    if (GetShortPathName(name, dosName.Data(), dosName.Capacity()) == 0)
    {
        TRACE_E("GetShortPathName() failed");
        dosName.Data()[0] = 0;
    }

    // find the file name and check if it has an extension - needed for masks
    const char* namePart = strrchr(name, '\\');
    if (namePart == NULL)
    {
        TRACE_E("Invalid parameter for ViewFileInt(): " << name);
        return FALSE;
    }
    namePart++;
    const char* tmpExt = strrchr(namePart, '.');
    //if (tmpExt == NULL || tmpExt == namePart) tmpExt = namePart + lstrlen(namePart); // ".cvspass" is not an extension...
    if (tmpExt == NULL)
        tmpExt = namePart + lstrlen(namePart); // ".cvspass" is treated as an extension in Windows...
    else
        tmpExt++;

    // position for viewers
    WINDOWPLACEMENT place;
    place.length = sizeof(WINDOWPLACEMENT);
    HWND hPlacementWindow = MainWindow->GetDetachedAwareDialogParent(MainWindow->HWindow);
    GetWindowPlacement(hPlacementWindow, &place);
    // GetWindowPlacement respects the Taskbar, so if the Taskbar is at the top or left,
    // the values are shifted by its dimensions. Perform correction.
    RECT monitorRect;
    RECT workRect;
    MultiMonGetClipRectByRect(&place.rcNormalPosition, &workRect, &monitorRect);
    OffsetRect(&place.rcNormalPosition, workRect.left - monitorRect.left,
               workRect.top - monitorRect.top);

    // if called, for example, from find and the main window is minimized,
    // we do not want a minimized viewer
    if (place.showCmd == SW_MINIMIZE || place.showCmd == SW_SHOWMINIMIZED ||
        place.showCmd == SW_SHOWMINNOACTIVE)
        place.showCmd = SW_SHOWNORMAL;

    // find the correct viewer and launch it
    CViewerMasks* masks = (altView ? MainWindow->AltViewerMasks : MainWindow->ViewerMasks);
    CViewerMasksItem* viewer = NULL;

    if (handlerID != 0xFFFFFFFF)
    {
        // attempt to find a viewer with matching HandlerID
        int j;
        for (j = 0; viewer == NULL && j < 2; j++)
        {
            CViewerMasks* masks2 = (j == 0 ? MainWindow->ViewerMasks : MainWindow->AltViewerMasks);
            int i;
            for (i = 0; viewer == NULL && i < masks2->Count; i++)
            {
                if (masks2->At(i)->HandlerID == handlerID)
                    viewer = masks2->At(i);
            }
        }
    }

    if (viewer == NULL)
    {
        int i;
        for (i = 0; i < masks->Count; i++)
        {
            int err;
            if (masks->At(i)->Masks->PrepareMasks(err))
            {
                if (masks->At(i)->Masks->AgreeMasks(namePart, tmpExt))
                {
                    viewer = masks->At(i);

                    if (viewer != NULL && viewer->ViewerType != VIEWER_EXTERNAL &&
                        viewer->ViewerType != VIEWER_INTERNAL)
                    { // plug-in viewers only
                        CPluginData* plugin = Plugins.Get(-viewer->ViewerType - 1);
                        if (plugin != NULL && plugin->SupportViewer)
                        {
                            if (!plugin->CanViewFile(name))
                                continue; // try to find another viewer, this one won't do it
                        }
                        else
                            TRACE_E("Unexpected error (before CanViewFile) in (Alt)ViewerMasks (invalid ViewerType).");
                    }
                    break; // everything is fine, open the viewer
                }
            }
            else
                TRACE_E("Unexpected error in group mask.");
        }
    }

    if (viewer != NULL)
    {
        //    if (MakeFileAvailOfflineIfOneDriveOnWin81(parent, name))
        //    {
        if (addToHistory)
            MainWindow->FileHistory->AddFile(fhitView, viewer->HandlerID, name); // add file to history

        switch (viewer->ViewerType)
        {
        case VIEWER_EXTERNAL:
        {
            CPathBuffer expCommand(SAL_MAX_PATH);
            CPathBuffer expArguments(SAL_MAX_PATH);
            CPathBuffer expInitDir(SAL_MAX_PATH);
            if (ExpandCommand(parent, viewer->Command, expCommand.Data(), expCommand.Capacity(), FALSE) &&
                ExpandArguments(parent, name, dosName.Data(), viewer->Arguments, expArguments.Data(), expArguments.Capacity(), NULL) &&
                ExpandInitDir(parent, name, dosName.Data(), viewer->InitDir, expInitDir.Data(), expInitDir.Capacity(), FALSE))
            {
                if (SystemPolicies.GetMyRunRestricted() &&
                    !SystemPolicies.GetMyCanRun(expCommand.Data()))
                {
                    MSGBOXEX_PARAMS params;
                    memset(&params, 0, sizeof(params));
                    params.HParent = parent;
                    params.Flags = MSGBOXEX_OK | MSGBOXEX_HELP | MSGBOXEX_ICONEXCLAMATION;
                    params.Caption = LoadStr(IDS_POLICIESRESTRICTION_TITLE);
                    params.Text = LoadStr(IDS_POLICIESRESTRICTION);
                    params.ContextHelpId = IDH_GROUPPOLICY;
                    params.HelpCallback = MessageBoxHelpCallback;
                    SalMessageBoxEx(&params);
                    break;
                }

                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                memset(&si, 0, sizeof(STARTUPINFO));
                si.cb = sizeof(STARTUPINFO);
                si.dwX = place.rcNormalPosition.left;
                si.dwY = place.rcNormalPosition.top;
                si.dwXSize = place.rcNormalPosition.right - place.rcNormalPosition.left;
                si.dwYSize = place.rcNormalPosition.bottom - place.rcNormalPosition.top;
                si.dwFlags |= STARTF_USEPOSITION | STARTF_USESIZE |
                              STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_SHOWNORMAL;

                CPathBuffer cmdLine(2 * SAL_MAX_PATH);
                lstrcpyn(cmdLine.Data(), expCommand.Data(), cmdLine.Capacity());
                AddDoubleQuotesIfNeeded(cmdLine.Data(), cmdLine.Capacity()); // CreateProcess wants the name with spaces in quotes (otherwise it tries various variants, see help)
                int len = (int)strlen(cmdLine.Data());
                int lArgs = (int)strlen(expArguments.Data());
                if (len + lArgs + 2 <= cmdLine.Capacity())
                {
                    cmdLine.Data()[len] = ' ';
                    memcpy(cmdLine.Data() + len + 1, expArguments.Data(), lArgs + 1);

                    MainWindow->SetDefaultDirectories();

                    if (expInitDir.Data()[0] == 0) // this should never happen
                    {
                        lstrcpyn(expInitDir.Data(), name, expInitDir.Capacity());
                        CutDirectory(expInitDir.Data());
                    }
                    if (!CreateProcessForFileAction(cmdLine.Data(), expInitDir.Data(), &si, &pi))
                    {
                        DWORD err = GetLastError();
                        CPathBuffer buff(4 * SAL_MAX_PATH);
                        _snprintf_s(buff.Data(), buff.Capacity(), _TRUNCATE, "%s", LoadStr(IDS_ERROREXECVIEW));
                        SalMessageBox(parent, buff.Data(), LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
                    }
                    else
                    {
                        success = TRUE;
                        if (returnLock)
                        {
                            lock = pi.hProcess;
                            lockOwner = TRUE;
                        }
                        else
                            HANDLES(CloseHandle(pi.hProcess));
                        HANDLES(CloseHandle(pi.hThread));
                    }
                }
                else
                {
                    CPathBuffer buff(4 * SAL_MAX_PATH);
                    _snprintf_s(buff.Data(), buff.Capacity(), _TRUNCATE, "%s %s %s", LoadStr(IDS_ERROREXECVIEW), expCommand.Data(), LoadStr(IDS_TOOLONGNAME));
                    SalMessageBox(parent, buff.Data(), LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
                }
            }
            break;
        }

        case VIEWER_INTERNAL:
        {
            if (Configuration.SavePosition &&
                Configuration.WindowPlacement.length != 0)
            {
                place = Configuration.WindowPlacement;
                // GetWindowPlacement respects the Taskbar, so if the Taskbar is at the top or left,
                // the values are shifted by its dimensions. Perform correction.
                RECT monitorRect2;
                RECT workRect2;
                MultiMonGetClipRectByRect(&place.rcNormalPosition, &workRect2, &monitorRect2);
                OffsetRect(&place.rcNormalPosition, workRect2.left - monitorRect2.left,
                           workRect2.top - monitorRect2.top);
                MultiMonEnsureRectVisible(&place.rcNormalPosition, TRUE);
            }

            HANDLE lockAux = NULL;
            BOOL lockOwnerAux = FALSE;
            if (OpenViewer(name, vtText,
                           place.rcNormalPosition.left,
                           place.rcNormalPosition.top,
                           place.rcNormalPosition.right - place.rcNormalPosition.left,
                           place.rcNormalPosition.bottom - place.rcNormalPosition.top,
                           place.showCmd,
                           returnLock, &lockAux, &lockOwnerAux, NULL,
                           enumFileNamesSourceUID, enumFileNamesLastFileIndex))
            {
                success = TRUE;
                if (returnLock && lockAux != NULL)
                {
                    lock = lockAux;
                    lockOwner = lockOwnerAux;
                }
            }
            break;
        }

        default: // plug-ins
        {
            HANDLE lockAux = NULL;
            BOOL lockOwnerAux = FALSE;

            CPluginData* plugin = Plugins.Get(-viewer->ViewerType - 1);
            if (plugin != NULL && plugin->SupportViewer)
            {
                if (plugin->ViewFile(name, place.rcNormalPosition.left, place.rcNormalPosition.top,
                                     place.rcNormalPosition.right - place.rcNormalPosition.left,
                                     place.rcNormalPosition.bottom - place.rcNormalPosition.top,
                                     place.showCmd, Configuration.AlwaysOnTop,
                                     returnLock, &lockAux, &lockOwnerAux,
                                     enumFileNamesSourceUID, enumFileNamesLastFileIndex,
                                     viewer->ViewerLabel))
                {
                    success = TRUE;
                    if (returnLock && lockAux != NULL)
                    {
                        lock = lockAux;
                        lockOwner = lockOwnerAux;
                    }
                }
            }
            else
                TRACE_E("Unexpected error in (Alt)ViewerMasks (invalid ViewerType).");
            break;
        }
        }
        //    }
    }
    else
    {
        char buff[SAL_MAX_PATH + 300];
        int textID = altView ? IDS_CANT_VIEW_FILE_ALT : IDS_CANT_VIEW_FILE;
        _snprintf_s(buff, _TRUNCATE, "%s", LoadStr(textID));
        SalMessageBox(parent, buff, LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
    }
    return success;
}

void CFilesWindow::EditFile(char* name, DWORD handlerID)
{
    CALL_STACK_MESSAGE3("CFilesWindow::EditFile(%s, %u)", name, handlerID);
    if (!Is(ptDisk) && name == NULL)
    {
        TRACE_E("Incorrect call to CFilesWindow::EditFile()");
        return;
    }

    // verify that the file is on an accessible path
    char path[SAL_MAX_PATH + 10];
    if (name == NULL)
    {
        if (CheckPath(TRUE) != ERROR_SUCCESS)
            return;
    }
    else
    {
        char* backSlash = strrchr(name, '\\');
        if (backSlash != NULL)
        {
            memcpy(path, name, backSlash - name);
            path[backSlash - name] = 0;
            if (CheckPath(TRUE, path) != ERROR_SUCCESS)
                return;
        }
    }

    BOOL addToHistory = name != NULL && Is(ptDisk);

    // if viewing/editing from the panel, obtain the full long name
    std::string unicodeDiskFileName; // UTF-8 full path for local files with Unicode/long names
    if (name == NULL)
    {
        int i = GetCaretIndex();
        if (i >= Dirs->Count && i < Dirs->Count + Files->Count)
        {
            CFileData* f = &Files->At(i - Dirs->Count);
            if (Is(ptDisk))
            {
                std::wstring wideName = GetPathW() != NULL && GetPathW()[0] != 0 ? std::wstring(GetPathW()) : FileActionTextToWide(GetPath());
                SalPathAppendW(wideName, f->UseWideName() ? f->NameW : FileActionTextToWide(f->Name).c_str());
                unicodeDiskFileName = SalWideToMultiBytePath(wideName.c_str(), CP_UTF8);

                lstrcpyn(path, GetPath(), SAL_MAX_PATH);
                if (GetPath()[strlen(GetPath()) - 1] != '\\')
                    strcat(path, "\\");
                char* s = path + strlen(path);
                if ((s - path) + f->NameLen >= SAL_MAX_PATH)
                {
                    if (f->DosName != NULL && strlen(f->DosName) + (s - path) < SAL_MAX_PATH)
                        strcpy(s, f->DosName);
                    else
                    {
                        SalMessageBox(HWindow, LoadStr(IDS_TOOLONGNAME), LoadStr(IDS_ERRORTITLE),
                                      MB_OK | MB_ICONEXCLAMATION);
                        return;
                    }
                }
                else
                    strcpy(s, f->Name);
                // try whether the file name is valid, otherwise try its DOS name as well
                // (handles files accessible only through Unicode or DOS names)
                if (f->DosName != NULL && SalGetFileAttributes(path) == 0xffffffff)
                {
                    DWORD err = GetLastError();
                    if (err == ERROR_FILE_NOT_FOUND || err == ERROR_INVALID_NAME)
                    {
                        if (strlen(f->DosName) + (s - path) < SAL_MAX_PATH)
                        {
                            strcpy(s, f->DosName);
                            if (SalGetFileAttributes(path) == 0xffffffff) // still error -> revert to the long name
                            {
                                if ((s - path) + f->NameLen < SAL_MAX_PATH)
                                    strcpy(s, f->Name);
                            }
                        }
                    }
                }
                name = !unicodeDiskFileName.empty() ? (char*)unicodeDiskFileName.c_str() : path;
                addToHistory = TRUE;
            }
        }
        else
        {
            return;
        }
    }

    // obtain the full DOS name
    CPathBuffer dosName(SAL_MAX_PATH);
    if (GetShortPathName(name, dosName.Data(), dosName.Capacity()) == 0)
    {
        TRACE_I("GetShortPathName() failed.");
        dosName.Data()[0] = 0;
    }

    // find the file name and check if it has an extension - needed for masks
    char* namePart = strrchr(name, '\\');
    if (namePart == NULL)
    {
        TRACE_E("Invalid parameter CFilesWindow::EditFile(): " << name);
        return;
    }
    namePart++;
    char* tmpExt = strrchr(namePart, '.');
    //if (tmpExt == NULL || tmpExt == namePart) tmpExt = namePart + lstrlen(namePart); // ".cvspass" is not an extension...
    if (tmpExt == NULL)
        tmpExt = namePart + lstrlen(namePart); // ".cvspass" is treated as an extension in Windows...
    else
        tmpExt++;

    // position for editors
    WINDOWPLACEMENT place;
    place.length = sizeof(WINDOWPLACEMENT);
    HWND hPlacementWindow = MainWindow->GetDetachedAwareDialogParent(MainWindow->HWindow);
    GetWindowPlacement(hPlacementWindow, &place);
    // GetWindowPlacement respects the Taskbar, so if the Taskbar is at the top or left,
    // the values are shifted by its dimensions. Perform correction.
    RECT monitorRect;
    RECT workRect;
    MultiMonGetClipRectByRect(&place.rcNormalPosition, &workRect, &monitorRect);
    OffsetRect(&place.rcNormalPosition, workRect.left - monitorRect.left,
               workRect.top - monitorRect.top);
    // if called, for example, from find and the main window is minimized,
    // we do not want a minimized editor
    if (place.showCmd == SW_MINIMIZE || place.showCmd == SW_SHOWMINIMIZED ||
        place.showCmd == SW_SHOWMINNOACTIVE)
        place.showCmd = SW_SHOWNORMAL;

    // find the correct editor and launch it
    CEditorMasks* masks = MainWindow->EditorMasks;

    CEditorMasksItem* editor = NULL;

    if (handlerID != 0xFFFFFFFF)
    {
        // attempt to find an editor with matching HandlerID
        int i;
        for (i = 0; editor == NULL && i < masks->Count; i++)
        {
            if (masks->At(i)->HandlerID == handlerID)
                editor = masks->At(i);
        }
    }

    if (editor == NULL)
    {
        int i;
        for (i = 0; i < masks->Count; i++)
        {
            int err;
            if (masks->At(i)->Masks->PrepareMasks(err))
            {
                if (masks->At(i)->Masks->AgreeMasks(namePart, tmpExt))
                {
                    editor = masks->At(i);
                    break;
                }
            }
            else
                TRACE_E("Unexpected error in group mask");
        }
    }

    if (editor != NULL)
    {
        if (addToHistory)
            MainWindow->FileHistory->AddFile(fhitEdit, editor->HandlerID, name); // add file to history

        CPathBuffer expCommand(SAL_MAX_PATH);
        CPathBuffer expArguments(SAL_MAX_PATH);
        CPathBuffer expInitDir(SAL_MAX_PATH);
        if (ExpandCommand(HWindow, editor->Command, expCommand.Data(), expCommand.Capacity(), FALSE) &&
            ExpandArguments(HWindow, name, dosName.Data(), editor->Arguments, expArguments.Data(), expArguments.Capacity(), NULL) &&
            ExpandInitDir(HWindow, name, dosName.Data(), editor->InitDir, expInitDir.Data(), expInitDir.Capacity(), FALSE))
        {
            if (SystemPolicies.GetMyRunRestricted() &&
                !SystemPolicies.GetMyCanRun(expCommand.Data()))
            {
                MSGBOXEX_PARAMS params;
                memset(&params, 0, sizeof(params));
                params.HParent = HWindow;
                params.Flags = MSGBOXEX_OK | MSGBOXEX_HELP | MSGBOXEX_ICONEXCLAMATION;
                params.Caption = LoadStr(IDS_POLICIESRESTRICTION_TITLE);
                params.Text = LoadStr(IDS_POLICIESRESTRICTION);
                params.ContextHelpId = IDH_GROUPPOLICY;
                params.HelpCallback = MessageBoxHelpCallback;
                SalMessageBoxEx(&params);
                return;
            }

            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            memset(&si, 0, sizeof(STARTUPINFO));
            si.cb = sizeof(STARTUPINFO);
            si.dwX = place.rcNormalPosition.left;
            si.dwY = place.rcNormalPosition.top;
            si.dwXSize = place.rcNormalPosition.right - place.rcNormalPosition.left;
            si.dwYSize = place.rcNormalPosition.bottom - place.rcNormalPosition.top;
            si.dwFlags |= STARTF_USEPOSITION | STARTF_USESIZE |
                          STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_SHOWNORMAL;

            CPathBuffer cmdLine(2 * SAL_MAX_PATH);
            lstrcpyn(cmdLine.Data(), expCommand.Data(), cmdLine.Capacity());
            AddDoubleQuotesIfNeeded(cmdLine.Data(), cmdLine.Capacity()); // CreateProcess wants the name with spaces in quotes (otherwise it tries various variants, see help)
            int len = (int)strlen(cmdLine.Data());
            int lArgs = (int)strlen(expArguments.Data());
            if (len + lArgs + 2 <= cmdLine.Capacity())
            {
                cmdLine.Data()[len] = ' ';
                memcpy(cmdLine.Data() + len + 1, expArguments.Data(), lArgs + 1);

                MainWindow->SetDefaultDirectories();

                if (expInitDir.Data()[0] == 0) // this should never happen
                {
                    lstrcpyn(expInitDir.Data(), name, expInitDir.Capacity());
                    CutDirectory(expInitDir.Data());
                }
                if (!CreateProcessForFileAction(cmdLine.Data(), expInitDir.Data(), &si, &pi))
                {
                    DWORD err = GetLastError();
                    CPathBuffer detail(2 * SAL_MAX_PATH);
                    _snprintf_s(detail.Data(), detail.Capacity(), _TRUNCATE, "%s", GetErrorText(err));
                    CPathBuffer buff(4 * SAL_MAX_PATH);
                    _snprintf_s(buff.Data(), buff.Capacity(), _TRUNCATE, "%s%s", LoadStr(IDS_ERROREXECEDIT), detail.Data());
                    SalMessageBox(HWindow, buff.Data(), LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
                }
                else
                {
                    HANDLES(CloseHandle(pi.hProcess));
                    HANDLES(CloseHandle(pi.hThread));
                }
            }
            else
            {
                CPathBuffer detail(2 * SAL_MAX_PATH);
                _snprintf_s(detail.Data(), detail.Capacity(), _TRUNCATE, "%s", LoadStr(IDS_TOOLONGNAME));
                CPathBuffer buff(4 * SAL_MAX_PATH);
                _snprintf_s(buff.Data(), buff.Capacity(), _TRUNCATE, "%s %s %s", LoadStr(IDS_ERROREXECEDIT), expCommand.Data(), LoadStr(IDS_TOOLONGNAME));
                SalMessageBox(HWindow, buff.Data(), LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
            }
        }
    }
    else
    {
        char buff[SAL_MAX_PATH + 300];
        _snprintf_s(buff, _countof(buff), _TRUNCATE, "%s %s", LoadStr(IDS_CANT_EDIT_FILE), name);
        SalMessageBox(HWindow, buff, LoadStr(IDS_ERRORTITLE),
                      MB_OK | MB_ICONEXCLAMATION);
    }
}

void CFilesWindow::EditNewFile()
{
    CALL_STACK_MESSAGE1("CFilesWindow::EditNewFile()");
    BeginStopRefresh(); // snooper takes a break

    // restore DefaultDir
    MainWindow->UpdateDefaultDir(TRUE);

    char path[SAL_MAX_PATH];
    if (Configuration.UseEditNewFileDefault)
        lstrcpyn(path, Configuration.EditNewFileDefault, SAL_MAX_PATH);
    else
        lstrcpyn(path, LoadStr(IDS_EDITNEWFILE_DEFAULTNAME), SAL_MAX_PATH);
    CTruncatedString subject;
    subject.Set(LoadStr(IDS_NEWFILENAME), NULL);

    BOOL first = TRUE;

    while (1)
    {
        CEditNewFileDialog dlg(HWindow, path, SAL_MAX_PATH, &subject, Configuration.EditNewHistory, EDITNEW_HISTORY_SIZE);

        // Some users always create .txt and are satisfied with overwriting just the extension; others create various files and want to overwrite the whole name,
        // so we compromised and introduced a dedicated option for Edit New File in the configuration.
        // ------------------
        // For EditNew, the smart selection of only the name makes no sense because people also change the extension, see our forum:
        // https://forum.altap.cz/viewtopic.php?t=2655
        // -----------------
        // Since Windows Vista, Microsoft introduced a demanded feature: quick rename selects only the name without the dot and extension
        // the same code appears here four times
        if (!Configuration.EditNewSelectAll)
        {
            int selectionEnd = -1;
            if (first)
            {
                const char* dot = strrchr(path, '.');
                if (dot != NULL && dot > path) // although ".cvspass" is an extension in Windows, Explorer selects the entire name, so we do the same
                                               //      if (dot != NULL)
                    selectionEnd = (int)(dot - path);
                dlg.SetSelectionEnd(selectionEnd);
                first = FALSE; // after an error we get the full file name, so we select it all
            }
        }

        if (dlg.Execute() == IDOK)
        {
            UpdateWindow(MainWindow->HWindow);

            // clean the name from undesirable characters at the beginning and end
            // we do this only for the last component; the previous ones already exist and it doesn't matter
            // (the system handles it) or they are checked during creation and an error is shown
            // (we don't clean them, we let the user do some work, it's easy enough)
            char* lastCompName = strrchr(path, '\\');
            MakeValidFileName(lastCompName != NULL ? lastCompName + 1 : path);

            char* errText;
            int errTextID;
            //      if (SalGetFullName(path, &errTextID, MainWindow->GetActivePanel()->Is(ptDisk) ?
            //                         MainWindow->GetActivePanel()->GetPath() : NULL, NextFocusName))
            if (SalGetFullName(path, &errTextID, Is(ptDisk) ? GetPath() : NULL, NextFocusName)) // for consistency with ChangePathToDisk()
            {
                char checkPath[MAX_PATH];
                strcpy(checkPath, path);
                CutDirectory(checkPath);
                if (CheckPath(TRUE, checkPath) != ERROR_SUCCESS)
                {
                    EndStopRefresh(); // snooper will start again now
                    return;
                }

                BOOL invalidName = FileNameInvalidForManualCreate(path);
                HANDLE hFile = INVALID_HANDLE_VALUE;
                if (invalidName)
                    SetLastError(ERROR_INVALID_NAME);
                else
                {
                    hFile = SalCreateFileEx(path, GENERIC_READ | GENERIC_WRITE, 0, FILE_ATTRIBUTE_NORMAL, NULL);
                    HANDLES_ADD_EX(__otQuiet, hFile != INVALID_HANDLE_VALUE, __htFile, __hoCreateFile, hFile, GetLastError(), TRUE);
                }
                BOOL editExisting = FALSE;
                if (hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_EXISTS)
                {
                    if (SalMessageBox(HWindow, LoadStr(IDS_EDITNEWALREADYEX), LoadStr(IDS_QUESTION),
                                      MB_YESNO | MB_ICONEXCLAMATION | MSGBOXEX_ESCAPEENABLED) == IDYES)
                    {
                        editExisting = TRUE;
                    }
                    else
                        break;
                }
                if (hFile != INVALID_HANDLE_VALUE || editExisting)
                {
                    if (!editExisting)
                        HANDLES(CloseHandle(hFile));

                    EditFile(path);

                    // change only in the directory where the file was created
                    MainWindow->PostChangeOnPathNotification(checkPath, FALSE);

                    break;
                }
                else
                    errText = GetErrorText(GetLastError());
            }
            else
                errText = LoadStr(errTextID);
            SalMessageBox(HWindow, errText, LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
        }
        else
            break;
    }
    EndStopRefresh(); // snooper will start again now
}

// fills the popup based on available viewers
void CFilesWindow::FillViewWithMenu(CMenuPopup* popup)
{
    CALL_STACK_MESSAGE1("CFilesWindow::FillViewWithMenu()");

    // remove existing items
    popup->RemoveAllItems();

    // retrieve the list of viewer indexes
    TDirectArray<CViewerMasksItem*> items(50, 10);
    if (!FillViewWithData(&items))
        return;

    MENU_ITEM_INFO mii;
    char buff[1024];
    int i;
    for (i = 0; i < items.Count; i++)
    {
        CViewerMasksItem* item = items[i];

        int imgIndex = -1; // no icon
        if (item->ViewerType < 0)
        {
            int pluginIndex = -item->ViewerType - 1;
            CPluginData* plugin = Plugins.Get(pluginIndex);
            lstrcpy(buff, plugin->Name);
            if (plugin->PluginIconIndex != -1) // the plugin has an icon
                imgIndex = pluginIndex;
        }
        if (item->ViewerType == VIEWER_EXTERNAL)
            _snprintf_s(buff, _TRUNCATE, "%s%s", LoadStr(IDS_VIEWWITH_EXTERNAL), item->Command);
        if (item->ViewerType == VIEWER_INTERNAL)
            lstrcpy(buff, LoadStr(IDS_VIEWWITH_INTERNAL));

        mii.Mask = MENU_MASK_TYPE | MENU_MASK_STRING | MENU_MASK_ID | MENU_MASK_IMAGEINDEX;
        mii.Type = MENU_TYPE_STRING;
        mii.String = buff;
        mii.ID = CM_VIEWWITH_MIN + i;
        mii.ImageIndex = imgIndex;
        if (mii.ID > CM_VIEWWITH_MAX)
        {
            TRACE_E("mii.ID > CM_VIEWWITH_MAX");
            break;
        }
        popup->InsertItem(-1, TRUE, &mii);
    }
    if (popup->GetItemCount() == 0)
    {
        mii.Mask = MENU_MASK_TYPE | MENU_MASK_STRING | MENU_MASK_STATE;
        mii.Type = MENU_TYPE_STRING;
        mii.State = MENU_STATE_GRAYED;
        mii.String = LoadStr(IDS_VIEWWITH_EMPTY);
        popup->InsertItem(-1, TRUE, &mii);
    }
    else
        popup->AssignHotKeys();
}

BOOL CFilesWindow::FillViewWithData(TDirectArray<CViewerMasksItem*>* items)
{
    // merging proceeds through normal and alternate viewers
    int type;
    for (type = 0; type < 2; type++)
    {
        CViewerMasks* masks;
        if (type == 0)
            masks = MainWindow->ViewerMasks;
        else
            masks = MainWindow->AltViewerMasks;

        int i;
        for (i = 0; i < masks->Count; i++)
        {
            CViewerMasksItem* item = masks->At(i);

            BOOL alreadyAdded = FALSE; // we do not want the item listed more than once

            int j;
            for (j = 0; j < items->Count; j++)
            {
                CViewerMasksItem* oldItem = items->At(j);

                if (item->ViewerType == VIEWER_EXTERNAL)
                {
                    if (stricmp(item->Command, oldItem->Command) == 0 &&
                        stricmp(item->Arguments, oldItem->Arguments) == 0 &&
                        stricmp(item->InitDir, oldItem->InitDir) == 0)
                    {
                        alreadyAdded = TRUE;
                        break;
                    }
                }
                else
                {
                    if (item->ViewerType == oldItem->ViewerType)
                    {
                        alreadyAdded = TRUE;
                        break;
                    }
                }
            }

            if (!alreadyAdded)
            {
                items->Add(masks->At(i));
                if (!items->IsGood())
                {
                    items->ResetState();
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

void CFilesWindow::OnViewFileWith(int index)
{
    BeginStopRefresh(); // snooper takes a break

    // get the list of viewer indexes
    TDirectArray<CViewerMasksItem*> items(50, 10);
    if (!FillViewWithData(&items))
    {
        EndStopRefresh(); // snooper will start again now
        return;
    }

    if (index < 0 || index >= items.Count)
    {
        TRACE_E("index=" << index);
        EndStopRefresh(); // snooper will start again now
        return;
    }

    ViewFile(NULL, FALSE, items[index]->HandlerID, Is(ptDisk) ? EnumFileNamesSourceUID : -1,
             -1 /* index determined by focus */);

    EndStopRefresh(); // snooper will start again now
}

void CFilesWindow::ViewFileWith(char* name, HWND hMenuParent, const POINT* menuPoint, DWORD* handlerID,
                                int enumFileNamesSourceUID, int enumFileNamesLastFileIndex)
{
    CALL_STACK_MESSAGE5("CFilesWindow::ViewFileWith(%s, , , %s, %d, %d)", name,
                        (handlerID == NULL ? "NULL" : "non-NULL"), enumFileNamesSourceUID,
                        enumFileNamesLastFileIndex);
    BeginStopRefresh(); // snooper takes a break
    if (handlerID != NULL)
        *handlerID = 0xFFFFFFFF;

    CMenuPopup contextPopup(CML_FILES_VIEWWITH);
    FillViewWithMenu(&contextPopup);
    DWORD cmd = contextPopup.Track(MENU_TRACK_RETURNCMD | MENU_TRACK_RIGHTBUTTON,
                                   menuPoint->x, menuPoint->y, hMenuParent, NULL);
    if (cmd >= CM_VIEWWITH_MIN && cmd <= CM_VIEWWITH_MAX)
    {
        // get the list of viewer indexes
        TDirectArray<CViewerMasksItem*> items(50, 10);
        if (!FillViewWithData(&items))
        {
            EndStopRefresh(); // snooper will start again now
            return;
        }

        int index = cmd - CM_VIEWWITH_MIN;
        if (handlerID == NULL)
            ViewFile(name, FALSE, items[index]->HandlerID, enumFileNamesSourceUID, enumFileNamesLastFileIndex);
        else
            *handlerID = items[index]->HandlerID;
    }

    EndStopRefresh(); // snooper will start again now
}

void CFilesWindow::FillEditWithMenu(CMenuPopup* popup)
{
    CALL_STACK_MESSAGE1("CFilesWindow::FillEditWithMenu()");

    // remove existing items
    popup->RemoveAllItems();

    // retrieve the list of editor indexes
    CEditorMasks* masks = MainWindow->EditorMasks;

    MENU_ITEM_INFO mii;
    char buff[1024];

    int i;
    for (i = 0; i < masks->Count; i++)
    {
        mii.Mask = MENU_MASK_TYPE | MENU_MASK_STRING | MENU_MASK_ID;
        mii.Type = MENU_TYPE_STRING;
        mii.String = buff;
        mii.ID = CM_EDITWITH_MIN + i;
        if (mii.ID > CM_EDITWITH_MAX)
        {
            TRACE_E("mii.ID > CM_EDITWITH_MAX");
            break;
        }

        CEditorMasksItem* item = masks->At(i);

        // if users have multiple rows of masks associated with one viewer/editor,
        // insert the item into the list only once
        BOOL alreadyAdded = FALSE;
        int j;
        for (j = 0; j < i; j++)
        {
            CEditorMasksItem* oldItem = masks->At(j);
            if (stricmp(item->Command, oldItem->Command) == 0 &&
                stricmp(item->Arguments, oldItem->Arguments) == 0 &&
                stricmp(item->InitDir, oldItem->InitDir) == 0)
            {
                alreadyAdded = TRUE;
                break;
            }
        }
        if (!alreadyAdded)
        {
            _snprintf_s(buff, _TRUNCATE, "%s", LoadStr(IDS_EDITWITH_EXTERNAL));
            popup->InsertItem(-1, TRUE, &mii);
        }
    }
    if (popup->GetItemCount() == 0)
    {
        mii.Mask = MENU_MASK_TYPE | MENU_MASK_STRING | MENU_MASK_STATE;
        mii.Type = MENU_TYPE_STRING;
        mii.State = MENU_STATE_GRAYED;
        mii.String = LoadStr(IDS_EDITWITH_EMPTY);
        popup->InsertItem(-1, TRUE, &mii);
    }
    else
        popup->AssignHotKeys();
}

void CFilesWindow::OnEditFileWith(int index)
{
    BeginStopRefresh(); // snooper takes a break

    // get the list of viewer indexes
    // get the list of editor indexes
    CEditorMasks* masks = MainWindow->EditorMasks;

    if (index < 0 || index >= masks->Count)
    {
        TRACE_E("index=" << index);
        EndStopRefresh(); // snooper will start again now
        return;
    }

    EditFile(NULL, masks->At(index)->HandlerID);

    EndStopRefresh(); // snooper will start again now
}

void CFilesWindow::EditFileWith(char* name, HWND hMenuParent, const POINT* menuPoint, DWORD* handlerID)
{
    CALL_STACK_MESSAGE3("CFilesWindow::EditFileWith(%s, , , %s)", name,
                        (handlerID == NULL ? "NULL" : "non-NULL"));
    BeginStopRefresh(); // snooper takes a break
    if (handlerID != NULL)
        *handlerID = 0xFFFFFFFF;

    // get the list of editor indexes
    CEditorMasks* masks = MainWindow->EditorMasks;

    // create menu
    CMenuPopup contextPopup;
    FillEditWithMenu(&contextPopup);
    DWORD cmd = contextPopup.Track(MENU_TRACK_NONOTIFY | MENU_TRACK_RETURNCMD | MENU_TRACK_RIGHTBUTTON,
                                   menuPoint->x, menuPoint->y, hMenuParent, NULL);
    if (cmd >= CM_EDITWITH_MIN && cmd <= CM_EDITWITH_MAX)
    {
        int index = cmd - CM_EDITWITH_MIN;
        if (handlerID == NULL)
            EditFile(name, masks->At(index)->HandlerID);
        else
            *handlerID = masks->At(index)->HandlerID;
    }

    EndStopRefresh(); // snooper will start again now
}

BOOL FileNameInvalidForManualCreate(const char* path)
{
    const char* name = strrchr(path, '\\');
    if (name != NULL)
    {
        name++;
        int nameLen = (int)strlen(name);
        return nameLen > 0 && ((unsigned char)*name <= ' ' ||
                               (unsigned char)name[nameLen - 1] <= ' ' ||
                               name[nameLen - 1] == '.');
    }
    return FALSE;
}

BOOL MakeValidFileName(char* path)
{
    // trim spaces at the beginning and spaces and dots at the end of the name; Explorer does it
    // and people wanted the same behavior, see https://forum.altap.cz/viewtopic.php?f=16&t=5891
    // and https://forum.altap.cz/viewtopic.php?f=2&t=4210
    BOOL ch = FALSE;
    char* n = path;
    while (*n != 0 && (unsigned char)*n <= ' ')
        n++;
    if (n > path)
    {
        memmove(path, n, strlen(n) + 1);
        ch = TRUE;
    }
    n = path + strlen(path);
    while (n > path && ((unsigned char)*(n - 1) <= ' ' || *(n - 1) == '.'))
        n--;
    if (*n != 0)
    {
        *n = 0;
        ch = TRUE;
    }
    return ch;
}

BOOL CutSpacesFromBothSides(char* path)
{
    // trim spaces at the beginning and end of the name
    BOOL ch = FALSE;
    char* n = path;
    while (*n != 0 && (unsigned char)*n <= ' ')
        n++;
    if (n > path)
    {
        memmove(path, n, strlen(n) + 1);
        ch = TRUE;
    }
    n = path + strlen(path);
    while (n > path && (unsigned char)*(n - 1) <= ' ')
        n--;
    if (*n != 0)
    {
        *n = 0;
        ch = TRUE;
    }
    return ch;
}

BOOL CutDoubleQuotesFromBothSides(char* path)
{
    int len = (int)strlen(path);
    if (len >= 2 && path[0] == '"' && path[len - 1] == '"')
    {
        memmove(path, path + 1, len - 2);
        path[len - 2] = 0;
        return TRUE;
    }
    return FALSE;
}

void CFilesWindow::CreateDir(CFilesWindow* target)
{
    CALL_STACK_MESSAGE1("CFilesWindow::CreateDir()");
    BeginStopRefresh(); // snooper takes a break

    char path[SAL_MAX_PATH], nextFocus[SAL_MAX_PATH];
    path[0] = 0;
    nextFocus[0] = 0;

    // restore DefaultDir
    MainWindow->UpdateDefaultDir(MainWindow->GetActivePanel() == this);

    if (Is(ptDisk)) // create directory on disk
    {
        CTruncatedString subject;
        subject.Set(LoadStr(IDS_CREATEDIRECTORY_TEXT), NULL);
        CCopyMoveDialog dlg(HWindow, path, SAL_MAX_PATH, LoadStr(IDS_CREATEDIRECTORY_TITLE),
                            &subject, IDD_CREATEDIRDIALOG,
                            Configuration.CreateDirHistory, CREATEDIR_HISTORY_SIZE,
                            FALSE);

    CREATE_AGAIN:

        if (dlg.Execute() == IDOK)
        {
            UpdateWindow(MainWindow->HWindow);

            // for disk paths we flip '/' to '\\' and eliminate duplicate backslashes
            SlashesToBackslashesAndRemoveDups(path);

            // clean the name from undesirable characters at the beginning and end
            // we do this only for the last component; the previous ones already exist and it doesn't matter
            // (the system handles it) or they are checked during creation and an error is shown
            // (we don't clean them, we let the user do some work, it's easy enough)
            char* lastCompName = strrchr(path, '\\');
            MakeValidFileName(lastCompName != NULL ? lastCompName + 1 : path);

            int errTextID;
            BOOL fullNameOK = FALSE;
            std::wstring pathW = SalMultiByteToWidePath(path, CP_UTF8);
            if (pathW.empty() && path[0] != 0)
                pathW = SalMultiByteToWidePath(path, CP_ACP);
            std::wstring nextFocusW;
            if (!pathW.empty())
            {
                if (wcschr(pathW.c_str(), L'\\') == NULL && wcschr(pathW.c_str(), L'/') == NULL)
                    nextFocusW = pathW;
                const wchar_t* curDirW = GetPathW();
                fullNameOK = SalGetFullNameW(pathW, &errTextID, curDirW, NULL, NULL, FALSE);
                if (fullNameOK)
                {
                    std::string pathUtf8 = SalWideToMultiBytePath(pathW.c_str(), CP_UTF8);
                    lstrcpyn(path, pathUtf8.c_str(), SAL_MAX_PATH);
                    if (!nextFocusW.empty())
                    {
                        std::string focusUtf8 = SalWideToMultiBytePath(nextFocusW.c_str(), CP_UTF8);
                        lstrcpyn(nextFocus, focusUtf8.c_str(), SAL_MAX_PATH);
                    }
                }
            }
            if ((!fullNameOK && !SalGetFullName(path, &errTextID, Is(ptDisk) ? GetPath() : NULL, nextFocus, NULL, SAL_MAX_PATH)) ||
                strlen(path) >= SAL_MAX_PATH)
            {
                if (strlen(path) >= SAL_MAX_PATH)
                    errTextID = IDS_TOOLONGPATH;
                /* even if the string is empty we want an error message
        if (errTextID == IDS_EMPTYNAMENOTALLOWED)
        {
          EndStopRefresh(); // snooper will start again now
          return; // empty string, nothing to do
        }
        */
                SalMessageBox(HWindow, LoadStr(errTextID), LoadStr(IDS_ERRORCREATINGDIR),
                              MB_OK | MB_ICONEXCLAMATION);
                goto CREATE_AGAIN;
            }
            else
            {
                char checkPath[3 * MAX_PATH];
                GetRootPath(checkPath, path);
                if (CheckPath(TRUE, checkPath) != ERROR_SUCCESS)
                    goto CREATE_AGAIN;
                strcpy(checkPath, path);
                CutDirectory(checkPath);
                char newDir[3 * MAX_PATH];
                if (!CheckAndCreateDirectory(checkPath, HWindow, FALSE, NULL, 0, newDir, TRUE, TRUE))
                    goto CREATE_AGAIN;
                if (newDir[0] != 0)
                {
                    // change only in the directory where the first directory was created (other directories did not exist, no reason to report changes there)
                    CutDirectory(newDir);
                    MainWindow->PostChangeOnPathNotification(newDir, FALSE);
                }

                //---  path creation
                while (1)
                {
                    HCURSOR oldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

                    DWORD err;
                    BOOL invalidName = FileNameInvalidForManualCreate(path);
                    BOOL created = FALSE;
                    if (!invalidName && !pathW.empty())
                    {
                        created = SalCreateDirectoryExW(pathW.c_str(), NULL);
                        err = created ? ERROR_SUCCESS : GetLastError();
                    }
                    else if (!invalidName)
                        created = SalCreateDirectoryEx(path, &err);
                    if (!invalidName && created)
                    {
                        SetCursor(oldCur);
                        if (nextFocus[0] != 0)
                            strcpy(NextFocusName, nextFocus);

                        // change only in the directory where the directory was created
                        MainWindow->PostChangeOnPathNotification(checkPath, FALSE);

                        EndStopRefresh(); // snooper will start again now
                        return;
                    }
                    else
                    {
                        if (invalidName)
                            err = ERROR_INVALID_NAME;
                        SetCursor(oldCur);

                        CFileErrorDlg dlg2(HWindow, LoadStr(IDS_ERRORCREATINGDIR), path, GetErrorText(err), FALSE, IDD_ERROR3);
                        dlg2.Execute();
                        goto CREATE_AGAIN;
                    }
                }
            }
        }
    }
    else
    {
        if (Is(ptPluginFS) && GetPluginFS()->NotEmpty() &&
            GetPluginFS()->IsServiceSupported(FS_SERVICE_CREATEDIR)) // FS is in the panel
        {
            // lower the thread priority to "normal" (so operations don't overload the machine)
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

            char newName[2 * MAX_PATH];
            newName[0] = 0;
            BOOL cancel = FALSE;
            BOOL ret = GetPluginFS()->CreateDir(GetPluginFS()->GetPluginFSName(), 1, HWindow, newName, cancel);
            if (!cancel) // not a cancel of the operation
            {
                if (!ret)
                {
                    CTruncatedString subject;
                    subject.Set(LoadStr(IDS_CREATEDIRECTORY_TEXT), NULL);
                    CCopyMoveDialog dlg(HWindow, path, SAL_MAX_PATH, LoadStr(IDS_CREATEDIRECTORY_TITLE),
                                        &subject, IDD_CREATEDIRDIALOG,
                                        Configuration.CreateDirHistory, CREATEDIR_HISTORY_SIZE,
                                        FALSE);
                    while (1)
                    {
                        // open the standard dialog
                        if (dlg.Execute() == IDOK)
                        {
                            strcpy(newName, path);
                            ret = GetPluginFS()->CreateDir(GetPluginFS()->GetPluginFSName(), 2, HWindow, newName, cancel);
                            if (ret || cancel)
                                break; // not an error (cancel or success)
                            strcpy(path, newName);
                        }
                        else
                        {
                            WaitForESCRelease();
                            cancel = TRUE;
                            break;
                        }
                    }
                }

                if (ret && !cancel) // operation completed successfully
                {
                    lstrcpyn(NextFocusName, newName, MAX_PATH); // ensure focus of the new name after refresh
                }
            }

            // raise the thread priority again, the operation has finished
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
        }
    }

    UpdateWindow(MainWindow->HWindow);
    EndStopRefresh(); // snooper will start again now
}


namespace
{
    std::wstring MultiByteFileNameToWideBest(const char* name)
    {
        if (name == NULL || *name == 0)
            return std::wstring();

        int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, NULL, 0);
        if (len > 0)
        {
            std::wstring ret(len - 1, L'\0');
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, &ret[0], len);
            return ret;
        }

        return SalMultiByteToWidePath(name, CP_ACP);
    }

    std::string WideFileNameToMultiByteBest(const std::wstring& name)
    {
        if (name.empty())
            return std::string();
        if (GetACP() == CP_UTF8)
            return SalWideToMultiBytePath(name.c_str(), CP_UTF8);

        BOOL usedDefaultChar = FALSE;
        int len = WideCharToMultiByte(CP_ACP, 0, name.c_str(), -1, NULL, 0, NULL, &usedDefaultChar);
        if (len > 0 && !usedDefaultChar)
        {
            std::string ret(len - 1, '\0');
            WideCharToMultiByte(CP_ACP, 0, name.c_str(), -1, &ret[0], len, NULL, NULL);
            return ret;
        }

        return SalWideToMultiBytePath(name.c_str(), CP_UTF8);
    }

    std::string WidePathToDialogText(const std::wstring& path)
    {
        if (path.empty())
            return std::string();
        if (GetACP() == CP_UTF8)
            return SalWideToMultiBytePath(path.c_str(), CP_UTF8);

        BOOL usedDefaultChar = FALSE;
        int len = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, path.c_str(), -1, NULL, 0, NULL, &usedDefaultChar);
        if (len > 0 && !usedDefaultChar)
        {
            std::string text(len, '\0');
            WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, path.c_str(), -1, &text[0], len, NULL, NULL);
            text.resize(len - 1);
            return text;
        }

        return SalWideToMultiBytePath(path.c_str(), CP_UTF8);
    }

    void GetFileOverwriteInfoW(char* buff, int buffLen, HANDLE file, DWORD attrs, FILETIME* fileTime = NULL, BOOL* getTimeFailed = NULL)
    {
        FILETIME lastWrite;
        SYSTEMTIME st;
        FILETIME ft;
        char date[50], time[50];
        if (!GetFileTime(file, NULL, NULL, &lastWrite) ||
            !FileTimeToLocalFileTime(&lastWrite, &ft) ||
            !FileTimeToSystemTime(&ft, &st))
        {
            if (getTimeFailed != NULL)
                *getTimeFailed = TRUE;
            date[0] = 0;
            time[0] = 0;
        }
        else
        {
            if (fileTime != NULL)
                *fileTime = ft;
            if (GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, time, 50) == 0)
                sprintf(time, "%u:%02u:%02u", st.wHour, st.wMinute, st.wSecond);
            if (GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, date, 50) == 0)
                sprintf(date, "%u.%u.%u", st.wDay, st.wMonth, st.wYear);
        }

        char attr[30];
        lstrcpy(attr, ", ");
        if (attrs != INVALID_FILE_ATTRIBUTES)
            GetAttrsString(attr + 2, attrs);
        if (strlen(attr) == 2)
            attr[0] = 0;

        char number[50];
        CQuadWord size;
        DWORD err;
        if (SalGetFileSize(file, size, err))
            NumberToStr(number, size);
        else
            number[0] = 0;

        _snprintf_s(buff, buffLen, _TRUNCATE, "%s, %s, %s%s", number, date, time, attr);
    }

    void ClearReadOnlyAttrW(const std::wstring& path, DWORD attr)
    {
        if (attr == INVALID_FILE_ATTRIBUTES)
            attr = GetFileAttributesW(path.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY) != 0)
            SetFileAttributesW(path.c_str(), attr & ~FILE_ATTRIBUTE_READONLY);
    }

    BOOL TryBypassDosNameOverwriteW(const std::wstring& srcPath, const std::wstring& tgtPath,
                                    const std::wstring& tgtPathDisplay, DWORD* err)
    {
        WIN32_FIND_DATAW data;
        HANDLE find = HANDLES_Q(FindFirstFileW(tgtPath.c_str(), &data));
        if (find == INVALID_HANDLE_VALUE)
            return FALSE;
        HANDLES(FindClose(find));

        const wchar_t* tgtName = SalPathFindFileNameW(tgtPathDisplay.c_str());
        if (tgtName == NULL || data.cAlternateFileName[0] == 0)
            return FALSE;

        BOOL matchesOnlyDosName =
            CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, tgtName, -1, data.cAlternateFileName, -1) == CSTR_EQUAL &&
            CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, tgtName, -1, data.cFileName, -1) != CSTR_EQUAL;
        if (!matchesOnlyDosName)
            return FALSE;

        std::wstring origFullName = tgtPathDisplay;
        size_t slash = origFullName.find_last_of(L"\\/");
        if (slash == std::wstring::npos)
            origFullName = data.cFileName;
        else
            origFullName.erase(slash + 1).append(data.cFileName);

        std::wstring tmpName = tgtPathDisplay;
        slash = tmpName.find_last_of(L"\\/");
        if (slash == std::wstring::npos)
            tmpName.clear();
        else
            tmpName.erase(slash + 1);

        if (tmpName.empty())
        {
            *err = ERROR_INVALID_NAME;
            return TRUE;
        }

        std::wstring origFullNameOp = origFullName.length() >= MAX_PATH ? SalPathAddExtendedPrefixW(origFullName.c_str()) : origFullName;
        DWORD num = (GetTickCount() / 10) % 0xFFF;
        std::wstring tmpNameOp;
        BOOL tmpRenamed = FALSE;
        for (int attempt = 0; attempt < 0x1000; attempt++)
        {
            WCHAR tmpPart[20];
            swprintf_s(tmpPart, _countof(tmpPart), L"sal%03X", num++ & 0xFFF);
            std::wstring candidate = tmpName + tmpPart;
            tmpNameOp = candidate.length() >= MAX_PATH ? SalPathAddExtendedPrefixW(candidate.c_str()) : candidate;
            if (MoveFileExW(origFullNameOp.c_str(), tmpNameOp.c_str(), 0))
            {
                tmpRenamed = TRUE;
                break;
            }
            DWORD moveErr = GetLastError();
            if (moveErr != ERROR_FILE_EXISTS && moveErr != ERROR_ALREADY_EXISTS)
            {
                *err = moveErr;
                return TRUE;
            }
        }

        if (!tmpRenamed)
        {
            *err = ERROR_ALREADY_EXISTS;
            return TRUE;
        }

        BOOL moveDone = MoveFileExW(srcPath.c_str(), tgtPath.c_str(), 0);
        DWORD moveErr = moveDone ? ERROR_SUCCESS : GetLastError();
        if (!MoveFileExW(tmpNameOp.c_str(), origFullNameOp.c_str(), 0))
        {
            TRACE_I("TryBypassDosNameOverwriteW(): unable to restore temporary DOS-name conflict file");
            if (moveDone)
            {
                MoveFileExW(tgtPath.c_str(), srcPath.c_str(), 0);
                moveDone = FALSE;
                moveErr = GetLastError();
                MoveFileExW(tmpNameOp.c_str(), origFullNameOp.c_str(), 0);
            }
            else
                moveErr = GetLastError();
        }

        *err = moveErr;
        return TRUE;
    }

    void BuildRenameSubjectFormat(char* buff, int buffLen, BOOL isDir)
    {
        const char* renameTo = LoadStr(IDS_RENAME_TO);
        const char* itemFormat = LoadStr(isDir ? IDS_QUESTION_DIRECTORY : IDS_QUESTION_FILE);
        const char* insertPos = NULL;
        for (const char* p = renameTo; *p != 0; p++)
        {
            if (*p == '%' && *(p + 1) == '%')
            {
                p++;
                continue;
            }
            if (*p == '%' && *(p + 1) == 's')
            {
                insertPos = p;
                break;
            }
        }

        if (insertPos != NULL)
        {
            std::string subjectFormat(renameTo, insertPos - renameTo);
            subjectFormat += itemFormat;
            subjectFormat += insertPos + 2;
            lstrcpyn(buff, subjectFormat.c_str(), buffLen);
        }
        else
            lstrcpyn(buff, renameTo, buffLen);
    }

    std::wstring FileDataDisplayNameW(const CFileData* f, const char* fallbackName)
    {
        if (f != NULL && f->UseWideName())
            return std::wstring(f->NameW);
        return MultiByteFileNameToWideBest(fallbackName);
    }

    void UpdateFileDataNameAfterRename(CFileData* f, const std::wstring& newNameW, BOOL isDir)
    {
        std::string newNameA = WideFileNameToMultiByteBest(newNameW);
        char* dupName = DupStr(newNameA.c_str());
        wchar_t* dupNameW = (wchar_t*)malloc((newNameW.length() + 1) * sizeof(wchar_t));
        if (dupNameW != NULL)
            wcscpy(dupNameW, newNameW.c_str());
        if (dupName != NULL)
        {
            if (f->Name != NULL)
                free(f->Name);
            if (f->NameW != NULL)
                free(f->NameW);
            f->Name = dupName;
            f->NameW = dupNameW;
            f->NameLen = (int)strlen(f->Name);
            if (isDir)
                f->Ext = f->Name + f->NameLen;
            else
            {
                f->Ext = strrchr(f->Name, '.');
                if (f->Ext == NULL)
                    f->Ext = f->Name + f->NameLen;
                else
                    f->Ext++;
            }
        }
        else if (dupNameW != NULL)
            free(dupNameW);
    }
}


void CFilesWindow::RenameFileInternalW(CFileData* f, const std::wstring& newNameW, BOOL isDir, BOOL* mayChange, BOOL* tryAgain)
{
    *tryAgain = TRUE;
    if (f == NULL || newNameW.empty() || newNameW.find_first_of(L"\\/:<>|\"") != std::wstring::npos)
        return;

    std::wstring basePath = GetPathW() != NULL && GetPathW()[0] != 0 ? std::wstring(GetPathW()) : SalMultiByteToWidePath(GetPath());
    if (basePath.empty())
        return;

    std::wstring oldNameW = f->UseWideName() ? std::wstring(f->NameW) : SalMultiByteToWidePath(f->Name);
    if (oldNameW.empty())
        return;
    if (CompareStringW(LOCALE_USER_DEFAULT, 0, oldNameW.c_str(), -1, newNameW.c_str(), -1) == CSTR_EQUAL)
    {
        *tryAgain = FALSE;
        return;
    }

    std::wstring srcPath = basePath;
    std::wstring tgtPath = basePath;
    SalPathAppendW(srcPath, oldNameW.c_str());
    SalPathAppendW(tgtPath, newNameW.c_str());
    if (srcPath.length() >= 32767 || tgtPath.length() >= 32767)
    {
        SalMessageBox(HWindow, LoadStr(IDS_TOOLONGNAME), LoadStr(IDS_ERRORRENAMINGFILE),
                      MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    std::wstring srcPathDisplay = srcPath;
    std::wstring tgtPathDisplay = tgtPath;
    if (srcPath.length() >= MAX_PATH)
        srcPath = SalPathAddExtendedPrefixW(srcPath.c_str());
    if (tgtPath.length() >= MAX_PATH)
        tgtPath = SalPathAddExtendedPrefixW(tgtPath.c_str());

    *mayChange = TRUE;
    if (MoveFileExW(srcPath.c_str(), tgtPath.c_str(), 0))
    {
        std::string nextFocus = SalWideToMultiBytePath(newNameW.c_str(), GetACP() == CP_UTF8 ? CP_UTF8 : CP_ACP);
        lstrcpyn(NextFocusName, nextFocus.c_str(), MAX_PATH);
        UpdateFileDataNameAfterRename(f, newNameW, isDir);
        *tryAgain = FALSE;
    }
    else
    {
        DWORD err = GetLastError();
        if ((err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS) &&
            CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, srcPathDisplay.c_str(), -1, tgtPathDisplay.c_str(), -1) != CSTR_EQUAL)
        {
            BOOL dosNameConflict = TryBypassDosNameOverwriteW(srcPath, tgtPath, tgtPathDisplay, &err);
            if (dosNameConflict && err == ERROR_SUCCESS)
            {
                std::string nextFocus = SalWideToMultiBytePath(newNameW.c_str(), GetACP() == CP_UTF8 ? CP_UTF8 : CP_ACP);
                lstrcpyn(NextFocusName, nextFocus.c_str(), MAX_PATH);
                UpdateFileDataNameAfterRename(f, newNameW, isDir);
                *tryAgain = FALSE;
            }

            DWORD inAttr = dosNameConflict ? INVALID_FILE_ATTRIBUTES : GetFileAttributesW(srcPath.c_str());
            DWORD outAttr = dosNameConflict ? INVALID_FILE_ATTRIBUTES : GetFileAttributesW(tgtPath.c_str());

            if (!dosNameConflict && inAttr != INVALID_FILE_ATTRIBUTES && outAttr != INVALID_FILE_ATTRIBUTES &&
                (inAttr & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
                (outAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                std::string srcText = WidePathToDialogText(srcPathDisplay);
                std::string tgtText = WidePathToDialogText(tgtPathDisplay);
                HANDLE in = HANDLES_Q(CreateFileW(srcPath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
                HANDLE out = HANDLES_Q(CreateFileW(tgtPath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
                if (in != INVALID_HANDLE_VALUE && out != INVALID_HANDLE_VALUE)
                {
                    char iAttr[101], oAttr[101];
                    GetFileOverwriteInfoW(iAttr, _countof(iAttr), in, inAttr);
                    GetFileOverwriteInfoW(oAttr, _countof(oAttr), out, outAttr);
                    HANDLES(CloseHandle(in));
                    HANDLES(CloseHandle(out));

                    COverwriteDlg dlg(HWindow, tgtText.c_str(), oAttr, srcText.c_str(), iAttr, TRUE);
                    int res = (int)dlg.Execute();

                    switch (res)
                    {
                    case IDCANCEL:
                        *tryAgain = FALSE;
                    case IDNO:
                        err = ERROR_SUCCESS;
                        break;

                    case IDYES:
                    {
                        ClearReadOnlyAttrW(tgtPath, outAttr);
                        if (!DeleteFileW(tgtPath.c_str()) || !MoveFileExW(srcPath.c_str(), tgtPath.c_str(), 0))
                            err = GetLastError();
                        else
                        {
                            err = ERROR_SUCCESS;
                            std::string nextFocus = SalWideToMultiBytePath(newNameW.c_str(), GetACP() == CP_UTF8 ? CP_UTF8 : CP_ACP);
                            lstrcpyn(NextFocusName, nextFocus.c_str(), MAX_PATH);
                            UpdateFileDataNameAfterRename(f, newNameW, isDir);
                            *tryAgain = FALSE;
                        }
                        break;
                    }
                    }
                }
                else
                {
                    if (in == INVALID_HANDLE_VALUE)
                        TRACE_E("Unable to open file " << srcText.c_str());
                    else
                        HANDLES(CloseHandle(in));
                    if (out == INVALID_HANDLE_VALUE)
                        TRACE_E("Unable to open file " << tgtText.c_str());
                    else
                        HANDLES(CloseHandle(out));
                }
            }
        }

        if (err != ERROR_SUCCESS)
        {
            TRACE_E("RenameFileInternalW(): MoveFileExW failed: " << GetErrorText(err));
            SalMessageBox(HWindow, GetErrorText(err), LoadStr(IDS_ERRORRENAMINGFILE),
                          MB_OK | MB_ICONEXCLAMATION);
        }
    }
}

void CFilesWindow::RenameFileInternal(CFileData* f, const char* formatedFileName, BOOL isDir, BOOL* mayChange, BOOL* tryAgain)
{
    *tryAgain = TRUE;
    const char* s = formatedFileName;
    while (*s != 0 && *s != '\\' && *s != '/' && *s != ':' &&
           *s >= 32 && *s != '<' && *s != '>' && *s != '|' && *s != '"')
        s++;
    if (formatedFileName[0] != 0 && *s == 0)
    {
        char finalName[2 * MAX_PATH];
        MaskName(finalName, 2 * MAX_PATH, f->Name, formatedFileName);

        // clean the name from undesirable characters at the beginning and end
        MakeValidFileName(finalName);

        std::wstring finalNameW = SalMultiByteToWidePath(finalName, GetACP() == CP_UTF8 ? CP_UTF8 : CP_ACP);
        if (!finalNameW.empty())
        {
            RenameFileInternalW(f, finalNameW, isDir, mayChange, tryAgain);
            return;
        }

        int l = (int)strlen(GetPath());
        char tgtPath[MAX_PATH];
        memmove(tgtPath, GetPath(), l);
        if (GetPath()[l - 1] != '\\')
            tgtPath[l++] = '\\';
        if (strlen(finalName) + l < MAX_PATH && (f->NameLen + l < MAX_PATH ||
                                                 f->DosName != NULL && strlen(f->DosName) + l < MAX_PATH))
        {
            strcpy(tgtPath + l, finalName);
            char path[MAX_PATH];
            strcpy(path, GetPath());
            char* end = path + l;
            if (*(end - 1) != '\\')
                *--end = '\\';
            if (f->NameLen + l < MAX_PATH)
                strcpy(path + l, f->Name);
            else
                strcpy(path + l, f->DosName);

            BOOL ret = FALSE;

            BOOL handsOFF = FALSE;
            CFilesWindow* otherPanel = MainWindow->GetNonActivePanel();
            int otherPanelPathLen = (int)strlen(otherPanel->GetPath());
            int pathLen = (int)strlen(path);
            // are we changing the path of the other panel?
            if (otherPanelPathLen >= pathLen &&
                StrNICmp(path, otherPanel->GetPath(), pathLen) == 0 &&
                (otherPanelPathLen == pathLen ||
                 otherPanel->GetPath()[pathLen] == '\\'))
            {
                otherPanel->HandsOff(TRUE);
                handsOFF = TRUE;
            }

            *mayChange = TRUE;

            // try renaming from the long name first and if there is a problem then
            // from the DOS name (handles files/directories accessible only via Unicode or DOS names)
            BOOL moveRet = SalMoveFile(path, tgtPath);
            DWORD err = 0;
            if (!moveRet)
            {
                err = GetLastError();
                if ((err == ERROR_FILE_NOT_FOUND || err == ERROR_INVALID_NAME) &&
                    f->DosName != NULL)
                {
                    strcpy(path + l, f->DosName);
                    moveRet = SalMoveFile(path, tgtPath);
                    if (!moveRet)
                        err = GetLastError();
                    strcpy(path + l, f->Name);
                }
            }

            if (moveRet)
            {

            REN_OPERATION_DONE:

                strcpy(NextFocusName, tgtPath + l);
                ret = TRUE;
            }
            else
            {
                if (StrICmp(path, tgtPath) != 0 && // if it isn't just change-case
                    (err == ERROR_FILE_EXISTS ||   // check whether it's only rewriting the DOS name of the file
                     err == ERROR_ALREADY_EXISTS))
                {
                    WIN32_FIND_DATA data;
                    HANDLE find = HANDLES_Q(FindFirstFile(tgtPath, &data));
                    if (find != INVALID_HANDLE_VALUE)
                    {
                        HANDLES(FindClose(find));
                        const char* tgtName = SalPathFindFileName(tgtPath);
                        if (StrICmp(tgtName, data.cAlternateFileName) == 0 && // match only for DOS name
                            StrICmp(tgtName, data.cFileName) != 0)            // (full name differs)
                        {
                            // rename ("clean up") the file/directory with the conflicting DOS name to a temporary 8.3 name (no extra DOS name needed)
                            char tmpName[MAX_PATH + 20];
                            lstrcpyn(tmpName, tgtPath, MAX_PATH);
                            CutDirectory(tmpName);
                            SalPathAddBackslash(tmpName, MAX_PATH + 20);
                            char* tmpNamePart = tmpName + strlen(tmpName);
                            char origFullName[MAX_PATH];
                            if (SalPathAppend(tmpName, data.cFileName, MAX_PATH))
                            {
                                strcpy(origFullName, tmpName);
                                DWORD num = (GetTickCount() / 10) % 0xFFF;
                                while (1)
                                {
                                    _snprintf_s(tmpNamePart, (MAX_PATH + 20) - (tmpNamePart - tmpName), _TRUNCATE, "sal%03X", num++);
                                    if (SalMoveFile(origFullName, tmpName))
                                        break;
                                    DWORD e = GetLastError();
                                    if (e != ERROR_FILE_EXISTS && e != ERROR_ALREADY_EXISTS)
                                    {
                                        tmpName[0] = 0;
                                        break;
                                    }
                                }
                                if (tmpName[0] != 0) // if we successfully "cleaned" the conflicting file, try moving
                                {                    // the file again, then return the temporary file its original name
                                    BOOL moveDone = SalMoveFile(path, tgtPath);
                                    if (!SalMoveFile(tmpName, origFullName))
                                    { // this can apparently happen: Windows creates a file named origFullName instead of 'tgtPath' (DOS name)
                                        TRACE_I("CFilesWindow::RenameFileInternal(): Unexpected situation: unable to rename file from tmp-name to original long file name! " << origFullName);
                                        if (moveDone)
                                        {
                                            if (SalMoveFile(tgtPath, path))
                                                moveDone = FALSE;
                                            if (!SalMoveFile(tmpName, origFullName))
                                                TRACE_E("CFilesWindow::RenameFileInternal(): Fatal unexpected situation: unable to rename file from tmp-name to original long file name! " << origFullName);
                                        }
                                    }

                                    if (moveDone)
                                        goto REN_OPERATION_DONE;
                                }
                            }
                            else
                                TRACE_E("CFilesWindow::RenameFileInternal(): Original full file name is too long, unable to bypass only-dos-name-overwrite problem!");
                        }
                    }
                }
                if ((err == ERROR_ALREADY_EXISTS ||
                     err == ERROR_FILE_EXISTS) &&
                    StrICmp(path, tgtPath) != 0) // overwrite the file?
                {
                    DWORD inAttr = SalGetFileAttributes(path);
                    DWORD outAttr = SalGetFileAttributes(tgtPath);

                    if ((inAttr & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
                        (outAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
                    { // only if both are files
                        HANDLE in = HANDLES_Q(CreateFile(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                                         NULL));
                        HANDLE out = HANDLES_Q(CreateFile(tgtPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                                          NULL));
                        if (in != INVALID_HANDLE_VALUE && out != INVALID_HANDLE_VALUE)
                        {
                            char iAttr[101], oAttr[101];
                            GetFileOverwriteInfo(iAttr, _countof(iAttr), in, path);
                            GetFileOverwriteInfo(oAttr, _countof(oAttr), out, tgtPath);
                            HANDLES(CloseHandle(in));
                            HANDLES(CloseHandle(out));

                            COverwriteDlg dlg(HWindow, tgtPath, oAttr, path, iAttr, TRUE);
                            int res = (int)dlg.Execute();

                            switch (res)
                            {
                            case IDCANCEL:
                                ret = TRUE;
                            case IDNO:
                                err = ERROR_SUCCESS;
                                break;

                            case IDYES:
                            {
                                ClearReadOnlyAttr(tgtPath); // so it can be deleted ...
                                if (!DeleteFile(tgtPath) || !SalMoveFile(path, tgtPath))
                                    err = GetLastError();
                                else
                                {
                                    err = ERROR_SUCCESS;
                                    ret = TRUE;
                                    strcpy(NextFocusName, tgtPath + l);
                                }
                                break;
                            }
                            }
                        }
                        else
                        {
                            if (in == INVALID_HANDLE_VALUE)
                                TRACE_E("Unable to open file " << path);
                            else
                                HANDLES(CloseHandle(in));
                            if (out == INVALID_HANDLE_VALUE)
                                TRACE_E("Unable to open file " << tgtPath);
                            else
                                HANDLES(CloseHandle(out));
                        }
                    }
                }

                if (err != ERROR_SUCCESS)
                    SalMessageBox(HWindow, GetErrorText(err), LoadStr(IDS_ERRORRENAMINGFILE),
                                  MB_OK | MB_ICONEXCLAMATION);
            }
            if (handsOFF)
                otherPanel->HandsOff(FALSE);
            *tryAgain = !ret;
        }
        else
            SalMessageBox(HWindow, LoadStr(IDS_TOOLONGNAME), LoadStr(IDS_ERRORRENAMINGFILE),
                          MB_OK | MB_ICONEXCLAMATION);
    }
    else
        SalMessageBox(HWindow, GetErrorText(ERROR_INVALID_NAME), LoadStr(IDS_ERRORRENAMINGFILE),
                      MB_OK | MB_ICONEXCLAMATION);
}

void CFilesWindow::RenameFile(int specialIndex)
{
    CALL_STACK_MESSAGE2("CFilesWindow::RenameFile(%d)", specialIndex);

    int i;
    if (specialIndex != -1)
        i = specialIndex;
    else
        i = GetCaretIndex();
    if (i < 0 || i >= Dirs->Count + Files->Count)
        return; // invalid index

    BOOL subDir;
    if (Dirs->Count > 0)
        subDir = (strcmp(Dirs->At(0).Name, "..") == 0);
    else
        subDir = FALSE;
    if (i == 0 && subDir)
        return; // we do not work with ".."

    CFileData* f = NULL;
    BOOL isDir = i < Dirs->Count;
    f = isDir ? &Dirs->At(i) : &Files->At(i - Dirs->Count);

    char formatedFileName[SAL_MAX_PATH];
    AlterFileName(formatedFileName, f->Name, -1, Configuration.FileNameFormat, 0, isDir);
    std::wstring formatedFileNameW = FileDataDisplayNameW(f, formatedFileName);
    if (f->UseWideName())
    {
        std::string formatedUtf8 = WideFileNameToMultiByteBest(formatedFileNameW);
        lstrcpyn(formatedFileName, formatedUtf8.c_str(), SAL_MAX_PATH);
    }

    char buff[200];
    BuildRenameSubjectFormat(buff, _countof(buff), isDir);
    CTruncatedString subject;
    subject.SetW(SalMultiByteToWidePath(buff, CP_ACP).c_str(), formatedFileNameW.c_str());
    CCopyMoveDialog dlg(HWindow, formatedFileName, SAL_MAX_PATH, LoadStr(IDS_RENAME_TITLE),
                        &subject, IDD_RENAMEDIALOG, Configuration.QuickRenameHistory,
                        QUICKRENAME_HISTORY_SIZE, FALSE);

    if (Is(ptDisk)) // rename on disk
    {
#ifndef _WIN64
        if (Windows64Bit && isDir)
        {
            char path[SAL_MAX_PATH];
            lstrcpyn(path, GetPath(), SAL_MAX_PATH);
            if (SalPathAppend(path, f->Name, SAL_MAX_PATH) && IsWin64RedirectedDir(path, NULL, FALSE))
            {
                char msg[300 + MAX_PATH];
                _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_ERRRENAMINGW64ALIAS), f->Name);
                SalMessageBox(MainWindow->HWindow, msg, LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
                return;
            }
        }
#endif // _WIN64

        BeginSuspendMode(); // snooper takes a break

        BOOL mayChange = FALSE;
        while (1)
        {
            // if no item is selected, select the one under focus and store its name
            char temporarySelected[MAX_PATH];
            SelectFocusedItemAndGetName(temporarySelected, MAX_PATH);

            // Since Windows Vista, Microsoft introduced a demanded feature: quick rename selects only the name without the dot and extension
            // the same code appears here four times
            if (!Configuration.QuickRenameSelectAll)
            {
                int selectionEnd = -1;
                if (!isDir)
                {
                    const char* dot = strrchr(formatedFileName, '.');
                    if (dot != NULL && dot > formatedFileName) // although ".cvspass" is an extension in Windows, Explorer selects the entire name, so we do the same
                                                               //        if (dot != NULL)
                        selectionEnd = (int)(dot - formatedFileName);
                }
                dlg.SetSelectionEnd(selectionEnd);
            }

            int dlgRes = (int)dlg.Execute();

            // if we selected an item, we deselect it again
            UnselectItemWithName(temporarySelected);

            if (dlgRes == IDOK)
            {
                UpdateWindow(MainWindow->HWindow);

                BOOL tryAgain;
                std::wstring newNameW = SalMultiByteToWidePath(formatedFileName, GetACP() == CP_UTF8 ? CP_UTF8 : CP_ACP);
                RenameFileInternalW(f, newNameW, isDir, &mayChange, &tryAgain);
                if (!tryAgain)
                    break;
            }
            else
                break;
        }

        // refresh of manually refreshed directories
        if (mayChange)
        {
            // change in the directory shown in the panel and, if a directory was renamed, then also in subdirectories
            MainWindow->PostChangeOnPathNotification(GetPath(), isDir);
        }

        // if a Salamander window is active, end suspend mode
        EndSuspendMode();
    }
    else
    {
        if (Is(ptPluginFS) && GetPluginFS()->NotEmpty() &&
            GetPluginFS()->IsServiceSupported(FS_SERVICE_QUICKRENAME)) // FS is in the panel
        {
            BeginSuspendMode(); // snooper takes a break

            // lower the thread priority to "normal" (so operations don't overload the machine)
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

            char newName[MAX_PATH];
            newName[0] = 0;
            BOOL cancel = FALSE;

            // if no item is selected, select the one under focus and store its name
            char temporarySelected[MAX_PATH];
            SelectFocusedItemAndGetName(temporarySelected, MAX_PATH);

            BOOL ret = GetPluginFS()->QuickRename(GetPluginFS()->GetPluginFSName(), 1, HWindow, *f, isDir, newName, cancel);

            // if we selected an item, we deselect it again
            UnselectItemWithName(temporarySelected);

            if (!cancel) // not a cancel of the operation
            {
                if (!ret)
                {
                    while (1)
                    {
                        // open the standard dialog
                        // if no item is selected, select the one under focus and store its name
                        SelectFocusedItemAndGetName(temporarySelected, MAX_PATH);

                        // Since Windows Vista, Microsoft introduced a demanded feature: quick rename selects only the name without the dot and extension
                        // the same code appears here four times
                        if (!Configuration.QuickRenameSelectAll)
                        {
                            int selectionEnd = -1;
                            if (!isDir)
                            {
                                const char* dot = strrchr(formatedFileName, '.');
                                if (dot != NULL && dot > formatedFileName) // although ".cvspass" is an extension in Windows, Explorer selects the entire name, so we do the same
                                                                           //        if (dot != NULL)
                                    selectionEnd = (int)(dot - formatedFileName);
                            }
                            dlg.SetSelectionEnd(selectionEnd);
                        }

                        int dlgRes = (int)dlg.Execute();

                        // if we selected an item, we deselect it again
                        UnselectItemWithName(temporarySelected);

                        if (dlgRes == IDOK)
                        {
                            strcpy(newName, formatedFileName);
                            ret = GetPluginFS()->QuickRename(GetPluginFS()->GetPluginFSName(), 2, HWindow, *f, isDir, newName, cancel);
                            if (ret || cancel)
                                break; // not an error (cancel or success)
                            strcpy(formatedFileName, newName);
                        }
                        else
                        {
                            WaitForESCRelease();
                            cancel = TRUE;
                            break;
                        }
                    }
                }

                if (ret && !cancel) // operation completed successfully
                {
                    strcpy(NextFocusName, newName); // ensure focus of the new name after refresh
                }
            }

            // raise the thread priority again, the operation has finished
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

            // if a Salamander window is active, end suspend mode
            EndSuspendMode();
        }
    }
}

void CFilesWindow::CancelUI()
{
    if (QuickSearchMode)
        EndQuickSearch();
    QuickRenameEnd();
}

BOOL CFilesWindow::IsQuickRenameActive()
{
    return QuickRenameWindow.HWindow != NULL;
}

void CFilesWindow::AdjustQuickRenameRect(const char* text, RECT* r)
{
    // measure the length of the text
    HDC hDC = HANDLES(GetDC(ListBox->HWindow));
    HFONT hOldFont = (HFONT)SelectObject(hDC, GetPanelFont());
    SIZE sz;
    GetTextExtentPoint32(hDC, text, (int)strlen(text), &sz);
    TEXTMETRIC tm;
    GetTextMetrics(hDC, &tm);
    SelectObject(hDC, hOldFont);
    HANDLES(ReleaseDC(ListBox->HWindow, hDC));

    int minWidth = QuickRenameRect.right - QuickRenameRect.left + 2;
    int minHeight = QuickRenameRect.bottom - QuickRenameRect.top;

    int optimalWidth = sz.cx + 4 + tm.tmHeight;

    r->left--;

    r->right = r->left + optimalWidth;

    if (r->right - r->left < minWidth)
        r->right = r->left + minWidth;

    // we do not want to exceed the panel boundaries
    RECT maxR = ListBox->FilesRect;
    if (r->left < maxR.left)
        r->left = maxR.left;
    if (r->right > maxR.right)
        r->right = maxR.right;
}

void CFilesWindow::AdjustQuickRenameRectW(const wchar_t* text, RECT* r)
{
    HDC hDC = HANDLES(GetDC(ListBox->HWindow));
    HFONT hOldFont = (HFONT)SelectObject(hDC, GetPanelFont());
    SIZE sz;
    GetTextExtentPoint32W(hDC, text, (int)wcslen(text), &sz);
    TEXTMETRIC tm;
    GetTextMetrics(hDC, &tm);
    SelectObject(hDC, hOldFont);
    HANDLES(ReleaseDC(ListBox->HWindow, hDC));

    int minWidth = QuickRenameRect.right - QuickRenameRect.left + 2;
    int optimalWidth = sz.cx + 4 + tm.tmHeight;

    r->left--;
    r->right = r->left + optimalWidth;

    if (r->right - r->left < minWidth)
        r->right = r->left + minWidth;

    RECT maxR = ListBox->FilesRect;
    if (r->left < maxR.left)
        r->left = maxR.left;
    if (r->right > maxR.right)
        r->right = maxR.right;
}

void CFilesWindow::AdjustQuickRenameWindow()
{
    if (!IsQuickRenameActive())
    {
        //    TRACE_E("QuickRenameWindow is not active.");
        return;
    }

    RECT r;
    GetWindowRect(QuickRenameWindow.HWindow, &r);
    MapWindowPoints(NULL, HWindow, (POINT*)&r, 2);

    if (IsWindowUnicode(QuickRenameWindow.HWindow))
    {
        int length = GetWindowTextLengthW(QuickRenameWindow.HWindow);
        if (length < 0)
            length = 0;
        std::vector<WCHAR> buff(length + 1);
        int copied = GetWindowTextW(QuickRenameWindow.HWindow, buff.data(), length + 1);
        if (copied < 0)
            copied = 0;
        buff[copied] = 0;
        AdjustQuickRenameRectW(buff.data(), &r);
    }
    else
    {
        char buff[3 * MAX_PATH];
        GetWindowText(QuickRenameWindow.HWindow, buff, 3 * MAX_PATH);
        AdjustQuickRenameRect(buff, &r);
    }
    SetWindowPos(QuickRenameWindow.HWindow, NULL, 0, 0,
                 r.right - r.left, r.bottom - r.top,
                 SWP_NOMOVE | SWP_NOZORDER);
}

/*
// I ran into a sorting issue: Vista keeps items in place, but Salamander needs to insert them
// so I'm shelving this for now
void
CFilesWindow::QuickRenameOnIndex(int index)
{
  if (index >= 0 && index < Dirs->Count + Files->Count)
  {
    QuickRenameIndex = index;
    SetCaretIndex(index, FALSE);

    RECT r;
    if (ListBox->GetItemRect(index, &r))
    {
      ListBox->GetIndex(r.left, r.top, FALSE, &QuickRenameRect);
      QuickRenameIndex = index;
      QuickRenameBegin(index, &QuickRenameRect);
    }
  }
}
*/

void CFilesWindow::QuickRenameBegin(int index, const RECT* labelRect)
{
    CALL_STACK_MESSAGE2("CFilesWindow::QuickRenameBegin(%d, )", index);

    if (!(Is(ptDisk) || Is(ptPluginFS) && GetPluginFS()->NotEmpty() &&
                            GetPluginFS()->IsServiceSupported(FS_SERVICE_QUICKRENAME)))
        return;

    if (QuickRenameWindow.HWindow != NULL)
    {
        TRACE_E("Quick Rename is already active");
        return;
    }

    if (index < 0 || index >= Dirs->Count + Files->Count)
        return; // invalid index

    BOOL subDir;
    if (Dirs->Count > 0)
        subDir = (strcmp(Dirs->At(0).Name, "..") == 0);
    else
        subDir = FALSE;
    if (index == 0 && subDir)
        return; // we do not work with ".."

    CFileData* f = NULL;
    BOOL isDir = index < Dirs->Count;
    f = isDir ? &Dirs->At(index) : &Files->At(index - Dirs->Count);

    char formatedFileName[MAX_PATH];
    AlterFileName(formatedFileName, f->Name, -1, Configuration.FileNameFormat, 0, isDir);
    std::wstring formatedFileNameW = FileDataDisplayNameW(f, formatedFileName);

    // Since Windows Vista, Microsoft introduced a demanded feature: quick rename selects only the name without the dot and extension
    // the same code appears here four times
    int selectionEndBytes = -1;
    int selectionEndChars = -1;
    if (!Configuration.QuickRenameSelectAll)
    {
        if (!isDir)
        {
            const wchar_t* dotW = wcsrchr(formatedFileNameW.c_str(), L'.');
            if (dotW != NULL && dotW > formatedFileNameW.c_str()) // although ".cvspass" is an extension in Windows, Explorer selects the entire name, so we do the same
            {
                selectionEndChars = (int)(dotW - formatedFileNameW.c_str());
                std::string prefix = WideFileNameToMultiByteBest(std::wstring(formatedFileNameW.c_str(), selectionEndChars));
                selectionEndBytes = (int)prefix.length();
            }
        }
    }

    // if this is a FS, we must first call QuickRename with mode=1
    // allowing the file system to open its own rename dialog
    if (Is(ptPluginFS) && GetPluginFS()->NotEmpty() &&
        GetPluginFS()->IsServiceSupported(FS_SERVICE_QUICKRENAME)) // FS is in the panel
    {
        BeginSuspendMode(); // snooper takes a break

        // lower the thread priority to "normal" (so operations don't overload the machine)
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

        char newName[MAX_PATH];
        newName[0] = 0;
        BOOL cancel = FALSE;

        // if no item is selected, select the one under focus and store its name
        char temporarySelected[MAX_PATH];
        SelectFocusedItemAndGetName(temporarySelected, MAX_PATH);

        BOOL ret = GetPluginFS()->QuickRename(GetPluginFS()->GetPluginFSName(), 1, HWindow, *f, isDir, newName, cancel);

        // if we selected an item, we deselect it again
        UnselectItemWithName(temporarySelected);

        if (ret && !cancel) // operation completed successfully
        {
            strcpy(NextFocusName, newName); // ensure focus of the new name after refresh
        }

        // raise the thread priority again, the operation has finished
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

        // if a Salamander window is active, end suspend mode
        EndSuspendMode();

        if (cancel || ret)
            return;
    }

    RECT r = *labelRect;
    AdjustQuickRenameRectW(formatedFileNameW.c_str(), &r);

    HWND hWnd = NULL;
    if (Is(ptDisk) || f->UseWideName() || GetACP() == CP_UTF8)
    {
        QuickRenameWindow.SetUnicodeWindow(TRUE);
        hWnd = QuickRenameWindow.CreateExW(0,
                                          L"edit",
                                          L"",
                                          WS_BORDER | WS_CHILD | WS_CLIPSIBLINGS | ES_AUTOHSCROLL | ES_LEFT,
                                          r.left, r.top, r.right - r.left, r.bottom - r.top,
                                          GetListBoxHWND(),
                                          NULL,
                                          HInstance,
                                          &QuickRenameWindow);
        if (hWnd != NULL)
            SetWindowTextW(hWnd, formatedFileNameW.c_str());
    }
    BOOL unicodeEdit = hWnd != NULL;
    if (hWnd == NULL)
    {
        QuickRenameWindow.SetUnicodeWindow(FALSE);
        hWnd = QuickRenameWindow.CreateEx(0,
                                          "edit",
                                          formatedFileName,
                                          WS_BORDER | WS_CHILD | WS_CLIPSIBLINGS | ES_AUTOHSCROLL | ES_LEFT,
                                          r.left, r.top, r.right - r.left, r.bottom - r.top,
                                          GetListBoxHWND(),
                                          NULL,
                                          HInstance,
                                          &QuickRenameWindow);
    }
    if (hWnd == NULL)
    {
        TRACE_E("Cannot create QuickRenameWindow");
        return;
    }

    BeginSuspendMode(TRUE); // snooper takes a break

    // font the same as the panel
    SendMessage(hWnd, WM_SETFONT, (WPARAM)GetPanelFont(), 0);
    int leftMargin = LOWORD(SendMessage(hWnd, EM_GETMARGINS, 0, 0));
    if (leftMargin < 2)
        SendMessage(hWnd, EM_SETMARGINS, EC_LEFTMARGIN, 2);

    // Select all or only the name without dot and extension according to QuickRenameSelectAll.
    int selectionEndForControl = selectionEndBytes;
    if (unicodeEdit)
    {
        if (selectionEndChars >= 0)
            selectionEndForControl = selectionEndChars;
    }
    else
    {
        if (selectionEndBytes >= 0)
            selectionEndForControl = selectionEndBytes;
    }
    if (selectionEndForControl >= 0)
    {
        if (unicodeEdit)
            SendMessageW(hWnd, EM_SETSEL, 0, selectionEndForControl);
        else
            SendMessage(hWnd, EM_SETSEL, 0, selectionEndForControl);
    }
    else
    {
        if (unicodeEdit)
            SendMessageW(hWnd, EM_SETSEL, 0, (LPARAM)-1);
        else
            SendMessage(hWnd, EM_SETSEL, 0, (LPARAM)-1);
    }
    ShowWindow(hWnd, SW_SHOW);
    SetFocus(hWnd);

    // Keep the selection in character units for Unicode edit controls.  Posting the
    // ANSI message here lets the control reinterpret the end offset later, which is
    // exactly where names containing surrogate pairs (emoji) could fall back to
    // "select all" and include the extension even when the option is disabled.
    if (unicodeEdit)
        SendMessageW(hWnd, EM_SETSEL, 0, selectionEndForControl >= 0 ? selectionEndForControl : (LPARAM)-1);
    else
        SendMessage(hWnd, EM_SETSEL, 0, selectionEndForControl >= 0 ? selectionEndForControl : (LPARAM)-1);

    return;
}

void CFilesWindow::QuickRenameEnd()
{
    CALL_STACK_MESSAGE1("CFilesWindow::QuickRenameEnd()");
    if (QuickRenameWindow.HWindow != NULL && QuickRenameWindow.GetCloseEnabled())
    {
        // if a Salamander window is active, end suspend mode
        EndSuspendMode(TRUE);

        // avoid cycles caused by WM_KILLFOCUS and similar
        BOOL old = QuickRenameWindow.GetCloseEnabled();
        QuickRenameWindow.SetCloseEnabled(FALSE);

        DestroyWindow(QuickRenameWindow.HWindow);

        QuickRenameWindow.SetCloseEnabled(old);
    }
}

BOOL CFilesWindow::HandeQuickRenameWindowKey(WPARAM wParam)
{
    CALL_STACK_MESSAGE2("CFilesWindow::HandeQuickRenameWindowKey(0x%IX)", wParam);

    if (wParam == VK_ESCAPE)
    {
        QuickRenameEnd();
        return TRUE;
    }

    int index = GetCaretIndex();
    if (index < 0 || index >= Dirs->Count + Files->Count)
        return TRUE; // invalid index
    CFileData* f = NULL;
    BOOL isDir = index < Dirs->Count;
    f = isDir ? &Dirs->At(index) : &Files->At(index - Dirs->Count);

    QuickRenameWindow.SetCloseEnabled(FALSE);

    HWND hWnd = QuickRenameWindow.HWindow;
    char newName[MAX_PATH];
    std::wstring newNameW;
    if (IsWindowUnicode(hWnd))
    {
        int length = GetWindowTextLengthW(hWnd);
        if (length < 0)
            length = 0;
        std::vector<WCHAR> text(length + 1);
        int copied = GetWindowTextW(hWnd, text.data(), length + 1);
        if (copied < 0)
            copied = 0;
        text[copied] = 0;
        newNameW.assign(text.data(), copied);
        std::string nameA = SalWideToMultiBytePath(newNameW.c_str(), GetACP() == CP_UTF8 ? CP_UTF8 : CP_ACP);
        lstrcpyn(newName, nameA.c_str(), MAX_PATH);
    }
    else
    {
        GetWindowText(hWnd, newName, MAX_PATH);
        newNameW = SalMultiByteToWidePath(newName, GetACP() == CP_UTF8 ? CP_UTF8 : CP_ACP);
    }


    // lower the thread priority to "normal" (so operations don't overload the machine)
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

    BOOL tryAgain = FALSE;
    BOOL mayChange = FALSE;
    if (Is(ptDisk))
    {
        // If this is an in-place rename and the user didn't change the name, we shouldn't
        // attempt to rename it because the user might be on a CD-ROM or other read-only disk
        // and we would display the "Access is denied" error. The user has no mouse option
        // to cancel the operation, so they would have to press Escape.
        // Explorer behaves this way now.
        if (IsWindowUnicode(hWnd))
        {
            std::wstring oldNameW = f->UseWideName() ? std::wstring(f->NameW) : MultiByteFileNameToWideBest(f->Name);
            if (CompareStringW(LOCALE_USER_DEFAULT, 0, oldNameW.c_str(), -1, newNameW.c_str(), -1) != CSTR_EQUAL)
                RenameFileInternalW(f, newNameW, isDir, &mayChange, &tryAgain);
        }
        else if (strcmp(f->Name, newName) != 0)
            RenameFileInternal(f, newName, isDir, &mayChange, &tryAgain);
    }
    else if (Is(ptPluginFS) && GetPluginFS()->NotEmpty() &&
             GetPluginFS()->IsServiceSupported(FS_SERVICE_QUICKRENAME)) // FS is in the panel
    {
        // open the standard dialog
        BOOL cancel;
        BOOL ret = GetPluginFS()->QuickRename(GetPluginFS()->GetPluginFSName(), 2, HWindow, *f, isDir, newName, cancel);
        if (!ret && !cancel)
        {
            tryAgain = TRUE;
            SetWindowText(hWnd, newName);
        }
        else
        {
            if (ret && !cancel) // operation completed successfully
            {
                strcpy(NextFocusName, newName); // ensure focus of the new name after refresh
            }
        }
    }

    // raise the thread priority again, the operation has finished
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    // refresh of manually refreshed directories
    if (mayChange)
    {
        // change in the directory shown in the panel and if a directory was renamed, then also in subdirectories
        MainWindow->PostChangeOnPathNotification(GetPath(), isDir);
    }

    QuickRenameWindow.SetCloseEnabled(TRUE);
    if (!tryAgain)
    {
        QuickRenameEnd();
        //    if (wParam == VK_TAB)
        //    {
        //      BOOL shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        //      PostMessage(HWindow, WM_USER_RENAME_NEXT_ITEM, !shiftPressed, 0);
        //    }
        return TRUE;
    }
    else
    {
        SetFocus(QuickRenameWindow.HWindow);
        return FALSE;
    }
}

void CFilesWindow::KillQuickRenameTimer()
{
    if (QuickRenameTimer != 0)
    {
        KillTimer(GetListBoxHWND(), QuickRenameTimer);
        QuickRenameTimer = 0;
    }
}

//****************************************************************************
//
// CQuickRenameWindow
//

CQuickRenameWindow::CQuickRenameWindow()
    : CWindow(ooStatic)
{
    FilesWindow = NULL;
    CloseEnabled = TRUE;
    SkipNextCharacter = FALSE;
    PendingHighSurrogate = 0;
}

void CQuickRenameWindow::SetPanel(CFilesWindow* filesWindow)
{
    FilesWindow = filesWindow;
}

void CQuickRenameWindow::SetCloseEnabled(BOOL closeEnabled)
{
    CloseEnabled = closeEnabled;
}

BOOL CQuickRenameWindow::GetCloseEnabled()
{
    return CloseEnabled;
}

LRESULT
CQuickRenameWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_UNICHAR:
    {
        if (wParam == UNICODE_NOCHAR)
            return TRUE;
        if (wParam > 0 && wParam <= 0x10FFFF)
        {
            WCHAR chars[3];
            if (wParam <= 0xFFFF)
            {
                chars[0] = (WCHAR)wParam;
                chars[1] = 0;
            }
            else
            {
                DWORD codePoint = (DWORD)wParam - 0x10000;
                chars[0] = (WCHAR)(0xD800 + (codePoint >> 10));
                chars[1] = (WCHAR)(0xDC00 + (codePoint & 0x3FF));
                chars[2] = 0;
            }
            SendMessageW(HWindow, EM_REPLACESEL, TRUE, (LPARAM)chars);
            if (FilesWindow != NULL)
                FilesWindow->AdjustQuickRenameWindow();
            return 0;
        }
        break;
    }

    case WM_CHAR:
    {
        if (SkipNextCharacter)
        {
            SkipNextCharacter = FALSE; // prevent a beep
            return FALSE;
        }

        if (wParam == VK_ESCAPE || wParam == VK_RETURN /*|| wParam == VK_TAB*/)
        {
            FilesWindow->HandeQuickRenameWindowKey(wParam);
            return 0;
        }

        if (IsWindowUnicode(HWindow))
        {
            if (wParam >= 0xD800 && wParam <= 0xDBFF)
            {
                PendingHighSurrogate = (WCHAR)wParam;
                return 0;
            }
            if (wParam >= 0xDC00 && wParam <= 0xDFFF && PendingHighSurrogate != 0)
            {
                WCHAR chars[3] = {PendingHighSurrogate, (WCHAR)wParam, 0};
                PendingHighSurrogate = 0;
                SendMessageW(HWindow, EM_REPLACESEL, TRUE, (LPARAM)chars);
                if (FilesWindow != NULL)
                    FilesWindow->AdjustQuickRenameWindow();
                return 0;
            }
            PendingHighSurrogate = 0;
            if (wParam >= 0x20)
            {
                WCHAR chars[2] = {(WCHAR)wParam, 0};
                SendMessageW(HWindow, EM_REPLACESEL, TRUE, (LPARAM)chars);
                if (FilesWindow != NULL)
                    FilesWindow->AdjustQuickRenameWindow();
                return 0;
            }
        }
        break;
    }

    case WM_KEYDOWN:
    {
        if (wParam == 'A')
        {
            // since Windows Vista, SelectAll works by default, so we leave select-all to them
            if (!WindowsVistaAndLater)
            {
                BOOL controlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                BOOL altPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;
                BOOL shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                if (controlPressed && !shiftPressed && !altPressed)
                {
                    SendMessage(HWindow, EM_SETSEL, 0, -1);
                    SkipNextCharacter = TRUE; // prevent a beep
                    return 0;
                }
            }
        }
        break;
    }
    }
    return CWindow::WindowProc(uMsg, wParam, lParam);
}
