# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import re
import sys


SOURCE = Path(__file__).resolve().parents[1] / "fileswn3.cpp"


def main() -> int:
    text = SOURCE.read_text(encoding="utf-8")

    selection = re.search(
        r"const bool useWideDiskPath\s*=\s*(.*?);", text, re.DOTALL
    )
    if selection is None:
        print("fileswn3.cpp no longer selects a wide directory enumeration path")
        return 1

    expression = selection.group(1)
    required = ("GetPathW()", "GetACP() == CP_UTF8", "strlen(GetPath()) >= MAX_PATH")
    missing = [token for token in required if token not in expression]
    if missing:
        print("wide directory enumeration selection is missing: " + ", ".join(missing))
        return 1

    if "FindFirstFileW" not in text or "FindNextFileW" not in text:
        print("wide directory enumeration must use both FindFirstFileW and FindNextFileW")
        return 1

    print("UTF-8 and long panel paths use wide directory enumeration")
    return 0


if __name__ == "__main__":
    sys.exit(main())
