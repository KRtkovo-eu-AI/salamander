# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def require(text: str, needle: str, description: str) -> None:
    if needle not in text:
        raise AssertionError(f"Missing {description}: {needle}")


def main() -> None:
    header = (ROOT / "src/viewer.h").read_text(encoding="utf-8")
    viewer2 = (ROOT / "src/viewer2.cpp").read_text(encoding="utf-8")
    viewer3 = (ROOT / "src/viewer3.cpp").read_text(encoding="utf-8")
    resources = (ROOT / "src/resource.rh2").read_text(encoding="utf-8")

    require(header, "WM_USER_VIEWERLOGCHANGE", "the viewer file-change message")
    require(viewer2, "ReadDirectoryChangesW", "event-driven log-view monitoring")
    require(viewer2, "FILE_NOTIFY_CHANGE_LAST_WRITE", "last-write notifications")
    require(viewer2, "FILE_NOTIFY_CHANGE_SIZE", "file-size notifications")
    require(viewer2, "_wcsnicmp(info->FileName, fileName.c_str(), nameLength)",
            "filtering notifications to the viewed file")
    require(viewer2, "StartLogViewWatcher();", "starting the watcher when log view is enabled")
    require(viewer2, "StopLogViewWatcher();", "stopping the watcher during viewer changes")
    require(viewer3, "case WM_USER_VIEWERLOGCHANGE:", "dispatching file-change notifications")
    require(viewer2, "HANDLE file = OpenViewerFileForRead(FileNameW, FileName);",
            "pre-opening the viewed file for automatic refresh")
    require(viewer2, "FileChanged(file, FALSE, fatalErr, FALSE);",
            "passing the pre-opened handle to FileChanged")
    require(viewer2, "err == ERROR_SHARING_VIOLATION || err == ERROR_LOCK_VIOLATION",
            "handling temporary writer locks without the global error path")
    require(viewer2, "SetTimer(HWindow, IDT_LOGVIEWRETRY, 200, NULL)",
            "scheduling the short lock retry")
    require(viewer2, "if (!LogViewRetryScheduled && LogViewMode)",
            "coalescing pending lock retries")
    require(viewer2, "CancelLogViewRetry();\n        StopLogViewWatcher();",
            "canceling retries when log mode is disabled")
    require(viewer2, "CancelLogViewRetry();\n    StopLogViewWatcher();",
            "canceling retries before opening another file")
    require(viewer2, "UpdateWindow(HWindow);\n    CancelLogViewRetry();",
            "canceling retries after a successful refresh")
    require(viewer3, "KillTimer(HWindow, IDT_LOGVIEWRETRY);\n            LogViewRetryScheduled = FALSE;",
            "making the retry timer one-shot")
    require(viewer3, "CancelLogViewRetry();\n        StopLogViewWatcher();",
            "canceling retries during window destruction")

    allowed_timer = "if (SetTimer(HWindow, IDT_LOGVIEWRETRY, 200, NULL) != 0)"
    timer_calls = [line.strip() for source in (viewer2, viewer3, resources)
                   for line in source.splitlines()
                   if "SetTimer(HWindow, IDT_LOGVIEW" in line]
    if timer_calls != [allowed_timer]:
        raise AssertionError(
            f"Log View Mode may use only its coalesced 200 ms one-shot retry: {timer_calls}")
    print("Viewer live-log source-contract tests passed.")


if __name__ == "__main__":
    main()
