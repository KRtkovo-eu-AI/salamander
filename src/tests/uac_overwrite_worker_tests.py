# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Match native-transfer guards in overwrite tests. | Keep thin-copy paths out of legacy broker fallback assertions. | #731 | GPT-6 |
# | 2026-11-11 | Compile actual overwrite confirmation and retry flow with isolated UI seams. | Keep exact revisions, Hidden/System decisions and move retirement ordering intact. | #731 | GPT-6.1 Sol |
"""Run production overwrite decisions without file mutations, elevation or a visible UI."""
import argparse
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    worker = read("src/worker.cpp")
    prepare = block(worker, "static bool PrepareWorkerOverwrite(")
    assert "op.Opcode != ocCopyFile && op.Opcode != ocMoveFile" in prepare
    assert "SameFileRevision(op.ScannedTargetRevision, request.Approval().revision)" in prepare
    ordinary_copy = block(worker, "BOOL DoCopyFile(")
    assert "chAttr = SetFileAttributesW(" in ordinary_copy
    assert "if (chAttr)\n                                            canElevateOverwrite = false;" in ordinary_copy
    target_marker = "if (originalTransfer && !transferError && !copyAsEncrypted && (err == ERROR_ACCESS_DENIED || protectedExistingMove)"
    target_denied = block(ordinary_copy, target_marker)
    assert target_denied.index("CloseHandle(in)") < target_denied.index("TryOverwriteDeniedSource(")
    assert "if (outcome == SourceOverwriteResult::Finished && isMove && skip)" in target_denied and "*skip = TRUE;" in target_denied
    policy_start = ordinary_copy.index("const DWORD existingAttributes = isMove && targetExists")
    policy_end = ordinary_copy.index(target_marker, policy_start)
    policy = ordinary_copy[policy_start:policy_end]
    assert ordinary_copy.index("protectedExistingMove") < ordinary_copy.index("WorkerClearReadOnlyW(WorkerOperationPathW(op->TargetName", policy_start)
    late = block(ordinary_copy, "if (originalTransfer && err == ERROR_ACCESS_DENIED && !copyAsEncrypted && !dlgData.SkipAllFileOpenOut)")
    assert late.index("CloseHandle(in)") < late.index("TryOverwriteDeniedSource(")
    assert "if (isMove && in != INVALID_HANDLE_VALUE)" in late
    assert "if (outcome == SourceOverwriteResult::Finished && isMove && skip)" in late and "*skip = TRUE;" in late
    assert "goto SKIP_OPEN;" in late
    ordinary_move = block(worker, "BOOL DoMoveFile(")
    assert "PrepareWorkerOverwrite(*op, *script, dlgData, moveOverwrite)" in ordinary_move
    assert "approvedMoveOverwrite = moveOverwrite.Approval();" in ordinary_move
    assert ordinary_move.index("PrepareWorkerOverwrite(*op, *script, dlgData, moveOverwrite)") < ordinary_move.index("WorkerClearReadOnlyW(WorkerOperationPathW(op->TargetName")
    assert "ProbeLocalElevationAccess(op->SourceNameW, DELETE | FILE_WRITE_ATTRIBUTES, false)" in ordinary_move
    assert "if (notError && !skip)" in ordinary_move
    code = read("src/tests/uac_overwrite_worker_tests.cpp.in")
    code = code.replace("@@EXISTING_MOVE_POLICY@@", policy)
    code = code.replace("@@DECISION@@", block(worker, "enum class SourceOverwriteResult", ";"))
    code = code.replace("@@CONFIRM@@", block(worker, "static SourceOverwriteResult ConfirmBrokerOverwriteAttributes("))
    code = code.replace("@@OVERWRITE@@", block(worker, "static SourceOverwriteResult TryOverwriteDeniedSource("))
    assert "@@" not in code
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    configs = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="uac-overwrite-worker-") as temp:
        folder = Path(temp)
        source, exe = folder / "test.cpp", folder / "test.exe"
        source.write_text(code, encoding="utf-8")
        for configuration in configs:
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Actual overwrite decisions:", configuration, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                             *flags, str(source), "/Fe:" + str(exe)], [str(exe)]):
                result = run(command, cwd=folder, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
