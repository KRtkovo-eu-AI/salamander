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
    edit_list = (ROOT / "edtlbwnd.cpp").read_text(encoding="utf-8")
    panel = (ROOT / "fileswn9.cpp").read_text(encoding="utf-8")
    menu = (ROOT / "mainwnd3.cpp").read_text(encoding="utf-8")
    gui = (ROOT / "gui.cpp").read_text(encoding="utf-8")
    file_tags = (ROOT / "filetags.h").read_text(encoding="utf-8")
    file_window = (ROOT / "fileswn5.cpp").read_text(encoding="utf-8")
    find = (ROOT / "find.cpp").read_text(encoding="utf-8")
    filter_source = (ROOT / "filter.cpp").read_text(encoding="utf-8")

    require(
        "SALAMANDER_VIEWTEMPLATE_AVAILABLEEXPLORERCOLUMNS" in model
        and "GetExplorerColumnCanonicalName(i)" in model,
        "available Explorer properties must persist by stable canonical identity",
    )
    require(
        "if (!availableLoaded)" in model
        and "ExplorerColumnVisible[j]" in model,
        "legacy configurations must migrate the union of used Explorer columns",
    )
    require(
        "IsExplorerColumnAvailable(explorerIndex)" in panel,
        "globally removed Explorer properties must not remain visible in panels",
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
        and "IsExplorerColumnInPanelTipCategory(explorerIndex, ptcExecutable)" in dialog
        and "IsExplorerColumnInPanelTipCategory(explorerIndex, ptcArchive)" in dialog,
        "Executable and Archive filters must share the panel file-tooltip property subsets",
    )
    require(
        '"%s\\r\\n\\r\\n%s: %s\\r\\n\\r\\n%s: %s\\r\\n\\r\\n%s: %s\\r\\n\\r\\n%s: %s%s%s"' in dialog,
        "property information fields must have readable blank-line spacing",
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
        and "EnableWindow(check, writable)" in file_window
        and "if (!row.Writable)" in file_window
        and "IDS_EDPROP_RESULT_DETAIL" in file_window
        and "IDS_EDPROP_REASON_UNSUPPORTED" in file_window,
        "unsupported custom properties must be unchecked and disabled and write errors must explain why",
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
        "IDD_EXPLORER_COLUMNS DIALOGEX 0, 0, 460, 320" in lang_rc,
        "the Windows property chooser must keep compact Configuration-like proportions",
    )
    require(
        "ExplorerColumnsDialogX" in dialog
        and "ExplorerColumnsDialogY" in dialog
        and "MapDialogRect(dialog, &rect);" in dialog
        and "if (MinWidth == 0 || MinHeight == 0)" in dialog
        and "MinHeight = windowRect.bottom - windowRect.top;" in dialog
        and "SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE" in dialog
        and "LayoutControls();" in dialog,
        "the resizable Windows property chooser must convert dialog units to pixels after initialization",
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
        "if (Available[i] && !Config->ExplorerColumnAvailable[i])" in dialog
        and "view->ExplorerColumnVisible[i] = TRUE;" in dialog,
        "newly added Explorer properties must be checked for the current view",
    )
    require(
        "void CCfgPageView::SyncExplorerColumnAvailabilityFromList()" in dialog
        and "Config.SetExplorerColumnAvailable(explorerIndex, TRUE);" in dialog
        and "StoreControls();\n                SyncExplorerColumnAvailabilityFromList();" in dialog,
        "the chooser must merge Windows properties already present in Available Columns before taking its snapshot",
    )
    require(
        "IDC_EDPROP_TAGS_ENABLE" in lang_rc
        and "IDC_EDPROP_TAGS_LIST" in lang_rc
        and "IDC_EDPROP_PROPERTIES_GROUP" in lang_rc
        and "IDC_EDPROP_OPERATION" not in lang_rc.split("IDD_EDIT_PROPERTIES", 1)[1].split("END", 1)[0]
        and 'IDS_MENU_FILES_EDITPROPERTIES, "Edit &Tags and Windows Properties..."' in lang_texts,
        "property editing must use the form layout and the requested Files-menu caption",
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(error, file=sys.stderr)
        raise SystemExit(1)
