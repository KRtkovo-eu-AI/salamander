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
        and "IDC_EXCOL_SEARCH" in dialog,
        "the property selector must retain categories, Common, and live search",
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
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(error, file=sys.stderr)
        raise SystemExit(1)
