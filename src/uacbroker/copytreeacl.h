/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Define a mandatory integrity profile for tree staging. | Protect working objects while retaining the target's inherited DACL. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include <windows.h>
#include <aclapi.h>
#include <sddl.h>
#include <cstddef>
#include <cstring>
#include <memory>

namespace UacCopyTree
{
namespace Detail
{
class StageProtection
{
public:
    explicit StageProtection(bool high) : rid(high ? SECURITY_MANDATORY_HIGH_RID : SECURITY_MANDATORY_MEDIUM_RID) {}

    DWORD Prepare(HANDLE target)
    {
        DWORD error = Check(target, true);
        if (error)
            return error;
        const bool high = rid == SECURITY_MANDATORY_HIGH_RID;
        error = MakeDescriptor(high ? L"S:(ML;OICI;NWNR;;;HI)" : L"S:(ML;OICI;NWNR;;;ME)", directory);
        if (error)
            return error;
        return MakeDescriptor(high ? L"S:(ML;;NWNR;;;HI)" : L"S:(ML;;NWNR;;;ME)", file);
    }

    DWORD Check(HANDLE object, bool isDirectory) const
    {
        PSECURITY_DESCRIPTOR raw = nullptr;
        PACL label = nullptr;
        const DWORD error = GetSecurityInfo(object, SE_FILE_OBJECT, LABEL_SECURITY_INFORMATION,
                                            nullptr, nullptr, nullptr, &label, &raw);
        const Descriptor descriptor(raw, LocalFree);
        if (error)
            return error;
        void* entry = nullptr;
        if (!label || label->AceCount != 1 || !GetAce(label, 0, &entry))
            return ERROR_NOT_SUPPORTED;
        const auto* ace = static_cast<const SYSTEM_MANDATORY_LABEL_ACE*>(entry);
        const BYTE inheritance = isDirectory ? OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE : 0;
        constexpr ACCESS_MASK policy = SYSTEM_MANDATORY_LABEL_NO_WRITE_UP | SYSTEM_MANDATORY_LABEL_NO_READ_UP;
        if (ace->Header.AceType != SYSTEM_MANDATORY_LABEL_ACE_TYPE ||
            ace->Header.AceSize != offsetof(SYSTEM_MANDATORY_LABEL_ACE, SidStart) + GetSidLengthRequired(1) ||
            (ace->Header.AceFlags & ~INHERITED_ACE) != inheritance || ace->Mask != policy)
            return ERROR_NOT_SUPPORTED;
        SID_IDENTIFIER_AUTHORITY authority = SECURITY_MANDATORY_LABEL_AUTHORITY;
        PSID sid = const_cast<DWORD*>(&ace->SidStart);
        return *GetSidSubAuthorityCount(sid) == 1 && IsValidSid(sid) &&
                       std::memcmp(GetSidIdentifierAuthority(sid), &authority, sizeof(authority)) == 0 &&
                       *GetSidSubAuthority(sid, 0) == rid
                   ? ERROR_SUCCESS
                   : ERROR_NOT_SUPPORTED;
    }

    PSECURITY_DESCRIPTOR Directory() const { return directory.get(); }
    PSECURITY_DESCRIPTOR File() const { return file.get(); }

private:
    using Descriptor = std::unique_ptr<void, decltype(&LocalFree)>;

    static DWORD MakeDescriptor(const wchar_t* text, Descriptor& descriptor)
    {
        PSECURITY_DESCRIPTOR raw = nullptr;
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(text, SDDL_REVISION_1, &raw, nullptr))
            return GetLastError();
        descriptor.reset(raw);
        return ERROR_SUCCESS;
    }

    DWORD rid;
    Descriptor directory{nullptr, LocalFree};
    Descriptor file{nullptr, LocalFree};
};
}
}
