# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual host/SDK create-directory wrappers and exercise their wide I/O."""
import argparse
from pathlib import Path
import shutil
import tempfile
from safe_file_paths_tests import block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    host = block(read("src/worker.cpp"), "BOOL SalCreateDirectoryEx(const char* name, DWORD* err)")
    sdk = block(read("src/zip.cpp"), "BOOL CSalamanderGeneral::SalCreateDirectoryEx(")
    generated = read("src/tests/create_directory_wrapper_tests.cpp.in")
    generated = generated.replace("@@WORKER_PATH_HEADER@@", (Path(__file__).resolve().parents[1] / "workerpath.h").as_posix())
    generated = generated.replace("@@HOST_WRAPPER@@", host).replace("@@SDK_WRAPPER@@", sdk)
    if "@@" in generated:
        raise AssertionError("unexpanded production fixture")
    env = compiler_environment()
    compiler = shutil.which("cl", path=env.get("PATH"))
    configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="create-directory-wrapper-") as temp:
        directory = Path(temp)
        source = directory / "test.cpp"
        source.write_text(generated, encoding="utf-8")
        exe = directory / "test.exe"
        for configuration in configurations:
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Actual host + SDK create-directory wrappers:", configuration, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                             *flags, str(source), "/Fe:" + str(exe)], [str(exe)]):
                result = run(command, cwd=directory, env=env, timeout=35)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
