# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-04 | Stop a timed-out association-cache compiler and its children. | A compiler left running after the timeout starved the following contract tests. | https://github.com/KRtkovo-eu-AI/salamander/issues/777 | Grok 4.7 |

"""Compile and exercise production association-cache logic without the GUI.

The C++ fixture substitutes icon pixels, window repainting and allocation failure;
the refresh handler, association insertion and cache lookup come from the actual
sources. --source-ref permits demonstrating that the same tests fail before a fix.
All generated sources and compiler outputs live in an automatically removed temp
directory. Requires a C++17 compiler (clang++, g++, or cl in a developer prompt).
"""

import argparse
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]


def source(name: str, revision: str | None) -> str:
    if revision is None:
        return (ROOT / name).read_text(encoding="utf-8-sig")
    return subprocess.check_output(
        ["git", "show", f"{revision}:src/{name}"], cwd=ROOT, timeout=15
    ).decode("utf-8-sig")


def function(text: str, signature: str) -> str:
    start = text.index(signature)
    # These top-level definitions have their closing brace at column zero;
    # nested scopes are indented. Fail loudly if their structure changes.
    end = text.index("\n}", start) + 2
    return text[start:end]


def terminate_process_tree(proc: subprocess.Popen) -> None:
    """Stop a timed-out compiler and the children Python's timeout leaves behind."""
    if os.name == "nt":
        try:
            subprocess.run(
                ["taskkill", "/F", "/T", "/PID", str(proc.pid)],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=8,
                check=False,
            )
        except (OSError, subprocess.TimeoutExpired):
            proc.kill()
    else:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except (ProcessLookupError, PermissionError, OSError):
            proc.kill()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            pass


def run_command(command: list[str], cwd: Path, timeout: int, check: bool) -> int:
    # A new session lets the timeout kill the compiler and every helper it starts.
    proc = subprocess.Popen(
        command,
        cwd=cwd,
        start_new_session=os.name != "nt",
    )
    try:
        code = proc.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        terminate_process_tree(proc)
        raise
    if check and code != 0:
        raise subprocess.CalledProcessError(code, command)
    return code


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref", help="read production sources from this git revision")
    parser.add_argument("--compiler", help="C++ compiler executable")
    args = parser.parse_args()
    compiler = args.compiler or next(
        (found for name in ("clang++", "g++", "cl") if (found := shutil.which(name))),
        None,
    )
    if compiler is None:
        parser.error("a C++17 compiler is required (clang++, g++, or cl)")

    cache = source("icncache.cpp", args.source_ref)
    header = source("icncache.h", args.source_ref)
    panel = source("fileswnb.cpp", args.source_ref)
    data_start = header.index("struct CAssociationIndexAndFlag")
    data_end = header.index("\n};", header.index("class CAssociationData", data_start)) + 3
    methods = "\n\n".join(function(cache, signature) for signature in (
        "static CAssociationsPixelIconSet* FindAssociationPixelSet(",
        "int CAssociations::GetPixelIconIndex(",
        "BOOL CAssociations::SetPixelIconIndex(",
        "BOOL CAssociations::IsAssociated(char* ext,",
    ))
    handler = panel[panel.index("    case WM_USER_REFRESHINDEX:"):
                    panel.index("    case WM_USER_DROPCOPYMOVE:")]
    template = (ROOT / "tests/association_icon_cache_tests.cpp.in").read_text(encoding="utf-8")
    for key, value in {
        "ASSOCIATION_DATA": header[data_start:data_end],
        "CACHE_METHODS": methods,
        "INSERT_DATA": function(cache, "void CAssociations::InsertData("),
        "REFRESH_HANDLER": handler,
    }.items():
        template = template.replace(f"@{key}@", value)
    if re.search(r"@[A-Z_]+@", template):
        raise RuntimeError("unexpanded production-source placeholder")

    # ignore_cleanup_errors covers a compiler that Windows has not released yet.
    with tempfile.TemporaryDirectory(
        prefix="salamander-association-tests-", ignore_cleanup_errors=True
    ) as folder:
        work = Path(folder)
        unit = work / "association_icon_cache_tests.cpp"
        binary = work / "association_icon_cache_tests.exe"
        unit.write_text(template, encoding="utf-8")
        if Path(compiler).stem.lower() == "cl":
            command = [compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8",
                       str(unit), f"/Fe:{binary}"]
        else:
            command = [compiler, "-std=c++17", "-O0", str(unit), "-o", str(binary)]
        # Stay inside the native runner's per-command cap, with time left to run.
        run_command(command, work, timeout=90, check=True)
        return run_command([str(binary)], work, timeout=15, check=False)


if __name__ == "__main__":
    raise SystemExit(main())
