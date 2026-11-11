# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise real conflict snapshot and resolution methods. | Reject stale native modal choices and their Apply All effect. | #731 | GPT-6.1 Terra |
"""Compile real coordinator methods with isolated path and queue seams."""
import argparse
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    worker, header = read("src/worker.cpp"), read("src/worker.h")
    uac = read("src/uacbroker/f7pilot.h")
    dialogs = read("src/dialogs.cpp")
    first = block(dialogs, "void CProgressDialog::PromptNextConflict()")
    assert "ResolveConflictSnapshot(conflict, decision," in first
    assert "ConflictAutomaticPromptConsumed = FALSE;" in first
    assert "Script == promptScript && !CancelWorker" in first
    assert "SetDeferredConflictDecision(" not in first
    manual_start = dialogs.index("if (command == IDC_PROGRESS_SOLVE || command == IDC_PROGRESS_SKIP)")
    manual_end = dialogs.index("if (command == IDC_PROGRESS_OVERWRITE_ALL || command == IDC_PROGRESS_SKIP_ALL)", manual_start)
    manual = dialogs[manual_start:manual_end]
    assert manual.count("ResolveConflictSnapshot(conflict, decision,") == 2
    assert "SetDeferredConflictDecision(" not in manual
    assert manual.index("else if (ret != IDB_SKIP && ret != IDB_SKIPALL)") < manual.index("ResolveConflictSnapshot(conflict, decision,")
    assert "return TRUE;" in manual

    code = read("src/tests/uac_conflict_snapshot_tests.cpp.in")
    snippets = {
        "@@REVISION@@": block(uac, "struct ArchiveRevision", ";"),
        "@@SAME_REVISION@@": block(uac, "inline bool SameFileRevision("),
        "@@SNAPSHOT@@": block(header, "struct CCopyMoveConflictSnapshot", ";"),
        "@@DECISION_SNAPSHOT@@": block(header, "struct CCopyMoveConflictDecisionSnapshot", ";"),
        "@@REQUEST@@": block(header, "struct CCopyMoveConflictRequest", ";"),
        "@@GET_SNAPSHOT@@": block(worker, "BOOL COperations::GetConflictSnapshot("),
        "@@GET_SNAPSHOT_LOCKED@@": block(worker, "void COperations::GetConflictSnapshotLocked("),
        "@@GET_DECISION_SNAPSHOT@@": block(worker, "BOOL COperations::GetConflictDecisionSnapshot("),
        "@@PUBLISH@@": block(worker, "void COperations::PublishConflictScanResult("),
        "@@REQUEUE@@": block(worker, "void COperations::RequeueChangedConflictItem("),
        "@@RESOLVE_REQUEST@@": block(worker, "BOOL COperations::ResolveConflictRequest("),
        "@@RESOLVE_SNAPSHOT@@": block(worker, "BOOL COperations::ResolveConflictSnapshot("),
        "@@SET_LOCKED@@": block(worker, "void COperations::SetDeferredConflictDecisionLocked("),
        "@@SET@@": block(worker, "void COperations::SetDeferredConflictDecision("),
    }
    for key, value in snippets.items():
        code = code.replace(key, value)
    assert "@@" not in code
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    configs = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="uac-conflict-snapshot-") as temp:
        folder = Path(temp)
        source, exe = folder / "test.cpp", folder / "test.exe"
        source.write_text(code, encoding="utf-8")
        for configuration in configs:
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Actual conflict coordinator:", configuration, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                             *flags, str(source), "/Fe:" + str(exe)], [str(exe)]):
                result = run(command, cwd=folder, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
