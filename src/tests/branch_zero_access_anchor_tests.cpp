// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Probe zero-access anchors for denied directories. | Do not infer safe UAC identity reuse from a name alone. | #731 | GPT-6.1 Terra |
#include "../uacbroker/listing.h"
#include <aclapi.h>
#include <sddl.h>
#include <cstdio>
#include <memory>
#include <string>
static HANDLE OpenDir(const std::wstring& path, DWORD access, DWORD sharing)
{
    return CreateFileW(path.c_str(), access, sharing, nullptr, OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_OVERLAPPED, nullptr);
}
int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 2;
    const std::wstring root=std::wstring(argv[1])+L"\\anchor-"+std::to_wstring(GetCurrentProcessId());
    const std::wstring child=root+L"\\child", moved=root+L"\\moved";
    if (!CreateDirectoryW(root.c_str(),nullptr) || !CreateDirectoryW(child.c_str(),nullptr)) return 3;
    HANDLE parent=OpenDir(root, FILE_LIST_DIRECTORY|FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE);
    HANDLE control=OpenDir(child, READ_CONTROL|WRITE_DAC|FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE);
    if (parent==INVALID_HANDLE_VALUE || control==INVALID_HANDLE_VALUE) return 4;
    PACL originalDacl=nullptr; PSECURITY_DESCRIPTOR original=nullptr;
    if (GetSecurityInfo(control,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,nullptr,nullptr,&originalDacl,nullptr,&original)) return 5;
    SECURITY_DESCRIPTOR_CONTROL flags{}; DWORD revision=0;
    if (!GetSecurityDescriptorControl(original,&flags,&revision)) return 6;
    PSECURITY_DESCRIPTOR deny=nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(D;;0x00000081;;;WD)(A;;FA;;;WD)",SDDL_REVISION_1,&deny,nullptr)) return 7;
    BOOL present=FALSE,defaulted=FALSE; PACL deniedDacl=nullptr;
    if (!GetSecurityDescriptorDacl(deny,&present,&deniedDacl,&defaulted) || !present ||
        SetSecurityInfo(control,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION,nullptr,nullptr,deniedDacl,nullptr)) return 8;
    HANDLE normal=OpenDir(child, FILE_LIST_DIRECTORY|FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE);
    const DWORD normalError=normal==INVALID_HANDLE_VALUE?GetLastError():ERROR_SUCCESS;
    if (normal!=INVALID_HANDLE_VALUE) CloseHandle(normal);
    FILE_ID_128 fromParent{}; const DWORD parentError=UacListing::CaptureChildDirectoryId(parent,L"child",fromParent);
    HANDLE anchor=OpenDir(child, 0, FILE_SHARE_READ|FILE_SHARE_WRITE);
    const DWORD anchorError=anchor==INVALID_HANDLE_VALUE?GetLastError():ERROR_SUCCESS;
    FILE_ID_INFO id{}; const bool identity=anchor!=INVALID_HANDLE_VALUE && GetFileInformationByHandleEx(anchor,FileIdInfo,&id,sizeof(id));
    const DWORD identityError=identity?ERROR_SUCCESS:GetLastError();
    const bool matches=identity && !parentError && !memcmp(id.FileId.Identifier,fromParent.Identifier,sizeof(id.FileId.Identifier));
    FILE_ATTRIBUTE_TAG_INFO tag{};
    const bool attributes=anchor!=INVALID_HANDLE_VALUE &&
        GetFileInformationByHandleEx(anchor,FileAttributeTagInfo,&tag,sizeof(tag)) &&
        (tag.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(tag.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
    wchar_t fs[16]{};
    const bool volumeInfo=anchor!=INVALID_HANDLE_VALUE &&
        GetVolumeInformationByHandleW(anchor,nullptr,0,nullptr,nullptr,nullptr,fs,ARRAYSIZE(fs)) && _wcsicmp(fs,L"NTFS")==0;
    const DWORD restore=SetSecurityInfo(control,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION|
        ((flags&SE_DACL_PROTECTED)?PROTECTED_DACL_SECURITY_INFORMATION:UNPROTECTED_DACL_SECURITY_INFORMATION),
        nullptr,nullptr,originalDacl,nullptr);
    LocalFree(deny); LocalFree(original);
    if (restore) return 9;
    FILE_BASIC_INFO originalBasic{};
    const bool capturedBasic=GetFileInformationByHandleEx(control,FileBasicInfo,&originalBasic,sizeof(originalBasic))!=FALSE;
    BOOL renamed=FALSE; DWORD renameError=0;
    if (anchor!=INVALID_HANDLE_VALUE) { renamed=MoveFileW(child.c_str(),moved.c_str()); renameError=renamed?0:GetLastError(); }
    bool replacementCreated=false, originalStillSame=false, replacementDiffers=false, removedWhileOpen=false;
    bool matchingMetadata=false, deletePending=false, noReusedId=true;
    unsigned attempts=0;
    DWORD removeError=0;
    if (renamed)
    {
        replacementCreated=CreateDirectoryW(child.c_str(),nullptr)!=FALSE;
        FILE_ID_INFO oldAfter{}, replacement{};
        HANDLE other=OpenDir(child,FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE);
        originalStillSame=GetFileInformationByHandleEx(anchor,FileIdInfo,&oldAfter,sizeof(oldAfter)) &&
            oldAfter.VolumeSerialNumber==id.VolumeSerialNumber && !memcmp(oldAfter.FileId.Identifier,id.FileId.Identifier,sizeof(id.FileId.Identifier));
        replacementDiffers=other!=INVALID_HANDLE_VALUE && GetFileInformationByHandleEx(other,FileIdInfo,&replacement,sizeof(replacement)) &&
            (replacement.VolumeSerialNumber!=id.VolumeSerialNumber || memcmp(replacement.FileId.Identifier,id.FileId.Identifier,sizeof(id.FileId.Identifier)));
        if (other!=INVALID_HANDLE_VALUE) CloseHandle(other);
        removedWhileOpen=RemoveDirectoryW(moved.c_str())!=FALSE;
        if (!removedWhileOpen) removeError=GetLastError();
        FILE_STANDARD_INFO standard{};
        deletePending=GetFileInformationByHandleEx(anchor,FileStandardInfo,&standard,sizeof(standard)) && standard.DeletePending;
        if (replacementCreated && capturedBasic)
        {
            HANDLE current=OpenDir(child,FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE);
            FILE_BASIC_INFO observed{};
            matchingMetadata=current!=INVALID_HANDLE_VALUE &&
                SetFileInformationByHandle(current,FileBasicInfo,&originalBasic,sizeof(originalBasic)) &&
                GetFileInformationByHandleEx(current,FileBasicInfo,&observed,sizeof(observed)) &&
                observed.CreationTime.QuadPart==originalBasic.CreationTime.QuadPart &&
                observed.LastWriteTime.QuadPart==originalBasic.LastWriteTime.QuadPart && observed.FileAttributes==originalBasic.FileAttributes;
            if (current!=INVALID_HANDLE_VALUE) CloseHandle(current);
            for (; attempts!=256 && noReusedId; ++attempts)
            {
                if (!RemoveDirectoryW(child.c_str()) || !CreateDirectoryW(child.c_str(),nullptr)) { noReusedId=false; break; }
                current=OpenDir(child,FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE);
                FILE_ID_INFO candidate{};
                if (current==INVALID_HANDLE_VALUE) { noReusedId=false; break; }
                const bool checked=SetFileInformationByHandle(current,FileBasicInfo,&originalBasic,sizeof(originalBasic)) &&
                    GetFileInformationByHandleEx(current,FileIdInfo,&candidate,sizeof(candidate));
                CloseHandle(current);
                if (!checked || (candidate.VolumeSerialNumber==id.VolumeSerialNumber &&
                    !memcmp(candidate.FileId.Identifier,id.FileId.Identifier,sizeof(id.FileId.Identifier)))) noReusedId=false;
            }
        }
    }
    std::printf("normalError=%lu parentCapture=%lu zeroOpen=%lu zeroIdentity=%lu parentIdMatches=%d renameWithAnchor=%d renameError=%lu oldIdStable=%d replacementDifferent=%d removedWhileOpen=%d removeError=%lu deletePending=%d matchingMetadata=%d attempts=%u noReusedId=%d zeroVolumeInfo=%d zeroAttributes=%d\n",
        normalError,parentError,anchorError,identityError,matches?1:0,renamed?1:0,renameError,originalStillSame?1:0,replacementDiffers?1:0,removedWhileOpen?1:0,removeError,deletePending?1:0,matchingMetadata?1:0,attempts,noReusedId?1:0,volumeInfo?1:0,attributes?1:0);
    if (anchor!=INVALID_HANDLE_VALUE) CloseHandle(anchor);
    CloseHandle(control); CloseHandle(parent);
    bool cleaned=true;
    if (renamed) { if (replacementCreated) cleaned=RemoveDirectoryW(child.c_str())!=FALSE; if (!removedWhileOpen) cleaned=(RemoveDirectoryW(moved.c_str())!=FALSE)&&cleaned; }
    else cleaned=RemoveDirectoryW(child.c_str())!=FALSE;
    cleaned=(RemoveDirectoryW(root.c_str())!=FALSE)&&cleaned;
    if (!cleaned) return 10;
    return normalError==ERROR_ACCESS_DENIED && !parentError && (!renamed || (replacementCreated && originalStillSame && replacementDiffers && matchingMetadata && noReusedId && attempts==256)) ? 0 : 11;
}
