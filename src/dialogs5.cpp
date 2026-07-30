// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"

#include <new>
#include <string>
#include <vector>

#include "tasklist.h"
#include "mainwnd.h"
#include "edtlbwnd.h"
#include "cfgdlg.h"
#include "dialogs.h"
#include "usermenu.h"
#include "execute.h"
#include "plugins.h"
#include "fileswnd.h"
#include "gui.h"
#include "menu.h"
#include "shellib.h"
#include "consts.h"
#include "darkmode.h"
#include "svg.h"
#include "plugins/salamatrix/salamatrix_storage.h"
#include "plugins/salamatrix/salamatrix_ui.h"
#include "third_party/darkmodelib/include/Darkmodelib.h"

static char LastSelectedPluginDLLName[MAX_PATH] = {0}; // after reopening Plugins Manager, select the last chosen plugin

namespace
{
BOOL HasStablePluginKey(const char* value, const char* stableKey)
{
    if (value == NULL || stableKey == NULL)
        return FALSE;

    int stableKeyLength = (int)strlen(stableKey);
    if ((int)strlen(value) < stableKeyLength)
        return FALSE;
    return StrNICmp(value, stableKey, stableKeyLength) == 0 &&
           (value[stableKeyLength] == 0 || value[stableKeyLength] == '-' || value[stableKeyLength] == '_');
}

BOOL IsPluginName(const char* value, const char* name)
{
    return value != NULL && name != NULL && StrICmp(value, name) == 0;
}

bool ShouldUsePluginsDarkPalette()
{
    return DarkModeShouldUseDarkColors();
}

BOOL GetLoadedSamandarinUpdateNotifier(CPluginData** plugin)
{
    int samandarinIndex;
    if (Plugins.FindDLL("samandarin\\samandarin.spl", samandarinIndex))
    {
        CPluginData* samandarin = Plugins.Get(samandarinIndex);
        if (samandarin != NULL && samandarin->GetLoaded())
        {
            if (plugin != NULL)
                *plugin = samandarin;
            return TRUE;
        }
    }

    if (plugin != NULL)
        *plugin = NULL;
    return FALSE;
}

void SetPluginManagerText(HWND ctrl, const char* text)
{
    if (text == NULL)
        text = "";

    int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0);
    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (required == 0)
    {
        codePage = CP_ACP;
        flags = 0;
        required = MultiByteToWideChar(codePage, flags, text, -1, NULL, 0);
    }

    if (required > 0)
    {
        std::vector<WCHAR> wide(required);
        if (MultiByteToWideChar(codePage, flags, text, -1, wide.data(), required) != 0)
        {
            // Some plugin metadata may already contain U+FFFD from an earlier
            // lossy conversion.  Repair the known copyright pattern before
            // showing it, otherwise SetWindowTextW would faithfully display
            // the replacement character.
            const WCHAR copyrightPrefix[] = L"Copyright ";
            size_t copyrightPrefixLen = ARRAYSIZE(copyrightPrefix) - 1;
            if (wcsncmp(wide.data(), copyrightPrefix, copyrightPrefixLen) == 0 &&
                wide[copyrightPrefixLen] == 0xFFFD)
            {
                wide[copyrightPrefixLen] = 0x00A9;
            }

            SetWindowTextW(ctrl, wide.data());
            return;
        }
    }

    SetWindowText(ctrl, text);
}

Salamatrix::Extensions::IExtensionsService* QueryExtensionService()
{
    CSalamanderServiceQuery query;
    memset(&query, 0, sizeof(query));
    query.ServiceId = SALAMATRIX_SERVICE_EXTENSIONS;
    query.MinimumVersion = SALAMATRIX_EXTENSIONS_VERSION_1_2;
    CSalamanderServiceResult result;
    memset(&result, 0, sizeof(result));
    if (!Plugins.QueryService(&query, &result) || result.Interface == NULL)
        return NULL;
    return static_cast<Salamatrix::Extensions::IExtensionsService*>(result.Interface);
}

Salamatrix::Storage::IStorageService* QueryExtensionStorageService()
{
    CSalamanderServiceQuery query;
    memset(&query, 0, sizeof(query));
    query.ServiceId = SALAMATRIX_SERVICE_STORAGE;
    query.MinimumVersion = SALAMATRIX_STORAGE_VERSION_1_0;
    CSalamanderServiceResult result;
    memset(&result, 0, sizeof(result));
    if (!Plugins.QueryService(&query, &result) || result.Interface == NULL)
        return NULL;
    return static_cast<Salamatrix::Storage::IStorageService*>(result.Interface);
}

Salamatrix::UI::IUIService* QueryExtensionUIService()
{
    CSalamanderServiceQuery query;
    memset(&query, 0, sizeof(query));
    query.ServiceId = SALAMATRIX_SERVICE_UI;
    query.MinimumVersion = SALAMATRIX_UI_VERSION_1_0;
    CSalamanderServiceResult result;
    memset(&result, 0, sizeof(result));
    if (!Plugins.QueryService(&query, &result) || result.Interface == NULL)
        return NULL;
    return static_cast<Salamatrix::UI::IUIService*>(result.Interface);
}
}

//
// ****************************************************************************
// CPluginsDlg
//

namespace
{
enum
{
    MaxPluginManagerExtensionSettings = 24
};

struct CExtensionSettingsControl
{
    Salamatrix::Extensions::ExtensionSettingInfo Setting;
    Salamatrix::UI::IControl* Control;
};

BOOL ConfigureManifestExtensionSettings(
    HWND parent,
    const Salamatrix::Extensions::ExtensionInfo& extension,
    Salamatrix::Extensions::IExtensionsService* extensions,
    Salamatrix::Storage::IStorageService* storage,
    Salamatrix::UI::IUIService* ui)
{
    if (extensions == NULL || storage == NULL || ui == NULL)
        return FALSE;
    const int settingCount = extensions->GetExtensionSettingCount(
        extension.Descriptor.Id);
    if (settingCount <= 0)
        return FALSE;
    if (settingCount > MaxPluginManagerExtensionSettings)
    {
        SalMessageBox(
            parent, LoadStr(IDS_PLUGINEXTCONFIGTOOMANY),
            LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    std::vector<Salamatrix::Extensions::ExtensionSettingInfo> settings(settingCount);
    int contentHeight = 12;
    for (int index = 0; index < settingCount; ++index)
    {
        if (!extensions->GetExtensionSettingInfo(
                extension.Descriptor.Id, index, &settings[index]))
            return FALSE;
        contentHeight += settings[index].Multiline ? 64 : 28;
    }

    Salamatrix::UI::DialogOptions options;
    options.Title = extension.Descriptor.Name;
    options.Parent = parent;
    options.Width = 440;
    options.Height = static_cast<short>(72 + contentHeight);
    Salamatrix::UI::IDialog* dialog = ui->CreateSalamatrixDialog(options);
    if (dialog == NULL)
        return FALSE;

    std::vector<CExtensionSettingsControl> controls;
    BOOL valid = TRUE;
    for (int index = 0; index < settingCount && valid; ++index)
    {
        CExtensionSettingsControl entry;
        entry.Control = NULL;
        entry.Setting = settings[index];

        const char* label = entry.Setting.Label[0] != 0
                                ? entry.Setting.Label
                                : entry.Setting.Key;
        std::string displayLabel;
        if (entry.Setting.Group[0] != 0)
        {
            displayLabel = entry.Setting.Group;
            displayLabel += ": ";
        }
        displayLabel += label;

        Salamatrix::UI::ControlOptions controlOptions;
        Salamatrix::UI::ControlLayout layout;
        layout.HasBounds = TRUE;
        layout.X = 12;
        layout.Y = 12;
        for (int previous = 0; previous < index; ++previous)
            layout.Y += settings[previous].Multiline ? 64 : 28;
        layout.Width = 408;
        layout.Height = entry.Setting.Multiline ? 54 : 18;
        if (entry.Setting.Type == Salamatrix::Extensions::ExtensionSettingBoolean)
        {
            controlOptions.Id = entry.Setting.Key;
            controlOptions.Text = displayLabel.c_str();
            BOOL checked = FALSE;
            if (storage->GetValueType(
                    extension.Descriptor.Id, entry.Setting.Key) ==
                Salamatrix::Storage::StorageValueBoolean)
            {
                storage->GetBoolean(
                    extension.Descriptor.Id, entry.Setting.Key, &checked);
            }
            controlOptions.Checked = checked;
            entry.Control = dialog->AddControlEx(
                Salamatrix::UI::ControlKindCheckBox, controlOptions, layout);
        }
        else
        {
            Salamatrix::UI::ControlOptions labelOptions;
            char labelId[140];
            _snprintf_s(
                labelId, _countof(labelId), _TRUNCATE,
                "label.%s", entry.Setting.Key);
            labelOptions.Id = labelId;
            labelOptions.Text = displayLabel.c_str();
            Salamatrix::UI::ControlLayout labelLayout = layout;
            labelLayout.Width = 150;
            if (dialog->AddControlEx(
                    Salamatrix::UI::ControlKindLabel,
                    labelOptions, labelLayout) == NULL)
            {
                valid = FALSE;
                break;
            }
            controlOptions.Id = entry.Setting.Key;
            std::vector<char> value(16385, 0);
            if (entry.Setting.Type == Salamatrix::Extensions::ExtensionSettingInteger)
            {
                LONGLONG integerValue = 0;
                if (storage->GetValueType(
                        extension.Descriptor.Id, entry.Setting.Key) ==
                    Salamatrix::Storage::StorageValueInteger)
                {
                    storage->GetInteger(
                        extension.Descriptor.Id, entry.Setting.Key, &integerValue);
                }
                _snprintf_s(
                    &value[0], value.size(), _TRUNCATE, "%lld", integerValue);
            }
            else if (storage->GetValueType(
                         extension.Descriptor.Id, entry.Setting.Key) ==
                     Salamatrix::Storage::StorageValueString)
            {
                storage->GetString(
                    extension.Descriptor.Id, entry.Setting.Key,
                    &value[0], static_cast<int>(value.size()), NULL);
            }
            controlOptions.Text = &value[0];
            controlOptions.Multiline = entry.Setting.Multiline;
            layout.X = 170;
            layout.Width = entry.Setting.Width;
            if (layout.Width < 120)
                layout.Width = 120;
            if (layout.Width > options.Width - layout.X - 12)
                layout.Width = options.Width - layout.X - 12;
            entry.Control = dialog->AddControlEx(
                Salamatrix::UI::ControlKindTextBox, controlOptions, layout);
        }
        if (entry.Control == NULL)
            valid = FALSE;
        else
            controls.push_back(entry);
    }

    Salamatrix::UI::ControlOptions okOptions;
    okOptions.Id = "ok";
    okOptions.Text = LoadStr(IDS_BUTTON_OK);
    okOptions.DialogResult = IDOK;
    Salamatrix::UI::ControlLayout okLayout;
    okLayout.HasBounds = TRUE;
    okLayout.X = 280;
    okLayout.Y = options.Height - 30;
    okLayout.Width = 65;
    okLayout.Height = 18;
    Salamatrix::UI::ControlOptions cancelOptions;
    cancelOptions.Id = "cancel";
    cancelOptions.Text = LoadStr(IDS_BUTTON_CANCEL);
    cancelOptions.DialogResult = IDCANCEL;
    Salamatrix::UI::ControlLayout cancelLayout = okLayout;
    cancelLayout.X = 355;
    if (dialog->AddControlEx(
            Salamatrix::UI::ControlKindButton, okOptions, okLayout) == NULL ||
        dialog->AddControlEx(
            Salamatrix::UI::ControlKindButton, cancelOptions, cancelLayout) == NULL)
    {
        valid = FALSE;
    }

    const int result = valid ? dialog->ShowModal() : IDCANCEL;
    if (result != IDOK)
    {
        ui->DestroyDialog(dialog);
        return FALSE;
    }

    for (size_t index = 0; index < controls.size(); ++index)
    {
        CExtensionSettingsControl& entry = controls[index];
        BOOL saved = FALSE;
        if (entry.Setting.Type == Salamatrix::Extensions::ExtensionSettingBoolean)
        {
            saved = storage->SetBoolean(
                extension.Descriptor.Id, entry.Setting.Key,
                entry.Control->GetChecked());
        }
        else
        {
            std::vector<char> value(16385, 0);
            if (!entry.Control->GetText(&value[0], static_cast<DWORD>(value.size())))
            {
                ui->DestroyDialog(dialog);
                return FALSE;
            }
            if (entry.Setting.Type == Salamatrix::Extensions::ExtensionSettingInteger)
            {
                char* end = NULL;
                errno = 0;
                const LONGLONG integerValue = _strtoi64(&value[0], &end, 10);
                while (end != NULL && (*end == ' ' || *end == '\t'))
                    ++end;
                if (errno != 0 || end == &value[0] || (end != NULL && *end != 0))
                {
                    char message[512];
                    _snprintf_s(
                        message, _countof(message), _TRUNCATE,
                        LoadStr(IDS_PLUGINEXTCONFIGINTEGER), entry.Setting.Key);
                    SalMessageBox(
                        parent, message, LoadStr(IDS_ERRORTITLE),
                        MB_OK | MB_ICONEXCLAMATION);
                    ui->DestroyDialog(dialog);
                    return FALSE;
                }
                saved = storage->SetInteger(
                    extension.Descriptor.Id, entry.Setting.Key, integerValue);
            }
            else
            {
                saved = storage->SetString(
                    extension.Descriptor.Id, entry.Setting.Key, &value[0]);
            }
        }
        if (!saved)
        {
            SalMessageBox(
                parent, LoadStr(IDS_PLUGINEXTCONFIGSAVEFAILED),
                LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
            ui->DestroyDialog(dialog);
            return FALSE;
        }
    }
    ui->DestroyDialog(dialog);
    return TRUE;
}
} // namespace

CPluginsDlg::CPluginsDlg(HWND hParent) : CCommonDialog(HLanguage, IDD_PLUGINS, IDD_PLUGINS, hParent)
{
    HListView = NULL;
    Header = NULL;
    HImageList = NULL;
    RefreshPanels = FALSE;
    DrivesBarChange = FALSE;
    FocusPlugin[0] = 0;
    Url = NULL;
    ShowInBarText[0] = 0;
    ShowInChDrvText[0] = 0;
    InstalledPluginsText[0] = 0;
    PluginTestText[0] = 0;
}

void CPluginsDlg::RefreshExtensionRows()
{
    ExtensionRows.clear();

    Salamatrix::Extensions::IExtensionsService* service = QueryExtensionService();
    if (service == NULL)
        return;
    int count = service->GetExtensionCount();
    for (int index = 0; index < count; ++index)
    {
        Salamatrix::Extensions::ExtensionInfo info;
        if (service->GetExtensionInfo(index, &info))
            ExtensionRows.push_back(info);
    }
}

void CPluginsDlg::LoadExtensionImages(HIMAGELIST imageList)
{
    ExtensionImageIndices.assign(ExtensionRows.size(), -1);
    if (imageList == NULL)
        return;

    int iconWidth = 0;
    int iconHeight = 0;
    if (!ImageList_GetIconSize(imageList, &iconWidth, &iconHeight) ||
        iconWidth <= 0 || iconWidth != iconHeight)
        return;

    const BOOL darkScheme = DarkModeIsWindowsDarkSchemeSelected();
    for (size_t index = 0; index < ExtensionRows.size(); ++index)
    {
        const Salamatrix::Extensions::ExtensionDescriptor& descriptor =
            ExtensionRows[index].Descriptor;
        const char* iconPath = descriptor.IconPath;
        const char* preferredPath = darkScheme && descriptor.IconDarkPath[0] != 0
                                        ? descriptor.IconDarkPath
                                        : descriptor.IconPath;
        if (iconPath == NULL || iconPath[0] == 0)
            continue;

        HBITMAP bitmap = NULL;
        BOOL rendered = RenderSVGIconBitmapFromFile(
            preferredPath, iconWidth, TRUE, &bitmap);
        if (!rendered && preferredPath != iconPath)
        {
            if (bitmap != NULL)
                HANDLES(DeleteObject(bitmap));
            bitmap = NULL;
            rendered = RenderSVGIconBitmapFromFile(
                iconPath, iconWidth, TRUE, &bitmap);
        }
        if (!rendered || bitmap == NULL)
            continue;
        int imageIndex = ImageList_Add(imageList, bitmap, NULL);
        HANDLES(DeleteObject(bitmap));
        if (imageIndex >= 0)
            ExtensionImageIndices[index] = imageIndex;
    }
}

void CPluginsDlg::AppendExtensionRows(BOOL setOnly)
{
    const int pluginCount = Plugins.GetCount();
    for (int index = 0; index < static_cast<int>(ExtensionRows.size()); ++index)
    {
        const int listIndex = pluginCount + index;
        const Salamatrix::Extensions::ExtensionInfo& extension = ExtensionRows[index];
        if (!setOnly)
        {
            LVITEM item;
            memset(&item, 0, sizeof(item));
            item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
            item.iItem = listIndex;
            item.iImage = index < static_cast<int>(ExtensionImageIndices.size())
                              ? ExtensionImageIndices[index]
                              : -1;
            char emptyText[] = "";
            item.pszText = emptyText;
            // Negative values identify rows that are not CPluginData records.
            item.lParam = -static_cast<LPARAM>(index + 1);
            ListView_InsertItem(HListView, &item);
        }

        if (index < static_cast<int>(ExtensionImageIndices.size()))
        {
            LVITEM imageItem;
            memset(&imageItem, 0, sizeof(imageItem));
            imageItem.mask = LVIF_IMAGE;
            imageItem.iItem = listIndex;
            imageItem.iImage = ExtensionImageIndices[index];
            ListView_SetItem(HListView, &imageItem);
        }

        ListView_SetItemText(
            HListView, listIndex, 0,
            const_cast<char*>(extension.Descriptor.Name));
        ListView_SetItemText(
            HListView, listIndex, 1,
            LoadStr(extension.State == Salamatrix::Extensions::ExtensionStateActive
                         ? IDS_PLUGINS_LOADED_YES
                         : IDS_PLUGINS_LOADED_NO));
        ListView_SetItemText(
            HListView, listIndex, 2,
            const_cast<char*>(extension.Descriptor.Version));
        ListView_SetItemText(
            HListView, listIndex, 3,
            const_cast<char*>(extension.Descriptor.EntryPoint));
    }
}

void CPluginsDlg::ApplyTheme()
{
    if (HListView == NULL)
        return;

    if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
    {
        DarkModeApplyTree(HWindow);
        DarkModeRefreshTitleBar(HWindow);
        DarkModeApplyStaticTextColors(HWindow, NULL);
        WinLib_DarkMode_PostDeferredRedraw(HWindow);
    }

    const bool useDark = ShouldUsePluginsDarkPalette();
    const COLORREF text = useDark ? DarkModeGetColors().readableText : GetSysColor(COLOR_WINDOWTEXT);
    const COLORREF background = useDark ? DarkModeGetColors().background : GetSysColor(COLOR_WINDOW);

    DarkModeUpdateListViewColors(HListView, text, background, useDark);

    if (Header != NULL && Header->HWindow != NULL)
    {
        DarkModeApplyWindow(Header->HWindow);
        InvalidateRect(Header->HWindow, NULL, TRUE);
    }
}

void CPluginsDlg::InitColumns()
{
    CALL_STACK_MESSAGE1("CPluginsDlg::InitColumns()");
    LV_COLUMN lvc;
    int header[4] = {IDS_PLUGINS_NAME, IDS_PLUGINS_LOADED, IDS_PLUGINS_VERSION, IDS_PLUGINS_LOCATION};

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    int i;
    for (i = 0; i < 4; i++) // create columns
    {
        lvc.pszText = LoadStr(header[i]);
        lvc.iSubItem = i;
        ListView_InsertColumn(HListView, i, &lvc);
        //    ListView_SetColumnWidth(HListView, i, LVSCW_AUTOSIZE_USEHEADER);   // widths will be set later in SetColumnWidths()
    }
}

void CPluginsDlg::SetColumnWidths()
{
    // set optimal widths for all columns
    int i;
    for (i = 0; i <= 3; i++)
        ListView_SetColumnWidth(HListView, i, i < 3 ? LVSCW_AUTOSIZE_USEHEADER : LVSCW_AUTOSIZE); // the last column shouldn’t stretch to fill the view; its content is wider than the header.

    // sum the widths of columns we want to show completely (everything after name, which we may shorten)
    int nameWidth = ListView_GetColumnWidth(HListView, 0);
    int otherWidths = 0 + GetSystemMetrics(SM_CXHSCROLL);
    for (i = 1; i < 4; i++)
        otherWidths += ListView_GetColumnWidth(HListView, i);

    //  SCROLLBARINFO si; // would need to be loaded dynamically, we won't bother
    //  si.cbSize = sizeof(si);
    //  GetScrollBarInfo(HListView, OBJID_VSCROLL, &si);

    RECT r;
    GetClientRect(HListView, &r);
    int lvWidth = r.right - r.left;
    if (nameWidth + otherWidths < lvWidth + 10 + 10)
    {
        // if space allows, enlarge the Loaded and Version columns by 10px (looks better)
        for (i = 1; i <= 2; i++)
            ListView_SetColumnWidth(HListView, i, ListView_GetColumnWidth(HListView, i) + 10);
        otherWidths += 10 + 10;
    }

    // the supplementary column will be the first one -- Name but only if the total width is not too large
    if (nameWidth + otherWidths < lvWidth)
        ListView_SetColumnWidth(HListView, 0, lvWidth - otherWidths);
}

void CPluginsDlg::RefreshListView(BOOL setOnly, int selIndex, const CPluginData* selectPlugin, BOOL setColumnWidths)
{
    SendMessage(HListView, WM_SETREDRAW, FALSE, 0);

    const int previousExtensionCount = static_cast<int>(ExtensionRows.size());
    RefreshExtensionRows();
    // A changed extension count changes the list-view row layout, so rebuild
    // the rows instead of attempting an in-place update with stale indexes.
    if (setOnly && previousExtensionCount != static_cast<int>(ExtensionRows.size()))
        setOnly = FALSE;

    HIMAGELIST hIcons = Plugins.CreateIconsList(FALSE); // destruction is handled by the listview
    HIMAGELIST hOldIcons = ListView_SetImageList(HListView, hIcons, LVSIL_SMALL);
    if (hOldIcons != NULL)
        ImageList_Destroy(hOldIcons);
    LoadExtensionImages(hIcons);

    int numOfLoaded = 0;
    Plugins.AddNamesToListView(HListView, setOnly, &numOfLoaded);
    AppendExtensionRows(setOnly);

    for (size_t index = 0; index < ExtensionRows.size(); ++index)
        if (ExtensionRows[index].State == Salamatrix::Extensions::ExtensionStateActive)
            ++numOfLoaded;

    if (Header != NULL)
    {
        char buf[300];
        sprintf(buf, InstalledPluginsText,
                Plugins.GetCount() + static_cast<int>(ExtensionRows.size()),
                numOfLoaded);
        SendMessage(Header->HWindow, WM_SETREDRAW, FALSE, 0);
        SetWindowText(Header->HWindow, buf);
        SendMessage(Header->HWindow, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(Header->HWindow, NULL, TRUE);
    }

    if (setColumnWidths)
        SetColumnWidths(); // set column widths

    // Petr: when this line was at the end of the function (which would be more logical)
    // the focused item remained hidden under the bottom scrollbar and was not fully visible,
    // apparently Windows needs a redraw first so the scrollbar is displayed and accounted for
    SendMessage(HListView, WM_SETREDRAW, TRUE, 0);

    if (!setOnly)
    {
        int count = ListView_GetItemCount(HListView);
        if (count > 0)
        {
            if (selectPlugin != NULL)
                selIndex = Plugins.GetPluginOrderIndex(selectPlugin);
            if (selIndex >= count)
                selIndex = count - 1;
            if (selIndex < 0)
                selIndex = 0;
            DWORD state = LVIS_SELECTED | LVIS_FOCUSED;
            ListView_SetItemState(HListView, selIndex, state, state);
            ListView_EnsureVisible(HListView, selIndex, FALSE);
        }
        else
            OnSelChanged(); // force empty items
    }
    else
    {
        int count = ListView_GetItemCount(HListView);
        if (count > 0 && selIndex != -1)
        {
            DWORD state = LVIS_SELECTED | LVIS_FOCUSED;
            ListView_SetItemState(HListView, selIndex, state, state);
            ListView_EnsureVisible(HListView, selIndex, FALSE);
        }
        OnSelChanged(); // force enable state update
    }

    ApplyTheme();
}

void CPluginsDlg::EnableButtons(CPluginData* plugin)
{
    Salamatrix::Extensions::ExtensionInfo* extension =
        plugin == NULL ? GetSelectedExtension() : NULL;
    const BOOL extensionActionable =
        extension != NULL &&
        extension->State != Salamatrix::Extensions::ExtensionStateActivating &&
        extension->State != Salamatrix::Extensions::ExtensionStateDeactivating;
    Salamatrix::Extensions::IExtensionsService* extensionService =
        extension != NULL ? QueryExtensionService() : NULL;
    const BOOL extensionConfigurable =
        extension != NULL && extensionService != NULL &&
        extensionService->GetExtensionSettingCount(
            extension->Descriptor.Id) > 0;

    if (extension != NULL)
    {
        SetWindowText(
            GetDlgItem(HWindow, IDB_PLUGINTEST),
            LoadStr(extension->State == Salamatrix::Extensions::ExtensionStateActive
                        ? IDS_PLUGINEXTDEACTIVATE
                        : IDS_PLUGINEXTACTIVATE));
    }
    else if (PluginTestText[0] != 0)
        SetWindowText(GetDlgItem(HWindow, IDB_PLUGINTEST), PluginTestText);

    HWND focus = GetFocus();
    BOOL changeFocus = FALSE;
    if (GetDlgItem(HWindow, IDB_PLUGINREMOVE) == focus && plugin == NULL)
        changeFocus = TRUE;
    EnableWindow(GetDlgItem(HWindow, IDB_PLUGINREMOVE), plugin != NULL);
    if (GetDlgItem(HWindow, IDB_PLUGINTEST) == focus &&
        plugin == NULL && !extensionActionable)
        changeFocus = TRUE;
    EnableWindow(GetDlgItem(HWindow, IDB_PLUGINTEST), plugin != NULL || extensionActionable);
    if (GetDlgItem(HWindow, IDB_PLUGINTESTALL) == focus && plugin == NULL)
        changeFocus = TRUE;
    EnableWindow(GetDlgItem(HWindow, IDB_PLUGINTESTALL), plugin != NULL);
    if (GetDlgItem(HWindow, IDB_PLUGINABOUT) == focus && plugin == NULL)
        changeFocus = TRUE;
    EnableWindow(GetDlgItem(HWindow, IDB_PLUGINFOCUS), plugin != NULL);
    if (GetDlgItem(HWindow, IDB_PLUGINFOCUS) == focus && plugin == NULL)
        changeFocus = TRUE;
    EnableWindow(GetDlgItem(HWindow, IDB_PLUGINABOUT), plugin != NULL);
    if (GetDlgItem(HWindow, IDB_PLUGINUNLOAD) == focus &&
        (plugin == NULL || !plugin->GetLoaded()))
        changeFocus = TRUE;
    EnableWindow(GetDlgItem(HWindow, IDB_PLUGINUNLOAD), plugin != NULL && plugin->GetLoaded());
    if (GetDlgItem(HWindow, IDB_PLUGINCONFIG) == focus &&
        ((plugin != NULL && !plugin->SupportConfiguration) ||
         (plugin == NULL && !extensionConfigurable)))
        changeFocus = TRUE;
    EnableWindow(
        GetDlgItem(HWindow, IDB_PLUGINCONFIG),
        (plugin != NULL && plugin->SupportConfiguration) ||
            extensionConfigurable);
    if (GetDlgItem(HWindow, IDB_PLUGINKEYS) == focus &&
        (plugin == NULL || plugin->MenuItems.Count == 0 && !plugin->SupportDynMenuExt))
        changeFocus = TRUE;
    EnableWindow(GetDlgItem(HWindow, IDB_PLUGINKEYS), plugin != NULL && (plugin->MenuItems.Count > 0 || plugin->SupportDynMenuExt));

    HWND updates = GetDlgItem(HWindow, IDB_PLUGINUPDATES);
    BOOL updatesVisible = GetLoadedSamandarinUpdateNotifier(NULL);
    if (updates == focus && !updatesVisible)
        changeFocus = TRUE;
    ShowWindow(updates, updatesVisible ? SW_SHOW : SW_HIDE);
    EnableWindow(updates, updatesVisible);

    if (changeFocus)
    {
        PostMessage(focus, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
        focus = GetDlgItem(HWindow, IDOK);
        PostMessage(focus, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
        SetFocus(focus);
    }
}

void CPluginsDlg::EnableHeader()
{
    DWORD mask = TLBHDRMASK_SORT;
    int index = ListView_GetNextItem(HListView, -1, LVNI_FOCUSED);
    int count = ListView_GetItemCount(HListView);
    if (index > 0)
        mask |= TLBHDRMASK_UP;
    if (index < count - 1)
        mask |= TLBHDRMASK_DOWN;
    Header->EnableToolbar(mask);
}

void CPluginsDlg::OnSelChanged()
{
    CPluginData* p = GetSelectedPlugin();
    Salamatrix::Extensions::ExtensionInfo* extension =
        p == NULL ? GetSelectedExtension() : NULL;
    HWND showInBar = GetDlgItem(HWindow, IDC_PLUGINSHOWINBAR);
    HWND showInChDrv = GetDlgItem(HWindow, IDC_PLUGINSHOWINCHDRV);
    if (p != NULL)
    {
        SetWindowText(GetDlgItem(HWindow, IDC_STATIC_7),
                      LoadStr(IDS_PLUGIN_ARCHIVES_LABEL));
        if (p->DLLName != NULL)
            lstrcpyn(LastSelectedPluginDLLName, p->DLLName, MAX_PATH);
        else
            LastSelectedPluginDLLName[0] = 0;

        if (!IsWindowVisible(showInBar))
            ShowWindow(showInBar, SW_SHOW);
        if (!IsWindowVisible(showInChDrv))
            ShowWindow(showInChDrv, SW_SHOW);

        // description
        SetPluginManagerText(GetDlgItem(HWindow, IDC_PLUGINDESCRIPTION), p->Description);
        // copyright
        SetPluginManagerText(GetDlgItem(HWindow, IDC_PLUGINCOPYRIGHT), p->Copyright);
        // www
        SetPluginManagerText(GetDlgItem(HWindow, IDC_PLUGINWWW),
                             p->PluginHomePageURL != NULL ? p->PluginHomePageURL : LoadStr(IDS_PLUGINURLNONE));
        Url->SetActionOpen(p->PluginHomePageURL != NULL ? p->PluginHomePageURL : "");
        // extension
        SetPluginManagerText(GetDlgItem(HWindow, IDC_PLUGINEXTENSIONS),
                             p->Extensions[0] == 0 ? LoadStr(IDS_PLUGINEXTNONE) : p->Extensions);
        // FS Name
        char buf[500];
        buf[0] = 0;
        int remainingSize = sizeof(buf); // store the list of FS names in 'buf', names will be separated by ';'
        int i;
        for (i = 0; remainingSize > 1 && i < p->FSNames.Count; i++)
        {
            _snprintf_s(buf + (sizeof(buf) - remainingSize), remainingSize, _TRUNCATE,
                        (i + 1 != p->FSNames.Count) ? "%s;" : "%s", p->FSNames[i]);
            remainingSize = (int)sizeof(buf) - (int)strlen(buf);
        }
        SetPluginManagerText(GetDlgItem(HWindow, IDC_PLUGINFSNAME),
                             buf[0] == 0 ? LoadStr(IDS_PLUGINFSNONE) : buf);
        // Functions
        // Salamatrix may already be installed in user configurations saved before
        // Preserve older configurations; identify Salamatrix by its stable registry key too.
        BOOL isSalamatrixProvider =
            HasStablePluginKey(p->RegKeyName, "SALAMATRIX") ||
            IsPluginName(p->Name, "Salamatrix Framework");
        BOOL isExtensionRuntime =
            HasStablePluginKey(p->RegKeyName, "JAVASCRIPT.RUNTIME") ||
            HasStablePluginKey(p->RegKeyName, "PHP.RUNTIME") ||
            HasStablePluginKey(p->RegKeyName, "LUA.RUNTIME") ||
            HasStablePluginKey(p->RegKeyName, "POWERSHELL.RUNTIME") ||
            HasStablePluginKey(p->RegKeyName, "PYTHON.RUNTIME") ||
            IsPluginName(p->Name, "JavaScript Runtime") ||
            IsPluginName(p->Name, "PHP Runtime") ||
            IsPluginName(p->Name, "Lua Runtime") ||
            IsPluginName(p->Name, "PowerShell Runtime") ||
            IsPluginName(p->Name, "Python Runtime");
        BOOL isExtensionHelper =
            HasStablePluginKey(p->RegKeyName, "SALAMATRIX.AI") ||
            IsPluginName(p->Name, "Salamatrix AI");
        BOOL isLocalAIModel =
            IsPluginName(p->Name, "Salamatrix AI Local LLaMA");
        BOOL supportAutomationFramework = p->SupportAutomationFramework || isSalamatrixProvider ||
                                          isExtensionRuntime || isExtensionHelper;
        buf[0] = 0;
        if (p->SupportPanelView)
            strcat(buf, LoadStr(IDS_PLUGINFUNCVIEW));
        if (p->SupportPanelEdit)
        {
            if (buf[0] != 0)
                strcat(buf, ",\n");
            strcat(buf, LoadStr(IDS_PLUGINFUNCEDIT));
        }
        if (p->SupportCustomPack)
        {
            if (buf[0] != 0)
                strcat(buf, ",\n");
            strcat(buf, LoadStr(IDS_PLUGINFUNCCUSTPACK));
        }
        if (p->SupportCustomUnpack)
        {
            if (p->SupportCustomPack)
                strcat(buf, ", "); // text of custom packer is shorter - same line
            else
            {
                if (buf[0] != 0)
                    strcat(buf, ",\n");
            }
            strcat(buf, LoadStr(IDS_PLUGINFUNCCUSTUNPACK));
        }
        if (p->SupportViewer)
        {
            if (buf[0] != 0)
                strcat(buf, ",\n");
            strcat(buf, LoadStr(IDS_PLUGINFUNCFILEVIEWER));
        }
        if (p->MenuItems.Count > 0 || p->SupportDynMenuExt)
        {
            if (p->SupportViewer)
                strcat(buf, ", "); // viewer text is shorter - same line
            else
            {
                if (buf[0] != 0)
                    strcat(buf, ",\n");
            }
            strcat(buf, LoadStr(IDS_PLUGINFUNCMENUEXTENSION));
        }

        if (p->SupportFS)
        {
            // viewer+menu text is shorter - same line
            if (p->SupportViewer || p->MenuItems.Count > 0 || p->SupportDynMenuExt)
                strcat(buf, ", ");
            else
            {
                if (buf[0] != 0)
                    strcat(buf, ",\n");
            }
            strcat(buf, LoadStr(IDS_PLUGINFUNCFILESYSTEM));
        }

        if (supportAutomationFramework)
        {
            if (p->SupportViewer || p->MenuItems.Count > 0 || p->SupportDynMenuExt || p->SupportFS)
                strcat(buf, ", ");
            else
            {
                if (buf[0] != 0)
                    strcat(buf, ",\n");
            }
            strcat(buf, LoadStr(
                isSalamatrixProvider
                    ? IDS_PLUGINFUNCEXTENSIONFRAMEWORK
                    : isLocalAIModel
                          ? IDS_PLUGINFUNCLOCALAIMODEL
                    : isExtensionRuntime
                          ? IDS_PLUGINFUNCEXTENSIONRUNTIME
                          : isExtensionHelper
                                ? IDS_PLUGINFUNCEXTENSIONHELPER
                          : IDS_PLUGINFUNCAUTORUNTIME));
        }

        // Thumbnails
        if (p->ThumbnailMasks.GetMasksString()[0] != 0)
        {
            if (p->SupportViewer || p->MenuItems.Count > 0 || p->SupportDynMenuExt || p->SupportFS || supportAutomationFramework)
                strcat(buf, ", ");
            else
            {
                if (buf[0] != 0)
                    strcat(buf, ",\n");
            }
            strcat(buf, LoadStr(IDS_PLUGINFUNCTHUMBLOADER));
            SetPluginManagerText(GetDlgItem(HWindow, IDC_PLUGINTHUMBNAILS), p->ThumbnailMasks.GetMasksString());
        }
        else
            SetPluginManagerText(GetDlgItem(HWindow, IDC_PLUGINTHUMBNAILS), LoadStr(IDS_PLUGINTHUMBNONE));

        SetPluginManagerText(GetDlgItem(HWindow, IDC_PLUGINFUNCTIONS), buf);

        char buff[MAX_PATH + 200];
        char pluginName[300];
        lstrcpyn(pluginName, p->Name, 299);
        DuplicateAmpersands(pluginName, 299); // plugin name may contain the '&' character
        sprintf(buff, ShowInBarText, pluginName);
        SetWindowText(showInBar, buff);

        char fsItemText[200];
        const char* itemText;
        if (p->ChDrvMenuFSItemName != NULL)
        {
            char* s = p->ChDrvMenuFSItemName;
            while (*s != 0 && *s != '\t')
                s++;
            if (*s == 0)
                itemText = p->ChDrvMenuFSItemName;
            else // there is at least one tab character
            {
                lstrcpyn(fsItemText, s + 1, 200);
                itemText = fsItemText;
                s = fsItemText;
                while (*s != 0 && *s != '\t')
                    s++;
                if (*s == '\t')
                    *s = 0;
            }
        }
        else
            itemText = "FS";

        sprintf(buff, ShowInChDrvText, itemText);
        SetWindowText(showInChDrv, buff);

        int orderIndex = ListView_GetNextItem(HListView, -1, LVIS_FOCUSED);
        if (orderIndex != -1)
        {
            BOOL hasMenu = p->MenuItems.Count > 0 || p->SupportDynMenuExt;
            BOOL showInBar2 = Plugins.GetShowInBar(Plugins.GetIndexByOrder(orderIndex));
            if (!hasMenu)
                showInBar2 = FALSE;
            CheckDlgButton(HWindow, IDC_PLUGINSHOWINBAR, showInBar2 ? BST_CHECKED : BST_UNCHECKED);
            EnableWindow(GetDlgItem(HWindow, IDC_PLUGINSHOWINBAR), hasMenu);

            BOOL hasItem = p->ChDrvMenuFSItemName != NULL;
            BOOL showInChDrv2 = Plugins.GetShowInChDrv(Plugins.GetIndexByOrder(orderIndex));
            if (!hasItem)
                showInChDrv2 = FALSE;
            CheckDlgButton(HWindow, IDC_PLUGINSHOWINCHDRV, showInChDrv2 ? BST_CHECKED : BST_UNCHECKED);
            EnableWindow(GetDlgItem(HWindow, IDC_PLUGINSHOWINCHDRV), hasItem);
        }

        EnableHeader();

        EnableButtons(p);
    }
    else if (extension != NULL)
    {
        SetWindowText(GetDlgItem(HWindow, IDC_STATIC_7),
                      LoadStr(IDS_PLUGIN_RUNTIME_LABEL));
        // Manifest extensions are informational rows in Plugin Manager. They
        // are not CPluginData records, so load/unload/configuration actions
        // remain disabled and no fake .SPL path is presented to the user.
        SetPluginManagerText(
            GetDlgItem(HWindow, IDC_PLUGINDESCRIPTION),
            (extension->Descriptor.Flags &
             Salamatrix::Extensions::ExtensionFlagPackage) != 0
                ? "Salamatrix extension package"
                : "Manifest extension");
        SetPluginManagerText(
            GetDlgItem(HWindow, IDC_PLUGINCOPYRIGHT),
            extension->Descriptor.Id);
        SetPluginManagerText(GetDlgItem(HWindow, IDC_PLUGINWWW), "");
        Url->SetActionOpen("");
        const BOOL dependencyUnavailable =
            (extension->Descriptor.Flags &
             Salamatrix::Extensions::ExtensionFlagDependencyUnavailable) != 0;
        const BOOL disabled =
            (extension->Descriptor.Flags &
             Salamatrix::Extensions::ExtensionFlagDisabled) != 0;
        const BOOL runtimeUnavailable =
            (extension->Descriptor.Flags &
             Salamatrix::Extensions::ExtensionFlagRuntimeUnavailable) != 0;
        const BOOL runtimeExecutableUnavailable =
            (extension->Descriptor.Flags &
             Salamatrix::Extensions::ExtensionFlagRuntimeExecutableUnavailable) != 0;
        char runtimeText[256];
        _snprintf_s(
            runtimeText,
            _countof(runtimeText),
            _TRUNCATE,
            "%s%s",
            extension->Descriptor.RuntimeId,
            disabled
                ? " (disabled)"
                : runtimeUnavailable
                ? " (runtime unavailable)"
                : runtimeExecutableUnavailable
                ? " (runtime executable unavailable)"
                : dependencyUnavailable
                      ? " (dependency unavailable)"
                : "");
        SetPluginManagerText(
            GetDlgItem(HWindow, IDC_PLUGINEXTENSIONS), runtimeText);
        SetPluginManagerText(GetDlgItem(HWindow, IDC_PLUGINFSNAME), "");
        SetPluginManagerText(
            GetDlgItem(HWindow, IDC_PLUGINTHUMBNAILS),
            LoadStr(IDS_PLUGINTHUMBNONE));
        SetPluginManagerText(
            GetDlgItem(HWindow, IDC_PLUGINFUNCTIONS),
            disabled
                ? LoadStr(IDS_PLUGINEXTDISABLED)
                : dependencyUnavailable
                ? LoadStr(IDS_PLUGINEXTWAITINGDEPENDENCY)
                : runtimeUnavailable
                ? LoadStr(IDS_PLUGINEXTWAITINGRUNTIME)
                : runtimeExecutableUnavailable
                      ? LoadStr(IDS_PLUGINEXTWAITINGEXECUTABLE)
                : "Extension");

        char extensionName[300];
        char extensionBarText[500];
        lstrcpyn(extensionName, extension->Descriptor.Name,
                 _countof(extensionName));
        DuplicateAmpersands(extensionName, _countof(extensionName));
        const char* extensionBarFormat =
            LoadStr(IDS_PLUGIN_SHOWINEXTENSIONBAR);
        const char* extensionNamePlaceholder =
            strstr(extensionBarFormat, "%s");
        if (extensionNamePlaceholder != NULL)
        {
            _snprintf_s(
                extensionBarText, _countof(extensionBarText), _TRUNCATE,
                "%.*s%s%s",
                static_cast<int>(extensionNamePlaceholder -
                                 extensionBarFormat),
                extensionBarFormat, extensionName,
                extensionNamePlaceholder + 2);
        }
        else
            lstrcpyn(extensionBarText, extensionBarFormat,
                     _countof(extensionBarText));
        SetWindowText(showInBar, extensionBarText);
        ShowWindow(showInBar, SW_SHOW);
        ShowWindow(showInChDrv, SW_HIDE);
        const BOOL hasExtensionBarButton =
            Plugins.HasExtensionBarButton(extension->Descriptor.Id);
        CheckDlgButton(
            HWindow, IDC_PLUGINSHOWINBAR,
            hasExtensionBarButton &&
                    Plugins.GetExtensionBarVisible(extension->Descriptor.Id)
                ? BST_CHECKED
                : BST_UNCHECKED);
        EnableWindow(showInBar, hasExtensionBarButton);
        EnableWindow(showInChDrv, FALSE);
        EnableButtons(NULL);
    }
    else
    {
        SetWindowText(GetDlgItem(HWindow, IDC_STATIC_7),
                      LoadStr(IDS_PLUGIN_ARCHIVES_LABEL));
        SetWindowText(GetDlgItem(HWindow, IDC_PLUGINDESCRIPTION), "");
        SetWindowText(GetDlgItem(HWindow, IDC_PLUGINCOPYRIGHT), "");
        SetWindowText(GetDlgItem(HWindow, IDC_PLUGINWWW), "");
        Url->SetActionOpen("");
        SetWindowText(GetDlgItem(HWindow, IDC_PLUGINEXTENSIONS), "");
        SetWindowText(GetDlgItem(HWindow, IDC_PLUGINFSNAME), "");
        SetWindowText(GetDlgItem(HWindow, IDC_PLUGINTHUMBNAILS), "");
        SetWindowText(GetDlgItem(HWindow, IDC_PLUGINFUNCTIONS), "");
        /*if (IsWindowVisible(showInBar))*/ ShowWindow(showInBar, SW_HIDE);     // condition commented out because it misbehaves during WM_INITDIALOG (the dialog is not visible as a whole -> the check fails)
        /*if (IsWindowVisible(showInChDrv))*/ ShowWindow(showInChDrv, SW_HIDE); // condition commented out because it misbehaves during WM_INITDIALOG (the dialog is not visible as a whole -> the check fails)
        EnableButtons(NULL);
    }
}

CPluginData*
CPluginsDlg::GetSelectedPlugin(int* index, int* lvIndex)
{
    int i = ListView_GetNextItem(HListView, -1, LVIS_FOCUSED);
    if (i == -1)
        return NULL;

    if (i >= Plugins.GetCount())
        return NULL;

    int orderIndex = Plugins.GetIndexByOrder(i);
    CPluginData* plugin = Plugins.Get(orderIndex);
    if (plugin != NULL && index != NULL)
    {
        *index = orderIndex;
        if (lvIndex != NULL)
            *lvIndex = i;
    }
    return plugin;
}

Salamatrix::Extensions::ExtensionInfo*
CPluginsDlg::GetSelectedExtension()
{
    int listIndex = ListView_GetNextItem(HListView, -1, LVIS_FOCUSED);
    int extensionIndex = listIndex - Plugins.GetCount();
    if (listIndex < 0 || extensionIndex < 0 ||
        extensionIndex >= static_cast<int>(ExtensionRows.size()))
        return NULL;
    return &ExtensionRows[extensionIndex];
}

void CPluginsDlg::OnContextMenu(int x, int y)
{
    HWND hListView = HListView;
    DWORD selCount = ListView_GetSelectedCount(hListView);
    if (selCount < 1)
        return;

    // fetch button texts and fill the context menu
    int ids[] = {-2, IDB_PLUGINCONFIG, // -2 -> next item will be default
                 IDB_PLUGINKEYS,
                 IDB_PLUGINABOUT,
                 IDB_PLUGINTEST,
                 IDB_PLUGINUNLOAD,
                 IDB_PLUGINFOCUS,
                 IDB_PLUGINREMOVE,
                 //               -1,                      // -1 -> separator
                 0}; // 0 -> terminator
    CMenuPopup popup;
    FillContextMenuFromButtons(&popup, HWindow, ids);

    DWORD cmd = popup.Track(MENU_TRACK_RETURNCMD | MENU_TRACK_RIGHTBUTTON,
                            x, y, HWindow, NULL);
    if (cmd != 0)
        PostMessage(HWindow, WM_COMMAND, cmd, 0);
}

void CPluginsDlg::OnMove(BOOL up)
{
    int index = ListView_GetNextItem(HListView, -1, LVIS_FOCUSED);
    if (index < 0 || index >= Plugins.GetCount())
        return; // manifest-extension rows have no persisted plugin order
    int newIndex = up ? index - 1 : index + 1;
    if (Plugins.ChangePluginsOrder(index, newIndex))
    {
        if (Configuration.KeepPluginsSorted)
            OnSort();
        Plugins.UpdatePluginsOrder(FALSE);
        RefreshListView(TRUE, newIndex);
    }
}

void CPluginsDlg::OnSort()
{
    Configuration.KeepPluginsSorted = !Configuration.KeepPluginsSorted;
    Header->CheckToolbar(Configuration.KeepPluginsSorted ? TLBHDRMASK_SORT : 0);
    if (Configuration.KeepPluginsSorted)
    {
        CPluginData* selectedPlugin = GetSelectedPlugin();
        Plugins.UpdatePluginsOrder(TRUE);
        RefreshListView(FALSE, -1, selectedPlugin);
    }
}

/*
void
CPluginsDlg::OnDrag()
{
  LVHITTESTINFO ht;
  DWORD pos = GetMessagePos();
  ht.pt.x = GET_X_LPARAM(pos);
  ht.pt.y = GET_Y_LPARAM(pos);
  ScreenToClient(HListView, &ht.pt);
  ListView_HitTest(HListView, &ht);
  int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
  if (ht.iItem != -1 && ht.iItem != index)
  {

  }
}
*/

INT_PTR
CPluginsDlg::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SLOW_CALL_STACK_MESSAGE4("CPluginsDlg::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // copy the Show In Bar checkbox text into our buffer
        GetDlgItemText(HWindow, IDC_PLUGINSHOWINBAR, ShowInBarText, 200);

        // The same button is reused as Activate/Deactivate for manifest
        // extensions; keep the localized native-plugin label for restoration.
        GetDlgItemText(HWindow, IDB_PLUGINTEST, PluginTestText, sizeof(PluginTestText));

        // copy the Show In Change Drive Menu checkbox text into our buffer
        GetDlgItemText(HWindow, IDC_PLUGINSHOWINCHDRV, ShowInChDrvText, 200);

        // copy the "Installed Plugins:" text into our buffer
        GetDlgItemText(HWindow, IDC_PLUGINHEADER, InstalledPluginsText, 200);

        SetWindowText(GetDlgItem(HWindow, IDB_PLUGINUPDATES), LoadStr(IDS_PLUGINUPDATES));

        // Add will have a drop-down
        // new CButton(HWindow, IDB_PLUGINADD, BTF_DROPDOWN);

        // www will be a hyperlink
        Url = new CHyperLink(HWindow, IDC_PLUGINWWW);
        if (Url == NULL)
            TRACE_E(LOW_MEMORY);

        // listview setup
        HListView = GetDlgItem(HWindow, IDL_PLUGINS);
        ListView_SetExtendedListViewStyleEx(HListView, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

        // header line
        Header = new CToolbarHeader(HWindow, IDC_PLUGINHEADER, HListView,
                                    TLBHDRMASK_SORT | TLBHDRMASK_UP | TLBHDRMASK_DOWN);
        if (Header == NULL)
            TRACE_E(LOW_MEMORY);

        Header->CheckToolbar(Configuration.KeepPluginsSorted ? TLBHDRMASK_SORT : 0);

        // insert columns
        InitColumns();

        // for convenience select the last chosen item (if it exists)
        CPluginData* lastSelectPluginData = NULL;
        int lastSelectedPluginIndex = 0;
        if (Plugins.FindDLL(LastSelectedPluginDLLName, lastSelectedPluginIndex))
            lastSelectPluginData = Plugins.Get(lastSelectedPluginIndex);

        // insert items
        RefreshListView(FALSE, 0, lastSelectPluginData, TRUE);

        ApplyTheme();

        break;
    }

    case WM_DESTROY:
    {
        MainWindow->OnPluginsStateChanged(); // maybe this should have a Dirty flag
        break;
    }

    case WM_NOTIFY:
    {
        if (wParam == IDL_PLUGINS)
        {
            LPNMHDR nmh = (LPNMHDR)lParam;
            switch (nmh->code)
            {
            case NM_DBLCLK:
            {
                PostMessage(HWindow, WM_COMMAND, IDB_PLUGINCONFIG, 0);
                break;
            }

            case NM_RCLICK:
            {
                DWORD pos = GetMessagePos();
                OnContextMenu(GET_X_LPARAM(pos), GET_Y_LPARAM(pos));
                return 0;
            }

            case LVN_KEYDOWN:
            {
                LPNMLVKEYDOWN nmhk = (LPNMLVKEYDOWN)nmh;
                BOOL shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                if (shiftPressed && nmhk->wVKey == VK_F10 || nmhk->wVKey == VK_APPS)
                {
                    POINT p;
                    GetListViewContextMenuPos(HListView, &p);
                    OnContextMenu(p.x, p.y);
                }
                if (nmhk->wVKey == VK_INSERT)
                    PostMessage(HWindow, WM_COMMAND, IDB_PLUGINADD, 0);
                if (nmhk->wVKey == VK_DELETE)
                    PostMessage(HWindow, WM_COMMAND, IDB_PLUGINREMOVE, 0);
                if ((GetKeyState(VK_MENU) & 0x8000) &&
                    (nmhk->wVKey == VK_UP || nmhk->wVKey == VK_DOWN))
                {
                    OnMove(nmhk->wVKey == VK_UP);
                }
                return 0;
            }

            case LVN_ITEMCHANGED:
            {
                LPNMLISTVIEW nmhi = (LPNMLISTVIEW)nmh;
                if (!(nmhi->uOldState & LVIS_SELECTED) && nmhi->uNewState & LVIS_SELECTED)
                {
                    OnSelChanged();
                }
                return 0;
            }

                //case LVN_BEGINDRAG:
                //{
                //  OnDrag();
                //  return 0;
                //}
            }
        }
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_PLUGINHEADER:
        {
            if (GetFocus() != HListView)
                SetFocus(HListView);
            switch (HIWORD(wParam))
            {
            case TLBHDR_UP:
                OnMove(TRUE);
                break;
            case TLBHDR_DOWN:
                OnMove(FALSE);
                break;
            case TLBHDR_SORT:
                if (!Configuration.KeepPluginsSorted)
                    OnSort();
                break;
            }
            break;
        }

        case IDC_PLUGINSHOWINBAR:
        {
            CPluginData* selectedPlugin = GetSelectedPlugin();
            if (selectedPlugin != NULL)
            {
                int index = Plugins.GetIndexByOrder(
                    ListView_GetNextItem(HListView, -1, LVIS_FOCUSED));
                if (index != -1)
                {
                    BOOL showInBar =
                        IsDlgButtonChecked(HWindow, IDC_PLUGINSHOWINBAR) ==
                        BST_CHECKED;
                    Plugins.SetShowInBar(index, showInBar);
                }
            }
            else
            {
                Salamatrix::Extensions::ExtensionInfo* extension =
                    GetSelectedExtension();
                if (extension != NULL)
                {
                    Plugins.SetExtensionBarVisible(
                        extension->Descriptor.Id,
                        IsDlgButtonChecked(HWindow, IDC_PLUGINSHOWINBAR) ==
                            BST_CHECKED);
                }
            }
            break;
        }

        case IDC_PLUGINSHOWINCHDRV:
        {
            if (GetSelectedPlugin() == NULL)
                break;
            int index = Plugins.GetIndexByOrder(ListView_GetNextItem(HListView, -1, LVIS_FOCUSED));
            if (index != -1)
            {
                DrivesBarChange = TRUE;
                BOOL showInChDrv = IsDlgButtonChecked(HWindow, IDC_PLUGINSHOWINCHDRV) == BST_CHECKED;
                Plugins.SetShowInChDrv(index, showInChDrv);
            }
            break;
        }

        case IDB_PLUGINABOUT:
        {
            RefreshPanels = TRUE;
            CPluginData* p = GetSelectedPlugin();
            if (p != NULL)
            {
                p->About(HWindow);
                RefreshListView(); // a DLL was loaded, we have fresher data ...
            }
            return 0;
        }

        case IDB_PLUGINCONFIG:
        {
            RefreshPanels = TRUE;
            CPluginData* p = GetSelectedPlugin();
            if (p != NULL && p->SupportConfiguration)
            {
                p->Configuration(HWindow);
                RefreshListView(); // a DLL was loaded, we have fresher data ...
            }
            else
            {
                Salamatrix::Extensions::ExtensionInfo* extension =
                    GetSelectedExtension();
                Salamatrix::Extensions::IExtensionsService* extensions =
                    QueryExtensionService();
                Salamatrix::Storage::IStorageService* storage =
                    QueryExtensionStorageService();
                Salamatrix::UI::IUIService* ui = QueryExtensionUIService();
                if (extension != NULL && extensions != NULL &&
                    extensions->GetExtensionSettingCount(
                        extension->Descriptor.Id) > 0)
                {
                    ConfigureManifestExtensionSettings(
                        HWindow, *extension, extensions, storage, ui);
                }
            }
            return 0;
        }

        case IDB_PLUGINKEYS:
        {
        AGAIN:
            CPluginData* p = GetSelectedPlugin();
            if (p != NULL && (p->MenuItems.Count > 0 || p->SupportDynMenuExt))
            {
                // if the plugin is already loaded, rebuild its menu - if not,
                // the menu will be rebuilt when the plugin loads
                if (p->GetLoaded() && p->SupportDynMenuExt)
                {
                    p->BuildMenu(HWindow, TRUE);
                    p->ReleasePluginDynMenuIcons(); // nobody needs this object (the menu is reloaded when displayed again)
                }
                // load the plugin if needed to get the latest version of the menu
                if (p->InitDLL(HWindow) &&
                    (p->MenuItems.Count > 0 ||
                     p->SupportDynMenuExt && p->GetPluginInterfaceForMenuExt()->NotEmpty()))
                {
                    if (p->MenuItems.Count > 0) // do not open the dialog for an empty dynamic menu (an empty static one fails earlier without a message)
                    {
                        CPluginKeys keys(HWindow, p);
                        if (keys.IsGood())
                        {
                            keys.Execute();
                            if (keys.Reset)
                            {
                                UpdateWindow(HWindow); // let Plugins Manager update so the Shortcut Keys window doesn't remain visible
                                p->Unload(HWindow, FALSE);
                                goto AGAIN;
                            }
                        }
                    }
                    else
                        TRACE_I("Plugin has dynamic menu which is empty (unexpected situation). We will not open Keyboard Shortcuts dialog.");
                }
                RefreshListView(); // a DLL was loaded, we have fresher data ...
            }
            return 0;
        }

        case IDB_PLUGINADD:
        {
            RefreshPanels = TRUE;
            char fileName[2000];
            fileName[0] = 0;
            OPENFILENAME ofn;
            memset(&ofn, 0, sizeof(OPENFILENAME));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = HWindow;
            char* s = LoadStr(IDS_PLUGINFILTER);
            ofn.lpstrFilter = s;
            while (*s != 0) // creating a double-null terminated list
            {
                if (*s == '|')
                    *s = 0;
                s++;
            }
            ofn.lpstrFile = fileName;
            ofn.nMaxFile = 2000;
            ofn.lpstrDefExt = "SPL";

            char buf[MAX_PATH];
            GetModuleFileName(HInstance, buf, MAX_PATH);
            s = strrchr(buf, '\\');
            if (s != NULL)
            {
                strcpy(s + 1, "plugins");
                ofn.lpstrInitialDir = buf;
            }

            ofn.nFilterIndex = 1;
            ofn.lpstrTitle = LoadStr(IDS_PLUGINADDTITLE);
            ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST |
                        OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_NOCHANGEDIR;

            if (SafeGetOpenFileName(&ofn))
            {
                // loop over all selected names
                char oneName[MAX_PATH];
                char* fName = NULL;
                int off = 0;
                strcpy(oneName, fileName);
                off = (int)strlen(oneName);
                if (off + 1 < 2000 && *(fileName + off + 1) != 0) // not a single name
                {
                    fName = oneName + off;
                    if (off > 0 && *(fileName + off - 1) != '\\') // missing backslash
                    {
                        *fName++ = '\\';
                        *fName = 0;
                    }
                }

                BOOL pluginAdded = FALSE;
                CPluginData* addedPlugin = NULL;
                while (1)
                {
                    if (fName != NULL && off + 1 < 2000)
                    {
                        strcpy(fName, fileName + off + 1);
                        off += (int)(strlen(fileName + off + 1) + 1);
                    }

                    // oneName contains the name of the x-th selected plugin (enumeration)
                    char pluginName[MAX_PATH];
                    if (StrNICmp(oneName, buf, (int)strlen(buf)) == 0 && oneName[(int)strlen(buf)] == '\\')
                    {
                        memmove(pluginName, oneName + strlen(buf) + 1, strlen(oneName) - strlen(buf) + 1 - 1);
                    }
                    else
                        strcpy(pluginName, oneName);
                    BOOL add = TRUE;
                    int index;
                    if (Plugins.FindDLL(pluginName, index))
                    {
                        char buf2[MAX_PATH + 300];
                        sprintf(buf2, LoadStr(IDS_PLUGINEXISTS), Plugins.Get(index)->Name,
                                Plugins.Get(index)->DLLName);
                        //                add = SalMessageBox(HWindow, buf2, LoadStr(IDS_QUESTION),
                        //                                    MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES;
                        SalMessageBox(HWindow, buf2, LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
                        add = FALSE;
                    }
                    if (add && Plugins.AddPlugin(HWindow, pluginName))
                    {
                        pluginAdded = TRUE;
                        int pluginIndex;
                        if (Plugins.FindDLL(pluginName, pluginIndex))
                            addedPlugin = Plugins.Get(pluginIndex);
                    }

                    // finish when two zero characters are found
                    if (off + 1 >= 2000 || *(fileName + off + 1) == 0)
                        break;
                }

                if (pluginAdded)
                {
                    if (Configuration.KeepPluginsSorted)
                        Plugins.UpdatePluginsOrder(TRUE);
                    // insert items and select the one that was added
                    RefreshListView(FALSE, -1, addedPlugin);
                }
            }
            return 0;
        }

        case IDB_PLUGINREMOVE:
        {
            RefreshPanels = TRUE;
            char name[MAX_PATH];
            int index, lvIndex;
            CPluginData* p = GetSelectedPlugin(&index, &lvIndex);
            if (p != NULL)
            {
                strcpy(name, p->Name);
                char buf[MAX_PATH + 100];
                sprintf(buf, LoadStr(IDS_PLUGINREMOVEOK), name);
                if (SalMessageBox(HWindow, buf, LoadStr(IDS_QUESTION),
                                  MB_YESNO | MB_ICONQUESTION) == IDYES)
                {
                    BOOL oldUnloadingPluginsForMainWindowClose = UnloadingPluginsForMainWindowClose;
                    UnloadingPluginsForMainWindowClose = TRUE;
                    Plugins.Remove(HWindow, index, TRUE);
                    UnloadingPluginsForMainWindowClose = oldUnloadingPluginsForMainWindowClose;
                    RefreshListView(FALSE, lvIndex); // a DLL was removed, we have fresher data ...
                }
            }
            return 0;
        }

        case IDB_PLUGINTEST:
        {
            RefreshPanels = TRUE;
            CPluginData* p = GetSelectedPlugin();
            if (p != NULL)
            {
                if (p->InitDLL(HWindow))
                {
                    char buf[MAX_PATH + 100];
                    sprintf(buf, LoadStr(IDS_PLUGINTESTOK), p->Name);
                    SalMessageBox(HWindow, buf, LoadStr(IDS_INFOTITLE), MB_OK | MB_ICONINFORMATION);
                }
                RefreshListView(); // a DLL was loaded, we have fresher data ...
            }
            else
            {
                int listIndex = ListView_GetNextItem(HListView, -1, LVIS_FOCUSED);
                Salamatrix::Extensions::ExtensionInfo* extension = GetSelectedExtension();
                Salamatrix::Extensions::IExtensionsService* service = QueryExtensionService();
                if (extension != NULL && service != NULL)
                {
                    const BOOL active =
                        extension->State == Salamatrix::Extensions::ExtensionStateActive;
                    const BOOL disabled =
                        (extension->Descriptor.Flags &
                         Salamatrix::Extensions::ExtensionFlagDisabled) != 0;
                    Salamatrix::Storage::IStorageService* storage =
                        QueryExtensionStorageService();
                    if (active)
                    {
                        // The existing Deactivate action is also the user
                        // facing persistent disable control for manifest
                        // packages. Do not mark it disabled until its worker
                        // has stopped successfully.
                        if (service->DeactivateExtension(
                                extension->Descriptor.Id) &&
                            (storage == NULL || storage->SetBoolean(
                                extension->Descriptor.Id,
                                "salamatrix.enabled", FALSE)))
                        {
                            service->SetExtensionDisabled(
                                extension->Descriptor.Id, TRUE);
                        }
                    }
                    else if (disabled)
                    {
                        if (storage == NULL || storage->SetBoolean(
                                extension->Descriptor.Id,
                                "salamatrix.enabled", TRUE))
                        {
                            if (service->SetExtensionDisabled(
                                    extension->Descriptor.Id, FALSE))
                            {
                                service->ActivateExtension(
                                    extension->Descriptor.Id);
                            }
                        }
                    }
                    else
                        service->ActivateExtension(extension->Descriptor.Id);
                    RefreshListView(TRUE, listIndex);
                }
            }
            return 0;
        }

        case IDB_PLUGINTESTALL:
        {
            RefreshPanels = TRUE;
            if (Plugins.TestAll(HWindow)) // at least one plugin was loaded, we have fresher data ...
            {
                RefreshListView();
            }
            return 0;
        }

        case IDB_PLUGINFOCUS:
        {
            CPluginData* p = GetSelectedPlugin();
#ifdef _WIN64 // FIXME_X64_WINSCP - this will probably need to be solved differently... (ignoring the missing WinSCP in the x64 version of Salamander)
            char bufText[MAX_PATH + 200];
            if (p != NULL && IsPluginUnsupportedOnX64(p->DLLName))
            {
                // inform the user that this plugin is available only in the 32-bit version (x86)
                // IDS_PLUGINISX86ONLY is not an ideal text but I don't care, it will do,
                // and we won't bother translators unnecessarily
                sprintf(bufText, LoadStr(IDS_PLUGINISX86ONLY), p->Name);
                SalMessageBox(HWindow, bufText, LoadStr(IDS_INFOTITLE), MB_OK | MB_ICONINFORMATION);
                return 0;
            }
#endif // _WIN64
            if (p != NULL)
            {
                char buf[MAX_PATH];
                char* s = p->DLLName;
                if ((*s != '\\' || *(s + 1) != '\\') && // not UNC
                    (*s == 0 || *(s + 1) != ':'))       // not "c:" -> relative path to plugins subdirectory
                {
                    GetModuleFileName(HInstance, buf, MAX_PATH);
                    s = strrchr(buf, '\\') + 1;
                    strcpy(s, "plugins\\");
                    strcat(s, p->DLLName);
                    s = buf;
                }
                strcpy(FocusPlugin, s);
                PostMessage(HWindow, WM_COMMAND, IDOK, 0);
            }
            return 0;
        }

        case IDB_PLUGINUPDATES:
        {
            CPluginData* samandarin;
            if (GetLoadedSamandarinUpdateNotifier(&samandarin))
            {
                BOOL unselect;
                samandarin->ExecuteMenuItem2(MainWindow->GetActivePanel(), HWindow, -1, 2, unselect);
            }
            return 0;
        }

        case IDB_PLUGINUNLOAD:
        {
            RefreshPanels = TRUE;
            CPluginData* p = GetSelectedPlugin();
            if (p != NULL)
            {
                p->Unload(HWindow, TRUE);
                RefreshListView(); // a DLL was loaded, we have fresher data ...
            }
            return 0;
        }
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    {
        LRESULT brush = 0;
        const bool handled = DarkModeHandleCtlColor(uMsg, wParam, lParam, brush);
        DARKMODE_RETURN_IF_HANDLED(handled, brush);

        if (ShouldUsePluginsDarkPalette())
        {
            HWND ctrl = reinterpret_cast<HWND>(lParam);
            if (ctrl != NULL)
            {
                int ctrlId = GetDlgCtrlID(ctrl);
                auto applyColors = [&](bool transparent) {
                    HDC dc = reinterpret_cast<HDC>(wParam);
                    HBRUSH dialogBrush = HDialogBrush != NULL ? HDialogBrush : GetSysColorBrush(COLOR_BTNFACE);
                    if (dc == NULL)
                        return reinterpret_cast<LRESULT>(dialogBrush);
                    const COLORREF background = DarkModeGetColors().background;
                    const COLORREF text = DarkModeGetColors().readableText;
                    SetTextColor(dc, text);
                    SetBkColor(dc, background);
                    SetBkMode(dc, transparent ? TRANSPARENT : OPAQUE);
                    return reinterpret_cast<LRESULT>(dialogBrush);
                };

                switch (ctrlId)
                {
                case IDC_PLUGINDESCRIPTION:
                case IDC_PLUGINCOPYRIGHT:
                case IDC_PLUGINWWW:
                case IDC_PLUGINEXTENSIONS:
                case IDC_PLUGINFSNAME:
                case IDC_PLUGINFUNCTIONS:
                case IDC_PLUGINHEADER:
                case IDC_PLUGINSHOWINBAR:
                case IDC_PLUGINSHOWINCHDRV:
                    return applyColors(true);

                case IDC_PLUGINTHUMBNAILS:
                    if (uMsg == WM_CTLCOLOREDIT)
                        return applyColors(false);
                    break;
                }
            }
        }
        if (handled)
            return brush;
        break;
    }

    case WM_THEMECHANGED:
    {
        ApplyTheme();
        return TRUE;
    }

    case WM_SETTINGCHANGE:
    {
        if (DarkModeHandleSettingChange(uMsg, lParam))
            ApplyTheme();
        return TRUE;
    }

    case WM_SYSCOLORCHANGE:
    {
        ApplyTheme();
        return TRUE;
    }
    }
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CPluginKeys
//

void GetKeyName(UINT vk, char* buff)
{
    LONG scan = (LONG)MapVirtualKey(vk, 0) << 16;
    GetKeyNameText(scan, buff, 50);
}

void GetHotKeyText(WORD hotKey, char* buff)
{
    BYTE wVirtKey = LOBYTE(hotKey);
    BYTE wMods = HIBYTE(hotKey);
    buff[0] = 0;
    if (wVirtKey != 0 || wMods != 0)
    {
        const char* plus = "+";
        if (wMods & HOTKEYF_CONTROL)
        {
            GetKeyName(VK_CONTROL, buff);
            lstrcat(buff, plus);
        }
        if (wMods & HOTKEYF_SHIFT)
        {
            GetKeyName(VK_SHIFT, buff + lstrlen(buff));
            lstrcat(buff, plus);
        }
        if (wMods & HOTKEYF_ALT)
        {
            GetKeyName(VK_MENU, buff + lstrlen(buff));
            lstrcat(buff, plus);
        }
        GetKeyName(wVirtKey, buff + lstrlen(buff));
    }
}

CPluginKeys::CPluginKeys(HWND hParent, CPluginData* plugin)
    : CCommonDialog(HLanguage, IDD_PLUGINKEYS, IDD_PLUGINKEYS, hParent)
{
    HListView = NULL;
    Header = NULL;
    Plugin = plugin;
    Reset = FALSE;
    HotKeys = (DWORD*)malloc(sizeof(DWORD) * Plugin->MenuItems.Count);
    if (HotKeys == NULL)
        TRACE_E(LOW_MEMORY);
    else
    {
        int i;
        for (i = 0; i < Plugin->MenuItems.Count; i++)
            HotKeys[i] = Plugin->MenuItems[i]->HotKey;
    }
}

CPluginKeys::~CPluginKeys()
{
    if (HotKeys != NULL)
        free(HotKeys);
}

BOOL CPluginKeys::IsGood()
{
    return HotKeys != NULL;
}

void CPluginKeys::Transfer(CTransferInfo& ti)
{
    if (ti.Type == ttDataFromWindow)
    {
        int i;
        for (i = 0; i < Plugin->MenuItems.Count; i++)
        {
            if (HotKeys[i] != 0)
            {
                WORD hotKey = HOTKEY_GET(HotKeys[i]);
                Plugins.RemoveHotKey(hotKey, NULL);        // remove 'hotKey' from all plugins
                Plugin->MenuItems[i]->HotKey = HotKeys[i]; // assign it to us
            }
        }
    }
}

void CPluginKeys::InitColumns()
{
    CALL_STACK_MESSAGE1("CPluginKeys::InitColumns()");
    LV_COLUMN lvc;
    int header[2] = {IDS_PLUGIN_COMMAND, IDS_PLUGIN_KEY};

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    int i;
    for (i = 0; i < 2; i++) // create columns
    {
        lvc.pszText = LoadStr(header[i]);
        lvc.iSubItem = i;
        ListView_InsertColumn(HListView, i, &lvc);
    }
}

void CPluginKeys::SetColumnWidths()
{
    RECT r;
    GetClientRect(HListView, &r);
    ListView_SetColumnWidth(HListView, 0, (int)((double)r.right * 0.70));
    // the last column will occupy the remaining space
    ListView_SetColumnWidth(HListView, 1, LVSCW_AUTOSIZE_USEHEADER);
}

void CPluginKeys::RefreshListView(BOOL setOnly)
{
    SendMessage(HListView, WM_SETREDRAW, FALSE, 0);

    HIMAGELIST hIcons = ImageList_LoadBitmap(HInstance, MAKEINTRESOURCE(IDB_MENUITEMS), 13, 0, RGB(255, 0, 255)); // destruction is handled by the listview
    HIMAGELIST hOldIcons = ListView_SetImageList(HListView, hIcons, LVSIL_SMALL);
    if (hOldIcons != NULL)
        ImageList_Destroy(hOldIcons);

    if (!setOnly)
        ListView_DeleteAllItems(HListView);

    int row = 0;
    int level = 0; // indentation
    int i;
    for (i = 0; i < Plugin->MenuItems.Count; i++)
    {
        CPluginMenuItem* item = Plugin->MenuItems[i];
        if (item->Type == pmitEndSubmenu && level > 0)
            level--;
        if ((item->Type != pmitItemOrSeparator && item->Type != pmitStartSubmenu) || item->Name == NULL)
            continue;

        if (!setOnly)
        {
            LVITEM lvi;
            lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_INDENT | LVIF_PARAM;
            lvi.iImage = item->Type == pmitStartSubmenu ? 1 : 0;
            lvi.iItem = row;
            lvi.iSubItem = 0;
            char pszTextBuff[] = "";
            lvi.pszText = pszTextBuff;
            lvi.iIndent = level;
            lvi.lParam = i; // for identification
            ListView_InsertItem(HListView, &lvi);
        }
        // command name
        char buff[500];
        lstrcpyn(buff, item->Name, 500);
        RemoveAmpersands(buff);

        // remove the hint from the text if present
        if ((item->HotKey & HOTKEY_HINT) != 0)
        {
            char* p = buff;
            while (*p != 0)
            {
                if (*p == '\t')
                {
                    *p = 0;
                    break;
                }
                p++;
            }
        }

        ListView_SetItemText(HListView, row, 0, buff);
        // shortcut key
        GetHotKeyText(LOWORD(HotKeys[i]), buff);
        ListView_SetItemText(HListView, row, 1, buff);
        row++;
        if (item->Type == pmitStartSubmenu)
            level++;
    }

    if (!setOnly)
    {
        DWORD state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(HListView, 0, state, state);
        ListView_EnsureVisible(HListView, 0, FALSE);
    }

    SendMessage(HListView, WM_SETREDRAW, TRUE, 0);
}

CPluginMenuItem*
CPluginKeys::GetItem(int index)
{
    LVITEM lvi;
    lvi.iItem = index;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_PARAM;
    ListView_GetItem(HListView, &lvi);
    int i = (int)lvi.lParam;
    if (i >= 0 && i < Plugin->MenuItems.Count)
        return Plugin->MenuItems[i];
    return NULL;
}

CPluginMenuItem*
CPluginKeys::GetSelectedItem(int* orgIndex)
{
    int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
    if (index == -1)
        return NULL;
    if (orgIndex != NULL)
    {
        LVITEM lvi;
        lvi.iItem = index;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_PARAM;
        ListView_GetItem(HListView, &lvi);
        *orgIndex = (int)lvi.lParam;
    }
    return GetItem(index);
}

WORD CPluginKeys::GetHotKey(BYTE* virtKey, BYTE* mods)
{
    WORD hotKey = (WORD)SendDlgItemMessage(HWindow, IDC_PLUGINKEY, HKM_GETHOTKEY, 0, 0);
    if (virtKey != NULL)
        *virtKey = LOBYTE(hotKey);
    if (mods != NULL)
        *mods = HIBYTE(hotKey);
    return hotKey;
}

void CPluginKeys::EnableButtons()
{
    // get the selected item
    int orgIndex;
    CPluginMenuItem* item = GetSelectedItem(&orgIndex);
    BOOL keyAssigned = ((item != NULL) && (HotKeys[orgIndex] & HOTKEY_MASK) != 0);
    BOOL validItem = ((item != NULL) && (item->Type == pmitItemOrSeparator)); // separators are filtered out

    // get the hotkey
    BYTE virtKey;
    WORD hotKey = GetHotKey(&virtKey, NULL);
    BOOL valiKey = (virtKey != 0) && !IsSalHotKey(hotKey);

    BOOL changeFocus = FALSE;
    HWND focus = GetFocus();

    EnableWindow(GetDlgItem(HWindow, IDC_PLUGINKEY), validItem);
    if (GetDlgItem(HWindow, IDC_ASSIGN) == focus && !(validItem && valiKey))
        changeFocus = TRUE;
    EnableWindow(GetDlgItem(HWindow, IDC_ASSIGN), validItem && valiKey);
    if (GetDlgItem(HWindow, IDC_REMOVE) == focus && !(validItem && keyAssigned))
        changeFocus = TRUE;
    EnableWindow(GetDlgItem(HWindow, IDC_REMOVE), validItem && keyAssigned);

    if (changeFocus)
    {
        PostMessage(focus, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
        focus = GetDlgItem(HWindow, IDOK);
        PostMessage(focus, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
        SetFocus(HListView);
    }
}

void CPluginKeys::HandleConflictWarning()
{
    char buff[500];
    buff[0] = 0;

    BYTE virtKey;
    WORD hotKey = GetHotKey(&virtKey);
    if (virtKey != 0)
    {
        // does the hot key belong to Salamander?
        if (IsSalHotKey(hotKey))
        {
            strcpy(buff, LoadStr(IDS_HOTKEY_SAL_CONFLICT));
        }

        // search in ours
        if (buff[0] == 0)
        {
            int i;
            for (i = 0; i < Plugin->MenuItems.Count; i++)
            {
                if (HOTKEY_GET(HotKeys[i]) == hotKey)
                {
                    sprintf(buff, LoadStr(IDS_HOTKEY_PLUGIN_CONFLICT), Plugin->Name);
                    break;
                }
            }
        }

        // search in other plugins
        if (buff[0] == 0)
        {
            int pluginIndex;
            int menuItemIndex;
            if (Plugins.FindHotKey(hotKey, TRUE, Plugin, &pluginIndex, &menuItemIndex))
            {
                CPluginData* plugin = Plugins.Get(pluginIndex);
                sprintf(buff, LoadStr(IDS_HOTKEY_PLUGIN_CONFLICT), plugin->Name);
            }
        }
    }
    SetDlgItemText(HWindow, IDC_CONFLICT_WARNING, buff);
}

INT_PTR
CPluginKeys::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CPluginKeys::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // dialog box title
        char buff[500];
        char buff2[500];
        GetWindowText(HWindow, buff, 500);
        sprintf(buff2, buff, Plugin->Name);
        SetWindowText(HWindow, buff2);

        // listview setup
        HListView = GetDlgItem(HWindow, IDL_COMMANDS);
        ListView_SetExtendedListViewStyleEx(HListView, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

        // header line
        Header = new CToolbarHeader(HWindow, IDC_PLUGINHEADER, HListView, 0);
        if (Header == NULL)
            TRACE_E(LOW_MEMORY);

        // insert columns
        InitColumns();

        // insert items
        RefreshListView(FALSE);

        // set column widths
        SetColumnWidths();

        EnableButtons();
        HandleConflictWarning();

        break;
    }

    case WM_SYSCOLORCHANGE:
    {
        ListView_SetBkColor(HListView, GetSysColor(COLOR_WINDOW));
        break;
    }

    case WM_SYSCOMMAND:
    {
        // suppress the beep when entering keys such as Alt+Shift+Z (for example)
        if (wParam == SC_KEYMENU && GetFocus() == GetDlgItem(HWindow, IDC_PLUGINKEY))
        {
            SetWindowLongPtr(HWindow, DWLP_MSGRESULT, 0);
            return TRUE;
        }
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_PLUGINKEY:
        {
            if (HIWORD(wParam) == EN_CHANGE)
            {
                EnableButtons();
                HandleConflictWarning();
            }
            break;
        }

        case IDC_ASSIGN:
        {
            int orgIndex;
            CPluginMenuItem* item = GetSelectedItem(&orgIndex);
            WORD hotKey = GetHotKey();
            // remove redundancies
            int i;
            for (i = 0; i < Plugin->MenuItems.Count; i++)
                if (HOTKEY_GET(HotKeys[i]) == hotKey)
                    HotKeys[i] = 0;
            HotKeys[orgIndex] = (HotKeys[orgIndex] & ~HOTKEY_MASK) | HOTKEY_DIRTY; // protect this change from being overridden by the plugin's Connect()
            HotKeys[orgIndex] |= (DWORD)hotKey;
            SendDlgItemMessage(HWindow, IDC_PLUGINKEY, HKM_SETHOTKEY, 0, 0);
            EnableButtons();
            HandleConflictWarning();
            RefreshListView(TRUE);
            return 0;
        }

        case IDC_REMOVE:
        {
            int orgIndex;
            CPluginMenuItem* item = GetSelectedItem(&orgIndex);
            BOOL keyAssigned = ((item != NULL) && (HotKeys[orgIndex] & HOTKEY_MASK) != 0);
            if (keyAssigned)
            {
                HotKeys[orgIndex] = (HotKeys[orgIndex] & ~HOTKEY_MASK) | HOTKEY_DIRTY; // protect this change from being overridden by the plugin's Connect()
                EnableButtons();
                HandleConflictWarning();
                RefreshListView(TRUE);
            }
            return 0;
        }

        case IDC_RESET:
        {
            char buf[MAX_PATH + 100];
            sprintf(buf, LoadStr(IDS_PLUGINRESETKEYS), Plugin->Name);
            if (SalMessageBox(HWindow, buf, LoadStr(IDS_INFOTITLE), MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
            {
                int i;
                for (i = 0; i < Plugin->MenuItems.Count; i++)
                    Plugin->MenuItems[i]->HotKey = 0; // clear hotkeys, the next Connect() will restore them
                Reset = TRUE;
                PostMessage(HWindow, WM_COMMAND, IDCANCEL, 0);
            }
            return 0;
        }
        }
        break;
    }

    case WM_NOTIFY:
    {
        if (wParam == IDL_COMMANDS)
        {
            switch (((LPNMHDR)lParam)->code)
            {
            case LVN_ITEMCHANGED:
            {
                int i;
                for (i = 0; i < ListView_GetItemCount(HListView); i++)
                {
                    CPluginMenuItem* item = GetItem(i);
                    if (item == NULL || item->Type == pmitStartSubmenu)
                    {
                        // clear SELECTION
                        ListView_SetItemState(HListView, i, 0, LVIS_SELECTED);
                    }
                }

                EnableButtons();
                return 0;
            }
            }
        }
        break;
    }
    }
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CArchiveUpdateDlg
//

CArchiveUpdateDlg::CArchiveUpdateDlg(HWND hParent, CFileTimeStamps* fileStamps, CFilesWindow* panel)
    : CCommonDialog(HLanguage, IDD_ARCHIVEUPDATE, IDD_ARCHIVEUPDATE, hParent)
{
    FileStamps = fileStamps;
    Panel = panel;
}

void CArchiveUpdateDlg::EnableButtons()
{
    int sel = (int)SendMessage(GetDlgItem(HWindow, IDL_UPDATEDFILES), LB_GETSELCOUNT, 0, 0);

    HWND focus = GetFocus();
    BOOL changeFocus = FALSE;
    if (GetDlgItem(HWindow, IDB_COPYSELTO) == focus && sel == 0)
        changeFocus = TRUE;
    EnableWindow(GetDlgItem(HWindow, IDB_COPYSELTO), sel != 0);
    if (GetDlgItem(HWindow, IDB_IGNORESEL) == focus && sel == 0)
        changeFocus = TRUE;
    EnableWindow(GetDlgItem(HWindow, IDB_IGNORESEL), sel != 0);
    if (changeFocus)
    {
        PostMessage(focus, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
        focus = GetDlgItem(HWindow, IDOK);
        PostMessage(focus, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
        SetFocus(focus);
    }
}

INT_PTR
CArchiveUpdateDlg::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CArchiveUpdateDlg::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        SetDlgItemText(HWindow, IDT_ARCHIVENAME, FileStamps->GetZIPFile());
        HWND list = GetDlgItem(HWindow, IDL_UPDATEDFILES);
        FileStamps->AddFilesToListBox(list);
        SendMessage(list, LB_SETSEL, TRUE, -1);
        PostMessage(HWindow, WM_COMMAND, MAKELONG(IDL_UPDATEDFILES, LBN_SELCHANGE), (LPARAM)list);
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDL_UPDATEDFILES:
        {
            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                EnableButtons();
            }
            break;
        }

        case IDCANCEL:
        {
            if (SendMessage(GetDlgItem(HWindow, IDL_UPDATEDFILES), LB_GETCOUNT, 0, 0) != 0)
            { // only if there are some files in the listbox
                if (SalMessageBox(HWindow, LoadStr(IDS_ARCREALLYIGNOREALL), LoadStr(IDS_QUESTION),
                                  MB_YESNO | MSGBOXEX_ESCAPEENABLED | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES)
                {
                    return 0; // cancel of cancel
                }
            }
            break;
        }

        case IDB_COPYSELTO:
        {
            HWND list = GetDlgItem(HWindow, IDL_UPDATEDFILES);
            int selCount = (int)SendMessage(list, LB_GETSELCOUNT, 0, 0);
            if (selCount > 0)
            {
                int* indexes = new int[selCount];
                if (indexes != NULL)
                {
                    SendMessage(list, LB_GETSELITEMS, selCount, (LPARAM)indexes);
                    IntSort(indexes, 0, selCount - 1); // indexes may not be sorted, so sort them just in case

                    char path[MAX_PATH];
                    char* initPath;
                    strcpy(path, FileStamps->GetZIPFile());
                    if (!CutDirectory(path))
                        initPath = NULL;
                    else
                        initPath = path;
                    if (Panel->CheckPath(TRUE, initPath) != ERROR_SUCCESS)
                        initPath = NULL;

                    // let the selected files be copied
                    FileStamps->CopyFilesTo(HWindow, indexes, selCount, initPath);

                    delete indexes;
                }
                else
                    TRACE_E(LOW_MEMORY);
            }
            return 0;
        }

        case IDB_IGNORESEL:
        {
            HWND list = GetDlgItem(HWindow, IDL_UPDATEDFILES);
            int selCount = (int)SendMessage(list, LB_GETSELCOUNT, 0, 0);
            if (selCount > 0)
            {
                if (SalMessageBox(HWindow, LoadStr(IDS_ARCREALLYIGNORESEL), LoadStr(IDS_QUESTION),
                                  MB_YESNO | MSGBOXEX_ESCAPEENABLED | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
                {
                    int* indexes = new int[selCount];
                    if (indexes != NULL)
                    {
                        SendMessage(list, LB_GETSELITEMS, selCount, (LPARAM)indexes);
                        IntSort(indexes, 0, selCount - 1);     // indexes may not be sorted, so sort them just in case
                        FileStamps->Remove(indexes, selCount); // remove selected files

                        // refill the listbox
                        SendMessage(list, LB_RESETCONTENT, 0, 0);
                        FileStamps->AddFilesToListBox(list);
                        PostMessage(HWindow, WM_COMMAND, MAKELONG(IDL_UPDATEDFILES, LBN_SELCHANGE), (LPARAM)list);

                        if (SendMessage(list, LB_GETCOUNT, 0, 0) == 0) // this was "ignore all"
                        {
                            PostMessage(HWindow, WM_COMMAND, IDCANCEL, 0);
                        }

                        delete indexes;
                    }
                    else
                        TRACE_E(LOW_MEMORY);
                }
            }
            return 0;
        }
        }
        break;
    }
    }
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CCfgPageConfirmations
//

CCfgPageConfirmations::CCfgPageConfirmations()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_CONFIRMATIONS, IDD_CFGPAGE_CONFIRMATIONS, PSP_USETITLE, NULL),
      List(20, 5)
{
    HTreeView = NULL;
    DisableNotification = FALSE;
}

void CCfgPageConfirmations::Transfer(CTransferInfo& ti)
{
    DisableNotification = TRUE;
    int i;
    for (i = 0; i < List.Count; i++)
    {
        CConfirmationItem* cnfrm = &List[i];
        if (ti.Type == ttDataToWindow)
        {
            cnfrm->Checked = *cnfrm->Variable;
            TreeView_SetItemState(HTreeView, cnfrm->HTreeItem,
                                  INDEXTOSTATEIMAGEMASK(cnfrm->Checked ? 2 : 1),
                                  TVIS_STATEIMAGEMASK);
        }
        else
        {
            // Read the checkbox state directly from the tree control when applying the page.
            // This keeps the saved configuration in sync even if the tree-view notification
            // for a checkbox toggle was not delivered before OK/Apply processing.
            cnfrm->Checked = ((TreeView_GetItemState(HTreeView, cnfrm->HTreeItem,
                                                     TVIS_STATEIMAGEMASK) &
                               TVIS_STATEIMAGEMASK) >>
                              12) == 2;
            *cnfrm->Variable = cnfrm->Checked;
        }
    }
    DisableNotification = FALSE;
}

HTREEITEM
CCfgPageConfirmations::AddItem(HTREEITEM hParent, int iImage, int textResID, int* value, int sectionIcon)
{
    TVINSERTSTRUCT tvis;
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_STATE; // | TVIF_PARAM;
    tvis.item.state = 0;
    if (iImage != -1)
    {
        tvis.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvis.item.iImage = iImage;
        tvis.item.iSelectedImage = iImage;
        tvis.item.state |= TVIS_EXPANDED;
    }
    else if (sectionIcon != -1)
    {
        tvis.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvis.item.iImage = sectionIcon;
        tvis.item.iSelectedImage = sectionIcon;
    }

    tvis.item.pszText = LoadStr(textResID);
    tvis.item.stateMask = tvis.item.state;

    HTREEITEM ret = TreeView_InsertItem(HTreeView, &tvis);
    if (iImage == -1)
    {
        CConfirmationItem item;
        item.HTreeItem = ret;
        item.Variable = value;
        item.Checked = 0;
        List.Add(item); // put the leaf handles into an array for easy access
    }
    else
    {
        // section header - remove checkbox completely (state image 0 = no checkbox)
        TreeView_SetItemState(HTreeView, ret, 0, TVIS_STATEIMAGEMASK);
    }
    return ret;
}

void CCfgPageConfirmations::InitTree()
{
    // Confirm On (icon: exclamation = 0)
    HConfirmOn = AddItem(NULL, 0, IDS_CNFRM_CONFIRMON, NULL);
    HTREEITEM hFirst = AddItem(HConfirmOn, -1, IDS_CNFRM_FILEDIRDEL, &Configuration.CnfrmFileDirDel, 0);
    AddItem(HConfirmOn, -1, IDS_CNFRM_NEDIRDEL, &Configuration.CnfrmNEDirDel, 0);
    AddItem(HConfirmOn, -1, IDS_CNFRM_FILEOVER, &Configuration.CnfrmFileOver, 0);
    AddItem(HConfirmOn, -1, IDS_CNFRM_SHFILEDEL, &Configuration.CnfrmSHFileDel, 0);
    AddItem(HConfirmOn, -1, IDS_CNFRM_SHDIRDEL, &Configuration.CnfrmSHDirDel, 0);
    AddItem(HConfirmOn, -1, IDS_CNFRM_SHFILEOVER, &Configuration.CnfrmSHFileOver, 0);
    AddItem(HConfirmOn, -1, IDS_CNFRM_DIRALREADYEXISTS, &Configuration.CnfrmDirOver, 0);
    AddItem(HConfirmOn, -1, IDS_CNFRM_NTFSPRESS, &Configuration.CnfrmNTFSPress, 0);
    AddItem(HConfirmOn, -1, IDS_CNFRM_NTFSCRYPT, &Configuration.CnfrmNTFSCrypt, 0);
    AddItem(HConfirmOn, -1, IDS_CNFRM_DAD, &Configuration.CnfrmDragDrop, 0);
    AddItem(HConfirmOn, -1, IDS_CNFRM_UMDIFFDLG, &Configuration.CnfrmShowNamesToCompare, 0);
    AddItem(HConfirmOn, -1, IDS_CNFRM_DSTIGNORED, &Configuration.CnfrmDSTShiftsIgnored, 0);
    AddItem(HConfirmOn, -1, IDS_CNFRM_DSTCANIGN, &Configuration.CnfrmDSTShiftsOccured, 0);

    // Show message (icon: question = 1)
    HShowMessage = AddItem(NULL, 1, IDS_CNFRM_SHOWMESSAGE, NULL);
    AddItem(HShowMessage, -1, IDS_CNFRM_CLOSEARCHIVE, &Configuration.CnfrmCloseArchive, 1);
    AddItem(HShowMessage, -1, IDS_CNFRM_CLOSEFIND, &Configuration.CnfrmCloseFind, 1);
    AddItem(HShowMessage, -1, IDS_CNFRM_STOPFIND, &Configuration.CnfrmStopFind, 1);
    AddItem(HShowMessage, -1, IDS_CNFRM_CREATEPATH, &Configuration.CnfrmCreatePath, 1);
    AddItem(HShowMessage, -1, IDS_CNFRM_ALWAYSONTOP, &Configuration.CnfrmAlwaysOnTop, 1);
    AddItem(HShowMessage, -1, IDS_CNFRM_ONSALCLOSE, &Configuration.CnfrmOnSalClose, 1);
    AddItem(HShowMessage, -1, IDS_CNFRM_DETACHCLOSE, &Configuration.CnfrmDetachClose, 1);
    AddItem(HShowMessage, -1, IDS_CNFRM_ONSENDEMAIL, &Configuration.CnfrmSendEmail, 1);
    AddItem(HShowMessage, -1, IDS_CNFRM_ONADDTOARCHIVE, &Configuration.CnfrmAddToArchive, 1);
    AddItem(HShowMessage, -1, IDS_CNFRM_ONCREATEDIR, &Configuration.CnfrmCreateDir, 1);
    AddItem(HShowMessage, -1, IDS_CNFRM_COPYMOVEOPTNS, &Configuration.CnfrmCopyMoveOptionsNS, 1);
    AddItem(HShowMessage, -1, IDS_CNFRM_CONFIRMDELETEEXTINFO, &Configuration.CnfrmConfirmDeleteExtInfo, 1);

    // Errors and Failures (icon: error = 2)
    HErrorsAndFailures = AddItem(NULL, 2, IDS_CNFRM_ERRORSFAILURES, NULL);
    AddItem(HErrorsAndFailures, -1, IDS_CNFRM_CHANGEDIRHISTORYERR, &Configuration.CnfrmChangeDirHistoryErr, 2);

    // select the first usable item
    TreeView_Select(HTreeView, hFirst, TVGN_CARET);
}

HIMAGELIST
CCfgPageConfirmations::CreateImageList()
{
    int iconSize = IconSizes[ICONSIZE_16];
    HIMAGELIST hIL = ImageList_Create(iconSize, iconSize, ILC_COLOR32 | ILC_MASK, 0, 4);

    HICON hIcon;
    LoadIconWithScaleDown(NULL, (PCWSTR)IDI_EXCLAMATION, iconSize, iconSize, &hIcon);
    ImageList_AddIcon(hIL, hIcon);
    DestroyIcon(hIcon);

    LoadIconWithScaleDown(NULL, (PCWSTR)IDI_QUESTION, iconSize, iconSize, &hIcon);
    ImageList_AddIcon(hIL, hIcon);
    DestroyIcon(hIcon);

    LoadIconWithScaleDown(HInstance, (PCWSTR)IDI_CONFIRM_ERRORS, iconSize, iconSize, &hIcon);
    ImageList_AddIcon(hIL, hIcon);
    DestroyIcon(hIcon);

    return hIL;
}

int CCfgPageConfirmations::FindInList(HTREEITEM hTreeItem)
{
    int i;
    for (i = 0; i < List.Count; i++)
    {
        if (List[i].HTreeItem == hTreeItem)
            return i;
    }
    return -1;
}

static HTREEITEM
GetConfirmationsTreeItemFromPoint(HWND hTreeView, POINT pt)
{
    TVHITTESTINFO hit;
    memset(&hit, 0, sizeof(hit));
    hit.pt = pt;
    HTREEITEM hItem = TreeView_HitTest(hTreeView, &hit);
    if (hItem != NULL && (hit.flags & (TVHT_ONITEM | TVHT_ONITEMRIGHT)) != 0)
        return hItem;

    hItem = TreeView_GetFirstVisible(hTreeView);
    while (hItem != NULL)
    {
        RECT r;
        if (TreeView_GetItemRect(hTreeView, hItem, &r, FALSE) &&
            pt.y >= r.top && pt.y < r.bottom)
        {
            return hItem;
        }
        hItem = TreeView_GetNextVisible(hTreeView, hItem);
    }
    return NULL;
}

INT_PTR
CCfgPageConfirmations::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        HTreeView = GetDlgItem(HWindow, IDC_CNFRM_TREE);

        DWORD style = (DWORD)GetWindowLongPtr(HTreeView, GWL_STYLE);
        style |= TVS_CHECKBOXES | TVS_FULLROWSELECT;
        SetWindowLongPtr(HTreeView, GWL_STYLE, style);

        dmlib::setDarkTreeViewCheckboxes(HTreeView);

        HImageList = CreateImageList();
        TreeView_SetImageList(HTreeView, HImageList, TVSIL_NORMAL);
        InitTree();

        // dialog elements should stretch with the dialog size, set split controls
        ElasticVerticalLayout(1, IDC_CNFRM_TREE);

        break;
    }

    case WM_DESTROY:
    {
        // according to MSDN, TreeView does not destroy the image list, but the W2K checked build complains 
        // during the following ImageList_Destroy call, so remove the image list just to be safe
        if (HTreeView != NULL)
            TreeView_SetImageList(HTreeView, NULL, TVSIL_NORMAL);
        if (HImageList != NULL)
        {
            ImageList_Destroy(HImageList);
            HImageList = NULL;
        }
        break;
    }

    case WM_NOTIFY:
    {
        if (DisableNotification)
            break;
        if (wParam == IDC_CNFRM_TREE)
        {
            LPNMHDR nmh = (LPNMHDR)lParam;
            if (nmh->code == TVN_ITEMCHANGED)
            {
                LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
                if ((pnmtv->itemNew.state & TVIS_STATEIMAGEMASK) != (pnmtv->itemOld.state & TVIS_STATEIMAGEMASK))
                {
                    int index = FindInList(pnmtv->itemNew.hItem);
                    if (index != -1)
                    {
                        List[index].Checked = ((pnmtv->itemNew.state & TVIS_STATEIMAGEMASK) >> 12) == 2;
                    }
                    else
                    {
                        // section header - remove checkbox completely
                        TreeView_SetItemState(HTreeView, pnmtv->itemNew.hItem,
                                              0, TVIS_STATEIMAGEMASK);
                    }
                }
            }
            if (nmh->code == NM_CLICK || nmh->code == NM_DBLCLK)
            {
                POINT pt;
                DWORD pos = GetMessagePos();
                pt.x = GET_X_LPARAM(pos);
                pt.y = GET_Y_LPARAM(pos);
                ScreenToClient(HTreeView, &pt);
                HTREEITEM hItem = GetConfirmationsTreeItemFromPoint(HTreeView, pt);
                if (hItem != NULL)
                {
                    TreeView_SelectItem(HTreeView, hItem);
                    if (nmh->code == NM_DBLCLK)
                    {
                        int index = FindInList(hItem);
                        if (index != -1)
                        {
                            List[index].Checked = !List[index].Checked;
                            TreeView_SetItemState(HTreeView, hItem,
                                                  INDEXTOSTATEIMAGEMASK(List[index].Checked ? 2 : 1),
                                                  TVIS_STATEIMAGEMASK);
                        }
                    }
                }
            }
        }
        break;
    }

    }
    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CCfgPageDrives
//

static const char* WIN32_LONG_PATHS_REG_KEY = "SYSTEM\\CurrentControlSet\\Control\\FileSystem";
static const char* WIN32_LONG_PATHS_REG_VALUE = "LongPathsEnabled";

static BOOL ReadWin32LongPathsEnabled(BOOL* enabled)
{
    if (enabled != NULL)
        *enabled = FALSE;

    HKEY key;
    LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, WIN32_LONG_PATHS_REG_KEY, 0,
                            KEY_QUERY_VALUE, &key);
    if (res != ERROR_SUCCESS)
        return FALSE;

    DWORD value = 0;
    DWORD type = REG_DWORD;
    DWORD size = sizeof(value);
    res = RegQueryValueEx(key, WIN32_LONG_PATHS_REG_VALUE, NULL, &type, (BYTE*)&value, &size);
    RegCloseKey(key);

    if (res == ERROR_FILE_NOT_FOUND)
        res = ERROR_SUCCESS;
    if (res == ERROR_SUCCESS && enabled != NULL)
        *enabled = (type == REG_DWORD && size == sizeof(value) && value != 0);
    return res == ERROR_SUCCESS;
}

static BOOL CanWriteWin32LongPathsEnabled()
{
    HKEY key;
    LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, WIN32_LONG_PATHS_REG_KEY, 0,
                            KEY_SET_VALUE, &key);
    if (res == ERROR_SUCCESS)
        RegCloseKey(key);
    return res == ERROR_SUCCESS;
}

static DWORD WriteWin32LongPathsEnabled(BOOL enabled)
{
    HKEY key;
    LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, WIN32_LONG_PATHS_REG_KEY, 0,
                            KEY_SET_VALUE, &key);
    if (res != ERROR_SUCCESS)
        return (DWORD)res;

    DWORD value = enabled ? 1 : 0;
    res = RegSetValueEx(key, WIN32_LONG_PATHS_REG_VALUE, 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
    RegCloseKey(key);
    return (DWORD)res;
}

CCfgPageDrives::CCfgPageDrives(BOOL focusIfPathIsInaccessibleGoTo)
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_DRIVES, IDD_CFGPAGE_DRIVES, PSP_USETITLE, NULL)
{
    IfPathIsInaccessibleGoToChanged = FALSE;
    FocusIfPathIsInaccessibleGoTo = focusIfPathIsInaccessibleGoTo;
    Win32LongPathsEnabled = FALSE;
    Win32LongPathsCanWrite = FALSE;
    HWin32LongPathsToolTip = NULL;
}

void CCfgPageDrives::Transfer(CTransferInfo& ti)
{
    ti.CheckBox(IDC_DRVSPEC_FLOPPYMON, Configuration.DrvSpecFloppyMon);
    ti.CheckBox(IDC_DRVSPEC_FLOPPYSIMPLE, Configuration.DrvSpecFloppySimple);
    ti.CheckBox(IDC_DRVSPEC_REMOVABLEMON, Configuration.DrvSpecRemovableMon);
    ti.CheckBox(IDC_DRVSPEC_REMOVABLESIMPLE, Configuration.DrvSpecRemovableSimple);
    ti.CheckBox(IDC_DRVSPEC_FIXEDMON, Configuration.DrvSpecFixedMon);
    ti.CheckBox(IDC_DRVSPEC_FIXEDSIMPLE, Configuration.DrvSpecFixedSimple);
    ti.CheckBox(IDC_DRVSPEC_CDROMMON, Configuration.DrvSpecCDROMMon);
    ti.CheckBox(IDC_DRVSPEC_CDROMSIMPLE, Configuration.DrvSpecCDROMSimple);
    ti.CheckBox(IDC_DRVSPEC_REMOTEMON, Configuration.DrvSpecRemoteMon);
    ti.CheckBox(IDC_DRVSPEC_REMOTESIMPLE, Configuration.DrvSpecRemoteSimple);
    ti.CheckBox(IDC_DRVSPEC_REMOTEACT, Configuration.DrvSpecRemoteDoNotRefreshOnAct);
    char path[SAL_MAX_PATH];
    char newPath[SAL_MAX_PATH];
    if (ti.Type == ttDataToWindow)
    {
        GetIfPathIsInaccessibleGoTo(path);
        ti.EditLine(IDE_DRVSPEC_ONERRGOTO, path, SAL_MAX_PATH);
        IfPathIsInaccessibleGoToChanged = FALSE;

        ReadWin32LongPathsEnabled(&Win32LongPathsEnabled);
        Win32LongPathsCanWrite = CanWriteWin32LongPathsEnabled();
        ti.CheckBox(IDC_DRVSPEC_LONGPATHS, Win32LongPathsEnabled);
        HWND hLongPaths = GetDlgItem(HWindow, IDC_DRVSPEC_LONGPATHS);
        EnableWindow(hLongPaths, Win32LongPathsCanWrite);
        if (!Win32LongPathsCanWrite)
        {
            if (HWin32LongPathsToolTip == NULL)
            {
                HWin32LongPathsToolTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, TTS_NOPREFIX | TTS_ALWAYSTIP,
                                                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                                        HWindow, NULL, HInstance, NULL);
                if (HWin32LongPathsToolTip != NULL)
                {
                    TOOLINFO toolInfo;
                    memset(&toolInfo, 0, sizeof(toolInfo));
                    toolInfo.cbSize = sizeof(toolInfo);
                    toolInfo.uFlags = TTF_SUBCLASS;
                    toolInfo.hwnd = HWindow;
                    toolInfo.uId = IDC_DRVSPEC_LONGPATHS;
                    toolInfo.hinst = HInstance;
                    toolInfo.lpszText = LoadStr(IDS_LONGPATHS_ADMINTOOLTIP);
                    GetWindowRect(hLongPaths, &toolInfo.rect);
                    MapWindowPoints(NULL, HWindow, (LPPOINT)&toolInfo.rect, 2);
                    SendMessage(HWin32LongPathsToolTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
                    SendMessage(HWin32LongPathsToolTip, TTM_SETDELAYTIME, TTDT_INITIAL, 400);
                    SendMessage(HWin32LongPathsToolTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 10000);
                }
            }
        }
        else if (HWin32LongPathsToolTip != NULL)
        {
            DestroyWindow(HWin32LongPathsToolTip);
            HWin32LongPathsToolTip = NULL;
        }
    }
    else
    {
        if (Win32LongPathsCanWrite)
        {
            BOOL newWin32LongPathsEnabled = Win32LongPathsEnabled;
            ti.CheckBox(IDC_DRVSPEC_LONGPATHS, newWin32LongPathsEnabled);
            if (newWin32LongPathsEnabled != Win32LongPathsEnabled)
            {
                DWORD res = WriteWin32LongPathsEnabled(newWin32LongPathsEnabled);
                if (res == ERROR_SUCCESS)
                {
                    Win32LongPathsEnabled = newWin32LongPathsEnabled;
                    SalMessageBox(HWindow, LoadStr(IDS_LONGPATHS_RESTART), LoadStr(IDS_INFOTITLE), MB_OK | MB_ICONINFORMATION);
                }
                else
                {
                    char buf[512];
                    _snprintf_s(buf, _TRUNCATE, LoadStr(IDS_LONGPATHS_SETERROR), GetErrorText(res));
                    SalMessageBox(HWindow, buf, LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
                }
            }
        }

        if (IfPathIsInaccessibleGoToChanged) // change only if the user actually edited the path
        {
            ti.EditLine(IDE_DRVSPEC_ONERRGOTO, newPath, SAL_MAX_PATH);
            GetIfPathIsInaccessibleGoTo(path, TRUE);
            if (IsTheSamePath(path, newPath)) // user wants to go to My Documents
            {
                Configuration.IfPathIsInaccessibleGoToIsMyDocs = TRUE;
                Configuration.IfPathIsInaccessibleGoTo[0] = 0;
            }
            else
            {
                Configuration.IfPathIsInaccessibleGoToIsMyDocs = FALSE;
                lstrcpyn(Configuration.IfPathIsInaccessibleGoTo, newPath, SAL_MAX_PATH);
            }
        }
    }
}

INT_PTR
CCfgPageDrives::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
    {
        if (HWin32LongPathsToolTip != NULL)
        {
            DestroyWindow(HWin32LongPathsToolTip);
            HWin32LongPathsToolTip = NULL;
        }
        break;
    }

    case WM_PAINT:
    {
        // Horrible mess - I need a message that arrives
        // after WM_INITDIALOG so we can set the focus
        // hopefully this will survive until the next Salamander version :-)
        if (FocusIfPathIsInaccessibleGoTo)
        {
            SendMessage(HWindow, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(HWindow, IDE_DRVSPEC_ONERRGOTO), TRUE);
            FocusIfPathIsInaccessibleGoTo = FALSE;
        }
        break;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDB_BROWSECOMMAND)
        {
            char path[MAX_PATH];
            GetDlgItemText(HWindow, IDE_DRVSPEC_ONERRGOTO, path, MAX_PATH);
            if (GetTargetDirectory(HWindow, HWindow, LoadStr(IDS_BROWSEONERRGOTOTITLE),
                                   LoadStr(IDS_BROWSEONERRGOTOTEXT), path, FALSE, path))
            {
                SetDlgItemText(HWindow, IDE_DRVSPEC_ONERRGOTO, path);
            }
        }
        if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDE_DRVSPEC_ONERRGOTO)
            IfPathIsInaccessibleGoToChanged = TRUE;
        break;
    }
    }
    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CCfgPageViewEdit
//

CCfgPageViewEdit::CCfgPageViewEdit()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_VIEWEDIT /*, empty page has no help */, PSP_USETITLE, NULL)
{
}

//
// ****************************************************************************
// CCfgPageViewers
//

CCfgPageViewers::CCfgPageViewers(BOOL alternative)
    : CCommonPropSheetPage(alternative ? LoadStr(IDS_ALTVIEWERS) : NULL, HLanguage,
                           IDD_CFGPAGE_VIEWERS, IDD_CFGPAGE_VIEWERS, PSP_USETITLE, NULL),
      ViewerMasks(10, 5)
{
    Alternative = alternative;
    SourceViewerMasks = Alternative ? MainWindow->AltViewerMasks : MainWindow->ViewerMasks;
    ViewerMasks.Load(*SourceViewerMasks);
    DisableNotification = FALSE;
    EditLB = NULL;
}

void CCfgPageViewers::Transfer(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageViewers::Transfer()");
    if (ti.Type == ttDataToWindow)
    {
        Dirty = FALSE;
        // populate the combo box with viewers
        HWND hCombo = GetDlgItem(HWindow, IDC_VIEW_TYPE);
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)LoadStr(IDS_VIEWER_EXTERNAL));
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)LoadStr(IDS_VIEWER_INTERNAL));
        int count = 0;
        int index;
        while ((index = Plugins.GetViewerIndex(count++)) != -1) // while "file viewer" plug-ins exist
        {
            CPluginData* p = Plugins.Get(index);
            if (p != NULL)
            {
                char buf[MAX_PATH];
                p->GetDisplayName(buf, MAX_PATH);
                SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)buf);
            }
            else
                TRACE_E("Unexpected situation in CCfgPageViewers::Transfer().");
        }

        // populate the list of viewers
        int i;
        for (i = 0; i < ViewerMasks.Count; i++)
            EditLB->AddItem((INT_PTR)ViewerMasks[i]);
        DisableNotification = TRUE;
        EditLB->SetCurSel(0);
        DisableNotification = FALSE;
        LoadControls();
        EnableControls();
    }
    else
    {
        MainWindow->EnterViewerMasksCS();
        SourceViewerMasks->Load(ViewerMasks);
        MainWindow->LeaveViewerMasksCS();
    }
}

void CCfgPageViewers::Validate(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageViewers::Validate()");
    if (Dirty)
    {
        int i;
        for (i = 0; i < ViewerMasks.Count; i++)
        {
            CMaskGroup masks(ViewerMasks[i]->Masks->GetMasksString());
            int errorPos1, errorPos2;
            const char* forbiddenChar = strchr(masks.GetMasksString(), '|');
            if (forbiddenChar != NULL || !masks.PrepareMasks(errorPos1))
            {
                if (forbiddenChar != NULL)
                    errorPos1 = (int)(forbiddenChar - masks.GetMasksString());
                EditLB->SetCurSel(i);
                SalMessageBox(HWindow, LoadStr(IDS_INCORRECTSYNTAX), LoadStr(IDS_ERRORTITLE),
                              MB_OK | MB_ICONEXCLAMATION);
                ti.ErrorOn(IDL_FILEMASKS);
                PostMessage(HWindow, WM_USER_EDIT, errorPos1, errorPos1 + 1);
                return;
            }

            CViewerMasksItem* item = ViewerMasks[i];
            if (item->ViewerType == VIEWER_EXTERNAL)
            {
                if (!ValidateCommandFile(HWindow, item->Command, errorPos1, errorPos2))
                {
                    EditLB->SetCurSel(i);
                    ti.ErrorOn(IDE_COMMAND);
                    PostMessage(GetDlgItem(HWindow, IDE_COMMAND), EM_SETSEL,
                                errorPos1, errorPos2);
                    return;
                }
                if (!ValidateArguments(HWindow, item->Arguments, errorPos1, errorPos2))
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
}

void CCfgPageViewers::LoadControls()
{
    CALL_STACK_MESSAGE1("CCfgPageViewers::LoadControls()");
    INT_PTR itemID;
    EditLB->GetCurSelItemID(itemID);
    BOOL empty = FALSE;
    if (itemID == -1)
        empty = TRUE;

    CViewerMasksItem* item = NULL;
    if (!empty)
        item = (CViewerMasksItem*)itemID;
    DisableNotification = TRUE;

    int type = item == NULL ? 2 : item->ViewerType;
    int cmbSel = -1;
    switch (type)
    {
    case VIEWER_EXTERNAL:
        cmbSel = 0;
        break;
    case VIEWER_INTERNAL:
        cmbSel = 1;
        break;

    default:
    {
        if (type < 0)
        {
            cmbSel = Plugins.GetViewerCount(-type - 1);
            if (cmbSel != -1)
                cmbSel += 2;
        }
    }
    }
    SendDlgItemMessage(HWindow, IDC_VIEW_TYPE, CB_SETCURSEL, cmbSel, 0);
    SendMessage(GetDlgItem(HWindow, IDE_COMMAND), EM_LIMITTEXT, MAX_PATH - 1, 0);
    SendMessage(GetDlgItem(HWindow, IDE_ARGUMENTS), EM_LIMITTEXT, MAX_PATH - 1, 0);
    SendMessage(GetDlgItem(HWindow, IDE_INITDIR), EM_LIMITTEXT, MAX_PATH - 1, 0);
    SendMessage(GetDlgItem(HWindow, IDE_COMMAND), WM_SETTEXT, 0,
                (LPARAM)(empty ? "" : item->Command));
    SendMessage(GetDlgItem(HWindow, IDE_ARGUMENTS), WM_SETTEXT, 0,
                (LPARAM)(empty ? "" : item->Arguments));
    SendMessage(GetDlgItem(HWindow, IDE_ARGUMENTS), EM_SETSEL, 0, -1); // so the browse overwrites the content
    SendMessage(GetDlgItem(HWindow, IDE_INITDIR), WM_SETTEXT, 0,
                (LPARAM)(empty ? "" : item->InitDir));
    SendMessage(GetDlgItem(HWindow, IDE_INITDIR), EM_SETSEL, 0, -1); // so the browse overwrites the content
    DisableNotification = FALSE;
}

void CCfgPageViewers::StoreControls()
{
    CALL_STACK_MESSAGE1("CCfgPageViewers::StoreControls()");
    int index;
    EditLB->GetCurSel(index);
    if (!DisableNotification && index >= 0 && index < EditLB->GetCount())
    {
        Dirty = TRUE;
        CViewerMasksItem* item = ViewerMasks[index];

        char command[MAX_PATH];
        char arguments[MAX_PATH];
        char initdir[MAX_PATH];
        SendMessage(GetDlgItem(HWindow, IDE_COMMAND), WM_GETTEXT,
                    MAX_PATH, (LPARAM)command);
        SendMessage(GetDlgItem(HWindow, IDE_ARGUMENTS), WM_GETTEXT,
                    MAX_PATH, (LPARAM)arguments);
        SendMessage(GetDlgItem(HWindow, IDE_INITDIR), WM_GETTEXT,
                    MAX_PATH, (LPARAM)initdir);
        item->Set(item->Masks->GetMasksString(), command, arguments, initdir);

        int cmbSel = (int)SendDlgItemMessage(HWindow, IDC_VIEW_TYPE, CB_GETCURSEL, 0, 0);
        int type;
        switch (cmbSel)
        {
        case 0:
            type = VIEWER_EXTERNAL;
            break;
        case 1:
            type = VIEWER_INTERNAL;
            break;

        default:
        {
            type = Plugins.GetViewerIndex(cmbSel - 2);
            if (type != -1)
                type = -type - 1;
            else
            {
                TRACE_E("Unexpected situation in CCfgPageViewers::StoreControls().");
                type = VIEWER_INTERNAL;
            }
            break;
        }
        }
        item->ViewerType = type;
    }
}

void CCfgPageViewers::EnableControls()
{
    CALL_STACK_MESSAGE1("CCfgPageViewers::EnableControls()");
    BOOL validItem = TRUE;
    INT_PTR itemID;
    EditLB->GetCurSelItemID(itemID);
    if (itemID == -1)
        validItem = FALSE;

    EnableWindow(GetDlgItem(HWindow, IDC_VIEW_TYPE), validItem);

    int type = (int)SendDlgItemMessage(HWindow, IDC_VIEW_TYPE, CB_GETCURSEL, 0, 0);
    BOOL external = (type == VIEWER_EXTERNAL);
    EnableWindow(GetDlgItem(HWindow, IDE_COMMAND), validItem & external);
    EnableWindow(GetDlgItem(HWindow, IDE_ARGUMENTS), validItem & external);
    EnableWindow(GetDlgItem(HWindow, IDE_INITDIR), validItem & external);

    EnableWindow(GetDlgItem(HWindow, IDB_BROWSECOMMAND), validItem & external);
    EnableWindow(GetDlgItem(HWindow, IDB_BROWSEARGUMENTS), validItem & external);
    EnableWindow(GetDlgItem(HWindow, IDB_BROWSEINITDIR), validItem & external);
}

INT_PTR
CCfgPageViewers::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CCfgPageViewers::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        EditLB = new CEditListBox(HWindow, IDL_FILEMASKS);
        if (EditLB == NULL)
            TRACE_E(LOW_MEMORY);
        else
        {
            EditLB->MakeHeader(IDS_MASKSHDR);
            EditLB->EnableDrag(::GetParent(HWindow));
        }
        ChangeToArrowButton(HWindow, IDB_BROWSECOMMAND);
        ChangeToArrowButton(HWindow, IDB_BROWSEARGUMENTS);
        ChangeToArrowButton(HWindow, IDB_BROWSEINITDIR);

        // dialog elements should stretch with the dialog size, set split controls
        ElasticVerticalLayout(1, IDL_FILEMASKS);

        break;
    }

    case WM_USER_EDIT:
    {
        SetFocus(GetDlgItem(HWindow, IDL_FILEMASKS));
        EditLB->OnBeginEdit((int)wParam, (int)lParam);
        return 0;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == EN_CHANGE || (LOWORD(wParam) == IDC_VIEW_TYPE && HIWORD(wParam) == CBN_SELCHANGE))
        {
            EnableControls();
            StoreControls();
        }

        if (LOWORD(wParam) == IDL_FILEMASKS && HIWORD(wParam) == LBN_SELCHANGE)
        {
            EditLB->OnSelChanged();
            LoadControls();
            EnableControls();
        }

        switch (LOWORD(wParam))
        {
        case IDB_BROWSECOMMAND:
        {
            TrackExecuteMenu(HWindow, IDB_BROWSECOMMAND, IDE_COMMAND, FALSE,
                             CommandExecutes, IDS_EXEFILTER);
            return 0;
        }

        case IDB_BROWSEARGUMENTS:
        {
            TrackExecuteMenu(HWindow, IDB_BROWSEARGUMENTS, IDE_ARGUMENTS, FALSE,
                             ArgumentsExecutes);
            return 0;
        }

        case IDB_BROWSEINITDIR:
        {
            TrackExecuteMenu(HWindow, IDB_BROWSEINITDIR, IDE_INITDIR, FALSE,
                             InitDirExecutes);
            return 0;
        }
        }
        break;
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
        case IDL_FILEMASKS:
        {
            switch (nmhdr->code)
            {
            case EDTLBN_GETDISPINFO:
            {
                EDTLB_DISPINFO* dispInfo = (EDTLB_DISPINFO*)lParam;
                if (dispInfo->ToDo == edtlbGetData)
                {
                    lstrcpyn(dispInfo->Buffer, ((CViewerMasksItem*)dispInfo->ItemID)->Masks->GetMasksString(), MAX_PATH);
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);
                    return TRUE;
                }
                else
                {
                    Dirty = TRUE;
                    CViewerMasksItem* item;
                    if (dispInfo->ItemID == -1)
                    {
                        item = new CViewerMasksItem();
                        if (item == NULL)
                        {
                            TRACE_E(LOW_MEMORY);
                            SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
                            return TRUE;
                        }
                        ViewerMasks.Add(item);
                        item->Set(dispInfo->Buffer, item->Command, item->Arguments, item->InitDir);
                        EditLB->SetItemData((INT_PTR)item);
                    }
                    else
                    {
                        item = (CViewerMasksItem*)dispInfo->ItemID;
                        item->Set(dispInfo->Buffer, item->Command, item->Arguments, item->InitDir);
                    }

                    LoadControls();
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

              char buf[sizeof(CViewerMasksItem)];
              memcpy(buf, ViewerMasks[srcIndex], sizeof(CViewerMasksItem));
              memcpy(ViewerMasks[srcIndex], ViewerMasks[dstIndex], sizeof(CViewerMasksItem));
              memcpy(ViewerMasks[dstIndex], buf, sizeof(CViewerMasksItem));

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

                char buf[sizeof(CViewerMasksItem)];
                memcpy(buf, ViewerMasks[srcIndex], sizeof(CViewerMasksItem));
                if (srcIndex < dstIndex)
                {
                    int i;
                    for (i = srcIndex; i < dstIndex; i++)
                        memcpy(ViewerMasks[i], ViewerMasks[i + 1], sizeof(CViewerMasksItem));
                }
                else
                {
                    int i;
                    for (i = srcIndex; i > dstIndex; i--)
                        memcpy(ViewerMasks[i], ViewerMasks[i - 1], sizeof(CViewerMasksItem));
                }
                memcpy(ViewerMasks[dstIndex], buf, sizeof(CViewerMasksItem));

                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE); // allow change
                return TRUE;
            }

            case EDTLBN_DELETEITEM:
            {
                int index;
                EditLB->GetCurSel(index);
                ViewerMasks.Delete(index);
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
        if (idCtrl == IDL_FILEMASKS)
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
// CCfgPageEditors
//

CCfgPageEditors::CCfgPageEditors()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_EDITORS, IDD_CFGPAGE_EDITORS, PSP_USETITLE, NULL),
      EditorMasks(10, 5)
{
    SourceEditorMasks = MainWindow->EditorMasks;
    EditorMasks.Load(*SourceEditorMasks);
    DisableNotification = FALSE;
    EditLB = NULL;
}

void CCfgPageEditors::Transfer(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageEditors::Transfer()");
    if (ti.Type == ttDataToWindow)
    {
        Dirty = FALSE;
        int i;
        for (i = 0; i < EditorMasks.Count; i++)
            EditLB->AddItem((INT_PTR)EditorMasks[i]);
        EditLB->SetCurSel(0);
        LoadControls();
        EnableControls();
    }
    else
    {
        SourceEditorMasks->Load(EditorMasks);
    }
}

void CCfgPageEditors::Validate(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageEditors::Validate()");
    if (Dirty)
    {
        int i;
        for (i = 0; i < EditorMasks.Count; i++)
        {
            CMaskGroup masks(EditorMasks[i]->Masks->GetMasksString());
            int errorPos1, errorPos2;
            if (!masks.PrepareMasks(errorPos1))
            {
                EditLB->SetCurSel(i);
                SalMessageBox(HWindow, LoadStr(IDS_INCORRECTSYNTAX), LoadStr(IDS_ERRORTITLE),
                              MB_OK | MB_ICONEXCLAMATION);
                ti.ErrorOn(IDL_FILEMASKS);
                PostMessage(HWindow, WM_USER_EDIT, errorPos1, errorPos1 + 1);
                return;
            }

            CEditorMasksItem* item = EditorMasks[i];
            if (!ValidateCommandFile(HWindow, item->Command, errorPos1, errorPos2))
            {
                EditLB->SetCurSel(i);
                ti.ErrorOn(IDE_COMMAND);
                PostMessage(GetDlgItem(HWindow, IDE_COMMAND), EM_SETSEL,
                            errorPos1, errorPos2);
                return;
            }
            if (!ValidateArguments(HWindow, item->Arguments, errorPos1, errorPos2))
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

void CCfgPageEditors::LoadControls()
{
    CALL_STACK_MESSAGE1("CCfgPageEditors::LoadControls()");
    INT_PTR itemID;
    EditLB->GetCurSelItemID(itemID);
    BOOL empty = FALSE;
    if (itemID == -1)
        empty = TRUE;

    CEditorMasksItem* item = NULL;
    if (!empty)
        item = (CEditorMasksItem*)itemID;
    DisableNotification = TRUE;
    SendMessage(GetDlgItem(HWindow, IDE_COMMAND), EM_LIMITTEXT, MAX_PATH - 1, 0);
    SendMessage(GetDlgItem(HWindow, IDE_ARGUMENTS), EM_LIMITTEXT, MAX_PATH - 1, 0);
    SendMessage(GetDlgItem(HWindow, IDE_INITDIR), EM_LIMITTEXT, MAX_PATH - 1, 0);
    SendMessage(GetDlgItem(HWindow, IDE_COMMAND), WM_SETTEXT, 0,
                (LPARAM)(empty ? "" : item->Command));
    SendMessage(GetDlgItem(HWindow, IDE_ARGUMENTS), WM_SETTEXT, 0,
                (LPARAM)(empty ? "" : item->Arguments));
    SendMessage(GetDlgItem(HWindow, IDE_ARGUMENTS), EM_SETSEL, 0, -1); // so the browse overwrites the content
    SendMessage(GetDlgItem(HWindow, IDE_INITDIR), WM_SETTEXT, 0,
                (LPARAM)(empty ? "" : item->InitDir));
    SendMessage(GetDlgItem(HWindow, IDE_INITDIR), EM_SETSEL, 0, -1); // so the browse overwrites the content
    DisableNotification = FALSE;
}

void CCfgPageEditors::StoreControls()
{
    int index;
    EditLB->GetCurSel(index);
    if (!DisableNotification && index >= 0 && index < EditLB->GetCount())
    {
        Dirty = TRUE;
        CEditorMasksItem* item = EditorMasks[index];

        char command[MAX_PATH];
        char arguments[MAX_PATH];
        char initdir[MAX_PATH];
        SendMessage(GetDlgItem(HWindow, IDE_COMMAND), WM_GETTEXT,
                    MAX_PATH, (LPARAM)command);
        SendMessage(GetDlgItem(HWindow, IDE_ARGUMENTS), WM_GETTEXT,
                    MAX_PATH, (LPARAM)arguments);
        SendMessage(GetDlgItem(HWindow, IDE_INITDIR), WM_GETTEXT,
                    MAX_PATH, (LPARAM)initdir);
        item->Set(item->Masks->GetMasksString(), command, arguments, initdir);
    }
}

void CCfgPageEditors::EnableControls()
{
    CALL_STACK_MESSAGE1("CCfgPageEditors::EnableControls()");
    BOOL validItem = TRUE;
    INT_PTR itemID;
    EditLB->GetCurSelItemID(itemID);
    if (itemID == -1)
        validItem = FALSE;

    EnableWindow(GetDlgItem(HWindow, IDE_COMMAND), validItem);
    EnableWindow(GetDlgItem(HWindow, IDE_ARGUMENTS), validItem);
    EnableWindow(GetDlgItem(HWindow, IDE_INITDIR), validItem);

    EnableWindow(GetDlgItem(HWindow, IDB_BROWSECOMMAND), validItem);
    EnableWindow(GetDlgItem(HWindow, IDB_BROWSEARGUMENTS), validItem);
    EnableWindow(GetDlgItem(HWindow, IDB_BROWSEINITDIR), validItem);
}

INT_PTR
CCfgPageEditors::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CCfgPageEditors::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        EditLB = new CEditListBox(HWindow, IDL_FILEMASKS);
        if (EditLB == NULL)
            TRACE_E(LOW_MEMORY);
        else
        {
            EditLB->MakeHeader(IDS_MASKSHDR);
            EditLB->EnableDrag(::GetParent(HWindow));
        }
        ChangeToArrowButton(HWindow, IDB_BROWSECOMMAND);
        ChangeToArrowButton(HWindow, IDB_BROWSEARGUMENTS);
        ChangeToArrowButton(HWindow, IDB_BROWSEINITDIR);

        // dialog elements should stretch with the dialog size, set split controls
        ElasticVerticalLayout(1, IDL_FILEMASKS);

        break;
    }

    case WM_USER_EDIT:
    {
        SetFocus(GetDlgItem(HWindow, IDL_FILEMASKS));
        EditLB->OnBeginEdit((int)wParam, (int)lParam);
        return 0;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == BN_CLICKED)
        {
            EnableControls();
            StoreControls();
        }

        if (LOWORD(wParam) == IDL_FILEMASKS && HIWORD(wParam) == LBN_SELCHANGE)
        {
            EditLB->OnSelChanged();
            EnableControls();
            LoadControls();
        }

        switch (LOWORD(wParam))
        {
        case IDB_BROWSECOMMAND:
        {
            TrackExecuteMenu(HWindow, IDB_BROWSECOMMAND, IDE_COMMAND, FALSE,
                             CommandExecutes, IDS_EXEFILTER);
            return 0;
        }

        case IDB_BROWSEARGUMENTS:
        {
            TrackExecuteMenu(HWindow, IDB_BROWSEARGUMENTS, IDE_ARGUMENTS, FALSE,
                             ArgumentsExecutes);
            return 0;
        }

        case IDB_BROWSEINITDIR:
        {
            TrackExecuteMenu(HWindow, IDB_BROWSEINITDIR, IDE_INITDIR, FALSE,
                             InitDirExecutes);
            return 0;
        }
        }
        break;
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
        case IDL_FILEMASKS:
        {
            switch (nmhdr->code)
            {
            case EDTLBN_GETDISPINFO:
            {
                EDTLB_DISPINFO* dispInfo = (EDTLB_DISPINFO*)lParam;
                if (dispInfo->ToDo == edtlbGetData)
                {
                    lstrcpyn(dispInfo->Buffer, ((CEditorMasksItem*)dispInfo->ItemID)->Masks->GetMasksString(), MAX_PATH);
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);
                    return TRUE;
                }
                else
                {
                    Dirty = TRUE;
                    CEditorMasksItem* item;
                    if (dispInfo->ItemID == -1)
                    {
                        item = new CEditorMasksItem();
                        if (item == NULL)
                        {
                            TRACE_E(LOW_MEMORY);
                            SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
                            return TRUE;
                        }
                        EditorMasks.Add(item);
                        item->Set(dispInfo->Buffer, item->Command, item->Arguments, item->InitDir);
                        EditLB->SetItemData((INT_PTR)item);
                    }
                    else
                    {
                        item = (CEditorMasksItem*)dispInfo->ItemID;
                        item->Set(dispInfo->Buffer, item->Command, item->Arguments, item->InitDir);
                    }

                    EnableControls();
                    LoadControls();
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

              char buf[sizeof(CEditorMasksItem)];
              memcpy(buf, EditorMasks[srcIndex], sizeof(CEditorMasksItem));
              memcpy(EditorMasks[srcIndex], EditorMasks[dstIndex], sizeof(CEditorMasksItem));
              memcpy(EditorMasks[dstIndex], buf, sizeof(CEditorMasksItem));

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

                char buf[sizeof(CEditorMasksItem)];
                memcpy(buf, EditorMasks[srcIndex], sizeof(CEditorMasksItem));
                if (srcIndex < dstIndex)
                {
                    int i;
                    for (i = srcIndex; i < dstIndex; i++)
                        memcpy(EditorMasks[i], EditorMasks[i + 1], sizeof(CEditorMasksItem));
                }
                else
                {
                    int i;
                    for (i = srcIndex; i > dstIndex; i--)
                        memcpy(EditorMasks[i], EditorMasks[i - 1], sizeof(CEditorMasksItem));
                }
                memcpy(EditorMasks[dstIndex], buf, sizeof(CEditorMasksItem));

                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE); // allow change
                return TRUE;
            }

            case EDTLBN_DELETEITEM:
            {
                int index;
                EditLB->GetCurSel(index);
                EditorMasks.Delete(index);
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
        if (idCtrl == IDL_FILEMASKS)
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
// CCfgPageMainWindow
//

CMainWindowIconItem MainWindowIcons[MAINWINDOWICONS_COUNT] =
    {
        {IDI_SALAMANDER_CLASSIC, IDS_SALAMANDERICON_DEFAULT}, // classic default icon
        {IDI_SALAMANDER_RED, IDS_SALAMANDERICON_RED},
        {IDI_SALAMANDER_GREEN, IDS_SALAMANDERICON_GREEN},
        {IDI_SALAMANDER_BLUE, IDS_SALAMANDERICON_BLUE},
        {IDI_SALAMANDER_SAMANDARIN, IDS_SALAMANDERICON_SAMANDARIN},
};

static const char* EXECUTE_TEMPLATE_DEFAULTCOMSPEC = "TemplateDefaultCOMSPEC";
static const char* EXECUTE_TEMPLATE_POWERSHELL = "TemplatePowerShell";
static const char* EXECUTE_TEMPLATE_POWERSHELL7 = "TemplatePowerShell7";
static const char* EXECUTE_TEMPLATE_POWERSHELL7_PATH = "C:\\Program Files\\PowerShell\\7\\pwsh.exe";

static CExecuteItem CommandShellApplicationExecutes[] =
    {
        {EXECUTE_BROWSE, IDS_EXECUTE_BROWSE, EIF_REPLACE_ALL},
        {EXECUTE_SEPARATOR, 0, 0},
        {EXECUTE_WINDIR, IDS_EXECUTE_WINDIR, EIF_VARIABLE},
        {EXECUTE_SYSDIR, IDS_EXECUTE_SYSDIR, EIF_VARIABLE},
        {EXECUTE_SALDIR, IDS_EXECUTE_SALDIR, EIF_VARIABLE},
        {EXECUTE_SEPARATOR, 0, 0},
        {EXECUTE_ENV, IDS_EXECUTE_ENV, EIF_CURSOR_1},
        {EXECUTE_SEPARATOR, 0, 0},
        {EXECUTE_SUBMENUSTART, IDS_EXECUTE_TEMPLATES, 0},
        {EXECUTE_TEMPLATE_DEFAULTCOMSPEC, IDS_EXECUTE_TEMPLATE_DEFAULTCOMSPEC, EIF_NO_INSERT},
        {EXECUTE_TEMPLATE_POWERSHELL, IDS_EXECUTE_TEMPLATE_POWERSHELL, EIF_NO_INSERT},
        {EXECUTE_TEMPLATE_POWERSHELL7, IDS_EXECUTE_TEMPLATE_POWERSHELL7, EIF_NO_INSERT},
        {EXECUTE_SUBMENUEND, 0, 0},
        {EXECUTE_TERMINATOR, 0, 0},
};


static BOOL FileExists(const char* path)
{
    DWORD attrs = GetFileAttributes(path);
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static BOOL FileExistsWPath(const wchar_t* path)
{
    DWORD attrs = GetFileAttributesW(path);
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static BOOL DirectoryExistsWPath(const wchar_t* path)
{
    DWORD attrs = GetFileAttributesW(path);
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static void AppendPathW(std::wstring& path, const wchar_t* suffix)
{
    if (!path.empty() && path[path.size() - 1] != L'\\')
        path += L'\\';
    path += suffix;
}

static std::string WideToAnsi(const std::wstring& value)
{
    if (value.empty())
        return std::string();
    int len = WideCharToMultiByte(CP_ACP, 0, value.c_str(), (int)value.size(), NULL, 0, NULL, NULL);
    if (len <= 0)
        return std::string();
    std::string out;
    out.resize(len);
    WideCharToMultiByte(CP_ACP, 0, value.c_str(), (int)value.size(), &out[0], len, NULL, NULL);
    return out;
}

static std::string Utf8ToAnsi(const std::string& value)
{
    if (value.empty())
        return std::string();
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), (int)value.size(), NULL, 0);
    if (wideLen <= 0)
        return value;
    std::wstring wide;
    wide.resize(wideLen);
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), (int)value.size(), &wide[0], wideLen);
    return WideToAnsi(wide);
}

static BOOL StoreWindowsTerminalPath(char* wtPath, int wtPathSize, const std::wstring& widePath)
{
    std::string path = WideToAnsi(widePath);
    if (path.empty())
        return FALSE;
    lstrcpyn(wtPath, path.c_str(), wtPathSize);
    return TRUE;
}

static BOOL FindWindowsTerminal(char* wtPath, int wtPathSize)
{
    wchar_t widePath[SAL_MAX_PATH];
    DWORD foundLen = SearchPathW(NULL, L"wt.exe", NULL, ARRAYSIZE(widePath), widePath, NULL);
    if (foundLen > 0 && foundLen < ARRAYSIZE(widePath) && StoreWindowsTerminalPath(wtPath, wtPathSize, widePath))
        return TRUE;

    wchar_t localAppData[SAL_MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, localAppData) == S_OK)
    {
        std::wstring path = localAppData;
        AppendPathW(path, L"Microsoft\\WindowsApps\\wt.exe");
        if (FileExistsWPath(path.c_str()) && StoreWindowsTerminalPath(wtPath, wtPathSize, path))
            return TRUE;
    }
    wtPath[0] = 0;
    return FALSE;
}

static void AddExistingSettingsPath(std::vector<std::wstring>& paths, const std::wstring& base, const wchar_t* suffix)
{
    std::wstring path = base;
    AppendPathW(path, suffix);
    if (FileExistsWPath(path.c_str()))
        paths.push_back(path);
}

static BOOL ReadTextFileWPath(const wchar_t* path, std::string& text)
{
    HANDLE file = HANDLES_Q(CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;
    DWORD size = GetFileSize(file, NULL);
    if (size == INVALID_FILE_SIZE || size > 1024 * 1024)
    {
        HANDLES(CloseHandle(file));
        return FALSE;
    }
    text.resize(size);
    DWORD read = 0;
    BOOL ok = size == 0 ? TRUE : ReadFile(file, &text[0], size, &read, NULL);
    HANDLES(CloseHandle(file));
    if (!ok)
        return FALSE;
    text.resize(read);
    return TRUE;
}

static void AddFragmentJsonFiles(std::vector<std::wstring>& paths, const std::wstring& directory, int depth = 0)
{
    if (depth > 6 || !DirectoryExistsWPath(directory.c_str()))
        return;

    std::wstring mask = directory;
    AppendPathW(mask, L"*");
    WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileW(mask.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE)
        return;
    do
    {
        if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0)
            continue;
        std::wstring child = directory;
        AppendPathW(child, data.cFileName);
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            AddFragmentJsonFiles(paths, child, depth + 1);
        else
        {
            const wchar_t* ext = wcsrchr(data.cFileName, L'.');
            if (ext != NULL && _wcsicmp(ext, L".json") == 0)
                paths.push_back(child);
        }
    } while (FindNextFileW(find, &data));
    FindClose(find);
}

struct CWindowsTerminalProfile
{
    std::string Name;
    std::string Guid;
    std::string CommandLine;
    std::string Icon;
    std::string Source;
};

static void InferWindowsTerminalProfileCommandLine(CWindowsTerminalProfile& profile);
static BOOL IsWindowsTerminalCommandLineSupported(const CWindowsTerminalProfile& profile);

static BOOL IsJsonEscaped(const std::string& json, size_t pos)
{
    size_t slashCount = 0;
    while (pos > 0 && json[--pos] == '\\')
        slashCount++;
    return (slashCount & 1) != 0;
}

static size_t FindJsonStringEnd(const std::string& json, size_t quote)
{
    for (size_t i = quote + 1; i < json.size(); i++)
        if (json[i] == '"' && !IsJsonEscaped(json, i))
            return i;
    return std::string::npos;
}

static BOOL SkipJsonComment(const std::string& json, size_t& pos)
{
    if (json[pos] != '/' || pos + 1 >= json.size())
        return FALSE;
    if (json[pos + 1] == '/')
    {
        pos += 2;
        while (pos < json.size() && json[pos] != '\r' && json[pos] != '\n')
            pos++;
        return TRUE;
    }
    if (json[pos + 1] == '*')
    {
        pos += 2;
        while (pos + 1 < json.size() && !(json[pos] == '*' && json[pos + 1] == '/'))
            pos++;
        if (pos + 1 < json.size())
            pos++;
        return TRUE;
    }
    return FALSE;
}

static void StripJsonComments(std::string& json)
{
    BOOL inString = FALSE;
    for (size_t i = 0; i < json.size(); i++)
    {
        if (json[i] == '"' && !IsJsonEscaped(json, i))
        {
            inString = !inString;
            continue;
        }
        if (inString || json[i] != '/' || i + 1 >= json.size())
            continue;

        size_t commentStart = i;
        if (SkipJsonComment(json, i))
        {
            for (size_t c = commentStart; c <= i && c < json.size(); c++)
                if (json[c] != '\r' && json[c] != '\n')
                    json[c] = ' ';
        }
    }
}

static size_t FindMatchingJsonChar(const std::string& json, size_t open, char openCh, char closeCh)
{
    int depth = 0;
    for (size_t i = open; i < json.size(); i++)
    {
        if (json[i] == '"')
        {
            i = FindJsonStringEnd(json, i);
            if (i == std::string::npos)
                return std::string::npos;
            continue;
        }
        if (json[i] == openCh)
            depth++;
        else if (json[i] == closeCh && --depth == 0)
            return i;
    }
    return std::string::npos;
}

static void AppendUtf8Codepoint(std::string& value, unsigned code)
{
    if (code < 0x80)
        value.push_back((char)code);
    else if (code < 0x800)
    {
        value.push_back((char)(0xC0 | (code >> 6)));
        value.push_back((char)(0x80 | (code & 0x3F)));
    }
    else
    {
        value.push_back((char)(0xE0 | (code >> 12)));
        value.push_back((char)(0x80 | ((code >> 6) & 0x3F)));
        value.push_back((char)(0x80 | (code & 0x3F)));
    }
}

static int HexDigitValue(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return -1;
}

static BOOL FindJsonStringProperty(const std::string& json, size_t objectStart, size_t objectEnd, const char* property, std::string& value)
{
    std::string key = "\"";
    key += property;
    key += "\"";
    size_t pos = objectStart;
    while ((pos = json.find(key, pos)) != std::string::npos && pos < objectEnd)
    {
        if (pos > objectStart && (isalnum((unsigned char)json[pos - 1]) || json[pos - 1] == '_'))
        {
            pos += key.size();
            continue;
        }
        size_t colon = json.find(':', pos + key.size());
        if (colon == std::string::npos || colon >= objectEnd)
            return FALSE;
        size_t quote = colon + 1;
        while (quote < objectEnd && (json[quote] == ' ' || json[quote] == '\t' || json[quote] == '\r' || json[quote] == '\n'))
            quote++;
        if (quote >= objectEnd || json[quote] != '"')
            return FALSE;

        value.clear();
        for (size_t i = quote + 1; i < objectEnd; i++)
        {
            char ch = json[i];
            if (ch == '"')
                return TRUE;
            if (ch == '\\' && i + 1 < objectEnd)
            {
                ch = json[++i];
                switch (ch)
                {
                case 'n': ch = '\n'; break;
                case 'r': ch = '\r'; break;
                case 't': ch = '\t'; break;
                case 'u':
                {
                    if (i + 4 < objectEnd)
                    {
                        unsigned code = 0;
                        BOOL ok = TRUE;
                        for (int h = 0; h < 4; h++)
                        {
                            int v = HexDigitValue(json[i + 1 + h]);
                            if (v < 0)
                                ok = FALSE;
                            code = (code << 4) | (unsigned)max(v, 0);
                        }
                        if (ok)
                        {
                            AppendUtf8Codepoint(value, code);
                            i += 4;
                            continue;
                        }
                    }
                    break;
                }
                }
            }
            value.push_back(ch);
        }
        return FALSE;
    }
    return FALSE;
}


static BOOL FindJsonBoolProperty(const std::string& json, size_t objectStart, size_t objectEnd, const char* property, BOOL& value)
{
    std::string key = "\"";
    key += property;
    key += "\"";
    size_t pos = objectStart;
    while ((pos = json.find(key, pos)) != std::string::npos && pos < objectEnd)
    {
        if (pos > objectStart && (isalnum((unsigned char)json[pos - 1]) || json[pos - 1] == '_'))
        {
            pos += key.size();
            continue;
        }
        size_t colon = json.find(':', pos + key.size());
        if (colon == std::string::npos || colon >= objectEnd)
            return FALSE;
        size_t boolStart = colon + 1;
        while (boolStart < objectEnd && (json[boolStart] == ' ' || json[boolStart] == '\t' || json[boolStart] == '\r' || json[boolStart] == '\n'))
            boolStart++;
        if (boolStart + 4 <= objectEnd && memcmp(json.c_str() + boolStart, "true", 4) == 0)
        {
            value = TRUE;
            return TRUE;
        }
        if (boolStart + 5 <= objectEnd && memcmp(json.c_str() + boolStart, "false", 5) == 0)
        {
            value = FALSE;
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

static BOOL FindJsonPropertyObjectOrArray(const std::string& json, size_t objectStart, size_t objectEnd, const char* property,
                                          char openCh, char closeCh, size_t& valueStart, size_t& valueEnd)
{
    std::string key = "\"";
    key += property;
    key += "\"";
    size_t pos = json.find(key, objectStart);
    if (pos == std::string::npos || pos >= objectEnd)
        return FALSE;
    pos = json.find(':', pos + key.size());
    if (pos == std::string::npos || pos >= objectEnd)
        return FALSE;
    pos++;
    while (pos < objectEnd && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n'))
        pos++;
    if (pos >= objectEnd || json[pos] != openCh)
        return FALSE;
    size_t end = FindMatchingJsonChar(json, pos, openCh, closeCh);
    if (end == std::string::npos || end >= objectEnd)
        return FALSE;
    valueStart = pos;
    valueEnd = end;
    return TRUE;
}

static BOOL IsSameWindowsTerminalProfileIdentity(const CWindowsTerminalProfile& profile, const std::string& name, const std::string& guid)
{
    if (!guid.empty() && !profile.Guid.empty())
        return _stricmp(profile.Guid.c_str(), guid.c_str()) == 0;
    return !profile.Name.empty() && _stricmp(profile.Name.c_str(), name.c_str()) == 0;
}

static void AddWindowsTerminalProfileFromObject(const std::string& json, size_t objectStart, size_t objectEnd,
                                                std::vector<CWindowsTerminalProfile>& profiles,
                                                std::vector<CWindowsTerminalProfile>& hiddenProfiles)
{
    std::string name;
    BOOL hasName = FindJsonStringProperty(json, objectStart, objectEnd, "name", name) && !name.empty();
    if (hasName)
        name = Utf8ToAnsi(name);

    std::string guid;
    if (FindJsonStringProperty(json, objectStart, objectEnd, "guid", guid))
        guid = Utf8ToAnsi(guid);

    BOOL hidden = FALSE;
    if (FindJsonBoolProperty(json, objectStart, objectEnd, "hidden", hidden) && hidden)
    {
        if (hasName || !guid.empty())
        {
            CWindowsTerminalProfile hiddenProfile;
            hiddenProfile.Name = name;
            hiddenProfile.Guid = guid;
            hiddenProfiles.push_back(hiddenProfile);
        }
        return;
    }

    if (!hasName)
        return;

    BOOL duplicate = FALSE;
    for (size_t i = 0; i < profiles.size(); i++)
        if (IsSameWindowsTerminalProfileIdentity(profiles[i], name, guid))
            duplicate = TRUE;
    for (size_t i = 0; i < hiddenProfiles.size(); i++)
        if (IsSameWindowsTerminalProfileIdentity(hiddenProfiles[i], name, guid))
            duplicate = TRUE;
    if (duplicate)
        return;

    CWindowsTerminalProfile profile;
    profile.Name = name;
    profile.Guid = guid;
    if (FindJsonStringProperty(json, objectStart, objectEnd, "commandline", profile.CommandLine))
        profile.CommandLine = Utf8ToAnsi(profile.CommandLine);
    if (FindJsonStringProperty(json, objectStart, objectEnd, "icon", profile.Icon))
        profile.Icon = Utf8ToAnsi(profile.Icon);
    if (FindJsonStringProperty(json, objectStart, objectEnd, "source", profile.Source))
        profile.Source = Utf8ToAnsi(profile.Source);
    InferWindowsTerminalProfileCommandLine(profile);
    if (!IsWindowsTerminalCommandLineSupported(profile))
        return;
    profiles.push_back(profile);
}

static BOOL FindNextProfileObjectInArray(const std::string& json, size_t arrayEnd, size_t& pos,
                                         size_t& objectStart, size_t& objectEnd)
{
    while (pos < arrayEnd)
    {
        if (json[pos] == '"')
        {
            pos = FindJsonStringEnd(json, pos);
            if (pos == std::string::npos)
                return FALSE;
        }
        else if (json[pos] == '/')
        {
            if (!SkipJsonComment(json, pos))
                pos++;
        }
        else if (json[pos] == '{')
        {
            objectStart = pos;
            objectEnd = FindMatchingJsonChar(json, objectStart, '{', '}');
            if (objectEnd == std::string::npos || objectEnd > arrayEnd)
                return FALSE;
            pos = objectEnd + 1;
            return TRUE;
        }
        else
            pos++;
    }
    return FALSE;
}

static void CollectProfilesFromArray(const std::string& json, size_t arrayStart, size_t arrayEnd,
                                     std::vector<CWindowsTerminalProfile>& profiles,
                                     std::vector<CWindowsTerminalProfile>& hiddenProfiles)
{
    size_t pos = arrayStart + 1;
    size_t objectStart;
    size_t objectEnd;
    while (FindNextProfileObjectInArray(json, arrayEnd, pos, objectStart, objectEnd))
        AddWindowsTerminalProfileFromObject(json, objectStart, objectEnd, profiles, hiddenProfiles);
}

static void CollectProfilesFromJson(const std::string& json, std::vector<CWindowsTerminalProfile>& profiles,
                                    std::vector<CWindowsTerminalProfile>& hiddenProfiles)
{
    size_t rootEnd = json.size();
    size_t profilesStart;
    size_t profilesEnd;
    if (FindJsonPropertyObjectOrArray(json, 0, rootEnd, "profiles", '{', '}', profilesStart, profilesEnd))
    {
        size_t listStart;
        size_t listEnd;
        if (FindJsonPropertyObjectOrArray(json, profilesStart, profilesEnd, "list", '[', ']', listStart, listEnd))
            CollectProfilesFromArray(json, listStart, listEnd, profiles, hiddenProfiles);
    }
    else if (FindJsonPropertyObjectOrArray(json, 0, rootEnd, "profiles", '[', ']', profilesStart, profilesEnd))
        CollectProfilesFromArray(json, profilesStart, profilesEnd, profiles, hiddenProfiles);
}

static void AddWindowsTerminalSettingsPaths(std::vector<std::wstring>& paths, const std::wstring& localAppData)
{
    // Prefer the settings store used by the normal wt.exe alias; fall back to Preview/unpackaged only when needed.
    size_t before = paths.size();
    AddExistingSettingsPath(paths, localAppData, L"Packages\\Microsoft.WindowsTerminal_8wekyb3d8bbwe\\LocalState\\settings.json");
    if (paths.size() == before)
        AddExistingSettingsPath(paths, localAppData, L"Packages\\Microsoft.WindowsTerminalPreview_8wekyb3d8bbwe\\LocalState\\settings.json");
    if (paths.size() == before)
        AddExistingSettingsPath(paths, localAppData, L"Microsoft\\Windows Terminal\\settings.json");
}

static void CollectWindowsTerminalProfiles(std::vector<CWindowsTerminalProfile>& profiles)
{
    wchar_t localAppData[SAL_MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, localAppData) != S_OK)
        return;

    std::vector<CWindowsTerminalProfile> hiddenProfiles;

    std::vector<std::wstring> paths;
    AddWindowsTerminalSettingsPaths(paths, localAppData);

    std::wstring fragments = localAppData;
    AppendPathW(fragments, L"Microsoft\\Windows Terminal\\Fragments");
    AddFragmentJsonFiles(paths, fragments);

    wchar_t programData[SAL_MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, programData) == S_OK)
    {
        fragments = programData;
        AppendPathW(fragments, L"Microsoft\\Windows Terminal\\Fragments");
        AddFragmentJsonFiles(paths, fragments);
    }

    for (size_t p = 0; p < paths.size(); p++)
    {
        std::string json;
        if (ReadTextFileWPath(paths[p].c_str(), json))
        {
            StripJsonComments(json);
            CollectProfilesFromJson(json, profiles, hiddenProfiles);
        }
    }
}

static void AppendMenuQuotedArg(char* args, int argsSize, const char* text)
{
    strncat_s(args, argsSize, "\"", _TRUNCATE);
    for (const char* s = text; *s != 0; s++)
    {
        if (*s == '"')
            strncat_s(args, argsSize, "\\\"", _TRUNCATE);
        else
        {
            char ch[2] = {*s, 0};
            strncat_s(args, argsSize, ch, _TRUNCATE);
        }
    }
    strncat_s(args, argsSize, "\"", _TRUNCATE);
}

static void AppendQuotedArgString(std::string& args, const char* text)
{
    args += '"';
    for (const char* s = text; *s != 0; s++)
    {
        if (*s == '"')
            args += "\\\"";
        else
            args += *s;
    }
    args += '"';
}

static const char* FindTextI(const char* text, const char* needle)
{
    size_t needleLen = strlen(needle);
    if (needleLen == 0)
        return text;
    for (const char* s = text; *s != 0; s++)
        if (_strnicmp(s, needle, needleLen) == 0)
            return s;
    return NULL;
}

static BOOL ContainsTextI(const std::string& text, const char* needle)
{
    size_t needleLen = strlen(needle);
    if (needleLen == 0)
        return TRUE;
    for (size_t i = 0; i < text.size(); i++)
        if (_strnicmp(text.c_str() + i, needle, needleLen) == 0)
            return TRUE;
    return FALSE;
}

static void CopyFirstCommandToken(const char* commandLine, char* exe, int exeSize)
{
    exe[0] = 0;
    while (*commandLine == ' ' || *commandLine == '\t')
        commandLine++;
    BOOL quoted = *commandLine == '"';
    if (quoted)
        commandLine++;
    int pos = 0;
    while (*commandLine != 0 && pos < exeSize - 1)
    {
        if (quoted ? *commandLine == '"' : (*commandLine == ' ' || *commandLine == '\t'))
            break;
        exe[pos++] = *commandLine++;
    }
    exe[pos] = 0;
}

static BOOL ContainsCommandLineSwitch(const std::string& commandLine, const char* sw)
{
    size_t swLen = strlen(sw);
    for (size_t i = 0; i < commandLine.size(); i++)
    {
        if ((i == 0 || commandLine[i - 1] == ' ' || commandLine[i - 1] == '\t' || commandLine[i - 1] == '"') &&
            _strnicmp(commandLine.c_str() + i, sw, swLen) == 0 &&
            (commandLine[i + swLen] == 0 || commandLine[i + swLen] == ' ' || commandLine[i + swLen] == '\t' || commandLine[i + swLen] == '"'))
            return TRUE;
    }
    return FALSE;
}

static BOOL IsBashLikeCommandLine(const std::string& commandLine)
{
    return ContainsTextI(commandLine, "bash") || ContainsTextI(commandLine, "sh.exe") ||
           ContainsTextI(commandLine, "zsh") || ContainsTextI(commandLine, "cygwin") ||
           ContainsTextI(commandLine, "git-bash") || ContainsTextI(commandLine, "mingw") ||
           ContainsTextI(commandLine, "msys");
}

static BOOL IsWindowsTerminalCommandLineSupported(const CWindowsTerminalProfile& profile)
{
    return !profile.CommandLine.empty() &&
           (ContainsTextI(profile.CommandLine, "pwsh") || ContainsTextI(profile.CommandLine, "powershell") ||
            ContainsTextI(profile.CommandLine, "cmd") || ContainsTextI(profile.CommandLine, "wsl") ||
            IsBashLikeCommandLine(profile.CommandLine));
}

static void AppendProfileCommandLine(char* args, int argsSize, const std::string& commandLine);

static const char* FindCommandLineSwitch(const std::string& commandLine, const char* sw)
{
    size_t swLen = strlen(sw);
    for (size_t i = 0; i < commandLine.size(); i++)
    {
        if ((i == 0 || commandLine[i - 1] == ' ' || commandLine[i - 1] == '\t' || commandLine[i - 1] == '"') &&
            _strnicmp(commandLine.c_str() + i, sw, swLen) == 0 &&
            (commandLine[i + swLen] == 0 || commandLine[i + swLen] == ' ' || commandLine[i + swLen] == '\t' || commandLine[i + swLen] == '"'))
            return commandLine.c_str() + i;
    }
    return NULL;
}

static BOOL AppendComposedBashCommand(char* args, int argsSize, const std::string& commandLine)
{
    const char* commandSwitch = FindCommandLineSwitch(commandLine, "-lc");
    if (commandSwitch == NULL)
        commandSwitch = FindCommandLineSwitch(commandLine, "-c");
    if (commandSwitch == NULL)
        return FALSE;

    const char* commandArgument = commandSwitch;
    while (*commandArgument != 0 && *commandArgument != ' ' && *commandArgument != '\t')
        commandArgument++;
    while (*commandArgument == ' ' || *commandArgument == '\t')
        commandArgument++;
    if (*commandArgument != '"')
        return FALSE;

    const char* commandEnd = commandArgument + 1;
    BOOL escaped = FALSE;
    while (*commandEnd != 0)
    {
        if (!escaped && *commandEnd == '"')
            break;
        escaped = !escaped && *commandEnd == '\\';
        if (*commandEnd != '\\')
            escaped = FALSE;
        commandEnd++;
    }
    if (*commandEnd != '"')
        return FALSE;

    AppendProfileCommandLine(args, argsSize, std::string(commandLine.c_str(), commandEnd - commandLine.c_str()));
    strncat_s(args, argsSize, "; eval {command}", _TRUNCATE);
    strncat_s(args, argsSize, commandEnd, _TRUNCATE);
    return TRUE;
}

static void AppendBashCommandExecution(char* args, int argsSize, const std::string& commandLine)
{
    // Git Bash/Cygwin/MSYS Windows Terminal profiles usually keep startup flags in the
    // profile commandline.  Do not append {command} as a positional script argument;
    // ask the shell to execute it explicitly.  If the profile already has -c/-lc, merge
    // Salamander's command into that existing shell command string.
    if (AppendComposedBashCommand(args, argsSize, commandLine))
        return;

    AppendProfileCommandLine(args, argsSize, commandLine);
    if (ContainsCommandLineSwitch(commandLine, "--login") || ContainsCommandLineSwitch(commandLine, "-l"))
        strncat_s(args, argsSize, " -c \"eval {command}\"", _TRUNCATE);
    else
        strncat_s(args, argsSize, " -lc \"eval {command}\"", _TRUNCATE);
}

static void AppendProfileCommandLine(char* args, int argsSize, const std::string& commandLine)
{
    const char* start = commandLine.c_str();
    while (*start == ' ' || *start == '\t')
        start++;
    strncat_s(args, argsSize, " ", _TRUNCATE);
    if (*start == '"')
    {
        strncat_s(args, argsSize, start, _TRUNCATE);
        return;
    }

    const char* tokenEnd = start;
    BOOL firstTokenCanBePathWithSpaces = FALSE;
    while (*tokenEnd != 0 && *tokenEnd != ' ' && *tokenEnd != '\t')
    {
        if (*tokenEnd == ':' || *tokenEnd == '\\' || *tokenEnd == '/')
            firstTokenCanBePathWithSpaces = TRUE;
        tokenEnd++;
    }

    if (firstTokenCanBePathWithSpaces)
    {
        // Only complete and quote the launcher token.  Do not scan into later
        // arguments such as cmd.exe /k "...\VsDevCmd.bat", where the .bat is an
        // initializer argument rather than the Windows Terminal commandline launcher.
        const char* searchEnd = start;
        while (*searchEnd != 0 && *searchEnd != '"')
            searchEnd++;

        const char* extensions[] = {".exe", ".cmd", ".bat", ".com"};
        const char* exeEnd = NULL;
        for (int e = 0; e < _countof(extensions); e++)
        {
            const char* found = FindTextI(start, extensions[e]);
            if (found != NULL && found < searchEnd && (exeEnd == NULL || found < exeEnd))
                exeEnd = found + strlen(extensions[e]);
        }
        if (exeEnd != NULL)
        {
            BOOL needsQuotes = FALSE;
            for (const char* s = start; s < exeEnd; s++)
                if (*s == ' ' || *s == '\t')
                    needsQuotes = TRUE;
            if (needsQuotes)
            {
                strncat_s(args, argsSize, "\"", _TRUNCATE);
                size_t len = exeEnd - start;
                std::string exe(start, len);
                strncat_s(args, argsSize, exe.c_str(), _TRUNCATE);
                strncat_s(args, argsSize, "\"", _TRUNCATE);
                strncat_s(args, argsSize, exeEnd, _TRUNCATE);
                return;
            }
        }
    }
    strncat_s(args, argsSize, start, _TRUNCATE);
}

static void InferWindowsTerminalProfileCommandLine(CWindowsTerminalProfile& profile)
{
    if (!profile.CommandLine.empty())
        return;
    if (ContainsTextI(profile.Source, "Windows.Terminal.PowershellCore") ||
        ContainsTextI(profile.Name, "PowerShell 7") || ContainsTextI(profile.Name, "pwsh"))
        profile.CommandLine = "pwsh.exe";
    else if (ContainsTextI(profile.Source, "Windows.Terminal.WindowsPowerShell") ||
             ContainsTextI(profile.Name, "Windows PowerShell"))
        profile.CommandLine = "powershell.exe";
    else if (ContainsTextI(profile.Name, "PowerShell"))
        profile.CommandLine = "pwsh.exe";
    else if (ContainsTextI(profile.Name, "Command Prompt") || ContainsTextI(profile.Name, "cmd"))
        profile.CommandLine = "cmd.exe";
    else if (ContainsTextI(profile.Name, "Ubuntu") || ContainsTextI(profile.Name, "Debian") ||
             ContainsTextI(profile.Name, "SLES") || ContainsTextI(profile.Name, "kali") ||
             ContainsTextI(profile.Source, "Windows.Terminal.Wsl"))
    {
        profile.CommandLine = "wsl.exe -d ";
        AppendQuotedArgString(profile.CommandLine, profile.Name.c_str());
    }
}

static void AppendWindowsTerminalProfileCommand(char* args, int argsSize, const CWindowsTerminalProfile& profile)
{
    if (IsBashLikeCommandLine(profile.CommandLine))
    {
        AppendBashCommandExecution(args, argsSize, profile.CommandLine);
        return;
    }

    AppendProfileCommandLine(args, argsSize, profile.CommandLine);
    if (ContainsTextI(profile.CommandLine, "pwsh") || ContainsTextI(profile.CommandLine, "powershell"))
    {
        if (ContainsCommandLineSwitch(profile.CommandLine, "-Command") ||
            ContainsCommandLineSwitch(profile.CommandLine, "-CommandWithArgs") ||
            ContainsCommandLineSwitch(profile.CommandLine, "-c"))
            strncat_s(args, argsSize, " \"; {command}\"", _TRUNCATE);
        else
            strncat_s(args, argsSize, " -NoExit -Command \"& {command}\"", _TRUNCATE);
    }
    else if (ContainsTextI(profile.CommandLine, "cmd"))
    {
        if (ContainsCommandLineSwitch(profile.CommandLine, "/K") || ContainsCommandLineSwitch(profile.CommandLine, "/C"))
            strncat_s(args, argsSize, " & \"{command}\"", _TRUNCATE);
        else
            strncat_s(args, argsSize, " /K \"{command}\"", _TRUNCATE);
    }
    else if (ContainsTextI(profile.CommandLine, "wsl"))
        strncat_s(args, argsSize, " --exec sh -lc \"eval {command}\"", _TRUNCATE);
}

static void SetWindowsTerminalProfileTemplate(HWND hWindow, const char* wtPath, const CWindowsTerminalProfile& profile)
{
    char args[CONFIG_COMMANDLINEARGS_MAXLEN];
    lstrcpyn(args, "-d . -p ", ARRAYSIZE(args));
    AppendMenuQuotedArg(args, ARRAYSIZE(args), !profile.Guid.empty() ? profile.Guid.c_str() : profile.Name.c_str());
    AppendWindowsTerminalProfileCommand(args, ARRAYSIZE(args), profile);

    SetDlgItemText(hWindow, IDC_CMDLINEAPP_PATH, wtPath[0] != 0 ? wtPath : "wt.exe");
    SetDlgItemText(hWindow, IDC_CMDLINEAPP_ARGS, args);
}

static int GetMenuIconSize()
{
    return GetIconSizeForSystemDPI(ICONSIZE_16);
}

static HBITMAP CreateMenuBitmapFromIcon(HICON hIcon)
{
    if (hIcon == NULL)
        return NULL;
    int iconSize = GetMenuIconSize();
    HDC hDC = HANDLES(GetDC(NULL));
    HDC hMemDC = HANDLES(CreateCompatibleDC(hDC));
    BITMAPINFOHEADER bmhdr;
    memset(&bmhdr, 0, sizeof(bmhdr));
    bmhdr.biSize = sizeof(bmhdr);
    bmhdr.biWidth = iconSize;
    bmhdr.biHeight = -iconSize;
    bmhdr.biPlanes = 1;
    bmhdr.biBitCount = 32;
    bmhdr.biCompression = BI_RGB;
    void* bits = NULL;
    HBITMAP hBitmap = HANDLES(CreateDIBSection(hMemDC, (CONST BITMAPINFO*)&bmhdr, DIB_RGB_COLORS, &bits, NULL, 0));
    if (hBitmap != NULL && bits != NULL)
    {
        memset(bits, 0, static_cast<size_t>(iconSize) * iconSize * 4);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
        DrawIconEx(hMemDC, 0, 0, hIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);
        SelectObject(hMemDC, hOldBitmap);
    }
    HANDLES(DeleteDC(hMemDC));
    HANDLES(ReleaseDC(NULL, hDC));
    return hBitmap;
}


static HBITMAP LoadBuiltinShellSVG(const char* svgName)
{
    HBITMAP hBitmap = NULL;
    RenderSVGIconBitmap(svgName, GetMenuIconSize(), TRUE, &hBitmap);
    return hBitmap;
}

static BOOL IsDeveloperProfile(const CWindowsTerminalProfile& profile)
{
    return ContainsTextI(profile.Name, "Developer ") &&
           (ContainsTextI(profile.Name, "Visual Studio") || ContainsTextI(profile.Name, " VS"));
}

static HBITMAP ComposeVisualStudioOverlay(HBITMAP hBase)
{
    HBITMAP hVisualStudio = LoadBuiltinShellSVG("VisualStudio");
    if (hBase == NULL || hVisualStudio == NULL)
    {
        if (hVisualStudio != NULL)
            HANDLES(DeleteObject(hVisualStudio));
        return hBase;
    }

    HDC hDC = HANDLES(GetDC(NULL));
    HDC hDstDC = HANDLES(CreateCompatibleDC(hDC));
    HDC hSrcDC = HANDLES(CreateCompatibleDC(hDC));
    HBITMAP hOldDst = (HBITMAP)SelectObject(hDstDC, hBase);
    HBITMAP hOldSrc = (HBITMAP)SelectObject(hSrcDC, hVisualStudio);
    BLENDFUNCTION bf;
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;
    int iconSize = GetMenuIconSize();
    int overlaySize = max(1, iconSize / 2);
    AlphaBlend(hDstDC, iconSize - overlaySize, iconSize - overlaySize, overlaySize, overlaySize,
               hSrcDC, 0, 0, iconSize, iconSize, bf);
    SelectObject(hSrcDC, hOldSrc);
    SelectObject(hDstDC, hOldDst);
    HANDLES(DeleteDC(hSrcDC));
    HANDLES(DeleteDC(hDstDC));
    HANDLES(ReleaseDC(NULL, hDC));
    HANDLES(DeleteObject(hVisualStudio));
    return hBase;
}

static HBITMAP LoadBuiltinWindowsTerminalProfileBitmap(const CWindowsTerminalProfile& profile)
{
    HBITMAP hBitmap = NULL;
    if (ContainsTextI(profile.Name, "Azure Cloud Shell"))
        hBitmap = LoadBuiltinShellSVG("AzureCloudShell");
    else if (ContainsTextI(profile.CommandLine, "pwsh") || ContainsTextI(profile.Name, "PowerShell 7"))
        hBitmap = LoadBuiltinShellSVG("PowerShell");
    else if (ContainsTextI(profile.CommandLine, "powershell") || ContainsTextI(profile.Name, "Windows PowerShell"))
        hBitmap = LoadBuiltinShellSVG("WindowsPowerShell");
    else if (ContainsTextI(profile.Name, "PowerShell"))
        hBitmap = LoadBuiltinShellSVG("PowerShell");

    if (IsDeveloperProfile(profile))
    {
        if (hBitmap == NULL)
        {
            CWindowsTerminalProfile inferred = profile;
            InferWindowsTerminalProfileCommandLine(inferred);
            if (ContainsTextI(inferred.CommandLine, "cmd"))
                hBitmap = LoadBuiltinShellSVG("CommandPrompt");
            else if (ContainsTextI(inferred.CommandLine, "powershell"))
                hBitmap = LoadBuiltinShellSVG("WindowsPowerShell");
            else if (ContainsTextI(inferred.CommandLine, "pwsh"))
                hBitmap = LoadBuiltinShellSVG("PowerShell");
        }
        hBitmap = ComposeVisualStudioOverlay(hBitmap);
    }
    return hBitmap;
}

static int GetWindowsTerminalProfileImageIndex(const CWindowsTerminalProfile& profile)
{
    if (IsDeveloperProfile(profile))
        return -1; // developer profiles use composed base-shell + Visual Studio overlay bitmaps
    if (ContainsTextI(profile.Name, "Command Prompt") || ContainsTextI(profile.Name, "cmd"))
        return IDX_TB_COMMANDPROMPT;
    if (ContainsTextI(profile.Name, "Azure Cloud Shell"))
        return IDX_TB_AZURECLOUDSHELL;
    if (_stricmp(profile.Name.c_str(), "PowerShell") == 0 ||
        ContainsTextI(profile.CommandLine, "pwsh") || ContainsTextI(profile.Name, "PowerShell 7"))
        return IDX_TB_POWERSHELL;
    if (ContainsTextI(profile.CommandLine, "powershell") || ContainsTextI(profile.Name, "Windows PowerShell"))
        return IDX_TB_WINDOWSPOWERSHELL;
    if (ContainsTextI(profile.Name, "PowerShell"))
        return IDX_TB_POWERSHELL;
    return -1;
}

static HBITMAP LoadWindowsTerminalProfileBitmap(const CWindowsTerminalProfile& profile)
{
    HBITMAP hBuiltin = LoadBuiltinWindowsTerminalProfileBitmap(profile);
    if (hBuiltin != NULL)
        return hBuiltin;

    SHFILEINFO shfi;
    memset(&shfi, 0, sizeof(shfi));
    if (!profile.Icon.empty() && FileExists(profile.Icon.c_str()) &&
        SHGetFileInfo(profile.Icon.c_str(), 0, &shfi, sizeof(shfi), SHGFI_ICON | SHGFI_SMALLICON) != 0)
    {
        HBITMAP hBitmap = CreateMenuBitmapFromIcon(shfi.hIcon);
        DestroyIcon(shfi.hIcon);
        return hBitmap;
    }
    if (!profile.CommandLine.empty())
    {
        char exe[SAL_MAX_PATH];
        CopyFirstCommandToken(profile.CommandLine.c_str(), exe, ARRAYSIZE(exe));
        if (FileExists(exe) && SHGetFileInfo(exe, 0, &shfi, sizeof(shfi), SHGFI_ICON | SHGFI_SMALLICON) != 0)
        {
            HBITMAP hBitmap = CreateMenuBitmapFromIcon(shfi.hIcon);
            DestroyIcon(shfi.hIcon);
            return hBitmap;
        }
    }
    return NULL;
}


static HICON CreateIconFromBitmap(HBITMAP hColorBitmap, int iconSize)
{
    if (hColorBitmap == NULL)
        return NULL;

    const int maskStride = ((iconSize + 15) / 16) * 2;
    BYTE* maskBits = (BYTE*)calloc(maskStride, iconSize);
    if (maskBits == NULL)
        return NULL;

    BITMAP bitmap;
    memset(&bitmap, 0, sizeof(bitmap));
    if (GetObject(hColorBitmap, sizeof(bitmap), &bitmap) == sizeof(bitmap) &&
        bitmap.bmBits != NULL && bitmap.bmBitsPixel == 32)
    {
        BYTE* colorBits = (BYTE*)bitmap.bmBits;
        int width = min(iconSize, bitmap.bmWidth);
        int height = min(iconSize, abs(bitmap.bmHeight));
        for (int y = 0; y < height; y++)
        {
            BYTE* src = colorBits + y * bitmap.bmWidthBytes;
            BYTE* dst = maskBits + y * maskStride;
            for (int x = 0; x < width; x++)
            {
                BYTE alpha = src[x * 4 + 3];
                if (alpha < 16)
                    dst[x / 8] |= 0x80 >> (x % 8); // 1 means transparent in an icon mask
            }
        }
    }

    HBITMAP hMaskBitmap = HANDLES(CreateBitmap(iconSize, iconSize, 1, 1, maskBits));
    free(maskBits);
    if (hMaskBitmap == NULL)
        return NULL;

    ICONINFO iconInfo;
    memset(&iconInfo, 0, sizeof(iconInfo));
    iconInfo.fIcon = TRUE;
    iconInfo.hbmMask = hMaskBitmap;
    iconInfo.hbmColor = hColorBitmap;
    HICON hIcon = CreateIconIndirect(&iconInfo);
    HANDLES(DeleteObject(hMaskBitmap));
    return hIcon;
}

static HICON CreateMenuIconFromBitmap(HBITMAP hBitmap)
{
    if (hBitmap == NULL)
        return NULL;

    HICON hIcon = CreateIconFromBitmap(hBitmap, GetMenuIconSize());
    HANDLES(DeleteObject(hBitmap));
    return hIcon;
}

static HICON AddMenuIcon(std::vector<HICON>& icons, HICON hIcon)
{
    if (hIcon != NULL)
        icons.push_back(hIcon);
    return hIcon;
}

static void DestroyMenuIcons(std::vector<HICON>& icons)
{
    for (size_t i = 0; i < icons.size(); i++)
        DestroyIcon(icons[i]);
    icons.clear();
}

static BOOL InsertCommandShellMenuItem(CMenuPopup* popup, DWORD id, const char* text, int imageIndex = -1, HICON hIcon = NULL, CMenuPopup* subMenu = NULL)
{
    MENU_ITEM_INFO mii;
    memset(&mii, 0, sizeof(mii));
    mii.Mask = MENU_MASK_TYPE | MENU_MASK_ID | MENU_MASK_STRING | MENU_MASK_STATE;
    mii.Type = MENU_TYPE_STRING;
    mii.ID = id;
    mii.String = (char*)text;
    mii.State = 0;
    if (imageIndex >= 0)
    {
        mii.Mask |= MENU_MASK_IMAGEINDEX;
        mii.ImageIndex = imageIndex;
    }
    else if (hIcon != NULL)
    {
        mii.Mask |= MENU_MASK_ICON;
        mii.HIcon = hIcon;
    }
    if (subMenu != NULL)
    {
        mii.Mask |= MENU_MASK_SUBMENU;
        mii.SubMenu = subMenu;
    }
    return popup->InsertItem(0xFFFFFFFF, TRUE, &mii);
}

static BOOL InsertCommandShellMenuSeparator(CMenuPopup* popup)
{
    MENU_ITEM_INFO mii;
    memset(&mii, 0, sizeof(mii));
    mii.Mask = MENU_MASK_TYPE;
    mii.Type = MENU_TYPE_SEPARATOR;
    return popup->InsertItem(0xFFFFFFFF, TRUE, &mii);
}

static CMenuPopup* NewCommandShellMenuPopup()
{
#ifdef new
#undef new
#define RESTORE_DEBUG_NEW_MACRO
#endif
    CMenuPopup* popup = new (std::nothrow) CMenuPopup();
#ifdef RESTORE_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_DEBUG_NEW_MACRO
#endif
    return popup;
}

static const CExecuteItem* TrackCommandShellApplicationMenu(HWND hWindow, std::string& wtProfile)
{
    char wtPath[SAL_MAX_PATH];
    std::vector<CWindowsTerminalProfile> wtProfiles;
    if (FindWindowsTerminal(wtPath, ARRAYSIZE(wtPath)))
        CollectWindowsTerminalProfiles(wtProfiles);

    HWND hButton = GetDlgItem(hWindow, IDC_CMDLINEAPP_BROWSE);
    RECT r;
    GetWindowRect(hButton, &r);

    std::vector<HICON> menuIcons;

    CMenuPopup popup;
    CMenuPopup* templatesPopup = NewCommandShellMenuPopup();
    CMenuPopup* windowsTerminalPopup = NewCommandShellMenuPopup();
    if (templatesPopup == NULL || windowsTerminalPopup == NULL)
    {
        if (templatesPopup != NULL)
            delete templatesPopup;
        if (windowsTerminalPopup != NULL)
            delete windowsTerminalPopup;
        return NULL;
    }

    popup.SetImageList(HGrayToolBarImageList);
    popup.SetHotImageList(HHotToolBarImageList);
    templatesPopup->SetImageList(HGrayToolBarImageList);
    templatesPopup->SetHotImageList(HHotToolBarImageList);
    windowsTerminalPopup->SetImageList(HGrayToolBarImageList);
    windowsTerminalPopup->SetHotImageList(HHotToolBarImageList);

    InsertCommandShellMenuItem(&popup, 1, LoadStr(IDS_EXECUTE_BROWSE));
    InsertCommandShellMenuSeparator(&popup);
    InsertCommandShellMenuItem(&popup, 3, LoadStr(IDS_EXECUTE_WINDIR));
    InsertCommandShellMenuItem(&popup, 4, LoadStr(IDS_EXECUTE_SYSDIR));
    InsertCommandShellMenuItem(&popup, 5, LoadStr(IDS_EXECUTE_SALDIR));
    InsertCommandShellMenuSeparator(&popup);
    InsertCommandShellMenuItem(&popup, 7, LoadStr(IDS_EXECUTE_ENV));
    InsertCommandShellMenuSeparator(&popup);

    InsertCommandShellMenuItem(templatesPopup, 100, LoadStr(IDS_EXECUTE_TEMPLATE_DEFAULTCOMSPEC), IDX_TB_COMMANDPROMPT);
    InsertCommandShellMenuItem(templatesPopup, 101, LoadStr(IDS_EXECUTE_TEMPLATE_POWERSHELL), IDX_TB_WINDOWSPOWERSHELL);
    if (FileExists(EXECUTE_TEMPLATE_POWERSHELL7_PATH))
        InsertCommandShellMenuItem(templatesPopup, 102, LoadStr(IDS_EXECUTE_TEMPLATE_POWERSHELL7), IDX_TB_POWERSHELL);

    if (!wtProfiles.empty())
    {
        InsertCommandShellMenuSeparator(templatesPopup);
        for (size_t i = 0; i < wtProfiles.size(); i++)
        {
            UINT id = 200 + (UINT)i;
            int imageIndex = GetWindowsTerminalProfileImageIndex(wtProfiles[i]);
            HICON hIcon = NULL;
            if (imageIndex < 0)
                hIcon = AddMenuIcon(menuIcons, CreateMenuIconFromBitmap(LoadWindowsTerminalProfileBitmap(wtProfiles[i])));
            InsertCommandShellMenuItem(windowsTerminalPopup, id, wtProfiles[i].Name.c_str(), imageIndex, hIcon);
        }
        InsertCommandShellMenuItem(templatesPopup, 0, LoadStr(IDS_EXECUTE_WINDOWS_TERMINAL), IDX_TB_WINDOWSTERMINAL, NULL, windowsTerminalPopup);
        windowsTerminalPopup = NULL; // ownership moved to templatesPopup
    }
    InsertCommandShellMenuItem(&popup, 0, LoadStr(IDS_EXECUTE_TEMPLATES), -1, NULL, templatesPopup);
    templatesPopup = NULL; // ownership moved to popup
    if (windowsTerminalPopup != NULL)
    {
        delete windowsTerminalPopup;
        windowsTerminalPopup = NULL;
    }

    DWORD cmd = popup.Track(MENU_TRACK_RETURNCMD | MENU_TRACK_RIGHTBUTTON,
                            r.right, r.top, hWindow, &r);
    DestroyMenuIcons(menuIcons);

    if (cmd == 0)
    {
        return NULL;
    }
    if (cmd == 1)
    {
        BrowseCommand(hWindow, IDC_CMDLINEAPP_PATH, IDS_EXEFILTER);
        return NULL;
    }
    if (cmd >= 200 && cmd < 200 + wtProfiles.size())
    {
        wtProfile = wtProfiles[cmd - 200].Name;
        SetWindowsTerminalProfileTemplate(hWindow, wtPath, wtProfiles[cmd - 200]);
        return NULL;
    }
    if (cmd == 3 || cmd == 4 || cmd == 5 || cmd == 7)
    {
        const char* text = cmd == 3 ? "$(WinDir)" : cmd == 4 ? "$(SysDir)" : cmd == 5 ? "$(SalDir)" : "$[]";
        HWND hEdit = GetDlgItem(hWindow, IDC_CMDLINEAPP_PATH);
        SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)text);
        if (cmd == 7)
        {
            DWORD start;
            DWORD end;
            SendMessage(hEdit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
            SendMessage(hEdit, EM_SETSEL, end - 1, end - 1);
        }
        SetFocus(hEdit);
        return NULL;
    }
    static CExecuteItem selected;
    selected.Keyword = "";
    selected.Flags = EIF_NO_INSERT;
    selected.NameResID = cmd == 100 ? IDS_EXECUTE_TEMPLATE_DEFAULTCOMSPEC :
                         cmd == 101 ? IDS_EXECUTE_TEMPLATE_POWERSHELL : IDS_EXECUTE_TEMPLATE_POWERSHELL7;
    return &selected;
}

#define COMMAND_SHELL_PLACEHOLDER_SUBCLASS_ID 1

static void PaintCommandShellPlaceholder(HWND hWindow, HFONT hPlaceholderFont)
{
    if (GetWindowTextLength(hWindow) != 0)
        return;
    if (GetDlgCtrlID(hWindow) == IDC_CMDLINEAPP_ARGS && IsWindowEnabled(hWindow))
        return;

    HDC hDC = GetDC(hWindow);
    if (hDC == NULL)
        return;

    RECT r;
    SendMessage(hWindow, EM_GETRECT, 0, (LPARAM)&r);
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, DarkModeShouldUseDarkColors() ? RGB(0xA0, 0xA0, 0xA0)
                                                     : GetSysColor(COLOR_GRAYTEXT));

    HFONT hOldFont = NULL;
    if (hPlaceholderFont != NULL)
        hOldFont = (HFONT)SelectObject(hDC, hPlaceholderFont);

    DrawText(hDC, LoadStr(IDS_CMDLINEAPP_PLACEHOLDER), -1, &r,
             DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);

    if (hOldFont != NULL)
        SelectObject(hDC, hOldFont);
    ReleaseDC(hWindow, hDC);
}

static LRESULT CALLBACK
CommandShellPlaceholderEditProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        LRESULT res = DefSubclassProc(hWindow, uMsg, wParam, lParam);
        PaintCommandShellPlaceholder(hWindow, (HFONT)dwRefData);
        return res;
    }

    case WM_CHAR:
    case WM_KEYDOWN:
    case WM_SETTEXT:
    case WM_ENABLE:
    case WM_CUT:
    case WM_PASTE:
    case WM_CLEAR:
    case WM_UNDO:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    {
        LRESULT res = DefSubclassProc(hWindow, uMsg, wParam, lParam);
        InvalidateRect(hWindow, NULL, TRUE);
        return res;
    }

    case WM_NCDESTROY:
    {
        RemoveWindowSubclass(hWindow, CommandShellPlaceholderEditProc, uIdSubclass);
        break;
    }
    }

    return DefSubclassProc(hWindow, uMsg, wParam, lParam);
}

CCfgPageMainWindow::CCfgPageMainWindow()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_MAINWINDOW, IDD_CFGPAGE_MAINWINDOW, PSP_USETITLE, NULL)
{
    HIconsList = NULL;
    HCommandShellPlaceholderFont = NULL;
}

CCfgPageMainWindow::~CCfgPageMainWindow()
{
    if (HIconsList != NULL)
        ImageList_Destroy(HIconsList);
    if (HCommandShellPlaceholderFont != NULL)
        HANDLES(DeleteObject(HCommandShellPlaceholderFont));
}

void CCfgPageMainWindow::LoadControls()
{
}

void CCfgPageMainWindow::EnableControls()
{
    BOOL usePrefix = IsDlgButtonChecked(HWindow, IDC_TITLEBAR_PREFIX);
    EnableWindow(GetDlgItem(HWindow, IDC_TITLEBAR_PREFIX_TEXT), usePrefix);

    BOOL hasShellApplication = GetWindowTextLength(GetDlgItem(HWindow, IDC_CMDLINEAPP_PATH)) > 0;
    HWND hArguments = GetDlgItem(HWindow, IDC_CMDLINEAPP_ARGS);
    EnableWindow(hArguments, hasShellApplication);
    if (!hasShellApplication)
        SetWindowText(hArguments, "");
}

void CCfgPageMainWindow::Transfer(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageMainWindow::Transfer()");

    if (ti.Type == ttDataToWindow)
    {
        int resIDs[3] = {IDS_TITLEBAR_DIRECTORY, IDS_TITLEBAR_COMPOSITE, IDS_TITLEBAR_FULLPATH}; // must correspond with TITLE_BAR_MODE_xxx
        int i;
        for (i = 0; i < 3; i++)
            SendDlgItemMessage(HWindow, IDC_TITLEBAR_MODE, CB_ADDSTRING, 0, (LPARAM)LoadStr(resIDs[i]));
    }

    ti.CheckBox(IDC_STATUSAREA, Configuration.StatusArea);
    ti.CheckBox(IDC_SPLASHSCREEN, Configuration.ShowSplashScreen);
    ti.CheckBox(IDC_TITLEBAR_PATH, Configuration.TitleBarShowPath);
    if (ti.Type == ttDataToWindow)
        SendDlgItemMessage(HWindow, IDC_TITLEBAR_MODE, CB_SETCURSEL, Configuration.TitleBarMode, 0);
    else
        Configuration.TitleBarMode = (int)SendDlgItemMessage(HWindow, IDC_TITLEBAR_MODE, CB_GETCURSEL, 0, 0);

    // back up data so we can detect change
    BOOL oldUseTitleBarPrefix = Configuration.UseTitleBarPrefix;
    char oldTitleBarPrefix[TITLE_PREFIX_MAX];
    lstrcpyn(oldTitleBarPrefix, Configuration.TitleBarPrefix, TITLE_PREFIX_MAX);

    ti.CheckBox(IDC_TITLEBAR_PREFIX, Configuration.UseTitleBarPrefix);
    ti.EditLine(IDC_TITLEBAR_PREFIX_TEXT, Configuration.TitleBarPrefix, TITLE_PREFIX_MAX);
    ti.EditLine(IDC_CMDLINEAPP_PATH, Configuration.CommandLineApplication, SAL_MAX_PATH);
    ti.EditLine(IDC_CMDLINEAPP_ARGS, Configuration.CommandLineArguments, CONFIG_COMMANDLINEARGS_MAXLEN);

    if (ti.Type == ttDataFromWindow)
    {
        // if the user changed prefix settings, remove any command line option
        if (Configuration.UseTitleBarPrefix != oldUseTitleBarPrefix ||
            Configuration.UseTitleBarPrefix && strcmp(Configuration.TitleBarPrefix, oldTitleBarPrefix) != 0)
        {
            Configuration.UseTitleBarPrefixForced = FALSE;
            Configuration.TitleBarPrefixForced[0] = 0;
        }
    }

    int oldMainWindowIconIndex = Configuration.MainWindowIconIndex;
    if (ti.Type == ttDataToWindow)
        SendDlgItemMessage(HWindow, IDC_TITLEBAR_ICON_INDEX, CB_SETCURSEL, Configuration.MainWindowIconIndex, 0);
    else
    {
        Configuration.MainWindowIconIndex = (int)SendDlgItemMessage(HWindow, IDC_TITLEBAR_ICON_INDEX, CB_GETCURSEL, 0, 0);
        if (Configuration.MainWindowIconIndex != oldMainWindowIconIndex)
            Configuration.MainWindowIconIndexForced = -1; // a change occurred, clear any command line option
    }

    if (ti.Type == ttDataToWindow)
    {
        EnableControls();
    }
}

void CCfgPageMainWindow::Validate(CTransferInfo& ti)
{
}

BOOL CCfgPageMainWindow::InitIconCombobox()
{
    HWND hCombo = GetDlgItem(HWindow, IDC_TITLEBAR_ICON_INDEX);

    // get the position of the original combobox
    RECT r;
    GetWindowRect(hCombo, &r);
    POINT p;
    p.x = r.left;
    p.y = r.top;
    ScreenToClient(HWindow, &p);

    // create the EX version capable of displaying an image list
    HWND hNewCombo = CreateWindowEx(0, WC_COMBOBOXEX, NULL,
                                    WS_BORDER | WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP,
                                    0, 0, 0, (MAINWINDOWICONS_COUNT + 1) * (r.bottom - r.top), // give it some reserve so the list is not clipped on HDPI
                                    HWindow,
                                    NULL,
                                    HInstance,
                                    NULL);
    SetWindowLongPtr(hNewCombo, GWLP_ID, IDC_TITLEBAR_ICON_INDEX);

    // since Vista, if font aliasing is set to Standard, the combobox had aliased font while the rest of the dialog 
    // had the classic non-aliased one; set the correct font
    HFONT hFont = (HFONT)SendMessage(hCombo, WM_GETFONT, 0, 0);
    SendMessage(hNewCombo, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

    HIconsList = ImageList_Create(16, 16, GetImageListColorFlags() | ILC_MASK, 0, 1);

    COMBOBOXEXITEM cbei;
    cbei.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
    int i;
    for (i = 0; i < MAINWINDOWICONS_COUNT; i++)
    {
        HICON hIcon = LoadIcon(HInstance, MAKEINTRESOURCE(MainWindowIcons[i].IconResID));
        ImageList_AddIcon(HIconsList, hIcon);
        DestroyIcon(hIcon);

        cbei.iItem = i;
        cbei.pszText = LoadStr(MainWindowIcons[i].TextResID);
        cbei.iImage = i;
        cbei.iSelectedImage = i;
        SendMessage(hNewCombo, CBEM_INSERTITEM, 0, (LPARAM)&cbei);
    }

    SendMessage(hNewCombo, CBEM_SETIMAGELIST, 0, (LPARAM)HIconsList);

    SetWindowPos(hNewCombo, hCombo, p.x, p.y, r.right - r.left, r.bottom - r.top, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    DestroyWindow(hCombo);

    return TRUE;
}

void CCfgPageMainWindow::InitCommandShellPlaceholders()
{
    HFONT hFont = (HFONT)SendDlgItemMessage(HWindow, IDC_CMDLINEAPP_PATH, WM_GETFONT, 0, 0);
    if (hFont != NULL)
    {
        LOGFONT logFont;
        if (GetObject(hFont, sizeof(LOGFONT), &logFont) == sizeof(LOGFONT))
        {
            logFont.lfItalic = TRUE;
            HCommandShellPlaceholderFont = HANDLES(CreateFontIndirect(&logFont));
        }
    }

    HWND hShellApplication = GetDlgItem(HWindow, IDC_CMDLINEAPP_PATH);
    if (hShellApplication != NULL)
        SetWindowSubclass(hShellApplication, CommandShellPlaceholderEditProc,
                          COMMAND_SHELL_PLACEHOLDER_SUBCLASS_ID, (DWORD_PTR)HCommandShellPlaceholderFont);

    HWND hArguments = GetDlgItem(HWindow, IDC_CMDLINEAPP_ARGS);
    if (hArguments != NULL)
        SetWindowSubclass(hArguments, CommandShellPlaceholderEditProc,
                          COMMAND_SHELL_PLACEHOLDER_SUBCLASS_ID, (DWORD_PTR)HCommandShellPlaceholderFont);
}

void CCfgPageMainWindow::ApplyCommandShellTemplate(int templateNameResID)
{
    switch (templateNameResID)
    {
    case IDS_EXECUTE_TEMPLATE_DEFAULTCOMSPEC:
    {
        SetDlgItemText(HWindow, IDC_CMDLINEAPP_PATH, "");
        SetDlgItemText(HWindow, IDC_CMDLINEAPP_ARGS, "");
        break;
    }

    case IDS_EXECUTE_TEMPLATE_POWERSHELL:
    {
        SetDlgItemText(HWindow, IDC_CMDLINEAPP_PATH, "powershell");
        SetDlgItemText(HWindow, IDC_CMDLINEAPP_ARGS, "-NoExit \"& {command}\"");
        break;
    }

    case IDS_EXECUTE_TEMPLATE_POWERSHELL7:
    {
        SetDlgItemText(HWindow, IDC_CMDLINEAPP_PATH, EXECUTE_TEMPLATE_POWERSHELL7_PATH);
        SetDlgItemText(HWindow, IDC_CMDLINEAPP_ARGS, "-NoExit -WorkingDirectory . -Command \"& {command}\"");
        break;
    }

    }

    HWND hShellApplication = GetDlgItem(HWindow, IDC_CMDLINEAPP_PATH);
    if (hShellApplication != NULL)
        InvalidateRect(hShellApplication, NULL, TRUE);

    HWND hArguments = GetDlgItem(HWindow, IDC_CMDLINEAPP_ARGS);
    if (hArguments != NULL)
        InvalidateRect(hArguments, NULL, TRUE);
}

INT_PTR
CCfgPageMainWindow::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // replace the existing combobox for icon color selection with its EX version
        InitIconCombobox();
        ChangeToArrowButton(HWindow, IDC_CMDLINEAPP_BROWSE);
        InitCommandShellPlaceholders();

        break;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED)
        {
            EnableControls();
        }

        switch (LOWORD(wParam))
        {
        case IDC_CMDLINEAPP_PATH:
            if (HIWORD(wParam) == EN_CHANGE)
                EnableControls();
            break;

        case IDC_CMDLINEAPP_BROWSE:
        {
            std::string wtProfile;
            const CExecuteItem* item = TrackCommandShellApplicationMenu(HWindow, wtProfile);
            if (item != NULL && (item->Flags & EIF_NO_INSERT))
                ApplyCommandShellTemplate(item->NameResID);
            EnableControls();
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
// CCfgPageAppearance
//

CCfgPageAppearance::CCfgPageAppearance()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_APPEARANCE, IDD_CFGPAGE_APPEARANCE, PSP_USETITLE, NULL)
{
    HPanelFont = NULL;
    LocalUseCustomPanelFont = UseCustomPanelFont;
    memcpy(&LocalPanelLogFont, &LogFont, sizeof(LocalPanelLogFont));
    NotificationEnabled = TRUE;
}

CCfgPageAppearance::~CCfgPageAppearance()
{
    if (HPanelFont != NULL)
        HANDLES(DeleteObject(HPanelFont));
}

void CCfgPageAppearance::LoadControls()
{
    CALL_STACK_MESSAGE1("CCfgPageAppearance::LoadControls()");

    LOGFONT logFont;

    if (LocalUseCustomPanelFont)
        logFont = LocalPanelLogFont;
    else
        GetSystemGUIFont(&logFont);

    HWND hEdit = GetDlgItem(HWindow, IDE_PANELFONT);
    int origHeight = logFont.lfHeight;
    logFont.lfHeight = GetWindowFontHeight(hEdit); // use the edit line's font size for font preview
    if (HPanelFont != NULL)
        HANDLES(DeleteObject(HPanelFont));
    HPanelFont = HANDLES(CreateFontIndirect(&logFont));

    HDC hDC = HANDLES(GetDC(HWindow));
    SendMessage(hEdit, WM_SETFONT, (WPARAM)HPanelFont, MAKELPARAM(TRUE, 0));
    char buf[LF_FACESIZE + 200];
    _snprintf_s(buf, _TRUNCATE, LoadStr(IDS_FONTDESCRIPTION),
                MulDiv(-origHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY)),
                logFont.lfFaceName,
                LoadStr(LocalUseCustomPanelFont ? IDS_FONTDESCRIPTION_CST : IDS_FONTDESCRIPTION_DEF));
    SetWindowText(hEdit, buf);

    HANDLES(ReleaseDC(HWindow, hDC));
}

void CCfgPageAppearance::EnableControls()
{
    BOOL pathInTitle = IsDlgButtonChecked(HWindow, IDC_TITLEBAR_PATH);
    EnableWindow(GetDlgItem(HWindow, IDC_TITLEBAR_MODE), pathInTitle);
}

void CCfgPageAppearance::Transfer(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageAppearance::Transfer()");

    ti.CheckBox(IDC_FULLROWSELECT, Configuration.FullRowSelect);       // mutually exclusive with FullRowHighlight
    ti.CheckBox(IDC_FULLROWHIGHLIGHT, Configuration.FullRowHighlight); // mutually exclusive with FullRowSelect
    ti.CheckBox(IDC_ICONTINCTURE, Configuration.UseIconTincture);
    ti.CheckBox(IDC_PANELCAPTION, Configuration.ShowPanelCaption);
    ti.CheckBox(IDC_PANELZOOM, Configuration.ShowPanelZoom);
    ti.CheckBox(IDC_PANELTREEVIEW, Configuration.TreeViewVisible);
    ti.CheckBox(IDC_PANELTOOLTIPS, Configuration.PanelTooltips);
    ti.CheckBox(IDC_SINGLECLICK, Configuration.SingleClick);

    ti.EditLine(IDC_INFOLINECONTENT, Configuration.InfoLineContent, 200);
    ti.EditLine(IDC_THUMBNAILSIZE, Configuration.ThumbnailSize);
    if (ti.Type == ttDataFromWindow)
        Configuration.ThumbnailSize = min(THUMBNAIL_SIZE_MAX, max(THUMBNAIL_SIZE_MIN, Configuration.ThumbnailSize));
    else
        SendDlgItemMessage(HWindow, IDC_THUMBNAILSIZE, EM_LIMITTEXT, 4, 0);

    if (ti.Type == ttDataToWindow)
    {
        EnableControls();
        LoadControls();
    }

    if (ti.Type == ttDataFromWindow)
    {
        UseCustomPanelFont = LocalUseCustomPanelFont;
        memcpy(&LogFont, &LocalPanelLogFont, sizeof(LocalPanelLogFont));
    }
}

void CCfgPageAppearance::Validate(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageAppearance::Validate()");
    HWND hWnd;
    if (ti.GetControl(hWnd, IDC_INFOLINECONTENT))
    {
        char buff[MAX_PATH];
        SendMessage(hWnd, WM_GETTEXT, MAX_PATH, (LPARAM)buff);
        int errorPos1, errorPos2;
        if (!ValidateInfoLineItems(HWindow, buff, errorPos1, errorPos2))
        {
            ti.ErrorOn(IDC_INFOLINECONTENT);
            PostMessage(hWnd, EM_SETSEL, errorPos1, errorPos2);
            return;
        }
    }
}

INT_PTR
CCfgPageAppearance::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        ChangeToArrowButton(HWindow, IDC_INFOLINEBROWSE);
        new CButton(HWindow, IDB_PANELFONT, BTF_RIGHTARROW);

        // attach the UpDown control to the edit line
        int resID[] = {IDC_THUMBNAILSIZE, -1};
        int upDownID[] = {IDC_THUMBNAILSIZE_UPDOWN};
        int i;
        for (i = 0; resID[i] != -1; i++)
        {
            HWND hEdit = GetDlgItem(HWindow, resID[i]);
            HWND hWnd = CreateUpDownControl(WS_VISIBLE | WS_CHILD | UDS_SETBUDDYINT |
                                                UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                                            0, 0, 0, 0, HWindow, upDownID[i], HInstance,
                                            hEdit, THUMBNAIL_SIZE_MAX, THUMBNAIL_SIZE_MIN, 0);
            // move the UpDown control in the z-order right after the edit line; otherwise
            // drawing the dialog on a slow machine looked odd
            // (the UpDown was drawn only after all the other controls)
            SetWindowPos(hWnd, hEdit, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            DarkModeApplyUpDownSubclass(hWnd);
        }

        break;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED)
        {
            EnableControls();
        }

        if (NotificationEnabled && HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_THUMBNAILSIZE)
        {
            // notification about changes in the edit line
            CTransferInfo ti(HWindow, ttDataFromWindow);
            int value;
            ti.EditLine(IDC_THUMBNAILSIZE, value); // the slider enforces the bounds itself
        }

        switch (LOWORD(wParam))
        {
        case IDC_FULLROWSELECT:
        {
            if (IsDlgButtonChecked(HWindow, IDC_FULLROWSELECT) == BST_CHECKED)
                CheckDlgButton(HWindow, IDC_FULLROWHIGHLIGHT, BST_UNCHECKED);
            return 0;
        }

        case IDC_FULLROWHIGHLIGHT:
        {
            if (IsDlgButtonChecked(HWindow, IDC_FULLROWHIGHLIGHT) == BST_CHECKED)
                CheckDlgButton(HWindow, IDC_FULLROWSELECT, BST_UNCHECKED);
            return 0;
        }

        case IDB_PANELFONT:
        {
            /* used by the export_mnu.py script which generates salmenu.mnu for the Translator
   keep synchronized with the InsertMenu() call below...
MENU_TEMPLATE_ITEM CfgPageAppearanceMenu[] = 
{
  {MNTT_PB, 0
  {MNTT_IT, IDS_USEDEFAULTFONT
  {MNTT_IT, IDS_USECUSTOMFONT
  {MNTT_PE, 0
};
*/
            HMENU hMenu = CreatePopupMenu();
            BOOL cstFont = LocalUseCustomPanelFont;
            InsertMenu(hMenu, 0xFFFFFFFF, cstFont ? 0 : MF_CHECKED | MF_BYCOMMAND | MF_STRING, 1, LoadStr(IDS_USEDEFAULTFONT));
            InsertMenu(hMenu, 0xFFFFFFFF, cstFont ? MF_CHECKED : 0 | MF_BYCOMMAND | MF_STRING, 2, LoadStr(IDS_USECUSTOMFONT));

            TPMPARAMS tpmPar;
            tpmPar.cbSize = sizeof(tpmPar);
            GetWindowRect((HWND)lParam, &tpmPar.rcExclude);
            DWORD cmd = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, tpmPar.rcExclude.right, tpmPar.rcExclude.top,
                                         HWindow, &tpmPar);
            if (cmd != 0)
            {
                if (cmd == 1) // standard font
                {
                    LocalUseCustomPanelFont = FALSE;
                    LoadControls();
                }
                if (cmd == 2) // custom font
                {
                    CHOOSEFONT cf;
                    memset(&cf, 0, sizeof(CHOOSEFONT));
                    cf.lStructSize = sizeof(CHOOSEFONT);
                    cf.hwndOwner = HWindow;
                    cf.lpLogFont = &LocalPanelLogFont;
                    cf.iPointSize = 10;
                    cf.Flags = CF_NOVERTFONTS | CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
                    DarkModePrepareChooseFont(&cf);
                    if (ChooseFont(&cf) != 0)
                    {
                        LocalUseCustomPanelFont = TRUE;
                        LoadControls();
                    }
                    return 0;
                }
            }
            return 0;
        }

        case IDC_INFOLINEBROWSE:
        {
            TrackExecuteMenu(HWindow, IDC_INFOLINEBROWSE, IDC_INFOLINECONTENT, FALSE,
                             InfoLineContentItems);
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
// CCfgPageChangeDrive
//

const int DRIVES_COUNT = 'z' - 'a' + 1;
const char FIRST_DRIVE = 'a'; // use 'A' here if uppercase letters are desired

// restrict the listbox so clicks outside existing items have no effect
class CDriveListBox : public CWindow
{
public:
    CDriveListBox(HWND hDlg, int ctrlID) : CWindow(hDlg, ctrlID) {}

protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_LBUTTONDOWN:
        {
            if (GetFocus() != HWindow)
                SendMessage(GetParent(HWindow), WM_NEXTDLGCTL, (WPARAM)HWindow, TRUE);
            int index = LOWORD(SendMessage(HWindow, LB_ITEMFROMPOINT, 0,
                                           MAKELPARAM(LOWORD(lParam), HIWORD(lParam))));
            if (index < 0 || index >= DRIVES_COUNT)
                return 0; // nonsense, ignore it

            // ignore clicks outside an item
            RECT r;
            SendMessage(HWindow, LB_GETITEMRECT, index, (LPARAM)&r);
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            if (!PtInRect(&r, pt))
                return 0;

            break;
        }

        case WM_LBUTTONDBLCLK:
        {
            SendMessage(HWindow, WM_LBUTTONDOWN, wParam, lParam);
            return 0;
        }
        }
        return CWindow::WindowProc(uMsg, wParam, lParam);
    }
};

CCfgPageChangeDrive::CCfgPageChangeDrive()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_CHANGEDRIVE, IDD_CFGPAGE_CHANGEDRIVE, PSP_USETITLE, NULL)
{
    CharSize.cx = 0;
    CharSize.cy = 0;
}

void CCfgPageChangeDrive::SetDrivesToListbox(int resID, DWORD drives)
{
    HWND hList = GetDlgItem(HWindow, resID);
    int i;
    for (i = 0; i < DRIVES_COUNT; i++)
    {
        BOOL select = (drives & (1 << i)) != 0;
        SendMessage(hList, LB_SETSEL, select, i);
    }
    // focus at the end; move it back to the beginning
    SendMessage(hList, LB_SETCARETINDEX, 0, FALSE);
}

DWORD
CCfgPageChangeDrive::GetDrivesFromListbox(int resID)
{
    HWND hList = GetDlgItem(HWindow, resID);
    DWORD drives = 0;
    int i;
    for (i = 0; i < DRIVES_COUNT; i++)
    {
        BOOL selected = (BOOL)SendMessage(hList, LB_GETSEL, i, 0);
        if (selected)
            drives |= (1 << i);
    }
    return drives;
}

void CCfgPageChangeDrive::Transfer(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageChangeDrive::Transfer()");

    int oldChangeDriveShowMountFolders = Configuration.ChangeDriveShowMountFolders;
    int oldChangeDriveMountFoldersMode = Configuration.ChangeDriveMountFoldersMode;
    int oldChangeDriveMountFoldersName = Configuration.ChangeDriveMountFoldersName;
    int oldChangeDriveMountFoldersDriveBar = Configuration.ChangeDriveMountFoldersDriveBar;

    ti.CheckBox(IDC_CHD_SHOWMOUNTFOLDERS, Configuration.ChangeDriveShowMountFolders);
    ti.CheckBox(IDC_CHD_MOUNTFOLDERS_VOLUMENAME, Configuration.ChangeDriveMountFoldersName);
    ti.CheckBox(IDC_CHD_MOUNTFOLDERS_DRIVEBAR, Configuration.ChangeDriveMountFoldersDriveBar);
    ti.CheckBox(IDC_CHD_SHOWMYDOC, Configuration.ChangeDriveShowMyDoc);
    ti.CheckBox(IDC_CHD_SHOW3DOBJECTS, Configuration.ChangeDriveShow3DObjects);
    ti.CheckBox(IDC_CHD_SHOWDESKTOP, Configuration.ChangeDriveShowDesktop);
    ti.CheckBox(IDC_CHD_SHOWDOWNLOADS, Configuration.ChangeDriveShowDownloads);
    ti.CheckBox(IDC_CHD_SHOWMUSIC, Configuration.ChangeDriveShowMusic);
    ti.CheckBox(IDC_CHD_SHOWPICTURES, Configuration.ChangeDriveShowPictures);
    ti.CheckBox(IDC_CHD_SHOWVIDEOS, Configuration.ChangeDriveShowVideos);
    ti.CheckBox(IDC_CHD_SHOWANOTHER, Configuration.ChangeDriveShowAnother);
    ti.CheckBox(IDC_CHD_SHOWNET, Configuration.ChangeDriveShowNet);
    ti.CheckBox(IDC_CHD_SHOWCLOUDSTORAGE, Configuration.ChangeDriveCloudStorage);

    if (ti.Type == ttDataToWindow)
    {
        const int MODE_ITEMS = 3;
        int modes[MODE_ITEMS] = {TITLE_BAR_MODE_DIRECTORY, TITLE_BAR_MODE_COMPOSITE, TITLE_BAR_MODE_FULLPATH};
        int resIDs[MODE_ITEMS] = {IDS_TITLEBAR_DIRECTORY, IDS_TITLEBAR_COMPOSITE, IDS_TITLEBAR_FULLPATH};
        SendDlgItemMessage(HWindow, IDC_CHD_MOUNTFOLDERS_MODE, CB_RESETCONTENT, 0, 0);
        BOOL selected = FALSE;
        for (int i = 0; i < MODE_ITEMS; i++)
        {
            SendDlgItemMessage(HWindow, IDC_CHD_MOUNTFOLDERS_MODE, CB_ADDSTRING, 0, (LPARAM)LoadStr(resIDs[i]));
            if (!selected && Configuration.ChangeDriveMountFoldersMode == modes[i])
            {
                SendDlgItemMessage(HWindow, IDC_CHD_MOUNTFOLDERS_MODE, CB_SETCURSEL, i, 0);
                selected = TRUE;
            }
        }
        if (!selected)
            SendDlgItemMessage(HWindow, IDC_CHD_MOUNTFOLDERS_MODE, CB_SETCURSEL, 0, 0);
        BOOL enableMountFolders = Configuration.ChangeDriveShowMountFolders;
        EnableWindow(GetDlgItem(HWindow, IDC_CHD_MOUNTFOLDERS_LABEL), enableMountFolders);
        EnableWindow(GetDlgItem(HWindow, IDC_CHD_MOUNTFOLDERS_MODE), enableMountFolders);
        EnableWindow(GetDlgItem(HWindow, IDC_CHD_MOUNTFOLDERS_VOLUMENAME), enableMountFolders);
        EnableWindow(GetDlgItem(HWindow, IDC_CHD_MOUNTFOLDERS_DRIVEBAR), enableMountFolders);

        SetDrivesToListbox(IDL_CHD_DRIVES, Configuration.VisibleDrives);
        SetDrivesToListbox(IDL_CHD_SEPARATORS, Configuration.SeparatedDrives);
        if (Plugins.GetFirstNethoodPluginFSName())
            EnableWindow(GetDlgItem(HWindow, IDC_CHD_SHOWNET), FALSE);
    }
    else
    {
        int index = (int)SendDlgItemMessage(HWindow, IDC_CHD_MOUNTFOLDERS_MODE, CB_GETCURSEL, 0, 0);
        int modes[3] = {TITLE_BAR_MODE_DIRECTORY, TITLE_BAR_MODE_COMPOSITE, TITLE_BAR_MODE_FULLPATH};
        Configuration.ChangeDriveMountFoldersMode = index >= 0 && index < 3 ? modes[index] : TITLE_BAR_MODE_DIRECTORY;
        if (oldChangeDriveShowMountFolders != Configuration.ChangeDriveShowMountFolders ||
            oldChangeDriveMountFoldersMode != Configuration.ChangeDriveMountFoldersMode ||
            oldChangeDriveMountFoldersName != Configuration.ChangeDriveMountFoldersName ||
            oldChangeDriveMountFoldersDriveBar != Configuration.ChangeDriveMountFoldersDriveBar)
        {
            PostMessage(MainWindow->HWindow, WM_USER_DRIVES_CHANGE, 0, 0);
        }
        Configuration.VisibleDrives = GetDrivesFromListbox(IDL_CHD_DRIVES);
        Configuration.SeparatedDrives = GetDrivesFromListbox(IDL_CHD_SEPARATORS);
    }
}

void CCfgPageChangeDrive::InitList(int resID)
{
    HWND hList = GetDlgItem(HWindow, resID);

    RECT r;
    GetWindowRect(hList, &r);

    SendMessage(hList, LB_SETCOLUMNWIDTH, CharSize.cx + 2, 0);
    // set the height according to the controls; LBS_NOINTEGRALHEIGHT is used because with some fonts the height would otherwise be zero
    SendMessage(hList, LB_SETITEMHEIGHT, 0, MAKELPARAM(/*CharSize.cy + 3*/ r.bottom - r.top - 4, 0));
    SendMessage(hList, LB_SETCOUNT, DRIVES_COUNT, 0);
}

INT_PTR
CCfgPageChangeDrive::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDC_CHD_SHOWMOUNTFOLDERS && HIWORD(wParam) == BN_CLICKED)
        {
            BOOL enableMountFolders = IsDlgButtonChecked(HWindow, IDC_CHD_SHOWMOUNTFOLDERS) == BST_CHECKED;
            EnableWindow(GetDlgItem(HWindow, IDC_CHD_MOUNTFOLDERS_LABEL), enableMountFolders);
            EnableWindow(GetDlgItem(HWindow, IDC_CHD_MOUNTFOLDERS_MODE), enableMountFolders);
            EnableWindow(GetDlgItem(HWindow, IDC_CHD_MOUNTFOLDERS_VOLUMENAME), enableMountFolders);
            EnableWindow(GetDlgItem(HWindow, IDC_CHD_MOUNTFOLDERS_DRIVEBAR), enableMountFolders);
        }
        if (LOWORD(wParam) == IDL_CHD_DRIVES || LOWORD(wParam) == IDL_CHD_SEPARATORS)
        {
            if (HIWORD(wParam) == LBN_SETFOCUS || HIWORD(wParam) == LBN_KILLFOCUS)
            {
                InvalidateRect((HWND)lParam, NULL, TRUE);
                return 0;
            }
        }
        break;
    }

    case WM_INITDIALOG:
    {
        int staticsArr[] = {IDC_STATIC_6, IDS_CHD_HOTPATHS, IDC_STATIC_7, IDS_CHD_PLUGINS, IDC_STATIC_8, 0};
        CondenseStaticTexts(HWindow, staticsArr);

        // store dimensions of the largest character in CharSize
        HFONT hFont = (HFONT)SendDlgItemMessage(HWindow, IDL_CHD_DRIVES, WM_GETFONT, 0, 0);
        HDC hDC = HANDLES(GetDC(HWindow));
        HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);
        char buff[] = " :";
        int i;
        for (i = 0; i < DRIVES_COUNT; i++)
        {
            buff[0] = FIRST_DRIVE + i;
            SIZE sz;
            GetTextExtentPoint32(hDC, buff, 2, &sz);
            if (sz.cx > CharSize.cx || sz.cy > CharSize.cy)
                CharSize = sz;
        }
        SelectObject(hDC, hOldFont);
        HANDLES(ReleaseDC(HWindow, hDC));

        // improved list boxes -- double-click works, clicks outside an item are ignored
        new CDriveListBox(HWindow, IDL_CHD_DRIVES);
        new CDriveListBox(HWindow, IDL_CHD_SEPARATORS);

        // setup listboxes
        InitList(IDL_CHD_DRIVES);
        InitList(IDL_CHD_SEPARATORS);

        // HINTS for Drives, Hot Paths, Plugins
        CHyperLink* hl;
        hl = new CHyperLink(HWindow, IDS_CHD_HOTPATHS, STF_DOTUNDERLINE);
        if (hl != NULL)
            hl->SetActionShowHint(LoadStr(IDS_CHDHOTPATHS_HINT));
        hl = new CHyperLink(HWindow, IDS_CHD_PLUGINS, STF_DOTUNDERLINE);
        if (hl != NULL)
            hl->SetActionShowHint(LoadStr(IDS_CHDPLUGINS_HINT));

        break;
    }

    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
        if ((int)lpdis->itemID >= 0 && (int)lpdis->itemID < DRIVES_COUNT)
        {
            RECT r = lpdis->rcItem;
            HDC hDC = lpdis->hDC;

            BOOL selected = (lpdis->itemState & ODS_SELECTED) != 0;
            BOOL focused = (GetFocus() == lpdis->hwndItem);

            const bool useDark = DarkModeShouldUseDarkColors();
            const DarkModeColors& darkColors = DarkModeGetColors();

            COLORREF bkBrushColor;
            if (useDark)
                bkBrushColor = darkColors.background;
            else
                bkBrushColor = GetSysColor(COLOR_WINDOW);
            HBRUSH bkBrush = CreateSolidBrush(bkBrushColor);
            FillRect(hDC, &r, bkBrush);
            DeleteObject(bkBrush);

            if (selected)
            {
                RECT rr = r;
                InflateRect(&rr, -1, -1);
                COLORREF selColor;
                if (useDark)
                    selColor = RGB(0x33, 0x33, 0x33);
                else
                    selColor = GetSysColor(focused ? COLOR_HIGHLIGHT : COLOR_3DFACE);
                HBRUSH selBrush = CreateSolidBrush(selColor);
                FillRect(hDC, &rr, selBrush);
                DeleteObject(selBrush);
            }

            COLORREF textColor;
            if (useDark)
                textColor = darkColors.readableText;
            else if (selected)
                textColor = GetSysColor(focused ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
            else
                textColor = GetSysColor(COLOR_GRAYTEXT);

            SetTextColor(hDC, textColor);
            SetBkMode(hDC, TRANSPARENT);
            RECT dr = r;
            char text[] = " :";
            text[0] = FIRST_DRIVE + lpdis->itemID;
            DrawText(hDC, text, 2, &dr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            if (lpdis->itemState & ODS_FOCUS)
            {
                SetTextColor(hDC, useDark ? darkColors.readableText : GetSysColor(COLOR_WINDOWTEXT));
                DrawFocusRect(hDC, &r);
            }
        }
        return TRUE;
    }

    case WM_CHARTOITEM:
    {
        int index = LowerCase[LOBYTE(LOWORD(wParam))] - 'a';
        HWND hList = (HWND)lParam;
        SendMessage(hList, LB_SETCARETINDEX, index, FALSE);
        BOOL selected = (BOOL)SendMessage(hList, LB_GETSEL, index, 0);
        SendMessage(hList, LB_SETSEL, !selected, index);
        return -1;
    }
    }
    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CCfgPagePanels
//

CCfgPagePanels::CCfgPagePanels()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_PANELS, IDD_CFGPAGE_PANELS, PSP_USETITLE, NULL)
{
}

void CCfgPagePanels::Transfer(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPagePanels::Transfer()");

    int oldUseTabs = Configuration.UsePanelTabs;
    int oldSortUsesLocale = Configuration.SortUsesLocale;
    int oldSortDetectNumbers = Configuration.SortDetectNumbers;

    // keep values in Configuration.FileNameFormat for backward compatibility
    const int MANGLE_ITEMS = 6;
    int mangles[MANGLE_ITEMS] = {4 /*ONTHEDISK*/, 5 /*EXPLORER*/, 6 /*VC*/, 7 /*PARTMIXEDCASE*/, 2 /*LOWERCASE*/, 3 /*UPPERCASE*/};

    const int SIZE_ITEMS = 3;
    int sizes[SIZE_ITEMS] = {SIZE_FORMAT_BYTES, SIZE_FORMAT_KB, SIZE_FORMAT_MIXED};

    if (ti.Type == ttDataToWindow)
    {
        int resIDs[MANGLE_ITEMS] = {IDS_NAMEMANGLE_ONTHEDISK, IDS_NAMEMANGLE_EXPLORER, IDS_NAMEMANGLE_VC, IDS_NAMEMANGLE_PARTMIXEDCASE, IDS_NAMEMANGLE_LOWERCASE, IDS_NAMEMANGLE_UPPERCASE}; // must correspond with TITLE_BAR_MODE_xxx
        BOOL selected = FALSE;
        int i;
        for (i = 0; i < MANGLE_ITEMS; i++)
        {
            SendDlgItemMessage(HWindow, IDC_NAMEMANGLE, CB_ADDSTRING, 0, (LPARAM)LoadStr(resIDs[i]));
            if (!selected && Configuration.FileNameFormat == mangles[i])
            {
                SendDlgItemMessage(HWindow, IDC_NAMEMANGLE, CB_SETCURSEL, i, 0);
                selected = TRUE;
            }
        }
        if (!selected)
            SendDlgItemMessage(HWindow, IDC_NAMEMANGLE, CB_SETCURSEL, 0, 0); // ONTHEDISK

        int resID2s[SIZE_ITEMS] = {IDS_SIZEMODE_BYTES, IDS_SIZEMODE_KB, IDS_SIZEMODE_MIXED}; // must correspond with TITLE_BAR_MODE_xxx
        selected = FALSE;
        for (i = 0; i < SIZE_ITEMS; i++)
        {
            SendDlgItemMessage(HWindow, IDC_SIZEFORMAT, CB_ADDSTRING, 0, (LPARAM)LoadStr(resID2s[i]));
            if (!selected && Configuration.SizeFormat == sizes[i])
            {
                SendDlgItemMessage(HWindow, IDC_SIZEFORMAT, CB_SETCURSEL, i, 0);
                selected = TRUE;
            }
        }
        if (!selected)
            SendDlgItemMessage(HWindow, IDC_SIZEFORMAT, CB_SETCURSEL, 0, 0); // SIZE_FORMAT_BYTES
    }
    else
    {
        int index = (int)SendDlgItemMessage(HWindow, IDC_NAMEMANGLE, CB_GETCURSEL, 0, 0);
        if (index < 0 || index >= MANGLE_ITEMS)
            index = 0; // ONTHEDISK
        Configuration.FileNameFormat = mangles[index];

        index = (int)SendDlgItemMessage(HWindow, IDC_SIZEFORMAT, CB_GETCURSEL, 0, 0);
        if (index < 0 || index >= SIZE_ITEMS)
            index = 0; // SIZE_FORMAT_BYTES
        Configuration.SizeFormat = sizes[index];
    }

    ti.CheckBox(IDC_PANELS_USETABS, Configuration.UsePanelTabs);
    ti.CheckBox(IDC_NOTHIDDENSYSTEM, Configuration.NotHiddenSystemFiles);
    ti.CheckBox(IDC_INCLUDEDIRS, Configuration.IncludeDirs);
    ti.CheckBox(IDC_DISABLEDANDD, Configuration.UseDragDropMinTime);
    ti.EditLine(IDE_DRAGDROPDELAY, Configuration.DragDropMinTime);
    ti.CheckBox(IDC_QUICKSEARCH_ALT, Configuration.QuickSearchEnterAlt);
    ti.CheckBox(IDE_PRIMARYCTXMENU, Configuration.PrimaryContextMenu);
    ti.CheckBox(IDC_SHIFTFORHOTPATHS, Configuration.ShiftForHotPaths);
    ti.CheckBox(IDC_CLICKTORENAME, Configuration.ClickQuickRename);
    ti.CheckBox(IDC_SORTUSESLOCALE, Configuration.SortUsesLocale);
    ti.CheckBox(IDC_SORTDETECTNUMBERS, Configuration.SortDetectNumbers);
    ti.CheckBox(IDC_SORTNEWERONTOP, Configuration.SortNewerOnTop);
    ti.CheckBox(IDC_SORTDIRSBYEXT, Configuration.SortDirsByExt);
    ti.CheckBox(IDC_SORTDIRSBYNAME, Configuration.SortDirsByName);

    if (ti.Type == ttDataToWindow)
        EnableControls();
    else
    {
        if (oldUseTabs != Configuration.UsePanelTabs)
            MainWindow->HandlePanelTabsEnabledChange(oldUseTabs != 0);

        if (oldSortUsesLocale != Configuration.SortUsesLocale ||
            oldSortDetectNumbers != Configuration.SortDetectNumbers)
        {
            int totalPanels = MainWindow->LeftPanelTabs.Count + MainWindow->RightPanelTabs.Count;
            TDirectArray<CFilesWindow*> panels(totalPanels, totalPanels);
            for (int i = 0; i < MainWindow->LeftPanelTabs.Count; i++)
                panels.Add(MainWindow->LeftPanelTabs[i]);
            for (int i = 0; i < MainWindow->RightPanelTabs.Count; i++)
                panels.Add(MainWindow->RightPanelTabs[i]);

            for (int i = 0; i < panels.Count; i++)
            {
                CFilesWindow* panel = panels[i];
                if (panel != NULL)
                {
                    if (panel->UseSystemIcons || panel->UseThumbnails)
                        panel->SleepIconCacheThread();
                    panel->SortDirectory();
                    if (panel->UseSystemIcons || panel->UseThumbnails)
                        panel->WakeupIconCacheThread();
                    if (panel == MainWindow->LeftPanel || panel == MainWindow->RightPanel)
                        panel->RefreshListBox(-1, -1, -1, FALSE, FALSE);
                }
            }
        }
    }
}

void CCfgPagePanels::EnableControls()
{
    BOOL useDDMin = IsDlgButtonChecked(HWindow, IDC_DISABLEDANDD);
    EnableWindow(GetDlgItem(HWindow, IDE_DRAGDROPDELAY), useDDMin);
}

INT_PTR
CCfgPagePanels::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED)
        {
            EnableControls();
            if (LOWORD(wParam) == IDC_PANELS_USETABS)
            {
                BOOL useTabs = IsDlgButtonChecked(HWindow, IDC_PANELS_USETABS) == BST_CHECKED;
                SendMessage(::GetParent(HWindow), WM_CFG_UPDATE_TABS_VISIBILITY, useTabs, 0);
            }
        }
        break;
    }
    }
    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CCfgPageTabs
//

CCfgPageTabs::CCfgPageTabs()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_TABS, IDD_CFGPAGE_TABS, PSP_USETITLE, NULL)
{
    activeBorderColorBtn = NULL;
    tabActiveBorderColor = CLR_INVALID;
}

static COLORREF TabActiveBorderGetDefaultColor()
{
    COLORREF borderColor;
    if (CurrentColors != NULL)
        borderColor = GetCOLORREF(CurrentColors[ACTIVE_CAPTION_BK]);
    else if (DarkModeShouldUseDarkColors())
        borderColor = DarkModeGetDialogBackgroundColor();
    else
        borderColor = GetSysColor(COLOR_ACTIVECAPTION);
    return LightenColorSimple(borderColor, 96);
}

void CCfgPageTabs::UpdateColorButton()
{
    if (activeBorderColorBtn != NULL)
    {
        BOOL checked = SendDlgItemMessage(HWindow, IDC_TABS_ACTIVEBORDER, BM_GETCHECK, 0, 0) == BST_CHECKED;
        EnableWindow(activeBorderColorBtn->HWindow, checked);
    }
}

INT_PTR CCfgPageTabs::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        INT_PTR ret = CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
        tabActiveBorderColor = Configuration.TabActiveBorderColor;
        activeBorderColorBtn = new CColorArrowButton(HWindow, IDC_TABS_ACTIVEBORDER_COLOR, TRUE);
        COLORREF color = (tabActiveBorderColor != CLR_INVALID) ? tabActiveBorderColor : TabActiveBorderGetDefaultColor();
        activeBorderColorBtn->SetColor(color, color);
        UpdateColorButton();
        return ret;
    }

    case WM_COMMAND:
    {
        WORD cmd = LOWORD(wParam);
        WORD code = HIWORD(wParam);

        if (cmd == IDC_TABS_ACTIVEBORDER && code == BN_CLICKED)
        {
            UpdateColorButton();
            break;
        }

        if (cmd == IDC_TABS_ACTIVEBORDER_COLOR && code == BN_CLICKED)
        {
            HMENU hMenu = CreatePopupMenu();
            if (hMenu != NULL)
            {
                BOOL isCustom = (tabActiveBorderColor != CLR_INVALID);
                InsertMenu(hMenu, 0xFFFFFFFF, isCustom ? MF_CHECKED : 0 | MF_BYCOMMAND | MF_STRING, 1, LoadStr(IDS_TABBORDER_CUSTOM_COLOR));
                InsertMenu(hMenu, 0xFFFFFFFF, isCustom ? 0 : MF_CHECKED | MF_BYCOMMAND | MF_STRING, 2, LoadStr(IDS_TABBORDER_AUTO_COLOR));

                TPMPARAMS tpmPar;
                tpmPar.cbSize = sizeof(tpmPar);
                GetWindowRect(activeBorderColorBtn->HWindow, &tpmPar.rcExclude);
                DWORD result = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                                                tpmPar.rcExclude.right, tpmPar.rcExclude.top, HWindow, &tpmPar);
                if (result == 1)
                {
                    CHOOSECOLOR cc;
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = HWindow;
                    cc.lpCustColors = (LPDWORD)CustomColors;
                    cc.rgbResult = (tabActiveBorderColor != CLR_INVALID) ? tabActiveBorderColor : TabActiveBorderGetDefaultColor();
                    cc.Flags = CC_RGBINIT | CC_FULLOPEN;
                    DarkModePrepareChooseColor(&cc);
                    if (ChooseColor(&cc) == TRUE)
                    {
                        tabActiveBorderColor = cc.rgbResult;
                        activeBorderColorBtn->SetColor(cc.rgbResult, cc.rgbResult);
                    }
                }
                else if (result == 2)
                {
                    tabActiveBorderColor = CLR_INVALID;
                    COLORREF color = TabActiveBorderGetDefaultColor();
                    activeBorderColorBtn->SetColor(color, color);
                }
                DestroyMenu(hMenu);
            }
            return TRUE;
        }
        break;
    }

    case WM_NOTIFY:
    {
        NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);
        if (nmhdr->code == PSN_KILLACTIVE)
        {
            Configuration.TabActiveBorderColor = tabActiveBorderColor;
        }
        break;
    }
    }

    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}

void CCfgPageTabs::Transfer(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CCfgPageTabs::Transfer()");

    const int MODE_ITEMS = 3;
    int modes[MODE_ITEMS] = {TITLE_BAR_MODE_DIRECTORY, TITLE_BAR_MODE_COMPOSITE, TITLE_BAR_MODE_FULLPATH};

    int oldMinWidth = Configuration.TabButtonMinWidth;
    int oldMaxWidth = Configuration.TabButtonMaxWidth;
    int oldActiveBorder = Configuration.TabActiveBorder;
    COLORREF oldActiveBorderColor = Configuration.TabActiveBorderColor;
    int oldCloseButtonActive = Configuration.TabCloseButtonActive;
    int oldCloseButtonAll = Configuration.TabCloseButtonAll;

    ti.EditLine(IDC_TABS_MINWIDTH, Configuration.TabButtonMinWidth);
    ti.EditLine(IDC_TABS_MAXWIDTH, Configuration.TabButtonMaxWidth);
    ti.CheckBox(IDC_TABS_ACTIVEBORDER, Configuration.TabActiveBorder);
    ti.CheckBox(IDC_TABS_CLOSEBUTTONACTIVE, Configuration.TabCloseButtonActive);
    ti.CheckBox(IDC_TABS_CLOSEBUTTONALL, Configuration.TabCloseButtonAll);

    if (ti.Type == ttDataToWindow)
    {
        int resIDs[MODE_ITEMS] = {IDS_TITLEBAR_DIRECTORY, IDS_TITLEBAR_COMPOSITE, IDS_TITLEBAR_FULLPATH};
        SendDlgItemMessage(HWindow, IDC_TABS_MODE, CB_RESETCONTENT, 0, 0);
        BOOL selected = FALSE;
        for (int i = 0; i < MODE_ITEMS; i++)
        {
            SendDlgItemMessage(HWindow, IDC_TABS_MODE, CB_ADDSTRING, 0, (LPARAM)LoadStr(resIDs[i]));
            if (!selected && Configuration.TabCaptionMode == modes[i])
            {
                SendDlgItemMessage(HWindow, IDC_TABS_MODE, CB_SETCURSEL, i, 0);
                selected = TRUE;
            }
        }
        if (!selected)
            SendDlgItemMessage(HWindow, IDC_TABS_MODE, CB_SETCURSEL, 0, 0);

        const int ALIGN_ITEMS = 2;
        int alignments[ALIGN_ITEMS] = {TAB_CAPTION_ALIGN_LEFT, TAB_CAPTION_ALIGN_CENTER};
        int alignResIDs[ALIGN_ITEMS] = {IDS_TABCAPTIONALIGN_LEFT, IDS_TABCAPTIONALIGN_CENTER};
        SendDlgItemMessage(HWindow, IDC_TABS_ALIGN, CB_RESETCONTENT, 0, 0);
        selected = FALSE;
        for (int i = 0; i < ALIGN_ITEMS; ++i)
        {
            SendDlgItemMessage(HWindow, IDC_TABS_ALIGN, CB_ADDSTRING, 0, (LPARAM)LoadStr(alignResIDs[i]));
            if (!selected && Configuration.TabCaptionAlignment == alignments[i])
            {
                SendDlgItemMessage(HWindow, IDC_TABS_ALIGN, CB_SETCURSEL, i, 0);
                selected = TRUE;
            }
        }
        if (!selected)
            SendDlgItemMessage(HWindow, IDC_TABS_ALIGN, CB_SETCURSEL, 1, 0);

        SendDlgItemMessage(HWindow, IDC_TABS_MINWIDTH, EM_LIMITTEXT, 4, 0);
        SendDlgItemMessage(HWindow, IDC_TABS_MAXWIDTH, EM_LIMITTEXT, 4, 0);
    }
    else
    {
        const int ALIGN_ITEMS = 2;
        int alignments[ALIGN_ITEMS] = {TAB_CAPTION_ALIGN_LEFT, TAB_CAPTION_ALIGN_CENTER};
        int index = (int)SendDlgItemMessage(HWindow, IDC_TABS_MODE, CB_GETCURSEL, 0, 0);
        if (index < 0 || index >= MODE_ITEMS)
            index = 0;
        int alignIndex = (int)SendDlgItemMessage(HWindow, IDC_TABS_ALIGN, CB_GETCURSEL, 0, 0);
        if (alignIndex < 0 || alignIndex >= ALIGN_ITEMS)
            alignIndex = 1;

        if (Configuration.TabButtonMinWidth < 0)
            Configuration.TabButtonMinWidth = 0;
        if (Configuration.TabButtonMaxWidth < 0)
            Configuration.TabButtonMaxWidth = 0;
        if (Configuration.TabButtonMinWidth > 0 && Configuration.TabButtonMaxWidth > 0 &&
            Configuration.TabButtonMinWidth > Configuration.TabButtonMaxWidth)
            Configuration.TabButtonMinWidth = Configuration.TabButtonMaxWidth;

        Configuration.TabActiveBorderColor = tabActiveBorderColor;
        int oldMode = Configuration.TabCaptionMode;
        Configuration.TabCaptionMode = modes[index];
        bool modeChanged = (Configuration.TabCaptionMode != oldMode);
        int newAlignment = alignments[alignIndex];
        bool alignmentChanged = (Configuration.TabCaptionAlignment != newAlignment);
        Configuration.TabCaptionAlignment = newAlignment;
        bool minChanged = (Configuration.TabButtonMinWidth != oldMinWidth);
        bool maxChanged = (Configuration.TabButtonMaxWidth != oldMaxWidth);
        bool activeBorderChanged = (Configuration.TabActiveBorder != oldActiveBorder);
        bool activeBorderColorChanged = (Configuration.TabActiveBorderColor != oldActiveBorderColor);
        bool closeActiveChanged = (Configuration.TabCloseButtonActive != oldCloseButtonActive);
        bool closeAllChanged = (Configuration.TabCloseButtonAll != oldCloseButtonAll);
        if ((modeChanged || minChanged || maxChanged || activeBorderChanged || activeBorderColorChanged ||
             alignmentChanged || closeActiveChanged || closeAllChanged) &&
            MainWindow != NULL)
            MainWindow->RefreshPanelTabLayout();
    }
}

//
// ****************************************************************************
// CTaskListDialog
//

CTaskListDialog::CTaskListDialog(HWND parent) : CCommonDialog(HLanguage, IDD_TASKLIST, IDD_TASKLIST, parent)
{
    DisplayedVersion = 0;
}

void CTaskListDialog::Refresh()
{
    CALL_STACK_MESSAGE1("CTaskListDialog::Refresh()");

    HWND list = GetDlgItem(HWindow, IDC_SALAMLIST);
    if (list == NULL)
    {
        TRACE_E("Unexpected situation in CTaskListDialog::Refresh()");
        return;
    }

    // save the text of the previously selected item
    char oldSelected[250];
    int oldIndex = (int)SendMessage(list, LB_GETCURSEL, 0, 0);
    if (oldIndex == LB_ERR || SendMessage(list, LB_GETTEXT, oldIndex, (LPARAM)oldSelected) == LB_ERR)
        oldSelected[0] = 0;

    SendMessage(list, LB_RESETCONTENT, 0, 0);

    CProcessListItem items[MAX_TL_ITEMS];
    int c = TaskList.GetItems(items, &DisplayedVersion);
    DWORD PID = GetCurrentProcessId();
    int i;
    for (i = 0; i < c; i++)
    {
        char date[50], time[50];
        if (GetTimeFormat(LOCALE_USER_DEFAULT, 0, &items[i].StartTime, NULL, time, 50) == 0)
        {
            sprintf(time, "%u:%02u:%02u", items[i].StartTime.wHour, items[i].StartTime.wMinute,
                    items[i].StartTime.wSecond);
        }
        if (GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &items[i].StartTime, NULL, date, 50) == 0)
        {
            sprintf(date, "%u.%u.%u", items[i].StartTime.wDay, items[i].StartTime.wMonth,
                    items[i].StartTime.wYear);
        }

        char buf[100];
        sprintf(buf, LoadStr(IDS_TASKLISTLINE), items[i].PID, date, time,
                (items[i].PID == PID ? LoadStr(IDS_TASKLISTCURLINE) : ""));
        SendMessage(list, LB_ADDSTRING, 0, (LPARAM)buf);

        if (strcmp(buf, oldSelected) == 0)
            SendMessage(list, LB_SETCURSEL, i, 0);
    }
    if (SendMessage(list, LB_GETCURSEL, 0, 0) == LB_ERR)
        SendMessage(list, LB_SETCURSEL, 0, 0); // fallback
}

DWORD
CTaskListDialog::GetCurPID()
{
    HWND list = GetDlgItem(HWindow, IDC_SALAMLIST);
    if (list != NULL)
    {
        int i = (int)SendMessage(list, LB_GETCARETINDEX, 0, 0);
        char buf[100];
        if (SendMessage(list, LB_GETTEXT, i, (LPARAM)buf) != LB_ERR)
        {
            char* s = buf + 4;
            char* end = s;
            while (*end != ' ' && *end != 0)
                end++;
            *end = 0;
            return atoi(s);
        }
    }
    return -1;
}

INT_PTR
CTaskListDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CTaskListDialog::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        Refresh();
        SetTimer(HWindow, IDT_UPDATETASKLIST, 200, NULL);
        break;
    }

    case WM_DESTROY:
    {
        KillTimer(HWindow, IDT_UPDATETASKLIST);
        break;
    }

    case WM_TIMER:
    {
        if (wParam == IDT_UPDATETASKLIST)
        {
            DWORD version;
            TaskList.GetItems(NULL, &version);
            if (version > DisplayedVersion)
                Refresh();
            return 0;
        }
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDB_BREAKTASK:
        {
            DWORD pid = GetCurPID();
            if (pid == GetCurrentProcessId())
            {
                SalMessageBox(HWindow, LoadStr(IDS_CURRENTPROCESSBREAK), LoadStr(IDS_ERRORTITLE),
                              MB_OK | MB_ICONEXCLAMATION);
                return 0;
            }
            if (pid != -1)
            {
                if (SalMessageBox(HWindow, LoadStr(IDS_CONFIRM_BREAK), LoadStr(IDS_QUESTION),
                                  MB_YESNO | MSGBOXEX_ESCAPEENABLED | MB_ICONQUESTION) == IDYES)
                {
                    if (!TaskList.FireEvent(TASKLIST_TODO_BREAK, pid))
                    {
                        TRACE_I("Unable to deliver break-message.");
                    }
                }
            }
            return 0;
        }

        case IDB_KILLTASK:
        {
            DWORD pid = GetCurPID();
            if (pid == GetCurrentProcessId())
            {
                if (SalMessageBox(HWindow, LoadStr(IDS_CURRENTPROCESSTERMINATE), LoadStr(IDS_QUESTION),
                                  MB_YESNO | MSGBOXEX_ESCAPEENABLED | MB_DEFBUTTON2 | MB_ICONQUESTION) == IDYES)
                {
                    TRACE_I("CTaskListDialog::DialogProc(IDB_KILLTASK): calling ExitProcess(668).");
                    // ExitProcess(668);
                    TerminateProcess(GetCurrentProcess(), 668); // harder exit (this call still performs some operations)
                }
                return 0;
            }
            if (pid != -1)
            {
                if (SalMessageBox(HWindow, LoadStr(IDS_CONFIRM_TERMINATE), LoadStr(IDS_QUESTION),
                                  MB_YESNO | MSGBOXEX_ESCAPEENABLED | MB_ICONQUESTION) == IDYES)
                {
                    if (!TaskList.FireEvent(TASKLIST_TODO_TERMINATE, pid))
                    {
                        TRACE_I("Unable to deliver terminate-message.");
                    }
                }
            }
            return 0;
        }

        case IDB_SHOWTASK:
        {
            DWORD pid = GetCurPID();
            if (pid != -1)
            {
                TaskList.FireEvent(TASKLIST_TODO_HIGHLIGHT, pid);
            }
            return 0;
        }

        case IDB_REFRESHLIST:
        {
            Refresh();
            return 0;
        }
        }
        break;
    }
    }
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CCfgPageKeyboard
//

CCfgPageKeyboard::CCfgPageKeyboard()
    : CCommonPropSheetPage(NULL, HLanguage, IDD_CFGPAGE_KEYBOARD, IDD_CFGPAGE_KEYBOARD, PSP_USETITLE, NULL)
{
}

void
CCfgPageKeyboard::Transfer(CTransferInfo& ti)
{
    ti.RadioButton(IDC_KEYBOARD_BACKSPACE_PARENT, 0, Configuration.BackspaceAction);
    ti.RadioButton(IDC_KEYBOARD_BACKSPACE_HISTORY, 1, Configuration.BackspaceAction);
}

INT_PTR
CCfgPageKeyboard::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CTaskListDialog::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        int staticsArr[] = {IDC_STATIC_2, IDC_KEYBOARD_SHORTCUTS, 0};
        CondenseStaticTexts(HWindow, staticsArr);

        CHyperLink* hl = new CHyperLink(HWindow, IDC_KEYBOARD_SHORTCUTS);
        if (hl != NULL)
            hl->SetActionPostCommand(CM_HELP_KEYBOARD);
        //        hl->SetActionOpen("https://www.altap.cz/salam_en/features/keyboard.html"); // beware, one more occurrence
        break;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == CM_HELP_KEYBOARD)
        {
            OpenHtmlHelp(NULL, HWindow, HHCDisplayContext, CM_HELP_KEYBOARD, FALSE);
            return 0;
        }
        break;
    }
    }
    return CCommonPropSheetPage::DialogProc(uMsg, wParam, lParam);
}
