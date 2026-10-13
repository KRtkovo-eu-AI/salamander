# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise actual disk SafeFile functions with owned Unicode and long-path fixtures."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
DEADLINE = time.monotonic() + 95


def run(command, **kwargs):
    timeout = min(kwargs.pop("timeout", 60), max(1, DEADLINE - time.monotonic()))
    return subprocess.run(command, timeout=timeout, **kwargs)


def compiler_environment():
    environment = os.environ.copy()
    if shutil.which("cl", path=environment.get("PATH")):
        return environment
    vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / (
        "Microsoft Visual Studio/Installer/vswhere.exe")
    result = run([str(vswhere), "-latest", "-products", "*", "-requires",
                  "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property",
                  "installationPath"], check=True, stdout=subprocess.PIPE, timeout=10)
    installation = result.stdout.decode("utf-8-sig").strip()
    if not installation:
        raise RuntimeError("MSVC C++ tools were not found")
    setup = Path(installation) / "Common7/Tools/VsDevCmd.bat"
    command = f'cmd /d /s /c "call "{setup}" -arch=x64 -host_arch=x64 >nul && set"'
    result = run(command, check=True, stdout=subprocess.PIPE, timeout=30)
    for line in result.stdout.decode(errors="replace").splitlines():
        if "=" in line and not line.startswith("="):
            key, value = line.split("=", 1)
            environment[key.upper()] = value
    return environment


def read(path, revision=None):
    if revision:
        return run(["git", "show", f"{revision}:{path}"], cwd=ROOT, check=True,
                   stdout=subprocess.PIPE, timeout=10).stdout.decode("utf-8-sig")
    return (ROOT / path).read_text(encoding="utf-8-sig")


def block(source, marker, trailing=""):
    start = source.index(marker)
    opening = source.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + trailing


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref", help="test production SafeFile functions from an earlier git revision")
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    production = read("src/safefile.cpp", args.source_ref)
    wide = read("src/common/widepath.cpp", args.source_ref)
    common = read("src/plugins/shared/spl_com.h")
    sdk = read("src/plugins/shared/spl_file.h")
    general = read("src/plugins/shared/spl_gen.h")
    concrete = block(read("src/salamand.h"), "class CSalamanderSafeFile :", ";")
    concrete = concrete.replace(" : public CSalamanderSafeFileAbstract", "")
    import re
    constants = "\n".join(line for line in general.splitlines()
                          if re.match(r"#define (DIALOG_|BUTTONS_)", line))
    ids = sorted(set(re.findall(r"\bIDS_\w+", production)))
    production = re.sub(r'^#include.*$', '', production, flags=re.M)
    pieces = {
        "SDK": block(common, "struct CQuadWord", ";") + "\n" +
               sdk[sdk.index("struct SAFE_FILE"):sdk.index("class CSalamanderSafeFileAbstract\n{")] +
               "\n" + constants + "\n" + block(general, "inline BOOL ButtonsContainsSkip("),
        "DECLARATIONS": concrete,
        "IDS": "enum { " + ", ".join(ids) + " };",
        "WIDE_HELPERS": "\n".join(block(wide, marker) for marker in (
            "BOOL SalIsExtendedLengthPathW(", "std::wstring SalPathAddExtendedPrefixW(",
            "BOOL IsValidPathUtf8Text(", "std::wstring SalMultiByteToWidePath(")),
        "GET_SIZE": block(read("src/salamdr5.cpp"), "BOOL SalGetFileSize("),
        "CREATE_DIRECTORY": block(read("src/salamdr3.cpp"), "BOOL SalCreateDirectoryExW("),
        "SAFE_FILE": production,
    }
    generated = read("src/tests/safe_file_paths_tests.cpp.in")
    for name, value in pieces.items():
        generated = generated.replace("@@" + name + "@@", value)
    if "@@" in generated:
        raise AssertionError("unexpanded fixture placeholder")
    environment = compiler_environment()
    compiler = shutil.which("cl", path=environment.get("PATH"))
    configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="safe-file-paths-") as directory:
        directory = Path(directory)
        source = directory / "test.cpp"
        source.write_text(generated, encoding="utf-8")
        executable = directory / "test.exe"
        for configuration in configurations:
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Actual SafeFile paths:", configuration, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", "/D_CRT_SECURE_NO_WARNINGS",
                             *flags, str(source), "/Fe:" + str(executable)], [str(executable)]):
                result = run(command, cwd=directory, env=environment, timeout=45)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
