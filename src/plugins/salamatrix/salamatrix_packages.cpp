// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "salamatrix_packages.h"
#include "salamatrix_api_docs.h"
#include "salamatrix_settings.h"
#include "../shared/salamatrix_thread_join.h"
#include "../shared/webviewviewer/native_viewer.h"
#include "../../darkmode.h"

#include <algorithm>
#include <memory>
#include <sstream>

namespace Salamatrix
{
namespace Packages
{

namespace
{
const int CommandOpenAutomationApiReference = 0x61ffffff;

struct MainThreadDispatch
{
    void* Context;
    Runtime::Protocol::MessageType Type;
    ULONGLONG RequestId;
    const char* PayloadJson;
    char* ResultJson;
    DWORD ResultCapacity;
    DWORD* ResultLength;
};

static __declspec(thread) MainThreadDispatch* CurrentMainThreadDispatch = NULL;

static std::string JsonEscape(const char* value)
{
    std::string result;
    const unsigned char* current =
        reinterpret_cast<const unsigned char*>(value != NULL ? value : "");
    while (*current != 0)
    {
        switch (*current)
        {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            if (*current < 0x20)
            {
                char escaped[7];
                _snprintf_s(
                    escaped, _countof(escaped), _TRUNCATE,
                    "\\u%04x", static_cast<unsigned int>(*current));
                result += escaped;
            }
            else
            {
                result.push_back(static_cast<char>(*current));
            }
            break;
        }
        ++current;
    }
    return result;
}

static std::string SideItemJson(const Sides::ItemInfo& item)
{
    ULARGE_INTEGER lastWrite;
    lastWrite.LowPart = item.LastWriteUtc.dwLowDateTime;
    lastWrite.HighPart = item.LastWriteUtc.dwHighDateTime;
    return std::string("{\"name\":\"") + JsonEscape(item.Name) +
           "\",\"path\":\"" + JsonEscape(item.Path) +
           "\",\"extension\":\"" + JsonEscape(item.Extension) +
           "\",\"size\":\"" +
           std::to_string(static_cast<unsigned long long>(item.Size.Value)) +
           "\",\"sizeValid\":" + (item.SizeValid ? "true" : "false") +
           ",\"attributes\":" + std::to_string(item.Attributes) +
           ",\"lastWriteUtc\":\"" +
           std::to_string(
               static_cast<unsigned long long>(lastWrite.QuadPart)) +
           "\",\"isDirectory\":" +
           (item.IsDirectory ? "true" : "false") +
           ",\"hidden\":" + (item.Hidden ? "true" : "false") +
           ",\"link\":" + (item.IsLink ? "true" : "false") +
           ",\"offline\":" + (item.IsOffline ? "true" : "false") + "}";
}

static bool StartsWith(const std::string& value, const char* prefix)
{
    const size_t length = strlen(prefix);
    return value.size() >= length && value.compare(0, length, prefix) == 0;
}

static const char* RuntimeCapabilityForMethod(const std::string& method)
{
    if (method == "salamander.sides.changePath" ||
        method == "salamander.sides.refresh" ||
        method == "salamander.sides.selectItem" ||
        method == "salamander.sides.selectAll" ||
        method == "salamander.sides.focusItem" ||
        method == "salamander.sides.createTab" ||
        method == "salamander.sides.closeTab" ||
        method == "salamander.sides.reorderTab" ||
        method == "salamander.sides.moveTab" ||
        method == "salamander.sides.setDetached")
        return "panels.write";
    if (StartsWith(method, "salamander.sides."))
        return "panels.read";
    if (StartsWith(method, "salamander.ui."))
        return "ui.dialogs";
    if (method == "salamander.clipboard.copyText")
        return "clipboard";
    if (StartsWith(method, "salamander.runtimes."))
        return "runtimes";
    if (StartsWith(method, "salamander.ai."))
        return "ai";
    if (StartsWith(method, "salamander.commands."))
        return "commands";
    if (StartsWith(method, "salamander.fileOperations."))
        return "file-operations";
    if (StartsWith(method, "salamander.storage."))
        return "storage";
    if (StartsWith(method, "salamander.events."))
        return "events";
    if (StartsWith(method, "salamander.fileSystem."))
        return "file-system";
    return NULL;
}

static bool ManifestAllowsCapability(
    const CExtensionManifest& manifest, const char* capability)
{
    if (capability == NULL || !manifest.CapabilitiesDeclared)
        return true;
    for (size_t index = 0; index < manifest.Capabilities.size(); ++index)
    {
        if (_stricmp(manifest.Capabilities[index].c_str(), "*") == 0 ||
            _stricmp(manifest.Capabilities[index].c_str(), capability) == 0)
            return true;
    }
    return false;
}

static const char* RuntimeEventName(Events::EventKind kind)
{
    switch (kind)
    {
    case Events::EventKindHostStartup: return "hostStartup";
    case Events::EventKindHostShutdown: return "hostShutdown";
    case Events::EventKindSettingsChanged: return "settingsChanged";
    case Events::EventKindConfigurationChanged: return "configurationChanged";
    case Events::EventKindColorsChanged: return "colorsChanged";
    case Events::EventKindPanelsSwapped: return "panelsSwapped";
    case Events::EventKindActivePanelChanged: return "activePanelChanged";
    case Events::EventKindSidePathChanged: return "sidePathChanged";
    case Events::EventKindSideSelectionChanged: return "sideSelectionChanged";
    case Events::EventKindSideTabChanged: return "sideTabChanged";
    case Events::EventKindSideRefreshed: return "sideRefreshed";
    case Events::EventKindPathChanged: return "pathChanged";
    case Events::EventKindSelectionChanged: return "selectionChanged";
    case Events::EventKindTabChanged: return "tabChanged";
    case Events::EventKindFileChanged: return "fileChanged";
    case Events::EventKindTabCreated: return "tabCreated";
    case Events::EventKindTabClosed: return "tabClosed";
    case Events::EventKindTabReordered: return "tabReordered";
    case Events::EventKindWindowDetached: return "windowDetached";
    case Events::EventKindWindowAttached: return "windowAttached";
    default: return NULL;
    }
}

static BOOL RuntimeEventKindFromName(
    const std::string& name, Events::EventKind* kind)
{
    if (kind == NULL)
        return FALSE;
    for (int value = Events::EventKindHostStartup;
         value <= Events::EventKindWindowAttached; ++value)
    {
        const Events::EventKind candidate =
            static_cast<Events::EventKind>(value);
        const char* candidateName = RuntimeEventName(candidate);
        if (candidateName != NULL &&
            _stricmp(name.c_str(), candidateName) == 0)
        {
            *kind = candidate;
            return TRUE;
        }
    }
    return FALSE;
}

static bool ManifestAllowsEvent(
    const CExtensionManifest& manifest, const std::string& eventName)
{
    if (!manifest.EventsDeclared)
        return true;
    for (size_t index = 0; index < manifest.Events.size(); ++index)
        if (_stricmp(manifest.Events[index].c_str(), eventName.c_str()) == 0)
            return true;
    return false;
}

static Sides::SideReference RuntimeSideFromName(const std::string& name)
{
    if (_stricmp(name.c_str(), "left") == 0)
        return Sides::SideReferenceLeft;
    if (_stricmp(name.c_str(), "right") == 0)
        return Sides::SideReferenceRight;
    if (_stricmp(name.c_str(), "target") == 0)
        return Sides::SideReferenceTarget;
    return Sides::SideReferenceSource;
}

static BOOL RuntimeTrySideFromName(
    const std::string& name, Sides::SideReference* side)
{
    if (side == NULL)
        return FALSE;
    if (_stricmp(name.c_str(), "source") == 0)
        *side = Sides::SideReferenceSource;
    else if (_stricmp(name.c_str(), "target") == 0)
        *side = Sides::SideReferenceTarget;
    else if (_stricmp(name.c_str(), "left") == 0)
        *side = Sides::SideReferenceLeft;
    else if (_stricmp(name.c_str(), "right") == 0)
        *side = Sides::SideReferenceRight;
    else
        return FALSE;
    return TRUE;
}

static BOOL FindRuntimeQuadWord(
    const char* json, const char* member, CQuadWord* value)
{
    if (value == NULL)
        return FALSE;
    std::string raw;
    if (!Runtime::Protocol::Json::FindRawMember(json, member, &raw) ||
        raw.empty())
        return FALSE;
    const char* begin = raw.c_str();
    while (*begin == ' ' || *begin == '\t' ||
           *begin == '\r' || *begin == '\n')
        ++begin;
    if (*begin == '"')
        ++begin;
    if (*begin == '-' || *begin == '\0')
        return FALSE;
    char* end = NULL;
    const ULONGLONG parsed = _strtoui64(begin, &end, 10);
    if (end == begin)
        return FALSE;
    if (*end == '"')
        ++end;
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
        ++end;
    if (*end != '\0')
        return FALSE;
    value->SetUI64(parsed);
    return TRUE;
}

static void CommandEventMasks(
    const CExtensionManifestCommand& command,
    DWORD* orMask,
    DWORD* andMask)
{
    *orMask = MENU_EVENT_TRUE;
    *andMask = MENU_EVENT_TRUE;
    if (_stricmp(command.Requires.c_str(), "disk") == 0)
        *andMask = MENU_EVENT_DISK;
    else if (_stricmp(command.Requires.c_str(), "focused") == 0)
    {
        *orMask = MENU_EVENT_FILE_FOCUSED | MENU_EVENT_DIR_FOCUSED;
        *andMask = MENU_EVENT_DISK;
    }
    else if (_stricmp(command.Requires.c_str(), "file") == 0)
    {
        *orMask = MENU_EVENT_FILE_FOCUSED | MENU_EVENT_FILES_SELECTED;
        *andMask = MENU_EVENT_DISK;
    }
    else if (_stricmp(command.Requires.c_str(), "selection") == 0)
    {
        *orMask = MENU_EVENT_FILES_SELECTED | MENU_EVENT_DIRS_SELECTED;
        *andMask = MENU_EVENT_DISK;
    }
}

static std::string CurrentSalamanderLocale(
    CSalamanderGeneralAbstract* general, WORD* languageId = NULL)
{
    UNREFERENCED_PARAMETER(general);
    WORD id = SalamanderLanguageID;
    if (id == 0)
        id = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    if (languageId != NULL)
        *languageId = id;

    WCHAR localeName[LOCALE_NAME_MAX_LENGTH];
    if (LCIDToLocaleName(
            MAKELCID(id, SORT_DEFAULT), localeName,
            _countof(localeName), 0) == 0)
    {
        return "en-US";
    }
    int length = WideCharToMultiByte(
        CP_UTF8, 0, localeName, -1, NULL, 0, NULL, NULL);
    if (length <= 1)
        return "en-US";
    std::vector<char> utf8(static_cast<size_t>(length));
    if (WideCharToMultiByte(
            CP_UTF8, 0, localeName, -1, &utf8[0], length,
            NULL, NULL) == 0)
    {
        return "en-US";
    }
    return std::string(&utf8[0]);
}

static int LocaleMatchScore(
    const std::string& available, const std::string& preferred)
{
    const size_t availableSeparator = available.find('-');
    const std::string availablePrimary =
        available.substr(0, availableSeparator);
    if (_stricmp(available.c_str(), preferred.c_str()) == 0)
        return 4;
    const size_t preferredSeparator = preferred.find('-');
    const std::string preferredPrimary =
        preferred.substr(0, preferredSeparator);
    if (_stricmp(availablePrimary.c_str(), preferredPrimary.c_str()) == 0)
        return availableSeparator == std::string::npos ? 3 : 2;
    return _stricmp(availablePrimary.c_str(), "en") == 0 ? 1 : 0;
}

static const char* FindLocalizedCommandTitle(
    const CExtensionManifestLocaleText& localized,
    const std::string& commandId)
{
    for (size_t index = 0; index < localized.Commands.size(); ++index)
    {
        if (_stricmp(
                localized.Commands[index].Id.c_str(),
                commandId.c_str()) == 0)
        {
            return localized.Commands[index].Title.c_str();
        }
    }
    return NULL;
}

static const CExtensionManifestLocalizedSetting* FindLocalizedSetting(
    const CExtensionManifestLocaleText& localized,
    const std::string& key)
{
    for (size_t index = 0; index < localized.Settings.size(); ++index)
    {
        if (_stricmp(
                localized.Settings[index].Key.c_str(),
                key.c_str()) == 0)
        {
            return &localized.Settings[index];
        }
    }
    return NULL;
}

static const CExtensionManifestLocalizedFileSystem* FindLocalizedFileSystem(
    const CExtensionManifestLocaleText& localized, const std::string& id)
{
    for (size_t index = 0; index < localized.FileSystems.size(); ++index)
        if (_stricmp(localized.FileSystems[index].Id.c_str(), id.c_str()) == 0)
            return &localized.FileSystems[index];
    return NULL;
}

static const CExtensionManifestLocalizedFileSystemColumn* FindLocalizedFileSystemColumn(
    const CExtensionManifestLocalizedFileSystem& localized, const std::string& id)
{
    for (size_t index = 0; index < localized.Columns.size(); ++index)
        if (_stricmp(localized.Columns[index].Id.c_str(), id.c_str()) == 0)
            return &localized.Columns[index];
    return NULL;
}

static const CExtensionManifestLocalizedFileSystemRootItem* FindLocalizedFileSystemRootItem(
    const CExtensionManifestLocalizedFileSystem& localized, const std::string& id)
{
    for (size_t index = 0; index < localized.RootItems.size(); ++index)
        if (_stricmp(localized.RootItems[index].Id.c_str(), id.c_str()) == 0)
            return &localized.RootItems[index];
    return NULL;
}

static const CExtensionManifestLocalizedFileSystemAction* FindLocalizedFileSystemAction(
    const CExtensionManifestLocalizedFileSystem& localized, const std::string& id)
{
    for (size_t index = 0; index < localized.Actions.size(); ++index)
        if (_stricmp(localized.Actions[index].Id.c_str(), id.c_str()) == 0)
            return &localized.Actions[index];
    return NULL;
}

static void AppendIconWord(std::vector<unsigned char>& output, WORD value)
{
    output.push_back(static_cast<unsigned char>(value & 0xff));
    output.push_back(static_cast<unsigned char>((value >> 8) & 0xff));
}

static void AppendIconDword(std::vector<unsigned char>& output, DWORD value)
{
    output.push_back(static_cast<unsigned char>(value & 0xff));
    output.push_back(static_cast<unsigned char>((value >> 8) & 0xff));
    output.push_back(static_cast<unsigned char>((value >> 16) & 0xff));
    output.push_back(static_cast<unsigned char>((value >> 24) & 0xff));
}

static std::string Base64Encode(const std::vector<unsigned char>& input)
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);
    for (size_t index = 0; index < input.size(); index += 3)
    {
        const unsigned int first = input[index];
        const unsigned int second =
            index + 1 < input.size() ? input[index + 1] : 0;
        const unsigned int third =
            index + 2 < input.size() ? input[index + 2] : 0;
        const unsigned int value = (first << 16) | (second << 8) | third;
        output.push_back(alphabet[(value >> 18) & 0x3f]);
        output.push_back(alphabet[(value >> 12) & 0x3f]);
        output.push_back(
            index + 1 < input.size() ? alphabet[(value >> 6) & 0x3f] : '=');
        output.push_back(
            index + 2 < input.size() ? alphabet[value & 0x3f] : '=');
    }
    return output;
}

static std::string SerializeWindowIcon(HICON icon)
{
    if (icon == NULL)
        return std::string();

    ICONINFO iconInfo;
    memset(&iconInfo, 0, sizeof(iconInfo));
    if (!GetIconInfo(icon, &iconInfo))
        return std::string();

    BITMAP colorBitmap;
    memset(&colorBitmap, 0, sizeof(colorBitmap));
    const bool hasColorBitmap =
        iconInfo.hbmColor != NULL &&
        GetObject(iconInfo.hbmColor, sizeof(colorBitmap), &colorBitmap) != 0;
    const int width = hasColorBitmap ? colorBitmap.bmWidth : 0;
    const int height = hasColorBitmap ? abs(colorBitmap.bmHeight) : 0;
    const DWORD colorBytes =
        width > 0 && height > 0
            ? static_cast<DWORD>(width * height * 4)
            : 0;
    const DWORD maskStride =
        width > 0 ? static_cast<DWORD>(((width + 31) / 32) * 4) : 0;
    const DWORD maskBytes = maskStride * static_cast<DWORD>(height);
    std::vector<unsigned char> colorBits(colorBytes);
    std::vector<unsigned char> maskBits(maskBytes, 0);

    bool copied = false;
    HDC screen = GetDC(NULL);
    if (screen != NULL && colorBytes > 0)
    {
        BITMAPINFO colorInfo;
        memset(&colorInfo, 0, sizeof(colorInfo));
        colorInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        colorInfo.bmiHeader.biWidth = width;
        colorInfo.bmiHeader.biHeight = height;
        colorInfo.bmiHeader.biPlanes = 1;
        colorInfo.bmiHeader.biBitCount = 32;
        colorInfo.bmiHeader.biCompression = BI_RGB;
        copied = GetDIBits(
                     screen, iconInfo.hbmColor, 0, height,
                     &colorBits[0], &colorInfo, DIB_RGB_COLORS) == height;

        if (copied && iconInfo.hbmMask != NULL && maskBytes > 0)
        {
            struct MonoBitmapInfo
            {
                BITMAPINFOHEADER Header;
                RGBQUAD Colors[2];
            } maskInfo;
            memset(&maskInfo, 0, sizeof(maskInfo));
            maskInfo.Header.biSize = sizeof(BITMAPINFOHEADER);
            maskInfo.Header.biWidth = width;
            maskInfo.Header.biHeight = height;
            maskInfo.Header.biPlanes = 1;
            maskInfo.Header.biBitCount = 1;
            maskInfo.Header.biCompression = BI_RGB;
            maskInfo.Colors[1].rgbBlue = 255;
            maskInfo.Colors[1].rgbGreen = 255;
            maskInfo.Colors[1].rgbRed = 255;
            GetDIBits(
                screen, iconInfo.hbmMask, 0, height,
                &maskBits[0], reinterpret_cast<BITMAPINFO*>(&maskInfo),
                DIB_RGB_COLORS);
        }
        ReleaseDC(NULL, screen);
    }

    // GetIconInfo creates independent bitmap handles even for shared icons.
    if (iconInfo.hbmColor != NULL)
        ::DeleteObject(iconInfo.hbmColor);
    if (iconInfo.hbmMask != NULL)
        ::DeleteObject(iconInfo.hbmMask);
    if (!copied || width > 255 || height > 255)
        return std::string();

    const DWORD imageBytes =
        sizeof(BITMAPINFOHEADER) + colorBytes + maskBytes;
    std::vector<unsigned char> ico;
    ico.reserve(6 + 16 + imageBytes);
    AppendIconWord(ico, 0);
    AppendIconWord(ico, 1);
    AppendIconWord(ico, 1);
    ico.push_back(static_cast<unsigned char>(width));
    ico.push_back(static_cast<unsigned char>(height));
    ico.push_back(0);
    ico.push_back(0);
    AppendIconWord(ico, 1);
    AppendIconWord(ico, 32);
    AppendIconDword(ico, imageBytes);
    AppendIconDword(ico, 22);
    AppendIconDword(ico, sizeof(BITMAPINFOHEADER));
    AppendIconDword(ico, static_cast<DWORD>(width));
    AppendIconDword(ico, static_cast<DWORD>(height * 2));
    AppendIconWord(ico, 1);
    AppendIconWord(ico, 32);
    AppendIconDword(ico, BI_RGB);
    AppendIconDword(ico, colorBytes);
    AppendIconDword(ico, 0);
    AppendIconDword(ico, 0);
    AppendIconDword(ico, 0);
    AppendIconDword(ico, 0);
    ico.insert(ico.end(), colorBits.begin(), colorBits.end());
    ico.insert(ico.end(), maskBits.begin(), maskBits.end());
    return Base64Encode(ico);
}

static Automation::IScriptRunner* QueryScriptRunner(
    CSalamanderGeneralAbstract* general)
{
    if (general == NULL)
        return NULL;
    CSalamanderServiceQuery query;
    CSalamanderServiceResult result;
    memset(&query, 0, sizeof(query));
    memset(&result, 0, sizeof(result));
    query.ServiceId = SALAMATRIX_SERVICE_SCRIPT_RUNNER;
    query.MinimumVersion = SALAMATRIX_SCRIPT_RUNNER_VERSION_1_0;
    if (!general->QueryService(&query, &result) || result.Interface == NULL)
        return NULL;
    return static_cast<Automation::IScriptRunner*>(result.Interface);
}

static bool IsExecutableAvailable(const std::string& executable)
{
    if (executable.empty())
        return true;
    std::wstring name(executable.begin(), executable.end());
    wchar_t path[32768];
    const DWORD length = SearchPathW(
        NULL, name.c_str(), NULL, _countof(path), path, NULL);
    return length > 0 && length < _countof(path);
}
}

class ScopedExclusiveSRWLock
{
private:
    SRWLOCK* Lock;

public:
    explicit ScopedExclusiveSRWLock(SRWLOCK* lock, BOOL tryOnly = FALSE) : Lock(NULL)
    {
        if (tryOnly)
        {
            if (TryAcquireSRWLockExclusive(lock))
                Lock = lock;
        }
        else
        {
            AcquireSRWLockExclusive(lock);
            Lock = lock;
        }
    }
    ScopedExclusiveSRWLock(SRWLOCK* lock, HWND mainWindow) : Lock(NULL)
    {
        const DWORD mainThreadId = mainWindow != NULL
            ? GetWindowThreadProcessId(mainWindow, NULL) : 0;
        if (mainThreadId == 0 || mainThreadId != GetCurrentThreadId())
        {
            AcquireSRWLockExclusive(lock);
            Lock = lock;
            return;
        }
        while (!TryAcquireSRWLockExclusive(lock))
        {
            const DWORD wait = MsgWaitForMultipleObjects(
                0, NULL, FALSE, 50, QS_SENDMESSAGE);
            if (wait == WAIT_OBJECT_0)
            {
                // Dispatch cross-thread SendMessage calls needed by the
                // listing worker, but do not consume posted UI input here.
                MSG message;
                PeekMessage(&message, NULL, WM_NULL, WM_NULL, PM_NOREMOVE);
            }
        }
        Lock = lock;
    }
    ~ScopedExclusiveSRWLock()
    {
        if (Lock != NULL)
            ReleaseSRWLockExclusive(Lock);
    }
    BOOL IsLocked() const { return Lock != NULL; }
};

struct PackageManager::Package
{
    PackageManager* Owner;
    CExtensionManifest Manifest;
    std::vector<CExtensionManifestCommand> InitialCommands;
    Extensions::ExtensionDescriptor Descriptor;
    std::wstring Directory;
    std::wstring EntryPoint;
    std::string Id;
    std::string EntryPointUtf8;
    std::string IconPath;
    std::string IconDarkPath;
    BOOL RuntimeUsable;
    BOOL SettingsReady;
    volatile LONG Stopping;
    CSalamanderForOperationsAbstract* Operations;
    UI::IProgressDialog* Progress;
    ULONGLONG ProgressId;
    struct RuntimeDialog
    {
        Package* Owner;
        ULONGLONG Id;
        UI::IDialog* Dialog;
        BOOL EventsEnabled;
        char EventName[128];
    };
    std::vector<RuntimeDialog*> Dialogs;
    ULONGLONG NextDialogId;
    std::vector<int> CommandIds;
    std::vector<DWORD> CommandHotKeys;
    std::vector<std::string> CommandIconPaths;
    std::vector<std::string> CommandIconDarkPaths;
    std::vector<int> InitialCommandIds;
    std::vector<DWORD> InitialCommandHotKeys;
    std::vector<std::string> InitialCommandIconPaths;
    std::vector<std::string> InitialCommandIconDarkPaths;
    BOOL CommandsChanged;
    std::vector<int> MenuIconIndices;
    std::vector<ULONGLONG> EventSubscriptions;
    Runtime::IRuntimeSession* Session;
    HANDLE PumpThread;
    SRWLOCK FileSystemExecutionLock;
    SRWLOCK FileSystemActionExecutionLock;
    volatile LONG FileSystemActionGeneration;
    volatile LONG FileSystemActionPending;
    BOOL FileSystemListing;
    const CExtensionManifestFileSystem* ListingFileSystem;
    std::vector<PackageManager::FileSystemItem> PendingFileSystemItems;

    Package(PackageManager* owner)
        : Owner(owner),
          RuntimeUsable(FALSE),
          SettingsReady(TRUE),
          Stopping(FALSE),
          Operations(NULL),
          Progress(NULL),
          ProgressId(0),
          NextDialogId(1),
          CommandsChanged(FALSE),
          Session(NULL),
          PumpThread(NULL),
          FileSystemActionGeneration(0),
          FileSystemActionPending(FALSE),
          FileSystemListing(FALSE),
          ListingFileSystem(NULL)
    {
        InitializeSRWLock(&FileSystemExecutionLock);
        InitializeSRWLock(&FileSystemActionExecutionLock);
    }
};

class PackageManager::ExecutionGuard
{
private:
    PackageManager* Owner;

    ExecutionGuard(const ExecutionGuard&);
    ExecutionGuard& operator=(const ExecutionGuard&);

public:
    explicit ExecutionGuard(PackageManager* owner)
        : Owner(owner)
    {
        if (Owner != NULL)
            Owner->BeginExecution();
    }

    ~ExecutionGuard()
    {
        if (Owner != NULL)
            Owner->FinishExecution();
    }
};

struct PackageManager::FileSystemActionTask
{
    PackageManager* Owner;
    Package* PackageContext;
    LONG Generation;
    std::string PackageId;
    std::string FileSystemId;
    std::string ActionId;
    std::string InvocationJson;
};

class PackageManager::MenuExtension : public CPluginInterfaceForMenuExtAbstract
{
private:
    PackageManager* Owner;

public:
    explicit MenuExtension(PackageManager* owner) : Owner(owner) {}

    virtual DWORD WINAPI GetMenuItemState(int id, DWORD eventMask)
    {
        if (Owner == NULL)
            return 0;
        if (id == CommandOpenAutomationApiReference)
        {
            char path[SAL_MAX_PATH];
            return Documentation::GetAutomationApiReferencePath(
                       Owner->General, path, _countof(path))
                       ? MENU_ITEM_STATE_ENABLED
                       : 0;
        }
        for (size_t p = 0; p < Owner->Packages.size(); ++p)
        {
            Package* package = Owner->Packages[p];
            if (!package->RuntimeUsable)
                continue;
            for (size_t c = 0; c < package->CommandIds.size(); ++c)
            {
                if (package->CommandIds[c] == id &&
                    c < package->Manifest.Commands.size())
                {
                    DWORD orMask = MENU_EVENT_TRUE;
                    DWORD andMask = MENU_EVENT_TRUE;
                    CommandEventMasks(
                        package->Manifest.Commands[c],
                        &orMask, &andMask);
                    const BOOL contextMatches =
                        (eventMask & orMask) != 0 &&
                        (eventMask & andMask) == andMask;
                    return package->Manifest.Commands[c].Enabled &&
                                   contextMatches
                               ? MENU_ITEM_STATE_ENABLED
                               : 0;
                }
            }
        }
        return 0;
    }

    virtual BOOL WINAPI ExecuteMenuItem(
        CSalamanderForOperationsAbstract* salamander,
        HWND parent,
        int id,
        DWORD eventMask)
    {
        UNREFERENCED_PARAMETER(salamander);
        UNREFERENCED_PARAMETER(eventMask);
        if (Owner == NULL)
            return FALSE;
        ExecutionGuard execution(Owner);
        if (id == CommandOpenAutomationApiReference)
            return Documentation::OpenAutomationApiReference(
                Owner->General, parent);
        for (size_t p = 0; p < Owner->Packages.size(); ++p)
        {
            Package* package = Owner->Packages[p];
            if (!package->RuntimeUsable)
                continue;
            for (size_t c = 0; c < package->CommandIds.size(); ++c)
            {
                if (package->CommandIds[c] == id &&
                    c < package->Manifest.Commands.size())
                {
                    const CExtensionManifestCommand& command =
                        package->Manifest.Commands[c];
                    if (!command.Enabled)
                        return FALSE;
                    if (!command.Path.empty())
                    {
                        if (!ManifestAllowsCapability(
                                package->Manifest, "panels.write"))
                            return FALSE;
                        int failReason = 0;
                        return Owner->General != NULL
                                   ? Owner->General->ChangePanelPath(
                                         PANEL_SOURCE, command.Path.c_str(),
                                         &failReason)
                                   : FALSE;
                    }
                    return Owner->ExecuteCommand(
                        package, salamander,
                        command.Id.c_str(), command.Handler.c_str());
                }
            }
        }
        return FALSE;
    }

    virtual BOOL WINAPI HelpForMenuItem(HWND parent, int id)
    {
        UNREFERENCED_PARAMETER(parent);
        UNREFERENCED_PARAMETER(id);
        return FALSE;
    }

    virtual void WINAPI BuildMenu(HWND parent, CSalamanderBuildMenuAbstract* builder)
    {
        UNREFERENCED_PARAMETER(parent);
        if (Owner == NULL || builder == NULL)
            return;
        int iconCount = 0;
        for (size_t p = 0; p < Owner->Packages.size(); ++p)
        {
            if (!Owner->Packages[p]->RuntimeUsable)
                continue;
            for (size_t c = 0; c < Owner->Packages[p]->Manifest.Commands.size(); ++c)
                if (Owner->Packages[p]->Manifest.Commands[c].Visible &&
                    Owner->Packages[p]->Manifest.Commands[c].Menu != "none")
                    ++iconCount;
            int packageMenuCommandCount = 0;
            for (size_t c = 0;
                 c < Owner->Packages[p]->Manifest.Commands.size(); ++c)
            {
                const CExtensionManifestCommand& command =
                    Owner->Packages[p]->Manifest.Commands[c];
                if (command.Visible &&
                    command.Menu != "none")
                    ++packageMenuCommandCount;
            }
            if (packageMenuCommandCount > 1 &&
                !Owner->Packages[p]->IconPath.empty())
                ++iconCount;
        }

        CGUIIconListAbstract* icons = NULL;
        if (SalamanderGUI != NULL && iconCount > 0)
        {
            icons = SalamanderGUI->CreateIconList();
            if (icons == NULL || !icons->Create(16, 16, iconCount))
            {
                if (icons != NULL)
                    SalamanderGUI->DestroyIconList(icons);
                icons = NULL;
            }
        }
        if (icons != NULL)
            builder->SetIconListForMenu(icons);

        int imageIndex = 0;
        bool packageMenuItemAdded = false;
        for (size_t p = 0; p < Owner->Packages.size(); ++p)
        {
            Package* package = Owner->Packages[p];
            if (!package->RuntimeUsable)
                continue;
            package->MenuIconIndices.clear();
            int packageMenuCommandCount = 0;
            for (size_t c = 0; c < package->Manifest.Commands.size(); ++c)
            {
                const std::string& menu = package->Manifest.Commands[c].Menu;
                if (package->Manifest.Commands[c].Visible &&
                    menu != "none")
                    ++packageMenuCommandCount;
            }
            if (packageMenuCommandCount > 1)
            {
                char packageTitle[256];
                StringCchCopyA(
                    packageTitle, _countof(packageTitle),
                    package->Manifest.Name.c_str());
                if (Owner->General != NULL)
                    Owner->General->DuplicateAmpersands(
                        packageTitle, _countof(packageTitle));
                int packageIconIndex = -1;
                if (icons != NULL && !package->IconPath.empty())
                {
                    packageIconIndex = imageIndex++;
                    const char* preferredPath =
                        DarkModeIsWindowsDarkSchemeSelected() &&
                                !package->IconDarkPath.empty()
                            ? package->IconDarkPath.c_str()
                            : package->IconPath.c_str();
                    HICON icon =
                        SalamanderGUI->CreateSVGIcon(preferredPath, 16);
                    if (icon == NULL &&
                        preferredPath != package->IconPath.c_str())
                    {
                        icon = SalamanderGUI->CreateSVGIcon(
                            package->IconPath.c_str(), 16);
                    }
                    if (icon != NULL)
                    {
                        icons->ReplaceIcon(packageIconIndex, icon);
                        DestroyIcon(icon);
                    }
                    else
                        packageIconIndex = -1;
                }
                builder->AddSubmenuStart(
                    packageIconIndex, packageTitle, 0, FALSE,
                    MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);
            }
            for (size_t c = 0; c < package->Manifest.Commands.size(); ++c)
            {
                const CExtensionManifestCommand& command =
                    package->Manifest.Commands[c];
                if (!command.Visible ||
                    command.Menu == "none")
                    continue;
                char title[256];
                StringCchCopyA(title, _countof(title), command.Title.c_str());
                int iconIndex = -1;
                if (icons != NULL)
                {
                    iconIndex = imageIndex++;
                    const std::string& iconPath = package->CommandIconPaths[c];
                    const std::string& iconDarkPath =
                        package->CommandIconDarkPaths[c];
                    const bool useDarkIcon =
                        DarkModeIsWindowsDarkSchemeSelected() &&
                        !iconDarkPath.empty();
                    const char* preferredPath =
                        useDarkIcon ? iconDarkPath.c_str() : iconPath.c_str();
                    HICON icon = iconPath.empty()
                                     ? NULL
                                     : SalamanderGUI->CreateSVGIcon(
                                           preferredPath, 16);
                    if (icon == NULL && useDarkIcon)
                        icon = SalamanderGUI->CreateSVGIcon(iconPath.c_str(), 16);
                    if (icon != NULL)
                    {
                        icons->ReplaceIcon(iconIndex, icon);
                        DestroyIcon(icon);
                    }
                    else
                        iconIndex = -1;
                }
                package->MenuIconIndices.push_back(iconIndex);
                DWORD orMask = MENU_EVENT_TRUE;
                DWORD andMask = MENU_EVENT_TRUE;
                CommandEventMasks(command, &orMask, &andMask);
                builder->AddMenuItem(
                    iconIndex, title,
                    c < package->CommandHotKeys.size()
                        ? package->CommandHotKeys[c]
                        : 0,
                    package->CommandIds[c], TRUE,
                    orMask, andMask, MENU_SKILLLEVEL_ALL);
                packageMenuItemAdded = true;
            }
            if (packageMenuCommandCount > 1)
                builder->AddSubmenuEnd();
        }
        if (packageMenuItemAdded)
            builder->AddMenuItem(-1, NULL, 0, 0, FALSE, 0, 0,
                                 MENU_SKILLLEVEL_ALL);
        builder->AddMenuItem(
            -1, "Automation API &Reference...", 0,
            CommandOpenAutomationApiReference, TRUE,
            MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);
    }
};

class PackageManager::ViewerExtension : public CPluginInterfaceForViewerAbstract
{
private:
    PackageManager* Owner;

public:
    explicit ViewerExtension(PackageManager* owner) : Owner(owner) {}

    virtual BOOL WINAPI ViewFile(
        const char* name, int left, int top, int width, int height,
        UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock,
        BOOL* lockOwner, CSalamanderPluginViewerData* viewerData,
        int enumFilesSourceUID, int enumFilesCurrentIndex)
    {
        UNREFERENCED_PARAMETER(returnLock);
        if (lock != NULL)
            *lock = NULL;
        if (lockOwner != NULL)
            *lockOwner = FALSE;
        if (Owner == NULL || name == NULL)
            return FALSE;
        std::string invocation =
            std::string("{\"role\":\"viewer\",\"path\":\"") +
            JsonEscape(name) + "\",\"window\":{" +
            "\"left\":" + std::to_string(left) +
            ",\"top\":" + std::to_string(top) +
            ",\"width\":" + std::to_string(width) +
            ",\"height\":" + std::to_string(height) +
            ",\"showCmd\":" + std::to_string(showCmd) +
            ",\"alwaysOnTop\":" + (alwaysOnTop ? "true" : "false") +
            "},\"enumFilesSourceUID\":" +
            std::to_string(enumFilesSourceUID) +
            ",\"enumFilesCurrentIndex\":" +
            std::to_string(enumFilesCurrentIndex) + "}";
        const char* viewerLabel = NULL;
        if (viewerData != NULL &&
            viewerData->Size >= sizeof(CSalamanderPluginViewerSelectionData))
        {
            const CSalamanderPluginViewerSelectionData* selection =
                static_cast<const CSalamanderPluginViewerSelectionData*>(viewerData);
            if (selection->SelectionMagic ==
                SALAMANDER_PLUGIN_VIEWER_SELECTION_MAGIC)
                viewerLabel = selection->ViewerLabel;
        }
        return Owner->RunViewer(name, invocation.c_str(), viewerLabel);
    }

    virtual BOOL WINAPI CanViewFile(const char* name)
    {
        UNREFERENCED_PARAMETER(name);
        // Salamander calls this interface only after matching a mask
        // registered by RegisterViewerMasks. Content sniffing stays optional
        // in v1 and can be done by the selected handler before opening UI.
        return TRUE;
    }
};

struct SalamatrixFileSystemItemData
{
    PackageManager::FileSystemItem Item;
    std::string PackageId;
    std::string FileSystemId;
};

static int WINAPI SalamatrixFileSystemSimpleIconIndex()
{
    return 0;
}

static BOOL SalamatrixFileIconPathToWide(
    const std::string& value, std::wstring* result)
{
    if (result == NULL)
        return FALSE;
    const int length = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1, NULL, 0);
    if (length <= 0)
        return FALSE;
    std::vector<wchar_t> buffer(static_cast<size_t>(length));
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1,
            &buffer[0], length) <= 0)
        return FALSE;
    result->assign(&buffer[0]);
    return TRUE;
}

static HICON SalamatrixExtractFileIcon(
    const std::string& fileIcon, int iconSize)
{
    if (fileIcon.empty())
        return NULL;
    std::wstring path;
    if (!SalamatrixFileIconPathToWide(fileIcon, &path))
        return NULL;
    std::wstring extractionPath = path;
    if (extractionPath.size() >= MAX_PATH &&
        extractionPath.compare(0, 4, L"\\\\?\\") != 0)
    {
        extractionPath = extractionPath.compare(0, 2, L"\\\\") == 0
                             ? L"\\\\?\\UNC\\" + extractionPath.substr(2)
                             : L"\\\\?\\" + extractionPath;
    }
    const int size = iconSize == SALICONSIZE_32 ? 32 : 16;
    HICON icon = NULL;
    UINT iconId = 0;
    UINT extracted = PrivateExtractIconsW(
        extractionPath.c_str(), 0, size, size, &icon, &iconId, 1,
        LR_DEFAULTCOLOR);
    if (extracted != 0 && extracted != static_cast<UINT>(-1) && icon != NULL)
        return icon;
    if (icon != NULL)
    {
        DestroyIcon(icon);
        icon = NULL;
    }
    if (extractionPath != path)
    {
        extracted = PrivateExtractIconsW(
            path.c_str(), 0, size, size, &icon, &iconId, 1,
            LR_DEFAULTCOLOR);
        if (extracted != 0 && extracted != static_cast<UINT>(-1) && icon != NULL)
            return icon;
        if (icon != NULL)
            DestroyIcon(icon);
    }
    return NULL;
}

static const CFileData** SalamatrixFsTransferFileData = NULL;
static int* SalamatrixFsTransferIsDir = NULL;
static char* SalamatrixFsTransferBuffer = NULL;
static int* SalamatrixFsTransferLen = NULL;
static DWORD* SalamatrixFsTransferActCustomData = NULL;
static volatile LONG SalamatrixFsNameColumnWidth[2] = {180, 180};
static volatile LONG SalamatrixFsNameColumnFixed[2] = {1, 1};
static const DWORD SALAMATRIX_FS_SIZE_COLUMN = 0x40000000;
static const DWORD SALAMATRIX_FS_DATETIME_COLUMN = 0x20000000;
static const DWORD SALAMATRIX_FS_SORT_KEY = 0x80000000;
static const DWORD SALAMATRIX_FS_COLUMN_FLAGS =
    SALAMATRIX_FS_SIZE_COLUMN | SALAMATRIX_FS_DATETIME_COLUMN |
    SALAMATRIX_FS_SORT_KEY;

static std::string SalamatrixFormatFileSystemDateTime(const std::string& value)
{
    char* end = NULL;
    const __int64 milliseconds = _strtoi64(value.c_str(), &end, 10);
    if (end == value.c_str() || end == NULL || *end != '\0')
        return value;
    const __int64 windowsEpochMilliseconds = 11644473600000LL;
    const unsigned __int64 ticks =
        static_cast<unsigned __int64>(milliseconds + windowsEpochMilliseconds) *
        10000ULL;
    FILETIME utc = {
        static_cast<DWORD>(ticks), static_cast<DWORD>(ticks >> 32)};
    SYSTEMTIME utcSystemTime;
    SYSTEMTIME systemTime;
    if (!FileTimeToSystemTime(&utc, &utcSystemTime) ||
        !SystemTimeToTzSpecificLocalTime(
            NULL, &utcSystemTime, &systemTime))
        return value;
    wchar_t date[80];
    wchar_t time[80];
    if (GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systemTime,
                       NULL, date, _countof(date)) == 0 ||
        GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &systemTime,
                       NULL, time, _countof(time)) == 0)
        return value;
    std::wstring formatted(date);
    formatted += L" ";
    formatted += time;
    const int length = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, formatted.c_str(), -1,
        NULL, 0, NULL, NULL);
    if (length <= 1)
        return value;
    std::string result(static_cast<size_t>(length), '\0');
    if (WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, formatted.c_str(), -1,
            &result[0], length, NULL, NULL) == 0)
        return value;
    result.resize(static_cast<size_t>(length - 1));
    return result;
}

static std::string SalamatrixFormatFileSystemColumnValue(
    const std::string& value, bool sizeColumn)
{
    if (!sizeColumn || value.empty())
        return value;
    char* end = NULL;
    const unsigned __int64 bytes = _strtoui64(value.c_str(), &end, 10);
    if (end == value.c_str() || end == NULL || *end != '\0')
        return value;
    if (SalamanderGeneral == NULL)
        return value;
    int sizeFormat = 2;
    int configType = 0;
    SalamanderGeneral->GetConfigParameter(
        SALCFG_SIZEFORMAT, &sizeFormat, sizeof(sizeFormat), &configType);
    int printMode = 0;
    if (sizeFormat == 0)
        printMode = 2;
    else if (sizeFormat == 1)
        printMode = 3;
    char formatted[200];
    const CQuadWord size(
        static_cast<DWORD>(bytes), static_cast<DWORD>(bytes >> 32));
    SalamanderGeneral->PrintDiskSize(formatted, size, printMode);
    return formatted;
}

static void WINAPI SalamatrixFileSystemColumnText()
{
    *SalamatrixFsTransferLen = 0;
    if (SalamatrixFsTransferFileData == NULL || *SalamatrixFsTransferFileData == NULL ||
        SalamatrixFsTransferIsDir == NULL || *SalamatrixFsTransferIsDir == 2 ||
        SalamatrixFsTransferActCustomData == NULL || *SalamatrixFsTransferActCustomData == 0)
        return;
    const SalamatrixFileSystemItemData* data =
        reinterpret_cast<const SalamatrixFileSystemItemData*>((*SalamatrixFsTransferFileData)->PluginData);
    const DWORD customData = *SalamatrixFsTransferActCustomData;
    const bool sizeColumn = (customData & SALAMATRIX_FS_SIZE_COLUMN) != 0;
    const bool dateTimeColumn =
        (customData & SALAMATRIX_FS_DATETIME_COLUMN) != 0;
    const bool sortKey = (customData & SALAMATRIX_FS_SORT_KEY) != 0;
    const size_t index = static_cast<size_t>(
        (customData & ~SALAMATRIX_FS_COLUMN_FLAGS) - 1);
    if (data == NULL || index >= data->Item.ColumnValues.size())
        return;
    const std::string value = dateTimeColumn && !sortKey
        ? SalamatrixFormatFileSystemDateTime(data->Item.ColumnValues[index])
        : SalamatrixFormatFileSystemColumnValue(
              data->Item.ColumnValues[index], sizeColumn);
    const size_t length = min(value.size(), static_cast<size_t>(TRANSFER_BUFFER_MAX));
    memcpy(SalamatrixFsTransferBuffer, value.data(), length);
    *SalamatrixFsTransferLen = static_cast<int>(length);
}

static const DWORD SALAMATRIX_FS_NAME_COLUMN_DETAILED = 0xFFFFFFFE;
static const DWORD SALAMATRIX_FS_NAME_COLUMN_COMPACT = 0xFFFFFFFF;

static void WINAPI SalamatrixFileSystemNameText()
{
    *SalamatrixFsTransferLen = 0;
    if (SalamatrixFsTransferFileData == NULL ||
        *SalamatrixFsTransferFileData == NULL ||
        SalamatrixFsTransferIsDir == NULL || *SalamatrixFsTransferIsDir == 2)
        return;
    const SalamatrixFileSystemItemData* data =
        reinterpret_cast<const SalamatrixFileSystemItemData*>(
            (*SalamatrixFsTransferFileData)->PluginData);
    if (data == NULL)
        return;
    const std::string& value =
        SalamatrixFsTransferActCustomData != NULL &&
                *SalamatrixFsTransferActCustomData == SALAMATRIX_FS_NAME_COLUMN_COMPACT &&
                !data->Item.CompactName.empty()
            ? data->Item.CompactName
            : data->Item.Name;
    const size_t length = min(
        value.size(), static_cast<size_t>(TRANSFER_BUFFER_MAX));
    memcpy(SalamatrixFsTransferBuffer, value.data(), length);
    *SalamatrixFsTransferLen = static_cast<int>(length);
}

class SalamatrixFileSystemPluginData : public CPluginDataInterfaceAbstract
{
private:
    HIMAGELIST Images;
    std::vector<CExtensionManifestFileSystem::Column> Columns;
    std::string DefaultFileIcon;

public:
    SalamatrixFileSystemPluginData(
        const std::vector<CExtensionManifestFileSystem::Column>& columns,
        const std::string& defaultFileIcon)
        : Images(NULL), Columns(columns), DefaultFileIcon(defaultFileIcon) {}
    virtual ~SalamatrixFileSystemPluginData()
    {
        if (Images != NULL)
            ImageList_Destroy(Images);
    }

    virtual BOOL WINAPI CallReleaseForFiles() { return TRUE; }
    virtual BOOL WINAPI CallReleaseForDirs() { return TRUE; }
    virtual void WINAPI ReleasePluginData(CFileData& file, BOOL isDir)
    {
        UNREFERENCED_PARAMETER(isDir);
        delete reinterpret_cast<SalamatrixFileSystemItemData*>(file.PluginData);
        file.PluginData = 0;
    }
    virtual void WINAPI GetFileDataForUpDir(const char* archivePath, CFileData& upDir)
    { UNREFERENCED_PARAMETER(archivePath); UNREFERENCED_PARAMETER(upDir); }
    virtual BOOL WINAPI GetFileDataForNewDir(const char* dirName, CFileData& dir)
    { UNREFERENCED_PARAMETER(dirName); UNREFERENCED_PARAMETER(dir); return TRUE; }
    virtual HIMAGELIST WINAPI GetSimplePluginIcons(int iconSize)
    {
        if (Images != NULL)
        {
            ImageList_Destroy(Images);
            Images = NULL;
        }
        const int size = iconSize == SALICONSIZE_32 ? 32 : 16;
        Images = ImageList_Create(size, size, ILC_COLOR32 | ILC_MASK, 1, 1);
        if (Images == NULL)
            return NULL;
        HICON icon = reinterpret_cast<HICON>(LoadImage(
            DLLInstance, MAKEINTRESOURCE(IDI_PLUGINICON), IMAGE_ICON,
            size, size, SalamanderGeneral->GetIconLRFlags()));
        if (icon != NULL)
        {
            ImageList_ReplaceIcon(Images, -1, icon);
            DestroyIcon(icon);
        }
        return Images;
    }
    virtual BOOL WINAPI HasSimplePluginIcon(CFileData& file, BOOL isDir)
    {
        UNREFERENCED_PARAMETER(isDir);
        const SalamatrixFileSystemItemData* data =
            reinterpret_cast<const SalamatrixFileSystemItemData*>(file.PluginData);
        return data == NULL ||
               (data->Item.Icon.empty() && data->Item.IconDark.empty() &&
                data->Item.FileIcon.empty() && DefaultFileIcon.empty());
    }
    virtual HICON WINAPI GetPluginIcon(
        const CFileData* file, int iconSize, BOOL& destroyIcon)
    {
        destroyIcon = TRUE;
        const SalamatrixFileSystemItemData* data = file != NULL
            ? reinterpret_cast<const SalamatrixFileSystemItemData*>(file->PluginData)
            : NULL;
        if (data != NULL && SalamanderGUI != NULL)
        {
            const bool dark = DarkModeIsWindowsDarkSchemeSelected() &&
                              !data->Item.IconDark.empty();
            const std::string& preferred = dark
                ? data->Item.IconDark : data->Item.Icon;
            HICON icon = preferred.empty() ? NULL
                : SalamanderGUI->CreateSVGIcon(
                      preferred.c_str(), iconSize == SALICONSIZE_32 ? 32 : 16);
            if (icon == NULL && dark && !data->Item.Icon.empty())
                icon = SalamanderGUI->CreateSVGIcon(
                    data->Item.Icon.c_str(), iconSize == SALICONSIZE_32 ? 32 : 16);
            if (icon != NULL)
                return icon;
        }
        HICON icon = data != NULL
            ? SalamatrixExtractFileIcon(data->Item.FileIcon, iconSize) : NULL;
        if (icon != NULL)
            return icon;
        icon = SalamatrixExtractFileIcon(DefaultFileIcon, iconSize);
        if (icon != NULL)
            return icon;
        return reinterpret_cast<HICON>(LoadImage(
            DLLInstance, MAKEINTRESOURCE(IDI_PLUGINICON), IMAGE_ICON,
            iconSize == SALICONSIZE_32 ? 32 : 16,
            iconSize == SALICONSIZE_32 ? 32 : 16,
            SalamanderGeneral->GetIconLRFlags()));
    }
    virtual int WINAPI CompareFilesFromFS(
        const CFileData* file1, const CFileData* file2)
    {
        const SalamatrixFileSystemItemData* data1 =
            reinterpret_cast<const SalamatrixFileSystemItemData*>(
                file1->PluginData);
        const SalamatrixFileSystemItemData* data2 =
            reinterpret_cast<const SalamatrixFileSystemItemData*>(
                file2->PluginData);
        if (data1 != NULL && data2 != NULL)
        {
            int result = lstrcmpiA(data1->Item.Id.c_str(), data2->Item.Id.c_str());
            return result != 0 ? result : strcmp(data1->Item.Id.c_str(), data2->Item.Id.c_str());
        }
        int result = lstrcmpiA(file1->Name, file2->Name);
        return result != 0 ? result : strcmp(file1->Name, file2->Name);
    }
    virtual void WINAPI SetupView(
        BOOL leftPanel, CSalamanderViewAbstract* view,
        const char* archivePath, const CFileData* upperDir)
    {
        UNREFERENCED_PARAMETER(archivePath);
        UNREFERENCED_PARAMETER(upperDir);
        const CFileData** fileData = NULL;
        int* isDir = NULL;
        char* buffer = NULL;
        int* length = NULL;
        DWORD* rowData = NULL;
        CPluginDataInterfaceAbstract** pluginData = NULL;
        DWORD* customData = NULL;
        view->GetTransferVariables(fileData, isDir, buffer, length, rowData, pluginData, customData);
        SalamatrixFsTransferFileData = fileData;
        SalamatrixFsTransferIsDir = isDir;
        SalamatrixFsTransferBuffer = buffer;
        SalamatrixFsTransferLen = length;
        SalamatrixFsTransferActCustomData = customData;
        view->SetPluginSimpleIconCallback(SalamatrixFileSystemSimpleIconIndex);
        CColumn* nameColumn = const_cast<CColumn*>(view->GetColumn(0));
        if (nameColumn != NULL)
        {
            nameColumn->GetText = SalamatrixFileSystemNameText;
            nameColumn->CustomData = view->GetViewMode() == VIEW_MODE_DETAILED
                                         ? SALAMATRIX_FS_NAME_COLUMN_DETAILED
                                         : SALAMATRIX_FS_NAME_COLUMN_COMPACT;
            nameColumn->ID = COLUMN_ID_CUSTOM;
            const int panelIndex = leftPanel ? 0 : 1;
            nameColumn->Width = static_cast<int>(InterlockedCompareExchange(
                &SalamatrixFsNameColumnWidth[panelIndex], 0, 0));
            nameColumn->FixedWidth = static_cast<unsigned>(InterlockedCompareExchange(
                &SalamatrixFsNameColumnFixed[panelIndex], 0, 0) != 0);
        }
        if (view->GetViewMode() == VIEW_MODE_DETAILED)
        {
            int insertAt = view->GetColumnsCount();
            for (size_t index = 0; index < Columns.size(); ++index)
            {
                CColumn column = {};
                StringCchCopyA(column.Name, _countof(column.Name), Columns[index].Name.c_str());
                StringCchCopyA(column.Description, _countof(column.Description), Columns[index].Description.c_str());
                column.GetText = SalamatrixFileSystemColumnText;
                column.CustomData = static_cast<DWORD>(index + 1) |
                    (Columns[index].Size ? SALAMATRIX_FS_SIZE_COLUMN : 0) |
                    (Columns[index].DateTime ? SALAMATRIX_FS_DATETIME_COLUMN : 0);
                column.SupportSorting = 1;
                column.LeftAlignment = Columns[index].Numeric ? 0 : 1;
                column.ID = COLUMN_ID_CUSTOM;
                column.Width = Columns[index].Width;
                column.FixedWidth = 1;
                view->InsertColumn(insertAt++, &column);
            }
        }
    }
    virtual void WINAPI ColumnFixedWidthShouldChange(
        BOOL leftPanel, const CColumn* column, int newFixedWidth)
    {
        if (column != NULL &&
            column->CustomData == SALAMATRIX_FS_NAME_COLUMN_DETAILED)
            InterlockedExchange(&SalamatrixFsNameColumnFixed[leftPanel ? 0 : 1],
                                newFixedWidth != 0 ? 1 : 0);
    }
    virtual void WINAPI ColumnWidthWasChanged(
        BOOL leftPanel, const CColumn* column, int newWidth)
    {
        if (column != NULL &&
            column->CustomData == SALAMATRIX_FS_NAME_COLUMN_DETAILED &&
            newWidth >= 24)
            InterlockedExchange(&SalamatrixFsNameColumnWidth[leftPanel ? 0 : 1],
                                newWidth);
    }
    virtual BOOL WINAPI GetInfoLineContent(
        int panel, const CFileData* file, BOOL isDir, int selectedFiles,
        int selectedDirs, BOOL displaySize, const CQuadWord& selectedSize,
        char* buffer, DWORD* hotTexts, int& hotTextsCount)
    {
        UNREFERENCED_PARAMETER(panel);
        UNREFERENCED_PARAMETER(isDir); UNREFERENCED_PARAMETER(selectedFiles);
        UNREFERENCED_PARAMETER(selectedDirs); UNREFERENCED_PARAMETER(displaySize);
        UNREFERENCED_PARAMETER(selectedSize); UNREFERENCED_PARAMETER(hotTexts);
        hotTextsCount = 0;
        if (buffer == NULL)
            return FALSE;
        buffer[0] = '\0';
        if (file == NULL || file->PluginData == 0 ||
            file->PluginData == static_cast<DWORD_PTR>(-1))
            return FALSE;
        const SalamatrixFileSystemItemData* data =
            reinterpret_cast<const SalamatrixFileSystemItemData*>(file->PluginData);
        size_t used = 0;
        for (size_t index = 0;
             index < Columns.size() && index < data->Item.ColumnValues.size();
             ++index)
        {
            const std::string value = Columns[index].DateTime
                ? SalamatrixFormatFileSystemDateTime(
                      data->Item.ColumnValues[index])
                : SalamatrixFormatFileSystemColumnValue(
                      data->Item.ColumnValues[index], Columns[index].Size);
            if (value.empty())
                continue;
            const char* separator = used == 0 ? "" : "\n";
            const int written = _snprintf_s(
                buffer + used, 1000 - used, _TRUNCATE, "%s%s: %s",
                separator, Columns[index].Name.c_str(), value.c_str());
            if (written < 0)
            {
                buffer[999] = '\0';
                break;
            }
            used += static_cast<size_t>(written);
            if (used >= 999)
                break;
        }
        return buffer[0] != '\0';
    }
    virtual BOOL WINAPI CanBeCopiedToClipboard() { return FALSE; }
    virtual BOOL WINAPI GetByteSize(const CFileData* file, BOOL isDir, CQuadWord* size)
    { UNREFERENCED_PARAMETER(file); UNREFERENCED_PARAMETER(isDir); if (size) size->SetUI64(0); return FALSE; }
    virtual BOOL WINAPI GetLastWriteDate(const CFileData* file, BOOL isDir, SYSTEMTIME* date)
    { UNREFERENCED_PARAMETER(file); UNREFERENCED_PARAMETER(isDir); UNREFERENCED_PARAMETER(date); return FALSE; }
    virtual BOOL WINAPI GetLastWriteTime(const CFileData* file, BOOL isDir, SYSTEMTIME* time)
    { UNREFERENCED_PARAMETER(file); UNREFERENCED_PARAMETER(isDir); UNREFERENCED_PARAMETER(time); return FALSE; }
};

class PackageManager::OpenFileSystem : public CPluginFSInterfaceAbstract
{
private:
    enum RefreshThreadState
    {
        RefreshThreadIdle = 0,
        RefreshThreadStarting = 1,
        RefreshThreadRunningState = 2,
        RefreshThreadStopping = 3
    };

    PackageManager* Owner;
    std::string Path;
    volatile LONG RefreshPosted;
    volatile LONG RefreshRequested;
    volatile LONG RefreshThreadRunning;
    volatile LONG ShuttingDown;
    volatile LONG PathGeneration;
    volatile LONG RefreshIntervalMs;
    volatile LONG RefreshDepth;
    std::vector<std::string> PeriodicRefreshPaths;
    HANDLE RefreshThread;
    CRITICAL_SECTION CacheLock;
    std::vector<FileSystemItem> CachedItems;
    BOOL CacheReady;
    std::string RefreshPackageId;
    std::string RefreshFileSystemId;
    std::string RefreshPath;
    LONG RefreshGeneration;

    BOOL ShouldRefreshPeriodically()
    {
        if (InterlockedCompareExchange(&RefreshIntervalMs, 0, 0) <= 0)
            return FALSE;
        const size_t separator = Path.find('!');
        if (separator == std::string::npos)
            return FALSE;
        unsigned int depth = 0;
        for (size_t i = separator + 1; i < Path.size(); ++i)
            if (Path[i] == '\\' && i + 1 < Path.size())
                ++depth;
        if (depth < static_cast<unsigned int>(
                InterlockedCompareExchange(&RefreshDepth, 0, 0)))
            return FALSE;
        if (PeriodicRefreshPaths.empty())
            return TRUE;
        const size_t relativeSeparator = Path.find('\\', separator + 1);
        if (relativeSeparator == std::string::npos ||
            relativeSeparator + 1 >= Path.size())
            return FALSE;
        const char* relativePath = Path.c_str() + relativeSeparator + 1;
        for (size_t index = 0; index < PeriodicRefreshPaths.size(); ++index)
            if (_stricmp(relativePath, PeriodicRefreshPaths[index].c_str()) == 0)
                return TRUE;
        return FALSE;
    }

    void PostPanelRefresh()
    {
        if (InterlockedCompareExchange(&RefreshPosted, 1, 0) == 0)
            SalamanderGeneral->PostRefreshPanelFS(this);
    }

    void ClearCacheForPathChange()
    {
        EnterCriticalSection(&CacheLock);
        CachedItems.clear();
        CacheReady = FALSE;
        LeaveCriticalSection(&CacheLock);
        InterlockedIncrement(&PathGeneration);
        InterlockedExchange(&RefreshRequested, 1);
    }

    static DWORD WINAPI RefreshThreadProc(void* context)
    {
        OpenFileSystem* fileSystem = static_cast<OpenFileSystem*>(context);
        if (fileSystem != NULL)
            fileSystem->RefreshInBackground();
        return 0;
    }

    void RefreshInBackground()
    {
        std::vector<FileSystemItem> items;
        unsigned int interval = static_cast<unsigned int>(
            InterlockedCompareExchange(&RefreshIntervalMs, 0, 0));
        const BOOL succeeded = Owner->ListFileSystem(
            RefreshPackageId, RefreshFileSystemId,
            Invocation("list", RefreshPackageId, RefreshFileSystemId, NULL,
                       NULL, RefreshPath.c_str()).c_str(),
            &items, &interval);
        const BOOL currentPath =
            RefreshGeneration == InterlockedCompareExchange(&PathGeneration, 0, 0);
        EnterCriticalSection(&CacheLock);
        if (currentPath &&
            InterlockedCompareExchange(&ShuttingDown, 0, 0) == 0)
        {
            CachedItems.clear();
            if (succeeded)
                CachedItems.swap(items);
            CacheReady = TRUE;
            if (succeeded)
                InterlockedExchange(&RefreshIntervalMs, static_cast<LONG>(interval));
        }
        LeaveCriticalSection(&CacheLock);
        InterlockedCompareExchange(
            &RefreshThreadRunning,
            RefreshThreadIdle, RefreshThreadRunningState);
        if (InterlockedCompareExchange(&ShuttingDown, 0, 0) == 0)
            PostPanelRefresh();
    }

    BOOL StartBackgroundRefresh(
        const std::string& packageId, const std::string& fileSystemId)
    {
        if (InterlockedCompareExchange(&ShuttingDown, 0, 0) != 0 ||
            InterlockedCompareExchange(
                &RefreshThreadRunning,
                RefreshThreadStarting, RefreshThreadIdle) != RefreshThreadIdle)
            return FALSE;

        // A previous worker changes the state immediately before returning.
        // Join that short epilogue before replacing its published handle.
        HANDLE refreshThread = reinterpret_cast<HANDLE>(
            InterlockedExchangePointer(
                reinterpret_cast<PVOID volatile*>(&RefreshThread), NULL));
        if (refreshThread != NULL)
        {
            if (!Runtime::WaitForThreadWithSentMessageDispatch(
                    refreshThread,
                    SalamanderGeneral != NULL
                        ? SalamanderGeneral->GetMainWindowHWND() : NULL))
            {
                InterlockedExchangePointer(
                    reinterpret_cast<PVOID volatile*>(&RefreshThread),
                    refreshThread);
                InterlockedExchange(
                    &RefreshThreadRunning, RefreshThreadIdle);
                return FALSE;
            }
            CloseHandle(refreshThread);
        }

        // CloseFS can run concurrently with a detached panel refresh.  The
        // Starting state keeps the object alive until this second check has
        // either rejected the start or published a joinable worker handle.
        if (InterlockedCompareExchange(&ShuttingDown, 0, 0) != 0)
        {
            InterlockedExchange(&RefreshThreadRunning, RefreshThreadIdle);
            return FALSE;
        }
        RefreshPackageId = packageId;
        RefreshFileSystemId = fileSystemId;
        RefreshPath = Path;
        RefreshGeneration = InterlockedCompareExchange(&PathGeneration, 0, 0);
        refreshThread = CreateThread(
            NULL, 0, RefreshThreadProc, this, CREATE_SUSPENDED, NULL);
        if (refreshThread == NULL)
        {
            InterlockedExchange(&RefreshThreadRunning, RefreshThreadIdle);
            return FALSE;
        }
        // Publish the handle before the worker can touch the object, then
        // expose Running.  CloseFS waits out Starting and can only observe
        // Running after there is a handle it can join.
        InterlockedExchangePointer(
            reinterpret_cast<PVOID volatile*>(&RefreshThread), refreshThread);
        InterlockedExchange(
            &RefreshThreadRunning, RefreshThreadRunningState);
        if (ResumeThread(refreshThread) == static_cast<DWORD>(-1))
        {
            if (InterlockedCompareExchangePointer(
                    reinterpret_cast<PVOID volatile*>(&RefreshThread),
                    NULL, refreshThread) == refreshThread)
                CloseHandle(refreshThread);
            InterlockedCompareExchange(
                &RefreshThreadRunning,
                RefreshThreadIdle, RefreshThreadRunningState);
            return FALSE;
        }
        return TRUE;
    }

    BOOL SplitProvider(std::string* packageId, std::string* fileSystemId) const
    {
        const size_t separator = Path.find('!');
        if (separator == std::string::npos || separator == 0 ||
            separator + 1 >= Path.size())
            return FALSE;
        *packageId = Path.substr(0, separator);
        const size_t pathSeparator = Path.find_first_of("\\/", separator + 1);
        *fileSystemId = Path.substr(
            separator + 1,
            pathSeparator == std::string::npos
                ? std::string::npos : pathSeparator - separator - 1);
        return TRUE;
    }

    static BOOL NormalizeUserPart(const char* userPart, std::string* normalized)
    {
        normalized->clear();
        const std::string input = userPart != NULL ? userPart : "";
        std::vector<std::string> components;
        size_t start = 0;
        while (start <= input.size())
        {
            const size_t end = input.find_first_of("\\/", start);
            const std::string component = input.substr(
                start, end == std::string::npos ? std::string::npos : end - start);
            if (!component.empty() && component != ".")
            {
                if (component == "..")
                {
                    if (!components.empty())
                        components.pop_back();
                }
                else
                    components.push_back(component);
            }
            if (end == std::string::npos)
                break;
            start = end + 1;
        }
        for (size_t index = 0; index < components.size(); ++index)
        {
            if (index != 0)
                *normalized += "\\";
            *normalized += components[index];
        }
        return TRUE;
    }

    static std::string Invocation(
        const char* operation, const std::string& packageId,
        const std::string& fileSystemId,
        const SalamatrixFileSystemItemData* data,
        const char* actionId = NULL,
        const char* path = NULL,
        HWND parentWindow = NULL)
    {
        std::string result = std::string("{\"role\":\"fileSystem\",\"operation\":\"") +
            JsonEscape(operation) + "\",\"packageId\":\"" + JsonEscape(packageId.c_str()) +
            "\",\"fileSystemId\":\"" + JsonEscape(fileSystemId.c_str()) + "\"";
        if (actionId != NULL)
            result += std::string(",\"actionId\":\"") + JsonEscape(actionId) + "\"";
        if (path != NULL)
            result += std::string(",\"path\":\"") + JsonEscape(path) + "\"";
        if (parentWindow != NULL)
            result += std::string(",\"parentWindow\":\"") +
                std::to_string(static_cast<unsigned long long>(
                    reinterpret_cast<ULONG_PTR>(parentWindow))) + "\"";
        if (data != NULL)
            result += std::string(",\"item\":{\"id\":\"") + JsonEscape(data->Item.Id.c_str()) +
                "\",\"name\":\"" + JsonEscape(data->Item.Name.c_str()) +
                "\",\"directory\":" + (data->Item.Directory ? "true" : "false") + "}";
        result += "}";
        return result;
    }

    static void AppendPanelNavigation(
        std::string* invocation, int panel,
        const SalamatrixFileSystemItemData* selectedData)
    {
        if (invocation == NULL || selectedData == NULL ||
            SalamanderGeneral == NULL || invocation->empty() ||
            invocation->back() != '}')
            return;
        std::vector<std::string> itemIds;
        int selectedIndex = -1;
        int enumeration = 0;
        BOOL isDir = FALSE;
        const CFileData* item = NULL;
        while ((item = SalamanderGeneral->GetPanelItem(
                    panel, &enumeration, &isDir)) != NULL)
        {
            if (isDir || item->PluginData == 0 ||
                item->PluginData == static_cast<DWORD_PTR>(-1))
                continue;
            SalamatrixFileSystemItemData* data =
                reinterpret_cast<SalamatrixFileSystemItemData*>(item->PluginData);
            if (data->PackageId != selectedData->PackageId ||
                data->FileSystemId != selectedData->FileSystemId)
                continue;
            if (data->Item.Id == selectedData->Item.Id)
                selectedIndex = static_cast<int>(itemIds.size());
            itemIds.push_back(data->Item.Id);
        }
        if (selectedIndex < 0 || itemIds.empty())
            return;
        // Invocation JSON is passed to one-shot workers on their command line.
        // A complete Event Viewer panel can contain hundreds of long base64
        // identities and exceed CreateProcessW's command-line limit. Keep a
        // useful ordered window around the selected item instead.
        const size_t maxNavigationItems = 64;
        size_t firstItem = 0;
        if (itemIds.size() > maxNavigationItems)
        {
            const size_t halfWindow = maxNavigationItems / 2;
            firstItem = static_cast<size_t>(selectedIndex) > halfWindow
                ? static_cast<size_t>(selectedIndex) - halfWindow : 0;
            if (firstItem + maxNavigationItems > itemIds.size())
                firstItem = itemIds.size() - maxNavigationItems;
        }
        const size_t lastItem = (std::min)(
            itemIds.size(), firstItem + maxNavigationItems);
        selectedIndex -= static_cast<int>(firstItem);
        invocation->pop_back();
        *invocation += ",\"panelItemIndex\":" +
            std::to_string(selectedIndex) + ",\"panelItemIds\":[";
        for (size_t index = firstItem; index < lastItem; ++index)
        {
            if (index != firstItem)
                *invocation += ",";
            *invocation += "\"" + JsonEscape(itemIds[index].c_str()) + "\"";
        }
        *invocation += "]}";
    }

    BOOL AddItem(
        CSalamanderDirectoryAbstract* dir,
        CPluginDataInterfaceAbstract* pluginData,
        const FileSystemItem& item,
        const std::string& packageId,
        const std::string& fileSystemId)
    {
        CFileData file;
        memset(&file, 0, sizeof(file));
        file.Name = SalamanderGeneral->DupStr(item.Name.c_str());
        if (file.Name == NULL)
            return FALSE;
        file.NameLen = static_cast<int>(strlen(file.Name));
        char* extension = strrchr(file.Name, '.');
        file.Ext = extension != NULL ? extension + 1 : file.Name + file.NameLen;
        file.Attr = item.Directory ? FILE_ATTRIBUTE_DIRECTORY : 0;
#ifdef new
#undef new
#define RESTORE_SALAMATRIX_FS_ITEM_DEBUG_NEW_MACRO
#endif
        SalamatrixFileSystemItemData* data =
            new (std::nothrow) SalamatrixFileSystemItemData();
#ifdef RESTORE_SALAMATRIX_FS_ITEM_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_SALAMATRIX_FS_ITEM_DEBUG_NEW_MACRO
#endif
        if (data == NULL)
        {
            SalamanderGeneral->Free(file.Name);
            return FALSE;
        }
        data->Item = item;
        data->PackageId = packageId;
        data->FileSystemId = fileSystemId;
        file.PluginData = reinterpret_cast<DWORD_PTR>(data);
        const BOOL added = item.Directory
            ? dir->AddDir(NULL, file, pluginData)
            : dir->AddFile(NULL, file, pluginData);
        if (!added)
        {
            delete data;
            SalamanderGeneral->Free(file.Name);
        }
        return added;
    }

public:
    explicit OpenFileSystem(PackageManager* owner)
        : Owner(owner), RefreshPosted(0), RefreshRequested(1),
          RefreshThreadRunning(0), ShuttingDown(0), PathGeneration(0),
          RefreshIntervalMs(3000), RefreshDepth(0), RefreshThread(NULL), CacheReady(FALSE),
          RefreshGeneration(0)
    {
        InitializeCriticalSection(&CacheLock);
    }

    virtual ~OpenFileSystem()
    {
        InterlockedExchange(&ShuttingDown, 1);
        for (;;)
        {
            const LONG state = InterlockedCompareExchange(
                &RefreshThreadRunning, RefreshThreadIdle, RefreshThreadIdle);
            if (state == RefreshThreadStarting)
            {
                SwitchToThread();
                continue;
            }
            if (state == RefreshThreadIdle &&
                InterlockedCompareExchange(
                    &RefreshThreadRunning,
                    RefreshThreadStopping, RefreshThreadIdle) != RefreshThreadIdle)
                continue;
            if (state == RefreshThreadRunningState &&
                InterlockedCompareExchange(
                    &RefreshThreadRunning,
                    RefreshThreadStopping,
                    RefreshThreadRunningState) != RefreshThreadRunningState)
                continue;
            break;
        }
        HANDLE refreshThread = reinterpret_cast<HANDLE>(
            InterlockedExchangePointer(
                reinterpret_cast<PVOID volatile*>(&RefreshThread), NULL));
        if (refreshThread != NULL)
        {
            // During application shutdown the panel is closed before the
            // Salamatrix plug-in reaches Release().  A listing worker can be
            // blocked in a persistent runtime call at that point.  Requesting
            // the session stop here makes that call return; otherwise every
            // open extension-FS panel can delay the later SaveConfig() by the
            // full runtime timeout.
            Owner->CancelFileSystemListingForShutdown(RefreshPackageId);
            Runtime::WaitForThreadWithSentMessageDispatch(
                refreshThread,
                SalamanderGeneral != NULL
                    ? SalamanderGeneral->GetMainWindowHWND() : NULL);
            CloseHandle(refreshThread);
        }
        DeleteCriticalSection(&CacheLock);
    }

    void RequestDataRefresh()
    {
        InterlockedExchange(&RefreshRequested, 1);
        PostPanelRefresh();
    }

    BOOL IsRoot() const { return Path.empty() ? TRUE : FALSE; }
    const std::string& GetPath() const { return Path; }
    std::string GetParentPath() const
    {
        const size_t separator = Path.find_last_of("\\/");
        return separator == std::string::npos
            ? std::string() : Path.substr(0, separator);
    }

    BOOL ExecuteDefault(const CFileData& file, int panel)
    {
        SalamatrixFileSystemItemData* data =
            reinterpret_cast<SalamatrixFileSystemItemData*>(file.PluginData);
        if (data == NULL || !data->Item.Enabled)
            return FALSE;
        for (size_t p = 0; p < Owner->Packages.size(); ++p)
        {
            Package* package = Owner->Packages[p];
            if (_stricmp(package->Id.c_str(), data->PackageId.c_str()) != 0)
                continue;
            for (size_t f = 0; f < package->Manifest.FileSystems.size(); ++f)
            {
                const CExtensionManifestFileSystem& fs = package->Manifest.FileSystems[f];
                if (_stricmp(fs.Id.c_str(), data->FileSystemId.c_str()) != 0)
                    continue;
                const CExtensionManifestFileSystem::Action* selected = NULL;
                for (size_t a = 0; a < fs.Actions.size(); ++a)
                    if (!fs.Actions[a].Separator && fs.Actions[a].Default &&
                        (fs.Actions[a].ItemIdPrefix.empty() ||
                         data->Item.Id.compare(0, fs.Actions[a].ItemIdPrefix.size(),
                                               fs.Actions[a].ItemIdPrefix) == 0))
                    { selected = &fs.Actions[a]; break; }
                const std::string actionId = selected != NULL ? selected->Id : "open";
                std::string invocation = Invocation(
                    "action", data->PackageId, data->FileSystemId,
                    data, actionId.c_str(), NULL,
                    SalamanderGeneral != NULL
                        ? SalamanderGeneral->GetMainWindowHWND() : NULL);
                AppendPanelNavigation(&invocation, panel, data);
                const BOOL executed = Owner->ExecuteFileSystemAction(
                    data->PackageId, data->FileSystemId,
                    actionId, invocation.c_str());
                if (executed && selected != NULL && selected->Refresh)
                    RequestDataRefresh();
                return executed;
            }
        }
        return FALSE;
    }

    virtual BOOL WINAPI GetCurrentPath(char* userPart)
    { StringCchCopyA(userPart, MAX_PATH, Path.c_str()); return TRUE; }
    virtual BOOL WINAPI GetFullName(CFileData& file, int isDir, char* buf, int bufSize)
    {
        UNREFERENCED_PARAMETER(isDir);
        const std::string full = Path.empty() ? file.Name : Path + "\\" + file.Name;
        return SUCCEEDED(StringCchCopyA(buf, bufSize, full.c_str()));
    }
    virtual BOOL WINAPI GetFullFSPath(HWND parent, const char* fsName, char* path, int pathSize, BOOL& success)
    {
        UNREFERENCED_PARAMETER(parent);
        const std::string full = std::string(fsName) + ":" + Path;
        success = SUCCEEDED(StringCchCopyA(path, pathSize, full.c_str()));
        return TRUE;
    }
    virtual BOOL WINAPI GetRootPath(char* userPart) { userPart[0] = '\0'; return TRUE; }
    virtual BOOL WINAPI IsCurrentPath(int currentFSNameIndex, int fsNameIndex, const char* userPart)
    { UNREFERENCED_PARAMETER(currentFSNameIndex); UNREFERENCED_PARAMETER(fsNameIndex); return _stricmp(Path.c_str(), userPart ? userPart : "") == 0; }
    virtual BOOL WINAPI IsOurPath(int currentFSNameIndex, int fsNameIndex, const char* userPart)
    { UNREFERENCED_PARAMETER(currentFSNameIndex); UNREFERENCED_PARAMETER(fsNameIndex); UNREFERENCED_PARAMETER(userPart); return TRUE; }
    virtual BOOL WINAPI ChangePath(int currentFSNameIndex, char* fsName, int fsNameIndex, const char* userPart, char* cutFileName, BOOL* pathWasCut, BOOL forceRefresh, int mode)
    {
        UNREFERENCED_PARAMETER(currentFSNameIndex); UNREFERENCED_PARAMETER(fsName);
        UNREFERENCED_PARAMETER(fsNameIndex); UNREFERENCED_PARAMETER(forceRefresh);
        UNREFERENCED_PARAMETER(mode);
        if (cutFileName != NULL) cutFileName[0] = '\0';
        if (pathWasCut != NULL) *pathWasCut = FALSE;
        std::string requested;
        NormalizeUserPart(userPart, &requested);
        if (requested.empty())
        {
            if (!Path.empty())
                ClearCacheForPathChange();
            Path.clear();
            return TRUE;
        }
        for (size_t p = 0; p < Owner->Packages.size(); ++p)
            for (size_t f = 0; f < Owner->Packages[p]->Manifest.FileSystems.size(); ++f)
            {
                const std::string provider = Owner->Packages[p]->Id + "!" +
                    Owner->Packages[p]->Manifest.FileSystems[f].Id;
                if (_stricmp(requested.c_str(), provider.c_str()) == 0 ||
                    (requested.size() > provider.size() &&
                     requested[provider.size()] == '\\' &&
                     _strnicmp(requested.c_str(), provider.c_str(), provider.size()) == 0))
                {
                    if (_stricmp(Path.c_str(), requested.c_str()) != 0)
                        ClearCacheForPathChange();
                    Path = requested;
                    return TRUE;
                }
            }
        return FALSE;
    }
    virtual BOOL WINAPI ListCurrentPath(CSalamanderDirectoryAbstract* dir, CPluginDataInterfaceAbstract*& pluginData, int& iconsType, BOOL forceRefresh)
    {
        if (forceRefresh)
            InterlockedExchange(&RefreshRequested, 1);
        InterlockedExchange(&RefreshPosted, 0);
        std::string packageId, fileSystemId;
        std::vector<CExtensionManifestFileSystem::Column> columns;
        std::vector<CExtensionManifestFileSystem::RootItem> rootItems;
        std::string defaultFileIcon;
        std::wstring packageDirectory;
        if (!Path.empty())
        {
            if (!SplitProvider(&packageId, &fileSystemId))
                return FALSE;
            for (size_t p = 0; p < Owner->Packages.size(); ++p)
                if (_stricmp(Owner->Packages[p]->Id.c_str(), packageId.c_str()) == 0)
                    for (size_t f = 0; f < Owner->Packages[p]->Manifest.FileSystems.size(); ++f)
                        if (_stricmp(Owner->Packages[p]->Manifest.FileSystems[f].Id.c_str(), fileSystemId.c_str()) == 0)
                        {
                            const CExtensionManifestFileSystem& fileSystem =
                                Owner->Packages[p]->Manifest.FileSystems[f];
                            columns = fileSystem.Columns;
                            rootItems = fileSystem.RootItems;
                            packageDirectory = Owner->Packages[p]->Directory;
                            InterlockedExchange(&RefreshDepth,
                                static_cast<LONG>(fileSystem.RefreshDepth));
                            PeriodicRefreshPaths = fileSystem.RefreshPaths;
                            std::wstring relative;
                            if (!fileSystem.DefaultFileIcon.empty() &&
                                PackageManager::ToWide(fileSystem.DefaultFileIcon, &relative))
                            {
                                PackageManager::ToUtf8(
                                    Owner->Packages[p]->Directory + L"\\" + relative,
                                    &defaultFileIcon);
                            }
                        }
        }
#ifdef new
#undef new
#define RESTORE_SALAMATRIX_FS_PLUGIN_DATA_DEBUG_NEW_MACRO
#endif
        pluginData = new (std::nothrow) SalamatrixFileSystemPluginData(
            columns, defaultFileIcon);
#ifdef RESTORE_SALAMATRIX_FS_PLUGIN_DATA_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_SALAMATRIX_FS_PLUGIN_DATA_DEBUG_NEW_MACRO
#endif
        if (pluginData == NULL) return FALSE;
        iconsType = pitFromPlugin;
        dir->SetValidData(VALID_DATA_NONE);
        if (!Path.empty())
        {
            CFileData up;
            memset(&up, 0, sizeof(up));
            up.Name = SalamanderGeneral->DupStr("..");
            if (up.Name == NULL)
                return FALSE;
            up.NameLen = 2;
            up.Ext = up.Name + up.NameLen;
            up.Attr = FILE_ATTRIBUTE_DIRECTORY;
            if (!dir->AddDir(NULL, up, NULL))
            {
                SalamanderGeneral->Free(up.Name);
                return FALSE;
            }
        }
        if (Path.empty())
        {
            for (size_t p = 0; p < Owner->Packages.size(); ++p)
            {
                Package* package = Owner->Packages[p];
                if (package == NULL || !package->RuntimeUsable) continue;
                for (size_t f = 0; f < package->Manifest.FileSystems.size(); ++f)
                {
                    const CExtensionManifestFileSystem& fs = package->Manifest.FileSystems[f];
                    FileSystemItem item;
                    item.Id = package->Id + "!" + fs.Id;
                    item.Name = fs.Name;
                    item.Directory = true;
                    std::wstring relative;
                    if (!fs.Icon.empty() && PackageManager::ToWide(fs.Icon, &relative))
                        PackageManager::ToUtf8(package->Directory + L"\\" + relative, &item.Icon);
                    if (!fs.IconDark.empty() && PackageManager::ToWide(fs.IconDark, &relative))
                        PackageManager::ToUtf8(package->Directory + L"\\" + relative, &item.IconDark);
                    if (!AddItem(dir, pluginData, item, package->Id, fs.Id)) return FALSE;
                }
            }
            return TRUE;
        }
        const std::string provider = packageId + "!" + fileSystemId;
        if (_stricmp(Path.c_str(), provider.c_str()) == 0 && !rootItems.empty())
        {
            for (size_t index = 0; index < rootItems.size(); ++index)
            {
                FileSystemItem item;
                item.Id = rootItems[index].Id;
                item.Name = rootItems[index].Name;
                item.Directory = true;
                std::wstring relative;
                if (!rootItems[index].Icon.empty() &&
                    PackageManager::ToWide(rootItems[index].Icon, &relative))
                    PackageManager::ToUtf8(packageDirectory + L"\\" + relative, &item.Icon);
                if (!rootItems[index].IconDark.empty() &&
                    PackageManager::ToWide(rootItems[index].IconDark, &relative))
                    PackageManager::ToUtf8(packageDirectory + L"\\" + relative, &item.IconDark);
                if (!AddItem(dir, pluginData, item, packageId, fileSystemId))
                    return FALSE;
            }
            return TRUE;
        }
        std::vector<FileSystemItem> items;
        BOOL cacheReady = FALSE;
        EnterCriticalSection(&CacheLock);
        cacheReady = CacheReady;
        if (cacheReady)
            items = CachedItems;
        LeaveCriticalSection(&CacheLock);
        const BOOL requested = InterlockedCompareExchange(&RefreshRequested, 0, 0) != 0;
        if ((!cacheReady || requested) &&
            StartBackgroundRefresh(packageId, fileSystemId))
            InterlockedExchange(&RefreshRequested, 0);
        for (size_t index = 0; index < items.size(); ++index)
            if (!AddItem(dir, pluginData, items[index], packageId, fileSystemId)) return FALSE;
        return TRUE;
    }
    virtual BOOL WINAPI TryCloseOrDetach(BOOL forceClose, BOOL canDetach, BOOL& detach, int reason)
    { UNREFERENCED_PARAMETER(forceClose); UNREFERENCED_PARAMETER(canDetach); UNREFERENCED_PARAMETER(reason); detach = FALSE; return TRUE; }
    virtual void WINAPI Event(int event, DWORD param)
    {
        if (event == FSE_PATHCHANGED && !Path.empty())
        {
            EnterCriticalSection(&CacheLock);
            const BOOL cacheReady = CacheReady;
            LeaveCriticalSection(&CacheLock);
            if (!cacheReady &&
                InterlockedCompareExchange(&RefreshThreadRunning, 0, 0) != 0)
                SalamanderGeneral->StartThrobber(static_cast<int>(param), NULL, 0);
        }
        if ((event == FSE_ACTIVATEREFRESH || event == FSE_TIMER) &&
            ShouldRefreshPeriodically())
            RequestDataRefresh();
        const LONG refreshInterval =
            InterlockedCompareExchange(&RefreshIntervalMs, 0, 0);
        if ((event == FSE_OPENED || event == FSE_ATTACHED || event == FSE_TIMER) &&
            refreshInterval > 0)
            SalamanderGeneral->AddPluginFSTimer(
                static_cast<DWORD>(refreshInterval), this, 1);
    }
    virtual void WINAPI ReleaseObject(HWND parent) { UNREFERENCED_PARAMETER(parent); }
    virtual DWORD WINAPI GetSupportedServices()
    {
        return FS_SERVICE_CONTEXTMENU | FS_SERVICE_GETFSICON |
               FS_SERVICE_VIEWFILE |
               FS_SERVICE_GETNEXTDIRLINEHOTPATH |
               FS_SERVICE_GETPATHFORMAINWNDTITLE |
               FS_SERVICE_NO_REFRESH_WAIT_CURSOR;
    }
    virtual BOOL WINAPI GetChangeDriveOrDisconnectItem(const char* fsName, char*& title, HICON& icon, BOOL& destroyIcon)
    {
        std::string text = std::string("\t") + fsName + ":" + Path;
        title = SalamanderGeneral->DupStr(text.c_str());
        icon = GetFSIcon(destroyIcon); return title != NULL;
    }
    virtual HICON WINAPI GetFSIcon(BOOL& destroyIcon)
    { destroyIcon = TRUE; return reinterpret_cast<HICON>(LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_PLUGINICON), IMAGE_ICON, 16, 16, SalamanderGeneral->GetIconLRFlags())); }
    virtual void WINAPI GetDropEffect(const char* srcFSPath, const char* tgtFSPath, DWORD allowedEffects, DWORD keyState, DWORD* dropEffect)
    { UNREFERENCED_PARAMETER(srcFSPath); UNREFERENCED_PARAMETER(tgtFSPath); UNREFERENCED_PARAMETER(allowedEffects); UNREFERENCED_PARAMETER(keyState); *dropEffect = DROPEFFECT_NONE; }
    virtual void WINAPI GetFSFreeSpace(CQuadWord* retValue) { retValue->SetUI64(0); }
    virtual BOOL WINAPI GetNextDirectoryLineHotPath(const char* text, int pathLen, int& offset)
    {
        if (text == NULL || offset < 0 || pathLen <= 0)
            return FALSE;
        const char* end = text + pathLen;
        const char* root = text;
        while (root < end && *root != ':')
            ++root;
        if (root < end && *root == ':')
            ++root;

        const char* current = text + offset;
        if (current >= end)
            return FALSE;
        if (current < root)
            current = root;
        else
        {
            while (current < end && (*current == '\\' || *current == '/'))
                ++current;
            while (current < end && *current != '\\' && *current != '/')
                ++current;
        }
        offset = static_cast<int>(current - text);
        return current < end;
    }
    virtual void WINAPI CompleteDirectoryLineHotPath(char* path, int pathBufSize)
    { UNREFERENCED_PARAMETER(path); UNREFERENCED_PARAMETER(pathBufSize); }
    virtual BOOL WINAPI GetPathForMainWindowTitle(const char* fsName, int mode, char* buf, int bufSize)
    {
        std::string title;
        if (mode == 1)
        {
            if (Path.empty())
                title = fsName;
            else
            {
                const size_t separator = Path.find_last_of("\\/");
                title = separator == std::string::npos
                    ? Path : Path.substr(separator + 1);
            }
        }
        else if (mode == 2)
        {
            const size_t first = Path.find_first_of("\\/");
            const size_t last = Path.find_last_of("\\/");
            if (first != std::string::npos && last != std::string::npos && first < last)
                title = std::string(fsName) + ":" + Path.substr(0, first + 1) +
                        "...\\" + Path.substr(last + 1);
            else
                title = std::string(fsName) + ":" + Path;
        }
        else
            title = std::string(fsName) + ":" + Path;
        return SUCCEEDED(StringCchCopyA(buf, bufSize, title.c_str()));
    }
    virtual void WINAPI ShowInfoDialog(const char* fsName, HWND parent) { UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(parent); }
    virtual BOOL WINAPI ExecuteCommandLine(HWND parent, char* command, int& selFrom, int& selTo)
    { UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(command); UNREFERENCED_PARAMETER(selFrom); UNREFERENCED_PARAMETER(selTo); return FALSE; }
    virtual BOOL WINAPI QuickRename(const char* fsName, int mode, HWND parent, CFileData& file, BOOL isDir, char* newName, BOOL& cancel)
    { UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(mode); UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(file); UNREFERENCED_PARAMETER(isDir); UNREFERENCED_PARAMETER(newName); cancel = FALSE; return FALSE; }
    virtual void WINAPI AcceptChangeOnPathNotification(const char* fsName, const char* path, BOOL includingSubdirs)
    { UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(path); UNREFERENCED_PARAMETER(includingSubdirs); RequestDataRefresh(); }
    virtual BOOL WINAPI CreateDir(const char* fsName, int mode, HWND parent, char* newName, BOOL& cancel)
    { UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(mode); UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(newName); cancel = FALSE; return FALSE; }
    virtual void WINAPI ViewFile(const char* fsName, HWND parent, CSalamanderForViewFileOnFSAbstract* salamander, CFileData& file)
    { UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(salamander); ExecuteDefault(file, SalamanderGeneral->GetSourcePanel()); }
    virtual BOOL WINAPI Delete(const char* fsName, int mode, HWND parent, int panel, int selectedFiles, int selectedDirs, BOOL& cancelOrError)
    { UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(mode); UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(panel); UNREFERENCED_PARAMETER(selectedFiles); UNREFERENCED_PARAMETER(selectedDirs); cancelOrError = FALSE; return FALSE; }
    virtual BOOL WINAPI CopyOrMoveFromFS(BOOL copy, int mode, const char* fsName, HWND parent, int panel, int selectedFiles, int selectedDirs, char* targetPath, BOOL& operationMask, BOOL& cancelOrHandlePath, HWND dropTarget)
    { UNREFERENCED_PARAMETER(copy); UNREFERENCED_PARAMETER(mode); UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(panel); UNREFERENCED_PARAMETER(selectedFiles); UNREFERENCED_PARAMETER(selectedDirs); UNREFERENCED_PARAMETER(targetPath); UNREFERENCED_PARAMETER(operationMask); UNREFERENCED_PARAMETER(dropTarget); cancelOrHandlePath = FALSE; return FALSE; }
    virtual BOOL WINAPI CopyOrMoveFromDiskToFS(BOOL copy, int mode, const char* fsName, HWND parent, const char* sourcePath, SalEnumSelection2 next, void* nextParam, int sourceFiles, int sourceDirs, char* targetPath, BOOL* invalidPathOrCancel)
    { UNREFERENCED_PARAMETER(copy); UNREFERENCED_PARAMETER(mode); UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(sourcePath); UNREFERENCED_PARAMETER(next); UNREFERENCED_PARAMETER(nextParam); UNREFERENCED_PARAMETER(sourceFiles); UNREFERENCED_PARAMETER(sourceDirs); UNREFERENCED_PARAMETER(targetPath); if (invalidPathOrCancel) *invalidPathOrCancel = FALSE; return FALSE; }
    virtual BOOL WINAPI ChangeAttributes(const char* fsName, HWND parent, int panel, int selectedFiles, int selectedDirs)
    { UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(panel); UNREFERENCED_PARAMETER(selectedFiles); UNREFERENCED_PARAMETER(selectedDirs); return FALSE; }
    virtual void WINAPI ShowProperties(const char* fsName, HWND parent, int panel, int selectedFiles, int selectedDirs)
    { UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(panel); UNREFERENCED_PARAMETER(selectedFiles); UNREFERENCED_PARAMETER(selectedDirs); }
    virtual void WINAPI ContextMenu(const char* fsName, HWND parent, int menuX, int menuY, int type, int panel, int selectedFiles, int selectedDirs)
    {
        UNREFERENCED_PARAMETER(fsName);
        if (type != fscmItemsInPanel || Path.empty()) return;
        int isDir = 0; const CFileData* file = NULL;
        if (selectedFiles == 0 && selectedDirs == 0)
            file = SalamanderGeneral->GetPanelFocusedItem(panel, &isDir);
        else { int index = 0; file = SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir); }
        if (file == NULL || file->PluginData == 0 ||
            file->PluginData == static_cast<DWORD_PTR>(-1))
            return;
        SalamatrixFileSystemItemData* data =
            reinterpret_cast<SalamatrixFileSystemItemData*>(file->PluginData);
        if (!data->Item.Enabled) return;
        HMENU menu = CreatePopupMenu(); if (menu == NULL) return;
        const CExtensionManifestFileSystem* manifestFs = NULL;
        for (size_t p = 0; p < Owner->Packages.size() && manifestFs == NULL; ++p)
            if (_stricmp(Owner->Packages[p]->Id.c_str(), data->PackageId.c_str()) == 0)
                for (size_t f = 0; f < Owner->Packages[p]->Manifest.FileSystems.size(); ++f)
                    if (_stricmp(Owner->Packages[p]->Manifest.FileSystems[f].Id.c_str(), data->FileSystemId.c_str()) == 0)
                    { manifestFs = &Owner->Packages[p]->Manifest.FileSystems[f]; break; }
        if (manifestFs == NULL || manifestFs->Actions.empty()) { DestroyMenu(menu); return; }
        for (size_t a = 0; a < manifestFs->Actions.size(); ++a)
        {
            const CExtensionManifestFileSystem::Action& action = manifestFs->Actions[a];
            if (!action.ItemIdPrefix.empty() &&
                data->Item.Id.compare(0, action.ItemIdPrefix.size(),
                                      action.ItemIdPrefix) != 0)
                continue;
            if (action.Separator)
            {
                AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
                continue;
            }
            std::wstring title;
            if (!PackageManager::ToWide(action.Title, &title))
                continue;
            AppendMenuW(menu, MF_STRING, 4000 + static_cast<UINT>(a), title.c_str());
            if (action.Default) SetMenuDefaultItem(menu, 4000 + static_cast<UINT>(a), FALSE);
        }
        const UINT selected = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, menuX, menuY, 0, parent, NULL);
        DestroyMenu(menu);
        if (selected >= 4000 && selected < 4000 + manifestFs->Actions.size())
        {
            const CExtensionManifestFileSystem::Action& action = manifestFs->Actions[selected - 4000];
            if (action.Separator)
                return;
            std::string invocation = Invocation(
                "action", data->PackageId, data->FileSystemId,
                data, action.Id.c_str(), NULL,
                SalamanderGeneral != NULL
                    ? SalamanderGeneral->GetMainWindowHWND() : NULL);
            AppendPanelNavigation(&invocation, panel, data);
            Owner->ExecuteFileSystemAction(data->PackageId, data->FileSystemId, action.Id, invocation.c_str());
            if (action.Refresh)
                RequestDataRefresh();
        }
    }
    virtual BOOL WINAPI HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult)
    { UNREFERENCED_PARAMETER(uMsg); UNREFERENCED_PARAMETER(wParam); UNREFERENCED_PARAMETER(lParam); UNREFERENCED_PARAMETER(plResult); return FALSE; }
    virtual BOOL WINAPI OpenFindDialog(const char* fsName, int panel)
    { UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(panel); return FALSE; }
    virtual void WINAPI OpenActiveFolder(const char* fsName, HWND parent)
    { UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(parent); }
    virtual void WINAPI GetAllowedDropEffects(int mode, const char* tgtFSPath, DWORD* allowedEffects)
    { UNREFERENCED_PARAMETER(mode); UNREFERENCED_PARAMETER(tgtFSPath); *allowedEffects = 0; }
    virtual BOOL WINAPI GetNoItemsInPanelText(char* textBuf, int textBufSize)
    { return SUCCEEDED(StringCchCopyA(textBuf, textBufSize, SalamanderGeneral->LoadStr(DLLInstance, IDS_FS_EMPTY))); }
    virtual void WINAPI ShowSecurityInfo(HWND parent) { UNREFERENCED_PARAMETER(parent); }
    virtual void WINAPI EnsureShareExistsOnServer(int panel, const char* server, const char* share)
    { UNREFERENCED_PARAMETER(panel); UNREFERENCED_PARAMETER(server); UNREFERENCED_PARAMETER(share); }
};

class PackageManager::FileSystemExtension : public CPluginInterfaceForFSAbstract
{
private:
    PackageManager* Owner;
public:
    explicit FileSystemExtension(PackageManager* owner) : Owner(owner) {}
    virtual CPluginFSInterfaceAbstract* WINAPI OpenFS(const char* fsName, int fsNameIndex)
    { UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(fsNameIndex); return new OpenFileSystem(Owner); }
    virtual void WINAPI CloseFS(CPluginFSInterfaceAbstract* fs) { delete fs; }
    virtual void WINAPI ExecuteChangeDriveMenuItem(int panel)
    { int failReason = 0; SalamanderGeneral->ChangePanelPathToPluginFS(panel, SalamatrixFSName, "", &failReason); }
    virtual BOOL WINAPI ChangeDriveMenuItemContextMenu(HWND parent, int panel, int x, int y, CPluginFSInterfaceAbstract* pluginFS, const char* pluginFSName, int pluginFSNameIndex, BOOL isDetachedFS, BOOL& refreshMenu, BOOL& closeMenu, int& postCmd, void*& postCmdParam)
    { UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(panel); UNREFERENCED_PARAMETER(x); UNREFERENCED_PARAMETER(y); UNREFERENCED_PARAMETER(pluginFS); UNREFERENCED_PARAMETER(pluginFSName); UNREFERENCED_PARAMETER(pluginFSNameIndex); UNREFERENCED_PARAMETER(isDetachedFS); UNREFERENCED_PARAMETER(refreshMenu); UNREFERENCED_PARAMETER(closeMenu); UNREFERENCED_PARAMETER(postCmd); UNREFERENCED_PARAMETER(postCmdParam); return FALSE; }
    virtual void WINAPI ExecuteChangeDrivePostCommand(int panel, int postCmd, void* postCmdParam)
    { UNREFERENCED_PARAMETER(panel); UNREFERENCED_PARAMETER(postCmd); UNREFERENCED_PARAMETER(postCmdParam); }
    virtual void WINAPI ExecuteOnFS(int panel, CPluginFSInterfaceAbstract* pluginFS, const char* pluginFSName, int pluginFSNameIndex, CFileData& file, int isDir)
    {
        UNREFERENCED_PARAMETER(pluginFSNameIndex);
        OpenFileSystem* opened = static_cast<OpenFileSystem*>(pluginFS);
        if (opened == NULL) return;
        if (isDir == 2)
        {
            const std::string parentPath = opened->GetParentPath();
            int failReason = 0;
            SalamanderGeneral->ChangePanelPathToPluginFS(
                panel, pluginFSName, parentPath.c_str(), &failReason);
            return;
        }
        SalamatrixFileSystemItemData* data = reinterpret_cast<SalamatrixFileSystemItemData*>(file.PluginData);
        if (data == NULL) return;
        if (opened->IsRoot() && isDir != 0)
        { int failReason = 0; SalamanderGeneral->ChangePanelPathToPluginFS(panel, pluginFSName, data->Item.Id.c_str(), &failReason); return; }
        if (isDir != 0)
        {
            const std::string path = opened->GetPath() + "\\" + data->Item.Id;
            int failReason = 0;
            SalamanderGeneral->ChangePanelPathToPluginFS(
                panel, pluginFSName, path.c_str(), &failReason);
            return;
        }
        opened->ExecuteDefault(file, panel);
    }
    virtual BOOL WINAPI DisconnectFS(HWND parent, BOOL isInPanel, int panel, CPluginFSInterfaceAbstract* pluginFS, const char* pluginFSName, int pluginFSNameIndex)
    { UNREFERENCED_PARAMETER(isInPanel); UNREFERENCED_PARAMETER(panel); UNREFERENCED_PARAMETER(pluginFSName); UNREFERENCED_PARAMETER(pluginFSNameIndex); SalamanderGeneral->CloseDetachedFS(parent, pluginFS); return TRUE; }
    virtual void WINAPI ConvertPathToInternal(const char* fsName, int fsNameIndex, char* fsUserPart)
    { UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(fsNameIndex); UNREFERENCED_PARAMETER(fsUserPart); }
    virtual void WINAPI ConvertPathToExternal(const char* fsName, int fsNameIndex, char* fsUserPart)
    { UNREFERENCED_PARAMETER(fsName); UNREFERENCED_PARAMETER(fsNameIndex); UNREFERENCED_PARAMETER(fsUserPart); }
    virtual void WINAPI EnsureShareExistsOnServer(int panel, const char* server, const char* share)
    { UNREFERENCED_PARAMETER(panel); UNREFERENCED_PARAMETER(server); UNREFERENCED_PARAMETER(share); }
};

PackageManager::PackageManager()
    : General(NULL),
      Runtimes(NULL),
      Extensions(NULL),
      Commands(NULL),
      FileOperations(NULL),
      Events(NULL),
      Sides(NULL),
      Storage(NULL),
      UI(NULL),
      Menu(NULL),
      Viewer(NULL),
      FileSystem(NULL),
      RefreshDeferred(FALSE),
      RefreshInProgress(FALSE),
      RefreshPending(FALSE),
      ActiveHostDispatches(0),
      ActiveExecutions(0),
      ShuttingDown(FALSE),
      ExecutionsIdleEvent(CreateEvent(NULL, TRUE, TRUE, NULL))
{
    InitializeCriticalSection(&FileSystemActionThreadsLock);
}

PackageManager::~PackageManager()
{
    Shutdown();
    if (ExecutionsIdleEvent != NULL)
    {
        CloseHandle(ExecutionsIdleEvent);
        ExecutionsIdleEvent = NULL;
    }
    DeleteCriticalSection(&FileSystemActionThreadsLock);
}

BOOL PackageManager::Initialize(
    CSalamanderGeneralAbstract* general,
    Runtime::IRuntimeService* runtimes,
    Extensions::IExtensionsService* extensions,
    Commands::ICommandService* commands,
    FileOperations::IFileOperationsService* fileOperations,
    Events::IEventsService* events,
    Sides::ISidesService* sides,
    Storage::IStorageService* storage,
    UI::IUIService* ui)
{
    InterlockedExchange(&ShuttingDown, FALSE);
    General = general;
    Runtimes = runtimes;
    Extensions = extensions;
    Commands = commands;
    FileOperations = fileOperations;
    Events = events;
    Sides = sides;
    Storage = storage;
    UI = ui;
    if (Menu == NULL)
        Menu = new MenuExtension(this);
    if (Viewer == NULL)
        Viewer = new ViewerExtension(this);
    if (FileSystem == NULL)
        FileSystem = new FileSystemExtension(this);
    if (Extensions != NULL)
    {
        Extensions->SetRefreshCallback(RefreshCallback, this);
        Extensions->SetManagementCallback(ManagementCallback, this);
    }
    return General != NULL && Runtimes != NULL && Extensions != NULL &&
           Commands != NULL && FileOperations != NULL && Events != NULL &&
           Sides != NULL && Storage != NULL && UI != NULL;
}

void PackageManager::Shutdown()
{
    InterlockedExchange(&ShuttingDown, TRUE);
    for (size_t index = 0; index < Packages.size(); ++index)
        InterlockedExchange(&Packages[index]->Stopping, TRUE);
    if (ExecutionsIdleEvent != NULL)
    {
        Runtime::WaitForThreadWithSentMessageDispatch(
            ExecutionsIdleEvent,
            General != NULL ? General->GetMainWindowHWND() : NULL);
    }
    std::vector<HANDLE> actionThreads;
    EnterCriticalSection(&FileSystemActionThreadsLock);
    actionThreads.swap(FileSystemActionThreads);
    LeaveCriticalSection(&FileSystemActionThreadsLock);
    for (size_t index = 0; index < actionThreads.size(); ++index)
    {
        Runtime::WaitForThreadWithSentMessageDispatch(
            actionThreads[index],
            General != NULL ? General->GetMainWindowHWND() : NULL);
        CloseHandle(actionThreads[index]);
    }
    if (Extensions != NULL)
    {
        Extensions->SetRefreshCallback(NULL, NULL);
        Extensions->SetManagementCallback(NULL, NULL);
    }
    UnregisterToolbarButtons();
    RemovePackages();
    delete Menu;
    Menu = NULL;
    delete Viewer;
    Viewer = NULL;
    delete FileSystem;
    FileSystem = NULL;
    Roots.clear();
    CustomPackages.clear();
    ExtensionOrder.clear();
    RemovedExtensions.clear();
    RegisteredViewerKeys.clear();
    General = NULL;
    Runtimes = NULL;
    Extensions = NULL;
    Commands = NULL;
    FileOperations = NULL;
    Events = NULL;
    Sides = NULL;
    Storage = NULL;
    UI = NULL;
}

void PackageManager::LoadConfiguration(HKEY key, CSalamanderRegistryAbstract* registry)
{
    Roots.clear();
    CustomPackages.clear();
    ExtensionOrder.clear();
    RemovedExtensions.clear();
    RegisteredViewerKeys.clear();
    Roots.push_back(ExpandRoot(L"$(SalDir)\\extensions"));
    Roots.push_back(ExpandRoot(L"$(SalDir)\\plugins\\automation\\scripts"));
    if (key == NULL || registry == NULL)
        return;
    HKEY rootsKey = NULL;
    if (registry->OpenKey(key, "ExtensionRoots", rootsKey))
    {
        char name[16];
        char path[SAL_MAX_PATH];
        for (int index = 1;; ++index)
        {
            _snprintf_s(name, _countof(name), _TRUNCATE, "%d", index);
            if (!registry->GetValue(
                    rootsKey, name, REG_SZ, path, _countof(path)))
                break;
            std::wstring root;
            if (!ToWide(path, &root))
                continue;
            if (root != ExpandRoot(L"$(SalDir)\\extensions") &&
                root != ExpandRoot(
                            L"$(SalDir)\\plugins\\automation\\scripts"))
                Roots.push_back(ExpandRoot(root));
        }
        registry->CloseKey(rootsKey);
    }

    struct StringListLoader
    {
        static void Load(
            HKEY parent, const char* subKey,
            CSalamanderRegistryAbstract* registry,
            std::vector<std::string>* values)
        {
            HKEY listKey = NULL;
            if (!registry->OpenKey(parent, subKey, listKey))
                return;
            char name[16];
            char value[512];
            for (int index = 1;; ++index)
            {
                _snprintf_s(name, _countof(name), _TRUNCATE, "%d", index);
                if (!registry->GetValue(
                        listKey, name, REG_SZ, value, _countof(value)))
                    break;
                if (value[0] != 0)
                    values->push_back(value);
            }
            registry->CloseKey(listKey);
        }
    };
    StringListLoader::Load(
        key, "ExtensionOrder", registry, &ExtensionOrder);
    StringListLoader::Load(
        key, "RemovedExtensions", registry, &RemovedExtensions);
    StringListLoader::Load(
        key, "RegisteredViewers", registry, &RegisteredViewerKeys);
    std::vector<std::string> customPackages;
    StringListLoader::Load(
        key, "ExtensionManifests", registry, &customPackages);
    for (size_t index = 0; index < customPackages.size(); ++index)
    {
        std::wstring packageDirectory;
        if (ToWide(customPackages[index], &packageDirectory))
            CustomPackages.push_back(packageDirectory);
    }
}

void PackageManager::SaveConfiguration(HKEY key, CSalamanderRegistryAbstract* registry)
{
    if (key == NULL || registry == NULL)
        return;
    HKEY rootsKey = NULL;
    if (!registry->CreateKey(key, "ExtensionRoots", rootsKey))
        return;
    registry->ClearKey(rootsKey);
    char name[16];
    for (size_t index = 0; index < Roots.size(); ++index)
    {
        _snprintf_s(name, _countof(name), _TRUNCATE, "%d", static_cast<int>(index + 1));
        std::string root;
        if (ToUtf8(Roots[index], &root))
            registry->SetValue(rootsKey, name, REG_SZ, root.c_str(), -1);
    }
    registry->CloseKey(rootsKey);

    struct StringListSaver
    {
        static void Save(
            HKEY parent, const char* subKey,
            CSalamanderRegistryAbstract* registry,
            const std::vector<std::string>& values)
        {
            HKEY listKey = NULL;
            if (!registry->CreateKey(parent, subKey, listKey))
                return;
            registry->ClearKey(listKey);
            char name[16];
            for (size_t index = 0; index < values.size(); ++index)
            {
                _snprintf_s(
                    name, _countof(name), _TRUNCATE, "%d",
                    static_cast<int>(index + 1));
                registry->SetValue(
                    listKey, name, REG_SZ, values[index].c_str(), -1);
            }
            registry->CloseKey(listKey);
        }
    };
    StringListSaver::Save(
        key, "ExtensionOrder", registry, ExtensionOrder);
    StringListSaver::Save(
        key, "RemovedExtensions", registry, RemovedExtensions);
    StringListSaver::Save(
        key, "RegisteredViewers", registry, RegisteredViewerKeys);
    std::vector<std::string> customPackages;
    for (size_t index = 0; index < CustomPackages.size(); ++index)
    {
        std::string packageDirectory;
        if (ToUtf8(CustomPackages[index], &packageDirectory))
            customPackages.push_back(packageDirectory);
    }
    StringListSaver::Save(
        key, "ExtensionManifests", registry, customPackages);
}

void PackageManager::Refresh()
{
    if (InterlockedCompareExchange(&ShuttingDown, FALSE, FALSE) != FALSE)
        return;
    // Runtime providers register and unregister through the same callback that
    // is used for user-requested catalog refreshes. During load-on-start those
    // callbacks arrive once per runtime provider; coalesce them into the one
    // final refresh requested by PLUGINEVENT_STARTUPCOMPLETE.
    if (RefreshDeferred)
    {
        RefreshPending = TRUE;
        return;
    }

    // The host registers this temporary service before unloading any plug-in.
    // Rebuilding and reactivating the complete extension catalog while runtime
    // providers are being removed is both wasted work and can repeat once per
    // provider, substantially extending shutdown.
    if (General != NULL)
    {
        CSalamanderServiceQuery query;
        CSalamanderServiceResult result;
        memset(&query, 0, sizeof(query));
        memset(&result, 0, sizeof(result));
        query.ServiceId = SALAMANDER_SERVICE_SHUTDOWN_PROGRESS;
        query.MinimumVersion = SALAMANDER_SHUTDOWN_PROGRESS_VERSION_1_0;
        if (General->QueryService(&query, &result))
        {
            RefreshPending = FALSE;
            return;
        }
    }

    // A package execution or modal host call keeps its Package context alive
    // while Windows pumps messages. Runtime/provider notifications can request
    // another catalog refresh from that nested loop. Defer it until the whole
    // package operation unwinds; deleting the package here would invalidate
    // the execution/dispatch context and its UI resources.
    if (RefreshInProgress || ActiveHostDispatches != 0 ||
        ActiveExecutions != 0)
    {
        RefreshPending = TRUE;
        return;
    }
    RefreshInProgress = TRUE;
    RefreshPending = FALSE;
    ReportStartupProgress(
        ssppDiscoveringExtensions, NULL, 0,
        static_cast<int>(Roots.size() + CustomPackages.size()));
    // Toolbar registrations outlive the package objects that contributed
    // them.  Drop them before rebuilding the package list so a runtime
    // availability refresh cannot leave stale or missing Extension Bar
    // entries behind.
    UnregisterToolbarButtons();
    RemovePackages();
    for (size_t index = 0; index < Roots.size(); ++index)
        DiscoverRoot(Roots[index]);
    for (size_t index = 0; index < CustomPackages.size(); ++index)
    {
        std::wstring parent = CustomPackages[index];
        size_t slash = parent.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
        {
            parent.erase(slash);
            DiscoverDirectory(parent, &CustomPackages[index]);
        }
    }
    ResolveDependenciesAndActivate();
    ApplyUserOrder();
    RegisterToolbarButtons();
    if (General != NULL)
        General->PostPluginMenuChanged();
    RefreshInProgress = FALSE;
    if (RefreshPending && ActiveHostDispatches == 0 &&
        ActiveExecutions == 0)
        Refresh();
}

void PackageManager::DiscoverRoot(const std::wstring& root)
{
    if (!root.empty())
        DiscoverDirectory(root);
}

void PackageManager::DiscoverDirectory(
    const std::wstring& directory,
    const std::wstring* onlyPackage)
{
    std::wstring pattern = directory + L"\\*";
    WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileW(pattern.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE)
        return;
    do
    {
        if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0)
            continue;
        std::wstring path = directory + L"\\" + data.cFileName;
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            if (onlyPackage != NULL &&
                _wcsicmp(path.c_str(), onlyPackage->c_str()) != 0)
                continue;
            std::wstring manifestPath = path + L"\\extension.json";
            std::string json;
            if (ReadUtf8File(manifestPath, &json))
            {
                CExtensionManifest manifest;
                CExtensionManifestError error;
                if (manifest.Parse(json.data(), json.size(), error) &&
                    !IsRemoved(manifest.Id) &&
                    CExtensionManifest::IsSafeRelativeEntryPoint(manifest.EntryPoint))
                {
                    const std::string baseName = manifest.Name;
                    const std::string preferred =
                        CurrentSalamanderLocale(General);
                    int selectedLocale = -1;
                    int bestLocaleScore = 0;
                    for (size_t localeIndex = 0;
                         localeIndex < manifest.Locales.size();
                         ++localeIndex)
                    {
                        const int score = LocaleMatchScore(
                            manifest.Locales[localeIndex].Language,
                            preferred);
                        if (score > bestLocaleScore)
                        {
                            selectedLocale =
                                static_cast<int>(localeIndex);
                            bestLocaleScore = score;
                        }
                    }
                    if (selectedLocale >= 0)
                    {
                        std::wstring localeRelative;
                        std::string localeJson;
                        CExtensionManifestLocaleText localized;
                        CExtensionManifestError localeError;
                        if (ToWide(
                                manifest.Locales[selectedLocale].File,
                                &localeRelative) &&
                            ReadUtf8File(
                                path + L"\\" + localeRelative,
                                &localeJson) &&
                            CExtensionManifest::ParseLocaleText(
                                localeJson.data(), localeJson.size(),
                                localized, localeError))
                        {
                            if (!localized.Name.empty())
                                manifest.Name = localized.Name;
                            if (!localized.Description.empty())
                                manifest.Description = localized.Description;
                            for (size_t commandIndex = 0;
                                 commandIndex < manifest.Commands.size();
                                 ++commandIndex)
                            {
                                const char* title =
                                    FindLocalizedCommandTitle(
                                        localized,
                                        manifest.Commands[commandIndex].Id);
                                if (title != NULL)
                                    manifest.Commands[commandIndex].Title = title;
                                else if (
                                    manifest.Commands[commandIndex].Title ==
                                    baseName)
                                    manifest.Commands[commandIndex].Title =
                                        manifest.Name;
                            }
                            for (size_t settingIndex = 0;
                                 settingIndex < manifest.Settings.size();
                                 ++settingIndex)
                            {
                                CExtensionManifestSetting& setting =
                                    manifest.Settings[settingIndex];
                                const CExtensionManifestLocalizedSetting*
                                    translated = FindLocalizedSetting(
                                        localized, setting.Key);
                                if (translated == NULL)
                                    continue;
                                if (!translated->Label.empty())
                                    setting.Label = translated->Label;
                                if (!translated->Description.empty())
                                    setting.Description =
                                        translated->Description;
                                if (!translated->Group.empty())
                                    setting.Group = translated->Group;
                            }
                            for (size_t fsIndex = 0; fsIndex < manifest.FileSystems.size(); ++fsIndex)
                            {
                                CExtensionManifestFileSystem& fileSystem = manifest.FileSystems[fsIndex];
                                const CExtensionManifestLocalizedFileSystem* translatedFs =
                                    FindLocalizedFileSystem(localized, fileSystem.Id);
                                if (translatedFs == NULL)
                                    continue;
                                if (!translatedFs->Name.empty())
                                    fileSystem.Name = translatedFs->Name;
                                for (size_t itemIndex = 0; itemIndex < fileSystem.RootItems.size(); ++itemIndex)
                                {
                                    CExtensionManifestFileSystem::RootItem& item = fileSystem.RootItems[itemIndex];
                                    const CExtensionManifestLocalizedFileSystemRootItem* translatedItem =
                                        FindLocalizedFileSystemRootItem(*translatedFs, item.Id);
                                    if (translatedItem != NULL)
                                        item.Name = translatedItem->Name;
                                }
                                for (size_t columnIndex = 0; columnIndex < fileSystem.Columns.size(); ++columnIndex)
                                {
                                    CExtensionManifestFileSystem::Column& column = fileSystem.Columns[columnIndex];
                                    const CExtensionManifestLocalizedFileSystemColumn* translatedColumn =
                                        FindLocalizedFileSystemColumn(*translatedFs, column.Id);
                                    if (translatedColumn == NULL)
                                        continue;
                                    if (!translatedColumn->Name.empty())
                                        column.Name = translatedColumn->Name;
                                    if (!translatedColumn->Description.empty())
                                        column.Description = translatedColumn->Description;
                                }
                                for (size_t actionIndex = 0; actionIndex < fileSystem.Actions.size(); ++actionIndex)
                                {
                                    CExtensionManifestFileSystem::Action& action = fileSystem.Actions[actionIndex];
                                    if (action.Separator)
                                        continue;
                                    const CExtensionManifestLocalizedFileSystemAction* translatedAction =
                                        FindLocalizedFileSystemAction(*translatedFs, action.Id);
                                    if (translatedAction != NULL)
                                        action.Title = translatedAction->Title;
                                }
                            }
                        }
                    }
                    for (size_t commandIndex = 0;
                         commandIndex < manifest.Commands.size();
                         ++commandIndex)
                    {
                        CExtensionManifestCommand& command =
                            manifest.Commands[commandIndex];
                        if (command.Enabled &&
                            !command.RequiresExecutable.empty() &&
                            !IsExecutableAvailable(
                                command.RequiresExecutable))
                        {
                            command.Enabled = false;
                        }
                    }
                    Package* package = new Package(this);
                    package->Manifest = manifest;
                    package->InitialCommands = manifest.Commands;
                    package->Directory = path;
                    package->EntryPoint = path + L"\\";
                    std::wstring relative;
                    ToWide(manifest.EntryPoint, &relative);
                    package->EntryPoint += relative;
                    package->Id = manifest.Id;
                    package->SettingsReady =
                        Settings::ApplyMigrations(
                            Storage, package->Id.c_str(),
                            manifest.SettingsVersion,
                            manifest.SettingsMigrations) &&
                        Settings::MaterializeDefaults(
                            Storage, package->Id.c_str(),
                            manifest.Settings);
                    ToUtf8(package->EntryPoint, &package->EntryPointUtf8);
                    std::wstring icon = path + L"\\";
                    std::wstring iconRelative;
                    if (!manifest.Icon.empty() && ToWide(manifest.Icon, &iconRelative))
                    {
                        std::string iconUtf8;
                        ToUtf8(icon + iconRelative, &iconUtf8);
                        package->IconPath = iconUtf8;
                    }
                    if (!manifest.IconDark.empty() && ToWide(manifest.IconDark, &iconRelative))
                    {
                        std::string iconUtf8;
                        ToUtf8(icon + iconRelative, &iconUtf8);
                        package->IconDarkPath = iconUtf8;
                    }
                    Extensions::ExtensionDescriptor descriptor;
                    StringCchCopyA(descriptor.Id, _countof(descriptor.Id), manifest.Id.c_str());
                    StringCchCopyA(descriptor.Name, _countof(descriptor.Name), manifest.Name.c_str());
                    StringCchCopyA(descriptor.Version, _countof(descriptor.Version), manifest.Version.c_str());
                    StringCchCopyA(descriptor.RuntimeId, _countof(descriptor.RuntimeId), manifest.RuntimeId.c_str());
                    std::string displayEntryPoint = package->EntryPointUtf8;
                    MakeDisplayEntryPoint(package->EntryPoint, &displayEntryPoint);
                    StringCchCopyA(descriptor.EntryPoint, _countof(descriptor.EntryPoint), displayEntryPoint.c_str());
                    StringCchCopyA(descriptor.IconPath, _countof(descriptor.IconPath), package->IconPath.c_str());
                    StringCchCopyA(descriptor.IconDarkPath, _countof(descriptor.IconDarkPath), package->IconDarkPath.c_str());
                    descriptor.Flags = Extensions::ExtensionFlagManifest |
                                       Extensions::ExtensionFlagPackage |
                                       Extensions::ExtensionFlagPersistent;
                    if (!manifest.Commands.empty())
                        descriptor.Flags |= Extensions::ExtensionFlagMenuExtension;
                    if (!manifest.Viewers.empty())
                        descriptor.Flags |= Extensions::ExtensionFlagViewer;
                    if (!manifest.FileSystems.empty())
                        descriptor.Flags |= Extensions::ExtensionFlagFileSystem;
                    BOOL enabled = TRUE;
                    if (Storage != NULL &&
                        Storage->GetValueType(
                            package->Id.c_str(), "salamatrix.enabled") ==
                            Storage::StorageValueBoolean)
                    {
                        Storage->GetBoolean(
                            package->Id.c_str(), "salamatrix.enabled",
                            &enabled);
                    }
                    if (!enabled)
                        descriptor.Flags |= Extensions::ExtensionFlagDisabled;
                    bool registeredRuntime = false;
                    bool availableRuntime = false;
                    for (int adapterIndex = 0;
                         adapterIndex < Runtimes->GetAdapterCount();
                         ++adapterIndex)
                    {
                        Runtime::IRuntimeAdapter* adapter =
                            Runtimes->GetAdapter(adapterIndex);
                        const Runtime::RuntimeAdapterDescriptor* runtimeDescriptor =
                            adapter != NULL ? adapter->GetDescriptor() : NULL;
                        if (runtimeDescriptor != NULL &&
                            runtimeDescriptor->RuntimeId != NULL &&
                            _stricmp(runtimeDescriptor->RuntimeId,
                                     manifest.RuntimeId.c_str()) == 0 &&
                            runtimeDescriptor->RuntimeVersion >=
                                manifest.MinimumRuntimeVersion)
                        {
                            registeredRuntime = true;
                            availableRuntime = adapter->IsAvailable() != FALSE;
                            break;
                        }
                    }
                    if (!registeredRuntime)
                        descriptor.Flags |= Extensions::ExtensionFlagRuntimeUnavailable;
                    else if (!availableRuntime)
                        descriptor.Flags |= Extensions::ExtensionFlagRuntimeExecutableUnavailable;

                    if ((!registeredRuntime || !availableRuntime) &&
                        _stricmp(manifest.RuntimeId.c_str(), "Automation.JScript") == 0 &&
                        QueryScriptRunner(General) != NULL)
                    {
                        // The legacy JScript provider is owned by Automation.
                        // Its public ScriptRunner service is sufficient for
                        // one-shot extension commands even if compatibility
                        // adapter discovery or availability raced startup.
                        descriptor.Flags &=
                            ~(Extensions::ExtensionFlagRuntimeUnavailable |
                              Extensions::ExtensionFlagRuntimeExecutableUnavailable);
                        registeredRuntime = true;
                        availableRuntime = true;
                    }
                    package->RuntimeUsable = registeredRuntime && availableRuntime;
                    package->Descriptor = descriptor;
                    if (Extensions->RegisterExtension(&descriptor, LifecycleCallback, package))
                    {
                        std::vector<Extensions::ExtensionSettingInfo> settings;
                        for (size_t setting = 0; setting < manifest.Settings.size(); ++setting)
                        {
                            const CExtensionManifestSetting& source = manifest.Settings[setting];
                            Extensions::ExtensionSettingInfo target;
                            StringCchCopyA(target.Key, _countof(target.Key), source.Key.c_str());
                            StringCchCopyA(target.Label, _countof(target.Label), source.Label.c_str());
                            StringCchCopyA(target.Description, _countof(target.Description), source.Description.c_str());
                            StringCchCopyA(target.Group, _countof(target.Group), source.Group.c_str());
                            target.Order = source.Order;
                            target.Width = source.Width;
                            target.Multiline = source.Multiline ? TRUE : FALSE;
                            target.Type = source.Type == ExtensionManifestSettingInteger
                                              ? Extensions::ExtensionSettingInteger
                                              : source.Type == ExtensionManifestSettingBoolean
                                                    ? Extensions::ExtensionSettingBoolean
                                                    : Extensions::ExtensionSettingString;
                            settings.push_back(target);
                        }
                        Extensions->SetExtensionSettingsSchema(
                            package->Id.c_str(),
                            settings.empty() ? NULL : &settings[0],
                            static_cast<int>(settings.size()));
                        for (size_t command = 0; command < manifest.Commands.size(); ++command)
                        {
                            int id = 0x62000000 + static_cast<int>(Packages.size() * 64 + command + 1);
                            package->CommandIds.push_back(id);
                            package->CommandHotKeys.push_back(0);
                            std::wstring commandIcon;
                            const std::string& declaredIcon =
                                !manifest.Commands[command].Icon.empty()
                                    ? manifest.Commands[command].Icon
                                    : manifest.Icon;
                            std::string commandIconUtf8 = package->IconPath;
                            if (!declaredIcon.empty() && ToWide(declaredIcon, &commandIcon))
                                ToUtf8(path + L"\\" + commandIcon, &commandIconUtf8);
                            package->CommandIconPaths.push_back(commandIconUtf8);
                            std::wstring commandIconDark;
                            const std::string declaredIconDark =
                                !manifest.Commands[command].IconDark.empty()
                                    ? manifest.Commands[command].IconDark
                                    : manifest.Commands[command].Icon.empty()
                                          ? manifest.IconDark
                                          : std::string();
                            std::string commandIconDarkUtf8;
                            if (!declaredIconDark.empty() &&
                                ToWide(declaredIconDark, &commandIconDark))
                            {
                                ToUtf8(path + L"\\" + commandIconDark,
                                       &commandIconDarkUtf8);
                            }
                            package->CommandIconDarkPaths.push_back(
                                commandIconDarkUtf8);
                        }
                        package->InitialCommandIds = package->CommandIds;
                        package->InitialCommandHotKeys = package->CommandHotKeys;
                        package->InitialCommandIconPaths = package->CommandIconPaths;
                        package->InitialCommandIconDarkPaths =
                            package->CommandIconDarkPaths;
                        Packages.push_back(package);
                    }
                    else
                    {
                        delete package;
                    }
                }
            }
            if (onlyPackage == NULL)
                DiscoverDirectory(path);
        }
    } while (FindNextFileW(find, &data));
    FindClose(find);
}

void PackageManager::ResolveDependenciesAndActivate()
{
    if (Extensions == NULL)
        return;

    for (size_t index = 0; index < Packages.size(); ++index)
    {
        Package* package = Packages[index];
        const char* progressDetail = package->Manifest.Name.empty()
                                         ? package->Id.c_str()
                                         : package->Manifest.Name.c_str();
        ReportStartupProgress(
            ssppRegisteringExtensions, progressDetail,
            static_cast<int>(index + 1), static_cast<int>(Packages.size()));
        bool dependencyUnavailable = false;
        for (size_t dependencyIndex = 0;
             dependencyIndex < package->Manifest.Dependencies.size();
             ++dependencyIndex)
        {
            const std::string& dependencyId =
                package->Manifest.Dependencies[dependencyIndex];
            Package* dependency = NULL;
            for (size_t candidate = 0; candidate < Packages.size(); ++candidate)
            {
                if (_stricmp(
                        Packages[candidate]->Id.c_str(),
                        dependencyId.c_str()) == 0)
                {
                    dependency = Packages[candidate];
                    break;
                }
            }
            if (dependency == NULL || !dependency->RuntimeUsable ||
                !dependency->SettingsReady ||
                (dependency->Descriptor.Flags &
                 Extensions::ExtensionFlagDisabled) != 0)
            {
                dependencyUnavailable = true;
                break;
            }
        }
        if (dependencyUnavailable)
            package->Descriptor.Flags |=
                Extensions::ExtensionFlagDependencyUnavailable;
        else
            package->Descriptor.Flags &=
                ~Extensions::ExtensionFlagDependencyUnavailable;
        Extensions->RegisterExtension(
            &package->Descriptor, LifecycleCallback, package);

        if (!package->Manifest.FileSystems.empty())
        {
            ReportStartupProgress(
                ssppRegisteringFileSystems, progressDetail,
                static_cast<int>(index + 1), static_cast<int>(Packages.size()));
        }
        bool hasMenuCommand = false;
        for (size_t commandIndex = 0;
             commandIndex < package->Manifest.Commands.size(); ++commandIndex)
        {
            const std::string& placement =
                package->Manifest.Commands[commandIndex].Menu;
            if (placement == "plugin" || placement == "both")
            {
                hasMenuCommand = true;
                break;
            }
        }
        if (hasMenuCommand)
        {
            ReportStartupProgress(
                ssppRegisteringMenuCommands, progressDetail,
                static_cast<int>(index + 1), static_cast<int>(Packages.size()));
        }
    }

    for (size_t index = 0; index < Packages.size(); ++index)
    {
        Package* package = Packages[index];
        const char* progressDetail = package->Manifest.Name.empty()
                                         ? package->Id.c_str()
                                         : package->Manifest.Name.c_str();
        const DWORD blocked =
            Extensions::ExtensionFlagDisabled |
            Extensions::ExtensionFlagRuntimeUnavailable |
            Extensions::ExtensionFlagRuntimeExecutableUnavailable |
            Extensions::ExtensionFlagDependencyUnavailable;
        const bool needsPersistentWorker =
            !package->Manifest.EventsDeclared ||
            !package->Manifest.Events.empty();
        if (needsPersistentWorker && package->SettingsReady &&
            (package->Descriptor.Flags & blocked) == 0)
        {
            ReportStartupProgress(
                ssppActivatingExtensions, progressDetail,
                static_cast<int>(index + 1), static_cast<int>(Packages.size()));
            Extensions->ActivateExtension(package->Id.c_str());
        }
    }
}

void PackageManager::RemovePackages()
{
    for (size_t index = 0; index < Packages.size(); ++index)
    {
        Package* package = Packages[index];
        const char* progressDetail = package->Manifest.Name.empty()
                                         ? package->Id.c_str()
                                         : package->Manifest.Name.c_str();
        InterlockedExchange(&package->Stopping, TRUE);
        if (Extensions != NULL)
        {
            ReportShutdownProgress(
                ssdpUnregisteringExtensions, progressDetail,
                static_cast<int>(index + 1),
                static_cast<int>(Packages.size()));
            Extensions->UnregisterExtension(package->Id.c_str(), package);
        }
        ReleaseEventSubscriptions(package);
        ReportShutdownProgress(
            ssdpStoppingExtensionRuntimes, progressDetail,
            static_cast<int>(index + 1),
            static_cast<int>(Packages.size()));
        StopSession(package);
        ReportShutdownProgress(
            ssdpClosingExtensionWindows, progressDetail,
            static_cast<int>(index + 1),
            static_cast<int>(Packages.size()));
        ReleaseProgress(package);
        ReleaseDialogs(package);
        delete package;
    }
    Packages.clear();
}

CPluginInterfaceForMenuExtAbstract* PackageManager::GetMenuExtension()
{
    return Menu;
}

CPluginInterfaceForViewerAbstract* PackageManager::GetViewerExtension()
{
    return Viewer;
}

CPluginInterfaceForFSAbstract* PackageManager::GetFileSystemExtension()
{
    return FileSystem;
}

void PackageManager::RegisterViewerMasks(CSalamanderConnectAbstract* salamander)
{
    if (salamander == NULL)
        return;
    for (size_t packageIndex = 0; packageIndex < Packages.size(); ++packageIndex)
    {
        Package* package = Packages[packageIndex];
        // Viewer masks and identities are declarative manifest metadata and
        // must be published during Connect even when the independent runtime
        // provider connects later in startup. RunViewer still requires a
        // usable runtime before dispatching the handler.
        if (package == NULL ||
            (package->Descriptor.Flags &
             Extensions::ExtensionFlagDisabled) != 0)
            continue;
        if (!package->Manifest.Viewers.empty())
        {
            const char* progressDetail = package->Manifest.Name.empty()
                                             ? package->Id.c_str()
                                             : package->Manifest.Name.c_str();
            ReportStartupProgress(
                ssppRegisteringViewers, progressDetail,
                static_cast<int>(packageIndex + 1),
                static_cast<int>(Packages.size()));
        }
        for (size_t viewerIndex = 0;
             viewerIndex < package->Manifest.Viewers.size(); ++viewerIndex)
        {
            const CExtensionManifestViewer& viewer =
                package->Manifest.Viewers[viewerIndex];
            std::string label = package->Manifest.Name;
            if (!viewer.Name.empty())
            {
                label += " - ";
                label += viewer.Name;
            }
            std::string group;
            const auto registerGroup = [&]()
            {
                if (group.empty())
                    return;
                const std::string key = package->Id + "|" + viewer.Handler + "|" + group;
                const bool firstRegistration =
                    std::find(RegisteredViewerKeys.begin(), RegisteredViewerKeys.end(), key) ==
                    RegisteredViewerKeys.end();

                // The non-forced call handles a fresh Salamatrix plug-in
                // installation and refreshes the label of an exact existing
                // association. The forced call is made only once for a newly
                // discovered extension viewer, so a later user removal stays
                // respected.
                salamander->AddViewerWithLabel(group.c_str(), FALSE, label.c_str());
                if (firstRegistration)
                {
                    salamander->AddViewerWithLabel(group.c_str(), TRUE, label.c_str());
                    RegisteredViewerKeys.push_back(key);
                }
                group.clear();
            };
            for (size_t patternIndex = 0;
                 patternIndex < viewer.Patterns.size(); ++patternIndex)
            {
                const std::string& pattern = viewer.Patterns[patternIndex];
                if (!group.empty() && group.size() + pattern.size() + 1 > 190)
                    registerGroup();
                if (!group.empty())
                    group += ";";
                group += pattern;
            }
            registerGroup();
        }
    }
}

void PackageManager::SetRefreshDeferred(BOOL deferred)
{
    if (RefreshDeferred == deferred)
        return;
    RefreshDeferred = deferred;
    if (!RefreshDeferred && RefreshPending)
        Refresh();
}

void PackageManager::CompleteStartupRefreshBatch()
{
    RefreshDeferred = FALSE;
    if (!RefreshPending)
        return;

    // The requests coalesced by PLUGINEVENT_STARTUPBATCHBEGIN come from
    // runtime providers registering during the load-on-start pass. The
    // manifests and package objects have already been loaded while restoring
    // configuration, so do not run Refresh(): it would tear down live file
    // systems and briefly blank every extension panel. Re-evaluate only the
    // runtime-dependent flags and activate packages that just became usable.
    RefreshPending = FALSE;
    for (size_t packageIndex = 0; packageIndex < Packages.size(); ++packageIndex)
    {
        Package* package = Packages[packageIndex];
        if (package == NULL)
            continue;

        bool registeredRuntime = false;
        bool availableRuntime = false;
        for (int adapterIndex = 0; adapterIndex < Runtimes->GetAdapterCount(); ++adapterIndex)
        {
            Runtime::IRuntimeAdapter* adapter = Runtimes->GetAdapter(adapterIndex);
            const Runtime::RuntimeAdapterDescriptor* descriptor =
                adapter != NULL ? adapter->GetDescriptor() : NULL;
            if (descriptor != NULL && descriptor->RuntimeId != NULL &&
                _stricmp(descriptor->RuntimeId, package->Manifest.RuntimeId.c_str()) == 0 &&
                descriptor->RuntimeVersion >= package->Manifest.MinimumRuntimeVersion)
            {
                registeredRuntime = true;
                availableRuntime = adapter->IsAvailable() != FALSE;
                break;
            }
        }
        if ((!registeredRuntime || !availableRuntime) &&
            _stricmp(package->Manifest.RuntimeId.c_str(), "Automation.JScript") == 0 &&
            QueryScriptRunner(General) != NULL)
        {
            registeredRuntime = true;
            availableRuntime = true;
        }

        package->Descriptor.Flags &=
            ~(Extensions::ExtensionFlagRuntimeUnavailable |
              Extensions::ExtensionFlagRuntimeExecutableUnavailable);
        if (!registeredRuntime)
            package->Descriptor.Flags |= Extensions::ExtensionFlagRuntimeUnavailable;
        else if (!availableRuntime)
            package->Descriptor.Flags |= Extensions::ExtensionFlagRuntimeExecutableUnavailable;
        package->RuntimeUsable = registeredRuntime && availableRuntime;
    }

    ResolveDependenciesAndActivate();
    UnregisterToolbarButtons();
    RegisterToolbarButtons();
    if (General != NULL)
        General->PostPluginMenuChanged();
}

BOOL WINAPI PackageManager::LifecycleCallback(
    void* context, Extensions::ExtensionAction action, const Extensions::ExtensionInfo* info)
{
    Package* package = static_cast<Package*>(context);
    if (package == NULL || package->Owner == NULL || info == NULL)
        return FALSE;
    return action == Extensions::ExtensionActionActivate
               ? package->Owner->Activate(package)
               : package->Owner->Deactivate(package);
}

BOOL WINAPI PackageManager::RefreshCallback(void* context)
{
    PackageManager* manager = static_cast<PackageManager*>(context);
    if (manager == NULL)
        return FALSE;
    manager->Refresh();
    return TRUE;
}

DWORD WINAPI PackageManager::PumpThreadProc(void* context)
{
    Package* package = static_cast<Package*>(context);
    while (package != NULL && package->Session != NULL && package->Session->IsAlive())
        package->Session->Pump(250);
    return 0;
}

BOOL PackageManager::Activate(Package* package)
{
    if (package == NULL || package->Session != NULL)
        return package != NULL;
    InterlockedExchange(&package->Stopping, FALSE);
    Runtime::IRuntimeAdapter* adapter = Runtimes->FindAdapter(
        package->Manifest.RuntimeId.c_str(), package->Manifest.MinimumRuntimeVersion);
    if (adapter == NULL || !adapter->IsAvailable())
        return FALSE;
    Runtime::RuntimeExecutionRequest request;
    request.ExtensionId = package->Id.c_str();
    request.EntryPoint = package->EntryPoint.c_str();
    request.ParentWindow = General->GetMsgBoxParent();
    request.Flags = Runtime::RuntimeExecutionFlagPersistentWorker |
                    Runtime::RuntimeExecutionFlagUseWorkerBootstrap;
    request.HostDispatch = HostDispatch;
    request.HostDispatchContext = package;
    if (!adapter->StartPersistent(&request, &package->Session) || package->Session == NULL)
        return FALSE;
    if (!package->Session->IsAlive())
    {
        package->Session->Release();
        package->Session = NULL;
        return FALSE;
    }
    package->PumpThread = CreateThread(NULL, 0, PumpThreadProc, package, 0, NULL);
    if (package->PumpThread == NULL)
    {
        package->Session->Stop();
        package->Session->Release();
        package->Session = NULL;
        return FALSE;
    }
    return TRUE;
}

BOOL PackageManager::Deactivate(Package* package)
{
    if (package == NULL)
        return TRUE;
    InterlockedExchange(&package->Stopping, TRUE);
    ReleaseEventSubscriptions(package);
    StopSession(package);
    ReleaseDialogs(package);
    if (package->CommandsChanged)
    {
        UnregisterToolbarButtons();
        package->Manifest.Commands = package->InitialCommands;
        package->CommandIds = package->InitialCommandIds;
        package->CommandHotKeys = package->InitialCommandHotKeys;
        package->CommandIconPaths = package->InitialCommandIconPaths;
        package->CommandIconDarkPaths = package->InitialCommandIconDarkPaths;
        package->CommandsChanged = FALSE;
        RefreshContributionFlags(package);
        RegisterToolbarButtons();
        if (General != NULL)
            General->PostPluginMenuChanged();
    }
    return TRUE;
}

void PackageManager::RefreshContributionFlags(Package* package)
{
    if (package == NULL)
        return;
    const DWORD contributionMask =
        Extensions::ExtensionFlagMenuExtension |
        Extensions::ExtensionFlagViewer |
        Extensions::ExtensionFlagFileSystem;
    DWORD flags = package->Descriptor.Flags & ~contributionMask;
    if (!package->Manifest.Commands.empty())
        flags |= Extensions::ExtensionFlagMenuExtension;
    if (!package->Manifest.Viewers.empty())
        flags |= Extensions::ExtensionFlagViewer;
    if (!package->Manifest.FileSystems.empty())
        flags |= Extensions::ExtensionFlagFileSystem;
    if (flags == package->Descriptor.Flags)
        return;
    package->Descriptor.Flags = flags;
    if (Extensions != NULL)
        Extensions->RegisterExtension(
            &package->Descriptor, LifecycleCallback, package);
}

void PackageManager::StopSession(Package* package)
{
    if (package == NULL)
        return;
    InterlockedExchange(&package->Stopping, TRUE);
    if (package->Session != NULL)
        package->Session->Stop();
    if (package->PumpThread != NULL)
    {
        // Do not release the session or package after a timed wait. Its pump
        // may be synchronously waiting for HostDispatchOnMainThread; dispatch
        // that sent message while joining so no thread and no callback can
        // outlive the package or the Salamatrix module.
        Runtime::WaitForThreadWithSentMessageDispatch(
            package->PumpThread,
            General != NULL ? General->GetMainWindowHWND() : NULL);
        CloseHandle(package->PumpThread);
        package->PumpThread = NULL;
    }
    if (package->Session != NULL)
    {
        package->Session->Release();
        package->Session = NULL;
    }
}

void PackageManager::CancelFileSystemListingForShutdown(
    const std::string& packageId)
{
    if (General == NULL || packageId.empty())
        return;

    CSalamanderServiceQuery query;
    CSalamanderServiceResult result;
    memset(&query, 0, sizeof(query));
    memset(&result, 0, sizeof(result));
    query.ServiceId = SALAMANDER_SERVICE_SHUTDOWN_PROGRESS;
    query.MinimumVersion = SALAMANDER_SHUTDOWN_PROGRESS_VERSION_1_0;
    if (!General->QueryService(&query, &result))
        return;

    for (size_t index = 0; index < Packages.size(); ++index)
    {
        Package* package = Packages[index];
        if (package != NULL &&
            _stricmp(package->Id.c_str(), packageId.c_str()) == 0)
        {
            InterlockedExchange(&package->Stopping, TRUE);
            // Do not release the session here: the listing worker still owns
            // its current call.  RemovePackages()/StopSession() joins and
            // releases it after the worker has unwound.
            if (package->Session != NULL)
                package->Session->Stop();
            return;
        }
    }
}

void PackageManager::ReleaseEventSubscriptions(Package* package)
{
    if (package == NULL)
        return;
    if (Events != NULL)
    {
        for (size_t index = 0;
             index < package->EventSubscriptions.size(); ++index)
            Events->Unsubscribe(package->EventSubscriptions[index]);
    }
    package->EventSubscriptions.clear();
}

void PackageManager::ReleaseDialogs(Package* package)
{
    if (package == NULL)
        return;
    for (size_t index = 0; index < package->Dialogs.size(); ++index)
    {
        Package::RuntimeDialog* binding = package->Dialogs[index];
        UI::IDialog* dialog = binding != NULL ? binding->Dialog : NULL;
        if (dialog != NULL && UI != NULL)
        {
            dialog->SetEventCallback(NULL, NULL);
            UI->DestroyDialog(dialog);
        }
        delete binding;
    }
    package->Dialogs.clear();
}

void PackageManager::ReleaseProgress(Package* package)
{
    if (package == NULL || package->Progress == NULL)
        return;
    package->Progress->Close();
    if (UI != NULL)
        UI->DestroyProgressDialog(package->Progress);
    package->Progress = NULL;
    package->ProgressId = 0;
}

BOOL PackageManager::ExecuteCommand(
    Package* package,
    CSalamanderForOperationsAbstract* operations,
    const char* commandId,
    const char* handler,
    const char* invocationJson)
{
    if (package == NULL || commandId == NULL || Runtimes == NULL)
        return FALSE;
    Runtime::IRuntimeAdapter* adapter = Runtimes->FindAdapter(
        package->Manifest.RuntimeId.c_str(), package->Manifest.MinimumRuntimeVersion);
    if (adapter == NULL || !adapter->IsAvailable())
    {
        if (_stricmp(package->Manifest.RuntimeId.c_str(), "Automation.JScript") == 0)
        {
            Automation::IScriptRunner* scriptRunner = QueryScriptRunner(General);
            if (scriptRunner != NULL)
            {
                Automation::GeneratedScriptRequest compatibilityRequest;
                compatibilityRequest.EntryPoint = package->EntryPoint.c_str();
                compatibilityRequest.RuntimeId = package->Manifest.RuntimeId.c_str();
                compatibilityRequest.ExtensionId = package->Id.c_str();
                compatibilityRequest.ParentWindow = General->GetMsgBoxParent();
                compatibilityRequest.TimeoutMs = 120000;
                compatibilityRequest.Operation = operations;
                Automation::GeneratedScriptResult compatibilityResult;
                BOOL executed = scriptRunner->ExecuteGenerated(
                    &compatibilityRequest, &compatibilityResult);
                package->Operations = NULL;
                return executed;
            }
        }
        return FALSE;
    }
    Runtime::RuntimeExecutionRequest request;
    request.ExtensionId = package->Id.c_str();
    request.CommandId = commandId;
    request.CommandHandler = handler;
    request.InvocationJson = invocationJson;
    request.EntryPoint = package->EntryPoint.c_str();
    request.ParentWindow = General->GetMsgBoxParent();
    request.Flags = Runtime::RuntimeExecutionFlagUseWorkerBootstrap |
                    Runtime::RuntimeExecutionFlagOneShotWorker;
    request.HostDispatch = HostDispatch;
    request.HostDispatchContext = package;
    package->Operations = operations;
    ReleaseProgress(package);
    Runtime::IRuntimeSession* session = NULL;
    if (!adapter->StartPersistent(&request, &session) || session == NULL)
    {
        const Runtime::RuntimeAdapterDescriptor* adapterDescriptor =
            adapter->GetDescriptor();
        if (adapterDescriptor == NULL ||
            (adapterDescriptor->Flags & Runtime::RuntimeAdapterFlagCompatibility) == 0)
        {
            package->Operations = NULL;
            return FALSE;
        }
        Automation::IScriptRunner* scriptRunner = QueryScriptRunner(General);
        if (scriptRunner != NULL)
        {
            Automation::GeneratedScriptRequest compatibilityRequest;
            compatibilityRequest.EntryPoint = request.EntryPoint;
            compatibilityRequest.RuntimeId = package->Manifest.RuntimeId.c_str();
            compatibilityRequest.ExtensionId = package->Id.c_str();
            compatibilityRequest.ParentWindow = request.ParentWindow;
            compatibilityRequest.TimeoutMs = request.TimeoutMs;
            compatibilityRequest.Operation = operations;
            Automation::GeneratedScriptResult compatibilityResult;
            BOOL executed = scriptRunner->ExecuteGenerated(
                &compatibilityRequest, &compatibilityResult);
            package->Operations = NULL;
            return executed;
        }
        package->Operations = NULL;
        return FALSE;
    }

    const ULONGLONG startedAt = GetTickCount64();
    while (session->IsAlive() && GetTickCount64() - startedAt < request.TimeoutMs)
        session->Pump(250);

    if (session->IsAlive())
        session->Stop();
    DWORD exitCode = 1;
    BOOL succeeded = session->GetExitCode(&exitCode) && exitCode == 0;
    session->Release();
    ReleaseProgress(package);
    package->Operations = NULL;
    return succeeded;
}

BOOL PackageManager::RunViewer(const char* fileName, const char* invocationJson,
                               const char* viewerLabel)
{
    if (fileName == NULL || General == NULL)
        return FALSE;
    ExecutionGuard execution(this);
    const char* extension = strrchr(fileName, '.');
    const BOOL hasExtension = extension != NULL && extension[1] != '\0';
    for (size_t packageIndex = 0; packageIndex < Packages.size(); ++packageIndex)
    {
        Package* package = Packages[packageIndex];
        if (package == NULL || !package->RuntimeUsable)
            continue;
        for (size_t viewerIndex = 0;
             viewerIndex < package->Manifest.Viewers.size(); ++viewerIndex)
        {
            const CExtensionManifestViewer& viewer =
                package->Manifest.Viewers[viewerIndex];
            if (viewerLabel != NULL && viewerLabel[0] != 0)
            {
                std::string label = package->Manifest.Name;
                if (!viewer.Name.empty())
                {
                    label += " - ";
                    label += viewer.Name;
                }
                if (_stricmp(label.c_str(), viewerLabel) == 0)
                {
                    return ExecuteCommand(
                        package, NULL, "salamatrix.viewer",
                        viewer.Handler.c_str(), invocationJson);
                }
                continue;
            }
            for (size_t patternIndex = 0;
                 patternIndex < viewer.Patterns.size(); ++patternIndex)
            {
                std::string masks = viewer.Patterns[patternIndex];
                size_t start = 0;
                while (start <= masks.size())
                {
                    size_t end = masks.find(';', start);
                    if (end == std::string::npos)
                        end = masks.size();
                    const std::string mask = masks.substr(start, end - start);
                    if (!mask.empty() &&
                        General->AgreeMask(fileName, mask.c_str(), hasExtension))
                    {
                        return ExecuteCommand(
                            package, NULL, "salamatrix.viewer",
                            viewer.Handler.c_str(), invocationJson);
                    }
                    if (end == masks.size())
                        break;
                    start = end + 1;
                }
            }
        }
    }
    return FALSE;
}

BOOL PackageManager::ListFileSystem(
    const std::string& packageId,
    const std::string& fileSystemId,
    const char* invocationJson,
    std::vector<FileSystemItem>* items,
    unsigned int* refreshIntervalMs)
{
    if (items == NULL)
        return FALSE;
    ExecutionGuard execution(this);
    items->clear();
    for (size_t packageIndex = 0; packageIndex < Packages.size(); ++packageIndex)
    {
        Package* package = Packages[packageIndex];
        if (package == NULL || !package->RuntimeUsable ||
            _stricmp(package->Id.c_str(), packageId.c_str()) != 0)
            continue;
        ScopedExclusiveSRWLock fileSystemExecution(
            &package->FileSystemExecutionLock);
        for (size_t fileSystemIndex = 0;
             fileSystemIndex < package->Manifest.FileSystems.size(); ++fileSystemIndex)
        {
            const CExtensionManifestFileSystem& fileSystem =
                package->Manifest.FileSystems[fileSystemIndex];
            if (_stricmp(fileSystem.Id.c_str(), fileSystemId.c_str()) != 0)
                continue;
            package->PendingFileSystemItems.clear();
            package->FileSystemListing = TRUE;
            package->ListingFileSystem = &fileSystem;
            const BOOL executed = ExecuteCommand(
                package, NULL, "salamatrix.fileSystem.list",
                fileSystem.ListHandler.c_str(), invocationJson);
            package->FileSystemListing = FALSE;
            package->ListingFileSystem = NULL;
            if (!executed)
            {
                package->PendingFileSystemItems.clear();
                return FALSE;
            }
            for (size_t itemIndex = 0;
                 itemIndex < package->PendingFileSystemItems.size(); ++itemIndex)
            {
                FileSystemItem item = package->PendingFileSystemItems[itemIndex];
                std::wstring relative;
                if (!item.Icon.empty() && ToWide(item.Icon, &relative))
                    ToUtf8(package->Directory + L"\\" + relative, &item.Icon);
                if (!item.IconDark.empty() && ToWide(item.IconDark, &relative))
                    ToUtf8(package->Directory + L"\\" + relative, &item.IconDark);
                items->push_back(item);
            }
            package->PendingFileSystemItems.clear();
            if (refreshIntervalMs != NULL)
                *refreshIntervalMs = fileSystem.RefreshIntervalMs;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL PackageManager::ExecuteFileSystemAction(
    const std::string& packageId,
    const std::string& fileSystemId,
    const std::string& actionId,
    const char* invocationJson)
{
    if (InterlockedCompareExchange(&ShuttingDown, FALSE, FALSE) != FALSE)
        return FALSE;
    // A non-refreshing action is normally a Properties-style modal command.
    // Starting and pumping its one-shot runtime inside the panel callback
    // prevents the main thread from dispatching ordinary window messages.
    // Queue it and let HostDispatch marshal native UI calls back to the main
    // thread only when the worker is ready to display the dialog.
    for (size_t packageIndex = 0; packageIndex < Packages.size(); ++packageIndex)
    {
        Package* package = Packages[packageIndex];
        if (package == NULL || !package->RuntimeUsable ||
            _stricmp(package->Id.c_str(), packageId.c_str()) != 0)
            continue;
        for (size_t fileSystemIndex = 0;
             fileSystemIndex < package->Manifest.FileSystems.size(); ++fileSystemIndex)
        {
            const CExtensionManifestFileSystem& fileSystem =
                package->Manifest.FileSystems[fileSystemIndex];
            if (_stricmp(fileSystem.Id.c_str(), fileSystemId.c_str()) != 0)
                continue;
            for (size_t actionIndex = 0;
                 actionIndex < fileSystem.Actions.size(); ++actionIndex)
            {
                const CExtensionManifestFileSystem::Action& action =
                    fileSystem.Actions[actionIndex];
                if (_stricmp(action.Id.c_str(), actionId.c_str()) == 0 &&
                    !action.Refresh)
                {
                    return QueueFileSystemAction(
                        packageId, fileSystemId, actionId, invocationJson);
                }
            }
        }
    }
    ExecutionGuard execution(this);
    return ExecuteFileSystemActionNow(
        packageId, fileSystemId, actionId, invocationJson);
}

BOOL PackageManager::ExecuteFileSystemActionNow(
    const std::string& packageId,
    const std::string& fileSystemId,
    const std::string& actionId,
    const char* invocationJson)
{
    // Actions must not hold the listing lock while they synchronously marshal
    // a modal dialog to the UI thread. A panel reload can otherwise block the
    // UI on FileSystemExecutionLock while this worker is blocked waiting for
    // that same UI thread: a classic lock inversion. Actions are serialized
    // independently; listings retain their original lock and stay responsive.
    for (size_t packageIndex = 0; packageIndex < Packages.size(); ++packageIndex)
    {
        Package* package = Packages[packageIndex];
        if (package == NULL || !package->RuntimeUsable ||
            _stricmp(package->Id.c_str(), packageId.c_str()) != 0)
            continue;
        ScopedExclusiveSRWLock actionExecution(
            &package->FileSystemActionExecutionLock);
        for (size_t fileSystemIndex = 0;
             fileSystemIndex < package->Manifest.FileSystems.size(); ++fileSystemIndex)
        {
            const CExtensionManifestFileSystem& fileSystem =
                package->Manifest.FileSystems[fileSystemIndex];
            if (_stricmp(fileSystem.Id.c_str(), fileSystemId.c_str()) != 0)
                continue;
            for (size_t actionIndex = 0;
                 actionIndex < fileSystem.Actions.size(); ++actionIndex)
            {
                const CExtensionManifestFileSystem::Action& action =
                    fileSystem.Actions[actionIndex];
                if (_stricmp(action.Id.c_str(), actionId.c_str()) == 0)
                    return ExecuteCommand(
                        package, NULL, action.Id.c_str(),
                        action.Handler.c_str(), invocationJson);
            }
            if (!fileSystem.OpenHandler.empty() &&
                (_stricmp(actionId.c_str(), "open") == 0 || actionId.empty()))
                return ExecuteCommand(
                    package, NULL, "salamatrix.fileSystem.open",
                    fileSystem.OpenHandler.c_str(), invocationJson);
        }
    }
    return FALSE;
}

BOOL PackageManager::QueueFileSystemAction(
    const std::string& packageId,
    const std::string& fileSystemId,
    const std::string& actionId,
    const char* invocationJson)
{
    Package* package = NULL;
    for (size_t index = 0; index < Packages.size(); ++index)
    {
        if (Packages[index] != NULL &&
            _stricmp(Packages[index]->Id.c_str(), packageId.c_str()) == 0)
        {
            package = Packages[index];
            break;
        }
    }
    if (package == NULL)
        return FALSE;
    // Ignore repeated double-click/context invocations while the same package
    // is already starting or showing a non-refreshing modal action. This keeps
    // impatient retries from building a queue of dialogs behind the package
    // execution lock.
    LONG generation = InterlockedIncrement(&package->FileSystemActionGeneration);
    if (generation == 0)
        generation = InterlockedIncrement(&package->FileSystemActionGeneration);
    if (InterlockedCompareExchange(
            &package->FileSystemActionPending, generation, 0) != 0)
        return TRUE;
#ifdef new
#undef new
#define RESTORE_SALAMATRIX_FS_ACTION_DEBUG_NEW_MACRO
#endif
    FileSystemActionTask* task = new (std::nothrow) FileSystemActionTask;
#ifdef RESTORE_SALAMATRIX_FS_ACTION_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_SALAMATRIX_FS_ACTION_DEBUG_NEW_MACRO
#endif
    if (task == NULL)
    {
        InterlockedCompareExchange(
            &package->FileSystemActionPending, 0, generation);
        return FALSE;
    }
    task->Owner = this;
    task->PackageContext = package;
    task->Generation = generation;
    task->PackageId = packageId;
    task->FileSystemId = fileSystemId;
    task->ActionId = actionId;
    task->InvocationJson = invocationJson != NULL ? invocationJson : "{}";
    BeginExecution();
    if (InterlockedCompareExchange(&ShuttingDown, FALSE, FALSE) != FALSE)
    {
        InterlockedCompareExchange(
            &package->FileSystemActionPending, 0, generation);
        FinishExecution();
        delete task;
        return FALSE;
    }
    HANDLE thread = CreateThread(
        NULL, 0, FileSystemActionThreadProc, task, 0, NULL);
    if (thread == NULL)
    {
        InterlockedCompareExchange(
            &package->FileSystemActionPending, 0, generation);
        FinishExecution();
        delete task;
        return FALSE;
    }
    EnterCriticalSection(&FileSystemActionThreadsLock);
    for (size_t index = FileSystemActionThreads.size(); index > 0; --index)
    {
        if (WaitForSingleObject(FileSystemActionThreads[index - 1], 0) ==
            WAIT_OBJECT_0)
        {
            CloseHandle(FileSystemActionThreads[index - 1]);
            FileSystemActionThreads.erase(
                FileSystemActionThreads.begin() + index - 1);
        }
    }
    FileSystemActionThreads.push_back(thread);
    LeaveCriticalSection(&FileSystemActionThreadsLock);
    return TRUE;
}

DWORD WINAPI PackageManager::FileSystemActionThreadProc(void* context)
{
    FileSystemActionTask* task = static_cast<FileSystemActionTask*>(context);
    if (task == NULL || task->Owner == NULL)
    {
        delete task;
        return 0;
    }
    PackageManager* owner = task->Owner;
    Package* package = task->PackageContext;
    if (InterlockedCompareExchange(&owner->ShuttingDown, FALSE, FALSE) == FALSE)
    {
        owner->ExecuteFileSystemActionNow(
            task->PackageId, task->FileSystemId,
            task->ActionId, task->InvocationJson.c_str());
    }
    if (package != NULL)
        InterlockedCompareExchange(
            &package->FileSystemActionPending, 0, task->Generation);
    delete task;
    owner->FinishExecution();
    return 0;
}

BOOL WINAPI PackageManager::ManagementCallback(
    void* context,
    Extensions::ExtensionManagementAction action,
    const char* extensionId,
    const wchar_t* manifestPath,
    int moveDelta)
{
    PackageManager* manager = static_cast<PackageManager*>(context);
    if (manager == NULL)
        return FALSE;
    switch (action)
    {
    case Extensions::ExtensionManagementInstallManifest:
        return manager->InstallManifest(manifestPath);
    case Extensions::ExtensionManagementRemove:
        return manager->RemoveExtension(extensionId);
    case Extensions::ExtensionManagementMove:
        return manager->MoveExtension(extensionId, moveDelta);
    }
    return FALSE;
}

bool PackageManager::IsRemoved(const std::string& extensionId) const
{
    for (size_t index = 0; index < RemovedExtensions.size(); ++index)
        if (_stricmp(
                RemovedExtensions[index].c_str(), extensionId.c_str()) == 0)
            return true;
    return false;
}

void PackageManager::ApplyUserOrder()
{
    for (size_t packageIndex = 0;
         packageIndex < Packages.size(); ++packageIndex)
    {
        const std::string& id = Packages[packageIndex]->Id;
        bool found = false;
        for (size_t orderIndex = 0;
             orderIndex < ExtensionOrder.size(); ++orderIndex)
        {
            if (_stricmp(ExtensionOrder[orderIndex].c_str(), id.c_str()) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
            ExtensionOrder.push_back(id);
    }

    std::stable_sort(
        Packages.begin(), Packages.end(),
        [this](const Package* left, const Package* right) {
            size_t leftOrder = ExtensionOrder.size();
            size_t rightOrder = ExtensionOrder.size();
            for (size_t index = 0; index < ExtensionOrder.size(); ++index)
            {
                if (_stricmp(
                        ExtensionOrder[index].c_str(), left->Id.c_str()) == 0)
                    leftOrder = index;
                if (_stricmp(
                        ExtensionOrder[index].c_str(), right->Id.c_str()) == 0)
                    rightOrder = index;
            }
            return leftOrder < rightOrder;
        });

    if (Extensions != NULL && !Packages.empty())
    {
        std::vector<const char*> ids;
        for (size_t index = 0; index < Packages.size(); ++index)
            ids.push_back(Packages[index]->Id.c_str());
        Extensions->ApplyExtensionOrder(
            &ids[0], static_cast<int>(ids.size()));
    }
}

BOOL PackageManager::InstallManifest(const wchar_t* manifestPath)
{
    if (manifestPath == NULL || manifestPath[0] == 0)
        return FALSE;
    wchar_t absolute[SAL_MAX_PATH];
    wchar_t* filePart = NULL;
    DWORD length = GetFullPathNameW(
        manifestPath, _countof(absolute), absolute, &filePart);
    if (length == 0 || length >= _countof(absolute) || filePart == NULL ||
        _wcsicmp(filePart, L"extension.json") != 0)
        return FALSE;

    std::string json;
    if (!ReadUtf8File(absolute, &json))
        return FALSE;
    CExtensionManifest manifest;
    CExtensionManifestError error;
    if (!manifest.Parse(json.data(), json.size(), error) ||
        !CExtensionManifest::IsSafeRelativeEntryPoint(manifest.EntryPoint))
        return FALSE;

    std::wstring packageDirectory(absolute);
    size_t slash = packageDirectory.find_last_of(L"\\/");
    if (slash == std::wstring::npos)
        return FALSE;
    packageDirectory.erase(slash);
    bool packageFound = false;
    for (size_t index = 0; index < CustomPackages.size(); ++index)
        if (_wcsicmp(
                CustomPackages[index].c_str(), packageDirectory.c_str()) == 0)
            packageFound = true;
    if (!packageFound)
        CustomPackages.push_back(packageDirectory);

    for (std::vector<std::string>::iterator item =
             RemovedExtensions.begin();
         item != RemovedExtensions.end();)
    {
        if (_stricmp(item->c_str(), manifest.Id.c_str()) == 0)
            item = RemovedExtensions.erase(item);
        else
            ++item;
    }
    Refresh();
    Extensions::ExtensionInfo installed;
    return Extensions != NULL &&
           Extensions->FindExtension(manifest.Id.c_str(), &installed) != FALSE;
}

BOOL PackageManager::RemoveExtension(const char* extensionId)
{
    if (extensionId == NULL || extensionId[0] == 0)
        return FALSE;
    bool managed = false;
    for (size_t index = 0; index < Packages.size(); ++index)
        if (_stricmp(Packages[index]->Id.c_str(), extensionId) == 0)
            managed = true;
    if (!managed)
        return FALSE;
    if (!IsRemoved(extensionId))
        RemovedExtensions.push_back(extensionId);
    Refresh();
    return TRUE;
}

BOOL PackageManager::MoveExtension(const char* extensionId, int delta)
{
    if (extensionId == NULL || (delta != -1 && delta != 1))
        return FALSE;
    size_t packageIndex = Packages.size();
    for (size_t index = 0; index < Packages.size(); ++index)
        if (_stricmp(Packages[index]->Id.c_str(), extensionId) == 0)
            packageIndex = index;
    if (packageIndex == Packages.size())
        return FALSE;
    const int target = static_cast<int>(packageIndex) + delta;
    if (target < 0 || target >= static_cast<int>(Packages.size()))
        return FALSE;

    UnregisterToolbarButtons();
    std::swap(Packages[packageIndex], Packages[target]);
    size_t firstOrder = ExtensionOrder.size();
    size_t secondOrder = ExtensionOrder.size();
    for (size_t index = 0; index < ExtensionOrder.size(); ++index)
    {
        if (_stricmp(
                ExtensionOrder[index].c_str(),
                Packages[packageIndex]->Id.c_str()) == 0)
            firstOrder = index;
        if (_stricmp(
                ExtensionOrder[index].c_str(),
                Packages[target]->Id.c_str()) == 0)
            secondOrder = index;
    }
    if (firstOrder < ExtensionOrder.size() &&
        secondOrder < ExtensionOrder.size())
        std::swap(ExtensionOrder[firstOrder], ExtensionOrder[secondOrder]);
    ApplyUserOrder();
    RegisterToolbarButtons();
    if (General != NULL)
        General->PostPluginMenuChanged();
    return TRUE;
}

BOOL WINAPI PackageManager::RuntimeDialogEventCallback(
    void* context, const UI::DialogEvent* event)
{
    Package::RuntimeDialog* binding =
        static_cast<Package::RuntimeDialog*>(context);
    if (binding == NULL || binding->Owner == NULL || event == NULL ||
        !binding->EventsEnabled || binding->EventName[0] == '\0' ||
        binding->Owner->Session == NULL)
        return FALSE;
    char dialogId[32];
    _ui64toa_s(binding->Id, dialogId, _countof(dialogId), 10);
    std::string eventJson =
        std::string("{\"event\":\"") + JsonEscape(binding->EventName) +
        "\",\"dialogId\":\"" + dialogId +
        "\",\"controlId\":\"" + JsonEscape(event->ControlId) +
        "\",\"kind\":" + std::to_string(static_cast<int>(event->Control)) +
        ",\"text\":\"" + JsonEscape(event->Text) +
        "\",\"checked\":" + (event->Checked ? "true" : "false") +
        ",\"selectedIndex\":" + std::to_string(event->SelectedIndex) + "}";
    std::string frame;
    if (!Runtime::Protocol::LineCodec::Encode(
            Runtime::Protocol::MessageEvent, 0, eventJson, &frame))
        return FALSE;
    return binding->Owner->Session->QueueFrame(
        frame.c_str(), static_cast<DWORD>(frame.size()));
}

BOOL WINAPI PackageManager::RuntimeEventCallback(
    void* context, const Events::EventPayload* event)
{
    Package* package = static_cast<Package*>(context);
    if (package == NULL || event == NULL || package->Session == NULL)
        return FALSE;
    const char* eventName = RuntimeEventName(event->Kind);
    if (eventName == NULL)
        return FALSE;
    const bool hasLifecycleFields =
        event->StructSize >= sizeof(Events::EventPayload);
    char tabId[32];
    char changedTabId[32];
    _ui64toa_s(event->ActiveTabId, tabId, _countof(tabId), 10);
    _ui64toa_s(
        hasLifecycleFields ? event->ChangedTabId : 0,
        changedTabId, _countof(changedTabId), 10);
    const std::string eventJson =
        std::string("{\"event\":\"") + eventName +
        "\",\"parameter\":" + std::to_string(event->Parameter) +
        ",\"activePanel\":" + std::to_string(event->ActivePanel) +
        ",\"tabId\":\"" + tabId +
        "\",\"changedTabId\":\"" + changedTabId +
        "\",\"tabIndex\":" +
        std::to_string(
            hasLifecycleFields ? event->ChangedTabIndex : -1) +
        ",\"previousTabIndex\":" +
        std::to_string(
            hasLifecycleFields ? event->PreviousTabIndex : -1) +
        ",\"pathType\":" + std::to_string(event->PathType) +
        ",\"path\":\"" + JsonEscape(event->Path) + "\"}";
    std::string frame;
    if (!Runtime::Protocol::LineCodec::Encode(
            Runtime::Protocol::MessageEvent, 0, eventJson, &frame))
        return FALSE;
    return package->Session->QueueFrame(
        frame.c_str(), static_cast<DWORD>(frame.size()));
}

BOOL WINAPI PackageManager::HostDispatch(
    void* context, Runtime::Protocol::MessageType type, ULONGLONG requestId,
    const char* payloadJson, char* resultJson, DWORD resultCapacity, DWORD* resultLength)
{
    UNREFERENCED_PARAMETER(requestId);
    if (context == NULL || payloadJson == NULL)
        return FALSE;
    Package* package = static_cast<Package*>(context);
    if (InterlockedCompareExchange(&package->Stopping, FALSE, FALSE) != FALSE)
        return CopyResult(
            "{\"ok\":false,\"error\":\"extension package is stopping\"}",
            resultJson, resultCapacity, resultLength);
    PackageManager* owner = package->Owner;
    if (type == Runtime::Protocol::MessageHello)
        return CopyResult(
            "{\"ok\":true,\"protocol\":1,\"services\":[\"commands\",\"fileOperations\",\"fileSystem\",\"sides\",\"storage\",\"ui\",\"events\",\"runtimes\"]}",
            resultJson, resultCapacity, resultLength);
    if (type != Runtime::Protocol::MessageCall)
        return FALSE;
    std::string method;
    if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "method", &method))
        return FALSE;
    // fileSystem.addItem only appends to the listing that is owned by the
    // current ListFileSystem call. Keep it on that worker thread: routing
    // every item through the UI thread both stalls the panel and can prevent
    // an asynchronous listing from completing while Salamander is inside a
    // plug-in callback. FileSystemExecutionLock serializes listings, so the
    // pending vector has a single writer for the whole operation.
    const BOOL backgroundFileSystemItem =
        (method == "salamander.fileSystem.addItem" ||
         method == "salamander.fileSystem.addItems") &&
        package->FileSystemListing;
    if (CurrentMainThreadDispatch == NULL && owner->General != NULL &&
        !backgroundFileSystemItem)
    {
        MainThreadDispatch call = {
            context, type, requestId, payloadJson,
            resultJson, resultCapacity, resultLength};
        // Modal host UI calls are allowed to wait for the user indefinitely.
        // A finite InvokeOnMainThread timeout would let this stack-backed call
        // return while the UI thread is still inside ShowModal/a picker, and
        // the eventual dialog close would resume through a dangling context.
        const BOOL interactiveModalCall =
            method == "salamander.ui.dialog.show" ||
            method == "salamander.ui.controls" ||
            method == "salamander.ui.fileProperties" ||
            method == "salamander.ui.messageBox" ||
            method == "salamander.ui.pickFile" ||
            method == "salamander.ui.pickFolder";
        return owner->General->InvokeOnMainThread(
            HostDispatchOnMainThread, &call,
            interactiveModalCall ? INFINITE : 120000);
    }
    const char* requiredCapability = RuntimeCapabilityForMethod(method);
    if (!ManifestAllowsCapability(package->Manifest, requiredCapability))
    {
        return CopyResult(
            std::string("{\"ok\":false,\"error\":\"capability denied\",\"code\":\"permission_denied\",\"capability\":\"") +
                JsonEscape(requiredCapability) + "\"}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.host.language")
    {
        WORD languageId = 0;
        const std::string locale =
            CurrentSalamanderLocale(owner->General, &languageId);
        return CopyResult(
            std::string("{\"ok\":true,\"locale\":\"") +
                JsonEscape(locale.c_str()) +
                "\",\"languageId\":" +
                std::to_string(static_cast<unsigned int>(languageId)) + "}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.host.uptime")
        return CopyResult(std::string("{\"ok\":true,\"milliseconds\":\"") +
                              std::to_string(static_cast<unsigned long long>(GetTickCount64())) + "\"}",
                          resultJson, resultCapacity, resultLength);
    if (method == "salamander.host.windowIcon")
    {
        HICON icon = NULL;
        bool destroyIcon = false;
        const bool useDarkIcon =
            DarkModeIsWindowsDarkSchemeSelected() &&
            !package->IconDarkPath.empty();
        const char* preferredPath =
            useDarkIcon ? package->IconDarkPath.c_str()
                        : package->IconPath.c_str();
        if (SalamanderGUI != NULL && preferredPath[0] != '\0')
        {
            icon = SalamanderGUI->CreateSVGIcon(preferredPath, 32);
            if (icon == NULL && useDarkIcon && !package->IconPath.empty())
                icon = SalamanderGUI->CreateSVGIcon(
                    package->IconPath.c_str(), 32);
            destroyIcon = icon != NULL;
        }

        if (icon == NULL && owner->General != NULL)
        {
            HWND mainWindow = owner->General->GetMainWindowHWND();
            HICON mainIcon = mainWindow != NULL
                                 ? reinterpret_cast<HICON>(SendMessage(
                                       mainWindow, WM_GETICON, ICON_BIG, 0))
                                 : NULL;
            if (mainIcon == NULL && mainWindow != NULL)
                mainIcon = reinterpret_cast<HICON>(SendMessage(
                    mainWindow, WM_GETICON, ICON_SMALL2, 0));
            if (mainIcon == NULL && mainWindow != NULL)
                mainIcon = reinterpret_cast<HICON>(GetClassLongPtr(
                    mainWindow, GCLP_HICON));
            if (mainIcon == NULL && mainWindow != NULL)
                mainIcon = reinterpret_cast<HICON>(GetClassLongPtr(
                    mainWindow, GCLP_HICONSM));
            if (mainIcon != NULL)
            {
                icon = CopyIcon(mainIcon);
                destroyIcon = icon != NULL;
                if (icon == NULL)
                    icon = mainIcon;
            }
        }

        const std::string encodedIcon = SerializeWindowIcon(icon);
        if (destroyIcon)
            DestroyIcon(icon);
        return CopyResult(
            std::string("{\"ok\":true,\"icon\":\"") +
                encodedIcon + "\"}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.host.appearance")
    {
        return CopyResult(
            DarkModeIsWindowsDarkSchemeSelected()
                ? "{\"ok\":true,\"windowsDarkMode\":true}"
                : "{\"ok\":true,\"windowsDarkMode\":false}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.runtimes.list")
    {
        if (owner->Runtimes == NULL)
            return FALSE;
        std::string response = "{\"ok\":true,\"runtimes\":[";
        for (int index = 0; index < owner->Runtimes->GetAdapterCount(); ++index)
        {
            Runtime::IRuntimeAdapter* adapter =
                owner->Runtimes->GetAdapter(index);
            const Runtime::RuntimeAdapterDescriptor* descriptor =
                adapter != NULL ? adapter->GetDescriptor() : NULL;
            if (descriptor == NULL || descriptor->RuntimeId == NULL)
                continue;
            if (response[response.size() - 1] != '[')
                response += ",";
            response += std::string("{\"id\":\"") +
                        JsonEscape(descriptor->RuntimeId) +
                        "\",\"name\":\"" +
                        JsonEscape(descriptor->DisplayName) +
                        "\",\"language\":\"" +
                        JsonEscape(descriptor->LanguageId) +
                        "\",\"extensions\":\"" +
                        JsonEscape(descriptor->FileExtensions) +
                        "\",\"version\":" +
                        std::to_string(descriptor->RuntimeVersion) +
                        ",\"available\":" +
                        (adapter->IsAvailable() ? "true}" : "false}");
        }
        response += "]}";
        return CopyResult(
            response, resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.events.subscribe")
    {
        std::string eventName;
        Events::EventKind kind;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "event", &eventName) ||
            !RuntimeEventKindFromName(eventName, &kind) ||
            !ManifestAllowsEvent(package->Manifest, eventName) ||
            package->EventSubscriptions.size() >= 32 ||
            owner->Events == NULL)
            return FALSE;
        ULONGLONG subscriptionId = 0;
        if (!owner->Events->Subscribe(
                kind, RuntimeEventCallback, package, &subscriptionId))
            return FALSE;
        package->EventSubscriptions.push_back(subscriptionId);
        char id[32];
        _ui64toa_s(subscriptionId, id, _countof(id), 10);
        return CopyResult(
            std::string("{\"ok\":true,\"subscriptionId\":\"") +
                id + "\"}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.fileSystem.addItem" ||
        method == "salamander.fileSystem.addItems")
    {
        if (!package->FileSystemListing)
            return CopyResult(
                "{\"ok\":false,\"error\":\"No file-system listing is active\",\"code\":\"invalid_state\"}",
                resultJson, resultCapacity, resultLength);
        std::vector<std::string> itemPayloads;
        if (method == "salamander.fileSystem.addItems")
        {
            std::string itemsJson;
            if (!Runtime::Protocol::Json::FindRawMember(
                    payloadJson, "items", &itemsJson))
                return CopyResult(
                    "{\"ok\":false,\"error\":\"File-system item batch requires an items array\",\"code\":\"invalid_argument\"}",
                    resultJson, resultCapacity, resultLength);
            size_t position = 0;
            Runtime::Protocol::Json::SkipWhitespace(itemsJson, &position);
            if (position >= itemsJson.size() || itemsJson[position] != '[')
                return CopyResult(
                    "{\"ok\":false,\"error\":\"File-system item batch requires an items array\",\"code\":\"invalid_argument\"}",
                    resultJson, resultCapacity, resultLength);
            ++position;
            Runtime::Protocol::Json::SkipWhitespace(itemsJson, &position);
            while (position < itemsJson.size() && itemsJson[position] != ']')
            {
                if (itemsJson[position] != '{')
                    return CopyResult(
                        "{\"ok\":false,\"error\":\"File-system item batch entries must be objects\",\"code\":\"invalid_argument\"}",
                        resultJson, resultCapacity, resultLength);
                const size_t itemStart = position;
                if (!Runtime::Protocol::Json::SkipValue(itemsJson, &position))
                    return CopyResult(
                        "{\"ok\":false,\"error\":\"File-system item batch contains invalid JSON\",\"code\":\"invalid_argument\"}",
                        resultJson, resultCapacity, resultLength);
                itemPayloads.push_back(itemsJson.substr(itemStart, position - itemStart));
                Runtime::Protocol::Json::SkipWhitespace(itemsJson, &position);
                if (position >= itemsJson.size() ||
                    (itemsJson[position] != ',' && itemsJson[position] != ']'))
                    return CopyResult(
                        "{\"ok\":false,\"error\":\"File-system item batch contains invalid JSON\",\"code\":\"invalid_argument\"}",
                        resultJson, resultCapacity, resultLength);
                if (itemsJson[position] == ',')
                {
                    ++position;
                    Runtime::Protocol::Json::SkipWhitespace(itemsJson, &position);
                }
            }
            if (position >= itemsJson.size() || itemsJson[position] != ']')
                return CopyResult(
                    "{\"ok\":false,\"error\":\"File-system item batch contains invalid JSON\",\"code\":\"invalid_argument\"}",
                    resultJson, resultCapacity, resultLength);
            ++position;
            Runtime::Protocol::Json::SkipWhitespace(itemsJson, &position);
            if (position != itemsJson.size())
                return CopyResult(
                    "{\"ok\":false,\"error\":\"File-system item batch contains invalid JSON\",\"code\":\"invalid_argument\"}",
                    resultJson, resultCapacity, resultLength);
        }
        else
            itemPayloads.push_back(payloadJson);
        if (package->PendingFileSystemItems.size() + itemPayloads.size() > 4096)
            return CopyResult(
                "{\"ok\":false,\"error\":\"File-system listing contains more than 4096 items\",\"code\":\"limit_exceeded\"}",
                resultJson, resultCapacity, resultLength);
        const size_t originalCount = package->PendingFileSystemItems.size();
        for (size_t itemIndex = 0; itemIndex < itemPayloads.size(); ++itemIndex)
        {
            const char* itemJson = itemPayloads[itemIndex].c_str();
            FileSystemItem item;
            const char* error = NULL;
            if (!Runtime::Protocol::Json::FindStringMember(itemJson, "id", &item.Id) ||
                !Runtime::Protocol::Json::FindStringMember(itemJson, "name", &item.Name))
                error = "{\"ok\":false,\"error\":\"File-system item requires id and name\",\"code\":\"invalid_argument\"}";
            Runtime::Protocol::Json::FindStringMember(itemJson, "icon", &item.Icon);
            Runtime::Protocol::Json::FindStringMember(itemJson, "iconDark", &item.IconDark);
            Runtime::Protocol::Json::FindStringMember(itemJson, "fileIcon", &item.FileIcon);
            Runtime::Protocol::Json::FindStringMember(itemJson, "compactName", &item.CompactName);
            BOOL boolValue = FALSE;
            if (Runtime::Protocol::Json::FindBoolMember(itemJson, "directory", &boolValue))
                item.Directory = boolValue != FALSE;
            boolValue = TRUE;
            if (Runtime::Protocol::Json::FindBoolMember(itemJson, "enabled", &boolValue))
                item.Enabled = boolValue != FALSE;
            if (error == NULL &&
                (item.Id.empty() || item.Id.size() > 255 || item.Name.empty() ||
                 item.Name.size() > 1023 || item.Name == "." || item.Name == ".." ||
                 item.Name.find('\\') != std::string::npos ||
                 item.Name.find('/') != std::string::npos))
                error = "{\"ok\":false,\"error\":\"File-system item id or name is invalid\",\"code\":\"invalid_argument\"}";
            if (error == NULL &&
                (item.CompactName.size() > 1023 ||
                 item.CompactName.find('\\') != std::string::npos ||
                 item.CompactName.find('/') != std::string::npos))
                error = "{\"ok\":false,\"error\":\"File-system item compact name is invalid\",\"code\":\"invalid_argument\"}";
            const std::string svgExtension = ".svg";
            const bool iconIsSvg = item.Icon.size() >= svgExtension.size() &&
                _stricmp(item.Icon.c_str() + item.Icon.size() - svgExtension.size(), svgExtension.c_str()) == 0;
            const bool darkIconIsSvg = item.IconDark.size() >= svgExtension.size() &&
                _stricmp(item.IconDark.c_str() + item.IconDark.size() - svgExtension.size(), svgExtension.c_str()) == 0;
            if (error == NULL &&
                ((!item.Icon.empty() &&
                  (!CExtensionManifest::IsSafeRelativeEntryPoint(item.Icon) || !iconIsSvg)) ||
                 (!item.IconDark.empty() &&
                  (!CExtensionManifest::IsSafeRelativeEntryPoint(item.IconDark) || !darkIconIsSvg))))
                error = "{\"ok\":false,\"error\":\"File-system item icons must be safe relative SVG paths\",\"code\":\"invalid_argument\"}";
            if (error == NULL && item.FileIcon.size() > 131068)
                error = "{\"ok\":false,\"error\":\"File-system item icon file path is too long\",\"code\":\"invalid_argument\"}";
            std::string columnValues;
            if (Runtime::Protocol::Json::FindRawMember(itemJson, "columns", &columnValues) &&
                (package->ListingFileSystem == NULL || columnValues.empty() || columnValues[0] != '{'))
                error = "{\"ok\":false,\"error\":\"File-system item columns require declared provider columns\",\"code\":\"invalid_argument\"}";
            if (error == NULL && package->ListingFileSystem != NULL)
            {
                for (size_t columnIndex = 0;
                     columnIndex < package->ListingFileSystem->Columns.size(); ++columnIndex)
                {
                    std::string value;
                    if (!columnValues.empty())
                        Runtime::Protocol::Json::FindStringMember(
                            columnValues.c_str(),
                            package->ListingFileSystem->Columns[columnIndex].Id.c_str(),
                            &value);
                    if (value.size() > 1023)
                    {
                        error = "{\"ok\":false,\"error\":\"File-system column value is longer than 1023 bytes\",\"code\":\"invalid_argument\"}";
                        break;
                    }
                    item.ColumnValues.push_back(value);
                }
            }
            if (error != NULL)
            {
                package->PendingFileSystemItems.resize(originalCount);
                return CopyResult(error, resultJson, resultCapacity, resultLength);
            }
            package->PendingFileSystemItems.push_back(item);
        }
        char addedCount[32];
        _ui64toa_s(itemPayloads.size(), addedCount, _countof(addedCount), 10);
        return CopyResult(
            std::string("{\"ok\":true,\"added\":true,\"addedCount\":") +
                addedCount + "}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.events.unsubscribe")
    {
        std::string idText;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "subscriptionId", &idText) ||
            owner->Events == NULL)
            return FALSE;
        char* end = NULL;
        const ULONGLONG subscriptionId =
            _strtoui64(idText.c_str(), &end, 10);
        if (end == idText.c_str() || *end != '\0')
            return FALSE;
        size_t found = package->EventSubscriptions.size();
        for (size_t index = 0;
             index < package->EventSubscriptions.size(); ++index)
            if (package->EventSubscriptions[index] == subscriptionId)
                found = index;
        if (found == package->EventSubscriptions.size() ||
            !owner->Events->Unsubscribe(subscriptionId))
            return FALSE;
        package->EventSubscriptions.erase(
            package->EventSubscriptions.begin() + found);
        return CopyResult(
            "{\"ok\":true}", resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.commands.execute")
    {
        std::string commandId;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "commandId", &commandId) ||
            owner->Commands == NULL)
            return FALSE;
        Commands::ExecuteOptions options;
        options.Parent = owner->General->GetMsgBoxParent();
        const Runtime::OperationResult operation =
            owner->Commands->Execute(commandId.c_str(), options);
        const char* resultName =
            operation == Runtime::OperationResultOk
                ? "ok"
                : operation == Runtime::OperationResultCancel
                      ? "cancel"
                      : operation == Runtime::OperationResultNotAvailable
                            ? "not_available"
                            : "error";
        return CopyResult(
            std::string("{\"ok\":") +
                (operation == Runtime::OperationResultOk ? "true" : "false") +
                ",\"result\":\"" + resultName + "\"}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.commands.register")
    {
        std::string commandId;
        std::string title;
        std::string handler;
        BOOL pluginMenu = TRUE;
        BOOL contextMenu = FALSE;
        BOOL toolbar = FALSE;
        BOOL enabled = TRUE;
        BOOL visible = TRUE;
        int hotKey = 0;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "commandId", &commandId) ||
            commandId.empty() || commandId.size() > 127)
            return FALSE;
        Runtime::Protocol::Json::FindStringMember(
            payloadJson, "title", &title);
        Runtime::Protocol::Json::FindStringMember(
            payloadJson, "handler", &handler);
        Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "pluginMenu", &pluginMenu);
        Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "contextMenu", &contextMenu);
        Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "toolbar", &toolbar);
        Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "enabled", &enabled);
        Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "visible", &visible);
        Runtime::Protocol::Json::FindIntegerMember(
            payloadJson, "hotKey", &hotKey);
        if (title.empty())
            title = commandId;

        size_t commandIndex = package->Manifest.Commands.size();
        for (size_t index = 0;
             index < package->Manifest.Commands.size(); ++index)
        {
            if (_stricmp(
                    package->Manifest.Commands[index].Id.c_str(),
                    commandId.c_str()) == 0)
            {
                commandIndex = index;
                break;
            }
        }
        const bool adding = commandIndex == package->Manifest.Commands.size();
        if (adding && package->Manifest.Commands.size() >= 64)
            return FALSE;

        CExtensionManifestCommand command;
        command.Id = commandId;
        command.Title = title;
        command.Handler = handler;
        command.Menu = pluginMenu
                           ? (contextMenu ? "both" : "plugin")
                           : (contextMenu ? "context" : "none");
        command.ContextMenu = contextMenu != FALSE;
        command.Toolbar = toolbar != FALSE;
        command.Enabled = enabled != FALSE;
        command.Visible = visible != FALSE;

        owner->UnregisterToolbarButtons();
        if (adding)
        {
            size_t packageIndex = 0;
            while (packageIndex < owner->Packages.size() &&
                   owner->Packages[packageIndex] != package)
                ++packageIndex;
            if (packageIndex == owner->Packages.size())
                return FALSE;
            package->Manifest.Commands.push_back(command);
            package->CommandIds.push_back(
                0x62000000 +
                static_cast<int>(packageIndex * 64 + commandIndex + 1));
            package->CommandHotKeys.push_back(static_cast<DWORD>(hotKey));
            package->CommandIconPaths.push_back(package->IconPath);
            package->CommandIconDarkPaths.push_back(package->IconDarkPath);
        }
        else
        {
            package->Manifest.Commands[commandIndex] = command;
            if (commandIndex < package->CommandHotKeys.size())
                package->CommandHotKeys[commandIndex] =
                    static_cast<DWORD>(hotKey);
        }
        package->CommandsChanged = TRUE;
        owner->RefreshContributionFlags(package);
        owner->RegisterToolbarButtons();
        owner->General->PostPluginMenuChanged();
        return CopyResult(
            "{\"ok\":true,\"registered\":true}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.commands.unregister")
    {
        std::string commandId;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "commandId", &commandId))
            return FALSE;
        size_t commandIndex = package->Manifest.Commands.size();
        for (size_t index = package->InitialCommands.size();
             index < package->Manifest.Commands.size(); ++index)
        {
            if (_stricmp(
                    package->Manifest.Commands[index].Id.c_str(),
                    commandId.c_str()) == 0)
            {
                commandIndex = index;
                break;
            }
        }
        if (commandIndex == package->Manifest.Commands.size())
            return CopyResult(
                "{\"ok\":true,\"unregistered\":false}",
                resultJson, resultCapacity, resultLength);
        owner->UnregisterToolbarButtons();
        package->Manifest.Commands.erase(
            package->Manifest.Commands.begin() + commandIndex);
        package->CommandIds.erase(
            package->CommandIds.begin() + commandIndex);
        package->CommandHotKeys.erase(
            package->CommandHotKeys.begin() + commandIndex);
        package->CommandIconPaths.erase(
            package->CommandIconPaths.begin() + commandIndex);
        package->CommandIconDarkPaths.erase(
            package->CommandIconDarkPaths.begin() + commandIndex);
        package->CommandsChanged = TRUE;
        owner->RefreshContributionFlags(package);
        owner->RegisterToolbarButtons();
        owner->General->PostPluginMenuChanged();
        return CopyResult(
            "{\"ok\":true,\"unregistered\":true}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.commands.setState")
    {
        std::string commandId;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "commandId", &commandId))
            return FALSE;
        size_t commandIndex = package->Manifest.Commands.size();
        for (size_t index = 0;
             index < package->Manifest.Commands.size(); ++index)
        {
            if (_stricmp(
                    package->Manifest.Commands[index].Id.c_str(),
                    commandId.c_str()) == 0)
            {
                commandIndex = index;
                break;
            }
        }
        if (commandIndex == package->Manifest.Commands.size())
            return CopyResult(
                "{\"ok\":true,\"updated\":false}",
                resultJson, resultCapacity, resultLength);
        BOOL enabled = package->Manifest.Commands[commandIndex].Enabled;
        BOOL visible = package->Manifest.Commands[commandIndex].Visible;
        Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "enabled", &enabled);
        Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "visible", &visible);
        const bool visibilityChanged =
            package->Manifest.Commands[commandIndex].Visible !=
            (visible != FALSE);
        package->Manifest.Commands[commandIndex].Enabled = enabled != FALSE;
        package->Manifest.Commands[commandIndex].Visible = visible != FALSE;
        package->CommandsChanged = TRUE;
        if (visibilityChanged)
        {
            owner->UnregisterToolbarButtons();
            owner->RegisterToolbarButtons();
        }
        owner->General->PostPluginMenuChanged();
        return CopyResult(
            "{\"ok\":true,\"updated\":true}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.fileOperations.rename" ||
        method == "salamander.fileOperations.copy" ||
        method == "salamander.fileOperations.move" ||
        method == "salamander.fileOperations.delete" ||
        method == "salamander.fileOperations.createDirectory" ||
        method == "salamander.fileOperations.refresh" ||
        method == "salamander.fileOperations.properties")
    {
        if (owner->FileOperations == NULL)
            return FALSE;
        FileOperations::InteractiveOptions options;
        options.Parent = owner->General->GetMsgBoxParent();
        std::string targetHint;
        Runtime::Protocol::Json::FindStringMember(
            payloadJson, "target", &targetHint);
        options.TargetHint = targetHint.empty() ? NULL : targetHint.c_str();
        Runtime::OperationResult operation = Runtime::OperationResultError;
        if (method == "salamander.fileOperations.rename")
            operation = owner->FileOperations->RenameInteractive(options);
        else if (method == "salamander.fileOperations.copy")
            operation = owner->FileOperations->CopyInteractive(options);
        else if (method == "salamander.fileOperations.move")
            operation = owner->FileOperations->MoveInteractive(options);
        else if (method == "salamander.fileOperations.delete")
            operation = owner->FileOperations->DeleteInteractive(options);
        else if (method == "salamander.fileOperations.createDirectory")
            operation = owner->FileOperations->CreateDirectoryInteractive(options);
        else if (method == "salamander.fileOperations.refresh")
            operation = owner->FileOperations->Refresh(options);
        else
            operation = owner->FileOperations->ShowProperties(options);
        const char* resultName =
            operation == Runtime::OperationResultOk
                ? "ok"
                : operation == Runtime::OperationResultCancel
                      ? "cancel"
                      : operation == Runtime::OperationResultNotAvailable
                            ? "not_available"
                            : "error";
        return CopyResult(
            std::string("{\"ok\":") +
                (operation == Runtime::OperationResultOk ? "true" : "false") +
                ",\"result\":\"" + resultName + "\"}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.sides.activeTab" ||
        method == "salamander.sides.tabs")
    {
        if (owner->Sides == NULL)
            return FALSE;
        std::string sideName;
        Runtime::Protocol::Json::FindStringMember(
            payloadJson, "side", &sideName);
        const Sides::SideReference side = RuntimeSideFromName(sideName);
        if (method == "salamander.sides.activeTab")
        {
            Sides::TabInfo info;
            if (!owner->Sides->GetActiveTabInfo(side, &info))
                return CopyResult(
                    "{\"ok\":true,\"tab\":null}",
                    resultJson, resultCapacity, resultLength);
            char id[32];
            _ui64toa_s(info.TabId, id, _countof(id), 10);
            char path[SALAMATRIX_SIDE_ITEM_PATH_CAPACITY];
            path[0] = '\0';
            int pathType = info.PathType;
            owner->Sides->GetTabPath(
                info.TabId, path, _countof(path), &pathType);
            return CopyResult(
                std::string("{\"ok\":true,\"tab\":{\"id\":\"") + id +
                    "\",\"index\":" + std::to_string(info.Index) +
                    ",\"side\":" +
                    std::to_string(static_cast<int>(info.PhysicalSide)) +
                    ",\"pathType\":" + std::to_string(pathType) +
                    ",\"flags\":" + std::to_string(info.Flags) +
                    ",\"path\":\"" + JsonEscape(path) + "\"}}",
                resultJson, resultCapacity, resultLength);
        }

        int count = owner->Sides->GetTabCount(side);
        if (count < 0)
            count = 0;
        const int returnedCount = count > 128 ? 128 : count;
        std::string response = "{\"ok\":true,\"tabs\":[";
        for (int index = 0; index < returnedCount; ++index)
        {
            Sides::TabInfo info;
            if (!owner->Sides->GetTabInfo(side, index, &info))
                continue;
            char id[32];
            _ui64toa_s(info.TabId, id, _countof(id), 10);
            char path[SALAMATRIX_SIDE_ITEM_PATH_CAPACITY];
            path[0] = '\0';
            int pathType = info.PathType;
            owner->Sides->GetTabPath(
                info.TabId, path, _countof(path), &pathType);
            if (response[response.size() - 1] != '[')
                response += ",";
            response += std::string("{\"id\":\"") + id +
                        "\",\"index\":" + std::to_string(info.Index) +
                        ",\"side\":" +
                        std::to_string(static_cast<int>(info.PhysicalSide)) +
                        ",\"pathType\":" + std::to_string(pathType) +
                        ",\"flags\":" + std::to_string(info.Flags) +
                        ",\"path\":\"" + JsonEscape(path) + "\"}";
        }
        response += "]}";
        return CopyResult(
            response, resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.sides.activateTab")
    {
        CQuadWord tabId;
        BOOL focus = TRUE;
        if (!FindRuntimeQuadWord(payloadJson, "tabId", &tabId) ||
            tabId.Value == 0 || owner->Sides == NULL)
            return FALSE;
        Runtime::Protocol::Json::FindBoolMember(payloadJson, "focus", &focus);
        const BOOL activated = owner->Sides->ActivateTab(tabId.Value, focus);
        if (activated)
        {
            Sides::TabInfo tab;
            Sides::SideReference side = Sides::SideReferenceSource;
            if (owner->Sides->GetTabInfoById(tabId.Value, &tab))
                side = tab.PhysicalSide;
            Events::PublishSideOperation(
                owner->Events, owner->Sides,
                Events::EventKindSideTabChanged, side, 0);
        }
        return CopyResult(
            std::string("{\"ok\":") +
                (activated ? "true,\"activated\":true}" :
                             "false,\"activated\":false}"),
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.sides.changePath" ||
        method == "salamander.sides.refresh")
    {
        if (owner->Sides == NULL)
            return FALSE;
        std::string sideName;
        Runtime::Protocol::Json::FindStringMember(
            payloadJson, "side", &sideName);
        const Sides::SideReference side = RuntimeSideFromName(sideName);
        if (method == "salamander.sides.changePath")
        {
            std::string path;
            if (!Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "path", &path) || path.empty())
                return FALSE;
            int failReason = 0;
            const BOOL changed = owner->Sides->ChangeActiveTabPath(
                side, path.c_str(), &failReason);
            if (changed)
                Events::PublishSideOperation(
                    owner->Events, owner->Sides,
                    Events::EventKindSidePathChanged, side, 0);
            return CopyResult(
                std::string("{\"ok\":") +
                    (changed ? "true" : "false") +
                    ",\"changed\":" + (changed ? "true" : "false") +
                    ",\"failReason\":" + std::to_string(failReason) + "}",
                resultJson, resultCapacity, resultLength);
        }
        BOOL force = FALSE;
        BOOL focusFirst = FALSE;
        Runtime::Protocol::Json::FindBoolMember(payloadJson, "force", &force);
        Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "focusFirstNewItem", &focusFirst);
        const BOOL refreshed = owner->Sides->Refresh(side, force, focusFirst);
        if (refreshed)
            Events::PublishSideOperation(
                owner->Events, owner->Sides,
                Events::EventKindSideRefreshed, side, 0);
        return CopyResult(
            std::string("{\"ok\":") +
                (refreshed ? "true}" : "false}"),
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.sides.focusItem")
    {
        if (owner->Sides == NULL)
            return FALSE;
        std::string sideName;
        int index = -1;
        BOOL partVisible = TRUE;
        Runtime::Protocol::Json::FindStringMember(
            payloadJson, "side", &sideName);
        if (!Runtime::Protocol::Json::FindIntegerMember(
                payloadJson, "index", &index) || index < 0)
            return FALSE;
        Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "partVisible", &partVisible);
        const Sides::SideReference side = RuntimeSideFromName(sideName);
        const BOOL changed = owner->Sides->FocusItem(
            side, index, partVisible);
        if (changed)
            Events::PublishSideOperation(
                owner->Events, owner->Sides,
                Events::EventKindSideSelectionChanged, side,
                static_cast<DWORD>(index));
        return CopyResult(
            std::string("{\"ok\":") +
                (changed ? "true,\"changed\":true}" :
                           "false,\"changed\":false}"),
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.sides.closeTab" ||
        method == "salamander.sides.reorderTab" ||
        method == "salamander.sides.moveTab" ||
        method == "salamander.sides.setDetached")
    {
        if (owner->Sides == NULL)
            return FALSE;
        BOOL ok = FALSE;
        if (method == "salamander.sides.setDetached")
        {
            BOOL detached = FALSE;
            if (!Runtime::Protocol::Json::FindBoolMember(
                    payloadJson, "detached", &detached))
                return FALSE;
            ok = owner->Sides->SetPanelsDetached(detached);
            return CopyResult(
                std::string("{\"ok\":") + (ok ? "true" : "false") +
                    ",\"detached\":" +
                    (detached ? "true}" : "false}"),
                resultJson, resultCapacity, resultLength);
        }
        CQuadWord tabId;
        if (!FindRuntimeQuadWord(payloadJson, "tabId", &tabId) ||
            tabId.Value == 0)
            return FALSE;
        if (method == "salamander.sides.closeTab")
            ok = owner->Sides->CloseTab(tabId.Value);
        else if (method == "salamander.sides.reorderTab")
        {
            int index = -1;
            if (!Runtime::Protocol::Json::FindIntegerMember(
                    payloadJson, "index", &index) || index < 0)
                return FALSE;
            ok = owner->Sides->ReorderTab(tabId.Value, index);
        }
        else
        {
            std::string sideName;
            Sides::SideReference side;
            int index = -1;
            if (!Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "side", &sideName) ||
                !RuntimeTrySideFromName(sideName, &side))
                return FALSE;
            Runtime::Protocol::Json::FindIntegerMember(
                payloadJson, "index", &index);
            ok = owner->Sides->MoveTab(tabId.Value, side, index);
        }
        return CopyResult(
            std::string("{\"ok\":") + (ok ? "true}" : "false}"),
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.sides.context")
    {
        std::string sideName;
        Runtime::Protocol::Json::FindStringMember(
            payloadJson, "side", &sideName);
        Sides::SideReference side = Sides::SideReferenceSource;
        if (_stricmp(sideName.c_str(), "left") == 0)
            side = Sides::SideReferenceLeft;
        else if (_stricmp(sideName.c_str(), "right") == 0)
            side = Sides::SideReferenceRight;
        else if (_stricmp(sideName.c_str(), "target") == 0)
            side = Sides::SideReferenceTarget;
        if (owner->Sides == NULL)
            return FALSE;
        char path[SALAMATRIX_SIDE_ITEM_PATH_CAPACITY];
        path[0] = '\0';
        int pathType = 0;
        if (!owner->Sides->GetPath(
                side, path, _countof(path), &pathType))
            return FALSE;

        const int selectedCount =
            owner->Sides->GetSelectedItemCount(side);
        const int returnedSelectedCount =
            selectedCount > 64 ? 64 : selectedCount;
        std::string selectedItems("[");
        for (int index = 0; index < returnedSelectedCount; ++index)
        {
            Sides::ItemInfo item;
            if (!owner->Sides->GetSelectedItem(side, index, &item))
                continue;
            if (selectedItems.size() > 1)
                selectedItems += ",";
            selectedItems += SideItemJson(item);
        }
        selectedItems += "]";

        Sides::ItemInfo focused;
        const BOOL hasFocused =
            owner->Sides->GetFocusedItem(side, &focused);
        return CopyResult(
            std::string("{\"ok\":true,\"path\":\"") +
                JsonEscape(path) +
                "\",\"pathType\":" + std::to_string(pathType) +
                ",\"selectedCount\":" +
                std::to_string(selectedCount) +
                ",\"selectedItemsTruncated\":" +
                (selectedCount > returnedSelectedCount ? "true" : "false") +
                ",\"selectedItems\":" + selectedItems +
                ",\"focusedItem\":" +
            (hasFocused ? SideItemJson(focused) : "null") + "}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.sides.selectAll" ||
        method == "salamander.sides.selectItem")
    {
        if (owner->Sides == NULL)
            return FALSE;

        std::string sideName;
        Runtime::Protocol::Json::FindStringMember(
            payloadJson, "side", &sideName);
        Sides::SideReference side = Sides::SideReferenceSource;
        if (_stricmp(sideName.c_str(), "left") == 0)
            side = Sides::SideReferenceLeft;
        else if (_stricmp(sideName.c_str(), "right") == 0)
            side = Sides::SideReferenceRight;
        else if (_stricmp(sideName.c_str(), "target") == 0)
            side = Sides::SideReferenceTarget;

        BOOL select = TRUE;
        BOOL repaint = TRUE;
        Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "select", &select);
        Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "repaint", &repaint);

        BOOL changed = FALSE;
        DWORD eventParameter = 0;
        if (method == "salamander.sides.selectAll")
        {
            changed = owner->Sides->SelectAll(side, select, repaint);
            eventParameter = select ? 1 : 0;
        }
        else
        {
            int index = -1;
            if (!Runtime::Protocol::Json::FindIntegerMember(
                    payloadJson, "index", &index) || index < 0)
                return FALSE;
            changed = owner->Sides->SetItemSelected(
                side, index, select, repaint);
            eventParameter = static_cast<DWORD>(index);
        }

        if (changed)
            Events::PublishSideOperation(
                owner->Events, owner->Sides,
                Events::EventKindSideSelectionChanged,
                side, eventParameter);

        return CopyResult(
            std::string("{\"ok\":true,\"changed\":") +
                (changed ? "true}" : "false}"),
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.sides.createTab")
    {
        if (owner->Sides == NULL)
            return FALSE;
        std::string sideName;
        std::string path;
        int index = -1;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "side", &sideName))
        {
            return FALSE;
        }
        std::string rawPath;
        const bool hasPath = Runtime::Protocol::Json::FindRawMember(
            payloadJson, "path", &rawPath);
        const bool pathIsNull = !hasPath || rawPath == "null";
        if (hasPath && !pathIsNull &&
            !Runtime::Protocol::Json::FindStringMember(
                payloadJson, "path", &path))
            return FALSE;
        Runtime::Protocol::Json::FindIntegerMember(
            payloadJson, "index", &index);
        Sides::SideReference side;
        if (!RuntimeTrySideFromName(sideName, &side))
            return FALSE;
        ULONGLONG tabId = 0;
        const BOOL created = owner->Sides->CreateTab(
            side, pathIsNull ? NULL : path.c_str(), index, &tabId);
        char id[32];
        _ui64toa_s(tabId, id, _countof(id), 10);
        return CopyResult(
            std::string("{\"created\":") +
                (created ? "true" : "false") +
                ",\"tabId\":\"" + id + "\"}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.clipboard.copyText")
    {
        std::string text;
        BOOL showEcho = FALSE;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "text", &text) ||
            owner->UI == NULL)
        {
            return FALSE;
        }
        Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "showEcho", &showEcho);
        const BOOL copied = owner->UI->CopyTextToClipboard(
            text.c_str(), showEcho, owner->General->GetMsgBoxParent());
        return CopyResult(
            std::string("{\"ok\":true,\"copied\":") +
                (copied ? "true}" : "false}"),
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.ui.notify")
    {
        std::string title, message;
        int timeout = 2500;
        Runtime::Protocol::Json::FindStringMember(payloadJson, "title", &title);
        Runtime::Protocol::Json::FindStringMember(payloadJson, "message", &message);
        Runtime::Protocol::Json::FindIntegerMember(payloadJson, "timeoutMs", &timeout);
        BOOL shown = owner->UI != NULL && owner->UI->ShowNotification(
            owner->General->GetMsgBoxParent(), title.c_str(), message.c_str(),
            static_cast<DWORD>(timeout > 0 ? timeout : 2500));
        return CopyResult(std::string("{\"ok\":true,\"shown\":") +
                              (shown ? "true}" : "false}"),
                          resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.ui.inputBox")
    {
        if (owner->UI == NULL ||
            owner->UI->GetVersion() < SALAMATRIX_UI_VERSION_1_4)
            return CopyResult(
                "{\"ok\":false,\"error\":\"input-box service unavailable\"}",
                resultJson, resultCapacity, resultLength);
        std::string prompt, title, initial;
        Runtime::Protocol::Json::FindStringMember(payloadJson, "prompt", &prompt);
        Runtime::Protocol::Json::FindStringMember(payloadJson, "title", &title);
        Runtime::Protocol::Json::FindStringMember(payloadJson, "initial", &initial);
        UI::DialogOptions dialogOptions;
        dialogOptions.Title = title.empty() ? "Salamatrix" : title.c_str();
        dialogOptions.Parent = owner->General->GetMsgBoxParent();
        dialogOptions.Width = 360;
        dialogOptions.Height = 118;
        UI::IDialog* dialog = owner->UI->CreateSalamatrixDialog(dialogOptions);
        if (dialog == NULL)
            return FALSE;
        UI::ControlOptions control;
        UI::ControlLayout layout;
        layout.HasBounds = TRUE;
        layout.X = 10; layout.Y = 10; layout.Width = 340; layout.Height = 18;
        control.Id = "prompt"; control.Text = prompt.c_str();
        if (dialog->AddControlEx(UI::ControlKindLabel, control, layout) == NULL)
        { owner->UI->DestroyDialog(dialog); return FALSE; }
        layout.Y = 34; layout.Height = 22;
        control = UI::ControlOptions(); control.Id = "value"; control.Text = initial.c_str();
        UI::IControl* valueControl =
            dialog->AddControlEx(UI::ControlKindTextBox, control, layout);
        if (valueControl == NULL)
        { owner->UI->DestroyDialog(dialog); return FALSE; }
        layout.X = 238; layout.Y = 72; layout.Width = 52; layout.Height = 24;
        control = UI::ControlOptions(); control.Id = "ok";
        control.Text = owner->General->LoadStr(DLLInstance, IDS_INPUT_OK);
        control.DialogResult = IDOK;
        if (dialog->AddControlEx(UI::ControlKindButton, control, layout) == NULL)
        { owner->UI->DestroyDialog(dialog); return FALSE; }
        layout.X = 298;
        control = UI::ControlOptions(); control.Id = "cancel";
        control.Text = owner->General->LoadStr(DLLInstance, IDS_INPUT_CANCEL);
        control.DialogResult = IDCANCEL;
        if (dialog->AddControlEx(UI::ControlKindButton, control, layout) == NULL)
        { owner->UI->DestroyDialog(dialog); return FALSE; }
        const BOOL accepted = dialog->ShowModal() == IDOK;
        char value[4096]; value[0] = '\0';
        valueControl->GetText(value, _countof(value));
        owner->UI->DestroyDialog(dialog);
        return CopyResult(
            std::string("{\"ok\":true,\"accepted\":") +
                (accepted ? "true" : "false") +
                ",\"value\":\"" + JsonEscape(value) + "\"}",
            resultJson, resultCapacity, resultLength);
    }
    if (method.compare(0, 21, "salamander.ui.dialog.") == 0)
    {
        if (owner->UI == NULL || owner->UI->GetVersion() < SALAMATRIX_UI_VERSION_1_4)
            return CopyResult("{\"ok\":false,\"error\":\"dialog service unavailable\"}",
                              resultJson, resultCapacity, resultLength);

        if (method == "salamander.ui.dialog.create")
        {
            std::string title;
            int width = 320;
            int height = 180;
            Runtime::Protocol::Json::FindStringMember(payloadJson, "title", &title);
            Runtime::Protocol::Json::FindIntegerMember(payloadJson, "width", &width);
            Runtime::Protocol::Json::FindIntegerMember(payloadJson, "height", &height);
            UI::DialogOptions options;
            options.Title = title.empty() ? "Salamatrix" : title.c_str();
            options.Parent = owner->General->GetMsgBoxParent();
            options.Width = static_cast<short>(width < 160 ? 160 : (width > 1200 ? 1200 : width));
            options.Height = static_cast<short>(height < 100 ? 100 : (height > 900 ? 900 : height));
            Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "resizable", &options.Resizable);
            UI::IDialog* dialog = owner->UI->CreateSalamatrixDialog(options);
            if (dialog == NULL)
                return FALSE;
#ifdef new
#undef new
#define RESTORE_SALAMATRIX_PACKAGE_DIALOG_DEBUG_NEW_MACRO
#endif
            Package::RuntimeDialog* binding =
                new (std::nothrow) Package::RuntimeDialog;
#ifdef RESTORE_SALAMATRIX_PACKAGE_DIALOG_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_SALAMATRIX_PACKAGE_DIALOG_DEBUG_NEW_MACRO
#endif
            if (binding == NULL)
            {
                owner->UI->DestroyDialog(dialog);
                return CopyResult("{\"ok\":false,\"error\":\"out of memory\"}",
                                  resultJson, resultCapacity, resultLength);
            }
            binding->Owner = package;
            binding->Id = package->NextDialogId++;
            if (binding->Id == 0)
                binding->Id = package->NextDialogId++;
            binding->Dialog = dialog;
            binding->EventsEnabled = FALSE;
            binding->EventName[0] = '\0';
            package->Dialogs.push_back(binding);
            char idText[32];
            _ui64toa_s(binding->Id, idText, _countof(idText), 10);
            return CopyResult(std::string("{\"ok\":true,\"dialogId\":\"") + idText + "\"}",
                              resultJson, resultCapacity, resultLength);
        }

        std::string idText;
        if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "dialogId", &idText))
            return CopyResult("{\"ok\":false,\"error\":\"dialogId is missing\"}",
                              resultJson, resultCapacity, resultLength);
        char* idEnd = NULL;
        const ULONGLONG dialogId = _strtoui64(idText.c_str(), &idEnd, 10);
        if (idEnd == idText.c_str() || *idEnd != '\0')
            return CopyResult("{\"ok\":false,\"error\":\"dialogId is invalid\"}",
                              resultJson, resultCapacity, resultLength);
        size_t dialogIndex = package->Dialogs.size();
        for (size_t index = 0; index < package->Dialogs.size(); ++index)
            if (package->Dialogs[index] != NULL && package->Dialogs[index]->Id == dialogId)
            {
                dialogIndex = index;
                break;
            }
        if (dialogIndex == package->Dialogs.size() || package->Dialogs[dialogIndex] == NULL ||
            package->Dialogs[dialogIndex]->Dialog == NULL)
            return CopyResult(
                std::string("{\"ok\":false,\"error\":\"dialog not found: ") +
                    JsonEscape(idText.c_str()) + "\"}",
                resultJson, resultCapacity, resultLength);
        Package::RuntimeDialog* binding = package->Dialogs[dialogIndex];
        UI::IDialog* dialog = binding->Dialog;

        if (method == "salamander.ui.dialog.add")
        {
            std::string kindName, controlId, controlText;
            if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "kind", &kindName) ||
                !Runtime::Protocol::Json::FindStringMember(payloadJson, "controlId", &controlId))
                return CopyResult(
                    "{\"ok\":false,\"error\":\"dialog control kind or id is missing\"}",
                    resultJson, resultCapacity, resultLength);
            Runtime::Protocol::Json::FindStringMember(payloadJson, "text", &controlText);
            struct KindName { const char* Name; UI::ControlKind Kind; };
            static const KindName kinds[] = {
                {"label", UI::ControlKindLabel}, {"textbox", UI::ControlKindTextBox},
                {"checkbox", UI::ControlKindCheckBox}, {"radio", UI::ControlKindRadioButton},
                {"combobox", UI::ControlKindComboBox}, {"button", UI::ControlKindButton},
                {"listview", UI::ControlKindListView}, {"treeview", UI::ControlKindTreeView},
                {"tabcontrol", UI::ControlKindTabControl}, {"folderpicker", UI::ControlKindFolderPicker},
                {"filepicker", UI::ControlKindFilePicker}, {"groupbox", UI::ControlKindGroupBox},
                {"statictext", UI::ControlKindStaticText}, {"hyperlink", UI::ControlKindHyperLink},
                {"progressbar", UI::ControlKindProgressBar}, {"arrowbutton", UI::ControlKindArrowButton},
                {"textarrowbutton", UI::ControlKindTextArrowButton},
                {"colorarrowbutton", UI::ControlKindColorArrowButton},
                {"toolbarheader", UI::ControlKindToolbarHeader}};
                UI::ControlKind kind = UI::ControlKindLabel;
                bool kindFound = false;
                for (size_t index = 0; index < _countof(kinds); ++index)
                    if (_stricmp(kindName.c_str(), kinds[index].Name) == 0)
                    {
                        kind = kinds[index].Kind;
                        kindFound = true;
                        break;
                    }
                if (!kindFound)
                    return CopyResult(
                        std::string("{\"ok\":false,\"error\":\"unknown dialog control kind: ") +
                            JsonEscape(kindName.c_str()) + "\"}",
                        resultJson, resultCapacity, resultLength);

                UI::ControlOptions options;
                options.Id = controlId.c_str();
                options.Text = controlText.c_str();
                Runtime::Protocol::Json::FindBoolMember(payloadJson, "readOnly", &options.ReadOnly);
                Runtime::Protocol::Json::FindBoolMember(payloadJson, "checked", &options.Checked);
                Runtime::Protocol::Json::FindBoolMember(payloadJson, "keepOpen", &options.KeepOpen);
                Runtime::Protocol::Json::FindBoolMember(payloadJson, "multiline", &options.Multiline);
                Runtime::Protocol::Json::FindIntegerMember(payloadJson, "dialogResult", &options.DialogResult);
                std::string fileFilter;
                Runtime::Protocol::Json::FindStringMember(payloadJson, "filter", &fileFilter);
                options.FileFilter = fileFilter.empty() ? NULL : fileFilter.c_str();
                Runtime::Protocol::Json::FindBoolMember(payloadJson, "save", &options.FileSave);
                UI::ControlLayout layout;
                std::string raw;
                layout.HasBounds = Runtime::Protocol::Json::FindRawMember(payloadJson, "x", &raw);
                if (layout.HasBounds)
                {
                    if (!Runtime::Protocol::Json::FindIntegerMember(payloadJson, "x", &layout.X) ||
                        !Runtime::Protocol::Json::FindIntegerMember(payloadJson, "y", &layout.Y) ||
                        !Runtime::Protocol::Json::FindIntegerMember(payloadJson, "width", &layout.Width) ||
                        !Runtime::Protocol::Json::FindIntegerMember(payloadJson, "height", &layout.Height))
                        return CopyResult(
                            std::string("{\"ok\":false,\"error\":\"invalid layout for dialog control: ") +
                                JsonEscape(controlId.c_str()) + "\"}",
                            resultJson, resultCapacity, resultLength);
                }
                if (!controlId.empty() && dialog->FindControl(controlId.c_str()) != NULL)
                    return CopyResult(
                        std::string("{\"ok\":false,\"error\":\"duplicate dialog control: ") +
                            JsonEscape(controlId.c_str()) + " in dialog " +
                            JsonEscape(idText.c_str()) + "\"}",
                        resultJson, resultCapacity, resultLength);
                UI::IControl* control = dialog->AddControlEx(kind, options, layout);
                if (control == NULL)
                    return CopyResult(
                        std::string("{\"ok\":false,\"error\":\"native dialog rejected control: ") +
                            JsonEscape(controlId.c_str()) + " in dialog " +
                            JsonEscape(idText.c_str()) + "\"}",
                        resultJson, resultCapacity, resultLength);
                int integerValue = 0;
                std::string stringValue;
                if (Runtime::Protocol::Json::FindIntegerMember(payloadJson, "styleFlags", &integerValue) &&
                    !control->SetStyleFlags(static_cast<DWORD>(integerValue))) return FALSE;
                if (Runtime::Protocol::Json::FindStringMember(payloadJson, "pathSeparator", &stringValue) &&
                    (stringValue.size() != 1 || !control->SetPathSeparator(stringValue[0]))) return FALSE;
                if (Runtime::Protocol::Json::FindStringMember(payloadJson, "toolTip", &stringValue) &&
                    !control->SetToolTipText(stringValue.c_str())) return FALSE;
                if (Runtime::Protocol::Json::FindStringMember(payloadJson, "actionOpen", &stringValue) &&
                    !control->SetActionOpen(stringValue.c_str())) return FALSE;
                if (Runtime::Protocol::Json::FindIntegerMember(payloadJson, "actionCommand", &integerValue) &&
                    !control->SetActionPostCommand(static_cast<WORD>(integerValue))) return FALSE;
                if (Runtime::Protocol::Json::FindStringMember(payloadJson, "actionHint", &stringValue) &&
                    !control->SetActionShowHint(stringValue.c_str())) return FALSE;
                std::string progressText;
                Runtime::Protocol::Json::FindStringMember(payloadJson, "progressText", &progressText);
                if (Runtime::Protocol::Json::FindIntegerMember(payloadJson, "progress", &integerValue) &&
                    !control->SetProgress(integerValue, progressText.empty() ? NULL : progressText.c_str())) return FALSE;
                LONGLONG current = 0, total = 0;
                if (Runtime::Protocol::Json::FindInteger64Member(payloadJson, "progressCurrent", &current) &&
                    (!Runtime::Protocol::Json::FindInteger64Member(payloadJson, "progressTotal", &total) ||
                     current < 0 || total < 0 || !control->SetProgressValues(current, total,
                        progressText.empty() ? NULL : progressText.c_str()))) return FALSE;
                int duration = 0, interval = 0;
                if (Runtime::Protocol::Json::FindIntegerMember(payloadJson, "indeterminateDuration", &duration) &&
                    (!Runtime::Protocol::Json::FindIntegerMember(payloadJson, "indeterminateInterval", &interval) ||
                     !control->SetIndeterminateTiming(static_cast<DWORD>(duration), static_cast<DWORD>(interval)))) return FALSE;
                int textColor = 0, backgroundColor = 0;
                if (Runtime::Protocol::Json::FindIntegerMember(payloadJson, "textColor", &textColor) &&
                    (!Runtime::Protocol::Json::FindIntegerMember(payloadJson, "backgroundColor", &backgroundColor) ||
                     !control->SetColor(static_cast<COLORREF>(textColor), static_cast<COLORREF>(backgroundColor)))) return FALSE;
                std::string alignId;
                int buttonMask = 0;
                if (Runtime::Protocol::Json::FindStringMember(payloadJson, "alignControlId", &alignId) &&
                    (!Runtime::Protocol::Json::FindIntegerMember(payloadJson, "buttonMask", &buttonMask) ||
                     !control->SetToolbarHeader(alignId.c_str(), static_cast<DWORD>(buttonMask)))) return FALSE;
            return CopyResult("{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.dialog.item")
        {
            std::string controlId, itemText;
            int parentIndex = -1;
            if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "controlId", &controlId) ||
                !Runtime::Protocol::Json::FindStringMember(payloadJson, "text", &itemText)) return FALSE;
            Runtime::Protocol::Json::FindIntegerMember(payloadJson, "parentIndex", &parentIndex);
            UI::IControl* control = dialog->FindControl(controlId.c_str());
            if (control == NULL || !control->AddItem(itemText.c_str(), parentIndex)) return FALSE;
            return CopyResult(std::string("{\"ok\":true,\"itemCount\":") +
                                  std::to_string(control->GetItemCount()) + "}",
                              resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.dialog.column")
        {
            std::string controlId, title;
            int width = 180;
            if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "controlId", &controlId) ||
                !Runtime::Protocol::Json::FindStringMember(payloadJson, "title", &title)) return FALSE;
            Runtime::Protocol::Json::FindIntegerMember(payloadJson, "width", &width);
            UI::IControl* control = dialog->FindControl(controlId.c_str());
            if (control == NULL || !control->AddColumn(title.c_str(), width)) return FALSE;
            return CopyResult("{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.dialog.selection")
        {
            std::string controlId; int selected = -1;
            if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "controlId", &controlId) ||
                !Runtime::Protocol::Json::FindIntegerMember(payloadJson, "index", &selected)) return FALSE;
            UI::IControl* control = dialog->FindControl(controlId.c_str());
            if (control == NULL || !control->SetSelectedIndex(selected)) return FALSE;
            return CopyResult(std::string("{\"ok\":true,\"selectedIndex\":") +
                                  std::to_string(control->GetSelectedIndex()) + "}",
                              resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.dialog.clearItems")
        {
            std::string controlId;
            if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "controlId", &controlId)) return FALSE;
            UI::IControl* control = dialog->FindControl(controlId.c_str());
            if (control == NULL || !control->ClearItems()) return FALSE;
            return CopyResult("{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.dialog.validation")
        {
            std::string controlId, message; BOOL required = FALSE;
            if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "controlId", &controlId)) return FALSE;
            Runtime::Protocol::Json::FindStringMember(payloadJson, "message", &message);
            Runtime::Protocol::Json::FindBoolMember(payloadJson, "required", &required);
            UI::IControl* control = dialog->FindControl(controlId.c_str());
            if (control == NULL || !control->SetRequired(required) ||
                !control->SetValidationMessage(message.c_str())) return FALSE;
            return CopyResult("{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.dialog.events")
        {
            BOOL enabled = FALSE;
            std::string eventName;
            Runtime::Protocol::Json::FindBoolMember(payloadJson, "enabled", &enabled);
            Runtime::Protocol::Json::FindStringMember(payloadJson, "event", &eventName);
            if (enabled)
            {
                if (eventName.empty() || eventName.size() >= _countof(binding->EventName) ||
                    StringCchCopyA(binding->EventName, _countof(binding->EventName), eventName.c_str()) != S_OK ||
                    !dialog->SetEventCallback(RuntimeDialogEventCallback, binding)) return FALSE;
                binding->EventsEnabled = TRUE;
            }
            else
            {
                if (!dialog->SetEventCallback(NULL, NULL)) return FALSE;
                binding->EventsEnabled = FALSE;
                binding->EventName[0] = '\0';
            }
            return CopyResult(std::string("{\"ok\":true,\"enabled\":") +
                                  (enabled ? "true}" : "false}"),
                              resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.dialog.show")
            return CopyResult(std::string("{\"ok\":true,\"result\":") +
                                  std::to_string(dialog->ShowModal()) + "}",
                              resultJson, resultCapacity, resultLength);
        if (method == "salamander.ui.dialog.get")
        {
            std::string controlId;
            if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "controlId", &controlId)) return FALSE;
            UI::IControl* control = dialog->FindControl(controlId.c_str());
            if (control == NULL) return FALSE;
            char value[4096]; value[0] = '\0'; control->GetText(value, _countof(value));
            return CopyResult(std::string("{\"ok\":true,\"text\":\"") + JsonEscape(value) +
                                  "\",\"checked\":" + (control->GetChecked() ? "true" : "false") +
                                  ",\"itemCount\":" + std::to_string(control->GetItemCount()) +
                                  ",\"selectedIndex\":" + std::to_string(control->GetSelectedIndex()) + "}",
                              resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.dialog.set")
        {
            std::string controlId, value;
            if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "controlId", &controlId) ||
                !Runtime::Protocol::Json::FindStringMember(payloadJson, "value", &value)) return FALSE;
            UI::IControl* control = dialog->FindControl(controlId.c_str());
            if (control == NULL || !control->SetText(value.c_str())) return FALSE;
            return CopyResult("{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.dialog.destroy" || method == "salamander.ui.dialog.close")
        {
            dialog->SetEventCallback(NULL, NULL);
            owner->UI->DestroyDialog(dialog);
            package->Dialogs.erase(package->Dialogs.begin() + dialogIndex);
            delete binding;
            // A properties-style action is usable again as soon as its modal
            // UI has closed. Do not keep swallowing a new double-click while
            // the one-shot process performs its final transport teardown.
            InterlockedExchange(&package->FileSystemActionPending, FALSE);
            return CopyResult("{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        return FALSE;
    }
    if (method == "salamander.ui.controls")
    {
        BOOL shown = owner->UI != NULL &&
                     owner->UI->GetVersion() >= SALAMATRIX_UI_VERSION_1_4 &&
                     owner->UI->ShowControlsShowcase(
                         owner->General->GetMsgBoxParent());
        return CopyResult(std::string("{\"ok\":true,\"shown\":") +
                              (shown ? "true}" : "false}"),
                          resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.ui.viewer.open")
    {
        std::string path;
        std::string renderer = "auto";
        if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "path", &path) || path.empty())
            return CopyResult("{\"ok\":false,\"error\":\"viewer path is required\"}", resultJson, resultCapacity, resultLength);
        Runtime::Protocol::Json::FindStringMember(payloadJson, "renderer", &renderer);
        std::wstring widePath;
        if (!ToWide(path, &widePath))
            return CopyResult("{\"ok\":false,\"error\":\"viewer path is not valid UTF-8\"}", resultJson, resultCapacity, resultLength);

        NativeViewerKind kind = NativeViewerKind::PrismText;
        if (_stricmp(renderer.c_str(), "document") == 0)
            kind = NativeViewerKind::RenderDocument;
        else if (_stricmp(renderer.c_str(), "prism") != 0 && _stricmp(renderer.c_str(), "auto") != 0)
            return CopyResult("{\"ok\":false,\"error\":\"renderer must be auto, prism, or document\"}", resultJson, resultCapacity, resultLength);
        else if (_stricmp(renderer.c_str(), "auto") == 0)
        {
            const char* extension = strrchr(path.c_str(), '.');
            if (extension != NULL)
            {
                static const char* const documentExtensions[] = {
                    ".html", ".htm", ".xhtml", ".mhtml", ".mht", ".md", ".markdown", ".mdown", ".mkd", ".mdx",
                    ".svg", ".svgz", ".webp", ".avif", ".apng", ".png", ".jpg", ".jpeg", ".jfif", ".gif",
                    ".bmp", ".ico", ".tif", ".tiff", ".pdf"};
                for (const char* candidate : documentExtensions)
                    if (_stricmp(extension, candidate) == 0) { kind = NativeViewerKind::RenderDocument; break; }
            }
        }

        RECT placement = {};
        HWND parent = owner->General->GetMainWindowHWND();
        GetWindowRect(parent, &placement);
        placement.left += 48;
        placement.top += 48;
        placement.right = placement.left + (std::max)(placement.right - placement.left - 96L, 640L);
        placement.bottom = placement.top + (std::max)(placement.bottom - placement.top - 96L, 480L);
        BOOL dark = FALSE;
        int configType = 0;
        owner->General->GetConfigParameter(SALCFG_USEWINDOWSDARKMODE, &dark, sizeof(dark), &configType);
        NativeViewerTheme theme = {dark != FALSE,
            owner->General->GetCurrentColor(SALCOL_VIEWER_FG_NORMAL), owner->General->GetCurrentColor(SALCOL_VIEWER_BK_NORMAL),
            owner->General->GetCurrentColor(SALCOL_HOT_PANEL)};
        NativeViewerStrings strings = {
            owner->General->LoadStrW(DLLInstance, IDS_VIEWER_NAME), owner->General->LoadStrW(DLLInstance, IDS_VIEWER_FILE),
            owner->General->LoadStrW(DLLInstance, IDS_VIEWER_VIEW), owner->General->LoadStrW(DLLInstance, IDS_VIEWER_CLOSE),
            owner->General->LoadStrW(DLLInstance, IDS_VIEWER_REFRESH), owner->General->LoadStrW(DLLInstance, IDS_VIEWER_ZOOM_IN),
            owner->General->LoadStrW(DLLInstance, IDS_VIEWER_ZOOM_OUT), owner->General->LoadStrW(DLLInstance, IDS_VIEWER_ZOOM_RESET),
            owner->General->LoadStrW(DLLInstance, IDS_VIEWER_LINE_NUMBERS), owner->General->LoadStrW(DLLInstance, IDS_VIEWER_WRAP_LINES),
            owner->General->LoadStrW(DLLInstance, IDS_VIEWER_SHOW_WHITESPACE),
            owner->General->LoadStrW(DLLInstance, IDS_VIEWER_LOADING), owner->General->LoadStrW(DLLInstance, IDS_VIEWER_READY),
            owner->General->LoadStrW(DLLInstance, IDS_VIEWER_WEBVIEW_FAILED), owner->General->LoadStrW(DLLInstance, IDS_VIEWER_OPEN_FAILED)};
        NativeViewerRequest request = {DLLInstance, parent, widePath.c_str(), placement, SW_SHOWNORMAL, false, NULL, kind, theme, strings};
        const bool opened = NativeViewer_Show(request);
        return CopyResult(std::string("{\"ok\":true,\"opened\":") + (opened ? "true}" : "false}"),
                          resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.ui.fileProperties")
    {
        std::string path;
        std::wstring widePath;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "path", &path) || path.empty() ||
            !ToWide(path, &widePath))
            return FALSE;
        SetLastError(ERROR_SUCCESS);
        const BOOL shown = SHObjectProperties(
            owner->General->GetMsgBoxParent(), SHOP_FILEPATH,
            widePath.c_str(), NULL);
        DWORD error = shown ? ERROR_SUCCESS : GetLastError();
        if (!shown && error == ERROR_SUCCESS)
            error = ERROR_GEN_FAILURE;
        return CopyResult(
            std::string("{\"ok\":") + (shown ? "true" : "false") +
                ",\"shown\":" + (shown ? "true" : "false") +
                ",\"error\":" + std::to_string(error) + "}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.ui.messageBox")
    {
        std::string title, message, buttons, icon;
        Runtime::Protocol::Json::FindStringMember(payloadJson, "title", &title);
        Runtime::Protocol::Json::FindStringMember(payloadJson, "message", &message);
        Runtime::Protocol::Json::FindStringMember(payloadJson, "buttons", &buttons);
        Runtime::Protocol::Json::FindStringMember(payloadJson, "icon", &icon);
        UINT flags = _stricmp(buttons.c_str(), "yesNo") == 0
                         ? MB_YESNO
                         : MB_OK;
        if (_stricmp(icon.c_str(), "error") == 0)
            flags |= MB_ICONERROR;
        else if (_stricmp(icon.c_str(), "warning") == 0)
            flags |= MB_ICONWARNING;
        else if (_stricmp(icon.c_str(), "question") == 0)
            flags |= MB_ICONQUESTION;
        else if (_stricmp(icon.c_str(), "none") != 0)
            flags |= MB_ICONINFORMATION;
        int result = owner->UI != NULL
                         ? owner->UI->ShowMessageBox(
                               owner->General->GetMsgBoxParent(),
                               message.c_str(), title.c_str(),
                               flags)
                         : 0;
        return CopyResult(
            std::string("{\"ok\":true,\"result\":") +
                std::to_string(result) + "}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.ui.pickFile")
    {
        if (owner->UI == NULL)
            return FALSE;

        BOOL save = FALSE;
        std::string title, filter, initialPath;
        Runtime::Protocol::Json::FindBoolMember(payloadJson, "save", &save);
        Runtime::Protocol::Json::FindStringMember(payloadJson, "title", &title);
        Runtime::Protocol::Json::FindStringMember(payloadJson, "filter", &filter);
        Runtime::Protocol::Json::FindStringMember(payloadJson, "initial", &initialPath);
        if (title.empty())
            title = save ? "Save file" : "Open file";

        std::vector<char> selectedPath(SAL_MAX_PATH * 3);
        BOOL selected = owner->UI->PickFile(
            owner->General->GetMsgBoxParent(), save, title.c_str(),
            filter.c_str(), initialPath.c_str(), &selectedPath[0],
            static_cast<DWORD>(selectedPath.size()));
        return CopyResult(
            std::string("{\"ok\":true,\"selected\":") +
                (selected ? "true" : "false") +
                ",\"path\":\"" +
                JsonEscape(selected ? &selectedPath[0] : "") + "\"}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.ui.pickFolder")
    {
        if (owner->UI == NULL)
            return FALSE;

        std::string title, initialPath;
        Runtime::Protocol::Json::FindStringMember(payloadJson, "title", &title);
        Runtime::Protocol::Json::FindStringMember(payloadJson, "initial", &initialPath);
        if (title.empty())
            title = "Select folder";

        std::vector<char> selectedPath(SAL_MAX_PATH * 3);
        BOOL selected = owner->UI->PickFolder(
            owner->General->GetMsgBoxParent(), title.c_str(),
            initialPath.c_str(), &selectedPath[0],
            static_cast<DWORD>(selectedPath.size()));
        return CopyResult(
            std::string("{\"ok\":true,\"selected\":") +
                (selected ? "true" : "false") +
                ",\"path\":\"" +
                JsonEscape(selected ? &selectedPath[0] : "") + "\"}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.ui.renderIcon")
    {
        std::string path;
        int size = 16;
        Runtime::Protocol::Json::FindStringMember(payloadJson, "path", &path);
        Runtime::Protocol::Json::FindIntegerMember(payloadJson, "size", &size);
        if (path.empty() || size < 1 || size > 256 || SalamanderGUI == NULL)
            return CopyResult(
                "{\"ok\":true,\"icon\":\"\"}",
                resultJson, resultCapacity, resultLength);

        HICON icon = SalamanderGUI->CreateSVGIcon(path.c_str(), size);
        const std::string encodedIcon = SerializeWindowIcon(icon);
        if (icon != NULL)
            DestroyIcon(icon);
        return CopyResult(
            std::string("{\"ok\":true,\"icon\":\"") +
                encodedIcon + "\"}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.ui.progress.create" ||
        method == "salamander.ui.progress.update" ||
        method == "salamander.ui.progress.step" ||
        method == "salamander.ui.progress.setTotals" ||
        method == "salamander.ui.progress.setPositions" ||
        method == "salamander.ui.progress.setTitle" ||
        method == "salamander.ui.progress.setCancelEnabled" ||
        method == "salamander.ui.progress.cancelled" ||
        method == "salamander.ui.progress.close")
    {
        if (owner->UI == NULL)
            return CopyResult(
                "{\"ok\":false,\"error\":\"progress service unavailable\"}",
                resultJson, resultCapacity, resultLength);

        if (method == "salamander.ui.progress.create")
        {
            if (package->Operations == NULL)
                return CopyResult(
                    "{\"ok\":false,\"error\":\"progress requires an operation context\"}",
                    resultJson, resultCapacity, resultLength);
            owner->ReleaseProgress(package);
            std::string title;
            Runtime::Protocol::Json::FindStringMember(payloadJson, "title", &title);
            BOOL twoProgressBars = FALSE;
            BOOL fileProgress = FALSE;
            BOOL cancelEnabled = TRUE;
            Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "twoProgressBars", &twoProgressBars);
            Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "fileProgress", &fileProgress);
            Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "cancelEnabled", &cancelEnabled);
            UI::ProgressDialogOptions options;
            options.Title = title.empty() ? "Salamatrix" : title.c_str();
            options.Parent = owner->General->GetMsgBoxParent();
            options.TwoProgressBars = twoProgressBars;
            options.FileProgress = fileProgress;
            options.CancelEnabled = cancelEnabled;
            package->Progress = owner->UI->CreateProgressDialog(package->Operations);
            if (package->Progress == NULL)
                return CopyResult(
                    "{\"ok\":false,\"error\":\"progress dialog unavailable\"}",
                    resultJson, resultCapacity, resultLength);
            package->Progress->Open(options);
            LONGLONG total = 0;
            if (Runtime::Protocol::Json::FindInteger64Member(
                    payloadJson, "total", &total) && total >= 0)
            {
                LONGLONG total2 = 0;
                if (Runtime::Protocol::Json::FindInteger64Member(
                        payloadJson, "total2", &total2) && total2 >= 0)
                {
                    CQuadWord first;
                    CQuadWord second;
                    first.SetUI64(static_cast<unsigned __int64>(total));
                    second.SetUI64(static_cast<unsigned __int64>(total2));
                    package->Progress->SetTotals(first, second);
                }
                else
                {
                    CQuadWord first;
                    first.SetUI64(static_cast<unsigned __int64>(total));
                    package->Progress->SetTotal(first);
                }
            }
            package->ProgressId = package->ProgressId == static_cast<ULONGLONG>(-1)
                                      ? 1
                                      : package->ProgressId + 1;
            if (package->ProgressId == 0)
                package->ProgressId = 1;
            char idText[32];
            _ui64toa_s(package->ProgressId, idText, _countof(idText), 10);
            return CopyResult(
                std::string("{\"ok\":true,\"progressId\":\"") +
                    idText + "\"}",
                resultJson, resultCapacity, resultLength);
        }

        std::string idText;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "progressId", &idText))
            return FALSE;
        char* idEnd = NULL;
        ULONGLONG progressId = _strtoui64(idText.c_str(), &idEnd, 10);
        if (idEnd == idText.c_str() || *idEnd != '\0' ||
            package->Progress == NULL || package->ProgressId != progressId)
            return FALSE;
        UI::IProgressDialog* progress = package->Progress;
        if (method == "salamander.ui.progress.close")
        {
            owner->ReleaseProgress(package);
            return CopyResult(
                "{\"ok\":true,\"closed\":true}",
                resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.progress.cancelled")
            return CopyResult(
                std::string("{\"ok\":true,\"cancelled\":") +
                    (progress->IsCancelled() ? "true}" : "false}"),
                resultJson, resultCapacity, resultLength);
        if (method == "salamander.ui.progress.setTitle")
        {
            std::string title;
            if (!Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "title", &title))
                return FALSE;
            progress->SetTitle(title.c_str());
            return CopyResult("{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.progress.setCancelEnabled")
        {
            BOOL enabled = TRUE;
            Runtime::Protocol::Json::FindBoolMember(payloadJson, "enabled", &enabled);
            progress->SetCancelEnabled(enabled);
            return CopyResult("{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.progress.setTotals")
        {
            LONGLONG total = 0;
            LONGLONG total2 = 0;
            if (!Runtime::Protocol::Json::FindInteger64Member(payloadJson, "total", &total) ||
                !Runtime::Protocol::Json::FindInteger64Member(payloadJson, "total2", &total2) ||
                total < 0 || total2 < 0)
                return FALSE;
            CQuadWord first;
            CQuadWord second;
            first.SetUI64(static_cast<unsigned __int64>(total));
            second.SetUI64(static_cast<unsigned __int64>(total2));
            progress->SetTotals(first, second);
            return CopyResult("{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }

        BOOL delayedPaint = TRUE;
        Runtime::Protocol::Json::FindBoolMember(payloadJson, "delayedPaint", &delayedPaint);
        BOOL continued = TRUE;
        if (method == "salamander.ui.progress.step")
        {
            int amount = 1;
            Runtime::Protocol::Json::FindIntegerMember(payloadJson, "amount", &amount);
            continued = progress->Step(amount, delayedPaint);
        }
        else
        {
            LONGLONG position = 0;
            if (!Runtime::Protocol::Json::FindInteger64Member(payloadJson, "position", &position) ||
                position < 0)
                return FALSE;
            CQuadWord first;
            first.SetUI64(static_cast<unsigned __int64>(position));
            LONGLONG total = 0;
            if (Runtime::Protocol::Json::FindInteger64Member(payloadJson, "total", &total) && total >= 0)
            {
                CQuadWord value;
                value.SetUI64(static_cast<unsigned __int64>(total));
                progress->SetTotal(value);
            }
            std::string text;
            Runtime::Protocol::Json::FindStringMember(payloadJson, "text", &text);
            if (!text.empty())
                progress->AddText(text.c_str(), delayedPaint);
            LONGLONG position2 = 0;
            if (Runtime::Protocol::Json::FindInteger64Member(payloadJson, "position2", &position2) && position2 >= 0)
            {
                CQuadWord second;
                second.SetUI64(static_cast<unsigned __int64>(position2));
                continued = progress->SetPositions(first, second, delayedPaint);
            }
            else
                continued = progress->SetPosition(first, delayedPaint);
        }
        return CopyResult(
            std::string("{\"ok\":true,\"continued\":") +
                (continued && !progress->IsCancelled() ? "true}" : "false}"),
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.storage.keys")
    {
        if (owner->Storage == NULL)
            return FALSE;
        const int count = owner->Storage->GetKeyCount(package->Id.c_str());
        if (count < 0 || count > 1024)
            return CopyResult(
                "{\"ok\":false,\"error\":\"storage enumeration failed\"}",
                resultJson, resultCapacity, resultLength);
        struct KeyRecord
        {
            std::string Key;
            Storage::StorageValueType Type;
        };
        std::vector<KeyRecord> records;
        records.reserve(static_cast<size_t>(count));
        for (int index = 0; index < count; ++index)
        {
            char key[256];
            int required = 0;
            Storage::StorageValueType type = Storage::StorageValueMissing;
            if (!owner->Storage->GetKeyAt(
                    package->Id.c_str(), index, key, _countof(key),
                    &required, &type) ||
                required <= 0 || required > static_cast<int>(_countof(key)) ||
                type < Storage::StorageValueString ||
                type > Storage::StorageValueBoolean ||
                key[required - 1] != '\0')
                return CopyResult(
                    "{\"ok\":false,\"error\":\"storage enumeration changed\"}",
                    resultJson, resultCapacity, resultLength);
            KeyRecord record;
            record.Key = key;
            record.Type = type;
            records.push_back(record);
        }
        std::sort(
            records.begin(), records.end(),
            [](const KeyRecord& left, const KeyRecord& right) {
                const int comparison =
                    _stricmp(left.Key.c_str(), right.Key.c_str());
                return comparison != 0 ? comparison < 0 : left.Key < right.Key;
            });
        std::string response = "{\"ok\":true,\"keys\":[";
        for (size_t index = 0; index < records.size(); ++index)
        {
            if (index != 0)
                response += ",";
            response += "{\"key\":\"" + JsonEscape(records[index].Key.c_str()) +
                        "\",\"type\":\"";
            response += records[index].Type == Storage::StorageValueInteger
                            ? "integer"
                            : records[index].Type == Storage::StorageValueBoolean
                                  ? "boolean"
                                  : "string";
            response += "\"}";
        }
        response += "]}";
        return CopyResult(
            response, resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.storage.schema")
    {
        std::string response = "{\"ok\":true,\"settings\":[";
        for (size_t index = 0; index < package->Manifest.Settings.size(); ++index)
        {
            const CExtensionManifestSetting& setting =
                package->Manifest.Settings[index];
            if (index != 0)
                response += ",";
            response += "{\"key\":\"" + JsonEscape(setting.Key.c_str()) +
                        "\",\"type\":\"";
            if (setting.Type == ExtensionManifestSettingInteger)
                response += "integer";
            else if (setting.Type == ExtensionManifestSettingBoolean)
                response += "boolean";
            else
                response += "string";
            response += "\"";
            if (setting.HasDefault)
            {
                response += ",\"hasDefault\":true,\"default\":";
                if (setting.Type == ExtensionManifestSettingString)
                    response += "\"" +
                                JsonEscape(setting.StringDefault.c_str()) +
                                "\"";
                else if (setting.Type == ExtensionManifestSettingInteger)
                    response += std::to_string(setting.IntegerDefault);
                else
                    response += setting.BooleanDefault ? "true" : "false";
            }
            else
                response += ",\"hasDefault\":false";
            response += ",\"label\":\"" + JsonEscape(setting.Label.c_str()) +
                        "\",\"description\":\"" +
                        JsonEscape(setting.Description.c_str()) +
                        "\",\"group\":\"" + JsonEscape(setting.Group.c_str()) +
                        "\",\"order\":" + std::to_string(setting.Order) +
                        ",\"width\":" + std::to_string(setting.Width) +
                        ",\"multiline\":" +
                        (setting.Multiline ? "true" : "false") + "}";
        }
        response += "]}";
        return CopyResult(
            response, resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.storage.clear")
    {
        if (owner->Storage == NULL)
            return FALSE;
        const BOOL cleared = owner->Storage->ClearExtension(package->Id.c_str());
        return CopyResult(
            std::string("{\"ok\":") + (cleared ? "true}" : "false}"),
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.storage.get")
    {
        std::string key;
        if (!Runtime::Protocol::Json::FindStringMember(
                payloadJson, "key", &key) || owner->Storage == NULL)
            return FALSE;
        const Storage::StorageValueType type = owner->Storage->GetValueType(
            package->Id.c_str(), key.c_str());
        if (type == Storage::StorageValueString)
        {
            std::vector<char> value(16385);
            int required = 0;
            if (!owner->Storage->GetString(
                    package->Id.c_str(), key.c_str(), &value[0],
                    static_cast<int>(value.size()), &required))
                return FALSE;
            return CopyResult(
                std::string("{\"ok\":true,\"type\":\"string\",\"value\":\"") +
                    JsonEscape(&value[0]) + "\"}",
                resultJson, resultCapacity, resultLength);
        }
        if (type == Storage::StorageValueInteger)
        {
            LONGLONG value = 0;
            if (!owner->Storage->GetInteger(
                    package->Id.c_str(), key.c_str(), &value))
                return FALSE;
            return CopyResult(
                std::string("{\"ok\":true,\"type\":\"integer\",\"value\":") +
                    std::to_string(static_cast<long long>(value)) + "}",
                resultJson, resultCapacity, resultLength);
        }
        if (type == Storage::StorageValueBoolean)
        {
            BOOL value = FALSE;
            if (!owner->Storage->GetBoolean(
                    package->Id.c_str(), key.c_str(), &value))
                return FALSE;
            return CopyResult(
                std::string("{\"ok\":true,\"type\":\"boolean\",\"value\":") +
                    (value ? "true}" : "false}"),
                resultJson, resultCapacity, resultLength);
        }
        return CopyResult(
            "{\"ok\":true,\"type\":\"missing\"}",
            resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.storage.set")
    {
        std::string key, raw;
        if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "key", &key) ||
            !Runtime::Protocol::Json::FindRawMember(payloadJson, "value", &raw) ||
            owner->Storage == NULL)
            return FALSE;
        BOOL stored = FALSE;
        if (raw == "true" || raw == "false")
        {
            BOOL value = raw == "true" ? TRUE : FALSE;
            stored = owner->Storage->SetBoolean(package->Id.c_str(), key.c_str(), value);
        }
        else if (!raw.empty() && raw[0] == '"')
        {
            std::string value;
            stored = Runtime::Protocol::Json::FindStringMember(payloadJson, "value", &value) &&
                     owner->Storage->SetString(package->Id.c_str(), key.c_str(), value.c_str());
        }
        else
        {
            LONGLONG value = 0;
            stored = Runtime::Protocol::Json::FindInteger64Member(payloadJson, "value", &value) &&
                     owner->Storage->SetInteger(package->Id.c_str(), key.c_str(), value);
        }
        return CopyResult(stored ? "{\"ok\":true}" : "{\"ok\":false}",
                          resultJson, resultCapacity, resultLength);
    }
    if (method == "salamander.storage.remove")
    {
        std::string key;
        if (!Runtime::Protocol::Json::FindStringMember(payloadJson, "key", &key) ||
            owner->Storage == NULL)
            return FALSE;
        BOOL removed = owner->Storage->DeleteValue(package->Id.c_str(), key.c_str());
        return CopyResult(std::string("{\"ok\":true,\"removed\":") +
                              (removed ? "true}" : "false}"),
                          resultJson, resultCapacity, resultLength);
    }
    return CopyResult("{\"ok\":false,\"error\":\"unsupported host method\"}",
                      resultJson, resultCapacity, resultLength);
}

BOOL WINAPI PackageManager::HostDispatchOnMainThread(void* context)
{
    MainThreadDispatch* call = static_cast<MainThreadDispatch*>(context);
    if (call == NULL)
        return FALSE;
    MainThreadDispatch* previous = CurrentMainThreadDispatch;
    CurrentMainThreadDispatch = call;
    Package* package = static_cast<Package*>(call->Context);
    if (package == NULL ||
        InterlockedCompareExchange(&package->Stopping, FALSE, FALSE) != FALSE)
    {
        CurrentMainThreadDispatch = previous;
        return FALSE;
    }
    PackageManager* owner = package != NULL ? package->Owner : NULL;
    if (owner != NULL)
        ++owner->ActiveHostDispatches;
    BOOL result = HostDispatch(
        call->Context, call->Type, call->RequestId, call->PayloadJson,
        call->ResultJson, call->ResultCapacity, call->ResultLength);
    CurrentMainThreadDispatch = previous;
    if (owner != NULL)
        owner->FinishHostDispatch();
    return result;
}

void PackageManager::FinishHostDispatch()
{
    if (ActiveHostDispatches > 0)
        --ActiveHostDispatches;
    if (ActiveHostDispatches == 0 && ActiveExecutions == 0 &&
        RefreshPending && !RefreshInProgress)
        Refresh();
}

void PackageManager::BeginExecution()
{
    if (ExecutionsIdleEvent != NULL)
        ResetEvent(ExecutionsIdleEvent);
    InterlockedIncrement(&ActiveExecutions);
}

void PackageManager::FinishExecution()
{
    const LONG active = InterlockedDecrement(&ActiveExecutions);
    if (active < 0)
    {
        InterlockedExchange(&ActiveExecutions, 0);
        if (ExecutionsIdleEvent != NULL)
            SetEvent(ExecutionsIdleEvent);
        return;
    }
    if (active == 0 && ExecutionsIdleEvent != NULL)
        SetEvent(ExecutionsIdleEvent);
    if (active == 0 && ActiveHostDispatches == 0 &&
        RefreshPending && !RefreshInProgress)
    {
        Refresh();
    }
}

void PackageManager::ReportStartupProgress(
    CSalamanderStartupProgressPhase phase, const char* detail,
    int current, int total) const
{
    if (General == NULL)
        return;
    CSalamanderServiceQuery query;
    CSalamanderServiceResult result;
    memset(&query, 0, sizeof(query));
    memset(&result, 0, sizeof(result));
    query.ServiceId = SALAMANDER_SERVICE_STARTUP_PROGRESS;
    query.MinimumVersion = SALAMANDER_STARTUP_PROGRESS_VERSION_1_0;
    if (General->QueryService(&query, &result) && result.Interface != NULL)
    {
        static_cast<CSalamanderStartupProgressAbstract*>(result.Interface)
            ->ReportStartupProgress(phase, detail, current, total);
    }
}

void PackageManager::ReportShutdownProgress(
    CSalamanderShutdownProgressPhase phase, const char* detail,
    int current, int total) const
{
    if (General == NULL)
        return;
    CSalamanderServiceQuery query;
    CSalamanderServiceResult result;
    memset(&query, 0, sizeof(query));
    memset(&result, 0, sizeof(result));
    query.ServiceId = SALAMANDER_SERVICE_SHUTDOWN_PROGRESS;
    query.MinimumVersion = SALAMANDER_SHUTDOWN_PROGRESS_VERSION_1_0;
    if (General->QueryService(&query, &result) && result.Interface != NULL)
    {
        static_cast<CSalamanderShutdownProgressAbstract*>(result.Interface)
            ->ReportShutdownProgress(phase, detail, current, total);
    }
}

void PackageManager::RegisterToolbarButtons()
{
    if (General == NULL)
        return;
    for (size_t p = 0; p < Packages.size(); ++p)
    {
        Package* package = Packages[p];
        if (!package->RuntimeUsable)
            continue;
        bool hasToolbarCommand = false;
        for (size_t c = 0; c < package->Manifest.Commands.size(); ++c)
        {
            const CExtensionManifestCommand& command =
                package->Manifest.Commands[c];
            if (command.Toolbar && command.Visible)
            {
                hasToolbarCommand = true;
                break;
            }
        }
        if (hasToolbarCommand)
        {
            const char* progressDetail = package->Manifest.Name.empty()
                                             ? package->Id.c_str()
                                             : package->Manifest.Name.c_str();
            ReportStartupProgress(
                ssppRegisteringToolbarButtons, progressDetail,
                static_cast<int>(p + 1), static_cast<int>(Packages.size()));
        }
        for (size_t c = 0; c < package->Manifest.Commands.size(); ++c)
        {
            const CExtensionManifestCommand& command = package->Manifest.Commands[c];
            if (!command.Toolbar || !command.Visible)
                continue;
            CSalamanderToolbarButton button;
            button.CommandId = package->CommandIds[c];
            button.Title = command.Title.c_str();
            button.IconPath = package->IconPath.c_str();
            button.IconDarkPath = package->IconDarkPath.empty() ? NULL : package->IconDarkPath.c_str();
            button.StableId = package->Id.c_str();
            std::vector<CSalamanderToolbarMenuItem> menuItems;
            if (command.ToolbarMenu)
            {
                button.Enabled = FALSE;
                for (size_t item = 0;
                     item < package->Manifest.Commands.size(); ++item)
                {
                    const CExtensionManifestCommand& menuCommand =
                        package->Manifest.Commands[item];
                    if (menuCommand.Visible && menuCommand.Enabled &&
                        (menuCommand.Menu == "plugin" ||
                         menuCommand.Menu == "both"))
                    {
                        button.Enabled = TRUE;
                    }
                    if (!menuCommand.Visible ||
                        (menuCommand.Menu != "plugin" &&
                         menuCommand.Menu != "both"))
                        continue;
                    CSalamanderToolbarMenuItem menuItem;
                    menuItem.CommandId = package->CommandIds[item];
                    menuItem.Title = menuCommand.Title.c_str();
                    menuItem.Enabled =
                        menuCommand.Enabled ? TRUE : FALSE;
                    menuItem.IconPath =
                        package->CommandIconPaths[item].empty()
                            ? NULL
                            : package->CommandIconPaths[item].c_str();
                    menuItem.IconDarkPath =
                        package->CommandIconDarkPaths[item].empty()
                            ? NULL
                            : package->CommandIconDarkPaths[item].c_str();
                    menuItems.push_back(menuItem);
                }
                button.MenuItems =
                    menuItems.empty() ? NULL : &menuItems[0];
                button.MenuItemCount =
                    static_cast<int>(menuItems.size());
            }
            else
                button.Enabled = command.Enabled ? TRUE : FALSE;
            General->RegisterToolbarButton(&button);
        }
    }
}

void PackageManager::UnregisterToolbarButtons()
{
    if (General == NULL)
        return;
    for (size_t p = 0; p < Packages.size(); ++p)
    {
        if (!Packages[p]->CommandIds.empty())
        {
            const char* progressDetail = Packages[p]->Manifest.Name.empty()
                                             ? Packages[p]->Id.c_str()
                                             : Packages[p]->Manifest.Name.c_str();
            ReportShutdownProgress(
                ssdpUnregisteringToolbarButtons, progressDetail,
                static_cast<int>(p + 1),
                static_cast<int>(Packages.size()));
        }
        for (size_t c = 0; c < Packages[p]->CommandIds.size(); ++c)
            General->UnregisterToolbarButton(Packages[p]->CommandIds[c]);
    }
}

std::wstring PackageManager::ExpandRoot(const std::wstring& root)
{
    if (root != L"$(SalDir)\\extensions" && root.find(L"$(SalDir)") != 0)
        return root;
    std::vector<wchar_t> module(SAL_MAX_PATH);
    DWORD length = GetModuleFileNameW(NULL, &module[0], static_cast<DWORD>(module.size()));
    if (length == 0 || length >= module.size())
        return root;
    std::wstring salDir(&module[0], length);
    size_t slash = salDir.find_last_of(L"\\/");
    if (slash != std::wstring::npos)
        salDir.erase(slash);
    return salDir + root.substr(9);
}

BOOL PackageManager::MakeDisplayEntryPoint(
    const std::wstring& entryPoint,
    std::string* display)
{
    if (display == NULL)
        return FALSE;
    std::wstring salDir = ExpandRoot(L"$(SalDir)");
    std::wstring value = entryPoint;
    if (entryPoint.size() > salDir.size() &&
        _wcsnicmp(entryPoint.c_str(), salDir.c_str(), salDir.size()) == 0 &&
        (entryPoint[salDir.size()] == L'\\' || entryPoint[salDir.size()] == L'/'))
    {
        value = entryPoint.substr(salDir.size() + 1);
    }
    return ToUtf8(value, display);
}

BOOL PackageManager::ReadUtf8File(const std::wstring& path, std::string* text)
{
    if (text == NULL)
        return FALSE;
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;
    LARGE_INTEGER size;
    BOOL ok = GetFileSizeEx(file, &size) && size.QuadPart >= 0 && size.QuadPart <= 4 * 1024 * 1024;
    if (ok)
    {
        text->assign(static_cast<size_t>(size.QuadPart), '\0');
        DWORD read = 0;
        ok = size.QuadPart == 0 || ReadFile(file, &(*text)[0], static_cast<DWORD>(size.QuadPart), &read, NULL);
        ok = ok && (size.QuadPart == 0 || read == static_cast<DWORD>(size.QuadPart));
    }
    CloseHandle(file);
    return ok;
}

BOOL PackageManager::ToUtf8(const std::wstring& value, std::string* result)
{
    if (result == NULL)
        return FALSE;
    int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.c_str(), -1, NULL, 0, NULL, NULL);
    if (length <= 0)
        return FALSE;
    std::vector<char> buffer(static_cast<size_t>(length));
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.c_str(), -1,
                            &buffer[0], length, NULL, NULL) <= 0)
        return FALSE;
    result->assign(&buffer[0]);
    return TRUE;
}

BOOL PackageManager::ToWide(const std::string& value, std::wstring* result)
{
    if (result == NULL)
        return FALSE;
    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1, NULL, 0);
    if (length <= 0)
        return FALSE;
    std::vector<wchar_t> buffer(static_cast<size_t>(length));
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1,
                            &buffer[0], length) <= 0)
        return FALSE;
    result->assign(&buffer[0]);
    return TRUE;
}

BOOL PackageManager::CopyResult(const std::string& value, char* result, DWORD capacity, DWORD* length)
{
    if (result == NULL || length == NULL || capacity == 0 || value.size() + 1 > capacity)
        return FALSE;
    memcpy(result, value.c_str(), value.size() + 1);
    *length = static_cast<DWORD>(value.size());
    return TRUE;
}

} // namespace Packages
} // namespace Salamatrix
