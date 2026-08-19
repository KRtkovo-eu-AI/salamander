# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    menu = (ROOT / "menu3.cpp").read_text(encoding="utf-8")
    function_start = menu.index("void CMenuPopup::DrawCheckImage")
    disabled_start = menu.index("if (item->State & MENU_STATE_GRAYED)", function_start)
    enabled_start = menu.index("\n    else\n    {", disabled_start)
    disabled = menu[disabled_start:enabled_start]

    require(
        "if (DarkModeShouldUseDarkColors())" in disabled
        and "HImageList" in disabled
        and "ILD_NORMAL | ILD_SCALE" in disabled,
        "dark disabled menu commands must use the toolbar grayscale image list",
    )
    require(
        "DST_ICON | DSS_MONO" in disabled
        and "SharedRes->GrayTextColor" in disabled,
        "dynamic disabled menu icons and overlays must use one flat gray pass",
    )
    require(
        "StretchBlt" in disabled
        and "HMenuHilightBrush" in disabled
        and "else\n        {" in disabled,
        "the legacy embossed disabled icon path must remain available for light mode",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
