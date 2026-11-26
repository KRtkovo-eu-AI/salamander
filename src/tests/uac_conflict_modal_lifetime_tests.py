# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise reentrant termination of conflict prompts. | Keep displayed names owned and never resolve after the worker frees its script. | #731 | GPT-6.1 Terra |
"""Run actual conflict UI methods using a simulated nested modal loop."""
import argparse
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    source = read("src/dialogs.cpp")
    code = read("src/tests/uac_conflict_modal_lifetime_tests.cpp.in")
    for key, value in {
        "@@GUARD@@": block(source, "class CConflictPromptPresentationGuard", ";"),
        "@@BEGIN@@": block(source, "void CProgressDialog::BeginConflictPromptPresentation()"),
        "@@END@@": block(source, "void CProgressDialog::EndConflictPromptPresentation()"),
        "@@PROMPT@@": block(source, "void CProgressDialog::PromptNextConflict()"),
        "@@MANUAL@@": block(source, "if (command == IDC_PROGRESS_SOLVE || command == IDC_PROGRESS_SKIP)"),
    }.items():
        code = code.replace(key, value)
    assert "@@" not in code
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    configs = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="uac-conflict-modal-") as temp:
        folder = Path(temp)
        test, exe = folder / "test.cpp", folder / "test.exe"
        test.write_text(code, encoding="utf-8")
        for configuration in configs:
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Actual conflict UI methods:", configuration, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                             *flags, str(test), "/Fe:" + str(exe)], [str(exe)]):
                result = run(command, cwd=folder, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
