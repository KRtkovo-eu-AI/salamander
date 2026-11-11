# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Extract the actual timestamp receipt decision. | Check that Ignore does not authorize a new elevated retry. | #731 | GPT-6.1 Terra |
# | 2026-11-11 | Run the actual late move receipt and local unlink decisions. | Retain Ignore semantics while unsafe expanded receipts remain disabled. | #731 | GPT-6.1 Terra |
"""Compile the current worker branches with a temporary, owned file fixture."""
import argparse
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    worker = block(read("src/worker.cpp"), "BOOL DoMoveFile(")
    start = worker.index("const bool capture =", worker.index("MoveCopyReceipt receipt;"))
    end = worker.index(";", start) + 1
    capture = worker[start:end]
    copy = block(read("src/worker.cpp"), "BOOL DoCopyFile(")
    older_start = copy.index("if ((op->OpFlags & OPFL_OVERWROLDERALRTESTED)")
    older_end = copy.index("// decide which algorithm to use for copying", older_start)
    older = copy[older_start:older_end]
    assert "CompareFileTime(&dataIn.ftLastWriteTime, &dataOut.ftLastWriteTime) <= 0" in older
    assert "*skip = TRUE;" in older and "return TRUE;" in older
    assert "if (notError && !skip)" in worker[end:]
    reset_start = copy.index("if (moveReceipt)", copy.index("COPY_AGAIN:"))
    reset_end = copy.index(";", reset_start) + 1
    reset_copy_attempt = copy[reset_start:reset_end]
    time_start = copy.index("// The newly supported Overwrite Older retry")
    time_end = copy.index(";", copy.index("moveReceipt->captured =", time_start)) + 1
    capture_after_times = copy[time_start:time_end]
    assert "capture ? &receipt : nullptr" in worker[end:]
    local_start = worker.index("const auto sourcePath =", end)
    local_end = worker.index("                {\n                    WaitForSingleObject", local_start)
    local = worker[local_start:local_end]
    full_removal = block(worker, "if (notError && !skip)")
    assert "else\n                        err = DeleteFileW(" in local
    assert "if (!protectedCompletion)" in local
    retry = worker[local_end:]
    assert retry.index("protectedCompletion = true;") < retry.index("ExecuteWorkerTransfer(")
    transfer = block(read("src/worker.cpp"), "static bool ExecuteWorkerTransfer(")
    unknown = block(transfer, "if (result.unknown)")
    assert "SendMessage(" in unknown and "return true;" in unknown
    code = read("src/tests/uac_move_completion_route_tests.cpp.in")
    code = code.replace("@@CAPTURE_POLICY@@", capture).replace("@@LOCAL_REMOVAL@@", local)
    code = code.replace("@@CAPTURE_AFTER_TIMES@@", capture_after_times)
    code = code.replace("@@RESET_COPY_ATTEMPT@@", reset_copy_attempt)
    code = code.replace("@@FULL_REMOVAL@@", full_removal)
    assert "@@" not in code
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="uac-move-route-") as temporary:
        folder = Path(temporary)
        source, exe = folder / "test.cpp", folder / "test.exe"
        source.write_text(code, encoding="utf-8")
        for configuration in configurations:
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Actual late move route:", configuration, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                             *flags, str(source), "/Fe:" + str(exe)], [str(exe)]):
                result = run(command, cwd=folder, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
