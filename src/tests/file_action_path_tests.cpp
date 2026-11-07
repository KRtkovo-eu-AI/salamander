// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include <windows.h>
#include <shellapi.h>
#include <cstdio>
#include <string>
#include <vector>
#include "../fileactionpath.h"
#include "native_path_test_support.h"
using namespace Salamander::FileActionPaths;
static int Failed = 0;
static void Check(bool ok, const char* name) { std::printf("%s: %s (error %lu)\n", ok ? "PASS" : "FAIL", name, ok ? 0UL : GetLastError()); if (!ok) ++Failed; }
static std::wstring Plain(std::wstring p) { if(p.rfind(L"\\\\?\\UNC\\",0)==0) return L"\\\\"+p.substr(8); if(p.rfind(L"\\\\?\\",0)==0) return p.substr(4); return p; }
static std::wstring LongName(const wchar_t* path) { DWORD n=GetLongPathNameW(path,NULL,0);if(!n)return Plain(path);std::wstring result(n,L'\0');DWORD size=GetLongPathNameW(path,&result[0],n);if(!size||size>=n)return Plain(path);result.resize(size);return Plain(result); }
static bool Wait(HANDLE process) { DWORD status=WaitForSingleObject(process,10000),code=1; if(status!=WAIT_OBJECT_0) TerminateProcess(process,9); GetExitCodeProcess(process,&code); CloseHandle(process); return status==WAIT_OBJECT_0 && code==0; }
static std::wstring Read(const std::wstring& path) {
 HANDLE h=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL); if(h==INVALID_HANDLE_VALUE)return L"";
 DWORD size=GetFileSize(h,NULL),got=0;std::wstring text(size/2,L'\0'); if(size)ReadFile(h,&text[0],size,&got,NULL);CloseHandle(h);return got==size?text:L"";
}
static bool LaunchResult(BOOL launched, const std::wstring& result,
                         const std::wstring& file, const std::wstring& directory)
{
    if (launched) return Read(result) == file + L"\n" + directory;
    // Volumes may disable 8.3 aliases. The documented Windows current-directory
    // limitation must then fail without starting the command in another folder.
    return InitialDirectory(Utf8(directory).c_str()).size() >= MAX_PATH &&
           GetFileAttributesW(result.c_str()) == INVALID_FILE_ATTRIBUTES;
}
int wmain(int argc,wchar_t**argv) {
 if(argc==4 && std::wstring(argv[1])==L"--record") {
  std::vector<wchar_t> cwd(32768);DWORD n=GetCurrentDirectoryW((DWORD)cwd.size(),cwd.data());if(!n||n>=cwd.size())return 3;
  std::wstring text=std::wstring(argv[3])+L"\n"+LongName(cwd.data());
  HANDLE h=CreateFileW(argv[2],GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);if(h==INVALID_HANDLE_VALUE)return 4;
  DWORD written=0;BOOL ok=WriteFile(h,text.data(),(DWORD)(text.size()*2),&written,NULL);CloseHandle(h);return ok?0:5;
 }
 if(argc!=1 && argc!=2)return 2;
 const std::wstring parent=NativePathTestParent(argc,argv);
 if(parent.empty())return 2;
 std::wstring root=parent+L"\\file-action-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
 std::vector<std::wstring> dirs;auto make=[&](const std::wstring&p){BOOL ok=CreateDirectoryW((L"\\\\?\\"+p).c_str(),NULL);if(ok)dirs.push_back(p);return ok!=FALSE;};if(!make(root))return 6;
 std::wstring unicode=root+L"\\日本-český-😀",bytes=root+L"\\"+std::wstring(100,L'日'),deep=unicode;
 if(!make(unicode)||!make(bytes))return 7;
 for(int i=0;i<8;++i){deep+=L"\\"+std::wstring(40,L'日')+std::to_wstring(i);if(!make(deep))return 8;}
 std::vector<wchar_t> exeBuffer(32768);GetModuleFileNameW(NULL,exeBuffer.data(),(DWORD)exeBuffer.size());std::wstring exe=exeBuffer.data();
 std::vector<std::wstring> parents={root,root,unicode,bytes,deep};
 std::vector<std::wstring> names={L"plain.txt",L"日本-😀.txt",L"ascii.txt",L"same.txt",L"český-😀.txt"};
 bool createdFiles=true,shortPaths=true;std::vector<std::wstring> files;
 for(size_t i=0;i<parents.size();++i){std::wstring file=parents[i]+L"\\"+names[i];HANDLE h=CreateFileW(InitialDirectory(Utf8(file).c_str()).c_str(),GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);if(h==INVALID_HANDLE_VALUE){createdFiles=false;continue;}CloseHandle(h);files.push_back(file);std::string shortName=ShortPath(Utf8(file).c_str());shortPaths=shortPaths&&!shortName.empty()&&GetFileAttributesW(InitialDirectory(shortName.c_str()).c_str())!=INVALID_FILE_ATTRIBUTES;}
 Check(createdFiles&&files.size()==parents.size(),"fixtures: ASCII, Unicode name/parents, UTF-8-byte-long and Win32-long paths");
 Check(shortPaths,"wide DOS expansion resolves every original file without truncation");
 for(size_t i=0;i<parents.size();++i){
  std::wstring file=parents[i]+L"\\"+names[i],result=root+L"\\direct"+std::to_wstring(i),args=L"--record \""+result+L"\" \""+file+L"\"",command=L"\""+exe+L"\" "+args;
  STARTUPINFOW si={};si.cb=sizeof(si);PROCESS_INFORMATION pi={};BOOL ok=LaunchProcess(Utf8(command).c_str(),Utf8(parents[i]).c_str(),CREATE_NO_WINDOW,&si,&pi);if(ok){CloseHandle(pi.hThread);ok=Wait(pi.hProcess);}
  Check(LaunchResult(ok,result,file,parents[i]),("CreateProcess real argv and working directory scenario "+std::to_string(i+1)).c_str());DeleteFileW(result.c_str());
 }
 for(size_t i: {size_t(2),size_t(4)}) {
  std::wstring file=parents[i]+L"\\"+names[i],result=root+L"\\shell"+std::to_wstring(i),args=L"--record \""+result+L"\" \""+file+L"\"",cwd=InitialDirectory(Utf8(parents[i]).c_str());
  SHELLEXECUTEINFOW sei={};sei.cbSize=sizeof(sei);sei.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_NO_UI;sei.lpFile=exe.c_str();sei.lpParameters=args.c_str();sei.lpDirectory=cwd.c_str();sei.nShow=SW_HIDE;
  BOOL ok=ShellExecuteExW(&sei);if(ok)ok=sei.hProcess&&Wait(sei.hProcess);
  Check(LaunchResult(ok,result,file,parents[i]),("ShellExecute real argv and working directory scenario "+std::to_string(i+1)).c_str());DeleteFileW(result.c_str());
 }
 for(size_t i: {size_t(2),size_t(4)}) {
  std::wstring file=parents[i]+L"\\"+names[i],result=root+L"\\batch"+std::to_wstring(i),batch=root+L"\\launch"+std::to_wstring(i)+L".bat";
  std::string text="@chcp 65001 >nul\r\n@pushd \""+Utf8(InitialDirectory(Utf8(parents[i]).c_str()))+"\" || exit /b 1\r\ncall \""+Utf8(exe)+"\" --record \""+Utf8(result)+"\" \""+Utf8(file)+"\"\r\n@popd\r\n";
  HANDLE h=CreateFileW(batch.c_str(),GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);DWORD written=0;if(h!=INVALID_HANDLE_VALUE){WriteFile(h,text.data(),(DWORD)text.size(),&written,NULL);CloseHandle(h);}
  std::vector<wchar_t> shell(32768);GetEnvironmentVariableW(L"COMSPEC",shell.data(),(DWORD)shell.size());std::wstring command=L"\""+std::wstring(shell.data())+L"\" /d /c \"\""+batch+L"\"\"";
  STARTUPINFOW si={};si.cb=sizeof(si);PROCESS_INFORMATION pi={};BOOL ok=LaunchProcess(Utf8(command).c_str(),NULL,CREATE_NO_WINDOW,&si,&pi);if(ok){CloseHandle(pi.hThread);ok=Wait(pi.hProcess);}
  Check(LaunchResult(ok,result,file,parents[i]),("UTF-8 batch argv and working directory scenario "+std::to_string(i+1)).c_str());DeleteFileW(result.c_str());DeleteFileW(batch.c_str());
 }
 bool clean=true;for(const auto& f:files)clean=DeleteFileW(InitialDirectory(Utf8(f).c_str()).c_str())!=FALSE&&clean;for(auto i=dirs.rbegin();i!=dirs.rend();++i)clean=RemoveDirectoryW(InitialDirectory(Utf8(*i).c_str()).c_str())!=FALSE&&clean;Check(clean,"cleanup only created fixtures");
 return Failed?1:0;
}
