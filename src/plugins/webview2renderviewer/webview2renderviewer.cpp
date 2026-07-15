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

#define NANOSVG_IMPLEMENTATION
#include "../../common/dep/nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../../common/dep/nanosvg/nanosvgrast.h"

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

static bool ReadSvgFile(const char* filename, std::vector<char>& svgData)
{
    svgData.clear();

    std::wstring path = MakeLongPath(ConvertPathToWide(filename));
    if (path.empty())
        return false;

    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    LARGE_INTEGER size;
    bool ok = GetFileSizeEx(file, &size) != FALSE && size.QuadPart > 0 &&
              static_cast<ULONGLONG>(size.QuadPart) <= kMaxDocumentFileSize;
    if (ok)
    {
        svgData.resize(static_cast<size_t>(size.QuadPart) + 1);
        DWORD read = 0;
        ok = ReadFile(file, svgData.data(), static_cast<DWORD>(size.QuadPart), &read, NULL) != FALSE &&
             read == static_cast<DWORD>(size.QuadPart);
        if (ok)
            svgData[static_cast<size_t>(size.QuadPart)] = '\0';
    }

    CloseHandle(file);
    if (!ok)
        svgData.clear();
    return ok;
}

static bool RenderSvgThumbnail(const char* filename, int thumbWidth, int thumbHeight,
                               CSalamanderThumbnailMakerAbstract* thumbMaker)
{
    if (thumbWidth <= 0 || thumbHeight <= 0)
        return false;

    std::vector<char> svgData;
    if (!ReadSvgFile(filename, svgData))
        return false;

    NSVGimage* image = nsvgParse(svgData.data(), "px", 96.0f);
    if (image == NULL || image->width <= 0.0f || image->height <= 0.0f)
    {
        if (image != NULL)
            nsvgDelete(image);
        return false;
    }

    NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
    if (rasterizer == NULL)
    {
        nsvgDelete(image);
        return false;
    }

    const float scaleX = static_cast<float>(thumbWidth) / image->width;
    const float scaleY = static_cast<float>(thumbHeight) / image->height;
    const float scale = scaleX < scaleY ? scaleX : scaleY;
    const float tx = (static_cast<float>(thumbWidth) - image->width * scale) * 0.5f;
    const float ty = (static_cast<float>(thumbHeight) - image->height * scale) * 0.5f;

    const size_t pixelCount = static_cast<size_t>(thumbWidth) * static_cast<size_t>(thumbHeight);
    if (pixelCount > (static_cast<size_t>(-1) / 4))
    {
        nsvgDeleteRasterizer(rasterizer);
        nsvgDelete(image);
        return false;
    }

    std::vector<unsigned char> rgba(pixelCount * 4, 0);
    nsvgRasterize(rasterizer, image, tx, ty, scale, rgba.data(), thumbWidth, thumbHeight, thumbWidth * 4);

    nsvgDeleteRasterizer(rasterizer);
    nsvgDelete(image);

    std::vector<DWORD> dibPixels(pixelCount);
    for (size_t i = 0; i < pixelCount; ++i)
    {
        const unsigned char r = rgba[i * 4];
        const unsigned char g = rgba[i * 4 + 1];
        const unsigned char b = rgba[i * 4 + 2];
        const unsigned char a = rgba[i * 4 + 3];

        const unsigned char outR = static_cast<unsigned char>((r * a + 255 * (255 - a)) / 255);
        const unsigned char outG = static_cast<unsigned char>((g * a + 255 * (255 - a)) / 255);
        const unsigned char outB = static_cast<unsigned char>((b * a + 255 * (255 - a)) / 255);
        dibPixels[i] = static_cast<DWORD>(outB) | (static_cast<DWORD>(outG) << 8) | (static_cast<DWORD>(outR) << 16);
    }

    if (!thumbMaker->SetParameters(thumbWidth, thumbHeight, 0))
        return false;

    return thumbMaker->ProcessBuffer(dibPixels.data(), thumbHeight) != FALSE;
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

    if (!RenderSvgThumbnail(filename, thumbWidth, thumbHeight, thumbMaker))
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
