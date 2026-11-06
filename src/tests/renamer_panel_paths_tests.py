# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise actual Renamer intake, source constructors and target-parent calculation without UI."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
DEADLINE = time.monotonic() + 95

# Toolchain discovery is local so this test does not depend on other feature tests.
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
    result = run(command, check=True, stdout=subprocess.PIPE, timeout=15)
    for line in result.stdout.decode(errors="replace").splitlines():
        if "=" in line and not line.startswith("="):
            key, value = line.split("=", 1)
            environment[key.upper()] = value
    return environment


def read(path, revision=None):
    if revision:
        return subprocess.check_output(["git", "show", f"{revision}:{path}"], cwd=ROOT,
                                       timeout=10).decode("utf-8-sig")
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
    parser.add_argument("--source-ref", help="exercise previous Renamer intake and constructors")
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    dialog = read("src/plugins/renamer/rendlg.cpp", args.source_ref)
    operations = read("src/plugins/renamer/rendlg2.cpp", args.source_ref)
    engine = read("src/plugins/renamer/crenamer.cpp", args.source_ref)
    engine_header = read("src/plugins/renamer/crenamer.h", args.source_ref)
    sdk = read("src/plugins/shared/spl_gen.h")
    host = read("src/zip.cpp")
    new_api = "SelectionValid" in dialog
    ctor_start = engine.index("CSourceFile::CSourceFile(")
    ctor_end = engine.index("// CRenamerOptions", ctor_start)
    rename = block(engine, "int CRenamer::Rename(")
    rename_decl = rename[:rename.index("{")].replace("CRenamer::", "") + ";"
    manual = block(operations, "int CRenamerDialog::GetManualModeNewName(")
    manual_decl = manual[:manual.index("{")].replace("CRenamerDialog::", "") + ";"
    pieces = {
        "SDK": block(sdk, "struct CSalamanderServiceQuery", ";") + "\n" +
               block(sdk, "struct CSalamanderServiceResult", ";") + "\n" +
               block(sdk, "class CSalamanderPanelItemPathsAbstract", ";") + "\n" +
               sdk[sdk.index("#define SALAMANDER_SERVICE_DISK_SELECTION"):sdk.index("class CSalamanderDiskSelectionAbstract")] + block(sdk, "class CSalamanderDiskSelectionAbstract", ";"),
        "CONVERSIONS": "\n".join(block(read("src/plugins/filecomp/precomp.h"), marker) for marker in
            ("static std::wstring PluginMultiByteToWidePath(", "static std::string PluginWideToMultiBytePath(")),
        "SERVICE": block(host, "class CPanelItemPathsService", ";"),
        "FACADE": '#include "' + str(ROOT / "src/plugins/shared/spl_diskselection.h").replace("\\", "/") + '"',
        "DISK_SERVICE": block(host, "class CDiskSelectionService", ";"),
        "PATH_HELPERS": ('#include "' + str(ROOT / "src/plugins/renamer/renamer_paths.h").replace('\\', '/') + '"') if new_api else '',
        "SOURCE_DECL": engine_header[engine_header.index("struct CSourceFile"):engine_header.index("enum CChangeCase")],
        "SOURCE_IMPL": engine[ctor_start:ctor_end],
        "ENUMS": block(engine_header, "enum CChangeCase", ";") + "\n" + block(engine_header, "enum CRenameSpec", ";") + "\n" + block(engine_header, "struct CExecuteNewNameParam", ";"),
        "INTAKE": block(dialog, "void CRenamerDialog::LoadSelection()"),
        "RENAME_DECL": rename_decl,
        "RENAME": rename,
        "MANUAL_DECL": manual_decl,
        "MANUAL": manual,
        "NEW_API": "1" if new_api else "0",
    }
    generated = read("src/tests/renamer_panel_paths_tests.cpp.in")
    for name, value in pieces.items():
        generated = generated.replace("@@" + name + "@@", value)
    if "@@" in generated:
        raise AssertionError("unexpanded fixture placeholder")
    environment = compiler_environment()
    compiler = shutil.which("cl", path=environment.get("PATH"))
    with tempfile.TemporaryDirectory(prefix="renamer-panel-paths-") as directory:
        directory = Path(directory)
        source = directory / "test.cpp"
        source.write_text(generated, encoding="utf-8")
        executable = directory / "test.exe"
        configurations = ("Debug", "Release") if args.configuration == "Both" else (args.configuration,)
        for configuration in configurations:
            flags = ["/MDd", "/D_DEBUG", "/D_CRTDBG_MAP_ALLOC", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Renamer native configuration:", configuration, flush=True)
            for command in ([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX",
                             "/wd4100", "/wd4505", "/wd4244", "/wd4267", "/D_CRT_SECURE_NO_WARNINGS", *flags,
                             str(source), "/Fe:" + str(executable), "user32.lib"], [str(executable)]):
                result = run(command, cwd=directory, env=environment, timeout=45)
                if result.returncode:
                    return result.returncode
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
