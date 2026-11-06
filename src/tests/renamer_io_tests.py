# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Run actual RenamerIO Win32 wrappers against disposable Unicode and long paths."""
import argparse
from pathlib import Path
import shutil
import tempfile
from renamer_panel_paths_tests import ROOT, block, compiler_environment, read, run

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    sdk = read("src/plugins/shared/spl_gen.h")
    generated = read("src/tests/renamer_io_tests.cpp.in")
    generated = generated.replace("@@SDK@@", "\n".join(block(sdk, marker, ";") for marker in (
        "struct CSalamanderServiceQuery", "struct CSalamanderServiceResult", "class CSalamanderPanelItemPathsAbstract")))
    generated = generated.replace("@@IO_HEADER@@", str(ROOT / "src/plugins/renamer/renamer_io.h").replace("\\", "/"))
    generated = generated.replace("@@HAS_SAME_ROOT@@", "1" if "inline BOOL SameRoot(" in read("src/plugins/renamer/renamer_io.h") else "0")
    if "@@" in generated:
        raise AssertionError("unexpanded I/O fixture marker")
    environment = compiler_environment()
    compiler = shutil.which("cl", path=environment.get("PATH"))
    configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="renamer-io-build-") as directory:
        directory = Path(directory)
        source = directory / "test.cpp"
        executable = directory / "test.exe"
        source.write_text(generated, encoding="utf-8")
        for configuration in configurations:
            print("Renamer I/O native configuration:", configuration, flush=True)
            flags = ["/MDd", "/D_DEBUG", "/D_CRTDBG_MAP_ALLOC", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", "/wd4100",
                             *flags, str(source), "/Fe:" + str(executable)], [str(executable)]):
                result = run(command, cwd=directory, env=environment, timeout=45)
                if result.returncode:
                    return result.returncode
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
