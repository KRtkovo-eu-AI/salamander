# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def require(text: str, needle: str, description: str) -> None:
    if needle not in text:
        raise AssertionError(f"Missing {description}: {needle}")


def main() -> None:
    mainwnd3 = (ROOT / "src/mainwnd3.cpp").read_text(encoding="utf-8")

    fast_start = mainwnd3.index("CMainWindow::WindowProc(UINT uMsg")
    impl_start = mainwnd3.index("CMainWindow::WindowProcImpl(UINT uMsg")
    fast_path = mainwnd3[fast_start:impl_start]
    implementation = mainwnd3[impl_start:]

    for message in (
        "case WM_GETMINMAXINFO:",
        "case WM_NCHITTEST:",
        "case WM_NCMOUSEMOVE:",
        "case WM_NCPAINT:",
        "case WM_SYNCPAINT:",
        "case WM_MOVE:",
        "case WM_MOVING:",
        "case WM_WINDOWPOSCHANGING:",
        "case WM_WINDOWPOSCHANGED:",
        "case WM_SETCURSOR:",
        "case WM_ERASEBKGND:",
        "case WM_PAINT:",
    ):
        require(fast_path, message, "interactive main-window move fast path")
        if message in implementation:
            raise AssertionError(
                f"Heavy WindowProcImpl must not handle drag message: {message}"
            )

    require(
        fast_path,
        "return CWindow::WindowProc(uMsg, wParam, lParam);",
        "default Windows processing for drag messages",
    )
    require(
        fast_path,
        "return WindowProcImpl(uMsg, wParam, lParam);",
        "non-drag dispatch to the complete message handler",
    )
    require(
        fast_path,
        "PostMessage(HWindow, WM_SIZE, SIZE_RESTORED",
        "deferred size recovery retained on the fast path",
    )

    print("Main-window move contract tests passed.")


if __name__ == "__main__":
    main()
