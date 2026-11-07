# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Build actual EXIF DLL and test Unicode thumbnail replacement and owned helpers."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import time
import tempfile
from disk_selection_snapshot_tests import ROOT, DEADLINE, block, compiler_environment, run


def build_bounded(command, directory, environment):
    process = subprocess.Popen(command, cwd=directory, env=environment,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        output, _ = process.communicate(timeout=min(40, max(1, DEADLINE-time.monotonic())))
        print(output.decode(errors="replace"), end="", flush=True)
        return process.returncode
    except subprocess.TimeoutExpired:
        subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                       timeout=5, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        process.wait(timeout=5)
        raise


def main():
    parser=argparse.ArgumentParser();parser.add_argument("--build-root",type=Path);parser.add_argument("--configuration",choices=("Debug","Release","Both"),default="Both");args=parser.parse_args()
    thumbs=(ROOT/"src/plugins/pictview/thumbs.cpp").read_text(encoding="utf-8-sig")
    precomp=(ROOT/"src/plugins/pictview/precomp.h").read_text(encoding="utf-8-sig")
    code=(ROOT/"src/tests/pictview_thumbnail_paths_tests.cpp.in").read_text(encoding="utf-8-sig")
    for name,source,marker,tail in (("WIDE",precomp,"static std::wstring PluginPathAddExtendedPrefixW",""),("TEMP",thumbs,"class CThumbnailTempFile",";"),("ATTRS",thumbs,"class CThumbnailFileAttributes",";"),("FORMAT",thumbs,"static std::string FormatThumbnailText",""),("WRITE",thumbs,"DWORD WINAPI MyMemWriteFunc","")):
        code=code.replace("@@"+name+"@@",block(source,marker,tail))
    env=compiler_environment();compiler=shutil.which("cl",path=env.get("PATH"));msbuild=shutil.which("MSBuild",path=env.get("PATH"))
    if not msbuild:
        msbuild=str(Path(env["VSINSTALLDIR"])/"MSBuild/Current/Bin/MSBuild.exe")
    toolsets=list((Path(env["VSINSTALLDIR"])/"MSBuild/Microsoft/VC").glob("v*/Platforms/x64/PlatformToolsets/v*/Toolset.props"))
    toolsets=[p.parent.name for p in toolsets if p.parent.name[1:].isdigit()]
    if not toolsets:raise RuntimeError("Installed MSVC platform toolset not found")
    toolset=max(toolsets,key=lambda name:int(name[1:]))
    sdk=env.get("WINDOWSSDKVERSION", "").rstrip("\\/")
    if not sdk:raise RuntimeError("Installed Windows SDK version not found")
    print(f"EXIF fixture toolchain: {toolset}, Windows SDK {sdk}",flush=True)
    with tempfile.TemporaryDirectory(prefix="pictview-thumbnail-", dir=ROOT.parent) as temporary:
        tmp=Path(temporary);source=tmp/"test.cpp";source.write_text(code,encoding="utf-8");exe=tmp/"test.exe"
        targets=tmp/"warnings.targets";targets.write_text('<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003"><ItemDefinitionGroup><ClCompile><TreatWarningAsError>true</TreatWarningAsError></ClCompile></ItemDefinitionGroup></Project>',encoding="utf-8")
        build=args.build_root or tmp/"build"
        for config,flags in (("Debug",["/MDd","/D_DEBUG","/Od"]),("Release",["/MD","/DNDEBUG","/O2"])):
            if args.configuration != "Both" and config != args.configuration:continue
            dll=build/f"salamander/{config}_x64/plugins/pictview/exif.dll"
            if not args.build_root:
                project=ROOT/"src/plugins/pictview/vcxproj/exif/exif.vcxproj"
                command=[msbuild,str(project),"/t:_PrepareForBuild;ResolveReferences;PrepareForBuild;InitializeBuildStatus;BuildGenerateSources;BuildCompile;BuildLink","/m:1","/nr:false","/nologo","/v:minimal",f"/p:Configuration={config}","/p:Platform=x64",f"/p:PlatformToolset={toolset}",f"/p:WindowsTargetPlatformVersion={sdk}",f"/p:OpenSalamanderReleaseClean={str(config=='Release').lower()}",f"/p:OPENSAL_BUILD_DIR={build}\\","/p:BuildProjectReferences=false",f"/p:ForceImportAfterCppTargets={targets}"]
                result=build_bounded(command,tmp,env)
                if result:return result
            if not dll.is_file():raise FileNotFoundError(dll)
            print("PictView actual EXIF configuration:",config,flush=True)
            for command in ([compiler,"/nologo","/std:c++17","/EHsc","/utf-8","/W4","/WX",*flags,str(source),"/Fe:"+str(exe),"ole32.lib","windowscodecs.lib"],[str(exe),str(dll),str(tmp/config)]):
                result=build_bounded(command,tmp,env)
                if result:return result
    return 0
if __name__=="__main__":raise SystemExit(main())
