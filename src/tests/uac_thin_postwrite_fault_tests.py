# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Exercise post-completion write-fault test hooks. | Verify fault ordering and native sync/async wiring without claiming a full F5 run. | #731 | GPT-6 |
"""Test extracted test-only worker hooks with stubs, not complete F5 or UAC."""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    config = parser.parse_args().configuration
    production = ROOT / "src/worker.cpp"
    before = hashlib.sha256(production.read_bytes()).hexdigest()
    worker = production.read_text(encoding="utf-8-sig")
    failure = worker[worker.index("static bool ThinCopyTestFailure("):
                     worker.index("// Observation must not change", worker.index("static bool ThinCopyTestFailure("))]
    observe = worker[worker.index("static void ThinCopyTestObserve("):
                     worker.index("static BOOL ThinCopyTestReadFile(")]
    read = worker[worker.index("static BOOL ThinCopyTestReadFile("):
                  worker.index("// This marker describes a full nonzero write")]
    write = worker[worker.index("// This marker describes a full nonzero write"):
                   worker.index("#define THIN_COPY_READ ThinCopyTestReadFile")]
    async_loop = worker[worker.index("void DoCopyFileLoopAsync("):]
    start = async_loop.index("                        BOOL res = GetOverlappedResult(out, asyncPar->GetOverlapped(i), &bytes, TRUE);")
    end = async_loop.index("#endif", start) + len("#endif")
    completion = async_loop[start:end]
    if 'ThinCopyTestMarkWriteCompleted(true);' not in completion:
        raise RuntimeError("real asynchronous completion does not mark successful writes")
    orig_loop = worker[worker.index("void DoCopyFileLoopOrig("):worker.index("void DoCopyFileLoopAsync(")]
    if 'THIN_COPY_WRITE(out, buffer, read, &written, NULL)' not in orig_loop:
        raise RuntimeError("real synchronous loop does not use the instrumented write")
    async_issue = worker[worker.index("BOOL CCopy_Context::StartWriting("):worker.index("BOOL CCopy_Context::", worker.index("BOOL CCopy_Context::StartWriting(") + 1)]
    if 'THIN_COPY_WRITE(*Out, AsyncPar->Buffers[blkIndex]' not in async_issue:
        raise RuntimeError("real asynchronous issue does not use the instrumented write")
    if '#define THIN_COPY_WRITE ThinCopyTestWriteFile' not in worker:
        raise RuntimeError("test build does not connect the instrumented write")
    template = (ROOT / "src/tests/uac_thin_postwrite_fault_tests.cpp.in").read_text(encoding="utf-8")
    code = (template.replace("@@FAILURE@@", failure).replace("@@OBSERVE@@", observe).replace("@@READ@@", read)
            .replace("@@WRITE@@", write).replace("@@ASYNC_COMPLETION@@", completion))
    if "@@" in code:
        raise RuntimeError("unexpanded placeholder")
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    if not compiler:
        raise RuntimeError("MSVC cl.exe unavailable")
    flags = ("/MTd", "/Od", "/D_DEBUG") if config == "Debug" else ("/MT", "/O2", "/DNDEBUG")
    with tempfile.TemporaryDirectory(prefix="uac-thin-postwrite-", dir=ROOT.parent / "builds") as temp:
        source = Path(temp) / "test.cpp"
        source.write_text(code, encoding="utf-8")
        exe = Path(temp) / "test.exe"
        built = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/W4", "/WX",
                     "/DSAM_UAC_THIN_COPY_TEST", *flags, str(source), "/Fe:" + str(exe)],
                    cwd=temp, env=env, timeout=50)
        if built.returncode:
            return built.returncode
        for mode in ("write", "write-after-completion", "async", "read", "read-pending", "none"):
            result = run([str(exe), mode], cwd=temp, env=env, timeout=15)
            if result.returncode:
                return result.returncode
    if hashlib.sha256(production.read_bytes()).hexdigest() != before:
        raise RuntimeError("worker.cpp changed during test")
    print("worker.cpp SHA-256:", before)
    print("Actual extracted hooks and async completion fragment; I/O and event delivery are stubs.")
    print("This does not exercise the complete DoCopyFile worker, the broker or UAC.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
