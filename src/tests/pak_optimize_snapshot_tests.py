# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Execute actual PAK Optimize command, SDK snapshot and archive opening methods."""
import argparse
from pathlib import Path
import shutil
import tempfile
from disk_selection_snapshot_tests import ROOT,block,compiler_environment,read,run


def main():
 parser=argparse.ArgumentParser();parser.add_argument("--configuration",choices=("Debug","Release","Both"),default="Both");args=parser.parse_args()
 sdk=read("src/plugins/shared/spl_gen.h");host=read("src/zip.cpp");menu=read("src/plugins/pak/spl/pak2.cpp");engine=read("src/plugins/pak/dll/pak_dll.cpp")
 # Reuse only mock panel discovery; snapshot and consumer logic is production code.
 prefix=read("src/tests/disk_selection_snapshot_tests.cpp.in").split("static void HostTests(){")[0]
 # S05 added a DemoPlug-only command shim to this shared prefix. It is not
 # part of the PAK fixture; retain its snapshot setup without unused commands.
 demo_start=prefix.index("// Compile the actual DemoPlug case,")
 prefix=prefix[:demo_start]+prefix[prefix.index("static void Reset()",demo_start):]
 prefix=prefix.replace("struct CQuadWord { unsigned __int64 Value=0; };","struct CQuadWord { unsigned __int64 Value=0; CQuadWord()=default; CQuadWord(unsigned a,unsigned b):Value((static_cast<unsigned __int64>(b)<<32)|a){} CQuadWord(int a,int b):CQuadWord(static_cast<unsigned>(a),static_cast<unsigned>(b)){} CQuadWord(unsigned a,int b):CQuadWord(a,static_cast<unsigned>(b)){} };")
 prefix=prefix.replace("struct CSalamanderGeneralAbstract {",'''extern std::vector<std::string> Notified;
struct CSalamanderGeneralAbstract {
 bool ArchivePanel=false;std::string ArchivePath;
 void GetErrorText(DWORD,char* p,int n){strcpy_s(p,n,"error");}
 void ShowMessageBox(const char*,const char*,int){}const char* SalPathFindFileName(const char* p){auto q=strrchr(p,'\\\\');return q?q+1:p;}
 void PostChangeOnPathNotification(const char* p,BOOL){Notified.push_back(p);}
''')
 prefix=prefix.replace("int* type,void*){++RootCalls;",'''int* type,char** archive){++RootCalls;if(ArchivePanel){strcpy_s(path,capacity,ArchivePath.c_str());if(archive)*archive=strstr(path,".pak")+4;return TRUE;}if(archive)*archive=nullptr;''')
 prefix=prefix.replace("*type=p->Disk?PATH_TYPE_WINDOWS:-1;","if(type)*type=p->Disk?PATH_TYPE_WINDOWS:-1;")
 pieces={"CONSTANTS":"\n".join(x for x in sdk.splitlines() if x.startswith("#define SALAMANDER_SERVICE_DISK_SELECTION ") or x.startswith("#define SALAMANDER_DISK_SELECTION_VERSION_") or x.startswith("#define SALDISKSELECTION_")),
 "SDK":"\n".join(block(sdk,x,";") for x in ("struct CSalamanderServiceQuery","struct CSalamanderServiceResult","class CSalamanderPanelItemPathsAbstract","struct CSalamanderDiskSelectionItem","class CSalamanderDiskSelectionSnapshotAbstract","class CSalamanderDiskSelectionAbstract")),
 "HOST":"\n".join(block(host,x,";") for x in ("class CPanelItemPathsService","class CDiskSelectionService"))+"\n"+block(host,"CSalamanderDiskSelectionAbstract* GetDiskSelectionService()"),"HEADER":str(ROOT/"src/plugins/shared/spl_diskselection.h").replace("\\","/"),"ENUM":"","DEMO":block(read("src/plugins/demoplug/menu.cpp"),"    case MENUCMD_DOPFILES:"),
 "TITLE":block(menu,"static std::string OptimizeProgressTitle"),"MENU":block(menu,"BOOL CPluginInterfaceForMenuExt::ExecuteMenuItem"),"OPENHELPER":block(engine,"static HANDLE CreateFileLongPath"),"OPEN":block(engine,"BOOL CPakIface::OpenPak"),"CLOSE":block(engine,"BOOL CPakIface::ClosePak")}
 code=prefix+read("src/tests/pak_optimize_snapshot_tests.cpp.in")
 for key,value in pieces.items():code=code.replace("@@"+key+"@@",value)
 if "@@" in code:raise AssertionError("Unexpanded fixture marker")
 env=compiler_environment();compiler=shutil.which("cl",path=env.get("PATH"))
 with tempfile.TemporaryDirectory(prefix="pak-snapshot-",dir=ROOT.parent) as temp:
  directory=Path(temp);source=directory/"test.cpp";source.write_text(code,encoding="utf-8");exe=directory/"test.exe"
  for config,flags in (("Debug",["/MDd","/D_DEBUG","/Od"]),("Release",["/MD","/DNDEBUG","/O2"])):
   if args.configuration!="Both" and config!=args.configuration:continue
   print("PAK Optimize configuration:",config,flush=True)
   for command in ([compiler,"/nologo","/std:c++17","/EHsc","/utf-8","/W4","/WX",*flags,str(source),"/Fe:"+str(exe),"user32.lib"],[str(exe),str(directory/config)]):
    result=run(command,cwd=directory,env=env,timeout=40)
    if result.returncode:return result.returncode
 return 0
if __name__=="__main__":raise SystemExit(main())
