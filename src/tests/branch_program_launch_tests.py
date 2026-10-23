# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Build actual salopen source and verify its Windows program broker on owned files."""
import argparse
from pathlib import Path
import shutil
import tempfile
from branch_panel_getters_tests import ROOT, compiler_environment, run, block

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    production = (ROOT / "src/fileswn2.cpp").read_text(encoding="utf-8-sig")
    template = (ROOT / "src/tests/branch_program_launch_tests.cpp.in").read_text(encoding="utf-8")
    pieces = {"PROGRAM_HEADER": str(ROOT / "src/branchprogram.h").replace(chr(92), "/"),
              "FILEACTION_HEADER": str(ROOT / "src/fileactionpath.h").replace(chr(92), "/"),
              "REGISTRY_ENUM": block(production, "enum class BranchLongPathSetting", ";"),
              "REGISTRY_FUNCTION": block(production, "static BranchLongPathSetting ReadBranchLongPathSetting()")}
    for key, value in pieces.items(): template = template.replace("@@" + key + "@@", value)
    if "@@" in template: raise AssertionError("Unexpanded fixture")
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    resource_compiler = shutil.which("rc", path=env.get("PATH"))
    configs = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="branch-program-") as temporary:
        directory = Path(temporary)
        source = directory / "test.cpp"
        source.write_text(template, encoding="utf-8")
        resource = directory / "broker.rc"
        resource.write_text('#include <windows.h>\n#include "' + str(ROOT / "src/salopen/salopen.rc").replace(chr(92), "/") + '"\n', encoding="utf-8")
        for config in configs:
            output = directory / config
            output.mkdir()
            broker = output / "salopen.exe"
            executable = output / "test.exe"
            flags = ["/MDd", "/D_DEBUG", "/Od"] if config == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            common = [compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX"]
            commands = [
                [resource_compiler, "/nologo", "/fo" + str(output / "broker.res"), str(resource)],
                [*[x for x in common if x != "/W4"], "/W3", *[x for x in flags if not x.startswith("/MD")], "/MT", "/GS-", "/Zl", "/DLSTRFIX_WITHOUT_RTL", "/DWINVER=0x0601", "/D_WIN32_WINNT=0x0601", "/D_WIN32_IE=0x0800", "/I" + str(ROOT / "src/common"), "/I" + str(ROOT / "src/salopen"), str(ROOT / "src/salopen/salopen.cpp"), str(ROOT / "src/common/lstrfix.cpp"), "/Fe:" + str(broker), "/link", "/NODEFAULTLIB", "/SUBSYSTEM:WINDOWS", "/ENTRY:WinMainCRTStartup", "kernel32.lib", "user32.lib", "gdi32.lib", "advapi32.lib", "shell32.lib", "ole32.lib", "oleaut32.lib", "uuid.lib", str(output / "broker.res"), "/MANIFEST:NO"],
                [*common, *flags, str(source), "/Fe:" + str(executable), "/link", "ole32.lib", "uuid.lib", "user32.lib"],
                [str(executable), str(broker), str(output)]]
            print("Actual program broker:", config, flush=True)
            for command in commands:
                result = run(command, cwd=(ROOT / "src/salopen") if command[0] == resource_compiler else output, env=env, timeout=45)
                if result.returncode: return result.returncode
    return 0

if __name__ == "__main__": raise SystemExit(main())
