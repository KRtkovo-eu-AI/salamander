// SPDX-FileCopyrightText: 2023-2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

//****************************************************************************
//
// Copyright (c) 2023-2024 Open Salamander Authors
//
// This is a part of the Open Salamander SDK library.
//
//****************************************************************************

#include "precomp.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <unordered_set>
#include <vector>

// objekt interfacu pluginu, jeho metody se volaji ze Salamandera
CPluginInterface PluginInterface;
// cast interfacu CPluginInterface pro viewer
CPluginInterfaceForViewer InterfaceForViewer;
CPluginInterfaceForThumbLoader InterfaceForThumbLoader;

// globalni data
const char* PluginNameEN = "WebView2 Render Viewer .NET"; // neprekladane jmeno pluginu
const char* PluginNameShort = "WEBVIEW2VIEWER";    // jmeno pluginu (kratce, bez mezer)

HINSTANCE DLLInstance = NULL; // handle k SPL-ku - jazykove nezavisle resourcy
HINSTANCE HLanguage = NULL;   // handle k SLG-cku - jazykove zavisle resourcy

// obecne rozhrani Salamandera - platne od startu az do ukonceni pluginu
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
CSalamanderGUIAbstract* SalamanderGUI = NULL;

// definice promenne pro "dbg.h"
CSalamanderDebugAbstract* SalamanderDebug = NULL;

// maximum file size (in bytes) allowed for the managed viewer
static const ULONGLONG kMaxDocumentFileSize = 32ULL * 1024ULL * 1024ULL; // 32 MB

// definice promenne pro "spl_com.h"
int SalamanderVersion = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DLLInstance = hinstDLL;

        INITCOMMONCONTROLSEX initCtrls;
        initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
        initCtrls.dwICC = ICC_BAR_CLASSES;
        if (!InitCommonControlsEx(&initCtrls))
        {
            MessageBox(NULL, "InitCommonControlsEx failed!", "Error", MB_OK | MB_ICONERROR);
            return FALSE; // DLL won't start
        }
    }

    return TRUE; // DLL can be loaded
}

// ****************************************************************************

char* LoadStr(int resID)
{
    return SalamanderGeneral->LoadStr(HLanguage, resID);
}

static void ShowStartupError(HWND parent, const char* text)
{
    SalamanderGeneral->SalMessageBox(parent, text, LoadStr(IDS_PLUGINNAME), MB_OK | MB_ICONERROR);
}

static std::wstring ConvertPathToWide(const char* path)
{
    if (path == NULL)
        return std::wstring();

    int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (required <= 0)
    {
        codePage = CP_ACP;
        flags = 0;
        required = MultiByteToWideChar(codePage, flags, path, -1, NULL, 0);
    }
    if (required <= 0)
        return std::wstring();

    std::wstring result;
    result.resize(static_cast<size_t>(required));
    if (MultiByteToWideChar(codePage, flags, path, -1, result.data(), required) <= 0)
        return std::wstring();

    result.resize(static_cast<size_t>(required) - 1);
    return result;
}

static std::wstring MakeLongPath(const std::wstring& path)
{
    if (path.empty() || path.rfind(L"\\\\?\\", 0) == 0)
        return path;

    if (path.rfind(L"\\\\", 0) == 0)
        return L"\\\\?\\UNC\\" + path.substr(2);

    if (path.length() >= 3 && path[1] == L':' && (path[2] == L'\\' || path[2] == L'/'))
        return L"\\\\?\\" + path;

    return path;
}

static bool IsFileTooLarge(const char* path, ULONGLONG limit)
{
    if (path == NULL || path[0] == '\0')
        return false;

    std::wstring widePath = MakeLongPath(ConvertPathToWide(path));
    if (widePath.empty())
        return false;

    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (!GetFileAttributesExW(widePath.c_str(), GetFileExInfoStandard, &attrs))
        return false;

    if (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return false;

    ULONGLONG size = (static_cast<ULONGLONG>(attrs.nFileSizeHigh) << 32) | attrs.nFileSizeLow;
    return size > limit;
}

//
// ****************************************************************************
// SalamanderPluginGetReqVer
//

#ifdef __BORLANDC__
extern "C"
{
    int WINAPI SalamanderPluginGetReqVer();
    CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander);
};
#endif // __BORLANDC__

int WINAPI SalamanderPluginGetReqVer()
{
    return LAST_VERSION_OF_SALAMANDER;
}

//
// ****************************************************************************
// SalamanderPluginEntry
//

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    // nastavime SalamanderDebug pro "dbg.h"
    SalamanderDebug = salamander->GetSalamanderDebug();
    // nastavime SalamanderVersion pro "spl_com.h"
    SalamanderVersion = salamander->GetVersion();
    HANDLES_CAN_USE_TRACE();
    CALL_STACK_MESSAGE1("SalamanderPluginEntry()");

    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
    {
        MessageBox(salamander->GetParentWindow(),
                   REQUIRE_LAST_VERSION_OF_SALAMANDER,
                   PluginNameEN, MB_OK | MB_ICONERROR);
        return NULL;
    }

    // nechame nacist jazykovy modul (.slg)
    HLanguage = salamander->LoadLanguageModule(salamander->GetParentWindow(), PluginNameEN);
    if (HLanguage == NULL)
        return NULL;

    // ziskame obecne rozhrani Salamandera
    SalamanderGeneral = salamander->GetSalamanderGeneral();
    SalamanderGUI = salamander->GetSalamanderGUI();

    salamander->SetBasicPluginData(LoadStr(IDS_PLUGINNAME), FUNCTION_VIEWER,
                                   VERSINFO_VERSION_NO_PLATFORM, VERSINFO_COPYRIGHT,
                                   LoadStr(IDS_PLUGIN_DESCRIPTION), PluginNameShort,
                                   NULL, NULL);

    salamander->SetPluginHomePageURL(LoadStr(IDS_PLUGIN_HOME));

    return &PluginInterface;
}

//
// ****************************************************************************
// CPluginInterface
//

void WINAPI CPluginInterface::About(HWND parent)
{
    char text[1024];
    _snprintf_s(text, _TRUNCATE,
                "%s\n\n%s",
                LoadStr(IDS_PLUGINNAME),
                LoadStr(IDS_PLUGIN_DESCRIPTION));
    SalamanderGeneral->SalMessageBox(parent, text, LoadStr(IDS_ABOUT), MB_OK | MB_ICONINFORMATION);
}

BOOL WINAPI CPluginInterface::Release(HWND parent, BOOL force)
{
    if (!ManagedBridge_RequestShutdown(parent, force != FALSE))
        return FALSE;

    ManagedBridge_Shutdown();
    return TRUE;
}

void WINAPI CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    CALL_STACK_MESSAGE1("CPluginInterface::Connect(,)");

    static const char* const kViewerExtensions[] = {
        "html", "htm", "xhtml", "mhtml", "mht",
        "md", "markdown", "mdown", "mkd", "mdx",
        "svg", "svgz",
        "webp", "avif", "apng",
        "pdf"
    };

    std::unordered_set<std::string> extensions;
    extensions.reserve(_countof(kViewerExtensions));

    auto addExtension = [&extensions](const char* ext) {
        if (ext != NULL && ext[0] != '\0')
        {
            std::string lowered(ext);
            std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
            extensions.insert(lowered);
        }
    };

    for (const char* ext : kViewerExtensions)
    {
        addExtension(ext);
    }

    if (!extensions.empty())
    {
        constexpr size_t kMaxPatternLength = 200;

        std::string pattern;
        pattern.reserve(kMaxPatternLength);

        auto flushPattern = [&]() {
            if (!pattern.empty())
            {
                salamander->AddViewer(pattern.c_str(), FALSE);
                pattern.clear();
            }
        };

        for (const std::string& ext : extensions)
        {
            const std::string token = std::string("*.") + ext;
            const size_t separator = pattern.empty() ? 0 : 1;

            if (!pattern.empty() && (pattern.size() + separator + token.size()) > kMaxPatternLength)
            {
                flushPattern();
            }

            if (!pattern.empty())
            {
                pattern.push_back(';');
            }

            if (!pattern.empty() && (pattern.size() + token.size()) > kMaxPatternLength)
            {
                flushPattern();
            }

            pattern.append(token);

            if (pattern.size() >= kMaxPatternLength)
            {
                flushPattern();
            }
        }

        flushPattern();
    }

    salamander->SetThumbnailLoader("*.svg");

    if (SalamanderGUI != NULL)
    {
        CGUIIconListAbstract* iconList = SalamanderGUI->CreateIconList();
        if (iconList != NULL)
        {
            if (iconList->Create(16, 16, 1))
            {
                UINT loadFlags = SalamanderGeneral != NULL ? SalamanderGeneral->GetIconLRFlags() : LR_DEFAULTCOLOR;
                HICON icon16 = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_WEBVIEW2RENDERVIEWER), IMAGE_ICON, 16, 16, loadFlags);
                if (icon16 != NULL)
                {
                    iconList->ReplaceIcon(0, icon16);
                    DestroyIcon(icon16);
                    salamander->SetIconListForGUI(iconList);
                    salamander->SetPluginIcon(0);
                    salamander->SetPluginMenuAndToolbarIcon(0);
                    iconList = NULL;
                }
            }

            if (iconList != NULL)
                SalamanderGUI->DestroyIconList(iconList);
        }
    }
}

CPluginInterfaceForViewerAbstract* WINAPI CPluginInterface::GetInterfaceForViewer()
{
    return &InterfaceForViewer;
}

CPluginInterfaceForThumbLoaderAbstract* WINAPI CPluginInterface::GetInterfaceForThumbLoader()
{
    return &InterfaceForThumbLoader;
}

//

// ****************************************************************************
// CPluginInterfaceForThumbLoader
//

static bool HasSvgExtension(const char* filename)
{
    if (filename == NULL)
        return false;

    const char* extension = strrchr(filename, '.');
    return extension != NULL && _stricmp(extension, ".svg") == 0;
}

static bool FeedHBitmapToThumbnailMaker(HBITMAP bitmap, CSalamanderThumbnailMakerAbstract* thumbMaker)
{
    BITMAP bm;
    if (bitmap == NULL || GetObject(bitmap, sizeof(bm), &bm) != sizeof(bm) || bm.bmWidth <= 0 || bm.bmHeight <= 0)
        return false;

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bm.bmWidth;
    bmi.bmiHeader.biHeight = -bm.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    const size_t pixelCount = static_cast<size_t>(bm.bmWidth) * static_cast<size_t>(bm.bmHeight);
    if (pixelCount > (static_cast<size_t>(-1) / sizeof(DWORD)))
        return false;

    std::vector<DWORD> pixels(pixelCount);
    HDC dc = GetDC(NULL);
    if (dc == NULL)
        return false;

    int copied = GetDIBits(dc, bitmap, 0, static_cast<UINT>(bm.bmHeight), pixels.data(), &bmi, DIB_RGB_COLORS);
    ReleaseDC(NULL, dc);
    if (copied != bm.bmHeight)
        return false;

    if (!thumbMaker->SetParameters(bm.bmWidth, bm.bmHeight, 0))
        return false;

    return thumbMaker->ProcessBuffer(pixels.data(), bm.bmHeight) != FALSE;
}

static bool TryLoadShellThumbnail(const char* filename, int thumbWidth, int thumbHeight,
                                  CSalamanderThumbnailMakerAbstract* thumbMaker)
{
    std::wstring path = ConvertPathToWide(filename);
    if (path.empty())
        return false;

    IShellItemImageFactory* imageFactory = NULL;
    HRESULT hr = SHCreateItemFromParsingName(path.c_str(), NULL, IID_PPV_ARGS(&imageFactory));
    if (FAILED(hr) || imageFactory == NULL)
        return false;

    SIZE size;
    size.cx = thumbWidth > 1 ? thumbWidth : 1;
    size.cy = thumbHeight > 1 ? thumbHeight : 1;

    HBITMAP bitmap = NULL;
    hr = imageFactory->GetImage(size, static_cast<SIIGBF>(SIIGBF_BIGGERSIZEOK | SIIGBF_THUMBNAILONLY), &bitmap);
    imageFactory->Release();

    if (FAILED(hr) || bitmap == NULL)
        return false;

    bool loaded = FeedHBitmapToThumbnailMaker(bitmap, thumbMaker);
    DeleteObject(bitmap);
    return loaded;
}

static bool CreateTemporaryThumbnailPath(std::wstring& tempFile)
{
    tempFile.clear();

    DWORD required = GetTempPathW(0, NULL);
    if (required == 0)
        return false;

    std::vector<wchar_t> tempPath(static_cast<size_t>(required) + 1);
    DWORD copied = GetTempPathW(static_cast<DWORD>(tempPath.size()), tempPath.data());
    if (copied == 0 || copied >= tempPath.size())
        return false;

    std::wstring basePath(tempPath.data(), copied);
    if (!basePath.empty() && basePath.back() != L'\\' && basePath.back() != L'/')
        basePath.push_back(L'\\');

    for (DWORD attempt = 0; attempt < 100; ++attempt)
    {
        wchar_t name[64];
        swprintf_s(name, L"w2thumb-%lu-%lu-%llu-%lu.raw",
                   GetCurrentProcessId(), GetCurrentThreadId(), GetTickCount64(), attempt);

        std::wstring candidate = basePath + name;
        HANDLE file = CreateFileW(candidate.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW,
                                  FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, NULL);
        if (file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(file);
            tempFile = candidate;
            return true;
        }

        DWORD error = GetLastError();
        if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS)
            return false;
    }

    return false;
}

static bool ReadExact(HANDLE file, void* buffer, DWORD bytes)
{
    BYTE* out = static_cast<BYTE*>(buffer);
    DWORD remaining = bytes;
    while (remaining > 0)
    {
        DWORD read = 0;
        if (!ReadFile(file, out, remaining, &read, NULL) || read == 0)
            return false;

        out += read;
        remaining -= read;
    }

    return true;
}

static bool FeedRenderedThumbnailToMaker(const wchar_t* thumbnailPath, CSalamanderThumbnailMakerAbstract* thumbMaker)
{
    HANDLE file = CreateFileW(thumbnailPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    DWORD header[2];
    bool ok = ReadExact(file, header, sizeof(header));
    DWORD width = header[0];
    DWORD height = header[1];
    if (ok && (width == 0 || height == 0 || width > 4096 || height > 4096))
        ok = false;

    std::vector<DWORD> pixels;
    if (ok)
    {
        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
        if (pixelCount > (static_cast<size_t>(-1) / sizeof(DWORD)))
        {
            ok = false;
        }
        else
        {
            pixels.resize(pixelCount);
            ok = ReadExact(file, pixels.data(), static_cast<DWORD>(pixelCount * sizeof(DWORD))) != false;
        }
    }

    CloseHandle(file);

    if (!ok)
        return false;

    if (!thumbMaker->SetParameters(static_cast<int>(width), static_cast<int>(height), 0))
        return false;

    return thumbMaker->ProcessBuffer(pixels.data(), static_cast<int>(height)) != FALSE;
}

BOOL WINAPI CPluginInterfaceForThumbLoader::LoadThumbnail(const char* filename, int thumbWidth, int thumbHeight,
                                                          CSalamanderThumbnailMakerAbstract* thumbMaker,
                                                          BOOL fastThumbnail)
{
    CALL_STACK_MESSAGE5("CPluginInterfaceForThumbLoader::LoadThumbnail(%s, %d, %d, , %d)",
                        filename, thumbWidth, thumbHeight, fastThumbnail);

    if (!HasSvgExtension(filename))
        return FALSE;

    if (thumbMaker == NULL || thumbMaker->GetCancelProcessing())
        return TRUE;

    if (TryLoadShellThumbnail(filename, thumbWidth, thumbHeight, thumbMaker))
        return TRUE;

    if (thumbMaker->GetCancelProcessing())
        return TRUE;

    std::wstring tempFile;
    if (!CreateTemporaryThumbnailPath(tempFile))
    {
        thumbMaker->SetError();
        return TRUE;
    }

    bool rendered = ManagedBridge_RenderThumbnail(NULL, filename, thumbWidth, thumbHeight, tempFile.c_str());
    bool loaded = false;
    if (rendered && !thumbMaker->GetCancelProcessing())
        loaded = FeedRenderedThumbnailToMaker(tempFile.c_str(), thumbMaker);

    DeleteFileW(tempFile.c_str());

    if (!rendered || !loaded)
        thumbMaker->SetError();

    return TRUE;
}

// ****************************************************************************
// CPluginInterfaceForViewer
//

BOOL WINAPI CPluginInterfaceForViewer::ViewFile(const char* name, int left, int top, int width, int height,
                                                UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock,
                                                BOOL* lockOwner, CSalamanderPluginViewerData* viewerData,
                                                int enumFilesSourceUID, int enumFilesCurrentIndex)
{
    CALL_STACK_MESSAGE1("CPluginInterfaceForViewer::ViewFile()");

    if (name == NULL || name[0] == '\0')
        return FALSE;

    HWND parent = SalamanderGeneral->GetMainWindowHWND();

    if (IsFileTooLarge(name, kMaxDocumentFileSize))
    {
        SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_FILE_TOO_LARGE), LoadStr(IDS_PLUGINNAME),
                                         MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }

    RECT placement;
    placement.left = left;
    placement.top = top;
    placement.right = left + width;
    placement.bottom = top + height;

    if (returnLock)
    {
        HANDLE fileLock = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
        if (fileLock == NULL)
        {
            ShowStartupError(parent, LoadStr(IDS_VIEWER_CREATE_EVENT_FAILED));
            return FALSE;
        }

        if (!ManagedBridge_ViewDocument(parent, name, placement, showCmd, alwaysOnTop, fileLock, true))
        {
            HANDLES(CloseHandle(fileLock));
            return FALSE;
        }

        if (lock != NULL)
            *lock = fileLock;
        if (lockOwner != NULL)
            *lockOwner = TRUE;
        return TRUE;
    }

    return ManagedBridge_ViewDocument(parent, name, placement, showCmd, alwaysOnTop, NULL, true);
}

BOOL WINAPI CPluginInterfaceForViewer::CanViewFile(const char* name)
{
    if (name == NULL)
        return FALSE;

    const char* extension = strrchr(name, '.');
    if (extension == NULL)
        return FALSE;

    static const char* const kExtensions[] = {
        ".html", ".htm", ".xhtml", ".mhtml", ".mht",
        ".md", ".markdown", ".mdown", ".mkd", ".mdx",
        ".svg", ".svgz",
        ".webp", ".avif", ".apng", ".png", ".jpg", ".jpeg", ".jfif", ".gif", ".bmp", ".ico", ".tif", ".tiff",
        ".pdf"
    };

    for (size_t i = 0; i < _countof(kExtensions); ++i)
    {
        if (_stricmp(extension, kExtensions[i]) == 0)
            return TRUE;
    }

    return FALSE;
}
