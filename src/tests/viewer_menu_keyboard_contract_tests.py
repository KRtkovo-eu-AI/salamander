# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-28 | Cover Internal Viewer menu keyboard dispatch. | Alt mnemonics and F10 need to reach the custom menu bar. | https://github.com/KRtkovo-eu-AI/salamander/issues/862 | openrouter/openai/gpt-6-luna-pro |

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, description: str) -> None:
    if not condition:
        raise AssertionError(description)


def main() -> None:
    viewer = (ROOT / "viewer2.cpp").read_text(encoding="utf-8")
    menubar = (ROOT / "menubar.cpp").read_text(encoding="utf-8")

    message_loop_start = viewer.find("while (GetMessage(&msg, NULL, 0, 0))")
    require(message_loop_start >= 0, "Internal Viewer message loop was not found")
    message_loop = viewer[message_loop_start:]
    menu_dispatch = message_loop.find("view->IsMenuBarMessage(&msg)")
    accelerator_dispatch = message_loop.find("TranslateAccelerator(viewHWindow, ViewerTable, &msg)")
    require(
        menu_dispatch >= 0 and accelerator_dispatch >= 0 and menu_dispatch < accelerator_dispatch,
        "viewer messages must pass through CMenuBar before accelerator translation",
    )
    require('#include "menu.h"' in viewer, "viewer2.cpp must include the complete CMenuBar declaration")
    require("ViewerMenuBar->IsMenuBarMessage(lpMsg)" in viewer,
            "CViewerWindow must forward keyboard messages to its custom menu bar")
    require("if (wParam == VK_F10)" in menubar, "the shared menu bar must handle F10")
    require("case WM_SYSCHAR:" in menubar, "the shared menu bar must handle Alt mnemonics")
    require("EnterMenuInternal(index, TRUE, FALSE);" in menubar,
            "an Alt mnemonic must open the matching top-level menu")

    print("Internal Viewer menu keyboard contract tests passed.")


if __name__ == "__main__":
    main()
