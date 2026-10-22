# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual owned selection host service and SDK facade; verify legacy ABI prefixes."""
import argparse
import hashlib
import json
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

def abi_check():
    def classes(text):
        text = re.sub(r"//[^\n]*|/\*.*?\*/", "", text, flags=re.S)
        result = {}
        for match in re.finditer(r"class\s+(\w+Abstract)\s*\{", text):
            body = block(text, match.group(0))
            result[match.group(1)] = [re.sub(r"\s+", " ", x).strip() for x in re.findall(r"virtual\s+.*?=\s*0\s*;", body, re.S)]
        return result
    current = classes(read("src/plugins/shared/spl_gen.h"))
    golden = json.loads((ROOT / "src/tests/disk_selection_snapshot_abi.json").read_text(encoding="utf-8"))
    for name, methods in golden["interfaces"].items():
        if current.get(name, [])[:len(methods)] != methods:
            raise AssertionError(f"Released SDK virtual prefix changed: {golden['release']}:{name}")
    if hashlib.sha256(read("src/plugins/shared/spl_com.h").encode()).hexdigest() != golden["spl_com_sha256"]:
        raise AssertionError("Released CFileData/legacy shared data changed")
    print(f"Released SDK ABI golden: {len(golden['interfaces'])} interfaces preserved", flush=True)
    # CI normally has a shallow checkout without release tags. The checked-in
    # released manifest is mandatory; available local refs add a second check.
    for reference in ("HEAD", golden["release"]):
        available = run(["git", "rev-parse", "--verify", reference + "^{commit}"], cwd=ROOT,
                        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=10)
        if available.returncode:
            continue
        old = classes(read("src/plugins/shared/spl_gen.h", reference))
        for name, methods in old.items():
            if current.get(name, [])[:len(methods)] != methods:
                raise AssertionError(f"SDK virtual prefix changed: {reference}:{name}")
        print(f"SDK ABI prefix {reference}: {len(old)} interfaces preserved", flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--configuration", choices=("Debug", "Release"))
    args = parser.parse_args()
    abi_check()
    sdk=read("src/plugins/shared/spl_gen.h")
    host=read("src/zip.cpp")
    generated=read("src/tests/disk_selection_snapshot_tests.cpp.in")
    pieces={
        "CONSTANTS":"\n".join(x for x in sdk.splitlines() if x.startswith("#define SALAMANDER_SERVICE_DISK_SELECTION ") or x.startswith("#define SALAMANDER_DISK_SELECTION_VERSION_") or x.startswith("#define SALDISKSELECTION_")),
        "SDK":"\n".join(block(sdk,x,";") for x in ("struct CSalamanderServiceQuery", "struct CSalamanderServiceResult", "class CSalamanderPanelItemPathsAbstract", "struct CSalamanderDiskSelectionItem", "class CSalamanderDiskSelectionSnapshotAbstract", "class CSalamanderDiskSelectionAbstract")),
        "ENUM":block(read("src/plugins/pictview/thumbs.cpp"),"class CEnumFiles",";"),
        "HOST":"\n".join(block(host,x,";") for x in ("class CPanelItemPathsService", "class CDiskSelectionService")) + "\n" + block(host,"CSalamanderDiskSelectionAbstract* GetDiskSelectionService()"),
        "DEMO":block(read("src/plugins/demoplug/menu.cpp"), "    case MENUCMD_DOPFILES:"),
        "HEADER":str(ROOT/"src/plugins/shared/spl_diskselection.h").replace("\\","/"),
    }
    for key,value in pieces.items():generated=generated.replace("@@"+key+"@@",value)
    if "@@" in generated:raise AssertionError("unexpanded placeholder")
    environment=compiler_environment();compiler=shutil.which("cl",path=environment.get("PATH"))
    with tempfile.TemporaryDirectory(prefix="disk-selection-snapshot-") as temp:
        directory=Path(temp);source=directory/"test.cpp";source.write_text(generated,encoding="utf-8");executable=directory/"test.exe"
        for config,flags in (("Debug",["/MDd","/D_DEBUG","/Od"]),("Release",["/MD","/DNDEBUG","/O2"])):
            if args.configuration and config != args.configuration:
                continue
            print("Snapshot native configuration:",config,flush=True)
            for command in ([compiler,"/nologo","/std:c++17","/EHsc","/utf-8","/W4","/WX",*flags,str(source),"/Fe:"+str(executable),"user32.lib"],[str(executable)]):
                result=run(command,cwd=directory,env=environment,timeout=45)
                if result.returncode:return result.returncode
    return 0
if __name__=="__main__":raise SystemExit(main())
