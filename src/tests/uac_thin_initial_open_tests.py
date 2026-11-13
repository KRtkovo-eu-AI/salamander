# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Test scoped target continuity before probe. | Reject substituted targets without elevating or deleting them. | #731 | GPT-6 |
# | 2026-11-13 | Check resume waits around scoped elevation. | Detect pause regression before preparation, consent and broker I/O. | #731 | GPT-6 |
# | 2026-11-13 | Test sticky source continuity before elevation. | Keep ordinary Retry behavior without trusting a changed source later. | #731 | GPT-6 |
# | 2026-11-13 | Test initial scoped open decisions and cancellation. | Preserve Skip All without suppressing native Retry and probe paths. | #731 | GPT-6 |
"""Test extracted thin-open logic, not complete DoCopyFile or actual UAC."""
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
    begin = worker.index("enum class ThinOpenPhase")
    phase = worker[begin:worker.index("\n", begin)]
    begin = worker.index("bool CThinCopyWorker::CancelBeforeElevation(")
    end = worker.index("static HANDLE ThinCopyOpenRetry(", begin)
    methods = worker[begin:end]
    begin = worker.index("bool CThinCopyWorker::SourceContinuityAllowsElevation()")
    end = worker.index("HANDLE CThinCopyWorker::Open(", begin)
    preparation = worker[begin:end]
    begin = worker.index("void CThinCopyWorker::RememberSource(")
    end = worker.index("bool CThinCopyWorker::ReportTargetFailure(", begin)
    remember = worker[begin:end]
    begin = worker.index("#define THIN_OPEN_SOURCE(ctx, local, overlapped)")
    end = worker.index("\n#else", worker.index("#define THIN_DELETE_TARGET", begin))
    macros = worker[begin:end]
    begin = worker.index("static HANDLE ThinCopyOpenLocalRetry(")
    end = worker.index("\n#define THIN_REMEMBER_SOURCE", begin)
    local_retry = worker[begin:end].replace("ThinCopyOpenLocalRetry(", "ActualThinCopyOpenLocalRetry(", 1)
    begin = worker.index("HANDLE CThinCopyWorker::Open(")
    end = worker.index("HANDLE CThinCopyWorker::CancelReceivedHandle(", begin)
    open_code = worker[begin:end]
    before_prepare = open_code.index("WaitBeforeElevation()")
    prepare = open_code.index("if (!EnsurePrepared())")
    prompt = open_code.index("SendMessage(progress, WM_USER_DIALOG, WORKER_DIALOG_UAC_TRANSFER")
    after_prompt = open_code.index("CancelBeforeElevation()", prompt)
    before_prompt = open_code.rfind("WaitBeforeElevation()", prepare, prompt)
    before_broker = open_code.index("WaitBeforeElevation()", after_prompt)
    broker = open_code.index("session->Open(progress", before_broker)
    if not before_prepare < prepare < before_prompt < prompt < after_prompt < before_broker < broker:
        raise RuntimeError("Pause/cancellation is not checked before preparation, prompt and broker")
    template = (ROOT / "src/tests/uac_thin_initial_open_tests.cpp.in").read_text(encoding="utf-8")
    code = (template.replace("@@PHASE@@", phase).replace("@@METHODS@@", methods)
            .replace("@@PREPARATION@@", preparation).replace("@@REMEMBER_SOURCE@@", remember)
            .replace("@@MACROS@@", macros).replace("@@LOCAL_RETRY@@", local_retry))
    if "@@" in code:
        raise RuntimeError("Missing test template replacement")
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    if not compiler:
        raise RuntimeError("MSVC cl.exe unavailable")
    flags = ("/MTd", "/Od", "/D_DEBUG") if config == "Debug" else ("/MT", "/O2", "/DNDEBUG")
    base = ROOT.parent / "builds"
    with tempfile.TemporaryDirectory(prefix="uac-thin-initial-open-", dir=base) as temp:
        source = Path(temp) / "test.cpp"
        source.write_text(code, encoding="utf-8")
        exe = Path(temp) / "test.exe"
        built = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/W4", "/WX", *flags,
                     str(source), "/Fe:" + str(exe)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        fixture = Path(temp) / 'fixture-\u6771'
        fixture.mkdir()
        (fixture / 'replacement.txt').write_bytes(b'unchanged target')
        result = run([str(exe), str(fixture)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
        if (fixture / 'missing.txt').exists() or (fixture / 'replacement.txt').read_bytes() != b'unchanged target':
            raise RuntimeError('Retry modified the missing or replacement fixture')
    if hashlib.sha256(production.read_bytes()).hexdigest() != before:
        raise RuntimeError("worker.cpp changed during test")
    print("worker.cpp SHA-256:", before)
    print("Actual extracted phase enum, cancellation guard, preparation, source identity tracking, open wrapper and macros;")
    print("broker preparation and identity are deterministic stubs; local Retry uses real CreateFileW on NTFS.")
    print("Open() pause/cancellation placement is checked structurally; this does not run DoCopyFile or UAC.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
