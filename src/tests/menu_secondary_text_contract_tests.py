# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    header = (ROOT / "menu.h").read_text(encoding="utf-8")
    sdk = (ROOT / "plugins/shared/spl_gui.h").read_text(encoding="utf-8")
    model = (ROOT / "menu2.cpp").read_text(encoding="utf-8")
    paint = (ROOT / "menu3.cpp").read_text(encoding="utf-8")
    flag = "MENU_STATE_RIGHTTEXT_GRAY"
    match = re.search(r"#define MENU_STATE_RIGHTTEXT_GRAY\s+(0x[0-9a-fA-F]+)", header)
    require(match is not None, "secondary-column style must be an internal state flag")
    value = int(match.group(1), 16)
    require(value != 0 and value & (value - 1) == 0, "style must use one state bit")
    for public_value in re.findall(r"#define MENU_STATE_\w+\s+(0x[0-9a-fA-F]+)", sdk):
        require(not value & int(public_value, 16), "style must not disable, check, or bold the item")
    require(flag not in sdk, "internal presentation must not extend the plugin SDK contract")

    draw = paint[paint.index("void CMenuPopup::DrawItem("):]
    right_start = draw.index("if (item->ColumnR != NULL)")
    restore = draw.index("// restore the original values", right_start)
    right = draw[right_start:restore]
    require(paint.count(flag) == 1 and flag in right,
            "only the right column may receive the secondary style; labels and icons must not change")
    require(right.index("MENU_STATE_GRAYED") < right.index(flag),
            "disabled-item rendering must take precedence over secondary styling")
    require("COLORREF rightTextColor = SharedRes->GrayTextColor;" in right,
            "unselected secondary text must follow the light/dark disabled-text palette")
    require("if (selected)" in right and "SharedRes->SelectedTextColor" in right
            and "SharedRes->SelectedBkColor" in right,
            "selected secondary text must follow the user's selection colors")
    require("SetTextColor(hDC, oldTextColor);" in draw[restore:],
            "secondary text must not leak its color into subsequent drawing")

    enable = model[model.index("CMenuPopup::EnableItem("):model.index("CMenuPopup::GetSubMenu(")]
    require("DWORD newState = item->State & ~MENU_STATE_GRAYED;" in enable,
            "enabling/disabling a command must preserve its secondary-column style")
    require("item->State = mii->State;" in model and "mii->State = item->State;" in model,
            "internal menu state must round-trip the secondary-column style")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
