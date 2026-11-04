# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-04 | Lock the shared panel and middle toolbar catalogs. | Panel customize must keep offering every middle-toolbar command. | https://github.com/KRtkovo-eu-AI/salamander/issues/777 | Grok 4.7 |

"""Panel toolbar customize offers the same commands as the middle toolbar."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]

# Buttons the panel toolbars already offered before the catalogs were unified.
ORIGINAL_PANEL_BUTTONS = (
    "TBBE_PARENT_DIR",
    "TBBE_ROOT_DIR",
    "TBBE_CHANGEDIR",
    "TBBE_AS_OTHER_PANEL",
    "TBBE_BACK",
    "TBBE_FORWARD",
    "TBBE_VIEW_MODE",
    "TBBE_SORT_NAME",
    "TBBE_SORT_EXT",
    "TBBE_SORT_SIZE",
    "TBBE_SORT_DATE",
    "TBBE_FILTER",
    "TBBE_REFRESH",
    "TBBE_SMART_COLUMN_MODE",
)


def fail(message: str) -> None:
    raise AssertionError(message)


def split_fields(body: str) -> list[str]:
    fields: list[str] = []
    current: list[str] = []
    depth = 0
    in_string = False
    for ch in body:
        if in_string:
            current.append(ch)
            if ch == '"':
                in_string = False
            continue
        if ch == '"':
            in_string = True
            current.append(ch)
        elif ch == "(":
            depth += 1
            current.append(ch)
        elif ch == ")":
            depth -= 1
            current.append(ch)
        elif ch == "," and depth == 0:
            fields.append("".join(current).strip())
            current = []
        else:
            current.append(ch)
    tail = "".join(current).strip()
    if tail:
        fields.append(tail)
    return fields


def function_body(source: str, signature: str, next_signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        fail(f"missing function {signature}")
    end = source.find(next_signature, start + len(signature))
    if end < 0:
        fail(f"missing function after {signature}")
    return source[start:end]


def main() -> None:
    toolbar4 = (ROOT / "toolbar4.cpp").read_text(encoding="utf-8")
    toolbar_h = (ROOT / "toolbar.h").read_text(encoding="utf-8")
    cfgdlg = (ROOT / "cfgdlg.h").read_text(encoding="utf-8")
    dialogs4 = (ROOT / "dialogs4.cpp").read_text(encoding="utf-8")
    mainwnd1 = (ROOT / "mainwnd1.cpp").read_text(encoding="utf-8")
    mainwnd2 = (ROOT / "mainwnd2.cpp").read_text(encoding="utf-8")
    mainwnd3 = (ROOT / "mainwnd3.cpp").read_text(encoding="utf-8")

    defines = {
        name: int(value)
        for name, value in re.findall(r"(?m)^#define (TBBE_[A-Z0-9_]+) +(\d+)", toolbar4)
    }
    if defines.get("TBBE_CHANGE_DRIVE_L") != 36 or defines.get("TBBE_CHANGE_DRIVE_R") != 51:
        fail("Change Drive button indexes drifted from the persisted panel defaults")
    if "TBBE_TERMINATOR" not in defines:
        fail("toolbar button terminator is missing")

    table_start = toolbar4.index("CButtonData ToolBarButtons[TBBE_TERMINATOR]")
    table_end = toolbar4.index("DWORD TopToolBarButtons[]", table_start)
    buttons = []
    for name, body in re.findall(r"/\*(TBBE_[A-Z0-9_]+)\*/\s*\{(.*?)\}", toolbar4[table_start:table_end], re.S):
        fields = split_fields(body)
        if len(fields) != 13:
            fail(f"{name} does not have the 13 toolbar-button fields: {fields}")
        buttons.append(
            {
                "name": name,
                "image": fields[0],
                "id": fields[3],
                "left": fields[4],
                "right": fields[5],
            }
        )
    if len(buttons) != defines["TBBE_TERMINATOR"]:
        fail("toolbar button table no longer matches TBBE_TERMINATOR")
    for index, button in enumerate(buttons):
        # Retired slots stay in the table so older layouts keep their indexes.
        # Their defines are commented out; every live define must still match.
        if button["name"] in defines and defines[button["name"]] != index:
            fail(f"{button['name']} is no longer persisted at index {index}")
    by_name = {button["name"]: button for button in buttons}

    catalog_start = toolbar4.index("DWORD TopToolBarButtons[]")
    catalog_end = toolbar4.index("void GetSVGIconsMainToolbar", catalog_start)
    catalog_text = re.sub(r"//.*?$", "", toolbar4[catalog_start:catalog_end], flags=re.M)
    catalog_text = re.sub(r"NIB2\((TBBE_[A-Z0-9_]+)\)", r"\1", catalog_text)
    catalog = re.findall(r"TBBE_[A-Z0-9_]+", catalog_text)
    if not catalog or catalog[-1] != "TBBE_TERMINATOR":
        fail("shared toolbar catalog lost its terminator")
    catalog = catalog[:-1]
    if len(catalog) < 80:
        fail("shared toolbar catalog shrank below the middle-toolbar set")
    if len(catalog) != len(set(catalog)):
        fail("shared toolbar catalog contains a duplicate button")
    if catalog[:3] != ["TBBE_BACK", "TBBE_FORWARD", "TBBE_HOTPATHSDROP"]:
        fail("Hot Paths is no longer offered beside Back and Forward")
    for name in ORIGINAL_PANEL_BUTTONS:
        if name not in catalog:
            fail(f"panel toolbar lost its original button {name}")
    if "TBBE_CHANGE_DRIVE_L" in catalog or "TBBE_CHANGE_DRIVE_R" in catalog:
        fail("Change Drive must stay a per-panel prefix, not a duplicated shared button")
    if "DWORD LeftToolBarButtons" in toolbar4 or "DWORD RightToolBarButtons" in toolbar4:
        fail("panel toolbars still have a separate shorter customize catalog")

    def resolve(button: dict[str, str], side: str) -> str:
        if side == "middle":
            return button["id"]
        side_id = button["left"] if side == "left" else button["right"]
        return button["id"] if side_id == "0" else side_id

    def assert_unique(names: list[str], side: str) -> None:
        resolved = []
        for name in names:
            button = by_name[name]
            if "0xFFFF" in button["image"]:
                fail(f"{name} is retired and would stop or blank the customize list")
            command = resolve(button, side)
            if command == "0":
                fail(f"{name} has no {side} command and would be inserted disabled")
            resolved.append(command)
        if len(resolved) != len(set(resolved)):
            duplicates = sorted({command for command in resolved if resolved.count(command) > 1})
            fail(f"{side} customize commands collide: {', '.join(duplicates)}")

    assert_unique(["TBBE_CHANGE_DRIVE_L"] + catalog, "left")
    assert_unique(["TBBE_CHANGE_DRIVE_R"] + catalog, "right")
    assert_unique(catalog, "middle")

    hot_paths = by_name["TBBE_HOTPATHSDROP"]
    if hot_paths["id"] != "CM_OPENHOTPATHSDROP" or hot_paths["left"] != "0" or hot_paths["right"] != "0":
        fail("Hot Paths must keep the shared dropdown command so panel routing can select the menu range")

    enum_body = function_body(toolbar4, "CMainToolBar::OnEnumButton", "CMainToolBar::OnReset")
    for needle in (
        "TopToolBarButtons",
        "TBBE_CHANGE_DRIVE_L",
        "TBBE_CHANGE_DRIVE_R",
        "index--",
    ):
        if needle not in enum_body:
            fail(f"panel customize enumeration no longer shares the middle catalog ({needle})")
    if "LeftToolBarButtons" in enum_body or "RightToolBarButtons" in enum_body:
        fail("panel customize enumeration still reads a private button list")

    fill_body = function_body(toolbar4, "CMainToolBar::FillTII", "CMainToolBar::Load")
    separator_at = fill_body.find("tbbeIndex == -1")
    image_at = fill_body.find("ToolBarButtons[tbbeIndex]")
    if separator_at < 0 or image_at < 0 or separator_at > image_at:
        fail("separator loading indexes the button table before recognizing index -1")
    fallback = fill_body.find("if (id == 0)")
    if fallback < 0 or "ToolBarButtons[tbbeIndex].ID" not in fill_body[fallback:]:
        fail("panel buttons without a side command no longer fall back to the middle-toolbar command")
    if "ToolBarButtons[tbbeIndex].Enabler" not in fill_body[fallback:]:
        fail("shared panel buttons no longer use the middle-toolbar enabler")

    hot_path_body = function_body(mainwnd3, "HotPathMenuCommandForToolBar", "CMainWindow::WindowProcImpl")
    for needle in (
        "CM_LEFTHOTPATH_MIN",
        "CM_RIGHTHOTPATH_MIN",
        "CM_ACTIVEHOTPATH_MIN",
        "LeftPanelTabs",
        "RightPanelTabs",
        "GetDetachedTabCount",
    ):
        if needle not in hot_path_body:
            fail(f"panel Hot Paths routing lost {needle}")
    dropdown = function_body(mainwnd3, "case CM_OPENHOTPATHSDROP:", "case CM_USERMENUDROP:")
    if "HotPathMenuCommandForToolBar(this, tlb)" not in dropdown:
        fail("Hot Paths dropdown no longer asks which panel owns the toolbar")

    for name in ("TopToolBar", "MiddleToolBar", "LeftToolBar", "RightToolBar"):
        if f"char {name}[1024];" not in cfgdlg:
            fail(f"{name} can no longer store a full shared toolbar layout")
        if f"sizeof(Configuration.{name})" not in mainwnd2:
            fail(f"{name} is not loaded with its real buffer size")
    if "LeftToolBar[200]" in cfgdlg or "RightToolBar[200]" in cfgdlg:
        fail("panel toolbar persistence is still capped at the old subset")
    for name in ("TopToolBar", "MiddleToolBar"):
        if f"Save(Configuration.{name}, sizeof(Configuration.{name}))" not in mainwnd1:
            fail(f"{name} save does not pass the buffer capacity")
    for name in ("TopToolBar", "MiddleToolBar", "LeftToolBar", "RightToolBar"):
        if f"Save(Configuration.{name}, sizeof(Configuration.{name}))" not in mainwnd3:
            fail(f"{name} save does not pass the buffer capacity")
    if "BOOL Save(char* data, int dataSize);" not in toolbar_h:
        fail("toolbar Save no longer receives the destination capacity")
    if "leftToolbar->CheckItem(CM_HELP_CONTEXT" not in mainwnd1 or "rightToolbar->CheckItem(CM_HELP_CONTEXT" not in mainwnd1:
        fail("panel toolbars do not reflect the shared context-help button state")

    if 'const char* DefLeftToolBar = "36";' not in dialogs4 or 'const char* DefRightToolBar = "51";' not in dialogs4:
        fail("default panel toolbars no longer start from Change Drive")

    indexes = [defines["TBBE_CHANGE_DRIVE_L"]] + [defines[name] for name in catalog]
    layout = ",".join(str(index) for index in indexes)
    layout += ",-1" * 40
    if len(layout) <= 200:
        fail("the enlarged panel-toolbar buffer is no longer required by the shared catalog")
    if len(layout) >= 1024:
        fail("a full panel toolbar plus separators does not fit in the persisted buffer")

    print("Panel toolbar customize source-contract tests passed.")


if __name__ == "__main__":
    main()
