# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual owned selection host service and SDK facade; verify legacy ABI prefixes."""
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
    sdk=read("src/plugins/shared/spl_gen.h")
    host=read("src/zip.cpp")
    generated=read("src/tests/checksum_selection_paths_tests.cpp.in")
    pieces={
        "CONSTANTS":"\n".join(x for x in sdk.splitlines() if x.startswith("#define SALAMANDER_SERVICE_DISK_SELECTION ") or x.startswith("#define SALAMANDER_DISK_SELECTION_VERSION_") or x.startswith("#define SALDISKSELECTION_")),
        "SDK":"\n".join(block(sdk,x,";") for x in ("struct CSalamanderServiceQuery", "struct CSalamanderServiceResult", "class CSalamanderPanelItemPathsAbstract", "struct CSalamanderDiskSelectionItem", "class CSalamanderDiskSelectionSnapshotAbstract", "class CSalamanderDiskSelectionAbstract")),
        "HOST":"\n".join(block(host,x,";") for x in ("class CPanelItemPathsService", "class CDiskSelectionService")),
        "HEADER":str(ROOT/"src/plugins/shared/spl_diskselection.h").replace("\\","/"),
    }
    dialog=read("src/plugins/checksum/dialogs.cpp")
    headers=read("src/plugins/checksum/dialogs.h")
    misc=read("src/plugins/checksum/misc.cpp")
    wrappers=read("src/plugins/checksum/wrappers.cpp")
    def function(marker, source=dialog):
        value=block(source,marker)
        end=source.index(value)+len(value)
        if source[end:].lstrip().startswith("catch"):
            value+="\n"+block(source[end:].lstrip(),"catch")
        return value
    selected = [
        "BOOL CSFVMD5Dialog::AddFileListItem(", "BOOL CCalculateDialog::AddDir(",
        "BOOL CCalculateDialog::GetFileList()", "unsigned CCalculateThread::Body()",
        "char* CVerifyDialog::LoadFile(", "BOOL CVerifyDialog::LoadSourceFile()",
        "BOOL OpenCalculateDialog(", "BOOL OpenVerifyDialog(", "void CCalculateDialog::SaveHashes()"]
    actual = "\n".join(function(x) for x in selected)
    pieces.update({
        "QUAD":block(read("src/plugins/shared/spl_com.h"),"struct CQuadWord",";"),
        "IDS":"enum { "+", ".join(sorted(set(re.findall(r"\bIDS_\w+",actual+misc))))+" };",
        "FILE_LIST":block(headers,"class FILELISTITEM",";")+"\n"+block(headers,"typedef struct _SEEDFILEINFO"," SEEDFILEINFO;")+"\ntypedef TIndirectArray<SEEDFILEINFO> TSeedFileList;",
        "FILE_INFO":block(headers,"struct FILEINFO",";"),
        "CONVERSIONS":block(read("src/plugins/checksum/precomp.h"),"static std::wstring PluginPathAddExtendedPrefixW("),
        "PATHS":str(ROOT/"src/plugins/checksum/checksum_paths.h").replace("\\","/"),
        "CALC_WORKER":block(dialog,"class CCalculateThread :",";"),
        "THREADS":block(dialog,"class CCalculateDialogThread :",";")+"\n"+block(dialog,"unsigned CCalculateDialogThread::Body()")+"\n"+block(dialog,"class CVerifyDialogThread :",";")+"\n"+block(dialog,"unsigned CVerifyDialogThread::Body()"),
        "FUNCTIONS":actual,
        "PARSER":block(misc,"void GetLastWord(")+"\n"+block(misc,"BYTE hex(")+"\n"+block(wrappers,"bool CCRCAlgo::ParseDigest("),
        "IO":block(misc,"BOOL SafeReadFile(")+"\n"+function("BOOL SafeOpenCreateFile(",misc),
    })
    for key,value in pieces.items():generated=generated.replace("@@"+key+"@@",value)
    if "@@" in generated:raise AssertionError("unexpanded placeholder")
    environment=compiler_environment();compiler=shutil.which("cl",path=environment.get("PATH"))
    with tempfile.TemporaryDirectory(prefix="checksum-selection-paths-") as temp:
        directory=Path(temp);source=directory/"test.cpp";source.write_text(generated,encoding="utf-8");executable=directory/"test.exe"
        for config,flags in (("Debug",["/MDd","/D_DEBUG","/Od"]),("Release",["/MD","/DNDEBUG","/O2"])):
            print("Checksum actual consumers:",config,flush=True)
            for command in ([compiler,"/nologo","/std:c++17","/EHsc","/utf-8","/W4","/WX","/D_CRT_SECURE_NO_WARNINGS","/wd4100","/wd4505",*flags,str(source),"/Fe:"+str(executable),"user32.lib"],[str(executable)]):
                result=run(command,cwd=directory,env=environment,timeout=45)
                if result.returncode:return result.returncode
    return 0
if __name__=="__main__":raise SystemExit(main())
