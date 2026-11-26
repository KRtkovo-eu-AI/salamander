# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-07 | Compile the F7 command with its real Windows wrappers. | Verify or preserve the F7 local creation contract. | | GPT-6.1 Sol (Codex) |
"""Exercise the actual F7 command and host/SDK create-directory wrappers."""
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
    panel = read("src/fileswn5.cpp")
    wide = read("src/common/widepath.cpp")
    pieces = {
        "WIDE_WRAPPER": "\n".join((
            block(wide, "BOOL SalIsExtendedLengthPathW("),
            block(wide, "std::wstring SalPathAddExtendedPrefixW("),
            block(read("src/salamdr3.cpp"), "BOOL SalCreateDirectoryExW("),
        )),
        "F7_COMMAND": "\n".join((
            block(panel, "BOOL FileNameInvalidForManualCreate("),
            block(panel, "struct LocalDirectoryCreateResult", ";"),
            block(panel, "static LocalDirectoryCreateResult CreateLocalDirectoryLeaf("),
        )),
    }
    for name, source in pieces.items():
        generated = generated.replace("@@" + name + "@@", source)
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
            print("Actual F7, host and SDK create-directory commands:", configuration, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                             *flags, str(source), "/Fe:" + str(exe), "advapi32.lib"], [str(exe)]):
                result = run(command, cwd=directory, env=env, timeout=35)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
