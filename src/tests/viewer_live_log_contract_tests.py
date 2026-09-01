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

    for source in (viewer2, viewer3, resources):
        if "IDT_LOGVIEWREFRESH" in source or "SetTimer(HWindow, IDT_LOGVIEW" in source:
            raise AssertionError("Log View Mode must not use a periodic timer")

    print("Viewer live-log source-contract tests passed.")


if __name__ == "__main__":
    main()
