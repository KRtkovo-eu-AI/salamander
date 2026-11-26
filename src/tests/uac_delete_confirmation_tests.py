# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise the actual shared delete confirmation and its worker ordering. | Preserve Hidden/System decisions before ordinary or elevated deletion. | #731 | GPT-6.1 Sol |
"""Compile the production delete confirmation with isolated UI seams; no file deletion or elevation."""
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
    helper = block(worker, "static FileDeleteDecision ConfirmDeleteFileAttributes(")
    verified = block(worker[worker.index("static BOOL ExecuteVerifiedWorkerLeaf("):],
                     "static bool TryDeleteVerifiedLeaf(HWND progress")
    confirmation = block(verified, "if (!accessDenied && op.Opcode == ocDeleteFile)")
    assert "ConfirmDeleteFileAttributes(" in confirmation
    assert "if (decision != FileDeleteDecision::Proceed)" in confirmation
    assert "success = decision == FileDeleteDecision::Skip;" in confirmation
    assert "return true;" in confirmation
    assert verified.index("ConfirmDeleteFileAttributes(") < verified.index("ExecuteVerifiedWorkerLeaf(")
    ordinary = block(worker, "BOOL DoDeleteFile(")
    assert ordinary.index("ConfirmDeleteFileAttributes(") < ordinary.index("WorkerClearReadOnlyW(")
    assert "IDS_CONFIRMSHFILEDELETE" not in ordinary
    assert "FileDeleteDecision::Skip" in ordinary and "FileDeleteDecision::Cancel" in ordinary
    # Directory Hidden/System confirmation remains in the existing planner.
    planner = read("src/fileswn6.cpp")
    directory = planner.split("BOOL CFilesWindow::BuildScriptDir(", 1)[1].split("BOOL CFilesWindow::BuildScriptFile(", 1)[0]
    assert "Configuration.CnfrmSHDirDel" in directory and "IDS_DELETESHDIR" in directory
    code = read("src/tests/uac_delete_confirmation_tests.cpp.in")
    code = code.replace("@@DECISION@@", block(worker, "enum class FileDeleteDecision", ";"))
    code = code.replace("@@CONFIRM@@", helper)
    assert "@@" not in code
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="uac-delete-confirmation-") as temp:
        directory = Path(temp)
        source = directory / "test.cpp"
        source.write_text(code, encoding="utf-8")
        exe = directory / "test.exe"
        for configuration in configurations:
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Actual delete confirmation:", configuration, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                             *flags, str(source), "/Fe:" + str(exe)], [str(exe)]):
                result = run(command, cwd=directory, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
