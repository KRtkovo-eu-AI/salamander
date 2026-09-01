# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def require(text: str, needle: str, description: str) -> None:
    if needle not in text:
        raise AssertionError(f"Missing {description}: {needle}")


def main() -> None:
    src = ROOT / "src"
    mainwnd_h = (src / "mainwnd.h").read_text(encoding="utf-8")
    mainwnd1 = (src / "mainwnd1.cpp").read_text(encoding="utf-8")
    mainwnd3 = (src / "mainwnd3.cpp").read_text(encoding="utf-8")

    require(mainwnd_h, "BOOL SettingChangeInProgress;", "per-window setting-change guard")
    require(mainwnd1, "SettingChangeInProgress = FALSE;", "guard initialization")

    handler_start = mainwnd3.index("case WM_SETTINGCHANGE:")
    handler_end = mainwnd3.index("case WM_USER_SHCHANGENOTIFY:", handler_start)
    handler = mainwnd3[handler_start:handler_end]
    require(handler, "if (SettingChangeInProgress)", "nested setting-change rejection")
    require(handler, "CSettingChangeGuard", "scope-bound guard reset")
    require(handler, "settingChangeGuard(SettingChangeInProgress)", "guard activation before refresh")
    require(handler, "SetEnvFont();", "guarded environment-font refresh")

    if handler.index("if (SettingChangeInProgress)") > handler.index("SetEnvFont();"):
        raise AssertionError("Setting-change guard must precede SetEnvFont")

    print("main_window_setting_change_contract_tests: ok")


if __name__ == "__main__":
    main()
