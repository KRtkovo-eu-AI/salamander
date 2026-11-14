# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Exercise native retry-tail verification with NTFS I/O. | Preserve original sync and async behavior while distinguishing a unit test from full F5. | #731 | GPT-6 |
"""Test extracted native retry-tail code; this is not DoCopyFile or UAC."""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    config = parser.parse_args().configuration
    worker_path = ROOT / "src/worker.cpp"
    header_path = ROOT / "src/worker.h"
    before_worker = hashlib.sha256(worker_path.read_bytes()).hexdigest()
    before_header = hashlib.sha256(header_path.read_bytes()).hexdigest()
    worker = worker_path.read_text(encoding="utf-8-sig")
    header = header_path.read_text(encoding="utf-8-sig")
    async_code = worker[worker.index("struct CAsyncCopyParams\n{"):
                        worker.index("// **********************************************************************************\n", worker.index("struct CAsyncCopyParams\n{"))]
    tail_code = worker[worker.index("BOOL SalSetFilePointer(HANDLE file, const CQuadWord& offset)"):
                       worker.index("// copies ADS into the newly created file/directory")]
    buffer_match = re.search(r"^#define ASYNC_COPY_BUF_SIZE \([^\r\n]*", header, re.MULTILINE)
    if not buffer_match:
        raise RuntimeError("native asynchronous buffer definition not found")
    template = (ROOT / "src/tests/uac_thin_retry_tail_tests.cpp.in").read_text(encoding="utf-8")
    code = (template.replace("@@BUFFER_SIZE@@", buffer_match.group(0))
            .replace("@@ASYNC_CODE@@", async_code).replace("@@TAIL_CODE@@", tail_code))
    if "@@" in code:
        raise RuntimeError("unexpanded native test placeholder")
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    if not compiler:
        raise RuntimeError("MSVC cl.exe unavailable")
    flags = ("/MTd", "/Od", "/D_DEBUG") if config == "Debug" else ("/MT", "/O2", "/DNDEBUG")
    with tempfile.TemporaryDirectory(prefix="uac-thin-retry-tail-", dir=ROOT.parent / "builds") as temp:
        source = Path(temp) / "test.cpp"
        source.write_text(code, encoding="utf-8")
        exe = Path(temp) / "test.exe"
        built = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                     "/wd4201", "/wd4244", *flags, str(source), "/Fe:" + str(exe)], cwd=temp, env=env, timeout=50)
        if built.returncode:
            return built.returncode
        result = run([str(exe), temp], cwd=temp, env=env, timeout=25)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(worker_path.read_bytes()).hexdigest() != before_worker:
        raise RuntimeError("worker.cpp changed during test")
    if hashlib.sha256(header_path.read_bytes()).hexdigest() != before_header:
        raise RuntimeError("worker.h changed during test")
    print("worker.cpp SHA-256:", before_worker)
    print("worker.h SHA-256:", before_header)
    print("Actual extracted CheckTailOfOutFile and CAsyncCopyParams; real NTFS/Win32 I/O.")
    print("No full DoCopyFile, broker, privilege elevation, GUI, or cross-process handle transfer is tested.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
