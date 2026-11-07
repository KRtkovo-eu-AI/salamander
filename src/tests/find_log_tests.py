# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-07 | Compile and exercise the actual Find log without a dialog. | Verify worker-owned inputs, bounded storage and snapshot cleanup under contention and allocation failure. | https://github.com/KRtkovo-eu-AI/salamander/issues/819 | GPT-6 (Codex) |
"""Run actual Find log methods and worker handoff with Win32 locks and TDirectArray."""
import argparse
from pathlib import Path
import re
import shutil
import tempfile

from branch_archive_selection_tests import ROOT, block, compiler_environment, read, run


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    header = read("src/find.h")
    implementation = read("src/finddlg2.cpp")
    start = implementation.index("#define DEFERRED_ERRORS_MAX ")
    end = implementation.index("CFindLogDialog::CFindLogDialog(", start)
    definitions = "\n".join(re.findall(r"^#define FLI_[A-Z]+\b[^\n]*", header, re.MULTILINE))
    if len(definitions.splitlines()) != 3:
        raise AssertionError("missing Find log flags")
    pieces = {
        "LOG_DECLARATION": definitions + "\n" + block(header, "struct CFindLogItem\n", ";")
        + "\n" + block(header, "class CFindLog\n", ";"),
        "LOG_IMPLEMENTATION": implementation[start:end],
        "WORKER_LOG": block(read("src/find.cpp"), "static void AddFindLog("),
    }
    text = read("src/tests/find_log_tests.cpp.in")
    for key, value in pieces.items():
        text = text.replace("@@" + key + "@@", value)
    if "@@" in text:
        raise AssertionError("unexpanded Find log fixture placeholder")

    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    with tempfile.TemporaryDirectory(prefix="find-log-native-") as temp:
        directory = Path(temp)
        source = directory / "find_log.cpp"
        source.write_text(text, encoding="utf-8")
        for config, flags in (("Debug", ["/MDd", "/D_DEBUG", "/Od"]),
                              ("Release", ["/MD", "/DNDEBUG", "/O2"])):
            if args.configuration not in (config, "Both"):
                continue
            binary = directory / ("find_log_" + config + ".exe")
            print("Actual Find log:", config, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                             *flags, "/I" + str(ROOT / "src"), str(source), "/Fe:" + str(binary)],
                            [str(binary)]):
                result = run(command, cwd=directory, env=env, timeout=35)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
