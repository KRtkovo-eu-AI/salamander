// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// Direct production plugin-wrapper coverage absent from donor PR #769.
// Fixture lifecycle adapted from its safe_file_paths_tests.cpp.in via S02a.
#include "plugin_handles_wide_test_support/precomp.h"
#include <vector>
#include "widepath.h"
CSalamanderDebugAbstract* SalamanderDebug = nullptr;
int SalamanderVersion = 0;
#ifdef MHANDLES_ENABLE
struct InspectableHandles : C__Handles
{
    int Count() const { return Handles.Count; }
    bool Contains(HANDLE h, C__HandlesType type, C__HandlesOrigin origin)
    {
        for (int i=0;i<Handles.Count;++i)
            if (Handles[i].Handle.Handle==h && Handles[i].Handle.Type==type && Handles[i].Handle.Origin==origin) return true;
        return false;
    }
};
#define TRACKED(t, operation) t.SetInfo(__FILE__,__LINE__,__otQuiet).operation
#else
struct InspectableHandles { int Count() const { return 0; } };
#define TRACKED(t, operation) ::operation
#endif
static int Checks = 0, Failures = 0;
static void Check(bool success, const char* description)
{
    ++Checks;
    if (!success) { ++Failures; std::printf("FAIL: %s (Win32 %lu)\n", description, GetLastError()); }
}
static std::string Utf8(const std::wstring& value)
{
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (count <= 0) std::abort();
    std::string result(count, '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.c_str(), -1, &result[0], count, nullptr, nullptr);
    result.resize(count - 1);
    return result;
}
static std::wstring Extended(const std::wstring& value) { return SalPathAddExtendedPrefixW(value.c_str()); }
struct Fixtures
{
    std::wstring Root, OldDirectory;
    std::vector<std::wstring> Files, Directories;
    Fixtures()
    {
        CWidePathBuffer buffer;
        DWORD count = GetCurrentDirectoryW(static_cast<DWORD>(buffer.Capacity()), buffer.Data());
        if (!count || count >= static_cast<size_t>(buffer.Capacity())) std::abort();
        OldDirectory.assign(buffer.Data(), count);
        count = GetTempPathW(static_cast<DWORD>(buffer.Capacity()), buffer.Data());
        if (!count || count >= static_cast<size_t>(buffer.Capacity())) std::abort();
        Root.assign(buffer.Data(), count);
        Root += L"Samandarin-plugin-handles-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
        if (!CreateDirectoryW(Root.c_str(), nullptr)) std::abort();
        Directories.push_back(Root);
    }
    bool Owned(const std::wstring& path) const
    {
        return path == Root || (path.size() > Root.size() &&
               path.compare(0, Root.size(), Root) == 0 && path[Root.size()] == L'\\');
    }
    ~Fixtures()
    {
        Check(SetCurrentDirectoryW(OldDirectory.c_str()) != FALSE, "restore working directory");
        for (auto i = Files.rbegin(); i != Files.rend(); ++i)
            Check(Owned(*i) && DeleteFileW(Extended(*i).c_str()) != FALSE, "remove own fixture file");
        for (auto i = Directories.rbegin(); i != Directories.rend(); ++i)
            Check(Owned(*i) && RemoveDirectoryW(Extended(*i).c_str()) != FALSE, "remove own fixture directory");
    }
    std::wstring Directory(const std::wstring& parent, const std::wstring& name)
    {
        const std::wstring path = parent + L"\\" + name;
        if (!Owned(path) || !CreateDirectoryW(Extended(path).c_str(), nullptr)) std::abort();
        Directories.push_back(path);
        return path;
    }
    std::wstring File(const std::wstring& parent, const std::wstring& name, DWORD size)
    {
        const std::wstring path = parent + L"\\" + name;
        if (!Owned(path)) std::abort();
        HANDLE handle = CreateFileW(Extended(path).c_str(), GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) std::abort();
        Files.push_back(path);
        std::string payload(size, 'x');
        DWORD written = 0;
        const BOOL ok = WriteFile(handle, payload.data(), size, &written, nullptr);
        CloseHandle(handle);
        if (!ok || written != size) std::abort();
        return path;
    }
};



static void RoundTrip(Fixtures& fixtures, const std::wstring& directory, const std::wstring& name, bool ansi)
{
    const auto path=fixtures.File(directory,name,3);const auto api=Extended(path);
    InspectableHandles tracker;
    HANDLE file=ansi ? TRACKED(tracker,CreateFileA(Utf8(path).c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr))
                     : TRACKED(tracker,CreateFileW(api.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
    Check(file!=INVALID_HANDLE_VALUE,"open exact owned file");
#ifdef MHANDLES_ENABLE
    Check(tracker.Count()==(file!=INVALID_HANDLE_VALUE?1:0),"file tracked exactly once");
    if(file!=INVALID_HANDLE_VALUE)Check(tracker.Contains(file,__htFile,__hoCreateFile),"file registry type/origin");
#endif
    if(file!=INVALID_HANDLE_VALUE)
    {
        HANDLE raw=::CreateFileW(api.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
        BY_HANDLE_FILE_INFORMATION actual={},expected={};
        Check(raw!=INVALID_HANDLE_VALUE && GetFileInformationByHandle(file,&actual) && GetFileInformationByHandle(raw,&expected) &&
            actual.dwVolumeSerialNumber==expected.dwVolumeSerialNumber && actual.nFileIndexHigh==expected.nFileIndexHigh && actual.nFileIndexLow==expected.nFileIndexLow,
            "wrapper opens the exact expected file identity");
        if(raw!=INVALID_HANDLE_VALUE)::CloseHandle(raw);
        char bytes[3]={};DWORD done=0;
        Check(ReadFile(file,bytes,3,&done,nullptr) && done==3 && memcmp(bytes,"xxx",3)==0,"read exact fixture contents");
        Check(TRACKED(tracker,CloseHandle(file))!=FALSE,"tracked file cleanup");
    }
    HANDLE find;
    if(ansi)
    {
        WIN32_FIND_DATAA data={};find=TRACKED(tracker,FindFirstFileA(Utf8(path).c_str(),&data));
        Check(find!=INVALID_HANDLE_VALUE && strcmp(data.cFileName,Utf8(name).c_str())==0 && data.nFileSizeLow==3,"A enumeration exact filename and size");
    }
    else
    {
#ifdef MHANDLES_ENABLE
        ArmDiagnostic(DiagnosticNormal);
#endif
        WIN32_FIND_DATAW data={};find=TRACKED(tracker,FindFirstFileW(api.c_str(),&data));
        Check(find!=INVALID_HANDLE_VALUE && name==data.cFileName && data.nFileSizeLow==3,"W enumeration exact Unicode filename and size");
#ifdef MHANDLES_ENABLE
        Check(CapturedDiagnostic()==Utf8(api),"full Unicode/long UTF8 diagnostic is not truncated");
        Check(DiagnosticAllocations()==DiagnosticFrees(),"full-path diagnostic allocations balanced");
        DisarmDiagnostic();
#endif
    }
#ifdef MHANDLES_ENABLE
    Check(tracker.Count()==(find!=INVALID_HANDLE_VALUE?1:0),"enumeration tracked once");
    if(find!=INVALID_HANDLE_VALUE)Check(tracker.Contains(find,__htFindFile,__hoFindFirstFile),"enumeration registry type/origin");
#endif
    if(find!=INVALID_HANDLE_VALUE)Check(TRACKED(tracker,FindClose(find))!=FALSE,"tracked enumeration cleanup");
    Check(tracker.Count()==0,"all file/enumeration handles balanced");
}
static void Legacy(Fixtures& fixtures)
{
    const auto path=fixtures.File(fixtures.Root,L"legacy.txt",2);const auto narrow=Utf8(path);
    HANDLE file=HANDLES_Q(CreateFile(narrow.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
    Check(file!=INVALID_HANDLE_VALUE,"ANSI generic CreateFile alias");
    if(file!=INVALID_HANDLE_VALUE)Check(HANDLES_Q(CloseHandle(file))!=FALSE,"generic file cleanup");
    WIN32_FIND_DATA data={};HANDLE find=HANDLES_Q(FindFirstFile(narrow.c_str(),&data));
    Check(find!=INVALID_HANDLE_VALUE && strcmp(data.cFileName,"legacy.txt")==0,"ANSI generic FindFirstFile alias");
    if(find!=INVALID_HANDLE_VALUE)Check(HANDLES_Q(FindClose(find))!=FALSE,"generic enumeration cleanup");
}
#ifdef MHANDLES_ENABLE
static void DiagnosticCases(Fixtures& fixtures)
{
    const auto existing=Extended(fixtures.File(fixtures.Root,L"diagnostic-český-東京.txt",5));
    const auto missing=Extended(fixtures.Root+L"\\missing-žluťoučký-東京.txt");
    for(int mode=DiagnosticNormal;mode<=DiagnosticConvertFailure;++mode)
    {
        InspectableHandles tracker;
        SetLastError(12345);HANDLE raw=::CreateFileW(missing.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);const DWORD rawError=GetLastError();
        Check(raw==INVALID_HANDLE_VALUE,"raw missing file expected failure");if(raw!=INVALID_HANDLE_VALUE)::CloseHandle(raw);
        ArmDiagnostic(static_cast<DiagnosticFault>(mode));SetLastError(12345);
        HANDLE file=TRACKED(tracker,CreateFileW(missing.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));const DWORD error=GetLastError();
        Check(file==INVALID_HANDLE_VALUE && error==rawError,"CreateFileW preserves actual error despite diagnostic faults");
        Check(tracker.Count()==0,"failed open adds no registry entry");
        Check(DiagnosticConversions()>0,"missing-file W diagnostic exercised");
        if(mode==DiagnosticNormal)Check(CapturedDiagnostic()==Utf8(missing),"failure diagnostic contains exact UTF8 path");
        Check(DiagnosticAllocations()==DiagnosticFrees(),"file diagnostic allocation balanced including second-pass failure");DisarmDiagnostic();
        for(bool success : {false,true})
        {
            const auto& path=success?existing:missing;WIN32_FIND_DATAW rawData={},data={};SetLastError(12345);
            raw=::FindFirstFileW(path.c_str(),&rawData);const DWORD expectedError=GetLastError();if(raw!=INVALID_HANDLE_VALUE)::FindClose(raw);
            ArmDiagnostic(static_cast<DiagnosticFault>(mode));SetLastError(12345);
            HANDLE find=TRACKED(tracker,FindFirstFileW(path.c_str(),&data));const DWORD findError=GetLastError();
            Check((find!=INVALID_HANDLE_VALUE)==success && findError==expectedError,"FindFirstFileW preserves result/error despite diagnostics");
            Check(tracker.Count()==(success?1:0),"diagnostic failure cannot change successful enumeration tracking");
            Check(DiagnosticConversions()>0,"enumeration W diagnostic exercised on success and failure");
            if(mode==DiagnosticNormal)Check(CapturedDiagnostic()==Utf8(path),"enumeration diagnostic exact UTF8 text");
            Check(DiagnosticAllocations()==DiagnosticFrees(),"enumeration diagnostic allocation balanced");DisarmDiagnostic();
            if(find!=INVALID_HANDLE_VALUE)
            {
                Check(std::wstring(data.cFileName)==L"diagnostic-český-東京.txt","faulted diagnostics cannot corrupt returned name");
                Check(tracker.Contains(find,__htFindFile,__hoFindFirstFile),"faulted diagnostic registry type/origin");
                Check(TRACKED(tracker,FindClose(find))!=FALSE,"faulted diagnostic enumeration cleanup");
            }
            Check(tracker.Count()==0,"diagnostic-case registry balanced");
        }
        ArmDiagnostic(static_cast<DiagnosticFault>(mode));SetLastError(12345);
        file=TRACKED(tracker,CreateFileW(existing.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
        Check(file!=INVALID_HANDLE_VALUE && DiagnosticConversions()==0,"successful CreateFileW needs no diagnostic conversion");DisarmDiagnostic();
        if(file!=INVALID_HANDLE_VALUE)Check(TRACKED(tracker,CloseHandle(file))!=FALSE,"successful open cleanup under diagnostic OOM mode");
    }
    ArmDiagnostic(DiagnosticNoMemory);Check(TestNullDiagnostic()==nullptr && DiagnosticConversions()==0,"null diagnostic path is safe without conversion");DisarmDiagnostic();
}
#endif
int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    {
        Fixtures fixtures;
        const bool ascii=Utf8(fixtures.Root).size()==fixtures.Root.size() && fixtures.Root.size()+40<MAX_PATH;
        Check(ascii,"own root fits ANSI API boundary");if(!ascii)return 1;
        const auto sentinel=fixtures.File(fixtures.Root,L"sentinel.txt",7);
        RoundTrip(fixtures,fixtures.Root,L"ascii.txt",true);
        RoundTrip(fixtures,fixtures.Root,L"český-東京.txt",false);
        RoundTrip(fixtures,fixtures.Root,std::wstring(180,L'東')+L".txt",false);
        auto unicode=fixtures.Directory(fixtures.Root,L"žluťoučký");RoundTrip(fixtures,unicode,L"ascii.txt",false);
        unicode=fixtures.Directory(unicode,L"東京-\U0001F642");RoundTrip(fixtures,unicode,L"český-東京.txt",false);
        const auto byteLong=fixtures.Directory(fixtures.Root,std::wstring(90,L'東'));
        Check(Utf8(byteLong).size()>MAX_PATH && byteLong.size()<MAX_PATH,"UTF8-byte-long path below UTF16 limit");RoundTrip(fixtures,byteLong,L"bytes.txt",false);
        auto deep=fixtures.Root;for(int i=0;i<18;++i)deep=fixtures.Directory(deep,L"component-東京-"+std::to_wstring(i));
        Check(deep.size()>MAX_PATH,"actual UTF16-long fixture");RoundTrip(fixtures,deep,L"long.txt",false);
        Legacy(fixtures);
#ifdef MHANDLES_ENABLE
        DiagnosticCases(fixtures);
#endif
        HANDLE untouched=::CreateFileW(sentinel.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);char bytes[7]={};DWORD done=0;
        Check(untouched!=INVALID_HANDLE_VALUE && ReadFile(untouched,bytes,7,&done,nullptr) && done==7 && memcmp(bytes,"xxxxxxx",7)==0,"unrelated own sentinel unchanged");
        if(untouched!=INVALID_HANDLE_VALUE)::CloseHandle(untouched);
    }
    std::printf("Plugin wide handles: %d checks, %d failures\n",Checks,Failures);return Failures?1:0;
}
