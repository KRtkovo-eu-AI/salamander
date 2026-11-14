# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Bound the extracted pre-consent cleanup block. | Keep the pause test aligned with batch consent handling. | #731 | GPT-6 |
# | 2026-11-13 | Test thin F5 pause and cancellation before elevation. | Exercise actual wait methods without automating UAC. | #731 | GPT-6 |
"""Test extracted thin-worker waits; this is not complete F5 or actual UAC."""
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
    begin = worker.index("bool CThinCopyWorker::CancelBeforeElevation()")
    end = worker.index("static HANDLE ThinCopyOpened(", begin)
    methods = worker[begin:end]
    begin = worker.index("HANDLE CThinCopyWorker::Open(")
    end = worker.index("HANDLE CThinCopyWorker::CancelReceivedHandle(", begin)
    opened = worker[begin:end]
    initial = opened.index("WaitBeforeElevation()")
    prepare = opened.index("if (!EnsurePrepared())")
    prompt = opened.index("SendMessage(progress, WM_USER_DIALOG, WORKER_DIALOG_UAC_TRANSFER")
    before_prompt = opened.rfind("WaitBeforeElevation()", prepare, prompt)
    after_prompt = opened.index("WaitBeforeElevation()", prompt)
    broker = opened.index("session->Open(progress", after_prompt)
    if not initial < prepare < before_prompt < prompt < after_prompt < broker:
        raise RuntimeError("Real Open no longer waits before preparation, consent and broker")
    erase_begin = opened.rfind("if (!WaitBeforeElevation())", prepare, prompt)
    erase_end = opened.index("\n                try\n", erase_begin)
    erase_block = opened[erase_begin:erase_end]
    template = (ROOT / "src/tests/uac_thin_pause_tests.cpp.in").read_text(encoding="utf-8")
    code = template.replace("@@WAIT_METHODS@@", methods).replace("@@ERASE_BLOCK@@", erase_block)
    if "@@" in code:
        raise RuntimeError("Missing test template replacement")
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    if not compiler:
        raise RuntimeError("MSVC cl.exe unavailable")
    flags = ("/MTd", "/Od", "/D_DEBUG") if config == "Debug" else ("/MT", "/O2", "/DNDEBUG")
    with tempfile.TemporaryDirectory(prefix="uac-thin-pause-", dir=ROOT.parent / "builds") as temp:
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
    print("Actual extracted wait methods and Win32 events; prompt/broker are test seams.")
    print("Real Open placement is checked structurally; complete DoCopyFile and UAC were not run.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
