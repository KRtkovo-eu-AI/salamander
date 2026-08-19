# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    model = (ROOT / "salamdr4.cpp").read_text(encoding="utf-8")
    header = (ROOT / "salamand.h").read_text(encoding="utf-8")
    dialog = (ROOT / "dialogs4.cpp").read_text(encoding="utf-8")
    dark_mode = (ROOT / "darkmode.cpp").read_text(encoding="utf-8")
    edit_list = (ROOT / "edtlbwnd.cpp").read_text(encoding="utf-8")
    panel = (ROOT / "fileswn9.cpp").read_text(encoding="utf-8")
    menu = (ROOT / "mainwnd3.cpp").read_text(encoding="utf-8")
    sort_menu = (ROOT / "fileswnb.cpp").read_text(encoding="utf-8")
    gui = (ROOT / "gui.cpp").read_text(encoding="utf-8")
    file_tags = (ROOT / "filetags.h").read_text(encoding="utf-8")
    file_window = (ROOT / "fileswn5.cpp").read_text(encoding="utf-8")
    find = (ROOT / "find.cpp").read_text(encoding="utf-8")
    filter_source = (ROOT / "filter.cpp").read_text(encoding="utf-8")
    installer = (ROOT.parent / "doc" / "runbook-setup" / "inno_setup_salamander_x64.iss").read_text(encoding="utf-8")

    require(
        "SALAMANDER_VIEWTEMPLATE_AVAILABLEEXPLORERCOLUMNS" in model
        and "GetExplorerColumnCanonicalName(i)" in model,
        "available Explorer properties must persist by stable canonical identity",
    )
    require(
        "SALAMANDER_VIEWTEMPLATE_EXPLORERAVAILABILITYVERSION" in model
        and "EXPLORER_COLUMN_AVAILABILITY_VERSION" in model
        and "memcpy(Get(2)->ExplorerColumnAvailable, ExplorerColumnAvailable" in model
        and "explorerAvailabilityVersion >= EXPLORER_COLUMN_AVAILABILITY_VERSION" in model
        and "Get(i)->ExplorerColumnAvailable[j] = TRUE;" in model
        and "RebuildExplorerColumnAvailable();" in model,
        "legacy and broken cloned configurations must migrate to independent per-view subsets while preserving visible columns",
    )
    require(
        "IsExplorerColumnAvailable(explorerIndex)" in panel,
        "globally removed Explorer properties must not remain visible in panels",
    )
    require(
        "column->GetText != InternalGetExplorerColumn" in sort_menu
        and "CM_LEFTSORTBY_MIN" in sort_menu
        and "CM_RIGHTSORTBY_MIN" in sort_menu
        and "SortType == stCustom && SortCustomData == column->CustomData" in sort_menu
        and "GetExplorerSortColumnByMenuIndex" in sort_menu
        and "targetPanel->ChangeCustomSortType(explorerIndex, TRUE);" in menu
        and "IsDetachedTabActive() && DetachedTabOriginalSide" in menu,
        "Sort By must list displayed Windows property columns, mark the active one, and route commands to main or detached panels",
    )
    require(
        "class CExplorerColumnsDialog" in (ROOT / "cfgdlg.h").read_text(encoding="utf-8")
        and "IDS_EXCOL_COMMON" in dialog
        and "GetExplorerColumnCategory" in dialog
        and "IDC_EXCOL_SEARCH" in dialog
        and "TVIF_PARAM" in dialog
        and "ListView_EnableGroupView(list, TRUE)" in dialog,
        "the property selector must retain working category filters, grouped categories, Common, and live search",
    )
    require(
        "void PaintDarkListViewGroupHeaders(HWND hwnd, HDC hdc, COLORREF background)" in dark_mode
        and "LVM_GETGROUPINFOBYINDEX" in dark_mode
        and "headerRect.top = LVGGR_HEADER;" in dark_mode
        and "LVM_GETGROUPRECT" in dark_mode
        and '#include "common/winlibdpi.h"' in dark_mode
        and "WinLibDPIGetWindowDPI(hwnd)" in dark_mode
        and "GetDPIForWindow(hwnd)" not in dark_mode
        and "FillRectWithColor(hdc, visibleRect, background);" in dark_mode
        and "DrawTextW(hdc, header, -1, &textRect, format);" in dark_mode
        and "PaintDarkListViewGroupHeaders(hwnd, hdc, background);" in dark_mode
        and "customDraw->dwItemType == LVCDI_ITEM" in dialog
        and "LVGMF_TEXTCOLOR" not in dialog,
        "dark grouped list views must repaint actual group rectangles after native painting and not treat groups as checkbox items",
    )
    require(
        "IDC_EXCOL_SELECTED_LIST" in dialog
        and "void CExplorerColumnsDialog::MoveSelected" in dialog
        and "ApplySelectedOrder();" in dialog,
        "selected properties must remain directly removable and reorderable",
    )
    require(
        '"ExplorerCategoryDocument"' in dialog
        and '"ExplorerCategoryImage"' in dialog
        and '"ExplorerCategoryAudio"' in dialog
        and '"ExplorerCategoryVideo"' in dialog
        and '"ExplorerCategoryExecutable"' in dialog
        and '"ExplorerCategoryArchive"' in dialog,
        "category rows must use category-specific icons",
    )
    require(
        "GetPanelTipPropertyKeys(category, keys" in panel
        and "int GetPanelTipPropertyKeys" in model
        and "IsExplorerColumnInPanelTipCategory(index, ptcExecutable)" in dialog
        and "IsExplorerColumnInPanelTipCategory(index, ptcArchive)" in dialog,
        "Executable and Archive filters must share the panel file-tooltip property subsets",
    )
    require(
        "IsExplorerColumnCompatibleWithCategory" in dialog
        and '"System.ProductName"' in dialog
        and '"System.Media."' in dialog
        and "nativeCategory == eccAudio" in dialog
        and "IsUsefulDescriptiveExplorerColumn(index)" in dialog
        and "IsUsefulFileMetadataExplorerColumn(index)" in dialog,
        "file-type categories must include useful properties whose native Windows category differs",
    )
    require(
        "GetPreferredExplorerNativeCategory" in dialog
        and "categoryOrder[categoryCount++] = preferredCategory;" in dialog
        and "category != preferredCategory" in dialog
        and "GetExplorerColumnCategory(i) != category" in dialog
        and "item.iGroupId = category;" in dialog,
        "type filters must show their matching native subcategory first and insert every group's items with its header",
    )
    require(
        "BYTE ExplorerColumnFavorite[EXPLORER_COLUMNS_COUNT]" in header
        and "SALAMANDER_VIEWTEMPLATE_FAVORITEEXPLORERCOLUMNS" in model
        and "SaveExplorerColumnSet(hKey, SALAMANDER_VIEWTEMPLATE_FAVORITEEXPLORERCOLUMNS" in model
        and "LoadExplorerColumnSet(hKey, SALAMANDER_VIEWTEMPLATE_FAVORITEEXPLORERCOLUMNS" in model
        and "GetExplorerColumnCanonicalName(i)" in model
        and "ecfFavorites" in dialog
        and "IDC_EXCOL_FAVORITE" in dialog
        and 'isFavorite ? "ExplorerCategoryFavoritesRed" : "ExplorerCategoryFavorites"' in dialog
        and "TTM_UPDATETIPTEXT" in dialog
        and "memcpy(Config->ExplorerColumnFavorite, Favorite" in dialog,
        "Favorites must be editable from Property information and persist globally by canonical identity",
    )
    require(
        (ROOT / "res" / "toolbars" / "ExplorerCategoryFavorites.svg").is_file()
        and (ROOT / "res" / "toolbars" / "darkmode" / "ExplorerCategoryFavorites.svg").is_file()
        and installer.count("ExplorerCategoryFavorites.svg") == 2,
        "the Favorites icon must ship in light and dark variants",
    )
    require(
        '"%s\\r\\n%s: %s\\r\\n%s: %s\\r\\n%s: %s\\r\\n%s: %s%s%s"' in dialog
        and '"%s\\r\\n\\r\\n%s: %s' not in dialog,
        "property information fields must use compact single-line spacing",
    )
    require(
        "const int leadingButtonOrder[] = {7, 8}; // Filter, Search" in gui
        and "LeadingButtonMask" in gui
        and "const int buttonOrder[TLBHDR_COUNT] = {0, 1, 2, 3, 6, 4, 5, 9, 8, 7};" in gui
        and "0); // the FILTER slot is the Windows-properties button and stays rightmost" in dialog
        and "ILC_COLOR32 | ILC_MASK" in gui,
        "Search/Filter must lead the right-aligned button block while the Windows-property action stays rightmost with SVG alpha",
    )
    require(
        "TLBHDRMASK_SEARCH | TLBHDRMASK_FILTER" in edit_list
        and "ToggleFilter();" in edit_list
        and "FilterVisible ? TLBHDRMASK_FILTER" in edit_list,
        "edit-list headers must expose mutually exclusive Search and Filter modes",
    )
    require(
        "TIndirectArray<CViewTemplate> ExtraItems" in header
        and "GetIndex(const CViewTemplate* item)" in header
        and "CM_ACTIVEEXTRAMODE_MIN" in menu
        and "if (i < VIEW_TEMPLATES_COUNT)" in menu,
        "additional detailed views must use stable storage and menu-only commands",
    )
    require(
        "ReadFileTagsW" in file_tags
        and "WriteFileTagsW" in file_tags
        and "GPS_READWRITE" in model
        and "store->SetValue(key, value)" in model
        and "store->Commit()" in model
        and "BOOL IsFilePropertyWritableW" in model
        and "capabilities->IsPropertyWritable(key) == S_OK" in model
        and "TagsWritable = IsPropertyWritableForAll(PKEY_Keywords)" in file_window
        and "EnableWindow(tagsCheck, TagsWritable)" in file_window
        and "SendMessage(tagsCheck, BM_SETCHECK, BST_UNCHECKED, 0)" in file_window
        and "AddValueToStdHistoryValues(Configuration.TagHistory" in file_window,
        "Tags UI must disable and uncheck unsupported selections while writes still use SetValue/Commit",
    )
    require(
        "IsPropertyWritableForAll(*key)" in file_window
        and "!PropertyRows[change->iItem].Writable" in file_window
        and "SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE)" in file_window
        and "if (!row.Writable)" in file_window
        and "IDS_EDPROP_RESULT_DETAIL" in file_window
        and "IDS_EDPROP_REASON_UNSUPPORTED" in file_window,
        "unsupported custom properties must be unchecked and disabled and write errors must explain why",
    )
    require(
        "std::wstring OriginalValue;" in file_window
        and "int InitialState;" in file_window
        and "row.Value != row.OriginalValue" in file_window
        and "GetPropertyState(rowIndex) != row.InitialState" in file_window
        and "if (state == BST_INDETERMINATE || !row.Modified)" in file_window
        and "BOOL tagsChanged = tagsState != InitialTagsState || tags != InitialTags;" in file_window
        and "TagsWritable && tagsChanged" in file_window,
        "Edit Tags and Windows Properties must write only values or checkbox states explicitly changed by the user",
    )
    require(
        "BOOL Modified;" in file_window
        and "void UpdatePropertyModifiedState(int rowIndex)" in file_window
        and 'RenderSVGIconBitmap("Modify"' in file_window
        and "item.iImage = row.Modified && PropertyImages != NULL ? 0 : I_IMAGENONE;" in file_window
        and "propertyAttempted[rowIndex] && !propertyFailed[rowIndex]" in file_window
        and "row.OriginalValue = row.Value;" in file_window
        and "UpdatePropertyModifiedState((int)rowIndex);" in file_window,
        "edited property rows must show an icon which clears only after every target file was updated successfully",
    )
    require(
        "InitFilePropertyValueFromText" in model
        and "description->GetEnumTypeList" in model
        and "IID_IPropertyEnumType" in model
        and "enumType->GetDisplayText" in model
        and "enumType->GetValue(value)" in model
        and "description->CoerceToCanonicalValue(value)" in model,
        "writable scalar and enum properties such as JPEG Orientation must convert display text to their canonical PROPVARIANT type",
    )
    require(
        "LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER" in file_window
        and "ListView_InsertColumn(PropertiesList, 0" in file_window
        and "ListView_InsertColumn(PropertiesList, 1" in file_window
        and "BeginPropertyEdit" in file_window
        and "ListView_GetSubItemRect(PropertiesList, row, 1" in file_window
        and "MainWindow->ViewTemplates.IsExplorerColumnAvailable(i)" in file_window
        and "if (!hadValue && !MainWindow->ViewTemplates.IsExplorerColumnAvailable(i))" in file_window,
        "Windows properties must use an editable Name/Value checklist and include selected or populated properties",
    )
    require(
        "Header->EnableToolbar(0);" in edit_list
        and "IsWindowEnabled(HWindow)" in gui
        and "DarkModeGetDisabledTextColor()" in gui,
        "disabling Tags must visibly disable its list, header caption, and header commands",
    )
    require(
        "EditWindowsProperties" in file_window
        and "IsExplorerColumnAvailable(i)" in file_window
        and "PKEY_Keywords" in file_window
        and "CEditListBox" in file_window
        and "IDC_EDPROP_TAGS_LIST" in file_window
        and "ELB_RIGHTARROW" in file_window
        and "BST_INDETERMINATE" in file_window
        and "CreateWindowExW" in file_window
        and "info->Index <= (int)Tags.size()" in file_window
        and "TagsList->SetItemID(i, (INT_PTR)Tags[i]);" in file_window,
        "the Files command must expose the Salamander Tags editor and checkbox-gated selected properties",
    )
    require(
        "Criteria.TestTags" in find
        and "IDC_FFA_USETAGS" in filter_source
        and "ftmmAny" in file_tags
        and "ftmmAll" in file_tags
        and "ftmmNone" in file_tags,
        "Find Advanced must support any/all/none exact tag matching",
    )
    lang_rc = (ROOT / "lang" / "lang.rc").read_text(encoding="utf-8")
    lang_texts = (ROOT / "lang" / "texts.rc2").read_text(encoding="utf-8")
    require(
        "IDD_EXPLORER_COLUMNS DIALOGEX 0, 0, 420, 320" in lang_rc,
        "the Windows property chooser must keep compact Configuration-like proportions",
    )
    require(
        "MulDiv(24, GetDPIForWindow(HWindow), USER_DEFAULT_SCREEN_DPI)" in dialog
        and "iconSize + MulDiv(2, GetDPIForWindow(HWindow), USER_DEFAULT_SCREEN_DPI)" in dialog
        and "MulDiv(18, GetDPIForWindow(HWindow), USER_DEFAULT_SCREEN_DPI)" in dialog
        and '"%s\\r\\n%s: %s\\r\\n%s: %s\\r\\n%s: %s\\r\\n%s: %s%s%s"' in dialog
        and 'GetExplorerColumnDescription(index)[0] != 0 ? "\\r\\n" : ""' in dialog,
        "the Windows property chooser must use Configuration-like compact rows and detail spacing",
    )
    require(
        "const int contentTop = ExplorerColumnsDialogY(HWindow, 27);" in dialog
        and "int detailsTop = contentTop + selectedHeight;" in dialog
        and "2 * rightHeight / 3" in dialog
        and "IDC_EXCOL_CATEGORIES,\"SysTreeView32\"" in lang_rc
        and ",4,27,105,271" in lang_rc
        and "IDC_EXCOL_SELECTED_GROUP,262,23,154,180" in lang_rc
        and "IDC_EXCOL_SELECTED_LIST,\"SysListView32\"" in lang_rc
        and ",270,37,138,135" in lang_rc
        and "IDC_EXCOL_DETAILS_GROUP,262,207,154,91" in lang_rc,
        "the chooser must add space below Search and reserve two thirds of the right side for selected properties",
    )
    require(
        "hdr->idFrom == IDC_EXCOL_CATEGORIES && hdr->code == NM_CLICK" in dialog
        and "hit.hItem != NULL && (hit.flags & TVHT_ONITEMBUTTON) == 0" in dialog
        and "TreeView_GetFirstVisible(hdr->hwndFrom)" in dialog
        and "hit.pt.y >= row.top && hit.pt.y < row.bottom" in dialog
        and "TreeView_SelectItem(hdr->hwndFrom, item);" in dialog,
        "the entire visible category row must be clickable, including empty space after its text",
    )
    require(
        "wParam == IDC_EXCOL_DETAILS" in dialog
        and "const int lineGap = ExplorerColumnsDialogY(HWindow, 2);" in dialog
        and "DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT" in dialog
        and "IDC_EXCOL_DETAILS,270,221,138,69,SS_LEFT | SS_NOPREFIX | SS_OWNERDRAW" in lang_rc,
        "Property information must preserve wrapping and add exactly 2 DLU between rows",
    )
    require(
        "ExplorerColumnsDialogX" in dialog
        and "ExplorerColumnsDialogY" in dialog
        and "MapDialogRect(dialog, &rect);" in dialog
        and "if (MinWidth == 0 || MinHeight == 0)" in dialog
        and "MinHeight = windowRect.bottom - windowRect.top;" in dialog
        and "int detailsRight = detailsLeft + detailsWidth;" in dialog
        and "detailsRight - ExplorerColumnsDialogX(HWindow, 50), buttonsY" in dialog
        and "SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE" in dialog
        and "LayoutControls();" in dialog,
        "the resizable Windows property chooser must convert dialog units to pixels and align Cancel with Property Information",
    )
    require(
        "const int favoriteButtonSize = max(1, MulDiv(moveButtonSize, 2, 3));" in dialog
        and "detailsRight - detailsInsetX - favoriteButtonSize" in dialog
        and "detailsTop + detailsInsetY" in dialog
        and "int iconSize = max(1, min(buttonRect.right, buttonRect.bottom));" in dialog
        and 'IDC_EXCOL_FAVORITE,"Button",BS_ICON | WS_TABSTOP,399,207,9,9' in lang_rc
        and 'DEFPUSHBUTTON   "OK",IDOK,309,302,50,14' in lang_rc
        and 'PUSHBUTTON      "Cancel",IDCANCEL,366,302,50,14' in lang_rc,
        "the compact Favorites button and bottom action buttons must keep the current aligned layout",
    )
    require(
        "BeginDeferWindowPos(15)" in dialog
        and "SWP_NOZORDER | SWP_NOREDRAW" in dialog
        and "RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW" in dialog,
        "the chooser resize must reposition children atomically and repaint the complete dialog",
    )
    require(
        "IDD_EXPLORER_COLUMNS DIALOGEX 0, 0, 420, 320" in lang_rc
        and 'CONTROL         "Move up",IDC_EXCOL_MOVE_UP,"Button",BS_ICON | WS_TABSTOP,324,178,14,14' in lang_rc
        and 'CONTROL         "Move down",IDC_EXCOL_MOVE_DOWN,"Button",BS_ICON | WS_TABSTOP,344,178,14,14' in lang_rc
        and 'const char* iconNames[] = {"MoveItemUp", "MoveItemDown"};' in dialog
        and "const int moveButtonSize = ExplorerColumnsDialogY(HWindow, 14);" in dialog,
        "the chooser must be narrower and use square SVG Move up/down buttons",
    )
    main_window_config = (ROOT / "mainwnd2.cpp").read_text(encoding="utf-8")
    require(
        "Configuration.ExplorerColumnsDialogWidth" in dialog
        and "Configuration.ExplorerColumnsDialogHeight" in dialog
        and "CONFIG_EXPLORER_COLUMNS_DIALOG_WIDTH" in main_window_config
        and "CONFIG_EXPLORER_COLUMNS_DIALOG_HEIGHT" in main_window_config,
        "the Windows property chooser must persist its last user-selected size",
    )
    require(
        "if (Available[i] && !view->ExplorerColumnAvailable[i])" in dialog
        and "view->ExplorerColumnVisible[i] = TRUE;" in dialog
        and "else if (!Available[i])" in dialog
        and "view->ExplorerColumnVisible[i] = FALSE;" in dialog,
        "added Explorer properties must be checked and removed properties hidden only for the current view",
    )
    require(
        "if (view->ExplorerColumnVisible[i])" in dialog
        and "Available[i] = TRUE;" in dialog
        and "if (Get(i)->ExplorerColumnVisible[j])" in model
        and "Get(i)->ExplorerColumnAvailable[j] = TRUE;" in model,
        "persisted visible Explorer properties must always be restored into chooser availability",
    )
    require(
        "void CExplorerColumnsDialog::NormalizeSelectedOrder()" in dialog
        and "NormalizeSelectedOrder();" in dialog
        and "BOOL oldDisableNotification = DisableNotification;" in dialog
        and "Available[index] && !used[index]" in dialog
        and "SelectedCount = count;" in dialog,
        "every checked Windows property must be normalized into Selected properties before the list is filled",
    )
    require(
        "item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;" in dialog
        and "item.iImage = I_IMAGENONE;" in dialog
        and "int inserted = ListView_InsertItem(list, &item);" in dialog,
        "Selected properties rows must explicitly opt out of the spacing image list",
    )
    require(
        'SetDlgItemText(HWindow, IDC_EXCOL_SEARCH, "");' not in dialog
        and dialog.count("BOOL oldDisableNotification = DisableNotification;") >= 2
        and dialog.count("DisableNotification = oldDisableNotification;") >= 2,
        "chooser initialization must not populate properties before checkbox styles exist, and nested fills must preserve notification suppression",
    )
    require(
        "void CExplorerColumnsDialog::UpdateDetails(HWND sourceList)" in dialog
        and dialog.count("UpdateDetails(hdr->hwndFrom);") >= 2
        and "if (list != propertiesList && list != selectedList)" in dialog,
        "Property information must use the listview which emitted the selection notification instead of relying on focus timing",
    )
    require(
        "FormatExplorerLocalizedArguments" in dialog
        and "_snprintf_s(text, _TRUNCATE, LoadStr(IDS_EXCOL_COUNT), selected)" not in dialog
        and "FormatEditPropertiesLocalizedArguments" in file_window
        and "LoadStr(IDS_EDPROP_RESULT), updated, failed" not in file_window
        and "LoadStr(IDS_EDPROP_RESULT_DETAIL), path.c_str()" not in file_window
        and file_window.count("new (std::nothrow) CTagItem") >= 2
        and "new (std::nothrow) CEditListBox" in file_window,
        "localized property summaries must avoid dynamic printf formats and low-memory branches must use nothrow allocation",
    )
    require(
        "void CCfgPageView::SyncExplorerColumnAvailabilityFromList(int viewIndex, BYTE* available)" in dialog
        and "view->ExplorerColumnAvailable[explorerIndex] = TRUE;" in dialog
        and "available[explorerIndex] = TRUE;" in dialog
        and "if (view != NULL)\n                    view->ExplorerColumnAvailable[explorerIndex] = TRUE;" in dialog
        and "SyncExplorerColumnAvailabilityFromList(viewIndex, available);" in dialog
        and "CExplorerColumnsDialog dialog(HWindow, &Config, viewIndex, available);" in dialog
        and "available != NULL ? available" in dialog
        and "BYTE available[EXPLORER_COLUMNS_COUNT];" in dialog
        and "if (index >= 0)\n        SelectIndex = index;" in dialog
        and "if (viewIndex < 0)\n                viewIndex = SelectIndex;" in dialog
        and dialog.index("SyncExplorerColumnAvailabilityFromList(viewIndex, available);")
        < dialog.index("SetFocus(HListView2);")
        < dialog.index("StoreControls();", dialog.index("if (LOWORD(wParam) == IDC_VIEWLIST_HEADER2)"))
        and "view->ExplorerColumnAvailable[explorerIndex] = TRUE;" in dialog,
        "Available Columns and the chooser must share an explicit snapshot of the selected view",
    )
    require(
        "ecfPlugins" in dialog
        and "LoadStr(IDS_EXCOL_PLUGINS)" in dialog
        and "for (int pluginIndex = 0; pluginIndex < Plugins.GetCount(); pluginIndex++)" in dialog
        and "ListView_InsertGroup(list, -1, &group);" in dialog
        and "definition->RuntimeAvailable" in dialog
        and '"extension:", 10' in dialog
        and "firstDefinition->OwnerName" in dialog
        and "BYTE PluginAvailable[PLUGIN_COLUMNS_COUNT]" in (ROOT / "cfgdlg.h").read_text(encoding="utf-8"),
        "Plugins must have their own category, installed-plugin subgroups, and per-view checkbox availability",
    )
    require(
        "WORD AllColumnOrder[VIEW_COLUMNS_COUNT]" in header
        and "SALAMANDER_VIEWTEMPLATE_ALLCOLUMNORDER" in model
        and "NormalizeViewColumnOrder" in model
        and "normalized[out++] = 0; // Name is permanently first." in model
        and "view.FinalizePluginColumns();" in (ROOT / "fileswn2.cpp").read_text(encoding="utf-8")
        and (ROOT / "fileswn3.cpp").read_text(encoding="utf-8").count("view.FinalizePluginColumns();") == 2
        and "RegisterPluginColumn" in model
        and "PluginColumnAvailable[catalogIndex]" in model,
        "native, Windows, and discovered plugin columns must share one persisted order with Name fixed first",
    )
    require(
        "SaveExplorerColumnAvailable(actKey, view->ExplorerColumnAvailable" in model
        and "LoadExplorerColumnAvailable(actKey, view->ExplorerColumnAvailable" in model
        and "BYTE ExplorerColumnAvailable[EXPLORER_COLUMNS_COUNT]; // Explorer properties offered by this view" in header,
        "each view must persist its Windows-property subset by canonical identity",
    )
    require(
        "LoadStr(IDS_EXCOL_VIEW_TITLE)" in dialog
        and 'IDS_EXCOL_VIEW_TITLE, "%s - Choose Windows Properties"' in lang_texts,
        "the chooser title must identify the view being edited",
    )
    require(
        "case LVN_BEGINLABELEDITA:" in dialog
        and "case LVN_BEGINLABELEDITW:" in dialog
        and "case LVN_ENDLABELEDITA:" in dialog
        and "case LVN_ENDLABELEDITW:" in dialog
        and "NMLVDISPINFOW* nmhd" in dialog
        and "NMLVDISPINFOA* nmhd" in dialog
        and "nmhk->wVKey == VK_INSERT)\n                    OnNew();" in dialog
        and "SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);" in dialog
        and "ListView_SetItemText(HListView, index, 0, name);" not in dialog,
        "custom-view label editing must accept both ANSI and Unicode notifications without overwriting the native edit result",
    )
    require(
        "IDC_EDPROP_TAGS_ENABLE" in lang_rc
        and "IDC_EDPROP_TAGS_LIST" in lang_rc
        and "IDC_EDPROP_PROPERTIES_GROUP" in lang_rc
        and "IDC_EDPROP_OPERATION" not in lang_rc.split("IDD_EDIT_PROPERTIES", 1)[1].split("END", 1)[0]
        and 'IDS_MENU_FILES_EDITPROPERTIES, "Edit &Tags and Windows Properties..."' in lang_texts,
        "property editing must use the form layout and the requested Files-menu caption",
    )
    require(
        'GROUPBOX        "",IDC_EDPROP_TAGS_GROUP,7,7,416,110' in lang_rc
        and 'CONTROL         "Change Tags",IDC_EDPROP_TAGS_ENABLE,"Button",BS_AUTO3STATE | WS_TABSTOP,14,4,90,12' in lang_rc
        and "LISTBOX         IDC_EDPROP_TAGS_LIST,14,33,402,76" in lang_rc
        and 'GROUPBOX        "Windows properties",IDC_EDPROP_PROPERTIES_GROUP,7,125,416,139' in lang_rc
        and "IDD_EDIT_PROPERTIES DIALOGEX 0, 0, 430, 291" in lang_rc
        and 'DEFPUSHBUTTON   "OK",IDOK,319,270,50,14' in lang_rc
        and 'PUSHBUTTON      "Cancel",IDCANCEL,373,270,50,14' in lang_rc,
        "Edit Tags must be the aligned title of a matching group and the dialog must not leave excess space above its buttons",
    )
    require(
        "SetWindowTextW(HWindow, title.c_str());" in file_window
        and "MultiByteToWideChar(CP_ACP" in file_window
        and "Paths[0].find_last_of(L\"\\\\/\")" in file_window
        and "ExpandPluralFilesDirs(fileCount" in file_window
        and "epfdmNormal, FALSE" in file_window
        and "LoadStr(IDS_EDPROP_VIEW_TITLE)" in file_window
        and 'IDS_EDPROP_VIEW_TITLE, "%s - Edit Tags and Windows Properties"' in lang_texts,
        "the Edit Tags and Windows Properties title must identify one Unicode file or a localized plural file count",
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(error, file=sys.stderr)
        raise SystemExit(1)
