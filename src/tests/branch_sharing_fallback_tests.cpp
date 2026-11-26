// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Test ordinary Branch fallback with a real DELETE handle. | Preserve readable entries without inventing elevated identity. | #731 | GPT-6.1 Terra |
#include "../branch_view.h"
#include <cstdio>
#include <string>
using namespace Salamander::BranchView;
static std::wstring root, child;
static bool holdRoot = false;
struct Cursor { std::wstring path; };
static DWORD Open(void*, const std::wstring& path, const DeniedDirectory*, DeniedDirectory& actual, void*& cursor)
{
    if (path == root && !holdRoot)
    {
        actual.Verified=true; actual.Volume=7; actual.FileId[0]=1; cursor=new Cursor{path}; return ERROR_SUCCESS;
    }
    HANDLE handle = CreateFileW(path.c_str(), FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return GetLastError();
    CloseHandle(handle); return ERROR_INVALID_DATA;
}
static DWORD Next(void*, void*, std::vector<IdentifiedPageRow>& rows, bool& done)
{
    IdentifiedPageRow row{}; wcscpy_s(row.Data.cFileName,L"child"); row.Data.dwFileAttributes=FILE_ATTRIBUTE_DIRECTORY; row.FileId[0]=2;
    rows.push_back(row); done=true; return ERROR_SUCCESS;
}
static void Close(void*, void* cursor) { delete static_cast<Cursor*>(cursor); }
static bool Capture(const std::wstring&, DeniedDirectory&) { return false; }
int wmain(int argc, wchar_t** argv)
{
    if (argc!=2) return 2;
    root=std::wstring(argv[1])+L"\\fallback-"+std::to_wstring(GetCurrentProcessId()); child=root+L"\\child";
    const std::wstring file=child+L"\\ordinary.txt";
    if (!CreateDirectoryW(root.c_str(),nullptr) || !CreateDirectoryW(child.c_str(),nullptr)) return 3;
    HANDLE item=CreateFileW(file.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (item==INVALID_HANDLE_VALUE) return 4; CloseHandle(item);
    HANDLE held=CreateFileW(child.c_str(),DELETE|FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if (held==INVALID_HANDLE_VALUE) return 5;
    HANDLE strict=CreateFileW(child.c_str(),FILE_LIST_DIRECTORY|FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,
        nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    DWORD strictError = strict==INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;
    if (strict!=INVALID_HANDLE_VALUE) CloseHandle(strict);
    WIN32_FIND_DATAW row{}; HANDLE ordinary=FindFirstFileW((child+L"\\*").c_str(),&row);
    const bool ordinaryWorks=ordinary!=INVALID_HANDLE_VALUE;
    if (ordinaryWorks) FindClose(ordinary);
    Scan scan; scan.SetLocalPager({Open,Next,Close}); scan.SetCaptureDenied(Capture);
    const bool started=scan.Start(root,false) && scan.WaitForTest(5000);
    std::vector<Entry> entries; Progress progress; scan.Drain(entries,progress);
    std::vector<DeniedDirectory> denied; scan.DrainDenied(denied);
    const bool success=strictError==ERROR_SHARING_VIOLATION && ordinaryWorks && started && entries.size()==1 &&
        entries[0].FullPath()==file && progress.Errors==0 && denied.empty();
    std::printf("%s: real DELETE holder blocks identity cursor but ordinary readable entry remains available without UAC\n",success?"PASS":"FAIL");
    CloseHandle(held);
    if (!DeleteFileW(file.c_str()) || !RemoveDirectoryW(child.c_str()) || !RemoveDirectoryW(root.c_str())) return 6;
    root=std::wstring(argv[1])+L"\\root-held-"+std::to_wstring(GetCurrentProcessId());
    const std::wstring rootFile=root+L"\\root-readable.txt";
    if (!CreateDirectoryW(root.c_str(),nullptr)) return 7;
    item=CreateFileW(rootFile.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (item==INVALID_HANDLE_VALUE) return 8; CloseHandle(item);
    held=CreateFileW(root.c_str(),DELETE|FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if (held==INVALID_HANDLE_VALUE) return 9;
    strict=CreateFileW(root.c_str(),FILE_LIST_DIRECTORY|FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,
        nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    strictError=strict==INVALID_HANDLE_VALUE?GetLastError():ERROR_SUCCESS;
    if (strict!=INVALID_HANDLE_VALUE) CloseHandle(strict);
    holdRoot=true;
    Scan rootScan; rootScan.SetLocalPager({Open,Next,Close}); rootScan.SetCaptureDenied(Capture);
    const bool rootStarted=rootScan.Start(root,false) && rootScan.WaitForTest(5000);
    std::vector<Entry> rootEntries; Progress rootProgress; rootScan.Drain(rootEntries,rootProgress);
    std::vector<DeniedDirectory> rootDenied; rootScan.DrainDenied(rootDenied);
    const bool rootSuccess=strictError==ERROR_SHARING_VIOLATION && rootStarted && rootEntries.size()==1 &&
        rootEntries[0].FullPath()==rootFile && rootProgress.Errors==0 && rootDenied.empty();
    std::printf("%s: real DELETE holder on root preserves ordinary readable listing without UAC\n",rootSuccess?"PASS":"FAIL");
    CloseHandle(held);
    if (!DeleteFileW(rootFile.c_str()) || !RemoveDirectoryW(root.c_str())) return 10;
    return success&&rootSuccess?0:1;
}
