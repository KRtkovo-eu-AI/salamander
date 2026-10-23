# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise the actual archive selection callback in Debug and Release, without UI or disk mutation."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
DEADLINE = time.monotonic() + 90

# Bounded MSVC discovery/build follows the other actual-function native fixtures.
def run(command, **kwargs):
    timeout = min(kwargs.pop("timeout", 45), max(1, DEADLINE - time.monotonic()))
    check = kwargs.pop("check", False)
    process = subprocess.Popen(command, **kwargs)
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5)
        process.kill()
        process.communicate(timeout=5)
        raise
    result = subprocess.CompletedProcess(command, process.returncode, stdout, stderr)
    if check:
        result.check_returncode()
    return result


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
    parser.add_argument("--source-ref", help="exercise the callback/state/converter at a previous Git revision")
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    source = read("src/fileswn7.cpp", args.source_ref)
    header = read("src/fileswnd.h", args.source_ref)
    converter = read("src/common/widepath.cpp", args.source_ref)
    sdk = read("src/plugins/shared/spl_com.h", args.source_ref)
    pieces = {
        "CONSTANTS": "\n".join(re.findall(r"^#define SALENUM_(?:SUCCESS|ERROR|CANCEL)\b[^\n]*", sdk, re.M)),
        "STATE": block(header, "struct CPanelTmpEnumData\n", ";"),
        "STATE_METHODS": "\n".join(block(source, marker) for marker in (
            "CPanelTmpEnumData::CPanelTmpEnumData()", "CPanelTmpEnumData::~CPanelTmpEnumData()",
            "void CPanelTmpEnumData::Reset()")),
        "CONVERTER": block(converter, "std::string SalWideToMultiBytePath("),
        "CALLBACK": block(source, "const char* WINAPI PanelEnumDiskSelection("),
    }
    generated = read("src/tests/branch_archive_selection_tests.cpp.in")
    for name, value in pieces.items():
        generated = generated.replace("@@" + name + "@@", value)
    if "@@" in generated:
        raise AssertionError("unexpanded archive fixture marker")
    environment = compiler_environment()
    compiler = shutil.which("cl", path=environment.get("PATH"))
    configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix="branch-archive-selection-") as directory:
        directory = Path(directory)
        source_file = directory / "test.cpp"
        executable = directory / "test.exe"
        source_file.write_text(generated, encoding="utf-8")
        for configuration in configurations:
            print("Branch archive actual callback native configuration:", configuration, flush=True)
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                             "/wd4100", "/wd4505", *flags, str(source_file), "/Fe:" + str(executable)],
                            [str(executable)]):
                result = run(command, cwd=directory, env=environment)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
