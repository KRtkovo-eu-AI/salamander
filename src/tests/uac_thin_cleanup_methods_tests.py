# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Cover deferred source cancellation and safe local retry. | Preserve live targets and reject side-effecting reopen. | #731 | GPT-6 |
# | 2026-11-12 | Exercise scoped cleanup state transitions. | Test failure handling with the actual helper methods and deterministic stubs. | #731 | GPT-6 |
"""Test extracted cleanup methods; this is not a full DoCopyFile test."""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import ROOT, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    config = parser.parse_args().configuration
    production = ROOT / "src/worker.cpp"
    before = hashlib.sha256(production.read_bytes()).hexdigest()
    worker = read("src/worker.cpp")
    start = worker.index("bool CThinCopyWorker::ReportTargetFailure(")
    end = worker.index("\n#endif", start)
    methods = worker[start:end]
    start = worker.index("#define THIN_DELETE_TARGET(ctx, local)")
    end = worker.index("\n#else", start)
    macro = worker[start:end]
    template = (ROOT / "src/tests/uac_thin_cleanup_methods_tests.cpp.in").read_text(encoding="utf-8")
    cancel = worker[worker.index("HANDLE CThinCopyWorker::CancelReceivedHandle("):
                    worker.index("void CThinCopyWorker::RememberSource(")]
    local_retry = worker[worker.index("static HANDLE ThinCopyOpenLocalRetry("):
                         worker.index("#define THIN_REMEMBER_SOURCE")]
    code = (template.replace("@@METHODS@@", methods).replace("@@CANCEL_METHOD@@", cancel)
            .replace("@@LOCAL_RETRY@@", local_retry).replace("@@MACRO@@", macro))
    assert "@@" not in code
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    if not compiler:
        raise RuntimeError("MSVC cl.exe unavailable")
    flags = ("/MTd", "/Od", "/D_DEBUG") if config == "Debug" else ("/MT", "/O2", "/DNDEBUG")
    base = ROOT.parent / "builds"
    with tempfile.TemporaryDirectory(prefix="uac-thin-cleanup-methods-", dir=base) as temp:
        source = Path(temp) / "test.cpp"
        source.write_text(code, encoding="utf-8")
        exe = Path(temp) / "test.exe"
        built = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/W4", "/WX", *flags,
                     str(source), "/Fe:" + str(exe)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(exe)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if hashlib.sha256(production.read_bytes()).hexdigest() != before:
        raise RuntimeError("worker.cpp changed during test")
    print("worker.cpp SHA-256:", before)
    print("Actual extracted cleanup methods and macro; broker, dialog and CloseHandle are deterministic stubs.")
    print("This does not exercise the complete DoCopyFile worker or UAC.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
