# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-24 | Prefer the active MSVC compiler for native tests. | GitHub runners ship Clang 20, which cannot compile the VS 18 STL that requires Clang 22. | https://github.com/KRtkovo-eu-AI/salamander/pull/865 | Grok 4.7 |

"""Compile the actual operation queue and scheduler with fake window messages.

No files are copied and no windows are opened. Generated compiler outputs live
in a disposable temporary directory; --source-ref checks an earlier revision.
"""

import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]


def source(name: str, revision: str | None) -> str:
    if revision is None:
        return (ROOT / name).read_text(encoding="utf-8-sig")
    return subprocess.check_output(
        ["git", "show", f"{revision}:src/{name}"], cwd=ROOT, timeout=15
    ).decode("utf-8-sig")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref")
    parser.add_argument("--compiler")
    args = parser.parse_args()
    compiler = args.compiler or next(
        (found for name in ("cl", "clang++", "g++") if (found := shutil.which(name))),
        None,
    )
    if compiler is None:
        parser.error("a C++17 compiler is required (clang++, g++, or cl in a developer prompt)")
    header = source("worker.h", args.source_ref)
    worker = source("worker.cpp", args.source_ref)
    declaration = header[header.index("class COperationsQueue //"):
                         header.index("extern COperationsQueue")]
    methods_start = worker.index("BOOL COperationsQueue::AddOperation(")
    last_method = worker.index("int COperationsQueue::GetNumOfOperations()", methods_start)
    methods = worker[methods_start:worker.index("\n}", last_method) + 2]
    fixture = (ROOT / "tests/operations_queue_tests.cpp.in").read_text(encoding="utf-8")
    fixture = fixture.replace("@QUEUE_DECLARATION@", declaration).replace("@QUEUE_METHODS@", methods)
    with tempfile.TemporaryDirectory(prefix="salamander-queue-tests-") as folder:
        work = Path(folder)
        unit = work / "operations_queue_tests.cpp"
        scheduler = work / "storagesched.cpp"
        binary = work / "operations_queue_tests.exe"
        unit.write_text(fixture, encoding="utf-8")
        scheduler.write_text(source("storagesched.cpp", args.source_ref), encoding="utf-8")
        (work / "storagesched.h").write_text(source("storagesched.h", args.source_ref), encoding="utf-8")
        if Path(compiler).stem.lower() == "cl":
            command = [compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8",
                       str(unit), str(scheduler), f"/Fe:{binary}"]
        else:
            command = [compiler, "-std=c++17", "-O0", str(unit), str(scheduler), "-o", str(binary)]
        subprocess.run(command, cwd=work, check=True, timeout=60)
        return subprocess.run([str(binary)], cwd=work, timeout=20).returncode


if __name__ == "__main__":
    raise SystemExit(main())
