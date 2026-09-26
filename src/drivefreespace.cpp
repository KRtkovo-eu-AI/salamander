// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "drivefreespace.h"
#include "drivefreespace_cache.h"

#include <winnetwk.h>
#include <shellapi.h>
#include <strsafe.h>
#include <vector>
#include <string>
#include <new>
#include <cwchar>

#pragma comment(lib, "mpr.lib")

namespace
{
using namespace DriveFreeSpaceDetail;
const wchar_t HelperSwitch[] = L"--sal-drive-free-space-helper";
const DWORD MappingMagic = 0x46534453;
const size_t MaximumPayload = 1024 * 1024;

struct SharedProbe
{
    DWORD Magic;
    DWORD Size;
    DWORD DeviceCharacters;
    UINT Type;
    wchar_t Drive;
    volatile LONG Complete;
    DWORD Error;
    ULONGLONG Bytes;
    FILETIME Time;
    // Null-terminated QueryDosDevice identity follows this header.
};

struct Request
{
    UINT Type = 0;
    std::wstring Device;
    HWND Window = nullptr;
    UINT Message = 0;
};

struct Job
{
    HANDLE Process = nullptr;
    HANDLE Mapping = nullptr;
    SharedProbe* Shared = nullptr;
    unsigned Drive = 0;
    std::uint64_t Token = 0;
    bool Terminating = false;
};

struct Service
{
    SRWLOCK Lock = SRWLOCK_INIT;
    Cache Values;
    Request Requests[26];
    HANDLE Wake = nullptr;
    HANDLE Thread = nullptr;
    HANDLE Processes = nullptr;
    bool Stop = false;
    bool Started = false;
};

// No destructor races an exceptional process shutdown. Normal shutdown joins
// the worker and closes its handles; this small process-lifetime object remains.
Service* ServiceInstance = nullptr;
INIT_ONCE ServiceOnce = INIT_ONCE_STATIC_INIT;

BOOL CALLBACK InitializeService(PINIT_ONCE, PVOID, PVOID*)
{
    ServiceInstance = new (std::nothrow) Service;
    return ServiceInstance != nullptr;
}

Service* GetService()
{
    if (!InitOnceExecuteOnce(&ServiceOnce, InitializeService, nullptr, nullptr))
        return nullptr;
    return ServiceInstance;
}

int DriveIndex(wchar_t letter)
{
    if (letter >= L'a' && letter <= L'z')
        letter -= L'a' - L'A';
    return letter >= L'A' && letter <= L'Z' ? letter - L'A' : -1;
}

std::wstring DeviceIdentity(wchar_t letter)
{
    wchar_t name[] = {letter, L':', 0};
    std::vector<wchar_t> buffer(256);
    while (buffer.size() <= MaximumPayload / sizeof(wchar_t))
    {
        if (QueryDosDeviceW(name, buffer.data(), static_cast<DWORD>(buffer.size())))
            return buffer.data();
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            break;
        buffer.resize(buffer.size() * 2);
    }
    return std::wstring();
}

bool ConnectedRemote(wchar_t letter, std::wstring& remote)
{
    wchar_t local[] = {letter, L':', 0};
    std::vector<wchar_t> buffer(256);
    for (;;)
    {
        DWORD size = static_cast<DWORD>(buffer.size());
        DWORD result = WNetGetConnectionW(local, buffer.data(), &size);
        // ERROR_CONNECTION_UNAVAIL also returns a remembered UNC. Do not use
        // it: opening that path can reconnect or cause credential prompts.
        if (result != NO_ERROR)
        {
            if (result == ERROR_MORE_DATA && size > buffer.size() &&
                size <= MaximumPayload / sizeof(wchar_t))
            {
                buffer.resize(size);
                continue;
            }
            SetLastError(result);
            return false;
        }
        remote = buffer.data();
        return !remote.empty();
    }
}

std::wstring ExtendedPath(const std::wstring& path)
{
    if (path.size() < MAX_PATH || path.compare(0, 4, L"\\\\?\\") == 0)
        return path;
    if (path.compare(0, 2, L"\\\\") == 0)
        return L"\\\\?\\UNC\\" + path.substr(2);
    return L"\\\\?\\" + path;
}

bool VolumeIdentity(const std::wstring& root, std::wstring& identity)
{
    std::vector<wchar_t> volume(128);
    if (!GetVolumeNameForVolumeMountPointW(root.c_str(), volume.data(),
                                         static_cast<DWORD>(volume.size())))
        return false;
    DWORD serial = 0;
    if (!GetVolumeInformationW(root.c_str(), nullptr, 0, &serial, nullptr, nullptr, nullptr, 0))
        return false;
    identity = volume.data();
    identity += std::to_wstring(serial);
    return true;
}

void RunProbe(SharedProbe* shared)
{
    const wchar_t* expected = reinterpret_cast<const wchar_t*>(shared + 1);
    std::wstring deviceBefore = DeviceIdentity(shared->Drive);
    if (deviceBefore.empty() || deviceBefore != expected)
    {
        shared->Error = ERROR_NOT_READY;
        return;
    }
    std::wstring root;
    std::wstring identityBefore;
    if (shared->Type == DRIVE_REMOTE)
    {
        if (!ConnectedRemote(shared->Drive, root))
        {
            shared->Error = GetLastError();
            return;
        }
        identityBefore = root;
        if (root.back() != L'\\')
            root += L'\\';
    }
    else
    {
        root += shared->Drive;
        root += L":\\";
        if (!VolumeIdentity(root, identityBefore))
        {
            shared->Error = GetLastError();
            return;
        }
    }

    ULARGE_INTEGER available = {};
    // lpFreeBytesAvailable respects per-user quotas; total free bytes does not.
    if (!GetDiskFreeSpaceExW(ExtendedPath(root).c_str(), &available, nullptr, nullptr))
    {
        shared->Error = GetLastError();
        return;
    }
    std::wstring identityAfter;
    bool stable = shared->Type == DRIVE_REMOTE
                      ? ConnectedRemote(shared->Drive, identityAfter)
                      : VolumeIdentity(root, identityAfter);
    if (!stable || identityBefore != identityAfter || deviceBefore != DeviceIdentity(shared->Drive))
    {
        shared->Error = ERROR_MEDIA_CHANGED;
        return;
    }
    shared->Bytes = available.QuadPart;
    GetSystemTimeAsFileTime(&shared->Time);
    shared->Error = ERROR_SUCCESS;
}

std::wstring ModulePath()
{
    std::vector<wchar_t> buffer(512);
    while (buffer.size() <= 32768)
    {
        DWORD count = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (count == 0)
            break;
        if (count < buffer.size())
            return std::wstring(buffer.data(), count);
        buffer.resize(buffer.size() * 2);
    }
    return std::wstring();
}

void CloseJob(Job& job, bool terminate)
{
    if (job.Process != nullptr)
    {
        if (terminate)
            TerminateProcess(job.Process, ERROR_TIMEOUT);
        CloseHandle(job.Process);
    }
    if (job.Shared != nullptr)
        UnmapViewOfFile(job.Shared);
    if (job.Mapping != nullptr)
        CloseHandle(job.Mapping);
    job = Job();
}

bool StartJob(Service& service, Job& job, const Request& request)
{
    std::wstring module = ModulePath();
    size_t bytes = sizeof(SharedProbe) + (request.Device.size() + 1) * sizeof(wchar_t);
    if (module.empty() || bytes > MaximumPayload)
        return false;
    SECURITY_ATTRIBUTES security = {sizeof(security), nullptr, TRUE};
    job.Mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, &security, PAGE_READWRITE, 0,
                                    static_cast<DWORD>(bytes), nullptr);
    if (job.Mapping == nullptr)
        return false;
    job.Shared = static_cast<SharedProbe*>(MapViewOfFile(job.Mapping, FILE_MAP_ALL_ACCESS, 0, 0, bytes));
    if (job.Shared == nullptr)
        return false;
    ZeroMemory(job.Shared, bytes);
    job.Shared->Magic = MappingMagic;
    job.Shared->Size = static_cast<DWORD>(bytes);
    job.Shared->DeviceCharacters = static_cast<DWORD>(request.Device.size() + 1);
    job.Shared->Drive = static_cast<wchar_t>(L'A' + job.Drive);
    job.Shared->Type = request.Type;
    job.Shared->Error = ERROR_IO_PENDING;
    memcpy(job.Shared + 1, request.Device.c_str(), (request.Device.size() + 1) * sizeof(wchar_t));

    wchar_t handleText[2 * sizeof(ULONG_PTR) + 1] = {};
    StringCchPrintfW(handleText, _countof(handleText), L"%llx",
                     static_cast<unsigned long long>(reinterpret_cast<ULONG_PTR>(job.Mapping)));
    std::wstring command = L"\"" + module + L"\" " + HelperSwitch + L" " + handleText;
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(0);
    SIZE_T attributeBytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
    std::vector<unsigned char> attributes(attributeBytes);
    STARTUPINFOEXW startup = {};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startup.StartupInfo.wShowWindow = SW_HIDE;
    startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &attributeBytes))
        return false;
    bool inherited = UpdateProcThreadAttribute(startup.lpAttributeList, 0,
                                              PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &job.Mapping,
                                              sizeof(job.Mapping), nullptr, nullptr) != FALSE;
    PROCESS_INFORMATION process = {};
    // The helper uses only absolute paths. Inherit the already-valid host CWD:
    // CreateProcess rejects a >MAX_PATH lpCurrentDirectory even when its
    // explicit extended-length application path is accepted.
    bool created = inherited && CreateProcessW(ExtendedPath(module).c_str(), mutableCommand.data(),
                                               nullptr, nullptr, TRUE,
                                               CREATE_SUSPENDED | CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT,
                                               nullptr, nullptr,
                                               &startup.StartupInfo, &process) != FALSE;
    DeleteProcThreadAttributeList(startup.lpAttributeList);
    if (!created)
        return false;
    job.Process = process.hProcess;
    bool assigned = AssignProcessToJobObject(service.Processes, process.hProcess) != FALSE;
    if (assigned)
        assigned = ResumeThread(process.hThread) != static_cast<DWORD>(-1);
    CloseHandle(process.hThread);
    return assigned;
}

DWORD WINAPI ServiceThread(void* parameter)
{
    Service& service = *static_cast<Service*>(parameter);
    Job jobs[MaxProbes];
    for (;;)
    {
        AcquireSRWLockExclusive(&service.Lock);
        bool stop = service.Stop;
        ReleaseSRWLockExclusive(&service.Lock);
        if (stop)
            break;
        for (Job& job : jobs)
        {
            if (job.Process == nullptr)
                continue;
            ULONGLONG now = GetTickCount64();
            bool finished = WaitForSingleObject(job.Process, 0) == WAIT_OBJECT_0;
            if (job.Terminating)
            {
                if (finished)
                    CloseJob(job, false);
                continue;
            }
            AcquireSRWLockExclusive(&service.Lock);
            bool current = service.Values.IsCurrent(job.Drive, job.Token);
            bool expired = service.Values.Expired(job.Drive, job.Token, now);
            if (current && (finished || expired))
            {
                bool success = finished && !expired && job.Shared->Complete == 1 && job.Shared->Error == ERROR_SUCCESS;
                ULARGE_INTEGER time = {};
                time.LowPart = job.Shared->Time.dwLowDateTime;
                time.HighPart = job.Shared->Time.dwHighDateTime;
                service.Values.Complete(job.Drive, job.Token, success, job.Shared->Bytes, now, time.QuadPart);
                const Request& request = service.Requests[job.Drive];
                // Shutdown takes the same lock before the host HWND can be
                // destroyed. Never post a captured handle after dropping it.
                if (!service.Stop && request.Window != nullptr && request.Message != 0)
                    PostMessageW(request.Window, request.Message, 0, 0);
            }
            ReleaseSRWLockExclusive(&service.Lock);
            if (!current || finished || expired)
            {
                if (finished)
                    CloseJob(job, false);
                else
                {
                    TerminateProcess(job.Process, ERROR_TIMEOUT);
                    // Keep this slot occupied until Windows confirms exit.
                    // A broken provider must never create an unbounded chain
                    // of timed-out processes still stuck in kernel I/O.
                    job.Terminating = true;
                }
            }
        }
        for (Job& job : jobs)
        {
            if (job.Process != nullptr)
                continue;
            AcquireSRWLockExclusive(&service.Lock);
            int drive = service.Stop ? -1 : service.Values.Reserve(GetTickCount64());
            Request request;
            if (drive >= 0)
            {
                job.Drive = static_cast<unsigned>(drive);
                job.Token = service.Values.Get(job.Drive).Token;
                request = service.Requests[job.Drive];
            }
            ReleaseSRWLockExclusive(&service.Lock);
            if (drive < 0)
                break;
            if (!StartJob(service, job, request))
            {
                AcquireSRWLockExclusive(&service.Lock);
                bool completed = service.Values.Complete(job.Drive, job.Token, false, 0, GetTickCount64(), 0);
                if (completed && !service.Stop && request.Window != nullptr && request.Message != 0)
                    PostMessageW(request.Window, request.Message, 0, 0);
                ReleaseSRWLockExclusive(&service.Lock);
                if (job.Process != nullptr)
                {
                    TerminateProcess(job.Process, ERROR_TIMEOUT);
                    job.Terminating = true;
                }
                else
                    CloseJob(job, false);
            }
        }
        bool active = false;
        for (const Job& job : jobs)
            active = active || job.Process != nullptr;
        WaitForSingleObject(service.Wake, active ? 25 : INFINITE);
    }
    for (Job& job : jobs)
        CloseJob(job, true);
    return 0;
}

bool StartService(Service& service)
{
    if (service.Started)
        return service.Thread != nullptr && !service.Stop;
    service.Started = true;
    service.Wake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    service.Processes = CreateJobObjectW(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (service.Wake == nullptr || service.Processes == nullptr ||
        !SetInformationJobObject(service.Processes, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
        return false;
    service.Thread = CreateThread(nullptr, 0, ServiceThread, &service, 0, nullptr);
    return service.Thread != nullptr;
}
}

DriveFreeSpaceValue DriveFreeSpaceQuery(wchar_t letter, UINT type, const wchar_t* remoteUNC,
                                       bool disconnected, DriveFreeSpaceMode mode, HWND window, UINT message)
{
    DriveFreeSpaceValue value;
    if (mode != dfsOnce && mode != dfsUpdated)
        mode = dfsOff;
    int drive = DriveIndex(letter);
    if (drive < 0 || (type != DRIVE_REMOVABLE && type != DRIVE_REMOTE))
        return value;
    Service* service = GetService();
    if (service == nullptr)
        return value;
    // QueryDosDevice reads the object-manager namespace, not the drive/media.
    std::wstring device = mode == dfsOff ? std::wstring() : DeviceIdentity(static_cast<wchar_t>(L'A' + drive));
    std::wstring identity = std::to_wstring(type) + L":" + device;
    if (remoteUNC != nullptr)
        identity += remoteUNC;
    AcquireSRWLockExclusive(&service->Lock);
    Request& request = service->Requests[drive];
    request.Type = type;
    request.Device = device;
    request.Window = window;
    request.Message = message;
    const Entry& entry = service->Values.Query(drive, identity, mode, GetTickCount64(), disconnected || device.empty());
    if (mode != dfsOff && entry.Queued)
    {
        if (!StartService(*service))
        {
            int reserved = service->Values.Reserve(GetTickCount64());
            if (reserved >= 0)
                service->Values.Complete(reserved, service->Values.Get(reserved).Token, false, 0, GetTickCount64(), 0);
        }
        if (service->Wake != nullptr)
            SetEvent(service->Wake);
    }
    value.Available = entry.Available;
    value.Pending = entry.Queued || entry.Running;
    value.Stale = entry.Stale;
    value.Bytes = entry.Bytes;
    ULARGE_INTEGER time = {};
    time.QuadPart = entry.SuccessTime;
    value.LastSuccess.dwLowDateTime = time.LowPart;
    value.LastSuccess.dwHighDateTime = time.HighPart;
    value.State = value.Pending ? dfsPending : value.Available ? (value.Stale ? dfsStale : dfsFresh)
                                                             : entry.Attempted ? dfsUnavailable : dfsUnknown;
    ReleaseSRWLockExclusive(&service->Lock);
    return value;
}

void DriveFreeSpaceInvalidate(wchar_t letter)
{
    Service* service = GetService();
    if (service == nullptr)
        return;
    AcquireSRWLockExclusive(&service->Lock);
    service->Values.Invalidate(DriveIndex(letter));
    if (service->Wake != nullptr)
        SetEvent(service->Wake);
    ReleaseSRWLockExclusive(&service->Lock);
}

void DriveFreeSpaceMarkStale(wchar_t letter)
{
    Service* service = GetService();
    if (service == nullptr)
        return;
    AcquireSRWLockExclusive(&service->Lock);
    service->Values.MarkStale(DriveIndex(letter));
    ReleaseSRWLockExclusive(&service->Lock);
}

void DriveFreeSpaceSetPolicies(DriveFreeSpaceMode removable, DriveFreeSpaceMode remote)
{
    Service* service = GetService();
    if (service == nullptr)
        return;
    AcquireSRWLockExclusive(&service->Lock);
    for (unsigned drive = 0; drive < 26; ++drive)
    {
        unsigned mode = service->Requests[drive].Type == DRIVE_REMOTE ? remote : removable;
        if (mode == dfsOff)
            service->Values.Reset(drive);
    }
    if (service->Wake != nullptr)
        SetEvent(service->Wake);
    ReleaseSRWLockExclusive(&service->Lock);
}

void DriveFreeSpaceShutdown()
{
    Service* service = ServiceInstance;
    if (service == nullptr)
        return;
    AcquireSRWLockExclusive(&service->Lock);
    service->Stop = true;
    if (service->Wake != nullptr)
        SetEvent(service->Wake);
    HANDLE thread = service->Thread;
    ReleaseSRWLockExclusive(&service->Lock);
    // The worker only waits on process handles and never enters a filesystem
    // provider. The job is a second safety net if shutdown interrupts launch.
    if (thread != nullptr && WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0)
        return; // Process exit closes the kill-on-close job; do not free live state.
    if (thread != nullptr)
        CloseHandle(thread);
    if (service->Processes != nullptr)
        CloseHandle(service->Processes);
    if (service->Wake != nullptr)
        CloseHandle(service->Wake);
    service->Thread = service->Processes = service->Wake = nullptr;
}

bool DriveFreeSpaceHelperDispatch(int& exitCode)
{
    // Avoid parsing/allocating on normal startup unless the private switch is
    // present. The exact argument is checked below, including argument count.
    if (wcsstr(GetCommandLineW(), HelperSwitch) == nullptr)
        return false;
    int count = 0;
    wchar_t** args = CommandLineToArgvW(GetCommandLineW(), &count);
    if (args == nullptr)
        return false;
    bool helper = count >= 2 && wcscmp(args[1], HelperSwitch) == 0;
    if (!helper)
    {
        LocalFree(args);
        return false;
    }
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX | SEM_NOGPFAULTERRORBOX);
    exitCode = ERROR_INVALID_PARAMETER;
    wchar_t* end = nullptr;
    unsigned long long number = count == 3 ? _wcstoui64(args[2], &end, 16) : 0;
    bool valid = count == 3 && number != 0 && end != args[2] && *end == 0 &&
                 number <= static_cast<unsigned long long>(~static_cast<ULONG_PTR>(0));
    LocalFree(args);
    if (!valid)
        return true;
    HANDLE mapping = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(number));
    SharedProbe* shared = static_cast<SharedProbe*>(MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0));
    if (shared == nullptr)
        return true;
    MEMORY_BASIC_INFORMATION memory = {};
    SIZE_T queried = VirtualQuery(shared, &memory, sizeof(memory));
    valid = queried != 0 && memory.RegionSize >= sizeof(SharedProbe) &&
            shared->Magic == MappingMagic && shared->Size >= sizeof(SharedProbe) + sizeof(wchar_t) &&
            shared->Size <= MaximumPayload && shared->Size <= memory.RegionSize &&
            shared->DeviceCharacters > 0 &&
            shared->DeviceCharacters == (shared->Size - sizeof(SharedProbe)) / sizeof(wchar_t) &&
            DriveIndex(shared->Drive) >= 0 &&
            (shared->Type == DRIVE_REMOTE || shared->Type == DRIVE_REMOVABLE);
    if (valid)
    {
        const wchar_t* identity = reinterpret_cast<const wchar_t*>(shared + 1);
        valid = identity[shared->DeviceCharacters - 1] == 0;
    }
    if (valid)
    {
        RunProbe(shared);
        InterlockedExchange(&shared->Complete, 1);
        exitCode = shared->Error;
    }
    UnmapViewOfFile(shared);
    CloseHandle(mapping);
    return true;
}
