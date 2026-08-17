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

std::mutex gWindowsLock;
std::vector<HWND> gWindows;
std::atomic<bool> gShuttingDown(false);

std::wstring CopyString(const wchar_t* value)
{
    return value != nullptr ? value : L"";
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

std::wstring CssColor(COLORREF color)
{
    wchar_t value[8];
    swprintf_s(value, L"#%02X%02X%02X", GetRValue(color), GetGValue(color), GetBValue(color));
    return value;
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
        const wchar_t* wide = reinterpret_cast<const wchar_t*>(data + 2);
        return std::wstring(wide, wide + (size - 2) / sizeof(wchar_t));
    }

    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                    reinterpret_cast<const char*>(data), static_cast<int>(size), nullptr, 0);
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (count <= 0)
    {
        codePage = CP_ACP;
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
    std::wstring loading;
    std::wstring ready;
    std::wstring initializationFailed;
    std::wstring openFailed;
};

class ViewerWindow
{
public:
    explicit ViewerWindow(std::unique_ptr<ViewerParameters> parameters) : parameters_(std::move(parameters)) {}
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
        AppendMenuW(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(view), parameters_->viewMenu.c_str());
        return bar;
    }

    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_CREATE:
            status_ = CreateWindowExW(0, STATUSCLASSNAMEW, parameters_->loading.c_str(),
                                      WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0,
                                      window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_NV_STATUS)), parameters_->module, nullptr);
            ApplyTheme();
            BeginBrowserInitialization();
            return 0;
        case WM_SIZE:
            ResizeChildren();
            return 0;
        case WM_COMMAND:
            HandleCommand(LOWORD(wParam));
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
        webView_->add_NavigationCompleted(
            Callback<ICoreWebView2NavigationCompletedEventHandler>(
                [this](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT
                {
                    BOOL success = FALSE;
                    args->get_IsSuccess(&success);
                    SetWindowTextW(status_, success ? parameters_->ready.c_str() : parameters_->openFailed.c_str());
                    return S_OK;
                }).Get(), &navigationToken_);
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
            std::wstring html = L"<!doctype html><html><head><meta charset='utf-8'><style>"
                L"html,body{margin:0;background:" + CssColor(parameters_->theme.background) +
                L";color:" + CssColor(parameters_->theme.foreground) +
                L"}pre{margin:0;padding:16px;white-space:pre;tab-size:4;font:14px Consolas,'Courier New',monospace}"
                L"</style><script src='https://prism.local/prism.js'></script>"
                L"<script>Prism.plugins.autoloader.languages_path='https://prism.local/components/';</script>"
                L"<script src='https://prism.local/plugins/autoloader/prism-autoloader.min.js'></script>"
                L"</head><body><pre><code class='language-" +
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
            std::wstring sourceUri = PathToFileUri(parameters_->filePath);
            size_t slash = sourceUri.find_last_of(L'/');
            std::wstring base = slash == std::wstring::npos ? L"" : sourceUri.substr(0, slash + 1);
            std::wstring html = L"<!doctype html><html><head><meta charset='utf-8'><base href='" +
                HtmlEncode(base) + L"'><style>html,body{margin:0;padding:16px;font:14px/1.6 Arial,sans-serif;background:" +
                CssColor(parameters_->theme.background) + L";color:" + CssColor(parameters_->theme.foreground) +
                L"}a{color:" + CssColor(parameters_->theme.accent) +
                L"}pre,code{font-family:Consolas,'Courier New',monospace}pre{padding:12px;overflow:auto}"
                L"table{border-collapse:collapse}th,td{border:1px solid #808080;padding:6px}img{max-width:100%}</style>"
                L"</head><body>" + fragment + L"</body></html>";
            webView_->NavigateToString(html.c_str());
            return;
        }
        std::wstring uri = PathToFileUri(parameters_->filePath);
        if (uri.empty())
            ShowError(parameters_->openFailed, E_INVALIDARG);
        else
            webView_->Navigate(uri.c_str());
    }

    void HandleCommand(int command)
    {
        if (command == IDM_NV_CLOSE)
            DestroyWindow(window_);
        else if (command == IDM_NV_REFRESH && webView_)
            webView_->Reload();
        else if (controller_ && (command == IDM_NV_ZOOM_IN || command == IDM_NV_ZOOM_OUT || command == IDM_NV_ZOOM_RESET))
        {
            double zoom = 1.0;
            controller_->get_ZoomFactor(&zoom);
            if (command == IDM_NV_ZOOM_IN)
                zoom = (std::min)(zoom + 0.1, 5.0);
            else if (command == IDM_NV_ZOOM_OUT)
                zoom = (std::max)(zoom - 0.1, 0.25);
            else
                zoom = 1.0;
            controller_->put_ZoomFactor(zoom);
        }
    }

    void ResizeChildren()
    {
        if (status_)
            SendMessageW(status_, WM_SIZE, 0, 0);
        RECT client = {};
        GetClientRect(window_, &client);
        if (status_)
        {
            RECT statusRect = {};
            GetWindowRect(status_, &statusRect);
            client.bottom -= statusRect.bottom - statusRect.top;
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
        webView_.Reset();
        if (controller_)
            controller_->Close();
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
    ComPtr<ICoreWebView2Environment> environment_;
    ComPtr<ICoreWebView2Controller> controller_;
    ComPtr<ICoreWebView2> webView_;
    EventRegistrationToken navigationToken_ = {};
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

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
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
