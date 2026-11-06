# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile actual Automation repository resolvers/manifest intake with real file I/O.

Expansion and SalPathAppend are explicit recording facades. Registry, GUI and
runtime processes are not used. Production parsing/conversion/file reads are real.
"""
import argparse
from contextlib import contextmanager
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "tests"))
from disk_selection_snapshot_tests import ROOT, block, compiler_environment, read, run

@contextmanager
def fixture_directory():
    folder = Path(tempfile.mkdtemp(prefix='automation-repository-paths-'))
    try:
        yield folder
    finally:
        assert folder.resolve().parent == Path(tempfile.gettempdir()).resolve()
        assert folder.name.startswith('automation-repository-paths-')
        shutil.rmtree('\\\\?\\' + str(folder.resolve()))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug','Release','Both'), default='Both')
    args = parser.parse_args()
    source = read('src/plugins/automation/scriptlist.cpp')
    shared = read('src/plugins/shared/spl_diskselection.h')
    markers = ('static BOOL ResolveScriptPath(', 'static BOOL ResolveScriptPathWide(',
               'static HANDLE OpenReadFilePath(', 'static BOOL ReadManifestTextFile(',
               'static BOOL Utf8ToNative(', 'static BOOL NativeToUtf8(',
               'static BOOL AppendSalPath(', 'static BOOL PathsEqual(',
               'static BOOL LoadManifestForEntryPoint(')
    pieces = '\n'.join(block(source, marker) for marker in markers) + '\n' + block(source, 'class CScopedAutomationRuntimeDispatch', ';') + '\n' + block(source, 'HRESULT CScriptInfo::LoadOleStringFromFile(').replace('CScriptInfo::LoadOleStringFromFile', 'LoadOleStringFromFile')
    # Reproduce the old manifest intake by removing only the new expansion prefix.
    old_manifest = block(source, markers[-1])
    prefix = '''    std::vector<TCHAR> expanded;
    if (!ResolveScriptPath(entryPointPath, expanded))
        return FALSE;
    entryPointPath = expanded.data();
'''
    assert prefix in old_manifest
    old_manifest = old_manifest.replace(prefix, '').replace('LoadManifestForEntryPoint', 'BeforeExpansionManifest')
    for method, field in (('bool CScriptInfo::ExecuteThroughRuntime(', 'm_szFileName'),
                          ('BOOL WINAPI CScriptInfo::RuntimeLifecycleCallback(', 'script->m_szFileName')):
        body = block(source, method)
        assert 'ResolveScriptPathWide(' + field in body
        assert 'request.EntryPoint =' in body and '.c_str()' in body
    execution = block(source, 'bool CScriptInfo::ExecuteThroughRuntime(')
    assert execution.index('if (!PublishSalamatrixManifestCommands())') < execution.index('adapter->StartPersistent(')
    assert execution.index('CScopedAutomationRuntimeDispatch liveDispatch(m_lRuntimeStopping);') < execution.index('adapter->StartPersistent(')
    dispatch = block(source, 'BOOL WINAPI CScriptInfo::RuntimeHostDispatch(')
    assert '&script->m_lRuntimeStopping, FALSE, FALSE) != FALSE' in dispatch
    environment = compiler_environment()
    compiler = shutil.which('cl', path=environment.get('PATH'))
    with fixture_directory() as folder:
        folder = Path(folder)
        base = folder/'runtime'; short = base/'scripts/main.py'
        long = base/('東京-'+'a'*90)/('žluťoučký-'+'b'*90)/('c'*90)/'main.py'
        manifest = {'schema':2,'id':'Port.RepositoryPaths','name':'Repository paths',
                    'version':'1.0.0','runtime':'Python.CPython','entryPoint':'main.py',
                    'capabilities':['panels.read'],'events':[],
                    'commands':[{'id':'run','title':'Run','handler':'run','menu':'both'}]}
        for entry in (short,long):
            parent = Path('\\\\?\\'+str(entry.parent)); parent.mkdir(parents=True)
            (parent/'extension.json').write_text(json.dumps(manifest), encoding='utf-8')
            (parent/'main.py').write_bytes(b'# disposable read-only fixture\n')
        (short.parent/'empty.js').write_bytes(b'')
        template = r'''#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <set>
#include "@@MANIFEST_HEADER@@"
#define SAL_MAX_PATH 32768
std::set<HANDLE> openHandles;int untrackedCloses=0;bool trackedClose=false;
HANDLE TrackOpen(HANDLE h){if(h!=INVALID_HANDLE_VALUE)openHandles.insert(h);return h;}
BOOL TrackClose(HANDLE h){if(trackedClose){if(openHandles.erase(h)!=1)++untrackedCloses;trackedClose=false;}return CloseHandle(h);}
#define HANDLES_Q(x) TrackOpen(x)
#define HANDLES(x) (trackedClose=true,(x))
#define CloseHandle(h) TrackClose(h)
struct ExpansionFacade {
 std::string Root; bool Fail=false; int Calls=0;
 bool ExpandPath(const char* input,char* output,int capacity){
  ++Calls;if(Fail||!input)return false;std::string value(input),prefix="$(SalDir)";
  if(value.compare(0,prefix.size(),prefix)==0)value.replace(0,prefix.size(),Root);
  if(value.size()>=static_cast<size_t>(capacity))return false;
  return strcpy_s(output,static_cast<size_t>(capacity),value.c_str())==0;
 }
} g_oAutomationPlugin;
struct PathAppendFacade {
 BOOL SalPathAppend(char* path,const char* name,int capacity){std::string joined(path);if(!joined.empty()&&joined.back()!='\\')joined+='\\';joined+=name;return joined.size()<static_cast<size_t>(capacity)&&strcpy_s(path,static_cast<size_t>(capacity),joined.c_str())==0;}
} general,*SalamanderGeneral=&general;
namespace SalamanderDiskSelection {
@@WIDE@@
}
@@PIECES@@
@@BASELINE@@
int checks=0,failures=0;
void Check(bool ok,const char* label){++checks;if(!ok){++failures;printf("FAIL %s\n",label);}}
int main(){
 volatile LONG stopping=TRUE;
 {CScopedAutomationRuntimeDispatch live(stopping);Check(stopping==FALSE,"one-shot dispatch live during execution");}
 Check(stopping==TRUE,"one-shot stopping guard restored after success");
 try{CScopedAutomationRuntimeDispatch live(stopping);throw 1;}catch(int){}
 Check(stopping==TRUE,"one-shot stopping guard restored after exception");
 stopping=FALSE;{CScopedAutomationRuntimeDispatch live(stopping);Check(stopping==FALSE,"already active dispatch preserved");}
 Check(stopping==FALSE,"already active dispatch restored");
 g_oAutomationPlugin.Root=@@ROOT@@;
 const char* shortPath=@@SHORT@@;const char* longPath=@@LONG@@;
 std::string macro="$(SalDir)\\scripts\\main.py";
 std::vector<TCHAR> native;std::wstring wide;
 std::vector<char> utf8(SAL_MAX_PATH*3);Check(NativeToUtf8(shortPath,utf8.data(),static_cast<int>(utf8.size()))&&std::string(utf8.data())==shortPath,"native UTF8 conversion ASCII control");
 Check(ResolveScriptPath(macro.c_str(),native)&&std::string(native.data())==shortPath,"stored macro resolved exactly");
 Check(ResolveScriptPathWide(macro.c_str(),wide)&&wide==SalamanderDiskSelection::WideFromPath(shortPath),"wide runtime macro resolved exactly");
 Check(ResolveScriptPath(shortPath,native)&&std::string(native.data())==shortPath,"absolute native path preserved");
 Check(ResolveScriptPathWide(longPath,wide)&&wide==SalamanderDiskSelection::WideFromPath(longPath)&&wide.size()>260,"long Unicode runtime path preserved");
 Check(!ResolveScriptPath(nullptr,native)&&!ResolveScriptPathWide(nullptr,wide),"null input refused");
 g_oAutomationPlugin.Fail=true;Check(!ResolveScriptPath(macro.c_str(),native)&&!ResolveScriptPathWide(macro.c_str(),wide),"expansion failure refused");g_oAutomationPlugin.Fail=false;
 std::string oversized(SAL_MAX_PATH,'x');Check(!ResolveScriptPath(oversized.c_str(),native),"capacity overflow refused without truncation");
 CExtensionManifest manifest;
 Check(!BeforeExpansionManifest(macro.c_str(),manifest),"regression reproduced: prior macro intake fails");
 Check(BeforeExpansionManifest(shortPath,manifest),"prior absolute intake control succeeds");
 Check(LoadManifestForEntryPoint(macro.c_str(),manifest)&&manifest.RuntimeId=="Python.CPython"&&manifest.Commands.size()==1,"actual macro manifest intake succeeds");
 Check(LoadManifestForEntryPoint(shortPath,manifest)&&manifest.Name=="Repository paths","absolute manifest compatibility");
 Check(LoadManifestForEntryPoint(longPath,manifest)&&manifest.RuntimeId=="Python.CPython","actual long Unicode manifest file I/O succeeds");
 Check(!LoadManifestForEntryPoint("$(SalDir)\\missing\\main.py",manifest),"missing entry manifest refused");
 LPOLESTR code=nullptr;ULONG count=0;
 Check(SUCCEEDED(LoadOleStringFromFile(shortPath,code,&count))&&count>0,"legacy mapped script read succeeds");free(code);
 Check(openHandles.empty(),"legacy mapped script tracked file closed");
 std::string emptyPath=g_oAutomationPlugin.Root+"\\scripts\\empty.js";
 Check(SUCCEEDED(LoadOleStringFromFile(emptyPath.c_str(),code,&count))&&code[0]==0,"legacy empty script read succeeds");free(code);
 Check(openHandles.empty(),"legacy empty script tracked file closed");
 Check(FAILED(LoadOleStringFromFile("missing-owned-script.js",code,&count)),"legacy missing script failure preserved");
 Check(openHandles.empty(),"all opened manifest handles released");
 Check(untrackedCloses==0,"debug handle tracking opens and closes match");
 printf("Automation repository paths: %d/%d checks passed\n",checks-failures,checks);return failures?1:0;
}
'''
        replacements = {'MANIFEST_HEADER':(ROOT/'src/plugins/salamatrix/salamatrix_manifest.h').as_posix(),
                        'WIDE':block(shared,'inline std::wstring WideFromPath('),
                        'PIECES':pieces,'BASELINE':old_manifest,
                        'ROOT':'u8'+json.dumps(str(base)), 'SHORT':'u8'+json.dumps(str(short)),
                        'LONG':'u8'+json.dumps(str(long))}
        for key,value in replacements.items():template=template.replace('@@'+key+'@@',value)
        assert '@@' not in template
        cpp=folder/'test.cpp';cpp.write_text(template,encoding='utf-8');exe=folder/'test.exe'
        configurations=('Debug','Release') if args.configuration=='Both' else (args.configuration,)
        for configuration in configurations:
            flags=['/MDd','/D_DEBUG','/Od'] if configuration=='Debug' else ['/MD','/DNDEBUG','/O2']
            command=[compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',
                     '/D_CRT_SECURE_NO_WARNINGS',*flags,str(cpp),
                     str(ROOT/'src/plugins/salamatrix/salamatrix_manifest.cpp'),
                     '/Fe:'+str(exe),'/link','shlwapi.lib']
            print('Automation repository paths native:',configuration,flush=True)
            for invocation in (command,[str(exe)]):
                result=run(invocation,cwd=folder,env=environment,timeout=40)
                if result.returncode:return result.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
