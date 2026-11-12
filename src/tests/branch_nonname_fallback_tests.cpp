// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Test ordinary traversal under a non-name-surrogate row. | Preserve readable descendants without opening an unverified elevated path. | #731 | GPT-6.1 Terra |
#include "../branch_view.h"
#include <aclapi.h>
#include <sddl.h>
#include <atomic>
#include <cstdio>
using namespace Salamander::BranchView;
static std::atomic<int> opens{0}, captures{0};
static DWORD Open(void*, const std::wstring&, const DeniedDirectory*, DeniedDirectory& actual, void*& cursor)
{
    ++opens; actual.Verified = true; actual.Volume = 1; actual.FileId[0] = 1;
    cursor = reinterpret_cast<void*>(1); return ERROR_SUCCESS;
}
static DWORD Next(void*, void*, std::vector<IdentifiedPageRow>& rows, bool& done)
{
    IdentifiedPageRow nonname{}; wcscpy_s(nonname.Data.cFileName, L"nonname"); nonname.FileId[0] = 2;
    nonname.Data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT;
    nonname.Data.dwReserved0 = 0x80000017; // synthetic non-name-surrogate tag, not a real driver
    IdentifiedPageRow link{}; wcscpy_s(link.Data.cFileName, L"link"); link.FileId[0] = 3;
    link.Data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT;
    link.Data.dwReserved0 = IO_REPARSE_TAG_MOUNT_POINT;
    rows = {nonname, link}; done = true; return ERROR_SUCCESS;
}
static void Close(void*, void*) {}
static bool Capture(const std::wstring&, DeniedDirectory&) { ++captures; return true; }
static bool File(const std::wstring& path)
{
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    CloseHandle(h); return true;
}
int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 2;
    const std::wstring root=std::wstring(argv[1])+L"\\fallback-"+std::to_wstring(GetCurrentProcessId());
    const std::wstring nonname=root+L"\\nonname", grand=nonname+L"\\grand", denied=grand+L"\\denied", link=root+L"\\link";
    if (!CreateDirectoryW(root.c_str(),nullptr) || !CreateDirectoryW(nonname.c_str(),nullptr) ||
        !CreateDirectoryW(grand.c_str(),nullptr) || !CreateDirectoryW(denied.c_str(),nullptr) ||
        !CreateDirectoryW(link.c_str(),nullptr) || !File(nonname+L"\\visible.txt") ||
        !File(grand+L"\\grand.txt") || !File(link+L"\\must-not-appear.txt")) return 3;
    HANDLE control=CreateFileW(denied.c_str(),READ_CONTROL|WRITE_DAC,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);
    if (control==INVALID_HANDLE_VALUE) return 4;
    PACL oldAcl=nullptr; PSECURITY_DESCRIPTOR oldSd=nullptr;
    if (GetSecurityInfo(control,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,nullptr,nullptr,&oldAcl,nullptr,&oldSd)) return 5;
    SECURITY_DESCRIPTOR_CONTROL flags{}; DWORD revision=0;
    if (!GetSecurityDescriptorControl(oldSd,&flags,&revision)) return 6;
    PSECURITY_DESCRIPTOR deny=nullptr; PACL acl=nullptr; BOOL present=FALSE,defaulted=FALSE;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(D;;0x00000081;;;WD)(A;;FA;;;WD)",SDDL_REVISION_1,&deny,nullptr) ||
        !GetSecurityDescriptorDacl(deny,&present,&acl,&defaulted) || !present ||
        SetSecurityInfo(control,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION,nullptr,nullptr,acl,nullptr)) return 7;
    Scan scan; scan.SetLocalPager({Open,Next,Close}); scan.SetCaptureDenied(Capture);
    bool ok=scan.Start(root,false) && scan.WaitForTest(5000);
    std::vector<Entry> entries; Progress progress{}; scan.Drain(entries,progress);
    std::vector<DeniedDirectory> attempted; scan.DrainDenied(attempted);
    bool visible=false, nested=false, leaked=false;
    for (const auto& entry:entries) { visible |= entry.FullPath()==nonname+L"\\visible.txt";
        nested |= entry.FullPath()==grand+L"\\grand.txt"; leaked |= entry.FullPath()==link+L"\\must-not-appear.txt"; }
    ok = ok && visible && nested && !leaked && opens.load()==1 && captures.load()==0 && attempted.empty() && progress.IdentityUnavailable;
    std::printf("%s: synthetic non-name row traverses readable grandchild without privileged retry; surrogate skipped\n",ok?"PASS":"FAIL");
    const DWORD restore=SetSecurityInfo(control,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION|
        ((flags&SE_DACL_PROTECTED)?PROTECTED_DACL_SECURITY_INFORMATION:UNPROTECTED_DACL_SECURITY_INFORMATION),nullptr,nullptr,oldAcl,nullptr);
    LocalFree(deny); LocalFree(oldSd); CloseHandle(control);
    DeleteFileW((nonname+L"\\visible.txt").c_str()); DeleteFileW((grand+L"\\grand.txt").c_str());
    DeleteFileW((link+L"\\must-not-appear.txt").c_str());
    RemoveDirectoryW(denied.c_str()); RemoveDirectoryW(grand.c_str()); RemoveDirectoryW(nonname.c_str()); RemoveDirectoryW(link.c_str()); RemoveDirectoryW(root.c_str());
    return ok && !restore ? 0 : 1;
}
