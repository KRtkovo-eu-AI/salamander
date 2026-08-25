# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def function_slice(text: str, start: str, end: str) -> str:
    first = text.find(start)
    last = text.find(end, first + len(start))
    if first < 0 or last < 0:
        raise AssertionError(f"Cannot locate function section: {start}")
    return text[first:last]


def require(text: str, needle: str, description: str) -> None:
    if needle not in text:
        raise AssertionError(f"Missing {description}: {needle}")


def require_absent(text: str, needle: str, description: str) -> None:
    if needle in text:
        raise AssertionError(f"Unexpected {description}: {needle}")


def require_after(text: str, first: str, second: str, description: str) -> None:
    first_pos = text.find(first)
    second_pos = text.find(second)
    if first_pos < 0 or second_pos < 0 or first_pos >= second_pos:
        raise AssertionError(f"Invalid ordering for {description}")


def check_find(src: Path) -> None:
    find_h = (src / "find.h").read_text(encoding="utf-8")
    finddlg1 = (src / "finddlg1.cpp").read_text(encoding="utf-8")
    finddlg2 = (src / "finddlg2.cpp").read_text(encoding="utf-8")
    menu4 = (src / "menu4.cpp").read_text(encoding="utf-8")
    resource = (src / "resource.rh2").read_text(encoding="utf-8")
    texts = (src / "texts.rh2").read_text(encoding="utf-8")
    texts_rc = (src / "lang/texts.rc2").read_text(encoding="utf-8")
    cfgdlg = (src / "cfgdlg.h").read_text(encoding="utf-8")
    dialogs4 = (src / "dialogs4.cpp").read_text(encoding="utf-8")
    mainwnd2 = (src / "mainwnd2.cpp").read_text(encoding="utf-8")
    consts = (src / "consts.h").read_text(encoding="utf-8")
    salamdr1 = (src / "salamdr1.cpp").read_text(encoding="utf-8")

    require(find_h, "int SortBy;", "Find listview sort column")
    require(find_h, "BOOL ReverseSort;", "Find reverse-sort flag")
    require(find_h, "void SortItems(int sortBy, BOOL reverse = TRUE, BOOL force = FALSE);",
            "Find SortItems toggle/force signature")

    sort_items = function_slice(
        finddlg1,
        "void CFoundFilesListView::SortItems(int sortBy, BOOL reverse, BOOL force)",
        "void CFoundFilesListView::SetDifferentByGroup()")
    require(sort_items, "if (sortBy == 4)", "Find Date and Time share one key")
    require(sort_items, "sortBy = 3;", "Find Time column maps to Date")
    require(sort_items, "if (sortBy == 5 || sortBy < 0)", "Find Attr column stays unsortable")
    require(sort_items, "ReverseSort = !ReverseSort;", "second SortItems call on the same column toggles reverse")
    require(sort_items, "ReverseSort = force ? reverse : FALSE;", "new column starts ascending unless forced")
    require(finddlg2, "FoundFilesListView->SortItems(1, FALSE, TRUE);",
            "Hide Duplicates forces Path sort without toggling reverse")

    compare = function_slice(
        finddlg1,
        "int CFoundFilesListView::CompareFunc(CFoundFilesData* f1, CFoundFilesData* f2, int sortBy)",
        "int CFoundFilesListView::CompareDuplicatesFunc(")
    require(compare, "const BOOL mixed = Configuration.FindSortFilesAndDirsTogether;",
            "Find CompareFunc reads the mixed grouping option")
    require(compare, "if (mixed || f1->IsDir == f2->IsDir)",
            "Find mixed grouping skips directory/file separation")
    require(compare, "res = f1->IsDir ? -1 : 1;",
            "Find keeps directories above files when mixed grouping is off")
    require(compare, "if (ReverseSort && res != 0)",
            "Find reverse applies to the sort criterion")
    if compare.find("if (mixed || f1->IsDir == f2->IsDir)") > compare.find("res = f1->IsDir ? -1 : 1;"):
        raise AssertionError("Find must not reverse directory/file grouping when mixed is off")

    header = function_slice(
        finddlg1,
        "void PaintFindHeaderSortArrow(HDC hdc, const RECT* itemRect, const char* text, UINT format, BOOL reverse)",
        "CFoundFilesListView::CFoundFilesListView(HWND dlg, int ctrlID, CFindDialog* findDialog)")
    require(header, "HHeaderSort", "Find header paints the panel sort bitmap")
    require(header, "PaintFindHeaderSortArrow", "Find header overlays hdrwnd.bmp arrows")
    require_absent(header, "HDF_SORTUP", "Find header must not use HDF_SORTUP as the sort indicator")
    require(header, "CDDS_ITEMPOSTPAINT", "Find light-mode header overlays the arrow after native chrome")
    require(finddlg1, "UpdateListViewSortHeaderOverlay", "Find dark-mode header overlay uses hdrwnd.bmp")
    require(salamdr1, "IDB_HEADER", "Find bitmap comes from the shared header resource")

    require(resource, "#define CM_FIND_SORTMIXED        2295", "Find Options command id")
    require(texts, "#define IDS_FFMENU_OPT_SORTMIXED        13348", "Find Options string id")
    require(texts_rc, 'IDS_FFMENU_OPT_SORTMIXED,  "Sort Files and Directories &Together"',
            "Find Options English string")
    require(menu4, "CM_FIND_SORTMIXED", "Find Options menu contains the mixed grouping item")
    require_after(menu4, "CM_FIND_FULLROWSEL", "CM_FIND_SORTMIXED",
                  "Find mixed grouping next to Full Row Select")
    require(finddlg1, "popup->CheckItem(CM_FIND_SORTMIXED, FALSE, Configuration.FindSortFilesAndDirsTogether);",
            "Find Options menu checks the mixed grouping item")
    require(finddlg1, "popup->CheckItem(CM_FIND_NAME, FALSE, sortBy == 0);",
            "Find View menu checks the active sort criterion")
    require(cfgdlg, "FindSortFilesAndDirsTogether", "Find mixed grouping is stored in Configuration")
    require(dialogs4, "FindSortFilesAndDirsTogether = FALSE;", "Find mixed grouping defaults to off")
    require(mainwnd2, 'CONFIG_FINDSORTMIXED_REG = "Sort Files And Dirs Together In Find"',
            "Find mixed grouping registry value")
    require(consts, "WM_USER_FINDSORTMIXED", "Find mixed grouping broadcast")
    require(finddlg1, "FindDialogQueue.BroadcastMessage(WM_USER_FINDSORTMIXED, 0, 0);",
            "changing the Find mixed option notifies other Find windows")


def check_renamer(src: Path) -> None:
    rendlg_h = (src / "plugins/renamer/rendlg.h").read_text(encoding="utf-8")
    rendlg = (src / "plugins/renamer/rendlg.cpp").read_text(encoding="utf-8")
    preview = (src / "plugins/renamer/preview.cpp").read_text(encoding="utf-8")
    renamer = (src / "plugins/renamer/renamer.cpp").read_text(encoding="utf-8")
    rh2 = (src / "plugins/renamer/renamer.rh2").read_text(encoding="utf-8")
    rc2 = (src / "plugins/renamer/RENAMER.RC2").read_text(encoding="utf-8")
    lang = (src / "plugins/renamer/lang/lang.rc2").read_text(encoding="utf-8")

    require(rendlg_h, "BOOL ReverseSort;", "Renamer reverse-sort flag")
    require(rendlg, "ReverseSort = !ReverseSort;", "Renamer toggles reverse on the same column")
    require(rendlg, "case CMD_SORTMIXED:", "Renamer Options command for mixed grouping")
    require(rh2, "#define IDS_MENU_SORTMIXED 1287", "Renamer mixed grouping string id")
    require(rh2, "#define CMD_SORTMIXED 628", "Renamer mixed grouping command id")
    require(lang, 'IDS_MENU_SORTMIXED, "Sort Files and Directories &Together"',
            "Renamer Options English string")
    require(rc2, "hdrwnd.bmp", "Renamer header bitmap is hdrwnd.bmp")
    require_absent(preview, "HDF_SORTUP", "Renamer must not use HDF_SORTUP as the sort indicator")
    require_absent(rendlg, "HDF_SORTUP", "Renamer dialog must not use HDF_SORTUP as the sort indicator")

    compare = function_slice(
        preview,
        "int CPreviewWindow::CompareFunc(CSourceFile* f1, CSourceFile* f2, int sortBy)",
        "void CPreviewWindow::QuickSort(")
    require(compare, "const BOOL mixed = SortFilesAndDirsTogether;",
            "Renamer CompareFunc reads the mixed grouping option")
    require(compare, "res = f1->IsDir ? -1 : 1;",
            "Renamer keeps directories above files when mixed grouping is off")
    require(compare, "RenamerDialog->ReverseSort", "Renamer reverse applies inside groups")

    require(preview, "case CI_DATE:", "Renamer Date shares the Time sort key")
    require(preview, "case CI_TIME:", "Renamer Time shares the Date sort key")
    require(preview, "next = 3;", "Renamer Date and Time use one compare key")
    require(preview, "HandleListViewHeaderSortCustomDraw", "Renamer header uses the shared hdrwnd overlay")
    require(preview, "UpdateListViewSortHeaderOverlay", "Renamer dark header overlay uses hdrwnd.bmp")
    require(renamer, 'CONFIG_SORTMIXED = "Sort Files And Dirs Together"',
            "Renamer mixed grouping is persisted")
    require(renamer, "SortFilesAndDirsTogether = FALSE;",
            "Renamer mixed grouping defaults to off")
    require(rendlg, "subMenu->CheckItem(CMD_SORTMIXED, FALSE, SortFilesAndDirsTogether);",
            "Renamer Options menu checks mixed grouping")
    get_sort = function_slice(
        preview,
        "int CPreviewWindow::GetSortCommand(int column)",
        "void CPreviewWindow::SetItemCount(")
    require(get_sort, "//case CI_NEWNAME:", "Renamer New Name column stays unsortable")


def check_regedt(src: Path) -> None:
    finddlg_h = (src / "plugins/regedt/finddlg.h").read_text(encoding="utf-8")
    finddlg = (src / "plugins/regedt/finddlg.cpp").read_text(encoding="utf-8")
    finddlg2 = (src / "plugins/regedt/finddlg2.cpp").read_text(encoding="utf-8")
    regedt = (src / "plugins/regedt/regedt.cpp").read_text(encoding="utf-8")
    lang_rh = (src / "plugins/regedt/lang/lang.rh").read_text(encoding="utf-8")
    lang_rc = (src / "plugins/regedt/lang/lang.rc").read_text(encoding="utf-8")
    rc2 = (src / "plugins/regedt/regedt.rc2").read_text(encoding="utf-8")

    require(finddlg_h, "BOOL ReverseSort;", "RegEdt reverse-sort flag")
    require(finddlg_h, "void SortItems(int sortBy, BOOL reverse = TRUE, BOOL force = FALSE);",
            "RegEdt SortItems toggle/force signature")
    require(finddlg_h, "BOOL SortKeysAndValuesTogether;", "RegEdt mixed grouping flag")

    sort_items = function_slice(
        finddlg,
        "void CFoundFilesListView::SortItems(int sortBy, BOOL reverse, BOOL force)",
        "CFoundFilesListView::At(int index)")
    require(sort_items, "if (sortBy == CI_DATE)", "RegEdt Date and Time share one key")
    require(sort_items, "sortBy = CI_TIME;", "RegEdt Date column maps to Time")
    require(sort_items, "if (sortBy == CI_DATA || sortBy < 0)", "RegEdt Data column stays unsortable")
    require(sort_items, "ReverseSort = !ReverseSort;", "second RegEdt SortItems call toggles reverse")

    compare = function_slice(
        finddlg,
        "int CFoundFilesListView::CompareFunc(CFoundFilesData* f1, CFoundFilesData* f2, int sortBy)",
        "void CFoundFilesListView::QuickSort(")
    require(compare, "SearchDialog->SortKeysAndValuesTogether",
            "RegEdt CompareFunc reads the mixed grouping option")
    require(compare, "res = f1->IsDir ? -1 : 1;",
            "RegEdt keeps keys above values when mixed grouping is off")
    require(compare, "if (ReverseSort && res != 0", "RegEdt reverse applies to the sort criterion")

    require(finddlg, "HandleListViewHeaderSortCustomDraw", "RegEdt header uses the shared hdrwnd overlay")
    require(finddlg, "UpdateListViewSortHeaderOverlay", "RegEdt dark header overlay uses hdrwnd.bmp")
    require_absent(finddlg, "HDF_SORTUP", "RegEdt must not use HDF_SORTUP as the sort indicator")
    require_absent(finddlg2, "HDF_SORTUP", "RegEdt dialog must not use HDF_SORTUP as the sort indicator")
    require(rc2, "hdrwnd.bmp", "RegEdt header bitmap is hdrwnd.bmp")
    require(lang_rh, "#define IDC_SORTMIXED                   178", "RegEdt mixed grouping control id")
    require(lang_rc, "Sort keys and values together", "RegEdt mixed grouping checkbox")
    require(finddlg2, "ti.CheckBox(IDC_SORTMIXED, SortKeysAndValuesTogether);",
            "RegEdt checkbox transfers mixed grouping")
    require(finddlg2, "case IDC_SORTMIXED:", "RegEdt checkbox applies mixed grouping immediately")
    require(regedt, 'CONFIG_SORTMIXED = "Sort Keys And Values Together"',
            "RegEdt mixed grouping is persisted")
    require(regedt, "SortKeysAndValuesTogether = FALSE;",
            "RegEdt mixed grouping defaults to off")
    require(finddlg2, "GetDlgItem(HWindow, IDC_SORTMIXED)",
            "RegEdt checkbox is laid out with Found Items")
    require_absent(
        function_slice(finddlg2, "void CFindDialog::LayoutControls(BOOL showOrHideControls)",
                       "void CFindDialog::EnableControls(BOOL enable)"),
        "ShowWindow(GetDlgItem(HWindow, IDC_SORTMIXED)",
        "RegEdt mixed grouping checkbox stays visible when search options are collapsed")


def check_shared_and_help(src: Path, root: Path) -> None:
    helper = (src / "plugins/shared/listview_sort_header.cpp").read_text(encoding="utf-8")
    helper_h = (src / "plugins/shared/listview_sort_header.h").read_text(encoding="utf-8")
    require(helper, "CreateMappedBitmap", "shared helper loads hdrwnd.bmp through CreateMappedBitmap")
    require(helper, "PaintListViewHeaderSortArrow", "shared helper paints the hdrwnd.bmp arrow")
    require(helper, "UpdateListViewSortHeaderOverlay", "shared helper overlays the arrow in dark mode")
    require_absent(helper, "HDF_SORTUP", "shared helper must not use HDF_SORTUP")
    require(helper_h, "ListViewHeaderColumnShowsSort", "Date and Time share a header arrow")

    find_help = (root / "help/src/hh/salamand/finddlg_main.htm").read_text(encoding="utf-8")
    basic_help = (root / "help/src/hh/salamand/basicwork_find.htm").read_text(encoding="utf-8")
    news = (root / "help/src/hh/salamand/introduction_news.htm").read_text(encoding="utf-8")
    renamer_help = (src / "plugins/renamer/help/hh/renamer/dlgboxes_batchrename.htm").read_text(encoding="utf-8")
    regedt_help = (src / "plugins/regedt/help/hh/regedt/dlgboxes_search.htm").read_text(encoding="utf-8")
    require(find_help, "Sort Files and Directories Together", "Find help documents mixed grouping")
    require(basic_help, "reverse the sort direction", "Find basic help documents reverse sort")
    require(news, "sorted in both", "What's New mentions bidirectional listview sorting")
    require(renamer_help, "Sort Files and Directories Together", "Renamer help documents mixed grouping")
    require(regedt_help, "Sort keys and values together", "RegEdt help documents mixed grouping")

    czech = (root / "translations/czech/salamand.slt").read_text(encoding="utf-8")
    require(czech, "13348,1,", "Czech Find translation for mixed grouping")
    czech_renamer = (root / "translations/czech/renamer.slt").read_text(encoding="utf-8")
    require(czech_renamer, "1287,1,", "Czech Renamer translation for mixed grouping")
    czech_regedt = (root / "translations/czech/regedt.slt").read_text(encoding="utf-8")
    require(czech_regedt, "178,80,131,160,12,1,", "Czech RegEdt translation for mixed grouping")


def main() -> None:
    src = ROOT / "src"
    check_find(src)
    check_renamer(src)
    check_regedt(src)
    check_shared_and_help(src, ROOT)
    print("find_sort_contract_tests: ok")


if __name__ == "__main__":
    main()
