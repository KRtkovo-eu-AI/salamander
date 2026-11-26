# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Run real NTFS directory pin probes. | Verify rename and pre-existing DELETE-handle boundaries. | #731 | GPT-6.1 Terra |
"""Compile and execute the non-destructive directory pin fixture."""
import argparse
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    environment = compiler_environment()
    compiler = shutil.which("cl", path=environment.get("PATH"))
    source = ROOT / "src/tests/uac_delete_tree_pin_tests.cpp"
    configs = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="uac-delete-tree-pin-") as temp:
        folder = Path(temp)
        executable = folder / "test.exe"
        for config in configs:
            flags = ["/MDd", "/D_DEBUG", "/Od"] if config == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Delete-tree directory pins:", config, flush=True)
            build = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", *flags,
                         str(source), "/Fe:" + str(executable), "/link", "ntdll.lib"],
                        cwd=folder, env=environment, timeout=35)
            if build.returncode:
                return build.returncode
            result = run([str(executable), str(folder)], cwd=folder, env=environment, timeout=15)
            if result.returncode:
                return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
