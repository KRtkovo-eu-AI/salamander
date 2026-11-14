# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Bound native modal reentrancy experiments. | Ensure a stuck SendMessage experiment leaves no running process. | #731 | GPT-6.1 Sol |
"""Run isolated Windows message experiments; this is not the production dialog."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    source = ROOT / "src/tests/uac_worker_prompt_reentrance_tests.cpp"
    configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="uac-modal-reentrance-") as temp:
        for config in configurations:
            flags = ("/MTd", "/Od", "/D_DEBUG") if config == "Debug" else ("/MT", "/O2", "/DNDEBUG")
            exe = Path(temp) / (config + ".exe")
            print(config, flush=True)
            result = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                          *flags, str(source), "/Fe:" + str(exe), "/link", "user32.lib"],
                         cwd=temp, env=env, timeout=30)
            if result.returncode:
                return result.returncode
            for mode in ("guard", "legacy"):
                for sent in ("posted", "sent"):
                    for cancel in ("normal", "cancel"):
                        try:
                            # subprocess.run kills and waits for the child on timeout.
                            result = subprocess.run([str(exe), mode, sent, cancel], cwd=temp, env=env, timeout=7)
                        except subprocess.TimeoutExpired:
                            print(f"TIMEOUT: {config} {mode} {sent} {cancel}", flush=True)
                            return 1
                        if result.returncode:
                            return result.returncode
    print("NOTE: native HWND/message test models the handler; it does not instantiate CProgressDialog.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
