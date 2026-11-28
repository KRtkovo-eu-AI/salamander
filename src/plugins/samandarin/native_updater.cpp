// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Fetch deduplicated catalog icons on the native worker. | Keep the original asynchronous icon behavior and cancel stale refresh work. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Join the canceled worker before honoring force-unload. | A forced plugin unload must not leave a thread executing unmapped code. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Report unrecognized GitHub release redirects as localized check errors. | Do not silently treat malformed release tags as an up-to-date result. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Pause the native worker before unload and resume it after abort. | Protect provider services while preserving retryable process-close behavior. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Run release and catalog checks on a bounded native worker and preserve process-exit ownership. | Remove CLR hosting and keep UI work on the Salamander thread. | | openrouter/openai/gpt-6-luna-pro |

#include "precomp.h"
#include "native_updater.h"
#include "native_http.h"
#include "native_install_security.h"
#include "native_installer.h"
#include "native_json.h"
#include "native_utils.h"

#include "../../darkmode.h"

#include <algorithm>
#include <deque>
#include <limits>
#include <new>
#include <string>
#include <utility>
#include <vector>

CSamandarinNativeCallGate SamandarinNativeCallGate;

namespace
{
constexpr int SAMANDARIN_COLOR_ITEM_FOREGROUND = 6;
constexpr int SAMANDARIN_COLOR_ITEM_BACKGROUND = 11;
}

namespace SamandarinNative
{
namespace
{
const wchar_t* const kReleaseUrl = L"https://github.com/KRtkovo-eu-AI/salamander/releases/latest";
constexpr ULONGLONG kDotNetFileTimeTicks = 504911232000000000ULL;
constexpr ULONGLONG kMinimumCheckDelayMs = 10000;
constexpr int kColorItemForeground = 6;
constexpr int kColorItemBackground = 11;

enum class WorkType
{
    Check,
    Catalog,
    CatalogIcons,
    Install,
};

struct WorkItem
{
    WorkType Type = WorkType::Check;
    HWND Target = nullptr;
    bool UserInitiated = false;
    bool ShowIfCurrent = false;
    std::vector<PluginUpdateRow> Rows;
    std::vector<PluginUpdateRow> Packages;
    std::vector<std::wstring> WebUrls;
};

SRWLOCK gWorkerLock = SRWLOCK_INIT;
HANDLE gWorkerThread = nullptr;
HANDLE gWorkerWakeEvent = nullptr;
volatile LONG gStopRequested = 0;
volatile LONG gCatalogIconCancelRequested = 0;
volatile LONG gWorkerPreflightPaused = 0;
bool gWorkerStopping = false;
bool gUpdaterInitialized = false;
ULONGLONG gNextAutomaticCheckTick = 0;
std::wstring gCurrentVersion;
std::deque<WorkItem> gWorkQueue;

template <typename T, typename... Args>
T* NewNoThrow(Args&&... args)
{
#if defined(_DEBUG) && defined(_MSC_VER)
#undef new
#define SAMANDARIN_RESTORE_DEBUG_NEW
#endif
    T* result = new (std::nothrow) T(std::forward<Args>(args)...);
#ifdef SAMANDARIN_RESTORE_DEBUG_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef SAMANDARIN_RESTORE_DEBUG_NEW
#endif
    return result;
}

bool IsStopping()
{
    return InterlockedCompareExchange(const_cast<LONG*>(&gStopRequested), 0, 0) != 0;
}

bool EqualsIgnoreCase(const std::wstring& left, const std::wstring& right)
{
    return CompareStringOrdinal(left.c_str(), static_cast<int>(left.size()),
                                right.c_str(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

std::wstring Trim(std::wstring value)
{
    const size_t first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return std::wstring();
    return value.substr(first, value.find_last_not_of(L" \t\r\n") - first + 1);
}

std::wstring GetCurrentVersion()
{
    std::string version = VERSINFO_SALAMANDER_VERSION;
    const size_t platform = version.find(" (");
    if (platform != std::string::npos) version.resize(platform);
    version += VERSINFO_SAMANDARIN_SUFFIX;
    return Utf8ToWide(version);
}

LONGLONG GetDotNetUtcTicks()
{
    FILETIME fileTime = {};
    GetSystemTimeAsFileTime(&fileTime);
    ULARGE_INTEGER ticks = {};
    ticks.LowPart = fileTime.dwLowDateTime;
    ticks.HighPart = fileTime.dwHighDateTime;
    if (ticks.QuadPart > static_cast<ULONGLONG>((std::numeric_limits<LONGLONG>::max)()) - kDotNetFileTimeTicks)
        return (std::numeric_limits<LONGLONG>::max)();
    return static_cast<LONGLONG>(ticks.QuadPart + kDotNetFileTimeTicks);
}

std::wstring GetSettingText(const char* value, size_t capacity)
{
    size_t length = 0;
    while (length < capacity && value[length] != '\0') ++length;
    return AnsiOrUtf8ToWide(std::string(value, length));
}

void StoreSettingText(char* destination, size_t capacity, const std::wstring& value)
{
    if (destination == nullptr || capacity == 0) return;
    memset(destination, 0, capacity);
    const std::string encoded = WideToAnsi(value);
    const size_t copy = (std::min)(capacity - 1, encoded.size());
    if (copy != 0) memcpy(destination, encoded.data(), copy);
}

bool IsNumericComponent(const std::wstring& value)
{
    bool digit = false;
    for (wchar_t c : value)
    {
        if (c >= L'0' && c <= L'9') digit = true;
        else if (c != L'.') return false;
    }
    return digit;
}

bool IsNumericVersion(const std::wstring& value)
{
    if (value.empty()) return false;
    bool sawComponent = false;
    size_t start = 0;
    for (size_t index = 0; index <= value.size(); ++index)
    {
        if (index != value.size() && value[index] != L'.') continue;
        const std::wstring component = value.substr(start, index - start);
        start = index + 1;
        if (component.empty()) continue;
        sawComponent = true;
        for (wchar_t c : component)
        {
            if (c < L'0' || c > L'9') return false;
        }
    }
    return sawComponent;
}

std::vector<std::wstring> SplitReleaseTag(const std::wstring& value)
{
    std::vector<std::wstring> result;
    size_t start = 0;
    for (size_t index = 0; index <= value.size(); ++index)
    {
        if (index != value.size() && value[index] != L'-') continue;
        if (index > start) result.push_back(value.substr(start, index - start));
        start = index + 1;
    }
    return result;
}

bool IsRecognizedReleaseTag(const std::wstring& value)
{
    const std::vector<std::wstring> segments = SplitReleaseTag(value);
    if (segments.size() < 3) return false;
    size_t marker = segments.size();
    for (size_t index = 0; index < segments.size(); ++index)
    {
        if (EqualsIgnoreCase(segments[index], L"samandarin"))
        {
            marker = index;
            break;
        }
    }
    if (marker == 0 || marker >= segments.size() - 1) return false;
    for (size_t index = 0; index < marker; ++index)
    {
        if (!IsNumericComponent(segments[index])) return false;
    }
    std::wstring releaseVersion = segments[marker + 1];
    for (size_t index = marker + 2; index < segments.size(); ++index)
        releaseVersion += L"-" + segments[index];
    return IsNumericVersion(releaseVersion);
}

bool HexValue(wchar_t c, unsigned int* result)
{
    if (c >= L'0' && c <= L'9') *result = static_cast<unsigned int>(c - L'0');
    else if (c >= L'a' && c <= L'f') *result = static_cast<unsigned int>(c - L'a' + 10);
    else if (c >= L'A' && c <= L'F') *result = static_cast<unsigned int>(c - L'A' + 10);
    else return false;
    return true;
}

std::wstring UrlDecode(const std::wstring& value)
{
    std::wstring result;
    for (size_t index = 0; index < value.size(); ++index)
    {
        if (value[index] == L'%' && index + 2 < value.size())
        {
            unsigned int high = 0, low = 0;
            if (HexValue(value[index + 1], &high) && HexValue(value[index + 2], &low))
            {
                result.push_back(static_cast<wchar_t>((high << 4) | low));
                index += 2;
                continue;
            }
        }
        result.push_back(value[index]);
    }
    return result;
}

bool ExtractReleaseVersion(const std::wstring& url, std::wstring* version, bool* noPublishedReleases)
{
    if (version != nullptr) version->clear();
    if (noPublishedReleases != nullptr) *noPublishedReleases = false;
    size_t pathStart = url.find(L"://");
    pathStart = pathStart == std::wstring::npos ? 0 : url.find(L'/', pathStart + 3);
    if (pathStart == std::wstring::npos) return false;
    std::wstring path = url.substr(pathStart);
    const size_t query = path.find_first_of(L"?#");
    if (query != std::wstring::npos) path.resize(query);
    while (path.size() > 1 && path.back() == L'/') path.pop_back();
    const std::wstring lowerPath = [&]()
    {
        std::wstring lower = path;
        for (wchar_t& c : lower) if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
        return lower;
    }();
    if (noPublishedReleases != nullptr)
        *noPublishedReleases = lowerPath.size() >= 9 && lowerPath.substr(lowerPath.size() - 9) == L"/releases";

    std::vector<std::wstring> segments;
    size_t start = 0;
    while (start < path.size())
    {
        while (start < path.size() && path[start] == L'/') ++start;
        if (start >= path.size()) break;
        const size_t slash = path.find(L'/', start);
        segments.push_back(path.substr(start, slash == std::wstring::npos ? std::wstring::npos : slash - start));
        if (slash == std::wstring::npos) break;
        start = slash + 1;
    }
    if (segments.empty()) return false;
    std::wstring candidate = UrlDecode(segments.back());
    if (EqualsIgnoreCase(candidate, L"latest") && segments.size() > 1)
        candidate = UrlDecode(segments[segments.size() - 2]);
    if (!IsRecognizedReleaseTag(candidate)) return false;
    if (version != nullptr) *version = candidate;
    return true;
}

DWORD FrequencyIntervalMs(int frequency)
{
    if (frequency == static_cast<int>(NativeUpdateFrequency::Disabled)) return 0;
    if (frequency == static_cast<int>(NativeUpdateFrequency::Daily)) return 24ull * 60ull * 60ull * 1000ull;
    if (frequency == static_cast<int>(NativeUpdateFrequency::Monthly)) return 30ull * 24ull * 60ull * 60ull * 1000ull;
    return 7ull * 24ull * 60ull * 60ull * 1000ull;
}

void ScheduleNextAutomaticCheck(const NativeUpdateSettings& settings)
{
    const DWORD interval = FrequencyIntervalMs(settings.Frequency);
    AcquireSRWLockExclusive(&gWorkerLock);
    if (interval == 0)
    {
        gNextAutomaticCheckTick = 0;
    }
    else
    {
        const ULONGLONG nowDotNet = static_cast<ULONGLONG>((std::max)(0LL, GetDotNetUtcTicks()));
        ULONGLONG waitMs = kMinimumCheckDelayMs;
        if (settings.HasLastCheckUtc && settings.LastCheckUtcTicks > 0)
        {
            const ULONGLONG last = static_cast<ULONGLONG>(settings.LastCheckUtcTicks);
            const ULONGLONG intervalTicks = static_cast<ULONGLONG>(interval) * 10000ull;
            const ULONGLONG next = last > (std::numeric_limits<ULONGLONG>::max)() - intervalTicks
                                       ? (std::numeric_limits<ULONGLONG>::max)()
                                       : last + intervalTicks;
            if (next > nowDotNet) waitMs = (next - nowDotNet) / 10000ull;
        }
        if (waitMs < kMinimumCheckDelayMs) waitMs = kMinimumCheckDelayMs;
        gNextAutomaticCheckTick = GetTickCount64() + waitMs;
    }
    HANDLE wake = gWorkerWakeEvent;
    ReleaseSRWLockExclusive(&gWorkerLock);
    if (wake != nullptr) SetEvent(wake);
}

void AppendErrors(const std::vector<std::wstring>& errors, std::wstring* text)
{
    if (text == nullptr) return;
    for (const std::wstring& error : errors)
    {
        if (!text->empty()) text->append(L"\r\n");
        text->append(error);
    }
}

void ProcessCheck(const WorkItem& work)
{
    if (IsStopping()) return;
    HttpResponse release;
    std::wstring releaseError;
    std::wstring latestVersion;
    bool noPublished = false;
    if (HttpGet(kReleaseUrl, 1024 * 1024, &release, &releaseError, &gStopRequested, false, true) &&
        !ExtractReleaseVersion(release.FinalUrl, &latestVersion, &noPublished) && !noPublished)
        releaseError = GetLocalizedString(IDS_PERFORM_CHECK_ERROR);

    std::vector<PluginUpdateRow> rows;
    std::vector<std::wstring> catalogErrors;
    if (!IsStopping()) FetchPluginCatalog(&rows, &catalogErrors, &gStopRequested);
    if (IsStopping()) return;

    NativeUpdateSettings settings = {};
    if (!Samandarin_LoadSettings(&settings))
    {
        settings.CheckOnStartup = TRUE;
        settings.Frequency = static_cast<int>(NativeUpdateFrequency::Weekly);
    }
    const std::wstring currentVersion = gCurrentVersion;
    bool notifyRelease = false;
    bool showCurrent = false;
    std::wstring versionForMessage;
    settings.HasLastCheckUtc = TRUE;
    settings.LastCheckUtcTicks = GetDotNetUtcTicks();
    if (!latestVersion.empty())
    {
        StoreSettingText(settings.LastKnownRemoteVersion, sizeof(settings.LastKnownRemoteVersion), latestVersion);
        const int comparison = CompareVersions(latestVersion, currentVersion);
        const std::wstring prompted = GetSettingText(settings.LastPromptedVersion, sizeof(settings.LastPromptedVersion));
        if (comparison > 0 && (work.UserInitiated || !EqualsIgnoreCase(prompted, latestVersion)))
        {
            StoreSettingText(settings.LastPromptedVersion, sizeof(settings.LastPromptedVersion), latestVersion);
            notifyRelease = true;
        }
        else if (comparison <= 0 && work.ShowIfCurrent)
        {
            showCurrent = true;
            versionForMessage = latestVersion;
        }
    }
    else if (work.ShowIfCurrent && releaseError.empty())
    {
        const std::wstring known = GetSettingText(settings.LastKnownRemoteVersion, sizeof(settings.LastKnownRemoteVersion));
        if (noPublished || (!known.empty() && CompareVersions(known, currentVersion) <= 0))
        {
            showCurrent = true;
            versionForMessage = known;
        }
    }
    Samandarin_SaveSettings(&settings);
    ScheduleNextAutomaticCheck(settings);

    auto* completion = NewNoThrow<CheckCompletion>();
    if (completion == nullptr) return;
    completion->Owner = work.Target;
    completion->CurrentVersion = currentVersion;
    completion->LatestVersion = versionForMessage.empty() ? latestVersion : versionForMessage;
    completion->Error = releaseError;
    completion->UserInitiated = work.UserInitiated;
    completion->ShowIfCurrent = showCurrent;
    completion->NoPublishedReleases = noPublished;
    completion->NotifyRelease = notifyRelease;
    completion->PluginRows = std::move(rows);
    AppendErrors(catalogErrors, &completion->CatalogError);
    NativeUi_PostCheckCompletion(completion);
}

void ProcessCatalog(const WorkItem& work)
{
    if (IsStopping()) return;
    auto* completion = NewNoThrow<CatalogCompletion>();
    if (completion == nullptr) return;
    completion->Target = work.Target;
    completion->Success = FetchPluginCatalog(&completion->Rows, &completion->Errors, &gStopRequested);
    if (IsStopping())
    {
        delete completion;
        return;
    }
    NativeUi_PostCatalogCompletion(completion);
}

constexpr size_t kMaximumCatalogIconCount = 256;
constexpr size_t kMaximumCatalogIconBytes = 1024 * 1024;
constexpr size_t kMaximumCatalogIconTotalBytes = 16 * 1024 * 1024;
constexpr size_t kCatalogIconWorkerCount = 6;

struct CatalogIconFetchContext
{
    const std::vector<std::wstring>* References = nullptr;
    std::vector<CatalogIconImage>* Results = nullptr;
    volatile LONG NextIndex = -1;
    SRWLOCK SizeLock = SRWLOCK_INIT;
    size_t TotalBytes = 0;
};

bool ReadCatalogIconFile(const std::wstring& path, std::string* bytes)
{
    if (bytes == nullptr || path.empty()) return false;
    bytes->clear();
    HANDLE file = CreateFileW(AddLongPathPrefix(path).c_str(), GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size = {};
    bool success = GetFileSizeEx(file, &size) && size.QuadPart > 0 &&
                   static_cast<ULONGLONG>(size.QuadPart) <= kMaximumCatalogIconBytes;
    if (success)
    {
        bytes->resize(static_cast<size_t>(size.QuadPart));
        size_t total = 0;
        while (total < bytes->size())
        {
            if (InterlockedCompareExchange(const_cast<LONG*>(&gCatalogIconCancelRequested), 0, 0) != 0 ||
                IsStopping())
            {
                success = false;
                break;
            }
            const DWORD request = static_cast<DWORD>((std::min)(
                bytes->size() - total, static_cast<size_t>(64 * 1024)));
            DWORD read = 0;
            if (!ReadFile(file, &(*bytes)[total], request, &read, nullptr) || read == 0)
            {
                success = false;
                break;
            }
            total += read;
        }
        success = success && total == bytes->size();
    }
    CloseHandle(file);
    if (!success) bytes->clear();
    return success;
}

DWORD WINAPI CatalogIconWorkerProc(void* parameter)
{
    auto* context = reinterpret_cast<CatalogIconFetchContext*>(parameter);
    if (context == nullptr || context->References == nullptr || context->Results == nullptr) return 0;
    for (;;)
    {
        if (IsStopping() ||
            InterlockedCompareExchange(const_cast<LONG*>(&gCatalogIconCancelRequested), 0, 0) != 0)
            return 0;
        const LONG index = InterlockedIncrement(const_cast<LONG*>(&context->NextIndex));
        if (index < 0 || static_cast<size_t>(index) >= context->References->size()) return 0;

        const std::wstring& reference = (*context->References)[static_cast<size_t>(index)];
        CatalogIconImage& image = (*context->Results)[static_cast<size_t>(index)];
        std::string bytes;
        bool success = false;
        if (IsAbsoluteHttpUrl(reference))
        {
            HttpResponse response;
            std::wstring error;
            success = HttpGet(reference, kMaximumCatalogIconBytes, &response, &error,
                              &gCatalogIconCancelRequested, true, false);
            if (success) bytes = std::move(response.Body);
        }
        else
        {
            success = ReadCatalogIconFile(reference, &bytes);
        }
        if (!success || bytes.empty() || IsStopping() ||
            InterlockedCompareExchange(const_cast<LONG*>(&gCatalogIconCancelRequested), 0, 0) != 0)
            continue;

        AcquireSRWLockExclusive(&context->SizeLock);
        if (bytes.size() <= kMaximumCatalogIconTotalBytes - context->TotalBytes)
        {
            context->TotalBytes += bytes.size();
            image.Reference = reference;
            image.Bytes = std::move(bytes);
        }
        ReleaseSRWLockExclusive(&context->SizeLock);
    }
}

void ProcessCatalogIcons(const WorkItem& work)
{
    if (IsStopping()) return;
    std::vector<std::wstring> references;
    references.reserve((std::min)(work.Rows.size(), kMaximumCatalogIconCount));
    for (const PluginUpdateRow& row : work.Rows)
    {
        const std::wstring reference = ResolveCatalogIconReference(row);
        if (reference.empty()) continue;
        const bool duplicate = std::any_of(references.begin(), references.end(),
                                           [&](const std::wstring& existing)
                                           { return EqualsIgnoreCase(existing, reference); });
        if (!duplicate) references.push_back(reference);
        if (references.size() >= kMaximumCatalogIconCount) break;
    }
    if (references.empty()) return;

    std::vector<CatalogIconImage> results(references.size());
    CatalogIconFetchContext context;
    context.References = &references;
    context.Results = &results;

    HANDLE workers[kCatalogIconWorkerCount] = {};
    size_t workerCount = 0;
    const size_t extraWorkers = (std::min)(references.size(), kCatalogIconWorkerCount) - 1;
    for (; workerCount < extraWorkers; ++workerCount)
    {
        workers[workerCount] = CreateThread(nullptr, 0, CatalogIconWorkerProc, &context, 0, nullptr);
        if (workers[workerCount] == nullptr) break;
    }
    CatalogIconWorkerProc(&context);
    if (workerCount != 0)
        WaitForMultipleObjects(static_cast<DWORD>(workerCount), workers, TRUE, INFINITE);
    for (size_t index = 0; index < workerCount; ++index) CloseHandle(workers[index]);

    if (IsStopping() ||
        InterlockedCompareExchange(const_cast<LONG*>(&gCatalogIconCancelRequested), 0, 0) != 0)
        return;
    auto* completion = NewNoThrow<CatalogIconsCompletion>();
    if (completion == nullptr) return;
    completion->Target = work.Target;
    for (CatalogIconImage& image : results)
    {
        if (!image.Bytes.empty()) completion->Images.push_back(std::move(image));
    }
    if (completion->Images.empty())
    {
        delete completion;
        return;
    }
    NativeUi_PostCatalogIcons(completion);
}

void ProcessInstall(const WorkItem& work)
{
    if (IsStopping()) return;
    auto* completion = NewNoThrow<InstallCompletion>();
    if (completion == nullptr) return;
    completion->Target = work.Target;
    completion->WebUrls = work.WebUrls;
    for (const PluginUpdateRow& package : work.Packages)
    {
        if (IsStopping())
        {
            completion->Success = false;
            completion->Error = L"The package installation was cancelled.";
            break;
        }
        std::wstring status;
        std::wstring error;
        if (!StageAndSchedulePackage(package, GetCurrentProcessId(), &gStopRequested, &status, &error))
        {
            completion->Success = false;
            completion->Error = error;
            break;
        }
        completion->Success = true;
        completion->Status = status;
    }
    if (IsStopping())
    {
        delete completion;
        return;
    }
    NativeUi_PostInstallCompletion(completion);
}

DWORD WINAPI WorkerThreadProc(void*)
{
    for (;;)
    {
        WorkItem item;
        bool hasWork = false;
        DWORD waitMs = INFINITE;
        AcquireSRWLockExclusive(&gWorkerLock);
        if (gWorkerStopping)
        {
            ReleaseSRWLockExclusive(&gWorkerLock);
            return 0;
        }
        if (!gWorkQueue.empty())
        {
            item = std::move(gWorkQueue.front());
            gWorkQueue.pop_front();
            hasWork = true;
        }
        else if (gNextAutomaticCheckTick != 0)
        {
            const ULONGLONG now = GetTickCount64();
            if (now >= gNextAutomaticCheckTick)
            {
                item.Type = WorkType::Check;
                item.UserInitiated = false;
                item.ShowIfCurrent = false;
                hasWork = true;
                gNextAutomaticCheckTick = 0;
            }
            else
            {
                waitMs = static_cast<DWORD>((std::min)(gNextAutomaticCheckTick - now,
                                                       static_cast<ULONGLONG>(MAXDWORD - 1)));
            }
        }
        ReleaseSRWLockExclusive(&gWorkerLock);

        if (hasWork)
        {
            if (item.Type == WorkType::Check) ProcessCheck(item);
            else if (item.Type == WorkType::Catalog) ProcessCatalog(item);
            else if (item.Type == WorkType::CatalogIcons) ProcessCatalogIcons(item);
            else ProcessInstall(item);
            continue;
        }
        if (gWorkerWakeEvent == nullptr) return 1;
        WaitForSingleObject(gWorkerWakeEvent, waitMs);
    }
}

bool EnsureWorker()
{
    AcquireSRWLockExclusive(&gWorkerLock);
    if (gWorkerStopping || IsStopping())
    {
        ReleaseSRWLockExclusive(&gWorkerLock);
        return false;
    }
    if (gWorkerThread != nullptr)
    {
        const DWORD threadState = WaitForSingleObject(gWorkerThread, 0);
        if (threadState == WAIT_TIMEOUT)
        {
            ReleaseSRWLockExclusive(&gWorkerLock);
            return true;
        }
        CloseHandle(gWorkerThread);
        gWorkerThread = nullptr;
        if (gWorkerWakeEvent != nullptr)
        {
            CloseHandle(gWorkerWakeEvent);
            gWorkerWakeEvent = nullptr;
        }
    }
    gWorkerWakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (gWorkerWakeEvent != nullptr)
        gWorkerThread = CreateThread(nullptr, 0, WorkerThreadProc, nullptr, 0, nullptr);
    const bool started = gWorkerThread != nullptr;
    if (!started && gWorkerWakeEvent != nullptr)
    {
        CloseHandle(gWorkerWakeEvent);
        gWorkerWakeEvent = nullptr;
    }
    ReleaseSRWLockExclusive(&gWorkerLock);
    return started;
}

bool QueueWork(WorkItem&& item, bool coalesceCatalog)
{
    if (!EnsureWorker()) return false;
    AcquireSRWLockExclusive(&gWorkerLock);
    if (gWorkerStopping || IsStopping())
    {
        ReleaseSRWLockExclusive(&gWorkerLock);
        return false;
    }
    if (coalesceCatalog)
    {
        for (const WorkItem& queued : gWorkQueue)
        {
            if (queued.Type == WorkType::Catalog && queued.Target == item.Target)
            {
                ReleaseSRWLockExclusive(&gWorkerLock);
                return true;
            }
        }
    }
    gWorkQueue.push_back(std::move(item));
    HANDLE wake = gWorkerWakeEvent;
    ReleaseSRWLockExclusive(&gWorkerLock);
    if (wake != nullptr) SetEvent(wake);
    return true;
}

void StopWorker()
{
    InterlockedExchange(const_cast<LONG*>(&gStopRequested), 1);
    InterlockedExchange(const_cast<LONG*>(&gCatalogIconCancelRequested), 1);
    AcquireSRWLockExclusive(&gWorkerLock);
    gWorkerStopping = true;
    HANDLE wake = gWorkerWakeEvent;
    ReleaseSRWLockExclusive(&gWorkerLock);
    if (wake != nullptr) SetEvent(wake);
}

bool JoinWorker(DWORD timeoutMs)
{
    HANDLE thread = nullptr;
    AcquireSRWLockShared(&gWorkerLock);
    thread = gWorkerThread;
    ReleaseSRWLockShared(&gWorkerLock);
    return WaitForSamandarinWorker(thread, timeoutMs) != FALSE;
}

void CloseWorkerHandles()
{
    AcquireSRWLockExclusive(&gWorkerLock);
    HANDLE thread = gWorkerThread;
    HANDLE wake = gWorkerWakeEvent;
    gWorkerThread = nullptr;
    gWorkerWakeEvent = nullptr;
    gWorkQueue.clear();
    gNextAutomaticCheckTick = 0;
    ReleaseSRWLockExclusive(&gWorkerLock);
    if (thread != nullptr) CloseHandle(thread);
    if (wake != nullptr) CloseHandle(wake);
}

void ResumeWorkerAfterProcessExit()
{
    if (InterlockedExchange(const_cast<LONG*>(&gWorkerPreflightPaused), 0) == 0 ||
        SamandarinNativeCallGate.IsClosing())
        return;
    InterlockedExchange(const_cast<LONG*>(&gStopRequested), 0);
    InterlockedExchange(const_cast<LONG*>(&gCatalogIconCancelRequested), 0);
    AcquireSRWLockExclusive(&gWorkerLock);
    gWorkerStopping = false;
    ReleaseSRWLockExclusive(&gWorkerLock);
    if (!gUpdaterInitialized || !EnsureWorker()) return;
    NativeUpdateSettings settings = {};
    if (!Samandarin_LoadSettings(&settings))
    {
        settings.CheckOnStartup = TRUE;
        settings.Frequency = static_cast<int>(NativeUpdateFrequency::Weekly);
    }
    ScheduleNextAutomaticCheck(settings);
}
} // namespace

void NativeUpdater_Initialize(HWND parent)
{
    if (gUpdaterInitialized) return;
    if (!NativeUi_Initialize(parent))
    {
        ShowInitializationError(parent);
        return;
    }
    gCurrentVersion = GetCurrentVersion();
    InterlockedExchange(const_cast<LONG*>(&gStopRequested), 0);
    InterlockedExchange(const_cast<LONG*>(&gCatalogIconCancelRequested), 0);
    AcquireSRWLockExclusive(&gWorkerLock);
    gWorkerStopping = false;
    gWorkQueue.clear();
    ReleaseSRWLockExclusive(&gWorkerLock);
    if (!EnsureWorker())
    {
        NativeUi_Shutdown();
        ShowInitializationError(parent);
        return;
    }
    gUpdaterInitialized = true;
    NativeUpdateSettings settings = {};
    if (!Samandarin_LoadSettings(&settings))
    {
        settings.CheckOnStartup = TRUE;
        settings.Frequency = static_cast<int>(NativeUpdateFrequency::Weekly);
    }
    ScheduleNextAutomaticCheck(settings);
    WorkItem startup;
    startup.Type = WorkType::Check;
    startup.Target = parent;
    startup.UserInitiated = false;
    startup.ShowIfCurrent = false;
    if (settings.CheckOnStartup) QueueWork(std::move(startup), false);

    std::wstring installError;
    if (TakeLastInstallError(&installError))
    {
        auto* completion = NewNoThrow<CheckCompletion>();
        if (completion != nullptr)
        {
            completion->CurrentVersion = gCurrentVersion;
            completion->Owner = parent;
            completion->Error = installError;
            completion->UserInitiated = true;
            completion->InstallError = true;
            NativeUi_PostCheckCompletion(completion);
        }
    }
}

BOOL NativeUpdater_Shutdown(DWORD timeoutMs)
{
    InterlockedExchange(const_cast<LONG*>(&gWorkerPreflightPaused), 1);
    StopWorker();
    if (!JoinWorker(timeoutMs))
    {
        ResumeWorkerAfterProcessExit();
        return FALSE;
    }
    CloseWorkerHandles();
    NativeUi_Shutdown();
    gUpdaterInitialized = false;
    InterlockedExchange(const_cast<LONG*>(&gWorkerPreflightPaused), 0);
    return TRUE;
}

BOOL NativeUpdater_ShutdownForce()
{
    InterlockedExchange(const_cast<LONG*>(&gWorkerPreflightPaused), 1);
    StopWorker();
    while (!JoinWorker(2000))
    {
        Sleep(50);
    }
    CloseWorkerHandles();
    NativeUi_Shutdown();
    gUpdaterInitialized = false;
    InterlockedExchange(const_cast<LONG*>(&gWorkerPreflightPaused), 0);
    return TRUE;
}

BOOL NativeUpdater_ShowConfiguration(HWND parent)
{
    NativeUpdateSettings settings = {};
    if (!Samandarin_LoadSettings(&settings)) return FALSE;
    const BOOL shown = NativeUi_ShowConfiguration(parent, settings, gCurrentVersion);
    if (shown)
    {
        Samandarin_LoadSettings(&settings);
        if (SalamanderGeneral != nullptr)
            SalamanderGeneral->SetFlagLoadOnSalamanderStart(settings.CheckOnStartup != 0);
        ScheduleNextAutomaticCheck(settings);
    }
    return shown;
}

BOOL NativeUpdater_CheckNow(HWND parent)
{
    if (NativeUpdater_IsProcessExiting()) return FALSE;
    WorkItem item;
    item.Type = WorkType::Check;
    item.Target = parent;
    item.UserInitiated = true;
    item.ShowIfCurrent = true;
    return QueueWork(std::move(item), false) ? TRUE : FALSE;
}

BOOL NativeUpdater_ShowPluginUpdates(HWND parent)
{
    if (NativeUpdater_IsProcessExiting() || !NativeUi_Initialize(parent)) return FALSE;
    return NativeUi_ShowPluginUpdates(parent);
}

void NativeUpdater_NotifyColorsChanged()
{
    NativeUi_NotifyColorsChanged();
}

BOOL NativeUpdater_IsProcessExiting()
{
    return SamandarinNativeCallGate.IsClosing() ||
                   InterlockedCompareExchange(const_cast<LONG*>(&gWorkerPreflightPaused), 0, 0) != 0
               ? TRUE
               : FALSE;
}

void NativeUpdater_RequestCatalogRefresh(HWND target)
{
    NativeUpdater_CancelCatalogIcons(target);
    WorkItem item;
    item.Type = WorkType::Catalog;
    item.Target = target;
    QueueWork(std::move(item), true);
}

void NativeUpdater_RequestCatalogIcons(HWND target, const std::vector<PluginUpdateRow>& rows)
{
    if (target == nullptr || rows.empty() || IsStopping()) return;
    WorkItem item;
    item.Type = WorkType::CatalogIcons;
    item.Target = target;
    item.Rows = rows;
    InterlockedExchange(const_cast<LONG*>(&gCatalogIconCancelRequested), 0);
    if (!QueueWork(std::move(item), false))
        InterlockedExchange(const_cast<LONG*>(&gCatalogIconCancelRequested), 1);
}

void NativeUpdater_CancelCatalogIcons(HWND target)
{
    InterlockedExchange(const_cast<LONG*>(&gCatalogIconCancelRequested), 1);
    AcquireSRWLockExclusive(&gWorkerLock);
    for (auto item = gWorkQueue.begin(); item != gWorkQueue.end();)
    {
        if (item->Type == WorkType::CatalogIcons && (target == nullptr || item->Target == target))
            item = gWorkQueue.erase(item);
        else
            ++item;
    }
    ReleaseSRWLockExclusive(&gWorkerLock);
}

void NativeUpdater_QueueInstallPlan(HWND target, std::vector<PluginUpdateRow> packages,
                                    std::vector<std::wstring> webUrls)
{
    WorkItem item;
    item.Type = WorkType::Install;
    item.Target = target;
    item.Packages = std::move(packages);
    item.WebUrls = std::move(webUrls);
    QueueWork(std::move(item), false);
}

BOOL NativeUpdater_PrepareForProcessExit(DWORD timeoutMs, BOOL finalize)
{
    InterlockedExchange(const_cast<LONG*>(&gWorkerPreflightPaused), 1);
    StopWorker();
    if (!JoinWorker(timeoutMs))
    {
        ResumeWorkerAfterProcessExit();
        return FALSE;
    }
    if (!finalize)
    {
        CloseWorkerHandles();
        return TRUE;
    }
    if (!SamandarinNativeCallGate.TryClose())
    {
        ResumeWorkerAfterProcessExit();
        return FALSE;
    }
    CloseWorkerHandles();
    NativeUi_Shutdown();
    gUpdaterInitialized = false;
    InterlockedExchange(const_cast<LONG*>(&gWorkerPreflightPaused), 0);
    HANDLES_FINALIZE_FOR_PROCESS_EXIT();
    return TRUE;
}

void NativeUpdater_ResumeAfterProcessExit()
{
    ResumeWorkerAfterProcessExit();
}
} // namespace SamandarinNative

extern "C" __declspec(dllexport) UINT32 __stdcall Samandarin_GetCurrentColor(int color)
{
    CSamandarinNativeCall nativeCall;
    if (!nativeCall.IsAllowed() || SalamanderGeneral == nullptr) return 0;
    return SalamanderGeneral->GetCurrentColor(color);
}

extern "C" __declspec(dllexport) void __stdcall Samandarin_SetDarkModeState(BOOL enabled)
{
    CSamandarinNativeCall nativeCall;
    if (!nativeCall.IsAllowed()) return;
    DarkModeSetEnabled(enabled != FALSE);
}

extern "C" __declspec(dllexport) void __stdcall Samandarin_ApplyDarkModeTree(HWND window)
{
    CSamandarinNativeCall nativeCall;
    if (!nativeCall.IsAllowed()) return;
    DarkModeAllowDarkScrollbars(window);
    DarkModeApplyTree(window);
}

extern "C" __declspec(dllexport) void __stdcall Samandarin_UpdateListViewDarkMode(HWND window)
{
    CSamandarinNativeCall nativeCall;
    if (!nativeCall.IsAllowed()) return;
    DarkModeAllowDarkScrollbars(window);
    DarkModeApplyTree(window);
    if (SalamanderGeneral != nullptr)
    {
        const COLORREF text = SalamanderGeneral->GetCurrentColor(SAMANDARIN_COLOR_ITEM_FOREGROUND);
        const COLORREF background = SalamanderGeneral->GetCurrentColor(SAMANDARIN_COLOR_ITEM_BACKGROUND);
        DarkModeUpdateListViewColors(window, text, background,
                                     text != GetSysColor(COLOR_WINDOWTEXT) || background != GetSysColor(COLOR_WINDOW));
    }
    else
    {
        DarkModeUpdateListViewColors(window);
    }
}

extern "C" __declspec(dllexport) int __stdcall Samandarin_GetLanguageModulePath(wchar_t* buffer, int bufferLength)
{
    CSamandarinNativeCall nativeCall;
    if (!nativeCall.IsAllowed() || HLanguage == nullptr || buffer == nullptr || bufferLength <= 0) return 0;
    buffer[0] = L'\0';
    const DWORD length = GetModuleFileNameW(HLanguage, buffer, static_cast<DWORD>(bufferLength));
    if (length == 0 || length >= static_cast<DWORD>(bufferLength))
    {
        buffer[0] = L'\0';
        return 0;
    }
    return static_cast<int>(length);
}

extern "C" __declspec(dllexport) int __stdcall Samandarin_LoadString(int resourceId, wchar_t* buffer, int bufferLength)
{
    CSamandarinNativeCall nativeCall;
    if (!nativeCall.IsAllowed() || HLanguage == nullptr || buffer == nullptr || bufferLength <= 0) return 0;
    buffer[0] = L'\0';
    return LoadStringW(HLanguage, resourceId, buffer, bufferLength);
}

extern "C" __declspec(dllexport) BOOL __stdcall Samandarin_PrepareForProcessExit(DWORD timeoutMs, BOOL finalize)
{
    return SamandarinNative::NativeUpdater_PrepareForProcessExit(timeoutMs, finalize);
}

extern "C" __declspec(dllexport) void __stdcall Samandarin_ResumeAfterProcessExit()
{
    SamandarinNative::NativeUpdater_ResumeAfterProcessExit();
}
