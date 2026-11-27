# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-27 | Verify Internal Viewer configuration navigation. | Keep the viewer's command linked to its moved configuration page. | https://github.com/KRtkovo-eu-AI/salamander/issues/861 | openrouter/openai/gpt-6-luna-pro |

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    config_header = (ROOT / "src" / "cfgdlg.h").read_text(encoding="utf-8-sig")
    config_dialog = (ROOT / "src" / "dialogs4.cpp").read_text(encoding="utf-8-sig")
    main_window = (ROOT / "src" / "mainwnd3.cpp").read_text(encoding="utf-8-sig")

    if "CCFG_MODE_INTERNAL_VIEWER = 3" not in config_header:
        raise AssertionError("the viewer configuration entry point must have a named mode")
    if "CCFG_PAGE_INTERNAL_VIEWER = 9" not in config_header:
        raise AssertionError("the viewer configuration mode must target the Internal Viewer page")
    if "mode == CCFG_MODE_INTERNAL_VIEWER ? CCFG_PAGE_INTERNAL_VIEWER" not in config_dialog:
        raise AssertionError("the named viewer mode must open the Internal Viewer page")
    if "/*09*/ Add(&PageViewer" not in config_dialog:
        raise AssertionError("the selected page index must continue to identify PageViewer")

    start = main_window.index("case WM_USER_VIEWERCONFIG:")
    end = main_window.index("case WM_USER_CONFIGURATION:", start)
    if "OnConfiguration(CCFG_MODE_INTERNAL_VIEWER, 0);" not in main_window[start:end]:
        raise AssertionError("the Internal Viewer command must use the named configuration mode")
    if "OnConfiguration(3, 0);" in main_window[start:end]:
        raise AssertionError("the viewer command must not retain the stale numeric page link")

    print("Internal Viewer configuration navigation tests passed.")


if __name__ == "__main__":
    main()
