// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#ifdef new
#define SAL_RESTORE_DEBUG_NEW
#undef new
#endif

#include <WebView2.h>
#include <shlwapi.h>
#include <wrl.h>

#ifdef SAL_RESTORE_DEBUG_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef SAL_RESTORE_DEBUG_NEW
#endif

#include "native_viewer.h"
#include "../plugindarkmode.h"
#include "../salamatrix/salamatrix_ui.h"
#include "../spl_gen.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <utility>
#include <vector>

#pragma comment(lib, "shlwapi.lib")

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace
{
constexpr UINT WM_NV_CLOSE_ALL = WM_APP + 0x631;
constexpr UINT WM_NV_APPLY_ZOOM = WM_APP + 0x632;
constexpr UINT WM_NV_CREATE_VIEWER = WM_APP + 0x633;
constexpr UINT WM_NV_STOP_HOST = WM_APP + 0x634;
constexpr UINT WM_NV_PREWARM_ENVIRONMENT = WM_APP + 0x635;
constexpr UINT WM_NV_PREPARATION_COMPLETE = WM_APP + 0x636;
constexpr size_t NV_PRISM_VIRTUAL_THRESHOLD = 512U * 1024U;
constexpr size_t NV_PRISM_FILE_LIMIT = 16U * 1024U * 1024U;
constexpr size_t NV_MARKDOWN_FILE_LIMIT = 32U * 1024U * 1024U;
constexpr size_t NV_VIRTUAL_CHUNK_LINES = 80;
constexpr size_t NV_VIRTUAL_CHUNK_CHARS = 96U * 1024U;
static_assert(NV_PRISM_VIRTUAL_THRESHOLD > NV_VIRTUAL_CHUNK_CHARS,
              "virtual highlighting chunks must stay smaller than the old full-document threshold");
constexpr int IDC_NV_STATUS = 101;
constexpr int IDM_NV_CLOSE = 40001;
constexpr int IDM_NV_REFRESH = 40002;
constexpr int IDM_NV_ZOOM_IN = 40003;
constexpr int IDM_NV_ZOOM_OUT = 40004;
constexpr int IDM_NV_ZOOM_RESET = 40005;
constexpr int IDM_NV_LINE_NUMBERS = 40006;
constexpr int IDM_NV_WRAP_LINES = 40007;
constexpr int IDM_NV_SHOW_WHITESPACE = 40008;
constexpr int IDM_NV_COLORS_VISUAL_STUDIO = 40009;
constexpr int IDM_NV_COLORS_PRISM = 40010;
constexpr int IDM_NV_COLORS_CUSTOM = 40011;
constexpr int IDM_NV_COLORS_EDIT_CUSTOM = 40012;
constexpr DWORD NV_PALETTE_VISUAL_STUDIO = 0;
constexpr DWORD NV_PALETTE_PRISM = 1;
constexpr DWORD NV_PALETTE_CUSTOM = 2;
constexpr int IDM_NV_SYNTAX_AUTOMATIC = 40999;
constexpr int IDM_NV_SYNTAX_FIRST = 41000;
constexpr int IDC_NV_ZOOM_RESET = 40101;
constexpr int IDC_NV_ZOOM_OUT = 40102;
constexpr int IDC_NV_ZOOM_EDIT = 40103;
constexpr int IDC_NV_ZOOM_IN = 40104;

#ifndef WM_UAHDRAWMENU
#define WM_UAHDRAWMENU 0x0091
#endif
#ifndef WM_UAHDRAWMENUITEM
#define WM_UAHDRAWMENUITEM 0x0092
#endif

struct NativeViewerUAHMenu { HMENU menu; HDC dc; DWORD flags; };
struct NativeViewerUAHMenuItem { int position; DWORD metrics[16]; };
struct NativeViewerUAHDrawMenuItem
{
    DRAWITEMSTRUCT draw;
    NativeViewerUAHMenu menu;
    NativeViewerUAHMenuItem item;
};

std::mutex gWindowsLock;
std::vector<HWND> gWindows;
std::atomic<bool> gShuttingDown(false);
std::mutex gViewerHostLock;
HANDLE gViewerHostThread = nullptr;
DWORD gViewerHostThreadId = 0;
HANDLE gViewerHostReady = nullptr;
ComPtr<ICoreWebView2Environment> gSharedEnvironment;
std::vector<HWND> gPendingEnvironmentWindows;
bool gCreatingSharedEnvironment = false;
std::atomic<uint64_t> gNextPreparationGeneration(1);

template <typename T, typename... Args>
T* NewNoThrow(Args&&... args)
{
#ifdef new
#define SAL_RESTORE_DEBUG_NEW_NOTHROW
#undef new
#endif
    T* value = new (std::nothrow) T(std::forward<Args>(args)...);
#ifdef SAL_RESTORE_DEBUG_NEW_NOTHROW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef SAL_RESTORE_DEBUG_NEW_NOTHROW
#endif
    return value;
}

using CreateWebView2EnvironmentFn = HRESULT(STDAPICALLTYPE*)(
    PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions*,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);

CreateWebView2EnvironmentFn GetCreateWebView2Environment()
{
    static CreateWebView2EnvironmentFn createEnvironment = []() -> CreateWebView2EnvironmentFn
    {
        std::vector<wchar_t> modulePath(512);
        for (;;)
        {
            DWORD length = GetModuleFileNameW(nullptr, modulePath.data(),
                                              static_cast<DWORD>(modulePath.size()));
            if (length == 0)
                return nullptr;
            if (length < modulePath.size() - 1)
            {
                modulePath.resize(length);
                break;
            }
            modulePath.resize(modulePath.size() * 2);
        }

        size_t slash = std::wstring(modulePath.data(), modulePath.size()).find_last_of(L"\\/");
        if (slash == std::wstring::npos)
            return nullptr;
        modulePath.resize(slash + 1);
        static constexpr wchar_t loaderName[] = L"utils\\WebView2Loader.dll";
        modulePath.insert(modulePath.end(), loaderName, loaderName + _countof(loaderName));

        HMODULE loader = LoadLibraryExW(modulePath.data(), nullptr,
                                        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                                        LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (loader == nullptr && GetLastError() == ERROR_INVALID_PARAMETER)
            loader = LoadLibraryW(modulePath.data());
        if (loader == nullptr)
            return nullptr;
        return reinterpret_cast<CreateWebView2EnvironmentFn>(
            GetProcAddress(loader, "CreateCoreWebView2EnvironmentWithOptions"));
    }();
    return createEnvironment;
}

constexpr const wchar_t* kViewerSettingsKey = L"Software\\Open Salamander\\ViewerFrame";

std::wstring ViewerUserDataFolder()
{
    PWSTR appData = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &appData)))
        return L"";
    std::wstring result = std::wstring(appData) + L"\\Open Salamander\\Native WebView2 Viewer";
    CoTaskMemFree(appData);
    return result;
}

DWORD ReadViewerSetting(const wchar_t* name, DWORD fallback)
{
    DWORD value = fallback;
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, kViewerSettingsKey, name, RRF_RT_REG_DWORD,
                     nullptr, &value, &size) != ERROR_SUCCESS)
        return fallback;
    return value;
}

void WriteViewerSetting(const wchar_t* name, DWORD value)
{
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kViewerSettingsKey, 0, nullptr, 0, KEY_SET_VALUE,
                        nullptr, &key, nullptr) == ERROR_SUCCESS)
    {
        RegSetValueExW(key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
        RegCloseKey(key);
    }
}

std::wstring ReadViewerSettingString(const wchar_t* name)
{
    DWORD size = 0;
    if (name == nullptr ||
        RegGetValueW(HKEY_CURRENT_USER, kViewerSettingsKey, name, RRF_RT_REG_SZ, nullptr, nullptr,
                     &size) != ERROR_SUCCESS ||
        size < sizeof(wchar_t))
        return {};
    std::wstring value(size / sizeof(wchar_t), L'\0');
    if (RegGetValueW(HKEY_CURRENT_USER, kViewerSettingsKey, name, RRF_RT_REG_SZ, nullptr, value.data(),
                     &size) != ERROR_SUCCESS)
        return {};
    if (!value.empty() && value.back() == L'\0')
        value.pop_back();
    const size_t length = wcsnlen(value.c_str(), value.size());
    value.resize(length);
    return value;
}

bool WriteViewerSettingString(const wchar_t* name, const std::wstring& value)
{
    HKEY key = nullptr;
    if (name == nullptr ||
        RegCreateKeyExW(HKEY_CURRENT_USER, kViewerSettingsKey, 0, nullptr, 0, KEY_SET_VALUE,
                        nullptr, &key, nullptr) != ERROR_SUCCESS)
        return false;
    const DWORD bytes = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    const LSTATUS status = RegSetValueExW(key, name, 0, REG_SZ,
                                          reinterpret_cast<const BYTE*>(value.c_str()), bytes);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

std::wstring CopyString(const wchar_t* value)
{
    return value != nullptr ? value : L"";
}

std::wstring ControlLabel(const std::wstring& menuLabel)
{
    std::wstring result = menuLabel.substr(0, menuLabel.find(L'\t'));
    result.erase(std::remove(result.begin(), result.end(), L'&'), result.end());
    return result;
}

std::wstring ToIoPath(const std::wstring& path)
{
    if (path.empty() || path.rfind(L"\\\\?\\", 0) == 0)
        return path;
    if (path.rfind(L"\\\\", 0) == 0)
        return L"\\\\?\\UNC\\" + path.substr(2);
    if (path.size() >= MAX_PATH)
        return L"\\\\?\\" + path;
    return path;
}

std::wstring FileNameOf(const std::wstring& path)
{
    size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? path : path.substr(slash + 1);
}

std::wstring ExtensionOf(const std::wstring& path)
{
    std::wstring name = FileNameOf(path);
    size_t dot = name.find_last_of(L'.');
    if (dot == std::wstring::npos)
        return L"";
    std::wstring ext = name.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), towlower);
    return ext;
}

std::wstring DirectoryOf(const std::wstring& path)
{
    size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"" : path.substr(0, slash);
}

std::wstring PrismLanguageForExtension(const std::wstring& extension)
{
    std::wstring value = extension.empty() ? L"none" : extension.substr(1);
    struct Mapping { const wchar_t* extension; const wchar_t* language; };
    static const Mapping mappings[] = {
        {L"axaml", L"markup"}, {L"bat", L"batch"}, {L"cmd", L"batch"}, {L"config", L"markup"},
        {L"cs", L"csharp"}, {L"csproj", L"markup"}, {L"cxx", L"cpp"}, {L"fsproj", L"markup"},
        {L"h", L"c"}, {L"hh", L"cpp"}, {L"hpp", L"cpp"}, {L"hxx", L"cpp"},
        {L"htm", L"markup"}, {L"html", L"markup"}, {L"js", L"javascript"},
        {L"jsonc", L"json"}, {L"json5", L"json"}, {L"md", L"markdown"},
        {L"markdown", L"markdown"}, {L"nuspec", L"markup"}, {L"plist", L"markup"},
        {L"props", L"markup"}, {L"ps1", L"powershell"}, {L"py", L"python"},
        {L"psd1", L"powershell"}, {L"psm1", L"powershell"}, {L"storyboard", L"markup"},
        {L"reg", L"properties"}, {L"rb", L"ruby"}, {L"sh", L"bash"},
        {L"targets", L"markup"}, {L"ts", L"typescript"}, {L"vcxproj", L"markup"},
        {L"vcproj", L"markup"}, {L"vbproj", L"markup"}, {L"xaml", L"markup"},
        {L"xlf", L"markup"}, {L"xml", L"markup"}, {L"yml", L"yaml"}
    };
    for (const Mapping& mapping : mappings)
        if (value == mapping.extension)
            return mapping.language;
    return value;
}

std::wstring ModuleDirectory(HINSTANCE module);
std::wstring PrismAssetsDirectory();

std::vector<std::wstring> InstalledPrismLanguages()
{
    std::vector<std::wstring> languages;
    const std::wstring folder = PrismAssetsDirectory();
    if (folder.empty())
        return languages;
    WIN32_FIND_DATAW data = {};
    HANDLE find = FindFirstFileW(ToIoPath(folder + L"\\components\\prism-*.min.js").c_str(), &data);
    if (find == INVALID_HANDLE_VALUE)
        return languages;
    do
    {
        std::wstring name = data.cFileName;
        constexpr size_t prefixLength = 6;
        constexpr size_t suffixLength = 7;
        if (name.size() > prefixLength + suffixLength &&
            name.compare(0, prefixLength, L"prism-") == 0 &&
            name.compare(name.size() - suffixLength, suffixLength, L".min.js") == 0)
        {
            name = name.substr(prefixLength, name.size() - prefixLength - suffixLength);
            if (name != L"core" && name.find(L"-extras") == std::wstring::npos &&
                name != L"javadoclike" && name != L"markup-templating")
                languages.push_back(name);
        }
    } while (FindNextFileW(find, &data));
    FindClose(find);
    std::sort(languages.begin(), languages.end());
    languages.erase(std::unique(languages.begin(), languages.end()), languages.end());
    return languages;
}

std::wstring HtmlEncode(const std::wstring& value)
{
    std::wstring result;
    result.reserve(value.size() + value.size() / 8);
    for (wchar_t ch : value)
    {
        switch (ch)
        {
        case L'&': result += L"&amp;"; break;
        case L'<': result += L"&lt;"; break;
        case L'>': result += L"&gt;"; break;
        case L'\"': result += L"&quot;"; break;
        default: result += ch; break;
        }
    }
    return result;
}

std::wstring WithBaseElement(std::wstring html, const std::wstring& baseUri)
{
    if (baseUri.empty())
        return html;
    std::wstring lower = html;
    std::transform(lower.begin(), lower.end(), lower.begin(), towlower);
    if (lower.find(L"<base") != std::wstring::npos)
        return html;
    const std::wstring element = L"<base href='" + HtmlEncode(baseUri) + L"'>";
    size_t head = lower.find(L"<head");
    if (head != std::wstring::npos)
    {
        size_t end = lower.find(L'>', head);
        if (end != std::wstring::npos)
        {
            html.insert(end + 1, element);
            return html;
        }
    }
    size_t root = lower.find(L"<html");
    if (root != std::wstring::npos)
    {
        size_t end = lower.find(L'>', root);
        if (end != std::wstring::npos)
        {
            html.insert(end + 1, L"<head>" + element + L"</head>");
            return html;
        }
    }
    return element + html;
}

std::wstring CssColor(COLORREF color)
{
    wchar_t value[8];
    swprintf_s(value, L"#%02X%02X%02X", GetRValue(color), GetGValue(color), GetBValue(color));
    return value;
}

COLORREF BlendColor(COLORREF background, COLORREF foreground, int foregroundPercent)
{
    const int backgroundPercent = 100 - foregroundPercent;
    return RGB((GetRValue(background) * backgroundPercent + GetRValue(foreground) * foregroundPercent) / 100,
               (GetGValue(background) * backgroundPercent + GetGValue(foreground) * foregroundPercent) / 100,
               (GetBValue(background) * backgroundPercent + GetBValue(foreground) * foregroundPercent) / 100);
}

bool ReadFileBytes(const std::wstring& path, std::vector<unsigned char>& bytes)
{
    HANDLE file = CreateFileW(ToIoPath(path).c_str(), GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    LARGE_INTEGER size = {};
    bool ok = GetFileSizeEx(file, &size) != FALSE && size.QuadPart >= 0 &&
              size.QuadPart <= 256LL * 1024LL * 1024LL;
    if (ok)
    {
        bytes.resize(static_cast<size_t>(size.QuadPart));
        size_t offset = 0;
        while (offset < bytes.size())
        {
            DWORD chunk = static_cast<DWORD>(std::min<size_t>(bytes.size() - offset, 1024 * 1024));
            DWORD read = 0;
            if (!ReadFile(file, bytes.data() + offset, chunk, &read, nullptr) || read == 0)
            {
                ok = false;
                break;
            }
            offset += read;
        }
    }
    CloseHandle(file);
    return ok;
}

std::wstring DecodeText(const std::vector<unsigned char>& bytes)
{
    if (bytes.empty())
        return L"";

    const unsigned char* data = bytes.data();
    size_t size = bytes.size();
    UINT codePage = CP_UTF8;
    if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
    {
        data += 3;
        size -= 3;
    }
    else if (size >= 2 && data[0] == 0xFF && data[1] == 0xFE)
    {
        data += 2;
        size -= 2;
        std::wstring result;
        result.reserve(size / 2);
        for (size_t i = 0; i + 1 < size; i += 2)
            result.push_back(static_cast<wchar_t>(data[i] | (data[i + 1] << 8)));
        return result;
    }
    else if (size >= 2 && data[0] == 0xFE && data[1] == 0xFF)
    {
        data += 2;
        size -= 2;
        std::wstring result;
        result.reserve(size / 2);
        for (size_t i = 0; i + 1 < size; i += 2)
            result.push_back(static_cast<wchar_t>((data[i] << 8) | data[i + 1]));
        return result;
    }

    // HTML replaces embedded U+0000 with U+FFFD. Detect the common BOM-less
    // UTF-16 case before treating the input as an 8-bit encoding.
    if (size >= 4)
    {
        const size_t pairs = (std::min)(size / 2, static_cast<size_t>(2048));
        size_t zeroEven = 0;
        size_t zeroOdd = 0;
        for (size_t i = 0; i < pairs; ++i)
        {
            zeroEven += data[i * 2] == 0;
            zeroOdd += data[i * 2 + 1] == 0;
        }
        const bool littleEndian = zeroOdd * 5 > pairs * 3 && zeroEven * 5 < pairs;
        const bool bigEndian = zeroEven * 5 > pairs * 3 && zeroOdd * 5 < pairs;
        if (littleEndian || bigEndian)
        {
            std::wstring result;
            result.reserve(size / 2);
            for (size_t i = 0; i + 1 < size; i += 2)
            {
                unsigned int value = littleEndian ? data[i] | (data[i + 1] << 8)
                                                  : (data[i] << 8) | data[i + 1];
                result.push_back(static_cast<wchar_t>(value));
            }
            return result;
        }
    }

    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                    reinterpret_cast<const char*>(data), static_cast<int>(size), nullptr, 0);
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (count <= 0)
    {
        // CP_ACP follows the user's Windows locale. On a Western code page,
        // valid Windows-1250 bytes can decode to U+FFFD or to unrelated Latin-1
        // characters. Text handled by these viewers uses UTF or Windows-1250,
        // so use CP1250 as the deterministic legacy fallback.
        codePage = 1250;
        flags = 0;
        count = MultiByteToWideChar(codePage, flags, reinterpret_cast<const char*>(data),
                                    static_cast<int>(size), nullptr, 0);
    }
    if (count <= 0)
        return L"";
    std::wstring result(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(codePage, flags, reinterpret_cast<const char*>(data),
                        static_cast<int>(size), result.data(), count);
    return result;
}

std::wstring PathToFileUri(const std::wstring& path)
{
    DWORD chars = 0;
    UrlCreateFromPathW(path.c_str(), nullptr, &chars, 0);
    if (chars == 0)
        return L"";
    std::wstring uri(chars, L'\0');
    if (FAILED(UrlCreateFromPathW(path.c_str(), uri.data(), &chars, 0)))
        return L"";
    uri.resize(chars);
    return uri;
}

std::string WideToUtf8(const std::wstring& value);

std::wstring UrlEncodePathSegment(const std::wstring& value)
{
    std::string utf8 = WideToUtf8(value);
    static const wchar_t hex[] = L"0123456789ABCDEF";
    std::wstring encoded;
    encoded.reserve(utf8.size() * 3);
    for (unsigned char ch : utf8)
    {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' || ch == '~')
            encoded.push_back(static_cast<wchar_t>(ch));
        else
        {
            encoded.push_back(L'%');
            encoded.push_back(hex[ch >> 4]);
            encoded.push_back(hex[ch & 15]);
        }
    }
    return encoded;
}

std::wstring ModuleDirectory(HINSTANCE module)
{
    std::vector<wchar_t> path(512);
    DWORD length = 0;
    for (;;)
    {
        length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0)
            return L"";
        if (length < path.size() - 1)
            break;
        path.resize(path.size() * 2);
    }
    std::wstring result(path.data(), length);
    size_t slash = result.find_last_of(L"\\/");
    if (slash == std::wstring::npos)
        return L"";
    result.resize(slash + 1);
    return result;
}

std::wstring PrismAssetsDirectory()
{
    return ModuleDirectory(nullptr) + L"plugins\\salamatrix\\prism";
}

bool WriteAll(HANDLE handle, const void* data, size_t size)
{
    const unsigned char* current = static_cast<const unsigned char*>(data);
    while (size != 0)
    {
        DWORD chunk = static_cast<DWORD>((std::min)(size, static_cast<size_t>(1024 * 1024)));
        DWORD written = 0;
        if (!WriteFile(handle, current, chunk, &written, nullptr) || written == 0)
            return false;
        current += written;
        size -= written;
    }
    return true;
}

bool ReadAll(HANDLE handle, void* data, size_t size)
{
    unsigned char* current = static_cast<unsigned char*>(data);
    while (size != 0)
    {
        DWORD chunk = static_cast<DWORD>((std::min)(size, static_cast<size_t>(1024 * 1024)));
        DWORD read = 0;
        if (!ReadFile(handle, current, chunk, &read, nullptr) || read == 0)
            return false;
        current += read;
        size -= read;
    }
    return true;
}

std::string WideToUtf8(const std::wstring& value)
{
    if (value.empty())
        return {};
    int count = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                                    nullptr, 0, nullptr, nullptr);
    if (count <= 0)
        return {};
    std::string result(static_cast<size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                        result.data(), count, nullptr, nullptr);
    return result;
}

std::wstring Utf8ToWideText(const std::string& value)
{
    if (value.empty())
        return {};
    int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0)
        return {};
    std::wstring result(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), count);
    return result;
}

std::wstring Utf8ToWide(const std::vector<unsigned char>& value)
{
    if (value.empty())
        return {};
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                    reinterpret_cast<const char*>(value.data()),
                                    static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0)
        return {};
    std::wstring result(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                        reinterpret_cast<const char*>(value.data()), static_cast<int>(value.size()),
                        result.data(), count);
    return result;
}

bool RenderMarkdown(HINSTANCE module, const std::wstring& markdown, std::wstring& html, std::wstring& error)
{
    UNREFERENCED_PARAMETER(module);
    std::wstring executable = ModuleDirectory(nullptr) + L"utils\\MarkdigRenderer.exe";
    if (GetFileAttributesW(ToIoPath(executable).c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        error = L"MarkdigRenderer.exe was not found.";
        return false;
    }

    SECURITY_ATTRIBUTES security = {sizeof(security), nullptr, TRUE};
    HANDLE childInputRead = nullptr;
    HANDLE parentInputWrite = nullptr;
    HANDLE parentOutputRead = nullptr;
    HANDLE childOutputWrite = nullptr;
    if (!CreatePipe(&childInputRead, &parentInputWrite, &security, 0) ||
        !CreatePipe(&parentOutputRead, &childOutputWrite, &security, 0))
    {
        error = L"Unable to create Markdig renderer pipes.";
        if (childInputRead) CloseHandle(childInputRead);
        if (parentInputWrite) CloseHandle(parentInputWrite);
        if (parentOutputRead) CloseHandle(parentOutputRead);
        if (childOutputWrite) CloseHandle(childOutputWrite);
        return false;
    }
    SetHandleInformation(parentInputWrite, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(parentOutputRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW startup = {sizeof(startup)};
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = childInputRead;
    startup.hStdOutput = childOutputWrite;
    startup.hStdError = childOutputWrite;
    PROCESS_INFORMATION process = {};
    std::wstring command = L"\"" + executable + L"\"";
    BOOL created = CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
    CloseHandle(childInputRead);
    CloseHandle(childOutputWrite);
    if (!created)
    {
        CloseHandle(parentInputWrite);
        CloseHandle(parentOutputRead);
        error = L"Unable to start MarkdigRenderer.exe.";
        return false;
    }

    std::string utf8 = WideToUtf8(markdown);
    uint32_t requestSize = static_cast<uint32_t>(utf8.size());
    bool ok = WriteAll(parentInputWrite, &requestSize, sizeof(requestSize)) &&
              WriteAll(parentInputWrite, utf8.data(), utf8.size());
    CloseHandle(parentInputWrite);

    unsigned char responseHeader[5] = {};
    ok = ok && ReadAll(parentOutputRead, responseHeader, sizeof(responseHeader));
    uint32_t responseSize = 0;
    if (ok)
        memcpy(&responseSize, responseHeader + 1, sizeof(responseSize));
    if (responseSize > 512U * 1024U * 1024U)
        ok = false;
    std::vector<unsigned char> response;
    if (ok)
    {
        response.resize(responseSize);
        ok = ReadAll(parentOutputRead, response.data(), response.size());
    }
    CloseHandle(parentOutputRead);

    DWORD wait = WaitForSingleObject(process.hProcess, 30000);
    if (wait == WAIT_TIMEOUT)
    {
        TerminateProcess(process.hProcess, 1);
        error = L"The Markdig renderer timed out.";
        ok = false;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (!ok)
    {
        if (error.empty())
            error = L"The Markdig renderer returned an invalid response.";
        return false;
    }
    std::wstring value = Utf8ToWide(response);
    if (responseHeader[0] == 0)
    {
        error = value;
        return false;
    }
    html = std::move(value);
    return true;
}

void AddLazyImageAttributes(std::wstring& html)
{
    const auto hasAttribute = [](const std::wstring& tag, const wchar_t* name)
    {
        const size_t nameLength = wcslen(name);
        size_t found = 0;
        while ((found = tag.find(name, found)) != std::wstring::npos)
        {
            const bool startsAttribute = found > 0 && iswspace(tag[found - 1]);
            size_t after = found + nameLength;
            while (after < tag.size() && iswspace(tag[after]))
                ++after;
            if (startsAttribute && (after == tag.size() || tag[after] == L'=' ||
                                    tag[after] == L'/' || tag[after] == L'>'))
                return true;
            found += nameLength;
        }
        return false;
    };
    size_t position = 0;
    while ((position = html.find(L'<', position)) != std::wstring::npos)
    {
        if (position + 4 > html.size() ||
            towlower(html[position + 1]) != L'i' ||
            towlower(html[position + 2]) != L'm' ||
            towlower(html[position + 3]) != L'g' ||
            (position + 4 < html.size() && !iswspace(html[position + 4]) &&
             html[position + 4] != L'/' && html[position + 4] != L'>'))
        {
            ++position;
            continue;
        }
        wchar_t quote = 0;
        size_t end = position + 4;
        for (; end < html.size(); ++end)
        {
            const wchar_t ch = html[end];
            if (quote != 0)
            {
                if (ch == quote)
                    quote = 0;
            }
            else if (ch == L'\'' || ch == L'"')
                quote = ch;
            else if (ch == L'>')
                break;
        }
        if (end == html.size())
            break;
        std::wstring tag = html.substr(position, end - position + 1);
        std::wstring lower = tag;
        std::transform(lower.begin(), lower.end(), lower.begin(), towlower);
        std::wstring attributes;
        if (!hasAttribute(lower, L"loading"))
            attributes += L" loading=\"lazy\"";
        if (!hasAttribute(lower, L"decoding"))
            attributes += L" decoding=\"async\"";
        if (!hasAttribute(lower, L"fetchpriority"))
            attributes += L" fetchpriority=\"low\"";
        if (!attributes.empty())
        {
            const size_t insert = end > position && html[end - 1] == L'/' ? end - 1 : end;
            html.insert(insert, attributes);
            position = end + attributes.size() + 1;
        }
        else
            position = end + 1;
    }
}

enum class PreparationKind
{
    PrismVirtual,
    Markdown
};

struct PreparationTarget
{
    std::atomic<long> references{1};
    std::mutex lock;
    bool alive = true;
};

void ReleasePreparationTarget(PreparationTarget* target)
{
    if (target != nullptr && target->references.fetch_sub(1) == 1)
        delete target;
}

struct PreparationContext
{
    HWND window = nullptr;
    PreparationTarget* target = nullptr;
    uint64_t generation = 0;
    HINSTANCE module = nullptr;
    std::wstring path;
    bool markdown = false;
};

struct PreparationResult
{
    uint64_t generation = 0;
    PreparationKind kind = PreparationKind::PrismVirtual;
    DWORD error = ERROR_SUCCESS;
    std::wstring text;
    std::wstring renderError;
    std::vector<size_t> lineStarts;
};

DWORD WINAPI PrepareDocumentWorker(void* parameter)
{
    std::unique_ptr<PreparationContext> context(static_cast<PreparationContext*>(parameter));
    PreparationResult* result = NewNoThrow<PreparationResult>();
    if (result == nullptr)
    {
        {
            std::lock_guard<std::mutex> guard(context->target->lock);
            if (context->target->alive)
                PostMessageW(context->window, WM_NV_PREPARATION_COMPLETE,
                             static_cast<WPARAM>(context->generation), 0);
        }
        ReleasePreparationTarget(context->target);
        return 0;
    }
    result->generation = context->generation;

    std::vector<unsigned char> bytes;
    if (!ReadFileBytes(context->path, bytes))
    {
        const DWORD error = GetLastError();
        result->error = error == ERROR_SUCCESS ? ERROR_READ_FAULT : error;
    }
    else if (bytes.size() > (context->markdown ? NV_MARKDOWN_FILE_LIMIT : NV_PRISM_FILE_LIMIT))
        result->error = ERROR_FILE_TOO_LARGE;
    else
    {
        result->text = DecodeText(bytes);
        if (context->markdown)
        {
            result->kind = PreparationKind::Markdown;
            std::wstring fragment;
            if (!RenderMarkdown(context->module, result->text, fragment, result->renderError))
                result->error = ERROR_INVALID_DATA;
            else
            {
                AddLazyImageAttributes(fragment);
                result->text = std::move(fragment);
            }
        }
        else
        {
            result->kind = PreparationKind::PrismVirtual;
            result->lineStarts.reserve(1 + result->text.size() / 40);
            result->lineStarts.push_back(0);
            for (size_t index = 0; index < result->text.size(); ++index)
                if (result->text[index] == L'\n')
                    result->lineStarts.push_back(index + 1);
        }
    }

    {
        std::lock_guard<std::mutex> guard(context->target->lock);
        if (!context->target->alive ||
            !PostMessageW(context->window, WM_NV_PREPARATION_COMPLETE, 0,
                          reinterpret_cast<LPARAM>(result)))
            delete result;
    }
    ReleasePreparationTarget(context->target);
    return 0;
}

std::wstring JsonString(const std::wstring& value)
{
    static const wchar_t hex[] = L"0123456789ABCDEF";
    std::wstring result;
    result.reserve(value.size() + 2);
    result.push_back(L'"');
    for (size_t index = 0; index < value.size(); ++index)
    {
        const wchar_t ch = value[index];
        switch (ch)
        {
        case L'"': result += L"\\\""; break;
        case L'\\': result += L"\\\\"; break;
        case L'\b': result += L"\\b"; break;
        case L'\f': result += L"\\f"; break;
        case L'\n': result += L"\\n"; break;
        case L'\r': result += L"\\r"; break;
        case L'\t': result += L"\\t"; break;
        default:
            if (ch < 0x20 || (ch >= 0xD800 && ch <= 0xDFFF))
            {
                wchar_t escaped[] = {L'\\', L'u', hex[(ch >> 12) & 15], hex[(ch >> 8) & 15],
                                     hex[(ch >> 4) & 15], hex[ch & 15], 0};
                if (ch >= 0xD800 && ch <= 0xDBFF &&
                    (index + 1 >= value.size() || value[index + 1] < 0xDC00 || value[index + 1] > 0xDFFF))
                    result += L"\\uFFFD";
                else if (ch >= 0xDC00 && ch <= 0xDFFF &&
                         (index == 0 || value[index - 1] < 0xD800 || value[index - 1] > 0xDBFF))
                    result += L"\\uFFFD";
                else
                    result += escaped;
            }
            else
                result.push_back(ch);
            break;
        }
    }
    result.push_back(L'"');
    return result;
}

bool IsMarkupPrismLanguage(std::wstring language)
{
    std::transform(language.begin(), language.end(), language.begin(), towlower);
    static const wchar_t* const names[] = {
        L"markup", L"xml", L"html", L"htm", L"xhtml", L"svg", L"mathml", L"ssml",
        L"atom", L"rss", L"config", L"targets", L"props", L"csproj", L"fsproj",
        L"vbproj", L"vcxproj", L"vcproj", L"xaml", L"axaml", L"nuspec", L"plist",
        L"storyboard", L"xlf"
    };
    for (const wchar_t* name : names)
        if (language == name)
            return true;
    return false;
}

size_t RFindTokenBefore(const std::wstring& text, const wchar_t* token, size_t offset)
{
    const size_t tokenLength = wcslen(token);
    if (offset < tokenLength)
        return std::wstring::npos;
    return text.rfind(token, offset - tokenLength);
}

bool MarkupCommentOpenAt(const std::wstring& text, size_t offset)
{
    const size_t open = RFindTokenBefore(text, L"<!--", offset);
    if (open == std::wstring::npos)
        return false;
    const size_t close = RFindTokenBefore(text, L"-->", offset);
    return close == std::wstring::npos || open > close;
}

struct ViewerParameters
{
    HINSTANCE module = nullptr;
    HWND owner = nullptr;
    std::wstring filePath;
    RECT placement = {};
    UINT showCommand = SW_SHOWNORMAL;
    bool alwaysOnTop = false;
    HANDLE closeEvent = nullptr;
    NativeViewerKind kind = NativeViewerKind::RenderDocument;
    NativeViewerTheme theme = {};
    LOGFONT menuFont = {};
    CSalamanderGUIAbstract* gui = nullptr;
    std::wstring pluginName;
    std::wstring fileMenu;
    std::wstring viewMenu;
    std::wstring close;
    std::wstring refresh;
    std::wstring zoomIn;
    std::wstring zoomOut;
    std::wstring zoomReset;
    std::wstring lineNumbers;
    std::wstring wrapLines;
    std::wstring showWhitespace;
    std::wstring loading;
    std::wstring ready;
    std::wstring initializationFailed;
    std::wstring openFailed;
    std::wstring syntaxHighlighter;
    std::wstring automatic;
    std::wstring colors;
    std::wstring visualStudio;
    std::wstring defaultPrism;
    std::wstring customPalette;
    std::wstring editCustom;
    std::wstring editCustomTitle;
    std::wstring save;
    std::wstring cancel;
    std::wstring light;
    std::wstring dark;
    std::wstring tokenComment;
    std::wstring tokenPunctuation;
    std::wstring tokenKeyword;
    std::wstring tokenControlKeyword;
    std::wstring tokenClassName;
    std::wstring tokenFunction;
    std::wstring tokenString;
    std::wstring tokenNumber;
    std::wstring tokenBoolean;
    std::wstring tokenVariable;
    std::wstring tokenNamespace;
    std::wstring tokenRegex;
    std::wstring saveFailed;
    std::wstring uiUnavailable;
    LOGFONT viewerFont = {};
    CSalamanderGeneralAbstract* general = nullptr;
};

constexpr size_t kPrismTokenCount = 12;
const char* const kPrismTokenKeys[kPrismTokenCount] = {
    "comment", "punctuation", "keyword", "controlKeyword", "className",
    "function", "string", "number", "boolean", "variable", "namespace", "regex"
};
const COLORREF kPrismTokenLightDefaults[kPrismTokenCount] = {
    RGB(0x00, 0x80, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xff), RGB(0x00, 0x00, 0xff),
    RGB(0x2b, 0x91, 0xaf), RGB(0x00, 0x00, 0x00), RGB(0xa3, 0x15, 0x15), RGB(0x09, 0x86, 0x58),
    RGB(0x00, 0x00, 0xff), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x81, 0x1f, 0x3f)
};
const COLORREF kPrismTokenDarkDefaults[kPrismTokenCount] = {
    RGB(0x6a, 0x99, 0x55), RGB(0xd4, 0xd4, 0xd4), RGB(0x56, 0x9c, 0xd6), RGB(0xc5, 0x86, 0xc0),
    RGB(0x4e, 0xc9, 0xb0), RGB(0xdc, 0xdc, 0xaa), RGB(0xce, 0x91, 0x78), RGB(0xb5, 0xce, 0xa8),
    RGB(0x56, 0x9c, 0xd6), RGB(0x9c, 0xdc, 0xfe), RGB(0xd4, 0xd4, 0xd4), RGB(0xd1, 0x69, 0x69)
};

std::wstring CustomPalettePath()
{
    return PrismAssetsDirectory() + L"\\viewer\\custom-palette.json";
}

std::string Utf8Or(const std::wstring& value, const char* fallback)
{
    std::string utf8 = WideToUtf8(value);
    return utf8.empty() ? std::string(fallback) : utf8;
}

std::string ColorToHex(COLORREF color)
{
    char text[8];
    sprintf_s(text, "#%02x%02x%02x", GetRValue(color), GetGValue(color), GetBValue(color));
    return text;
}

bool ParseHexColor(const std::string& text, COLORREF& color)
{
    if (text.size() != 7 || text[0] != '#')
        return false;
    unsigned red = 0;
    unsigned green = 0;
    unsigned blue = 0;
    if (sscanf_s(text.c_str() + 1, "%02x%02x%02x", &red, &green, &blue) != 3)
        return false;
    color = RGB(red, green, blue);
    return true;
}

bool FindJsonObject(const std::string& json, const char* name, size_t& begin, size_t& end)
{
    const std::string key = std::string("\"") + name + "\"";
    size_t pos = json.find(key);
    if (pos == std::string::npos)
        return false;
    pos = json.find('{', pos);
    if (pos == std::string::npos)
        return false;
    int depth = 1;
    size_t index = pos + 1;
    while (index < json.size() && depth > 0)
    {
        if (json[index] == '{')
            ++depth;
        else if (json[index] == '}')
            --depth;
        ++index;
    }
    if (depth != 0)
        return false;
    begin = pos;
    end = index;
    return true;
}

bool ReadTokenColor(const std::string& json, size_t begin, size_t end, const char* key, COLORREF& color)
{
    const std::string needle = std::string("\"") + key + "\"";
    size_t pos = json.find(needle, begin);
    if (pos == std::string::npos || pos >= end)
        return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos || pos >= end)
        return false;
    pos = json.find('"', pos);
    if (pos == std::string::npos || pos + 8 >= end)
        return false;
    return ParseHexColor(json.substr(pos + 1, 7), color);
}

bool ParseCustomPaletteJson(const std::string& json, COLORREF light[kPrismTokenCount], COLORREF dark[kPrismTokenCount])
{
    size_t lightBegin = 0;
    size_t lightEnd = 0;
    size_t darkBegin = 0;
    size_t darkEnd = 0;
    if (!FindJsonObject(json, "light", lightBegin, lightEnd) ||
        !FindJsonObject(json, "dark", darkBegin, darkEnd))
        return false;
    for (size_t index = 0; index < kPrismTokenCount; ++index)
    {
        ReadTokenColor(json, lightBegin, lightEnd, kPrismTokenKeys[index], light[index]);
        ReadTokenColor(json, darkBegin, darkEnd, kPrismTokenKeys[index], dark[index]);
    }
    return true;
}

bool ReadCustomPaletteFile(COLORREF light[kPrismTokenCount], COLORREF dark[kPrismTokenCount])
{
    memcpy(light, kPrismTokenLightDefaults, sizeof(kPrismTokenLightDefaults));
    memcpy(dark, kPrismTokenDarkDefaults, sizeof(kPrismTokenDarkDefaults));
    const std::wstring stored = ReadViewerSettingString(L"PrismCustomPalette");
    if (!stored.empty() && ParseCustomPaletteJson(WideToUtf8(stored), light, dark))
        return true;
    const std::wstring path = CustomPalettePath();
    HANDLE handle = CreateFileW(ToIoPath(path).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        return false;
    LARGE_INTEGER size = {};
    std::string json;
    bool ok = GetFileSizeEx(handle, &size) && size.QuadPart > 0 && size.QuadPart < 64 * 1024;
    if (ok)
    {
        json.resize(static_cast<size_t>(size.QuadPart));
        ok = ReadAll(handle, json.data(), json.size());
    }
    CloseHandle(handle);
    return ok && ParseCustomPaletteJson(json, light, dark);
}

std::string FormatPaletteMapsJson(const COLORREF light[kPrismTokenCount], const COLORREF dark[kPrismTokenCount])
{
    std::string json = "{\"light\":{";
    for (size_t index = 0; index < kPrismTokenCount; ++index)
    {
        json += "\"";
        json += kPrismTokenKeys[index];
        json += "\":\"";
        json += ColorToHex(light[index]);
        json += index + 1 < kPrismTokenCount ? "\"," : "\"";
    }
    json += "},\"dark\":{";
    for (size_t index = 0; index < kPrismTokenCount; ++index)
    {
        json += "\"";
        json += kPrismTokenKeys[index];
        json += "\":\"";
        json += ColorToHex(dark[index]);
        json += index + 1 < kPrismTokenCount ? "\"," : "\"";
    }
    json += "}}";
    return json;
}

std::string FormatCustomPaletteJson(const COLORREF light[kPrismTokenCount], const COLORREF dark[kPrismTokenCount])
{
    std::string json =
        "{\n"
        "  \"_comment\": \"Prism Text Viewer custom token colors. Choose Colors > Custom, or Colors > Edit Custom.\",\n"
        "  \"light\": {\n";
    for (size_t index = 0; index < kPrismTokenCount; ++index)
    {
        json += "    \"";
        json += kPrismTokenKeys[index];
        json += "\": \"";
        json += ColorToHex(light[index]);
        json += index + 1 < kPrismTokenCount ? "\",\n" : "\"\n";
    }
    json += "  },\n  \"dark\": {\n";
    for (size_t index = 0; index < kPrismTokenCount; ++index)
    {
        json += "    \"";
        json += kPrismTokenKeys[index];
        json += "\": \"";
        json += ColorToHex(dark[index]);
        json += index + 1 < kPrismTokenCount ? "\",\n" : "\"\n";
    }
    json += "  }\n}\n";
    return json;
}

void AppendCustomPaletteJson(std::wstring& json, const COLORREF* light, const COLORREF* dark)
{
    COLORREF lightColors[kPrismTokenCount];
    COLORREF darkColors[kPrismTokenCount];
    if (light == nullptr || dark == nullptr)
        ReadCustomPaletteFile(lightColors, darkColors);
    json += L",\"custom\":";
    json += Utf8ToWideText(FormatPaletteMapsJson(
        light != nullptr ? light : lightColors,
        dark != nullptr ? dark : darkColors));
}

bool WriteCustomPaletteFile(const COLORREF light[kPrismTokenCount], const COLORREF dark[kPrismTokenCount])
{
    const std::string json = FormatCustomPaletteJson(light, dark);
    const bool stored = WriteViewerSettingString(L"PrismCustomPalette", Utf8ToWideText(json));
    const std::wstring path = CustomPalettePath();
    const std::wstring ioPath = ToIoPath(path);
    HANDLE handle = CreateFileW(ioPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        return stored;
    const bool written = WriteAll(handle, json.data(), json.size());
    CloseHandle(handle);
    return written || stored;
}

Salamatrix::UI::IUIService* QueryViewerUI(CSalamanderGeneralAbstract* general)
{
    if (general == nullptr)
        return nullptr;
    CSalamanderServiceQuery query = {};
    query.ServiceId = SALAMATRIX_SERVICE_UI;
    query.MinimumVersion = SALAMATRIX_UI_VERSION_1_2;
    CSalamanderServiceResult result = {};
    return general->QueryService(&query, &result)
               ? static_cast<Salamatrix::UI::IUIService*>(result.Interface)
               : nullptr;
}

Salamatrix::UI::IControl* AddPaletteControl(
    Salamatrix::UI::IDialog* dialog,
    Salamatrix::UI::ControlKind kind,
    const char* id,
    const char* text,
    int x, int y, int width, int height,
    int dialogResult = 0)
{
    Salamatrix::UI::ControlOptions options;
    options.Id = id;
    options.Text = text;
    options.DialogResult = dialogResult;
    Salamatrix::UI::ControlLayout layout;
    layout.HasBounds = TRUE;
    layout.X = x;
    layout.Y = y;
    layout.Width = width;
    layout.Height = height;
    return dialog->AddControlEx(kind, options, layout);
}

class ViewerWindow
{
public:
    explicit ViewerWindow(std::unique_ptr<ViewerParameters> parameters)
        : parameters_(std::move(parameters)), preparationTarget_(NewNoThrow<PreparationTarget>())
    {
        zoomPercent_ = static_cast<int>(ReadViewerSetting(L"ZoomPercent", 100));
        zoomPercent_ = (std::max)(25, (std::min)(zoomPercent_, 500));
        if (parameters_->kind == NativeViewerKind::PrismText)
        {
            showLineNumbers_ = ReadViewerSetting(L"PrismLineNumbers", 0) != 0;
            wrapLines_ = ReadViewerSetting(L"PrismWrapLines", 0) != 0;
            showWhitespace_ = ReadViewerSetting(L"PrismShowWhitespace", 0) != 0;
            colorPalette_ = ReadViewerSetting(L"PrismColorPalette", NV_PALETTE_VISUAL_STUDIO);
            if (colorPalette_ > NV_PALETTE_CUSTOM)
                colorPalette_ = NV_PALETTE_VISUAL_STUDIO;
            automaticLanguage_ = PrismLanguageForExtension(ExtensionOf(parameters_->filePath));
            installedLanguages_ = InstalledPrismLanguages();
            activeLanguage_ = automaticLanguage_;
        }
    }
    ~ViewerWindow()
    {
        ClosePreparationTarget();
        CloseBrowser();
        if (parameters_->gui != nullptr && menuBar_ != nullptr)
            parameters_->gui->DestroyMenuBar(menuBar_);
        if (parameters_->gui != nullptr && mainMenu_ != nullptr)
            parameters_->gui->DestroyMenuPopup(mainMenu_);
        if (menuFont_ != nullptr && menuFont_ != GetStockObject(DEFAULT_GUI_FONT))
            DeleteObject(menuFont_);
        if (parameters_->closeEvent != nullptr)
            SetEvent(parameters_->closeEvent);
    }

    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        ViewerWindow* self = reinterpret_cast<ViewerWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<ViewerWindow*>(create->lpCreateParams);
            self->window_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self == nullptr)
            return DefWindowProcW(window, message, wParam, lParam);
        LRESULT result = self->HandleMessage(message, wParam, lParam);
        if (message == WM_NCDESTROY)
        {
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            self->ClosePreparationTarget();
            delete self;
        }
        return result;
    }

    bool Create()
    {
        const wchar_t* className = parameters_->kind == NativeViewerKind::PrismText
                                       ? L"OpenSalamander.NativePrismViewer"
                                       : L"OpenSalamander.NativeWebViewViewer";
        WNDCLASSEXW cls = {sizeof(cls)};
        cls.lpfnWndProc = WindowProc;
        cls.hInstance = parameters_->module;
        cls.hIcon = static_cast<HICON>(LoadImageW(parameters_->module, MAKEINTRESOURCEW(8000), IMAGE_ICON,
                                                  GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
        cls.hIconSm = static_cast<HICON>(LoadImageW(parameters_->module, MAKEINTRESOURCEW(8000), IMAGE_ICON,
                                                    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
        cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
        // Do not let USER paint COLOR_WINDOW before the WebView controller and
        // document exist.  WM_ERASEBKGND below uses the viewer's actual theme.
        cls.hbrBackground = nullptr;
        cls.lpszClassName = className;
        RegisterClassExW(&cls);

        int width = (std::max)(parameters_->placement.right - parameters_->placement.left, 320L);
        int height = (std::max)(parameters_->placement.bottom - parameters_->placement.top, 240L);
        std::wstring title = WindowTitle();
        window_ = CreateWindowExW(parameters_->alwaysOnTop ? WS_EX_TOPMOST : 0, className, title.c_str(),
                                  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                  parameters_->placement.left, parameters_->placement.top, width, height,
                                  nullptr, nullptr, parameters_->module, this);
        return window_ != nullptr;
    }

    HWND Window() const { return window_; }
    void Show()
    {
        ShowWindow(window_, parameters_->showCommand);
        // A new viewer is an explicit open request.  Do not leave it behind an
        // already open viewer window on the host UI thread.
        BringWindowToTop(window_);
        SetForegroundWindow(window_);
        UpdateWindow(window_);
    }

private:
    struct SyntaxMenuItem
    {
        int command;
        std::wstring language;
        CGUIMenuPopupAbstract* menu;
    };

    CGUIMenuPopupAbstract* mainMenu_ = nullptr;
    CGUIMenuBarAbstract* menuBar_ = nullptr;
    CGUIMenuPopupAbstract* colorsMenu_ = nullptr;
    CGUIMenuPopupAbstract* syntaxMenu_ = nullptr;

    std::wstring WindowTitle() const
    {
        std::wstring title = FileNameOf(parameters_->filePath) + L" - " + parameters_->pluginName;
        if (parameters_->kind == NativeViewerKind::PrismText)
            title += L" - [" + (activeLanguage_.empty() ? std::wstring(L"none") : activeLanguage_) + L"]";
        return title;
    }

    void UpdateWindowTitle()
    {
        if (window_ != nullptr)
        {
            const std::wstring title = WindowTitle();
            SetWindowTextW(window_, title.c_str());
        }
    }

    HMENU CreateMenuBar()
    {
        HMENU bar = CreateMenu();
        HMENU file = CreatePopupMenu();
        AppendMenuW(file, MF_STRING, IDM_NV_CLOSE, parameters_->close.c_str());
        // Native menu bars do not add enough breathing room for these short
        // captions. Padding also gives the first item a left inset.
        std::wstring fileCaption = L"  " + parameters_->fileMenu + L"  ";
        AppendMenuW(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(file), fileCaption.c_str());
        HMENU view = CreatePopupMenu();
        AppendMenuW(view, MF_STRING, IDM_NV_REFRESH, parameters_->refresh.c_str());
        AppendMenuW(view, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(view, MF_STRING, IDM_NV_ZOOM_IN, parameters_->zoomIn.c_str());
        AppendMenuW(view, MF_STRING, IDM_NV_ZOOM_OUT, parameters_->zoomOut.c_str());
        AppendMenuW(view, MF_STRING, IDM_NV_ZOOM_RESET, parameters_->zoomReset.c_str());
        if (parameters_->kind == NativeViewerKind::PrismText)
        {
            AppendMenuW(view, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(view, MF_STRING, IDM_NV_LINE_NUMBERS, parameters_->lineNumbers.c_str());
            AppendMenuW(view, MF_STRING, IDM_NV_WRAP_LINES, parameters_->wrapLines.c_str());
            AppendMenuW(view, MF_STRING, IDM_NV_SHOW_WHITESPACE, parameters_->showWhitespace.c_str());
        }
        std::wstring viewCaption = L"  " + parameters_->viewMenu + L"  ";
        AppendMenuW(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(view), viewCaption.c_str());
        if (parameters_->kind == NativeViewerKind::PrismText)
        {
            HMENU colors = CreatePopupMenu();
            AppendMenuW(colors, MF_STRING, IDM_NV_COLORS_VISUAL_STUDIO,
                        (parameters_->visualStudio.empty() ? L"&Visual Studio" : parameters_->visualStudio.c_str()));
            AppendMenuW(colors, MF_STRING, IDM_NV_COLORS_PRISM,
                        (parameters_->defaultPrism.empty() ? L"Default &Prism" : parameters_->defaultPrism.c_str()));
            AppendMenuW(colors, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(colors, MF_STRING, IDM_NV_COLORS_CUSTOM,
                        (parameters_->customPalette.empty() ? L"&Custom" : parameters_->customPalette.c_str()));
            AppendMenuW(colors, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(colors, MF_STRING, IDM_NV_COLORS_EDIT_CUSTOM,
                        (parameters_->editCustom.empty() ? L"Edit &Custom" : parameters_->editCustom.c_str()));
            const std::wstring colorsCaption = L"  " +
                (parameters_->colors.empty() ? std::wstring(L"&Colors") : parameters_->colors) + L"  ";
            AppendMenuW(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(colors), colorsCaption.c_str());
            HMENU syntax = CreatePopupMenu();
            AppendMenuW(syntax, MF_STRING, IDM_NV_SYNTAX_AUTOMATIC,
                        (parameters_->automatic.empty() ? L"&Automatic" : parameters_->automatic.c_str()));
            AppendMenuW(syntax, MF_SEPARATOR, 0, nullptr);
            wchar_t currentGroup = 0;
            HMENU groupMenu = nullptr;
            for (size_t index = 0; index < installedLanguages_.size(); ++index)
            {
                const std::wstring& language = installedLanguages_[index];
                wchar_t group = language.empty() ? L'#' : static_cast<wchar_t>(towupper(language[0]));
                if (group < L'A' || group > L'Z')
                    group = L'#';
                if (group != currentGroup)
                {
                    currentGroup = group;
                    groupMenu = CreatePopupMenu();
                    const std::wstring groupCaption(1, group);
                    AppendMenuW(syntax, MF_POPUP, reinterpret_cast<UINT_PTR>(groupMenu), groupCaption.c_str());
                }
                AppendMenuW(groupMenu, MF_STRING, IDM_NV_SYNTAX_FIRST + static_cast<UINT>(index),
                            language.c_str());
            }
            const std::wstring syntaxCaption = L"  " +
                (parameters_->syntaxHighlighter.empty() ? std::wstring(L"Syntax &Highlighter")
                                                        : parameters_->syntaxHighlighter) + L"  ";
            AppendMenuW(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(syntax), syntaxCaption.c_str());
        }
        return bar;
    }

    static std::string Utf8MenuText(const std::wstring& text)
    {
        const int bytes = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string result(bytes > 0 ? bytes : 0, '\0');
        if (bytes > 0)
            WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &result[0], bytes, nullptr, nullptr);
        if (!result.empty())
            result.pop_back();
        return result;
    }

    static BOOL AddMenuItem(CGUIMenuPopupAbstract* menu, const std::wstring& text, DWORD id,
                            CGUIMenuPopupAbstract* subMenu = nullptr)
    {
        std::string utf8 = Utf8MenuText(text);
        MENU_ITEM_INFO item = {};
        item.Mask = MENU_MASK_TYPE | MENU_MASK_STRING | MENU_MASK_ID | MENU_MASK_SUBMENU;
        item.Type = MENU_TYPE_STRING;
        item.ID = id;
        item.String = const_cast<char*>(utf8.c_str());
        item.SubMenu = subMenu;
        return menu->InsertItem(-1, TRUE, &item);
    }

    static BOOL AddMenuSeparator(CGUIMenuPopupAbstract* menu)
    {
        MENU_ITEM_INFO item = {};
        item.Mask = MENU_MASK_TYPE;
        item.Type = MENU_TYPE_SEPARATOR;
        return menu->InsertItem(-1, TRUE, &item);
    }

    bool CreateSyntaxHighlighterMenu()
    {
        syntaxMenu_ = parameters_->gui->CreateMenuPopup();
        if (syntaxMenu_ == nullptr)
            return false;
        AddMenuItem(syntaxMenu_, parameters_->automatic.empty() ? L"&Automatic" : parameters_->automatic,
                    IDM_NV_SYNTAX_AUTOMATIC);
        AddMenuSeparator(syntaxMenu_);
        wchar_t currentGroup = 0;
        CGUIMenuPopupAbstract* groupMenu = nullptr;
        for (size_t index = 0; index < installedLanguages_.size(); ++index)
        {
            const std::wstring& language = installedLanguages_[index];
            wchar_t group = language.empty() ? L'#' : static_cast<wchar_t>(towupper(language[0]));
            if (group < L'A' || group > L'Z')
                group = L'#';
            if (group != currentGroup)
            {
                currentGroup = group;
                groupMenu = parameters_->gui->CreateMenuPopup();
                if (groupMenu == nullptr)
                    return false;
                AddMenuItem(syntaxMenu_, std::wstring(1, group), 0, groupMenu);
            }
            const int command = IDM_NV_SYNTAX_FIRST + static_cast<int>(index);
            AddMenuItem(groupMenu, language, command);
            syntaxMenuItems_.push_back({command, language, groupMenu});
        }
        return true;
    }

    bool CreateColorsMenu()
    {
        colorsMenu_ = parameters_->gui->CreateMenuPopup();
        if (colorsMenu_ == nullptr)
            return false;
        AddMenuItem(colorsMenu_, parameters_->visualStudio.empty() ? L"&Visual Studio" : parameters_->visualStudio,
                    IDM_NV_COLORS_VISUAL_STUDIO);
        AddMenuItem(colorsMenu_, parameters_->defaultPrism.empty() ? L"Default &Prism" : parameters_->defaultPrism,
                    IDM_NV_COLORS_PRISM);
        AddMenuSeparator(colorsMenu_);
        AddMenuItem(colorsMenu_, parameters_->customPalette.empty() ? L"&Custom" : parameters_->customPalette,
                    IDM_NV_COLORS_CUSTOM);
        AddMenuSeparator(colorsMenu_);
        AddMenuItem(colorsMenu_, parameters_->editCustom.empty() ? L"Edit &Custom" : parameters_->editCustom,
                    IDM_NV_COLORS_EDIT_CUSTOM);
        return true;
    }

    bool CreateCustomMenuBar()
    {
        if (parameters_->gui == nullptr)
            return false;
        mainMenu_ = parameters_->gui->CreateMenuPopup();
        CGUIMenuPopupAbstract* file = parameters_->gui->CreateMenuPopup();
        CGUIMenuPopupAbstract* view = parameters_->gui->CreateMenuPopup();
        if (mainMenu_ == nullptr || file == nullptr || view == nullptr)
            return false;
        AddMenuItem(file, parameters_->close, IDM_NV_CLOSE);
        AddMenuItem(view, parameters_->refresh, IDM_NV_REFRESH);
        AddMenuSeparator(view);
        AddMenuItem(view, parameters_->zoomIn, IDM_NV_ZOOM_IN);
        AddMenuItem(view, parameters_->zoomOut, IDM_NV_ZOOM_OUT);
        AddMenuItem(view, parameters_->zoomReset, IDM_NV_ZOOM_RESET);
        if (parameters_->kind == NativeViewerKind::PrismText)
        {
            AddMenuSeparator(view);
            AddMenuItem(view, parameters_->lineNumbers, IDM_NV_LINE_NUMBERS);
            AddMenuItem(view, parameters_->wrapLines, IDM_NV_WRAP_LINES);
            AddMenuItem(view, parameters_->showWhitespace, IDM_NV_SHOW_WHITESPACE);
            if (!CreateColorsMenu() || !CreateSyntaxHighlighterMenu())
                return false;
        }
        if (!AddMenuItem(mainMenu_, parameters_->fileMenu, 0, file) ||
            !AddMenuItem(mainMenu_, parameters_->viewMenu, 0, view) ||
            (parameters_->kind == NativeViewerKind::PrismText &&
             (!AddMenuItem(mainMenu_,
                           parameters_->colors.empty() ? L"&Colors" : parameters_->colors,
                           0, colorsMenu_) ||
              !AddMenuItem(mainMenu_,
                          parameters_->syntaxHighlighter.empty() ? L"Syntax &Highlighter"
                                                                 : parameters_->syntaxHighlighter,
                          0, syntaxMenu_))))
            return false;
        menuBar_ = parameters_->gui->CreateMenuBar(mainMenu_, window_);
        if (menuBar_ == nullptr || !menuBar_->CreateWnd(window_))
            return false;
        menuBar_->SetFont();
        return true;
    }

    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        LRESULT colorResult = 0;
        if (PluginDarkMode_HandleCtlColor(message, wParam, lParam, &colorResult))
            return colorResult;

        switch (message)
        {
        case WM_ERASEBKGND:
        {
            RECT client = {};
            GetClientRect(window_, &client);
            HBRUSH brush = CreateSolidBrush(parameters_->theme.background);
            if (brush != nullptr)
            {
                FillRect(reinterpret_cast<HDC>(wParam), &client, brush);
                DeleteObject(brush);
            }
            return 1;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT paint = {};
            HDC dc = BeginPaint(window_, &paint);
            HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(DC_BRUSH));
            COLORREF oldColor = SetDCBrushColor(dc, parameters_->theme.background);
            FillRect(dc, &paint.rcPaint, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
            SetDCBrushColor(dc, oldColor);
            SelectObject(dc, oldBrush);
            EndPaint(window_, &paint);
            return 0;
        }
        case WM_UAHDRAWMENUITEM:
            if (!parameters_->theme.dark && lParam != 0)
            {
                PaintLightMenuBarItem(reinterpret_cast<NativeViewerUAHDrawMenuItem*>(lParam));
                return 0;
            }
            break;
        case WM_CREATE:
        {
            if (!CreateCustomMenuBar())
                return -1;
            status_ = CreateWindowExW(0, STATUSCLASSNAMEW, parameters_->loading.c_str(),
                                       WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBARS_SIZEGRIP, 0, 0, 0, 0,
                                       window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_NV_STATUS)), parameters_->module, nullptr);
            const std::wstring resetLabel = ControlLabel(parameters_->zoomReset);
            zoomReset_ = CreateWindowExW(0, L"BUTTON", resetLabel.c_str(),
                                          WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON | BS_FLAT,
                                          0, 0, 0, 0, window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_NV_ZOOM_RESET)), parameters_->module, nullptr);
            zoomOut_ = CreateWindowExW(0, L"BUTTON", L"-", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON | BS_FLAT,
                                        0, 0, 0, 0, window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_NV_ZOOM_OUT)), parameters_->module, nullptr);
            zoomEdit_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"100 %", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | ES_CENTER | ES_AUTOHSCROLL,
                                         0, 0, 0, 0, window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_NV_ZOOM_EDIT)), parameters_->module, nullptr);
            zoomIn_ = CreateWindowExW(0, L"BUTTON", L"+", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON | BS_FLAT,
                                       0, 0, 0, 0, window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_NV_ZOOM_IN)), parameters_->module, nullptr);
            ApplyChromeFont();
            ApplyMenuFont();
            UpdateZoomDisplay(zoomPercent_);
            UpdateViewMenuChecks();
            ApplyTheme();
            BeginBrowserInitialization();
            return 0;
        }
        case WM_SIZE:
            ResizeChildren();
            return 0;
        case WM_COMMAND:
            HandleCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
                DestroyWindow(window_);
            return 0;
        case WM_SETTINGCHANGE:
        case WM_THEMECHANGED:
            ApplyTheme();
            return 0;
        case WM_NV_CLOSE_ALL:
            DestroyWindow(window_);
            return 0;
        case WM_NV_APPLY_ZOOM:
            ApplyZoomEdit();
            return 0;
        case WM_NV_PREPARATION_COMPLETE:
            HandlePreparationResult(reinterpret_cast<PreparationResult*>(lParam),
                                    static_cast<uint64_t>(wParam));
            return 0;
        case WM_CLOSE:
            DestroyWindow(window_);
            return 0;
        case WM_DESTROY:
            RemoveWindow();
            return 0;
        }
        return DefWindowProcW(window_, message, wParam, lParam);
    }

    void ApplyTheme()
    {
        PluginDarkMode_SetHostPolicyAvailable(TRUE, parameters_->theme.dark ? TRUE : FALSE);
        PluginDarkMode_SetHostColors(parameters_->theme.foreground, parameters_->theme.background);
        PluginDarkMode_ApplyTitleBar(window_);
        PluginDarkMode_ApplyMenuBar(window_);
        PluginDarkMode_ApplyStatusBar(status_);
        PluginDarkMode_ApplyListTreeThemeRecursive(window_);
        RedrawWindow(window_, nullptr, nullptr,
                     RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
    }

    void SetLoadProgress(int percent)
    {
        loadProgress_ = (std::max)(0, (std::min)(percent, 100));
        if (status_ == nullptr || loadProgress_ >= 100)
            return;
        SetWindowTextW(status_, (parameters_->loading + L" " + std::to_wstring(loadProgress_) + L" %").c_str());
    }

    void CompletePrismDisplay()
    {
        SetWindowTextW(status_, parameters_->ready.c_str());
        ShowPrismBrowser();
        loadProgress_ = 100;
    }

    void ShowPrismBrowser()
    {
        ApplyControllerZoom();
        if (!browserVisible_ && controller_)
        {
            controller_->put_IsVisible(TRUE);
            browserVisible_ = true;
        }
    }

    void ApplyControllerZoom()
    {
        if (controller_)
            controller_->put_ZoomFactor(static_cast<double>(zoomPercent_) / 100.0);
    }

    void ApplyChromeFont()
    {
        HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        for (HWND control : {status_, zoomReset_, zoomOut_, zoomEdit_, zoomIn_})
            if (control != nullptr)
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }

    void ApplyMenuFont()
    {
        if (menuFont_ != nullptr && menuFont_ != GetStockObject(DEFAULT_GUI_FONT))
            DeleteObject(menuFont_);
        menuFont_ = CreateFontIndirect(&parameters_->menuFont);
        if (menuFont_ == nullptr)
            menuFont_ = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        SendMessageW(window_, WM_SETFONT, reinterpret_cast<WPARAM>(menuFont_), FALSE);
        SetPropW(window_, L"OpenSalamander.UIFont", menuFont_);
    }

    void PaintLightMenuBarItem(const NativeViewerUAHDrawMenuItem* item)
    {
        if (item == nullptr || menuFont_ == nullptr)
            return;
        wchar_t text[MAX_PATH] = {};
        MENUITEMINFOW info = {};
        info.cbSize = sizeof(info);
        info.fMask = MIIM_STRING;
        info.dwTypeData = text;
        info.cch = _countof(text) - 1;
        if (!GetMenuItemInfoW(item->menu.menu, static_cast<UINT>(item->item.position), TRUE, &info))
            return;
        const bool selected = (item->draw.itemState & (ODS_SELECTED | ODS_HOTLIGHT)) != 0;
        FillRect(item->menu.dc, &item->draw.rcItem,
                 GetSysColorBrush(selected ? COLOR_MENUHILIGHT : COLOR_MENUBAR));
        HGDIOBJ oldFont = SelectObject(item->menu.dc, menuFont_);
        int oldBkMode = SetBkMode(item->menu.dc, TRANSPARENT);
        COLORREF oldText = SetTextColor(item->menu.dc,
                                        GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));
        RECT rect = item->draw.rcItem;
        DrawTextW(item->menu.dc, text, -1, &rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER |
                  ((item->draw.itemState & ODS_NOACCEL) != 0 ? DT_HIDEPREFIX : 0));
        SetTextColor(item->menu.dc, oldText);
        SetBkMode(item->menu.dc, oldBkMode);
        SelectObject(item->menu.dc, oldFont);
    }

    void BeginBrowserInitialization()
    {
        SetLoadProgress(15);
        if (gSharedEnvironment)
        {
            CreateBrowserController(gSharedEnvironment.Get());
            return;
        }

        gPendingEnvironmentWindows.push_back(window_);
        if (gCreatingSharedEnvironment)
            return;

        gCreatingSharedEnvironment = true;
        std::wstring userData = ViewerUserDataFolder();
        CreateWebView2EnvironmentFn createEnvironment = GetCreateWebView2Environment();
        HRESULT hr = createEnvironment != nullptr
            ? createEnvironment(nullptr, userData.empty() ? nullptr : userData.c_str(),
            nullptr, Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT
                {
                    if (FAILED(result) || environment == nullptr)
                    {
                        for (HWND target : gPendingEnvironmentWindows)
                        {
                            ViewerWindow* self = IsWindow(target)
                                ? reinterpret_cast<ViewerWindow*>(GetWindowLongPtrW(target, GWLP_USERDATA)) : nullptr;
                            if (self != nullptr)
                                self->ShowError(self->parameters_->initializationFailed, result);
                        }
                        gPendingEnvironmentWindows.clear();
                        gCreatingSharedEnvironment = false;
                        return S_OK;
                    }
                    gSharedEnvironment = environment;
                    std::vector<HWND> pending;
                    pending.swap(gPendingEnvironmentWindows);
                    gCreatingSharedEnvironment = false;
                    for (HWND target : pending)
                    {
                        ViewerWindow* self = IsWindow(target)
                            ? reinterpret_cast<ViewerWindow*>(GetWindowLongPtrW(target, GWLP_USERDATA)) : nullptr;
                        if (self != nullptr)
                            self->CreateBrowserController(gSharedEnvironment.Get());
                    }
                    return S_OK;
                }).Get())
            : HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
        if (FAILED(hr))
        {
            gCreatingSharedEnvironment = false;
            std::vector<HWND> pending;
            pending.swap(gPendingEnvironmentWindows);
            for (HWND target : pending)
            {
                ViewerWindow* self = IsWindow(target)
                    ? reinterpret_cast<ViewerWindow*>(GetWindowLongPtrW(target, GWLP_USERDATA)) : nullptr;
                if (self != nullptr)
                    self->ShowError(self->parameters_->initializationFailed, hr);
            }
        }
    }

public:
    void CreateBrowserController(ICoreWebView2Environment* environment)
    {
        if (environment == nullptr)
            return;
        SetLoadProgress(35);
        const HWND target = window_;
        environment->CreateCoreWebView2Controller(target,
            Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [target](HRESULT controllerResult, ICoreWebView2Controller* controller) -> HRESULT
                {
                    ViewerWindow* self = IsWindow(target)
                        ? reinterpret_cast<ViewerWindow*>(GetWindowLongPtrW(target, GWLP_USERDATA)) : nullptr;
                    if (self == nullptr)
                        return S_OK;
                    if (FAILED(controllerResult) || controller == nullptr)
                    {
                        self->ShowError(self->parameters_->initializationFailed, controllerResult);
                        return S_OK;
                    }
                    self->controller_ = controller;
                    // The WebView default is white.  Keep the controller hidden
                    // while assigning its background and loading the first page,
                    // otherwise a dark viewer visibly flashes white on opening.
                    self->controller_->put_IsVisible(FALSE);
                    self->controller_->get_CoreWebView2(&self->webView_);
                    self->SetLoadProgress(55);
                    self->ConfigureBrowser();
                    self->ResizeChildren();
                    self->LoadDocument();
                    return S_OK;
                }).Get());
    }

private:
    void ConfigureBrowser()
    {
        ComPtr<ICoreWebView2Controller2> controller2;
        if (SUCCEEDED(controller_.As(&controller2)) && controller2)
        {
            COREWEBVIEW2_COLOR background = {
                255,
                GetRValue(parameters_->theme.background),
                GetGValue(parameters_->theme.background),
                GetBValue(parameters_->theme.background)};
            controller2->put_DefaultBackgroundColor(background);
        }
        ComPtr<ICoreWebView2Settings> settings;
        if (SUCCEEDED(webView_->get_Settings(&settings)) && settings)
        {
            settings->put_IsStatusBarEnabled(FALSE);
            settings->put_AreDefaultContextMenusEnabled(TRUE);
            settings->put_AreDevToolsEnabled(TRUE);
            settings->put_IsZoomControlEnabled(TRUE);
        }
        ComPtr<ICoreWebView2_13> webView13;
        if (SUCCEEDED(webView_.As(&webView13)))
        {
            ComPtr<ICoreWebView2Profile> profile;
            if (SUCCEEDED(webView13->get_Profile(&profile)) && profile)
                profile->put_PreferredColorScheme(parameters_->theme.dark
                    ? COREWEBVIEW2_PREFERRED_COLOR_SCHEME_DARK
                    : COREWEBVIEW2_PREFERRED_COLOR_SCHEME_LIGHT);
        }
        const HWND viewerWindow = window_;
        webView_->add_NavigationStarting(
            Callback<ICoreWebView2NavigationStartingEventHandler>(
                [viewerWindow](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs*) -> HRESULT
                {
                    ViewerWindow* self = IsWindow(viewerWindow)
                        ? reinterpret_cast<ViewerWindow*>(GetWindowLongPtrW(viewerWindow, GWLP_USERDATA)) : nullptr;
                    if (self != nullptr)
                        self->SetLoadProgress(90);
                    return S_OK;
                }).Get(), &navigationStartingToken_);
        ComPtr<ICoreWebView2_2> webView2;
        if (SUCCEEDED(webView_.As(&webView2)) && webView2)
        {
            webView2->add_DOMContentLoaded(
                Callback<ICoreWebView2DOMContentLoadedEventHandler>(
                    [viewerWindow](ICoreWebView2*, ICoreWebView2DOMContentLoadedEventArgs*) -> HRESULT
                    {
                        ViewerWindow* self = IsWindow(viewerWindow)
                            ? reinterpret_cast<ViewerWindow*>(GetWindowLongPtrW(viewerWindow, GWLP_USERDATA)) : nullptr;
                        if (self == nullptr || self->parameters_->kind != NativeViewerKind::RenderDocument)
                            return S_OK;
                        if (self->parameters_->theme.dark)
                        {
                            std::wstring script = L"(function(){if(!document||!document.documentElement)return;"
                                L"document.documentElement.style.colorScheme='dark';"
                                L"var s=document.getElementById('salamander-viewer-dark');if(!s){s=document.createElement('style');"
                                L"s.id='salamander-viewer-dark';s.textContent=':where(html,body){background-color:" +
                                CssColor(self->parameters_->theme.background) + L";color:" +
                                CssColor(self->parameters_->theme.foreground) +
                                L"}:where(a:link){color:" + CssColor(self->parameters_->theme.accent) +
                                L"}';document.head.appendChild(s);}})();";
                            self->webView_->ExecuteScript(script.c_str(), nullptr);
                        }
                        SetWindowTextW(self->status_, self->parameters_->ready.c_str());
                        if (!self->browserVisible_ && self->controller_)
                        {
                            self->controller_->put_IsVisible(TRUE);
                            self->browserVisible_ = true;
                        }
                        self->loadProgress_ = 100;
                        return S_OK;
                    }).Get(), &domContentLoadedToken_);
        }
        controller_->add_ZoomFactorChanged(
            Callback<ICoreWebView2ZoomFactorChangedEventHandler>(
                [viewerWindow](ICoreWebView2Controller* controller, IUnknown*) -> HRESULT
                {
                    ViewerWindow* self = IsWindow(viewerWindow)
                        ? reinterpret_cast<ViewerWindow*>(GetWindowLongPtrW(viewerWindow, GWLP_USERDATA)) : nullptr;
                    if (self != nullptr)
                    {
                        double zoom = 1.0;
                        if (SUCCEEDED(controller->get_ZoomFactor(&zoom)))
                        {
                            self->UpdateZoomDisplay(static_cast<int>(zoom * 100.0 + 0.5));
                            WriteViewerSetting(L"ZoomPercent", static_cast<DWORD>(self->zoomPercent_));
                            if (self->parameters_->kind == NativeViewerKind::PrismText && self->webView_)
                                self->webView_->PostWebMessageAsString(L"salamander-resize");
                        }
                    }
                    return S_OK;
                }).Get(), &zoomChangedToken_);
        controller_->add_AcceleratorKeyPressed(
            Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
                [viewerWindow](ICoreWebView2Controller*, ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT
                {
                    UINT virtualKey = 0;
                    COREWEBVIEW2_KEY_EVENT_KIND kind = COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN;
                    args->get_VirtualKey(&virtualKey);
                    args->get_KeyEventKind(&kind);
                    if (kind != COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN && kind != COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN)
                        return S_OK;
                    int command = 0;
                    if (virtualKey == VK_ESCAPE)
                    {
                        args->put_Handled(TRUE);
                        PostMessageW(viewerWindow, WM_CLOSE, 0, 0);
                    }
                    else if (virtualKey == VK_F5)
                        command = IDM_NV_REFRESH;
                    else if (virtualKey == VK_F2)
                        command = IDM_NV_WRAP_LINES;
                    else if ((GetKeyState(VK_CONTROL) & 0x8000) != 0)
                    {
                        if (virtualKey == '0' || virtualKey == VK_NUMPAD0)
                            command = IDM_NV_ZOOM_RESET;
                        else if (virtualKey == VK_OEM_PLUS || virtualKey == VK_ADD)
                            command = IDM_NV_ZOOM_IN;
                        else if (virtualKey == VK_OEM_MINUS || virtualKey == VK_SUBTRACT)
                            command = IDM_NV_ZOOM_OUT;
                    }
                    if (command != 0)
                    {
                        args->put_Handled(TRUE);
                        PostMessageW(viewerWindow, WM_COMMAND, command, 0);
                    }
                    return S_OK;
                }).Get(), &acceleratorToken_);
        webView_->add_WebMessageReceived(
                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                    [viewerWindow](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
                    {
                        ViewerWindow* self = IsWindow(viewerWindow)
                            ? reinterpret_cast<ViewerWindow*>(GetWindowLongPtrW(viewerWindow, GWLP_USERDATA)) : nullptr;
                        if (self == nullptr)
                            return S_OK;
                        LPWSTR message = nullptr;
                        if (args == nullptr || FAILED(args->TryGetWebMessageAsString(&message)) || message == nullptr)
                            return S_OK;

                        const std::wstring value(message);
                        CoTaskMemFree(message);
                        if (value == L"salamander-prism-ready")
                        {
                            self->CompletePrismDisplay();
                            return S_OK;
                        }
                        if (value == L"salamander-prism-theme-ready")
                        {
                            self->ShowPrismBrowser();
                            return S_OK;
                        }
                        if (value == L"salamander-virtual-ready")
                        {
                            self->virtualInitSent_ = false;
                            self->PostVirtualInit();
                            return S_OK;
                        }
                        constexpr wchar_t chunkPrefix[] = L"salamander-chunk:";
                        if (value.compare(0, std::size(chunkPrefix) - 1, chunkPrefix) == 0)
                        {
                            self->HandleVirtualChunk(value);
                            return S_OK;
                        }
                        if (self->parameters_->kind != NativeViewerKind::RenderDocument)
                            return S_OK;
                        constexpr wchar_t prefix[] = L"salamander-link:";
                        if (value == L"salamander-link-clear")
                            SetWindowTextW(self->status_, self->parameters_->ready.c_str());
                        else if (value.compare(0, std::size(prefix) - 1, prefix) == 0)
                            SetWindowTextW(self->status_, value.c_str() + (std::size(prefix) - 1));
                        return S_OK;
                    }).Get(), &webMessageToken_);
        webView_->AddWebResourceRequestedFilter(
            L"https://markdown.local/*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_DOCUMENT);
        webView_->add_WebResourceRequested(
            Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                [viewerWindow](ICoreWebView2*, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT
                {
                    ViewerWindow* self = IsWindow(viewerWindow)
                        ? reinterpret_cast<ViewerWindow*>(GetWindowLongPtrW(viewerWindow, GWLP_USERDATA)) : nullptr;
                    if (self == nullptr || args == nullptr || self->markdownDocumentUri_.empty())
                        return S_OK;
                    ComPtr<ICoreWebView2WebResourceRequest> request;
                    LPWSTR uri = nullptr;
                    if (FAILED(args->get_Request(&request)) || !request ||
                        FAILED(request->get_Uri(&uri)) || uri == nullptr)
                        return S_OK;
                    const bool matches = self->markdownDocumentUri_ == uri;
                    CoTaskMemFree(uri);
                    if (!matches || self->markdownDocumentUtf8_.size() > UINT_MAX || !gSharedEnvironment)
                        return S_OK;
                    ComPtr<IStream> content;
                    content.Attach(SHCreateMemStream(
                        reinterpret_cast<const BYTE*>(self->markdownDocumentUtf8_.data()),
                        static_cast<UINT>(self->markdownDocumentUtf8_.size())));
                    if (!content)
                        return E_OUTOFMEMORY;
                    ComPtr<ICoreWebView2WebResourceResponse> response;
                    HRESULT hr = gSharedEnvironment->CreateWebResourceResponse(
                        content.Get(), 200, L"OK",
                        L"Content-Type: text/html; charset=utf-8\r\nCache-Control: no-store",
                        &response);
                    if (SUCCEEDED(hr) && response)
                        hr = args->put_Response(response.Get());
                    return hr;
                }).Get(), &webResourceRequestedToken_);

        if (parameters_->kind == NativeViewerKind::RenderDocument)
        {
            constexpr wchar_t hoverScript[] =
                L"(function(){document.addEventListener('mouseover',function(e){var a=e.target&&e.target.closest?e.target.closest('a'):null;"
                L"if(a&&a.href)window.chrome.webview.postMessage('salamander-link:'+a.href);});"
                L"document.addEventListener('mouseout',function(e){var a=e.target&&e.target.closest?e.target.closest('a'):null;"
                L"if(a&&!a.contains(e.relatedTarget))window.chrome.webview.postMessage('salamander-link-clear');});})();";
            webView_->AddScriptToExecuteOnDocumentCreated(hoverScript, nullptr);
        }
        webView_->add_NavigationCompleted(
            Callback<ICoreWebView2NavigationCompletedEventHandler>(
                [viewerWindow](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
                {
                    ViewerWindow* self = IsWindow(viewerWindow)
                        ? reinterpret_cast<ViewerWindow*>(GetWindowLongPtrW(viewerWindow, GWLP_USERDATA)) : nullptr;
                    if (self == nullptr)
                        return S_OK;
                    BOOL success = FALSE;
                    args->get_IsSuccess(&success);
                    if (success)
                        self->ApplyControllerZoom();
                    if (!success)
                    {
                        SetWindowTextW(self->status_, self->parameters_->openFailed.c_str());
                        if (!self->browserVisible_ && self->controller_)
                        {
                            self->controller_->put_IsVisible(TRUE);
                            self->browserVisible_ = true;
                        }
                        self->loadProgress_ = 100;
                    }
                    return S_OK;
                }).Get(), &navigationToken_);
    }

    std::wstring MapLocalDocument()
    {
        ComPtr<ICoreWebView2_3> webView3;
        std::wstring folder = DirectoryOf(parameters_->filePath);
        if (folder.empty() || FAILED(webView_.As(&webView3)))
            return L"";
        HRESULT hr = webView3->SetVirtualHostNameToFolderMapping(
            L"document.local", folder.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
        if (FAILED(hr))
            return L"";
        return L"https://document.local/" + UrlEncodePathSegment(FileNameOf(parameters_->filePath));
    }

    struct PrismStyleMetrics
    {
        double fontSize = 13;
        double lineHeight = 13;
        double charWidth = 8;
        double gutterWidth = 17;
        std::wstring fontFace = L"Consolas";
        int fontWeight = FW_NORMAL;
        std::wstring fontStyle = L"normal";
        std::wstring textDecoration = L"none";
        std::wstring gutterForeground;
        std::wstring gutterBackground;
    };

    PrismStyleMetrics GetPrismStyleMetrics(size_t lineCount)
    {
        PrismStyleMetrics style;
        TEXTMETRIC viewerMetrics = {};
        viewerMetrics.tmHeight = parameters_->viewerFont.lfHeight != 0
                                     ? abs(parameters_->viewerFont.lfHeight) : 13;
        viewerMetrics.tmAveCharWidth = 8;
        HDC fontDC = GetDC(window_);
        HFONT viewerFont = CreateFontIndirect(&parameters_->viewerFont);
        if (fontDC != nullptr && viewerFont != nullptr)
        {
            HFONT oldFont = static_cast<HFONT>(SelectObject(fontDC, viewerFont));
            GetTextMetrics(fontDC, &viewerMetrics);
            SelectObject(fontDC, oldFont);
        }
        if (viewerFont != nullptr)
            DeleteObject(viewerFont);
        if (fontDC != nullptr)
            ReleaseDC(window_, fontDC);
        style.fontSize = parameters_->viewerFont.lfHeight != 0
                           ? abs(parameters_->viewerFont.lfHeight) : 13;
        style.lineHeight = (std::max)(viewerMetrics.tmHeight, 1L);
        const double charPixelWidth = (std::max)(viewerMetrics.tmAveCharWidth, 1L);
        style.charWidth = charPixelWidth;
        int lineDigits = 1;
        for (size_t value = lineCount; value >= 10; value /= 10)
            ++lineDigits;
        style.gutterWidth = 1.0 + (lineDigits + 1) * style.charWidth;
        if (parameters_->viewerFont.lfFaceName[0] != '\0')
        {
            int length = MultiByteToWideChar(CP_ACP, 0, parameters_->viewerFont.lfFaceName,
                                             -1, nullptr, 0);
            if (length > 1)
            {
                std::vector<wchar_t> face(static_cast<size_t>(length));
                if (MultiByteToWideChar(CP_ACP, 0, parameters_->viewerFont.lfFaceName, -1,
                                        face.data(), length) > 0)
                    style.fontFace.assign(face.data());
            }
        }
        style.fontWeight = parameters_->viewerFont.lfWeight > 0
                             ? parameters_->viewerFont.lfWeight : FW_NORMAL;
        style.fontStyle = parameters_->viewerFont.lfItalic ? L"italic" : L"normal";
        style.textDecoration = parameters_->viewerFont.lfUnderline ? L"underline"
                               : parameters_->viewerFont.lfStrikeOut ? L"line-through" : L"none";
        style.gutterBackground = parameters_->theme.dark ? L"#262626" : L"#f5f5f5";
        style.gutterForeground = parameters_->theme.dark ? L"#a0a0a0" : L"#606060";
        return style;
    }

    void ClosePreparationTarget()
    {
        if (preparationTarget_ == nullptr)
            return;
        {
            std::lock_guard<std::mutex> guard(preparationTarget_->lock);
            preparationTarget_->alive = false;
            preparationGeneration_ = gNextPreparationGeneration.fetch_add(1);
            MSG pending = {};
            while (PeekMessageW(&pending, window_, WM_NV_PREPARATION_COMPLETE,
                                WM_NV_PREPARATION_COMPLETE, PM_REMOVE))
                delete reinterpret_cast<PreparationResult*>(pending.lParam);
        }
        ReleasePreparationTarget(preparationTarget_);
        preparationTarget_ = nullptr;
    }

    void StartPreparation(bool markdown)
    {
        if (preparationTarget_ == nullptr)
        {
            ShowError(parameters_->openFailed, E_OUTOFMEMORY);
            return;
        }
        preparationGeneration_ = gNextPreparationGeneration.fetch_add(1);
        virtualGeneration_ = 0;
        virtualInitSent_ = false;
        virtualText_.clear();
        virtualLineStarts_.clear();
        PreparationContext* context = NewNoThrow<PreparationContext>();
        if (context == nullptr)
        {
            ShowError(parameters_->openFailed, E_OUTOFMEMORY);
            return;
        }
        context->window = window_;
        context->target = preparationTarget_;
        preparationTarget_->references.fetch_add(1);
        context->generation = preparationGeneration_;
        context->module = parameters_->module;
        context->path = parameters_->filePath;
        context->markdown = markdown;
        HANDLE worker = CreateThread(nullptr, 0, PrepareDocumentWorker, context, 0, nullptr);
        if (worker == nullptr)
        {
            ReleasePreparationTarget(context->target);
            delete context;
            ShowError(parameters_->openFailed, HRESULT_FROM_WIN32(GetLastError()));
            return;
        }
        CloseHandle(worker);
    }

    void NavigateMarkdown(const std::wstring& fragment)
    {
        std::wstring mappedUri = MapLocalDocument();
        std::wstring base = mappedUri.empty() ? L"" : L"https://document.local/";
        COLORREF codeBackground = BlendColor(parameters_->theme.background, parameters_->theme.foreground,
                                              parameters_->theme.dark ? 10 : 6);
        COLORREF border = BlendColor(parameters_->theme.background, parameters_->theme.foreground,
                                     parameters_->theme.dark ? 22 : 18);
        std::wstring html = L"<!doctype html><html><head><meta charset='utf-8'><base href='" +
            HtmlEncode(base) + L"'><style>html,body{margin:0;padding:16px;font:14px/1.6 Arial,sans-serif;background:" +
            CssColor(parameters_->theme.background) + L";color:" + CssColor(parameters_->theme.foreground) +
            L"}h1,h2,h3,h4,h5,h6{color:" + CssColor(parameters_->theme.foreground) +
            L";margin-top:1.2em}a{color:" + CssColor(parameters_->theme.accent) +
            L";text-decoration:none}a:hover{text-decoration:underline}code{font-family:Consolas,'Courier New',monospace;background:" +
            CssColor(codeBackground) + L";padding:2px 4px;border-radius:4px}pre{padding:12px;overflow:auto;border-radius:6px;background:" +
            CssColor(codeBackground) + L";border:1px solid " + CssColor(border) +
            L"}pre code{padding:0;background:transparent}table{border-collapse:collapse;margin:1em 0;width:100%}th,td{border:1px solid " +
            CssColor(border) + L";padding:8px;text-align:left}blockquote{border-left:4px solid " +
            CssColor(parameters_->theme.accent) + L";margin:1em 0;padding:.5em 1em;background:" +
            CssColor(codeBackground) + L"}img,video,iframe{max-width:100%;height:auto}hr{border:0;border-top:1px solid " +
            CssColor(border) + L";margin:2em 0}ul,ol{margin:0 0 1em 1.5em}</style>"
            L"</head><body>" + fragment + L"</body></html>";
        markdownDocumentUtf8_ = WideToUtf8(html);
        markdownDocumentUri_ = L"https://markdown.local/document.html?generation=" +
                               std::to_wstring(preparationGeneration_);
        SetLoadProgress(80);
        webView_->Navigate(markdownDocumentUri_.c_str());
    }

    void HandlePreparationResult(PreparationResult* raw, uint64_t failedGeneration)
    {
        std::unique_ptr<PreparationResult> result(raw);
        if (result == nullptr)
        {
            if (failedGeneration == preparationGeneration_)
                ShowError(parameters_->openFailed, E_OUTOFMEMORY);
            return;
        }
        if (result->generation != preparationGeneration_ || !webView_)
            return;
        if (result->error != ERROR_SUCCESS)
        {
            if (result->kind == PreparationKind::Markdown && !result->renderError.empty())
            {
                MessageBoxW(window_, result->renderError.c_str(), parameters_->pluginName.c_str(),
                            MB_OK | MB_ICONERROR);
                SetWindowTextW(status_, parameters_->openFailed.c_str());
            }
            else
                ShowError(parameters_->openFailed, HRESULT_FROM_WIN32(result->error));
            return;
        }
        if (result->kind == PreparationKind::Markdown)
        {
            NavigateMarkdown(result->text);
            return;
        }
        activeLanguage_ = selectedLanguage_.empty() ? automaticLanguage_ : selectedLanguage_;
        UpdateWindowTitle();
        virtualText_ = std::move(result->text);
        virtualLineStarts_ = std::move(result->lineStarts);
        virtualGeneration_ = result->generation;
        ComPtr<ICoreWebView2_3> webView3;
        if (FAILED(webView_.As(&webView3)) || !webView3)
        {
            ShowError(parameters_->openFailed, E_NOINTERFACE);
            return;
        }
        const std::wstring folder = PrismAssetsDirectory();
        if (FAILED(webView3->SetVirtualHostNameToFolderMapping(
                L"prism.local", folder.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW)))
        {
            ShowError(parameters_->openFailed, E_FAIL);
            return;
        }
        SetLoadProgress(80);
        const std::wstring uri = L"https://prism.local/viewer/virtual-viewer.html?g=" +
                                 std::to_wstring(virtualGeneration_);
        webView_->Navigate(uri.c_str());
    }

    void PostVirtualInit()
    {
        if (!webView_ || virtualGeneration_ == 0 || virtualInitSent_)
            return;
        const size_t lineCount = virtualLineStarts_.size();
        const PrismStyleMetrics style = GetPrismStyleMetrics(lineCount);
        std::wstring json =
            L"{\"type\":\"init\",\"generation\":" + std::to_wstring(virtualGeneration_) +
            L",\"lineCount\":" + std::to_wstring(lineCount) +
            L",\"chunkLines\":" + std::to_wstring(NV_VIRTUAL_CHUNK_LINES) +
            L",\"language\":" + JsonString(activeLanguage_) +
            L",\"showLineNumbers\":" + (showLineNumbers_ ? std::wstring(L"true") : L"false") +
            L",\"wrapLines\":" + (wrapLines_ ? std::wstring(L"true") : L"false") +
            L",\"showWhitespace\":" + (showWhitespace_ ? std::wstring(L"true") : L"false") +
            L",\"lineHeight\":" + std::to_wstring(style.lineHeight) +
            L",\"charWidth\":" + std::to_wstring(style.charWidth) +
            L",\"gutterWidth\":" + std::to_wstring(style.gutterWidth) +
            L",\"fontSize\":" + std::to_wstring(style.fontSize) +
            L",\"fontFace\":" + JsonString(style.fontFace) +
            L",\"fontWeight\":" + JsonString(std::to_wstring(style.fontWeight)) +
            L",\"fontStyle\":" + JsonString(style.fontStyle) +
            L",\"textDecoration\":" + JsonString(style.textDecoration) +
            L",\"foreground\":" + JsonString(CssColor(parameters_->theme.foreground)) +
            L",\"background\":" + JsonString(CssColor(parameters_->theme.background)) +
            L",\"selectedForeground\":" + JsonString(CssColor(parameters_->theme.selectedForeground)) +
            L",\"selectedBackground\":" + JsonString(CssColor(parameters_->theme.selectedBackground)) +
            L",\"gutterForeground\":" + JsonString(style.gutterForeground) +
            L",\"gutterBackground\":" + JsonString(style.gutterBackground) +
            L",\"palette\":" + JsonString(ColorPaletteName());
        AppendCustomPaletteJson(json, nullptr, nullptr);
        json += L"}";
        if (SUCCEEDED(webView_->PostWebMessageAsJson(json.c_str())))
            virtualInitSent_ = true;
    }

    std::wstring ColorPaletteName() const
    {
        if (colorPalette_ == NV_PALETTE_PRISM)
            return L"prism";
        if (colorPalette_ == NV_PALETTE_CUSTOM)
            return L"custom";
        return L"visual-studio";
    }

    void PostColorPalette(const COLORREF* light = nullptr, const COLORREF* dark = nullptr)
    {
        if (!webView_ || !virtualInitSent_)
            return;
        std::wstring json =
            L"{\"type\":\"palette\",\"palette\":" + JsonString(ColorPaletteName());
        AppendCustomPaletteJson(json, light, dark);
        json += L"}";
        webView_->PostWebMessageAsJson(json.c_str());
    }

    std::wstring Utf8ToWideString(const std::string& value) const
    {
        if (value.empty())
            return {};
        int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                                        nullptr, 0);
        if (count <= 0)
            return {};
        std::wstring result(static_cast<size_t>(count), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                            result.data(), count);
        return result;
    }

    const std::wstring& TokenLabel(size_t index) const
    {
        switch (index)
        {
        case 0: return parameters_->tokenComment;
        case 1: return parameters_->tokenPunctuation;
        case 2: return parameters_->tokenKeyword;
        case 3: return parameters_->tokenControlKeyword;
        case 4: return parameters_->tokenClassName;
        case 5: return parameters_->tokenFunction;
        case 6: return parameters_->tokenString;
        case 7: return parameters_->tokenNumber;
        case 8: return parameters_->tokenBoolean;
        case 9: return parameters_->tokenVariable;
        case 10: return parameters_->tokenNamespace;
        default: return parameters_->tokenRegex;
        }
    }

    void ShowEditCustomPalette()
    {
        Salamatrix::UI::IUIService* ui = QueryViewerUI(parameters_->general);
        const std::string title = Utf8Or(parameters_->editCustomTitle, "Edit Custom");
        if (ui == nullptr)
        {
            const std::string message = Utf8Or(
                parameters_->uiUnavailable, "Salamatrix.UI is not available.");
            MessageBoxW(window_, Utf8ToWideString(message).c_str(),
                        Utf8ToWideString(title).c_str(), MB_OK | MB_ICONWARNING);
            return;
        }

        COLORREF lightColors[kPrismTokenCount];
        COLORREF darkColors[kPrismTokenCount];
        ReadCustomPaletteFile(lightColors, darkColors);

        Salamatrix::UI::DialogOptions options;
        options.Title = title.c_str();
        options.Parent = window_;
        options.Width = 184;
        options.Height = 216;
        Salamatrix::UI::IDialog* dialog = ui->CreateSalamatrixDialog(options);
        if (dialog == nullptr)
            return;

        const char* tokenFallbacks[kPrismTokenCount] = {
            "Comment", "Punctuation", "Keyword", "Control keyword", "Class name",
            "Function", "String", "Number", "Boolean", "Variable", "Namespace",
            "Regular expression"
        };
        std::string lightHeader = Utf8Or(parameters_->light, "Light");
        std::string darkHeader = Utf8Or(parameters_->dark, "Dark");
        std::string saveText = Utf8Or(parameters_->save, "&Save");
        std::string cancelText = Utf8Or(parameters_->cancel, "Cancel");
        std::string labelText[kPrismTokenCount];
        std::string labelIds[kPrismTokenCount];
        std::string lightIds[kPrismTokenCount];
        std::string darkIds[kPrismTokenCount];
        for (size_t index = 0; index < kPrismTokenCount; ++index)
        {
            labelText[index] = Utf8Or(TokenLabel(index), tokenFallbacks[index]);
            labelIds[index] = std::string("lbl-") + kPrismTokenKeys[index];
            lightIds[index] = std::string("light-") + kPrismTokenKeys[index];
            darkIds[index] = std::string("dark-") + kPrismTokenKeys[index];
        }

        constexpr int kLabelX = 8;
        constexpr int kLabelW = 96;
        constexpr int kColorW = 30;
        constexpr int kColorH = 12;
        constexpr int kRow = 14;
        constexpr int kLightX = 110;
        constexpr int kDarkX = 146;
        AddPaletteControl(dialog, Salamatrix::UI::ControlKindLabel, "hdr-light",
                          lightHeader.c_str(), kLightX, 6, kColorW, 8);
        AddPaletteControl(dialog, Salamatrix::UI::ControlKindLabel, "hdr-dark",
                          darkHeader.c_str(), kDarkX, 6, kColorW, 8);
        Salamatrix::UI::IControl* lightButtons[kPrismTokenCount] = {};
        Salamatrix::UI::IControl* darkButtons[kPrismTokenCount] = {};
        for (size_t index = 0; index < kPrismTokenCount; ++index)
        {
            const int buttonY = 20 + static_cast<int>(index) * kRow;
            AddPaletteControl(dialog, Salamatrix::UI::ControlKindLabel, labelIds[index].c_str(),
                              labelText[index].c_str(), kLabelX, buttonY + 2, kLabelW, 8);
            lightButtons[index] = AddPaletteControl(
                dialog, Salamatrix::UI::ControlKindColorArrowButton, lightIds[index].c_str(), "",
                kLightX, buttonY, kColorW, kColorH);
            darkButtons[index] = AddPaletteControl(
                dialog, Salamatrix::UI::ControlKindColorArrowButton, darkIds[index].c_str(), "",
                kDarkX, buttonY, kColorW, kColorH);
            if (lightButtons[index] != nullptr)
                lightButtons[index]->SetColor(lightColors[index], lightColors[index]);
            if (darkButtons[index] != nullptr)
                darkButtons[index]->SetColor(darkColors[index], darkColors[index]);
        }
        Salamatrix::UI::IControl* save = AddPaletteControl(
            dialog, Salamatrix::UI::ControlKindButton, "save", saveText.c_str(),
            70, 196, 50, 14, IDOK);
        AddPaletteControl(dialog, Salamatrix::UI::ControlKindButton, "cancel", cancelText.c_str(),
                          126, 196, 50, 14, IDCANCEL);
        if (save != nullptr)
            save->SetStyleFlags(Salamatrix::UI::ButtonDefault);

        const int result = dialog->ShowModal();
        if (result == IDOK)
        {
            for (size_t index = 0; index < kPrismTokenCount; ++index)
            {
                COLORREF text = 0;
                COLORREF background = 0;
                if (lightButtons[index] != nullptr &&
                    lightButtons[index]->GetColor(&text, &background))
                    lightColors[index] = background;
                if (darkButtons[index] != nullptr &&
                    darkButtons[index]->GetColor(&text, &background))
                    darkColors[index] = background;
            }
            if (!WriteCustomPaletteFile(lightColors, darkColors))
            {
                const std::string message = Utf8Or(
                    parameters_->saveFailed, "Unable to save the custom color palette.");
                ui->ShowMessageBox(window_, message.c_str(), title.c_str(),
                                   MB_OK | MB_ICONWARNING);
            }
            else
            {
                colorPalette_ = NV_PALETTE_CUSTOM;
                WriteViewerSetting(L"PrismColorPalette", colorPalette_);
                UpdateViewMenuChecks();
                PostColorPalette(lightColors, darkColors);
            }
        }
        ui->DestroyDialog(dialog);
    }

    static bool ParseUnsigned(const std::wstring& value, uint64_t& number)
    {
        if (value.empty() ||
            std::find_if(value.begin(), value.end(), [](wchar_t ch) { return ch < L'0' || ch > L'9'; }) != value.end())
            return false;
        wchar_t* end = nullptr;
        errno = 0;
        unsigned long long parsed = wcstoull(value.c_str(), &end, 10);
        if (errno == ERANGE || end == nullptr || *end != L'\0')
            return false;
        number = static_cast<uint64_t>(parsed);
        return true;
    }

    void HandleVirtualChunk(const std::wstring& message)
    {
        constexpr wchar_t prefix[] = L"salamander-chunk:";
        const size_t prefixLength = std::size(prefix) - 1;
        if (message.compare(0, prefixLength, prefix) != 0)
            return;
        const size_t first = message.find(L':', prefixLength);
        const size_t second = first == std::wstring::npos ? std::wstring::npos : message.find(L':', first + 1);
        if (first == std::wstring::npos || second == std::wstring::npos ||
            message.find(L':', second + 1) != std::wstring::npos)
            return;
        uint64_t generation = 0;
        uint64_t start = 0;
        uint64_t requested = 0;
        if (!ParseUnsigned(message.substr(prefixLength, first - prefixLength), generation) ||
            !ParseUnsigned(message.substr(first + 1, second - first - 1), start) ||
            !ParseUnsigned(message.substr(second + 1), requested) ||
            generation != virtualGeneration_ || generation != preparationGeneration_ ||
            requested == 0 || requested > NV_VIRTUAL_CHUNK_LINES ||
            start >= virtualLineStarts_.size() ||
            requested > virtualLineStarts_.size() - static_cast<size_t>(start))
            return;
        const size_t startLine = static_cast<size_t>(start);
        const size_t lineCount = static_cast<size_t>(requested);
        const size_t textStart = virtualLineStarts_[startLine];
        size_t textEnd = startLine + lineCount < virtualLineStarts_.size()
                           ? virtualLineStarts_[startLine + lineCount] : virtualText_.size();
        if (textEnd - textStart > NV_VIRTUAL_CHUNK_CHARS)
        {
            textEnd = textStart + NV_VIRTUAL_CHUNK_CHARS;
            if (textEnd < virtualText_.size() && textEnd > textStart &&
                virtualText_[textEnd - 1] >= 0xD800 && virtualText_[textEnd - 1] <= 0xDBFF &&
                virtualText_[textEnd] >= 0xDC00 && virtualText_[textEnd] <= 0xDFFF)
                --textEnd;
        }
        const bool insideMarkupComment =
            IsMarkupPrismLanguage(activeLanguage_) &&
            MarkupCommentOpenAt(virtualText_, textStart);
        const std::wstring json =
            L"{\"type\":\"chunk\",\"generation\":" + std::to_wstring(generation) +
            L",\"startLine\":" + std::to_wstring(startLine) +
            L",\"lineCount\":" + std::to_wstring(lineCount) +
            L",\"insideMarkupComment\":" + (insideMarkupComment ? std::wstring(L"true") : std::wstring(L"false")) +
            L",\"text\":" + JsonString(virtualText_.substr(textStart, textEnd - textStart)) + L"}";
        if (FAILED(webView_->PostWebMessageAsJson(json.c_str())))
        {
            const std::wstring fallback =
                L"{\"type\":\"chunk\",\"generation\":" + std::to_wstring(generation) +
                L",\"startLine\":" + std::to_wstring(startLine) +
                L",\"lineCount\":" + std::to_wstring(lineCount) +
                L",\"insideMarkupComment\":false" +
                L",\"text\":\"\"}";
            webView_->PostWebMessageAsJson(fallback.c_str());
        }
    }

    void LoadDocument()
    {
        if (!webView_)
            return;
        SetLoadProgress(70);
        const std::wstring extension = ExtensionOf(parameters_->filePath);
        if (parameters_->kind == NativeViewerKind::PrismText)
        {
            StartPreparation(false);
            return;
        }

        if (extension == L".html" || extension == L".htm" || extension == L".xhtml")
        {
            std::wstring mappedUri = MapLocalDocument();
            if (mappedUri.empty())
                ShowError(parameters_->openFailed, E_INVALIDARG);
            else
            {
                SetLoadProgress(80);
                webView_->Navigate(mappedUri.c_str());
            }
            return;
        }

        if (extension == L".svg")
        {
            std::wstring mappedUri = MapLocalDocument();
            if (mappedUri.empty())
                ShowError(parameters_->openFailed, E_INVALIDARG);
            else
            {
                SetLoadProgress(80);
                webView_->Navigate(mappedUri.c_str());
            }
            return;
        }

        if (extension == L".md" || extension == L".markdown" || extension == L".mdown" ||
            extension == L".mkd" || extension == L".mdx")
        {
            StartPreparation(true);
            return;
        }
        std::wstring uri = MapLocalDocument();
        if (uri.empty())
            ShowError(parameters_->openFailed, E_INVALIDARG);
        else
        {
            SetLoadProgress(80);
            webView_->Navigate(uri.c_str());
        }
    }

    void UpdateZoomDisplay(int percent)
    {
        zoomPercent_ = (std::max)(25, (std::min)(percent, 500));
        wchar_t text[16];
        swprintf_s(text, L"%d %%", zoomPercent_);
        if (zoomEdit_)
            SetWindowTextW(zoomEdit_, text);
    }

    void SetZoom(int percent)
    {
        UpdateZoomDisplay(percent);
        WriteViewerSetting(L"ZoomPercent", static_cast<DWORD>(zoomPercent_));
        if (controller_)
            controller_->put_ZoomFactor(static_cast<double>(zoomPercent_) / 100.0);
    }

    void ApplyZoomEdit()
    {
        wchar_t text[32];
        GetWindowTextW(zoomEdit_, text, static_cast<int>(std::size(text)));
        SetZoom(_wtoi(text));
    }

    void UpdateViewMenuChecks()
    {
        if (mainMenu_ == nullptr)
            return;
        CGUIMenuPopupAbstract* view = mainMenu_->GetSubMenu(1, TRUE);
        if (view == nullptr)
            return;
        view->CheckItem(IDM_NV_LINE_NUMBERS, FALSE, showLineNumbers_ ? TRUE : FALSE);
        view->CheckItem(IDM_NV_WRAP_LINES, FALSE, wrapLines_ ? TRUE : FALSE);
        view->CheckItem(IDM_NV_SHOW_WHITESPACE, FALSE, showWhitespace_ ? TRUE : FALSE);
        if (colorsMenu_ != nullptr)
        {
            const int checked = colorPalette_ == NV_PALETTE_PRISM ? IDM_NV_COLORS_PRISM
                                : colorPalette_ == NV_PALETTE_CUSTOM ? IDM_NV_COLORS_CUSTOM
                                                                     : IDM_NV_COLORS_VISUAL_STUDIO;
            colorsMenu_->CheckRadioItem(IDM_NV_COLORS_VISUAL_STUDIO, IDM_NV_COLORS_CUSTOM, checked, FALSE);
        }
        if (syntaxMenu_ != nullptr)
            syntaxMenu_->CheckItem(IDM_NV_SYNTAX_AUTOMATIC, FALSE, selectedLanguage_.empty() ? TRUE : FALSE);
        for (const SyntaxMenuItem& item : syntaxMenuItems_)
            item.menu->CheckItem(item.command, FALSE, selectedLanguage_ == item.language ? TRUE : FALSE);
    }

    void HandleCommand(int command, int notification)
    {
        if (command == IDM_NV_CLOSE)
            DestroyWindow(window_);
        else if (command == IDM_NV_REFRESH && webView_)
            LoadDocument();
        else if (command == IDM_NV_ZOOM_IN || command == IDC_NV_ZOOM_IN)
            SetZoom(zoomPercent_ + 10);
        else if (command == IDM_NV_ZOOM_OUT || command == IDC_NV_ZOOM_OUT)
            SetZoom(zoomPercent_ - 10);
        else if (command == IDM_NV_ZOOM_RESET || command == IDC_NV_ZOOM_RESET)
            SetZoom(100);
        else if (command == IDC_NV_ZOOM_EDIT && notification == EN_KILLFOCUS)
            ApplyZoomEdit();
        else if (parameters_->kind == NativeViewerKind::PrismText &&
                 (command == IDM_NV_LINE_NUMBERS || command == IDM_NV_WRAP_LINES || command == IDM_NV_SHOW_WHITESPACE))
        {
            if (command == IDM_NV_LINE_NUMBERS)
            {
                showLineNumbers_ = !showLineNumbers_;
                WriteViewerSetting(L"PrismLineNumbers", showLineNumbers_ ? 1 : 0);
            }
            else if (command == IDM_NV_WRAP_LINES)
            {
                wrapLines_ = !wrapLines_;
                WriteViewerSetting(L"PrismWrapLines", wrapLines_ ? 1 : 0);
            }
            else
            {
                showWhitespace_ = !showWhitespace_;
                WriteViewerSetting(L"PrismShowWhitespace", showWhitespace_ ? 1 : 0);
            }
            UpdateViewMenuChecks();
            LoadDocument();
        }
        else if (parameters_->kind == NativeViewerKind::PrismText &&
                 (command == IDM_NV_COLORS_VISUAL_STUDIO || command == IDM_NV_COLORS_PRISM ||
                  command == IDM_NV_COLORS_CUSTOM))
        {
            colorPalette_ = command == IDM_NV_COLORS_PRISM ? NV_PALETTE_PRISM
                            : command == IDM_NV_COLORS_CUSTOM ? NV_PALETTE_CUSTOM
                                                             : NV_PALETTE_VISUAL_STUDIO;
            WriteViewerSetting(L"PrismColorPalette", colorPalette_);
            UpdateViewMenuChecks();
            PostColorPalette();
        }
        else if (parameters_->kind == NativeViewerKind::PrismText &&
                 command == IDM_NV_COLORS_EDIT_CUSTOM)
        {
            ShowEditCustomPalette();
        }
        else if (parameters_->kind == NativeViewerKind::PrismText && command == IDM_NV_SYNTAX_AUTOMATIC)
        {
            selectedLanguage_.clear();
            UpdateViewMenuChecks();
            LoadDocument();
        }
        else if (parameters_->kind == NativeViewerKind::PrismText &&
                 command >= IDM_NV_SYNTAX_FIRST &&
                 command < IDM_NV_SYNTAX_FIRST + static_cast<int>(syntaxMenuItems_.size()))
        {
            selectedLanguage_ = syntaxMenuItems_[command - IDM_NV_SYNTAX_FIRST].language;
            UpdateViewMenuChecks();
            LoadDocument();
        }
    }

    void ResizeChildren()
    {
        if (status_)
        {
            SendMessageW(status_, WM_SIZE, 0, 0);
        }
        RECT client = {};
        GetClientRect(window_, &client);
        if (menuBar_ != nullptr)
        {
            const int menuHeight = menuBar_->GetNeededHeight();
            SetWindowPos(menuBar_->GetHWND(), HWND_TOP, 0, 0, client.right, menuHeight,
                         SWP_NOACTIVATE | SWP_SHOWWINDOW);
            client.top += menuHeight;
        }
        int statusHeight = 0;
        if (status_)
        {
            RECT statusRect = {};
            GetWindowRect(status_, &statusRect);
            statusHeight = statusRect.bottom - statusRect.top;
            client.bottom -= statusHeight;

            const int buttonWidth = 24;
            const int editWidth = 56;
            const int resetWidth = 42;
            const int gripWidth = GetSystemMetrics(SM_CXVSCROLL);
            const int top = client.bottom + 2;
            const int height = (std::max)(statusHeight - 4, 1);
            int x = client.right - gripWidth;
            x -= buttonWidth;
            SetWindowPos(zoomIn_, HWND_TOP, x, top, buttonWidth, height, SWP_NOACTIVATE);
            x -= editWidth;
            SetWindowPos(zoomEdit_, HWND_TOP, x, top + 1, editWidth, (std::max)(height - 2, 1), SWP_NOACTIVATE);
            x -= buttonWidth;
            SetWindowPos(zoomOut_, HWND_TOP, x, top, buttonWidth, height, SWP_NOACTIVATE);
            x -= resetWidth;
            SetWindowPos(zoomReset_, HWND_TOP, x, top, resetWidth, height, SWP_NOACTIVATE);
            int parts[] = {(std::max)(x - 4, 0), -1};
            SendMessageW(status_, SB_SETPARTS, 2, reinterpret_cast<LPARAM>(parts));
            SetWindowPos(status_, HWND_BOTTOM, 0, client.bottom, client.right, statusHeight, SWP_NOACTIVATE);
            // The child controls and WebView controller repaint after their
            // bounds change. Invalidating every descendant here queued a full
            // WebView repaint for every intermediate sizing message.
            InvalidateRect(window_, nullptr, FALSE);
        }
        if (controller_)
        {
            controller_->put_Bounds(client);
            if (parameters_->kind == NativeViewerKind::PrismText && webView_)
                webView_->PostWebMessageAsString(L"salamander-resize");
        }
    }

    void ShowError(const std::wstring& message, HRESULT error)
    {
        wchar_t detail[32];
        swprintf_s(detail, L"\n\n0x%08X", static_cast<unsigned int>(error));
        std::wstring full = message + detail;
        MessageBoxW(window_, full.c_str(), parameters_->pluginName.c_str(), MB_OK | MB_ICONERROR);
        SetWindowTextW(status_, message.c_str());
    }

    void CloseBrowser()
    {
        if (webView_ && navigationStartingToken_.value != 0)
            webView_->remove_NavigationStarting(navigationStartingToken_);
        if (webView_ && domContentLoadedToken_.value != 0)
        {
            ComPtr<ICoreWebView2_2> webView2;
            if (SUCCEEDED(webView_.As(&webView2)) && webView2)
                webView2->remove_DOMContentLoaded(domContentLoadedToken_);
        }
        if (webView_ && navigationToken_.value != 0)
            webView_->remove_NavigationCompleted(navigationToken_);
        if (webView_ && webMessageToken_.value != 0)
            webView_->remove_WebMessageReceived(webMessageToken_);
        if (webView_ && webResourceRequestedToken_.value != 0)
            webView_->remove_WebResourceRequested(webResourceRequestedToken_);
        webView_.Reset();
        if (controller_)
        {
            if (zoomChangedToken_.value != 0)
                controller_->remove_ZoomFactorChanged(zoomChangedToken_);
            if (acceleratorToken_.value != 0)
                controller_->remove_AcceleratorKeyPressed(acceleratorToken_);
            controller_->Close();
        }
        controller_.Reset();
    }

    void RemoveWindow()
    {
        CloseBrowser();
        std::lock_guard<std::mutex> guard(gWindowsLock);
        auto found = std::find(gWindows.begin(), gWindows.end(), window_);
        if (found != gWindows.end())
            gWindows.erase(found);
    }

    std::unique_ptr<ViewerParameters> parameters_;
    HWND window_ = nullptr;
    HFONT menuFont_ = nullptr;
    HWND status_ = nullptr;
    HWND zoomReset_ = nullptr;
    HWND zoomOut_ = nullptr;
    HWND zoomEdit_ = nullptr;
    HWND zoomIn_ = nullptr;
    ComPtr<ICoreWebView2Controller> controller_;
    ComPtr<ICoreWebView2> webView_;
    EventRegistrationToken navigationToken_ = {};
    EventRegistrationToken navigationStartingToken_ = {};
    EventRegistrationToken domContentLoadedToken_ = {};
    EventRegistrationToken webMessageToken_ = {};
    EventRegistrationToken webResourceRequestedToken_ = {};
    EventRegistrationToken acceleratorToken_ = {};
    EventRegistrationToken zoomChangedToken_ = {};
    int zoomPercent_ = 100;
    bool showLineNumbers_ = false;
    bool wrapLines_ = false;
    bool showWhitespace_ = false;
    DWORD colorPalette_ = NV_PALETTE_VISUAL_STUDIO;
    std::wstring automaticLanguage_;
    std::wstring selectedLanguage_;
    std::wstring activeLanguage_;
    std::vector<std::wstring> installedLanguages_;
    std::vector<SyntaxMenuItem> syntaxMenuItems_;
    bool browserVisible_ = false;
    int loadProgress_ = 0;
    PreparationTarget* preparationTarget_ = nullptr;
    uint64_t preparationGeneration_ = 0;
    uint64_t virtualGeneration_ = 0;
    bool virtualInitSent_ = false;
    std::wstring virtualText_;
    std::vector<size_t> virtualLineStarts_;
    std::wstring markdownDocumentUri_;
    std::string markdownDocumentUtf8_;
};

void PrewarmSharedEnvironment()
{
    if (gSharedEnvironment || gCreatingSharedEnvironment)
        return;

    gCreatingSharedEnvironment = true;
    std::wstring userData = ViewerUserDataFolder();
    CreateWebView2EnvironmentFn createEnvironment = GetCreateWebView2Environment();
    if (createEnvironment == nullptr)
    {
        gCreatingSharedEnvironment = false;
        return;
    }
    HRESULT hr = createEnvironment(nullptr, userData.empty() ? nullptr : userData.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT
            {
                gCreatingSharedEnvironment = false;
                if (FAILED(result) || environment == nullptr)
                    return S_OK;
                gSharedEnvironment = environment;
                std::vector<HWND> pending;
                pending.swap(gPendingEnvironmentWindows);
                for (HWND target : pending)
                {
                    ViewerWindow* viewer = IsWindow(target)
                        ? reinterpret_cast<ViewerWindow*>(GetWindowLongPtrW(target, GWLP_USERDATA)) : nullptr;
                    if (viewer != nullptr)
                        viewer->CreateBrowserController(gSharedEnvironment.Get());
                }
                return S_OK;
            }).Get());
    if (FAILED(hr))
        gCreatingSharedEnvironment = false;
}

static void CreateViewerWindow(ViewerParameters* raw)
{
    std::unique_ptr<ViewerParameters> parameters(static_cast<ViewerParameters*>(raw));
    ViewerWindow* viewer = NewNoThrow<ViewerWindow>(std::move(parameters));
    if (viewer == nullptr)
        return;
    if (!viewer->Create())
    {
        delete viewer;
        return;
    }
    {
        std::lock_guard<std::mutex> guard(gWindowsLock);
        gWindows.push_back(viewer->Window());
    }
    viewer->Show();
}

DWORD WINAPI ViewerHostThread(void* readyEvent)
{
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
    {
        SetEvent(static_cast<HANDLE>(readyEvent));
        return 1;
    }

    // PostThreadMessage requires a message queue to exist before the caller
    // can enqueue the first viewer request.
    MSG message;
    PeekMessageW(&message, nullptr, 0, 0, PM_NOREMOVE);
    {
        std::lock_guard<std::mutex> guard(gViewerHostLock);
        gViewerHostThreadId = GetCurrentThreadId();
    }
    SetEvent(static_cast<HANDLE>(readyEvent));

    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        if (message.hwnd == nullptr && message.message == WM_NV_CREATE_VIEWER)
        {
            CreateViewerWindow(reinterpret_cast<ViewerParameters*>(message.wParam));
            continue;
        }
        if (message.hwnd == nullptr && message.message == WM_NV_PREWARM_ENVIRONMENT)
        {
            PrewarmSharedEnvironment();
            continue;
        }
        if (message.hwnd == nullptr && message.message == WM_NV_STOP_HOST)
            break;

        if (message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN)
        {
            HWND viewerWindow = message.hwnd != nullptr ? GetAncestor(message.hwnd, GA_ROOT) : nullptr;
            if (viewerWindow == nullptr)
                continue;
            if (message.message == WM_KEYDOWN && message.wParam == VK_RETURN &&
                GetParent(message.hwnd) == viewerWindow &&
                GetDlgCtrlID(message.hwnd) == IDC_NV_ZOOM_EDIT)
            {
                SendMessageW(viewerWindow, WM_NV_APPLY_ZOOM, 0, 0);
                SendMessageW(message.hwnd, EM_SETSEL, 0, -1);
                continue;
            }
            int command = 0;
            if (message.wParam == VK_ESCAPE)
            {
                PostMessageW(viewerWindow, WM_CLOSE, 0, 0);
                continue;
            }
            if (message.wParam == VK_F5)
                command = IDM_NV_REFRESH;
            else if (message.wParam == VK_F2)
                command = IDM_NV_WRAP_LINES;
            else if ((GetKeyState(VK_CONTROL) & 0x8000) != 0)
            {
                if (message.wParam == '0' || message.wParam == VK_NUMPAD0)
                    command = IDM_NV_ZOOM_RESET;
                else if (message.wParam == VK_OEM_PLUS || message.wParam == VK_ADD)
                    command = IDM_NV_ZOOM_IN;
                else if (message.wParam == VK_OEM_MINUS || message.wParam == VK_SUBTRACT)
                    command = IDM_NV_ZOOM_OUT;
            }
            if (command != 0)
            {
                PostMessageW(viewerWindow, WM_COMMAND, command, 0);
                continue;
            }
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    gPendingEnvironmentWindows.clear();
    gSharedEnvironment.Reset();
    gCreatingSharedEnvironment = false;
    CoUninitialize();
    return 0;
}

bool EnsureViewerHost(DWORD* hostThreadId)
{
    HANDLE readyEvent = nullptr;
    {
        std::lock_guard<std::mutex> guard(gViewerHostLock);
        if (gViewerHostThread == nullptr)
        {
            gViewerHostReady = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            if (gViewerHostReady == nullptr)
                return false;
            gViewerHostThread = CreateThread(nullptr, 0, ViewerHostThread, gViewerHostReady, 0, nullptr);
            if (gViewerHostThread == nullptr)
            {
                CloseHandle(gViewerHostReady);
                gViewerHostReady = nullptr;
                return false;
            }
        }
        readyEvent = gViewerHostReady;
    }
    if (WaitForSingleObject(readyEvent, 10000) != WAIT_OBJECT_0)
        return false;
    std::lock_guard<std::mutex> guard(gViewerHostLock);
    if (gViewerHostThreadId == 0)
        return false;
    if (hostThreadId != nullptr)
        *hostThreadId = gViewerHostThreadId;
    return true;
}
}

bool NativeViewer_EnsureInitialized()
{
    // Loading WebView2Loader here moves the disk/DLL work out of the first
    // viewer invocation (Connect calls this while the plug-in is initialized).
    if (gShuttingDown.load() || GetCreateWebView2Environment() == nullptr)
        return false;
    DWORD hostThreadId = 0;
    return EnsureViewerHost(&hostThreadId) &&
           PostThreadMessageW(hostThreadId, WM_NV_PREWARM_ENVIRONMENT, 0, 0) != FALSE;
}

bool NativeViewer_Show(const NativeViewerRequest& request)
{
    if (gShuttingDown.load() || request.filePath == nullptr || request.filePath[0] == L'\0')
        return false;
    std::unique_ptr<ViewerParameters> data(NewNoThrow<ViewerParameters>());
    if (!data)
        return false;
    data->module = request.module;
    data->owner = request.owner;
    data->filePath = request.filePath;
    data->placement = request.placement;
    data->showCommand = request.showCommand;
    data->alwaysOnTop = request.alwaysOnTop;
    data->closeEvent = request.closeEvent;
    data->kind = request.kind;
    data->theme = request.theme;
    data->menuFont = request.menuFont;
    data->viewerFont = request.viewerFont;
    data->gui = request.gui;
    data->pluginName = CopyString(request.strings.pluginName);
    data->fileMenu = CopyString(request.strings.fileMenu);
    data->viewMenu = CopyString(request.strings.viewMenu);
    data->close = CopyString(request.strings.close);
    data->refresh = CopyString(request.strings.refresh);
    data->zoomIn = CopyString(request.strings.zoomIn);
    data->zoomOut = CopyString(request.strings.zoomOut);
    data->zoomReset = CopyString(request.strings.zoomReset);
    data->lineNumbers = CopyString(request.strings.lineNumbers);
    data->wrapLines = CopyString(request.strings.wrapLines);
    data->showWhitespace = CopyString(request.strings.showWhitespace);
    data->loading = CopyString(request.strings.loading);
    data->ready = CopyString(request.strings.ready);
    data->initializationFailed = CopyString(request.strings.initializationFailed);
    data->openFailed = CopyString(request.strings.openFailed);
    data->syntaxHighlighter = CopyString(request.strings.syntaxHighlighter);
    data->automatic = CopyString(request.strings.automatic);
    data->colors = CopyString(request.strings.colors);
    data->visualStudio = CopyString(request.strings.visualStudio);
    data->defaultPrism = CopyString(request.strings.defaultPrism);
    data->customPalette = CopyString(request.strings.customPalette);
    data->editCustom = CopyString(request.strings.editCustom);
    data->editCustomTitle = CopyString(request.strings.editCustomTitle);
    data->save = CopyString(request.strings.save);
    data->cancel = CopyString(request.strings.cancel);
    data->light = CopyString(request.strings.light);
    data->dark = CopyString(request.strings.dark);
    data->tokenComment = CopyString(request.strings.tokenComment);
    data->tokenPunctuation = CopyString(request.strings.tokenPunctuation);
    data->tokenKeyword = CopyString(request.strings.tokenKeyword);
    data->tokenControlKeyword = CopyString(request.strings.tokenControlKeyword);
    data->tokenClassName = CopyString(request.strings.tokenClassName);
    data->tokenFunction = CopyString(request.strings.tokenFunction);
    data->tokenString = CopyString(request.strings.tokenString);
    data->tokenNumber = CopyString(request.strings.tokenNumber);
    data->tokenBoolean = CopyString(request.strings.tokenBoolean);
    data->tokenVariable = CopyString(request.strings.tokenVariable);
    data->tokenNamespace = CopyString(request.strings.tokenNamespace);
    data->tokenRegex = CopyString(request.strings.tokenRegex);
    data->saveFailed = CopyString(request.strings.saveFailed);
    data->uiUnavailable = CopyString(request.strings.uiUnavailable);
    data->general = request.general;

    DWORD hostThreadId = 0;
    if (!EnsureViewerHost(&hostThreadId))
        return false;
    if (hostThreadId == 0 || !PostThreadMessageW(hostThreadId, WM_NV_CREATE_VIEWER,
                                                   reinterpret_cast<WPARAM>(data.get()), 0))
        return false;
    data.release();
    return true;
}

bool NativeViewer_RequestShutdown(bool forceClose)
{
    std::vector<HWND> windows;
    {
        std::lock_guard<std::mutex> guard(gWindowsLock);
        windows = gWindows;
    }
    if (!forceClose && !windows.empty())
        return false;
    for (HWND window : windows)
        PostMessageW(window, WM_NV_CLOSE_ALL, 0, 0);
    if (forceClose)
    {
        for (int attempt = 0; attempt < 500; ++attempt)
        {
            {
                std::lock_guard<std::mutex> guard(gWindowsLock);
                if (gWindows.empty())
                    return true;
            }
            Sleep(10);
        }
        return false;
    }
    return true;
}

void NativeViewer_Shutdown()
{
    gShuttingDown.store(true);
    NativeViewer_RequestShutdown(true);
    HANDLE thread = nullptr;
    DWORD threadId = 0;
    {
        std::lock_guard<std::mutex> guard(gViewerHostLock);
        thread = gViewerHostThread;
        threadId = gViewerHostThreadId;
    }
    if (thread != nullptr && threadId != 0)
    {
        PostThreadMessageW(threadId, WM_NV_STOP_HOST, 0, 0);
        WaitForSingleObject(thread, 5000);
        std::lock_guard<std::mutex> guard(gViewerHostLock);
        CloseHandle(gViewerHostThread);
        gViewerHostThread = nullptr;
        gViewerHostThreadId = 0;
        CloseHandle(gViewerHostReady);
        gViewerHostReady = nullptr;
    }
}
