# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise actual Undelete restore and snapshot intake; stub EFS only, never encrypt."""
import argparse
import re
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
    result = run(command, check=True, stdout=subprocess.PIPE, timeout=30)
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
    parser=argparse.ArgumentParser();parser.add_argument("--configuration",choices=("Debug","Release","Both"),default="Both");args=parser.parse_args()
    sdk=read("src/plugins/shared/spl_gen.h")
    host=read("src/zip.cpp")
    generated=read("src/tests/undelete_selection_paths_tests.cpp.in")
    restore=read("src/plugins/undelete/restore.cpp")
    entry=read("src/plugins/undelete/undelete.cpp")
    actual=restore[restore.index("#define COPY_BUFFER_SIZE"):]
    actual_entry=block(entry,"static BOOL ExecuteRestoreEncrypted(")+"\n"+block(entry[entry.index("static BOOL ExecuteRestoreEncrypted("):],"catch (const std::bad_alloc&)")
    assert "return ExecuteRestoreEncrypted(parent);" in entry
    assert "GetPanelFocusedItem" not in actual and "GetPanelSelectedItem" not in actual and "GetPanelPath" not in actual
    dialogs=read("src/plugins/undelete/dialogs.cpp")
    assert "UndeletePaths::FocusedImagePath(SalamanderGeneral, Panel)" in dialogs
    assert "UndeletePaths::LegacyImagePath(image, Volume, sizeof(Volume))" in dialogs
    assert "SetDlgItemTextW(HWindow, IDC_EDIT_IMAGE, image.c_str())" in dialogs
    pieces={
        "CONSTANTS":"\n".join(x for x in sdk.splitlines() if x.startswith("#define SALAMANDER_SERVICE_DISK_SELECTION ") or x.startswith("#define SALAMANDER_DISK_SELECTION_VERSION_") or x.startswith("#define SALDISKSELECTION_")),
        "SDK":"\n".join(block(sdk,x,";") for x in ("struct CSalamanderServiceQuery", "struct CSalamanderServiceResult", "class CSalamanderPanelItemPathsAbstract", "struct CSalamanderDiskSelectionItem", "class CSalamanderDiskSelectionSnapshotAbstract", "class CSalamanderDiskSelectionAbstract")),
        "HOST":"\n".join(block(host,x,";") for x in ("class CPanelItemPathsService", "class CDiskSelectionService")),
        "QUAD":block(read("src/plugins/shared/spl_com.h"),"struct CQuadWord",";"),
        "HEADER":str(ROOT/"src/plugins/shared/spl_diskselection.h").replace("\\","/"),
        "PATHS":str(ROOT/"src/plugins/undelete/undelete_paths.h").replace("\\","/"),
        "IDS":"enum { "+", ".join(sorted(set(re.findall(r"\bIDS_\w+",actual+actual_entry))))+" };",
        "RESTORE":actual,
        "ENTRY":actual_entry,
    }
    for key,value in pieces.items():generated=generated.replace("@@"+key+"@@",value)
    if "@@" in generated:raise AssertionError("unexpanded placeholder")
    environment=compiler_environment();compiler=shutil.which("cl",path=environment.get("PATH"))
    with tempfile.TemporaryDirectory(prefix="undelete-selection-paths-") as temp:
        directory=Path(temp);source=directory/"test.cpp";source.write_text(generated,encoding="utf-8");executable=directory/"test.exe"
        for config,flags in (("Debug",["/MDd","/D_DEBUG","/Od"]),("Release",["/MD","/DNDEBUG","/O2"])):
            if args.configuration!="Both" and config!=args.configuration:continue
            print("Undelete actual restore/owned selection (EFS stub):",config,flush=True)
            for command in ([compiler,"/nologo","/std:c++17","/EHsc","/utf-8","/W4","/WX","/D_CRT_SECURE_NO_WARNINGS","/wd4100","/wd4505",*flags,str(source),"/Fe:"+str(executable),"user32.lib"],[str(executable)]):
                result=run(command,cwd=directory,env=environment,timeout=45)
                if result.returncode:return result.returncode
    return 0
if __name__=="__main__":raise SystemExit(main())
