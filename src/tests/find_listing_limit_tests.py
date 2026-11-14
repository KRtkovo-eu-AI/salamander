# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Test actual protected Find per-directory bounds. | Preserve the existing 4096-entry broker limit. | #731 | GPT-6.1 Terra |
"""Compile the real broker enumerator and test it on a temporary NTFS directory."""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    with tempfile.TemporaryDirectory(prefix="find-real-listing-") as temp:
        folder = Path(temp)
        for mode in (("Debug", "Release") if args.configuration == "Both" else (args.configuration,)):
            flags = ["/MTd", "/Od", "/D_DEBUG"] if mode == "Debug" else ["/MT", "/O2", "/DNDEBUG"]
            exe = folder / (mode + ".exe")
            print("Real NTFS per-directory listing:", mode, flush=True)
            compile_result = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", *flags,
                                  str(ROOT / "src/tests/find_listing_limit_tests.cpp"), "/Fe:" + str(exe),
                                  "/link", "advapi32.lib"], cwd=folder, env=env, timeout=45)
            if compile_result.returncode:
                return compile_result.returncode
            result = run([str(exe), str(folder)], cwd=folder, env=env, timeout=55)
            if result.returncode:
                return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
