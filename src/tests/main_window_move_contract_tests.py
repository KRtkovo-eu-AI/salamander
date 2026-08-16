# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def require(text: str, needle: str, description: str) -> None:
    if needle not in text:
        raise AssertionError(f"Missing {description}: {needle}")


def main() -> None:
    mainwnd3 = (ROOT / "src/mainwnd3.cpp").read_text(encoding="utf-8")
    mainwnd1 = (ROOT / "src/mainwnd1.cpp").read_text(encoding="utf-8")
    winlib = (ROOT / "src/common/winlib.cpp").read_text(encoding="utf-8")

    for profiler_marker in (
        "CMoveMessageProfile",
        "BeginMoveMessageProfile",
        "RecordMoveMessageProfile",
        "EndMoveMessageProfile",
        ".move-profile.txt",
    ):
        if profiler_marker in winlib:
            raise AssertionError(
                f"Temporary main-window move profiler remains: {profiler_marker}"
            )

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
    if "EnableNonClientDpiScaling" in mainwnd3:
        raise AssertionError("PMv2 main window must not enable legacy non-client DPI scaling")
    if "SetDWMTransitionsForInteractiveMove" in mainwnd3:
        raise AssertionError("main-window dragging must not toggle unrelated DWM transitions")
    require(
        fast_path,
        "FlushDWMForInteractiveMove();",
        "compositor synchronization for position-only drag",
    )
    require(
        fast_path,
        "(windowPos->flags & SWP_NOSIZE) != 0",
        "DWM synchronization limited to position-only changes",
    )

    dpi_changed_start = implementation.index("case WM_DPICHANGED:")
    apply_dpi_start = implementation.index("case WM_USER_APPLY_DPI_CHANGE:")
    dpi_changed = implementation[dpi_changed_start:apply_dpi_start]
    require(
        dpi_changed,
        "RefreshDPI(TRUE, dpi, suggestedRect);",
        "synchronous PMv2 resource and geometry update in WM_DPICHANGED",
    )
    if "DPIRefreshDeferredForSizeMove" in mainwnd3:
        raise AssertionError("Microsoft-style candidate must not defer WM_DPICHANGED")
    if "ShouldDeferMainWindowChildDPIRefresh" in mainwnd3 or \
       "ShouldDeferMainWindowChildDPIRefresh" in winlib:
        raise AssertionError("PMv2 child DPI notifications must run immediately")
    require(
        mainwnd1,
        "LeftTabWindow->RefreshDPIResources();",
        "left tab resources rebuilt by the complete main-window DPI refresh",
    )
    require(
        mainwnd1,
        "RightTabWindow->RefreshDPIResources();",
        "attached right tab resources rebuilt by the complete main-window DPI refresh",
    )
    require(
        implementation,
        "case WM_POWERBROADCAST:",
        "power suspend and resume handling",
    )
    require(
        implementation,
        "GetWindowPlacement(HWindow, &PreSuspendWindowPlacement)",
        "main-window placement captured before suspend",
    )
    require(
        implementation,
        "ScheduleResumeWindowPlacementRestore(HWindow);",
        "placement restore deferred until resume display changes settle",
    )
    require(
        mainwnd3,
        "MultiMonEnsureRectVisible(&placement.rcNormalPosition, FALSE);",
        "pre-suspend placement constrained to the resumed monitor topology",
    )
    require(
        mainwnd3,
        "SetWindowPlacement(hWindow, &placement);",
        "complete pre-suspend placement restored after resume",
    )

    print("Main-window move contract tests passed.")


if __name__ == "__main__":
    main()
