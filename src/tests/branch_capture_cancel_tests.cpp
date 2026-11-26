// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Exercise cancellation during denied-directory identity capture. | Release anchors that finish after Stop rather than enqueue them. | #731 | GPT-6.1 Terra |
#include "../branch_view.h"
#include <aclapi.h>
#include <sddl.h>
#include <cstdio>
#include <memory>
using namespace Salamander::BranchView;
static HANDLE entered, resumeCapture, released;
static bool Capture(const std::wstring&, DeniedDirectory& denied)
{
    denied.Anchor = std::shared_ptr<void>(reinterpret_cast<void*>(1), [](void*) { SetEvent(released); });
    SetEvent(entered);
    WaitForSingleObject(resumeCapture, 5000);
    return true;
}
int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 2;
    std::wstring child = std::wstring(argv[1]) + L"\\denied-" + std::to_wstring(GetCurrentProcessId());
    if (!CreateDirectoryW(child.c_str(), nullptr)) return 3;
    HANDLE control = CreateFileW(child.c_str(), READ_CONTROL | WRITE_DAC,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (control == INVALID_HANDLE_VALUE) return 4;
    PACL oldAcl = nullptr; PSECURITY_DESCRIPTOR oldSd = nullptr;
    if (GetSecurityInfo(control, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &oldAcl, nullptr, &oldSd)) return 5;
    SECURITY_DESCRIPTOR_CONTROL flags{}; DWORD revision = 0;
    if (!GetSecurityDescriptorControl(oldSd, &flags, &revision)) return 6;
    PSECURITY_DESCRIPTOR deny = nullptr; PACL acl = nullptr; BOOL present = FALSE, defaulted = FALSE;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(D;;0x00000081;;;WD)(A;;FA;;;WD)", SDDL_REVISION_1, &deny, nullptr) ||
        !GetSecurityDescriptorDacl(deny, &present, &acl, &defaulted) || !present ||
        SetSecurityInfo(control, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, nullptr, nullptr, acl, nullptr)) return 7;
    entered = CreateEventW(nullptr, TRUE, FALSE, nullptr); resumeCapture = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    released = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    Scan scan; scan.SetCaptureDenied(Capture);
    bool ok = entered && resumeCapture && released && scan.Start(child, false) && WaitForSingleObject(entered, 5000) == WAIT_OBJECT_0;
    scan.Cancel(); SetEvent(resumeCapture);
    ok = ok && WaitForSingleObject(released, 5000) == WAIT_OBJECT_0;
    std::printf("%s: Cancel while capture is pending releases late anchor\n", ok ? "PASS" : "FAIL");
    const DWORD restore = SetSecurityInfo(control, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION |
        ((flags & SE_DACL_PROTECTED) ? PROTECTED_DACL_SECURITY_INFORMATION : UNPROTECTED_DACL_SECURITY_INFORMATION),
        nullptr, nullptr, oldAcl, nullptr);
    LocalFree(deny); LocalFree(oldSd); CloseHandle(control);
    if (entered) CloseHandle(entered); if (resumeCapture) CloseHandle(resumeCapture); if (released) CloseHandle(released);
    RemoveDirectoryW(child.c_str());
    return ok && !restore ? 0 : 1;
}
