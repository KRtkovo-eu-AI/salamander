// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Exercise unverified root fallback after path replacement. | Keep descendants outside elevated retry scope. | #731 | GPT-6.1 Terra |
#include "../branch_view.h"
#include <aclapi.h>
#include <sddl.h>
#include <atomic>
#include <cstdio>
using namespace Salamander::BranchView;
static std::atomic<int> opens{0}, captures{0};
static std::wstring original, replacement, moved;
static DWORD injected;
static bool deniedRoot=false;
static HANDLE control=INVALID_HANDLE_VALUE;
static PACL oldAcl=nullptr, acl=nullptr;
static PSECURITY_DESCRIPTOR oldSd=nullptr, deny=nullptr;
static SECURITY_DESCRIPTOR_CONTROL flags{};
static DWORD Open(void*, const std::wstring&, const DeniedDirectory*, DeniedDirectory&, void*&)
{
    ++opens;
    if (deniedRoot) return ERROR_ACCESS_DENIED;
    if (opens == 1)
    {
        if (!MoveFileW(original.c_str(), moved.c_str())) { std::printf("first rename failed %lu\n", GetLastError()); return ERROR_INVALID_DATA; }
        if (!MoveFileW(replacement.c_str(), original.c_str())) { std::printf("second rename failed %lu\n", GetLastError()); return ERROR_INVALID_DATA; }
        control=CreateFileW((original+L"\\grand\\denied").c_str(),READ_CONTROL|WRITE_DAC,
            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);
        DWORD revision=0;
        if (control==INVALID_HANDLE_VALUE ||
            GetSecurityInfo(control,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,nullptr,nullptr,&oldAcl,nullptr,&oldSd) ||
            !GetSecurityDescriptorControl(oldSd,&flags,&revision) ||
            SetSecurityInfo(control,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION,nullptr,nullptr,acl,nullptr))
            return ERROR_INVALID_DATA;
        return injected;
    }
    return ERROR_INVALID_DATA;
}
static DWORD Next(void*, void*, std::vector<IdentifiedPageRow>&, bool&) { return ERROR_INVALID_DATA; }
static void Close(void*, void*) {}
static bool Capture(const std::wstring&, DeniedDirectory&) { ++captures; return true; }
static bool File(const std::wstring& path)
{
    HANDLE h=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (h==INVALID_HANDLE_VALUE) return false;
    CloseHandle(h); return true;
}
static bool Case(const std::wstring& parent, DWORD code, const wchar_t* label)
{
    const std::wstring root=parent+L"\\scope-"+std::to_wstring(code);
    original=root+L"\\original"; moved=root+L"\\moved"; replacement=root+L"\\replacement";
    const std::wstring grand=replacement+L"\\grand", denied=grand+L"\\denied";
    if (!CreateDirectoryW(root.c_str(),nullptr) || !CreateDirectoryW(original.c_str(),nullptr) ||
        !CreateDirectoryW(replacement.c_str(),nullptr) || !CreateDirectoryW(grand.c_str(),nullptr) ||
        !CreateDirectoryW(denied.c_str(),nullptr) || !File(grand+L"\\visible.txt")) return false;
    control=INVALID_HANDLE_VALUE; oldAcl=nullptr; oldSd=nullptr; deny=nullptr; acl=nullptr; flags=0;
    BOOL present=FALSE,defaulted=FALSE;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(D;;0x00000081;;;WD)(A;;FA;;;WD)",SDDL_REVISION_1,&deny,nullptr) ||
        !GetSecurityDescriptorDacl(deny,&present,&acl,&defaulted) || !present) return false;
    opens=0; captures=0; injected=code; deniedRoot=false;
    Scan scan; scan.SetLocalPager({Open,Next,Close}); scan.SetCaptureDenied(Capture);
    bool ok=scan.Start(original,false) && scan.WaitForTest(5000);
    std::vector<Entry> entries; Progress progress{}; scan.Drain(entries,progress);
    std::vector<DeniedDirectory> attempted; scan.DrainDenied(attempted);
    ok = ok && opens.load()==1 && captures.load()==0 && attempted.empty() && progress.IdentityUnavailable && entries.size()==1 &&
        entries[0].FullPath()==original+L"\\grand\\visible.txt";
    if (!ok) std::wprintf(L"opens=%d captures=%d denied=%zu unavailable=%d errors=%u error=%lu entries=%zu first=%ls\n",
        opens.load(),captures.load(),attempted.size(),progress.IdentityUnavailable?1:0,progress.Errors,progress.LastError,
        entries.size(),entries.empty()?L"":entries[0].FullPath().c_str());
    std::wprintf(L"%ls: %ls root replacement is ordinary-only through all descendants\n",ok?L"PASS":L"FAIL",label);
    const DWORD restore=SetSecurityInfo(control,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION|
        ((flags&SE_DACL_PROTECTED)?PROTECTED_DACL_SECURITY_INFORMATION:UNPROTECTED_DACL_SECURITY_INFORMATION),nullptr,nullptr,oldAcl,nullptr);
    LocalFree(deny); LocalFree(oldSd); CloseHandle(control);
    DeleteFileW((original+L"\\grand\\visible.txt").c_str());
    RemoveDirectoryW((original+L"\\grand\\denied").c_str()); RemoveDirectoryW((original+L"\\grand").c_str());
    RemoveDirectoryW(original.c_str()); RemoveDirectoryW(moved.c_str()); RemoveDirectoryW(root.c_str());
    return ok && !restore;
}
static bool DeniedRoot(const std::wstring& parent)
{
    const std::wstring root=parent+L"\\denied-root";
    if (!CreateDirectoryW(root.c_str(),nullptr)) return false;
    HANDLE h=CreateFileW(root.c_str(),READ_CONTROL|WRITE_DAC,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);
    if (h==INVALID_HANDLE_VALUE) return false;
    PACL previous=nullptr, restricted=nullptr; PSECURITY_DESCRIPTOR previousSd=nullptr, restrictedSd=nullptr;
    SECURITY_DESCRIPTOR_CONTROL attributes{}; DWORD revision=0; BOOL present=FALSE,defaulted=FALSE;
    if (GetSecurityInfo(h,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,nullptr,nullptr,&previous,nullptr,&previousSd) ||
        !GetSecurityDescriptorControl(previousSd,&attributes,&revision) ||
        !ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(D;;0x00000081;;;WD)(A;;FA;;;WD)",SDDL_REVISION_1,&restrictedSd,nullptr) ||
        !GetSecurityDescriptorDacl(restrictedSd,&present,&restricted,&defaulted) || !present ||
        SetSecurityInfo(h,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION,nullptr,nullptr,restricted,nullptr)) return false;
    opens=0; captures=0; deniedRoot=true;
    Scan scan; scan.SetLocalPager({Open,Next,Close}); scan.SetCaptureDenied(Capture);
    bool ok=scan.Start(root,false) && scan.WaitForTest(5000);
    std::vector<DeniedDirectory> attempted; scan.DrainDenied(attempted);
    ok = ok && opens.load()==1 && captures.load()==1 && attempted.size()==1 && attempted[0].Verified && attempted[0].Path==root;
    std::printf("%s: failed root FindFirst retains identity-capture retry\n",ok?"PASS":"FAIL");
    const DWORD restore=SetSecurityInfo(h,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION|
        ((attributes&SE_DACL_PROTECTED)?PROTECTED_DACL_SECURITY_INFORMATION:UNPROTECTED_DACL_SECURITY_INFORMATION),nullptr,nullptr,previous,nullptr);
    LocalFree(restrictedSd); LocalFree(previousSd); CloseHandle(h); RemoveDirectoryW(root.c_str());
    deniedRoot=false;
    return ok && !restore;
}
int wmain(int argc,wchar_t** argv)
{
    if (argc!=2) return 2;
    bool ok=true;
    ok &= Case(argv[1],ERROR_REPARSE_TAG_INVALID,L"reparse root");
    ok &= Case(argv[1],ERROR_SHARING_VIOLATION,L"sharing fallback");
    ok &= Case(argv[1],ERROR_NOT_SUPPORTED,L"unsupported fallback");
    ok &= Case(argv[1],ERROR_ACCESS_DENIED,L"readable root access-denied fallback");
    ok &= DeniedRoot(argv[1]);
    return ok ? 0 : 1;
}
