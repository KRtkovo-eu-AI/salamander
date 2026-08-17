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

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#pragma comment(lib, "shlwapi.lib")

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace
{
constexpr UINT WM_NV_CLOSE_ALL = WM_APP + 0x631;
constexpr int IDC_NV_STATUS = 101;
constexpr int IDM_NV_CLOSE = 40001;
constexpr int IDM_NV_REFRESH = 40002;
constexpr int IDM_NV_ZOOM_IN = 40003;
constexpr int IDM_NV_ZOOM_OUT = 40004;
constexpr int IDM_NV_ZOOM_RESET = 40005;
constexpr int IDM_NV_LINE_NUMBERS = 40006;
constexpr int IDM_NV_WRAP_LINES = 40007;
constexpr int IDM_NV_SHOW_WHITESPACE = 40008;
constexpr int IDC_NV_ZOOM_RESET = 40101;
constexpr int IDC_NV_ZOOM_OUT = 40102;
constexpr int IDC_NV_ZOOM_EDIT = 40103;
constexpr int IDC_NV_ZOOM_IN = 40104;

std::mutex gWindowsLock;
std::vector<HWND> gWindows;
std::atomic<bool> gShuttingDown(false);

constexpr const wchar_t* kViewerSettingsKey = L"Software\\Open Salamander\\ViewerFrame";

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
        {L"axaml", L"xml"}, {L"cmd", L"batch"}, {L"config", L"xml"},
        {L"csproj", L"xml"}, {L"cxx", L"cpp"}, {L"fsproj", L"xml"},
        {L"h", L"c"}, {L"hh", L"cpp"}, {L"hpp", L"cpp"}, {L"hxx", L"cpp"},
        {L"htm", L"html"}, {L"jsonc", L"json"}, {L"json5", L"json"},
        {L"md", L"markdown"}, {L"markdown", L"markdown"}, {L"nuspec", L"xml"},
        {L"plist", L"xml"}, {L"props", L"xml"}, {L"ps1", L"powershell"},
        {L"psd1", L"powershell"}, {L"psm1", L"powershell"}, {L"storyboard", L"xml"},
        {L"targets", L"xml"}, {L"vcxproj", L"xml"}, {L"vcproj", L"xml"},
        {L"vbproj", L"xml"}, {L"xaml", L"xml"}, {L"xlf", L"xml"}, {L"yml", L"yaml"}
    };
    for (const Mapping& mapping : mappings)
        if (value == mapping.extension)
            return mapping.language;
    return value;
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
    std::wstring executable = ModuleDirectory(module) + L"MarkdigRenderer.exe";
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
};

class ViewerWindow
{
public:
    explicit ViewerWindow(std::unique_ptr<ViewerParameters> parameters) : parameters_(std::move(parameters))
    {
        zoomPercent_ = static_cast<int>(ReadViewerSetting(L"ZoomPercent", 100));
        zoomPercent_ = (std::max)(25, (std::min)(zoomPercent_, 500));
        if (parameters_->kind == NativeViewerKind::PrismText)
        {
            showLineNumbers_ = ReadViewerSetting(L"PrismLineNumbers", 0) != 0;
            wrapLines_ = ReadViewerSetting(L"PrismWrapLines", 0) != 0;
            showWhitespace_ = ReadViewerSetting(L"PrismShowWhitespace", 0) != 0;
        }
    }
    ~ViewerWindow()
    {
        CloseBrowser();
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
        cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        cls.lpszClassName = className;
        RegisterClassExW(&cls);

        int width = (std::max)(parameters_->placement.right - parameters_->placement.left, 320L);
        int height = (std::max)(parameters_->placement.bottom - parameters_->placement.top, 240L);
        std::wstring title = FileNameOf(parameters_->filePath) + L" - " + parameters_->pluginName;
        window_ = CreateWindowExW(parameters_->alwaysOnTop ? WS_EX_TOPMOST : 0, className, title.c_str(),
                                  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                  parameters_->placement.left, parameters_->placement.top, width, height,
                                  nullptr, CreateMenuBar(), parameters_->module, this);
        return window_ != nullptr;
    }

    HWND Window() const { return window_; }
    void Show()
    {
        ShowWindow(window_, parameters_->showCommand);
        UpdateWindow(window_);
    }

private:
    HMENU CreateMenuBar()
    {
        HMENU bar = CreateMenu();
        HMENU file = CreatePopupMenu();
        AppendMenuW(file, MF_STRING, IDM_NV_CLOSE, parameters_->close.c_str());
        AppendMenuW(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(file), parameters_->fileMenu.c_str());
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
        AppendMenuW(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(view), parameters_->viewMenu.c_str());
        return bar;
    }

    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_CREATE:
        {
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
        case WM_CLOSE:
            DestroyWindow(window_);
            return 0;
        case WM_DESTROY:
            RemoveWindow();
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(window_, message, wParam, lParam);
    }

    void ApplyTheme()
    {
        PluginDarkMode_SetHostPolicyAvailable(TRUE, parameters_->theme.dark ? TRUE : FALSE);
        PluginDarkMode_SetHostColors(parameters_->theme.foreground, parameters_->theme.background);
        PluginDarkMode_ApplyTitleBar(window_);
        PluginDarkMode_ApplyListTreeThemeRecursive(window_);
    }

    void ApplyChromeFont()
    {
        HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        for (HWND control : {status_, zoomReset_, zoomOut_, zoomEdit_, zoomIn_})
            if (control != nullptr)
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }

    void BeginBrowserInitialization()
    {
        std::wstring userData;
        PWSTR appData = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &appData)))
        {
            userData = std::wstring(appData) + L"\\Open Salamander\\Native WebView2 Viewer";
            CoTaskMemFree(appData);
        }
        HWND target = window_;
        HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, userData.empty() ? nullptr : userData.c_str(),
            nullptr, Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [target](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT
                {
                    ViewerWindow* self = IsWindow(target)
                        ? reinterpret_cast<ViewerWindow*>(GetWindowLongPtrW(target, GWLP_USERDATA)) : nullptr;
                    if (self == nullptr)
                        return S_OK;
                    if (FAILED(result) || environment == nullptr)
                    {
                        self->ShowError(self->parameters_->initializationFailed, result);
                        return S_OK;
                    }
                    self->environment_ = environment;
                    return environment->CreateCoreWebView2Controller(target,
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
                                self->controller_->get_CoreWebView2(&self->webView_);
                                self->ConfigureBrowser();
                                self->ResizeChildren();
                                self->LoadDocument();
                                return S_OK;
                            }).Get());
                }).Get());
        if (FAILED(hr))
            ShowError(parameters_->initializationFailed, hr);
    }

    void ConfigureBrowser()
    {
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
        if (parameters_->kind == NativeViewerKind::RenderDocument)
        {
            webView_->add_WebMessageReceived(
                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                    [this](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
                    {
                        LPWSTR message = nullptr;
                        if (args == nullptr || FAILED(args->TryGetWebMessageAsString(&message)) || message == nullptr)
                            return S_OK;

                        const std::wstring value(message);
                        CoTaskMemFree(message);
                        constexpr wchar_t prefix[] = L"salamander-link:";
                        if (value == L"salamander-link-clear")
                            SetWindowTextW(status_, parameters_->ready.c_str());
                        else if (value.compare(0, std::size(prefix) - 1, prefix) == 0)
                            SetWindowTextW(status_, value.c_str() + (std::size(prefix) - 1));
                        return S_OK;
                    }).Get(), &webMessageToken_);

            constexpr wchar_t hoverScript[] =
                L"(function(){document.addEventListener('mouseover',function(e){var a=e.target&&e.target.closest?e.target.closest('a'):null;"
                L"if(a&&a.href)window.chrome.webview.postMessage('salamander-link:'+a.href);});"
                L"document.addEventListener('mouseout',function(e){var a=e.target&&e.target.closest?e.target.closest('a'):null;"
                L"if(a&&!a.contains(e.relatedTarget))window.chrome.webview.postMessage('salamander-link-clear');});})();";
            webView_->AddScriptToExecuteOnDocumentCreated(hoverScript, nullptr);
        }
        controller_->put_ZoomFactor(static_cast<double>(zoomPercent_) / 100.0);
        webView_->add_NavigationCompleted(
            Callback<ICoreWebView2NavigationCompletedEventHandler>(
                [this](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
                {
                    BOOL success = FALSE;
                    args->get_IsSuccess(&success);
                    SetWindowTextW(status_, success ? parameters_->ready.c_str() : parameters_->openFailed.c_str());
                    if (success && parameters_->theme.dark && parameters_->kind == NativeViewerKind::RenderDocument)
                    {
                        std::wstring script = L"(function(){if(!document||!document.documentElement)return;"
                            L"document.documentElement.style.colorScheme='dark';"
                            L"var s=document.getElementById('salamander-viewer-dark');if(!s){s=document.createElement('style');"
                            L"s.id='salamander-viewer-dark';s.textContent=':where(html,body){background-color:" +
                            CssColor(parameters_->theme.background) + L";color:" + CssColor(parameters_->theme.foreground) +
                            L"}:where(a:link){color:" + CssColor(parameters_->theme.accent) + L"}';document.head.appendChild(s);}})();";
                        webView_->ExecuteScript(script.c_str(), nullptr);
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

    void LoadDocument()
    {
        if (!webView_)
            return;
        std::wstring extension = ExtensionOf(parameters_->filePath);
        if (parameters_->kind == NativeViewerKind::PrismText)
        {
            std::vector<unsigned char> bytes;
            if (!ReadFileBytes(parameters_->filePath, bytes))
            {
                ShowError(parameters_->openFailed, HRESULT_FROM_WIN32(GetLastError()));
                return;
            }
            std::wstring text = DecodeText(bytes);
            std::wstring language = PrismLanguageForExtension(extension);
            const wchar_t* prismTheme = parameters_->theme.dark ? L"prism-tomorrow.css" : L"prism.css";
            std::wstring preClasses = L"language-" + language;
            if (showLineNumbers_)
                preClasses += L" line-numbers";
            std::wstring html = L"<!doctype html><html><head><meta charset='utf-8'>"
                L"<link rel='stylesheet' href='https://prism.local/themes/" + std::wstring(prismTheme) + L"'>"
                L"<link rel='stylesheet' href='https://prism.local/plugins/line-numbers/prism-line-numbers.css'>" +
                (showWhitespace_ ? L"<link rel='stylesheet' href='https://prism.local/plugins/show-invisibles/prism-show-invisibles.css'>" : L"") +
                L"<style>"
                L"html,body{margin:0;width:100%;height:100%;overflow:hidden;background:" + CssColor(parameters_->theme.background) +
                L";color:" + CssColor(parameters_->theme.foreground) +
                L";color-scheme:" + std::wstring(parameters_->theme.dark ? L"dark" : L"light") +
                L"}pre[class*='language-']{box-sizing:border-box;margin:0;width:100%;height:100%;padding-top:16px;padding-bottom:16px;"
                L"overflow:auto;white-space:" + std::wstring(wrapLines_ ? L"pre-wrap" : L"pre") +
                L";overflow-wrap:" + std::wstring(wrapLines_ ? L"anywhere" : L"normal") +
                L";tab-size:4;font:14px Consolas,'Courier New',monospace}"
                L"</style><script src='https://prism.local/prism.js'></script>"
                L"<script src='https://prism.local/plugins/autoloader/prism-autoloader.min.js'></script>"
                L"<script>Prism.plugins.autoloader.languages_path='https://prism.local/components/';</script>"
                L"<script src='https://prism.local/plugins/line-numbers/prism-line-numbers.min.js'></script>" +
                (showWhitespace_ ? L"<script src='https://prism.local/plugins/show-invisibles/prism-show-invisibles.min.js'></script>" : L"") +
                L"</head><body><pre class='" + HtmlEncode(preClasses) + L"'><code class='language-" +
                HtmlEncode(language) + L"'>" + HtmlEncode(text) + L"</code></pre></body></html>";

            ComPtr<ICoreWebView2_3> webView3;
            if (SUCCEEDED(webView_.As(&webView3)))
            {
                std::vector<wchar_t> modulePath(512);
                DWORD length = 0;
                for (;;)
                {
                    length = GetModuleFileNameW(parameters_->module, modulePath.data(),
                                                static_cast<DWORD>(modulePath.size()));
                    if (length == 0)
                        break;
                    if (length < modulePath.size() - 1)
                        break;
                    modulePath.resize(modulePath.size() * 2);
                }
                if (length > 0)
                {
                    std::wstring folder(modulePath.data(), length);
                    size_t slash = folder.find_last_of(L"\\/");
                    if (slash != std::wstring::npos)
                    {
                        folder.resize(slash + 1);
                        folder += L"prism";
                        webView3->SetVirtualHostNameToFolderMapping(L"prism.local", folder.c_str(),
                            COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS);
                    }
                }
            }
            webView_->NavigateToString(html.c_str());
            return;
        }

        if (extension == L".html" || extension == L".htm" || extension == L".xhtml")
        {
            std::vector<unsigned char> bytes;
            if (!ReadFileBytes(parameters_->filePath, bytes))
            {
                ShowError(parameters_->openFailed, HRESULT_FROM_WIN32(GetLastError()));
                return;
            }
            std::wstring mappedUri = MapLocalDocument();
            std::wstring base = mappedUri.empty() ? L"" : L"https://document.local/";
            std::wstring html = WithBaseElement(DecodeText(bytes), base);
            webView_->NavigateToString(html.c_str());
            return;
        }

        if (extension == L".md" || extension == L".markdown" || extension == L".mdown" ||
            extension == L".mkd" || extension == L".mdx")
        {
            std::vector<unsigned char> bytes;
            if (!ReadFileBytes(parameters_->filePath, bytes))
            {
                ShowError(parameters_->openFailed, HRESULT_FROM_WIN32(GetLastError()));
                return;
            }
            std::wstring fragment;
            std::wstring renderError;
            if (!RenderMarkdown(parameters_->module, DecodeText(bytes), fragment, renderError))
            {
                MessageBoxW(window_, renderError.c_str(), parameters_->pluginName.c_str(), MB_OK | MB_ICONERROR);
                SetWindowTextW(status_, parameters_->openFailed.c_str());
                return;
            }
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
            webView_->NavigateToString(html.c_str());
            return;
        }
        std::wstring uri = MapLocalDocument();
        if (uri.empty())
            ShowError(parameters_->openFailed, E_INVALIDARG);
        else
            webView_->Navigate(uri.c_str());
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

    void UpdateViewMenuChecks()
    {
        HMENU menu = GetMenu(window_);
        CheckMenuItem(menu, IDM_NV_LINE_NUMBERS, MF_BYCOMMAND | (showLineNumbers_ ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_NV_WRAP_LINES, MF_BYCOMMAND | (wrapLines_ ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(menu, IDM_NV_SHOW_WHITESPACE, MF_BYCOMMAND | (showWhitespace_ ? MF_CHECKED : MF_UNCHECKED));
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
        {
            wchar_t text[32];
            GetWindowTextW(zoomEdit_, text, static_cast<int>(std::size(text)));
            SetZoom(_wtoi(text));
        }
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
    }

    void ResizeChildren()
    {
        if (status_)
        {
            SendMessageW(status_, WM_SIZE, 0, 0);
        }
        RECT client = {};
        GetClientRect(window_, &client);
        int statusHeight = 0;
        if (status_)
        {
            RECT statusRect = {};
            GetWindowRect(status_, &statusRect);
            statusHeight = statusRect.bottom - statusRect.top;
            client.bottom -= statusHeight;

            const int buttonWidth = 24;
            const int editWidth = 56;
            const int resetWidth = 76;
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
            RedrawWindow(window_, nullptr, nullptr,
                         RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
        if (controller_)
            controller_->put_Bounds(client);
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
        if (webView_ && navigationToken_.value != 0)
            webView_->remove_NavigationCompleted(navigationToken_);
        if (webView_ && webMessageToken_.value != 0)
            webView_->remove_WebMessageReceived(webMessageToken_);
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
        environment_.Reset();
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
    HWND status_ = nullptr;
    HWND zoomReset_ = nullptr;
    HWND zoomOut_ = nullptr;
    HWND zoomEdit_ = nullptr;
    HWND zoomIn_ = nullptr;
    ComPtr<ICoreWebView2Environment> environment_;
    ComPtr<ICoreWebView2Controller> controller_;
    ComPtr<ICoreWebView2> webView_;
    EventRegistrationToken navigationToken_ = {};
    EventRegistrationToken webMessageToken_ = {};
    EventRegistrationToken acceleratorToken_ = {};
    EventRegistrationToken zoomChangedToken_ = {};
    int zoomPercent_ = 100;
    bool showLineNumbers_ = false;
    bool wrapLines_ = false;
    bool showWhitespace_ = false;
};

DWORD WINAPI ViewerThread(void* raw)
{
    std::unique_ptr<ViewerParameters> parameters(static_cast<ViewerParameters*>(raw));
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
    {
        if (parameters->closeEvent)
            SetEvent(parameters->closeEvent);
        return 1;
    }

    ViewerWindow* viewer = new ViewerWindow(std::move(parameters));
    if (!viewer->Create())
    {
        delete viewer;
        CoUninitialize();
        return 1;
    }
    {
        std::lock_guard<std::mutex> guard(gWindowsLock);
        gWindows.push_back(viewer->Window());
    }
    viewer->Show();
    const HWND viewerWindow = viewer->Window();

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        if (message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN)
        {
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
    CoUninitialize();
    return 0;
}
}

bool NativeViewer_EnsureInitialized()
{
    return !gShuttingDown.load();
}

bool NativeViewer_Show(const NativeViewerRequest& request)
{
    if (gShuttingDown.load() || request.filePath == nullptr || request.filePath[0] == L'\0')
        return false;
    std::unique_ptr<ViewerParameters> data(new ViewerParameters());
    data->module = request.module;
    data->owner = request.owner;
    data->filePath = request.filePath;
    data->placement = request.placement;
    data->showCommand = request.showCommand;
    data->alwaysOnTop = request.alwaysOnTop;
    data->closeEvent = request.closeEvent;
    data->kind = request.kind;
    data->theme = request.theme;
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

    HANDLE thread = CreateThread(nullptr, 0, ViewerThread, data.get(), 0, nullptr);
    if (thread == nullptr)
        return false;
    data.release();
    CloseHandle(thread);
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
}
