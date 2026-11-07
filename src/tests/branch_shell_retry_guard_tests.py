# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual Shell retry preparation and post-retry routing."""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    production = read("src/shellsup.cpp")
    action = production[production.index("void ShellAction(CFilesWindow* panel,"):]
    retry = action.index("incomplete shell context menu, retrying QueryContextMenu")
    begin = action.rfind("                if (panel->ContextMenu != NULL && h != NULL)", 0, retry)
    end = action.index("                    RemoveUselessSeparatorsFromMenu(h);", retry)
    setup = action[begin:end]
    fallback = block(action, "                else if (panel->IsBranchView() && useSelection && !onlyPanelMenu)")
    generated = read("src/tests/branch_shell_retry_guard_tests.cpp.in")
    generated = generated.replace("@@PREPARE_AND_GUARD@@", setup).replace("@@FALLBACK@@", fallback)
    if "@@" in generated:
        raise AssertionError("unexpanded production fixture")
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="branch-shell-retry-") as temp:
        directory = Path(temp)
        source = directory / "test.cpp"
        source.write_text(generated, encoding="utf-8")
        exe = directory / "test.exe"
        for configuration in configurations:
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Actual Shell retry guard:", configuration, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                             *flags, str(source), "/Fe:" + str(exe), "user32.lib"], [str(exe)]):
                result = run(command, cwd=directory, env=env, timeout=35)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
