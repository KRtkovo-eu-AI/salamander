// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <windows.h>
#include <shlobj.h>
#include <urlmon.h>
#ifdef SALAMANDER_BRANCHPROGRAM_CLIENT
#include <string>
#endif

// Private host/salopen protocol. The selected program is never passed to raw
// CreateProcess: Attachment Services owns authorization AND actual execution.
namespace Salamander { namespace BranchProgram {
const DWORD Magic = 0x33504742;
const DWORD MaxSourceChars = 32767;
struct Request
{
    DWORD Bytes;
    DWORD Signature;
    DWORD SourceChars;
    HRESULT Result;
    DWORD Stage;
    ULONG_PTR Owner;
    ULONG_PTR File;
    ULONG_PTR Directory;
    // SourceChars UTF16 characters and a terminating NUL follow this header.
};
inline bool IsProgram(const wchar_t* source)
{
    if (source == NULL) return false;
    size_t length = 0;
    while (source[length] != 0) ++length;
    if (length < 4 || source[length - 4] != L'.') return false;
    const wchar_t* extension = source + length - 3;
    return (extension[0] == L'e' || extension[0] == L'E') &&
           (extension[1] == L'x' || extension[1] == L'X') &&
           (extension[2] == L'e' || extension[2] == L'E');
}
inline HRESULT LastErrorResult()
{
    const DWORD error = GetLastError();
    return HRESULT_FROM_WIN32(error == ERROR_SUCCESS ? ERROR_GEN_FAILURE : error);
}
inline bool SameObject(HANDLE first, HANDLE second)
{
    BY_HANDLE_FILE_INFORMATION a, b;
    return GetFileInformationByHandle(first, &a) && GetFileInformationByHandle(second, &b) &&
           a.dwVolumeSerialNumber == b.dwVolumeSerialNumber &&
           a.nFileIndexHigh == b.nFileIndexHigh && a.nFileIndexLow == b.nFileIndexLow;
}
#ifdef SALAMANDER_BRANCHPROGRAM_CLIENT
struct OwnedHandle
{
    HANDLE Value;
    explicit OwnedHandle(HANDLE value) : Value(value) {}
    ~OwnedHandle() { Reset(); }
    void Reset() { if (Value != NULL && Value != INVALID_HANDLE_VALUE) CloseHandle(Value); Value = INVALID_HANDLE_VALUE; }
private:
    OwnedHandle(const OwnedHandle&);
    OwnedHandle& operator=(const OwnedHandle&);
};
#endif
inline HRESULT ValidateSource(const wchar_t* source, const wchar_t*& plain, DWORD& length)
{
    if (!IsProgram(source)) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    plain = source;
    if (source[0] == L'\\' && source[1] == L'\\' && source[2] == L'?' && source[3] == L'\\') plain += 4;
    // No UNC, device, stream, relative or Shell-normalized alternative identity.
    if (!((plain[0] >= L'A' && plain[0] <= L'Z') || (plain[0] >= L'a' && plain[0] <= L'z')) ||
        plain[1] != L':' || plain[2] != L'\\') return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    length = 0;
    while (plain[length] != 0 && length <= MaxSourceChars) ++length;
    if (length > MaxSourceChars) return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    for (DWORD i = 3; i < length; ++i)
        if (plain[i] < L' ' || plain[i] == L':' || plain[i] == L'/' || plain[i] == L'%' || plain[i] == L'#' ||
            ((plain[i] == L'.' || plain[i] == L' ') && (i + 1 == length || plain[i + 1] == L'\\')))
            return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    return S_OK;
}
inline HRESULT Execute(HWND owner, const wchar_t* source, DWORD* stage = NULL,
                       HANDLE* selectedFile = NULL, HANDLE selectedDirectory = INVALID_HANDLE_VALUE)
{
    HANDLE file = selectedFile == NULL ? INVALID_HANDLE_VALUE : *selectedFile;
    HANDLE expectedDirectory = selectedDirectory;
    if (stage != NULL) *stage = 1;
    const wchar_t* plain = NULL;
    DWORD length = 0;
    const HRESULT syntax = ValidateSource(source, plain, length);
    if (FAILED(syntax)) return syntax;
    wchar_t* native = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, (length + 5) * sizeof(wchar_t));
    wchar_t* parent = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, (length + 5) * sizeof(wchar_t));
    wchar_t* current = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, (MaxSourceChars + 1) * sizeof(wchar_t));
    if (native == NULL || parent == NULL || current == NULL)
    {
        if (native != NULL) HeapFree(GetProcessHeap(), 0, native);
        if (parent != NULL) HeapFree(GetProcessHeap(), 0, parent);
        if (current != NULL) HeapFree(GetProcessHeap(), 0, current);
        return E_OUTOFMEMORY;
    }
    HANDLE actualDirectory = INVALID_HANDLE_VALUE;
    IInternetSecurityManager* security = NULL;
    IAttachmentExecute* attachment = NULL;
    HANDLE child = NULL;
    HRESULT result = E_FAIL;
    do
    {
        if (stage != NULL) *stage = 2;
        // The host pins the selected objects before starting this helper. Paths
        // from the shared packet are comparisons only, never file-open inputs.
        if (file == NULL || file == INVALID_HANDLE_VALUE ||
            expectedDirectory == NULL || expectedDirectory == INVALID_HANDLE_VALUE ||
            GetFileType(file) != FILE_TYPE_DISK || GetFileType(expectedDirectory) != FILE_TYPE_DISK)
        { result = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE); break; }
        BY_HANDLE_FILE_INFORMATION selectedInfo;
        if (!GetFileInformationByHandle(file, &selectedInfo) ||
            (selectedInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        { result = HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT); break; }
        const DWORD finalLength = GetFinalPathNameByHandleW(file, native, length + 5, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        const DWORD parentLength = GetFinalPathNameByHandleW(expectedDirectory, parent, length + 5, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        if (parentLength == 0 || parentLength >= length + 5)
        { result = HRESULT_FROM_WIN32(ERROR_DIRECTORY); break; }
        if (finalLength == 0 || finalLength >= length + 5 ||
            finalLength < 4 || native[0] != L'\\' || native[1] != L'\\' || native[2] != L'?' || native[3] != L'\\' ||
            finalLength != length + 4 || CompareStringOrdinal(native + 4, (int)length, plain, (int)length, TRUE) != CSTR_EQUAL)
        { result = HRESULT_FROM_WIN32(ERROR_INVALID_NAME); break; }
        // Exact component boundary, not a textual directory-prefix allowance.
        DWORD parentEnd = parentLength;
        if (parentEnd != 0 && parent[parentEnd - 1] == L'\\') --parentEnd;
        DWORD fileParentEnd = finalLength;
        while (fileParentEnd != 0 && native[fileParentEnd - 1] != L'\\') --fileParentEnd;
        if (fileParentEnd == 0 || fileParentEnd - 1 != parentEnd ||
            CompareStringOrdinal(native, (int)parentEnd, parent, (int)parentEnd, TRUE) != CSTR_EQUAL)
        { result = HRESULT_FROM_WIN32(ERROR_DIRECTORY); break; }
        // Use only the kernel-resolved identity for binary and Windows trust APIs.
        plain = native + 4;
        DWORD binaryType;
        if (!GetBinaryTypeW(native, &binaryType)) { result = LastErrorResult(); break; }
        if (binaryType != SCS_32BIT_BINARY && binaryType != SCS_64BIT_BINARY)
        { result = HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT); break; }
        if (stage != NULL) *stage = 3;
        const DWORD count = GetCurrentDirectoryW(MaxSourceChars + 1, current);
        if (count == 0 || count > MaxSourceChars)
        { result = HRESULT_FROM_WIN32(ERROR_DIRECTORY); break; }
        actualDirectory = CreateFileW(current, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (actualDirectory == INVALID_HANDLE_VALUE || !SameObject(expectedDirectory, actualDirectory))
        { result = HRESULT_FROM_WIN32(ERROR_DIRECTORY); break; }
        if (stage != NULL) *stage = 4;
        result = CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IInternetSecurityManager, (void**)&security);
        if (FAILED(result)) break;
        DWORD zone = (DWORD)URLZONE_INVALID;
        if (stage != NULL) *stage = 5;
        result = security->MapUrlToZone(plain, &zone, MUTZ_ISFILE | MUTZ_REQUIRESAVEDFILECHECK);
        if (FAILED(result)) break;
        // Saved-file-aware Windows classification; never assign our own zone or
        // replace an Internet/unknown origin with a trusted local origin.
        if (zone != URLZONE_LOCAL_MACHINE)
        { result = HRESULT_FROM_WIN32(ERROR_ACCESS_DISABLED_BY_POLICY); break; }
        if (stage != NULL) *stage = 6;
        result = CoCreateInstance(CLSID_AttachmentServices, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IAttachmentExecute, (void**)&attachment);
        if (FAILED(result)) break;
        // Stable client identity for Windows Attachment Services preferences.
        const GUID client = {0x71991a5c, 0xc82e, 0x4cc8, {0xa7, 0x5d, 0x03, 0x53, 0xd0, 0x55, 0x91, 0xa6}};
        result = attachment->SetClientGuid(client);
        if (FAILED(result)) break;
        if (stage != NULL) *stage = 7;
        result = attachment->SetLocalPath(plain);
        if (FAILED(result)) break;
        // The actual existing source, already checked WITH its saved zone. No
        // fabricated download/referrer URL and no file/ADS rewriting by us.
        if (stage != NULL) *stage = 8;
        result = attachment->SetSource(plain);
        if (FAILED(result)) break;
        if (stage != NULL) *stage = 9;
        result = attachment->CheckPolicy();
        if (result != S_OK)
        { if (SUCCEEDED(result)) result = HRESULT_FROM_WIN32(ERROR_ACCESS_DISABLED_BY_POLICY); break; }
        // Do not substitute CheckPolicy + CreateProcess for this operation:
        // Execute can invoke scanners, trust services and required Windows UI.
        if (stage != NULL) *stage = 10;
        // Windows trust services may annotate or quarantine the file. Release
        // our read lock for their final validation; this is not an atomic file
        // system transaction and no alternate execution follows a failure.
        CloseHandle(file); file = INVALID_HANDLE_VALUE; *selectedFile = INVALID_HANDLE_VALUE;
        result = attachment->Execute(owner, L"open", &child);
        if (SUCCEEDED(result) && (result != S_OK || child == NULL)) result = E_FAIL;
    } while (false);
    if (child != NULL) CloseHandle(child);
    if (attachment != NULL) attachment->Release();
    if (security != NULL) security->Release();
    if (actualDirectory != INVALID_HANDLE_VALUE) CloseHandle(actualDirectory);
    HeapFree(GetProcessHeap(), 0, current);
    HeapFree(GetProcessHeap(), 0, parent);
    HeapFree(GetProcessHeap(), 0, native);
    return result;
}
inline DWORD Receive(DWORD processId, HANDLE remoteMapping, DWORD bytes)
{
    if (bytes < sizeof(Request) + sizeof(wchar_t) || bytes > sizeof(Request) + (MaxSourceChars + 1) * sizeof(wchar_t))
        return ERROR_INVALID_DATA;
    HANDLE process = OpenProcess(PROCESS_DUP_HANDLE, FALSE, processId);
    if (process == NULL) return ERROR_ACCESS_DENIED;
    HANDLE mapping = NULL;
    const BOOL duplicated = DuplicateHandle(process, remoteMapping, GetCurrentProcess(), &mapping, 0, FALSE, DUPLICATE_SAME_ACCESS);
    CloseHandle(process);
    if (!duplicated) return ERROR_INVALID_HANDLE;
    Request* request = (Request*)MapViewOfFile(mapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, bytes);
    if (request == NULL) { CloseHandle(mapping); return ERROR_INVALID_DATA; }
    DWORD result = ERROR_INVALID_DATA;
    // Snapshot once before interpreting the writable cross-process packet.
    Request* packet = (Request*)HeapAlloc(GetProcessHeap(), 0, bytes);
    if (packet != NULL)
    {
        const BYTE* input = (const BYTE*)request;
        BYTE* output = (BYTE*)packet;
        for (DWORD i = 0; i < bytes; ++i) output[i] = input[i];
        if (packet->Signature == Magic && packet->Bytes == bytes && packet->SourceChars <= MaxSourceChars &&
            bytes == sizeof(Request) + (packet->SourceChars + 1) * sizeof(wchar_t))
        {
            const wchar_t* source = (const wchar_t*)(packet + 1);
            DWORD length = 0;
            while (length < packet->SourceChars && source[length] != 0) ++length;
            if (length == packet->SourceChars && source[length] == 0 && packet->File != packet->Directory)
            {
                // Only these two handles were explicitly inherited. Do not let a
                // target process inherit them. Receive closes every remaining pin.
                HANDLE file = (HANDLE)packet->File, directory = (HANDLE)packet->Directory;
                SetHandleInformation(file, HANDLE_FLAG_INHERIT, 0);
                SetHandleInformation(directory, HANDLE_FLAG_INHERIT, 0);
                packet->Result = Execute((HWND)packet->Owner, source, &packet->Stage, &file, directory);
                if (file != NULL && file != INVALID_HANDLE_VALUE) CloseHandle(file);
                if (directory != NULL && directory != INVALID_HANDLE_VALUE) CloseHandle(directory);
                request->Stage = packet->Stage;
                request->Result = packet->Result;
                result = ERROR_SUCCESS;
            }
        }
        HeapFree(GetProcessHeap(), 0, packet);
    }
    UnmapViewOfFile(request);
    CloseHandle(mapping);
    return result;
}

#ifdef SALAMANDER_BRANCHPROGRAM_CLIENT
// Only our installed helper is created here; it executes the selected program
// exclusively through Windows Attachment Services, with the requested CWD.
inline HRESULT Launch(HWND owner, const std::wstring& source, const std::wstring& directory,
                      const std::wstring& helper, DWORD* stage = NULL)
{
    if (!IsWindow(owner) || source.empty() || source.size() > MaxSourceChars || directory.empty() || helper.empty())
        return E_INVALIDARG;
    const wchar_t* plain = NULL;
    DWORD sourceLength = 0;
    const HRESULT syntax = ValidateSource(source.c_str(), plain, sourceLength);
    if (FAILED(syntax)) return syntax;
    const DWORD bytes = (DWORD)(sizeof(Request) + (source.size() + 1) * sizeof(wchar_t));
    HANDLE mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, bytes, NULL);
    if (mapping == NULL) return LastErrorResult();
    Request* request = (Request*)MapViewOfFile(mapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, bytes);
    if (request == NULL) { const HRESULT error = LastErrorResult(); CloseHandle(mapping); return error; }
    request->Bytes = bytes; request->Signature = Magic; request->SourceChars = (DWORD)source.size();
    SECURITY_ATTRIBUTES attributes = {sizeof(attributes), NULL, TRUE};
    const std::wstring nativeSource = source.compare(0, 4, L"\\\\?\\") == 0 ? source : L"\\\\?\\" + source;
    OwnedHandle file(CreateFileW(nativeSource.c_str(), GENERIC_READ, FILE_SHARE_READ,
        &attributes, OPEN_EXISTING, 0, NULL));
    if (file.Value == INVALID_HANDLE_VALUE)
    { const HRESULT error = LastErrorResult(); UnmapViewOfFile(request); CloseHandle(mapping); return error; }
    const std::wstring nativeDirectory = directory.compare(0, 4, L"\\\\?\\") == 0 ? directory : L"\\\\?\\" + directory;
    OwnedHandle folder(CreateFileW(nativeDirectory.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        &attributes, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL));
    if (file.Value == INVALID_HANDLE_VALUE || folder.Value == INVALID_HANDLE_VALUE)
    { const HRESULT error = LastErrorResult(); UnmapViewOfFile(request); CloseHandle(mapping); return error; }
    request->File = (ULONG_PTR)file.Value; request->Directory = (ULONG_PTR)folder.Value;
    request->Stage = 0;
    request->Result = E_PENDING; request->Owner = (ULONG_PTR)GetAncestor(owner, GA_ROOT);
    wchar_t* text = (wchar_t*)(request + 1);
    for (size_t i = 0; i <= source.size(); ++i) text[i] = source.c_str()[i];
    std::wstring command = L"\"" + helper + L"\" --branch-program " +
        std::to_wstring(GetCurrentProcessId()) + L" " + std::to_wstring((ULONG_PTR)mapping) +
        L" " + std::to_wstring(bytes);
    SIZE_T attributeBytes = 0;
    InitializeProcThreadAttributeList(NULL, 1, 0, &attributeBytes);
    LPPROC_THREAD_ATTRIBUTE_LIST list = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, attributeBytes);
    HANDLE inherited[] = {file.Value, folder.Value};
    bool initialized = list != NULL && InitializeProcThreadAttributeList(list, 1, 0, &attributeBytes) != FALSE;
    bool configured = initialized && UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        inherited, sizeof(inherited), NULL, NULL) != FALSE;
    STARTUPINFOEXW startup = {}; startup.StartupInfo.cb = sizeof(startup); startup.lpAttributeList = list;
    PROCESS_INFORMATION process = {};
    const BOOL created = configured && CreateProcessW(helper.c_str(), &command[0], NULL, NULL, TRUE,
        CREATE_NO_WINDOW | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT,
        NULL, directory.c_str(), &startup.StartupInfo, &process);
    const HRESULT creationError = LastErrorResult();
    if (initialized) DeleteProcThreadAttributeList(list);
    if (list != NULL) HeapFree(GetProcessHeap(), 0, list);
    // Child has its own pins. Release ours before it can ask Windows to execute,
    // preserving the prior lock through validation without blocking trust services.
    file.Reset(); folder.Reset();
    HRESULT result;
    if (!created) result = creationError;
    else if (ResumeThread(process.hThread) == (DWORD)-1)
    {
        result = LastErrorResult(); TerminateProcess(process.hProcess, ERROR_GEN_FAILURE);
        WaitForSingleObject(process.hProcess, 5000); CloseHandle(process.hThread); CloseHandle(process.hProcess);
    }
    else
    {
        CloseHandle(process.hThread);
        // Service/Shell code can send synchronous messages to its owner. Keep
        // sent messages and painting serviced without dispatching new commands
        // that could invalidate the active panel while its operation is pending.
        DWORD waited = WAIT_TIMEOUT;
        const ULONGLONG deadline = GetTickCount64() + 30000;
        while (GetTickCount64() < deadline)
        {
            waited = MsgWaitForMultipleObjects(1, &process.hProcess, FALSE, 50, QS_SENDMESSAGE | QS_PAINT);
            if (waited == WAIT_OBJECT_0 || waited == WAIT_FAILED) break;
            MSG message;
            while (PeekMessageW(&message, NULL, WM_PAINT, WM_PAINT, PM_REMOVE)) DispatchMessageW(&message);
        }
        if (waited == WAIT_OBJECT_0)
        {
            DWORD code = ERROR_GEN_FAILURE;
            if (GetExitCodeProcess(process.hProcess, &code) && code == ERROR_SUCCESS)
                result = request->Result == E_PENDING ? E_FAIL : request->Result;
            else result = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
        else
        {
            // Cancel our own bounded broker operation, never retry the target.
            TerminateProcess(process.hProcess, ERROR_TIMEOUT);
            WaitForSingleObject(process.hProcess, 5000);
            result = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        }
        CloseHandle(process.hProcess);
    }
    if (stage != NULL) *stage = request->Stage;
    UnmapViewOfFile(request); CloseHandle(mapping);
    return result;
}
#endif
} }
