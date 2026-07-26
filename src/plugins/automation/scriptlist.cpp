// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
	Automation Plugin for Open Salamander
	
	Copyright (c) 2009-2026 Milan Kase <manison@manison.cz>
	Copyright (c) 2010-2026 Open Salamander Authors
	
	scriptlist.cpp
	List of the scripts stored in the repositories.
*/

#include "precomp.h"
#include "scriptlist.h"
#include "extensionmanifest.h"
#include "scriptsite.h"
#include "aututils.h"
#include "engassoc.h"
#include "knownengines.h"
#include "automationplug.h"
#include "shim.h"
#include "abortpalette.h"
#include "abortmodal.h"
#include "lang\lang.rh"

extern HINSTANCE g_hInstance;
extern HINSTANCE g_hLangInst;
extern CSalamanderGeneralAbstract* SalamanderGeneral;

static HANDLE OpenReadFilePath(PCTSTR path)
{
    if (path == NULL)
        return INVALID_HANDLE_VALUE;
    std::wstring widePath;
#ifdef UNICODE
    widePath.assign(path);
#else
    int required = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    UINT codePage = CP_UTF8;
    if (required <= 0)
    {
        required = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
        codePage = CP_ACP;
    }
    if (required <= 0)
        return INVALID_HANDLE_VALUE;
    std::vector<wchar_t> converted(static_cast<size_t>(required));
    if (MultiByteToWideChar(
            codePage,
            codePage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0,
            path,
            -1,
            &converted[0],
            required) <= 0)
        return INVALID_HANDLE_VALUE;
    widePath.assign(&converted[0]);
#endif
    if (widePath.size() >= MAX_PATH && widePath.compare(0, 4, L"\\\\?\\") != 0)
    {
        if (widePath.size() >= 2 && widePath[0] == L'\\' && widePath[1] == L'\\')
            widePath = L"\\\\?\\UNC\\" + widePath.substr(2);
        else
            widePath = L"\\\\?\\" + widePath;
    }
    return ::CreateFileW(
        widePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

static BOOL Utf8ToWideText(const std::string& value, std::wstring& output)
{
    int required = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1, NULL, 0);
    if (required <= 0)
    {
        output.clear();
        return FALSE;
    }
    std::vector<wchar_t> buffer(static_cast<size_t>(required));
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1,
            &buffer[0], required) <= 0)
    {
        output.clear();
        return FALSE;
    }
    output.assign(&buffer[0]);
    return TRUE;
}

static volatile LONG g_nextRuntimeCommandMenuId = 0x60000000;
extern CAutomationPluginInterface g_oAutomationPlugin;

static BOOL ReadSmallTextFile(PCTSTR path, char* buffer, DWORD bufferSize)
{
    HANDLE hFile = OpenReadFilePath(path);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD bytesRead = 0;
    BOOL ok = ReadFile(hFile, buffer, bufferSize - 1, &bytesRead, NULL);
    HANDLES(CloseHandle(hFile));

    if (!ok || bytesRead == 0)
        return FALSE;

    buffer[bytesRead] = 0;
    return TRUE;
}

static BOOL ReadManifestTextFile(PCTSTR path, std::string& text)
{
    HANDLE hFile = OpenReadFilePath(path);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    LARGE_INTEGER fileSize;
    BOOL validSize =
        GetFileSizeEx(hFile, &fileSize) &&
        fileSize.QuadPart > 0 &&
        fileSize.QuadPart <= 1024 * 1024;
    if (!validSize)
    {
        HANDLES(CloseHandle(hFile));
        return FALSE;
    }

    text.resize(static_cast<size_t>(fileSize.QuadPart));
    DWORD bytesRead = 0;
    BOOL read = ReadFile(
        hFile, &text[0], static_cast<DWORD>(text.size()), &bytesRead, NULL);
    HANDLES(CloseHandle(hFile));
    if (!read || bytesRead != static_cast<DWORD>(text.size()))
    {
        text.clear();
        return FALSE;
    }
    return TRUE;
}

static BOOL Utf8ToNative(
    const std::string& value,
    PTSTR output,
    int outputCount)
{
    if (output == NULL || outputCount <= 0)
        return FALSE;

#ifdef UNICODE
    int converted = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1, output, outputCount);
    if (converted == 0)
        output[0] = L'\0';
    return converted != 0;
#else
    int wideLength = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1, NULL, 0);
    if (wideLength == 0)
    {
        output[0] = '\0';
        return FALSE;
    }
    std::vector<wchar_t> wideValue(static_cast<size_t>(wideLength));
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1,
            &wideValue[0], wideLength) == 0)
    {
        output[0] = '\0';
        return FALSE;
    }
    int converted = WideCharToMultiByte(
        CP_ACP, 0, &wideValue[0], -1, output, outputCount, NULL, NULL);
    if (converted == 0)
        output[0] = '\0';
    return converted != 0;
#endif
}

static BOOL NativeToUtf8(PCTSTR value, char* output, int outputCount)
{
    if (value == NULL || output == NULL || outputCount <= 0)
        return FALSE;

#ifdef UNICODE
    int converted = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, output, outputCount,
        NULL, NULL);
    if (converted == 0)
        output[0] = '\0';
    return converted != 0;
#else
    int wideLength = MultiByteToWideChar(CP_ACP, 0, value, -1, NULL, 0);
    if (wideLength == 0)
    {
        output[0] = '\0';
        return FALSE;
    }
    std::vector<wchar_t> wideValue(static_cast<size_t>(wideLength));
    if (MultiByteToWideChar(
            CP_ACP, 0, value, -1, &wideValue[0], wideLength) == 0)
    {
        output[0] = '\0';
        return FALSE;
    }
    int converted = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, &wideValue[0], -1,
        output, outputCount, NULL, NULL);
    if (converted == 0)
        output[0] = '\0';
    return converted != 0;
#endif
}

// SalamanderGeneral exposes the historical path helpers in UTF-8/ANSI form,
// while Automation is built in either ANSI or Unicode mode.  Keep the public
// call in one place and perform an explicit UTF-8 conversion for Unicode
// builds, without truncating the SAL_MAX_PATH buffer.
static BOOL AppendSalPath(PTSTR path, PCTSTR name, int pathCapacity)
{
    if (path == NULL || name == NULL || pathCapacity <= 0 ||
        SalamanderGeneral == NULL)
        return FALSE;

#ifdef UNICODE
    const size_t utf8Capacity = static_cast<size_t>(pathCapacity) * 3u;
    std::vector<char> pathUtf8(utf8Capacity);
    std::vector<char> nameUtf8(utf8Capacity);
    if (!NativeToUtf8(path, &pathUtf8[0], static_cast<int>(pathUtf8.size())) ||
        !NativeToUtf8(name, &nameUtf8[0], static_cast<int>(nameUtf8.size())))
        return FALSE;
    if (!SalamanderGeneral->SalPathAppend(
            &pathUtf8[0], &nameUtf8[0], static_cast<int>(pathUtf8.size())))
        return FALSE;
    return Utf8ToNative(
        std::string(&pathUtf8[0]), path, pathCapacity);
#else
    return SalamanderGeneral->SalPathAppend(path, name, pathCapacity);
#endif
}

struct RuntimeInputBoxContext
{
    std::wstring Title;
    std::wstring Prompt;
    std::wstring Initial;
    char* Output;
    DWORD OutputCapacity;
    BOOL Accepted;

    RuntimeInputBoxContext()
        : Output(NULL),
          OutputCapacity(0),
          Accepted(FALSE)
    {
    }
};

static void AppendDialogWord(std::vector<BYTE>& bytes, WORD value)
{
    bytes.push_back(static_cast<BYTE>(value & 0xff));
    bytes.push_back(static_cast<BYTE>((value >> 8) & 0xff));
}

static void AppendDialogString(std::vector<BYTE>& bytes, const wchar_t* value)
{
    if (value != NULL)
    {
        while (*value != L'\0')
        {
            AppendDialogWord(bytes, static_cast<WORD>(*value));
            ++value;
        }
    }
    AppendDialogWord(bytes, 0);
}

static void AlignDialogTemplate(std::vector<BYTE>& bytes)
{
    while ((bytes.size() & 3) != 0)
        bytes.push_back(0);
}

static void AppendDialogItem(
    std::vector<BYTE>& bytes,
    short x,
    short y,
    short width,
    short height,
    WORD id,
    DWORD style,
    WORD classOrdinal,
    const wchar_t* title)
{
    AlignDialogTemplate(bytes);
    size_t offset = bytes.size();
    bytes.resize(offset + sizeof(DLGITEMTEMPLATE), 0);
    DLGITEMTEMPLATE* item =
        reinterpret_cast<DLGITEMTEMPLATE*>(&bytes[offset]);
    item->x = x;
    item->y = y;
    item->cx = width;
    item->cy = height;
    item->id = id;
    item->style = style;
    item->dwExtendedStyle = 0;
    AppendDialogWord(bytes, 0xffff);
    AppendDialogWord(bytes, classOrdinal);
    AppendDialogString(bytes, title);
    AppendDialogWord(bytes, 0);
}

static INT_PTR CALLBACK RuntimeInputBoxProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    RuntimeInputBoxContext* context =
        reinterpret_cast<RuntimeInputBoxContext*>(
            GetWindowLongPtr(hwnd, DWLP_USER));
    if (message == WM_INITDIALOG)
    {
        context = reinterpret_cast<RuntimeInputBoxContext*>(lParam);
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);
        SetWindowTextW(hwnd, context->Title.c_str());
        SetDlgItemTextW(hwnd, 1000, context->Prompt.c_str());
        SetDlgItemTextW(hwnd, 1001, context->Initial.c_str());
        SetFocus(GetDlgItem(hwnd, 1001));
        SendDlgItemMessage(hwnd, 1001, EM_SETSEL, 0, -1);
        return FALSE;
    }
    if (message != WM_COMMAND || context == NULL)
        return FALSE;
    WORD command = LOWORD(wParam);
    if (command != IDOK && command != IDCANCEL)
        return FALSE;
    if (command == IDOK && context->Output != NULL &&
        context->OutputCapacity != 0)
    {
        wchar_t value[4096];
        GetDlgItemTextW(hwnd, 1001, value, _countof(value));
#ifdef UNICODE
        NativeToUtf8(value, context->Output,
                     static_cast<int>(context->OutputCapacity));
#else
        WideCharToMultiByte(CP_UTF8, 0, value, -1, context->Output,
                            static_cast<int>(context->OutputCapacity), NULL, NULL);
#endif
        context->Accepted = TRUE;
    }
    EndDialog(hwnd, command);
    return TRUE;
}

BOOL ShowRuntimeInputBox(
    HWND parent,
    const std::string& title,
    const std::string& prompt,
    const std::string& initial,
    char* output,
    DWORD outputCapacity)
{
    if (output == NULL || outputCapacity == 0)
        return FALSE;
    output[0] = '\0';
    RuntimeInputBoxContext context;
    wchar_t titleBuffer[512];
    wchar_t promptBuffer[2048];
    wchar_t initialBuffer[4096];
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, title.c_str(), -1,
                            titleBuffer, _countof(titleBuffer)) == 0 ||
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, prompt.c_str(), -1,
                            promptBuffer, _countof(promptBuffer)) == 0 ||
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, initial.c_str(), -1,
                            initialBuffer, _countof(initialBuffer)) == 0)
        return FALSE;
    context.Title.assign(titleBuffer);
    context.Prompt.assign(promptBuffer);
    context.Initial.assign(initialBuffer);
    context.Output = output;
    context.OutputCapacity = outputCapacity;

    std::vector<BYTE> dialog;
    dialog.resize(sizeof(DLGTEMPLATE), 0);
    DLGTEMPLATE* header = reinterpret_cast<DLGTEMPLATE*>(&dialog[0]);
    header->style = WS_POPUP | WS_BORDER | WS_SYSMENU | WS_CAPTION |
                    DS_MODALFRAME | DS_SETFONT;
    header->dwExtendedStyle = 0;
    header->cdit = 4;
    header->x = 10;
    header->y = 10;
    header->cx = 260;
    header->cy = 92;
    AppendDialogWord(dialog, 0); // no menu
    AppendDialogWord(dialog, 0); // default dialog class
    AppendDialogString(dialog, context.Title.c_str());
    AppendDialogWord(dialog, 8); // point size
    AppendDialogString(dialog, L"MS Shell Dlg");
    AppendDialogItem(dialog, 8, 8, 244, 12, 1000,
                     WS_CHILD | WS_VISIBLE, 0x0082, context.Prompt.c_str());
    AppendDialogItem(dialog, 8, 24, 244, 14, 1001,
                     WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                     0x0081, context.Initial.c_str());
    AppendDialogItem(dialog, 142, 60, 52, 14, IDOK,
                     WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                     0x0080, L"OK");
    AppendDialogItem(dialog, 200, 60, 52, 14, IDCANCEL,
                     WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                     0x0080, L"Cancel");
    DialogBoxIndirectParamW(
        g_hLangInst,
        reinterpret_cast<DLGTEMPLATE*>(&dialog[0]),
        parent,
        RuntimeInputBoxProc,
        reinterpret_cast<LPARAM>(&context));
    return context.Accepted;
}

static BOOL PathsEqual(PCTSTR first, PCTSTR second)
{
    std::vector<TCHAR> firstFull(SAL_MAX_PATH);
    std::vector<TCHAR> secondFull(SAL_MAX_PATH);
    DWORD firstLength = GetFullPathName(
        first, static_cast<DWORD>(firstFull.size()), &firstFull[0], NULL);
    DWORD secondLength = GetFullPathName(
        second, static_cast<DWORD>(secondFull.size()), &secondFull[0], NULL);
    if (firstLength == 0 || firstLength >= firstFull.size() ||
        secondLength == 0 || secondLength >= secondFull.size())
    {
        return FALSE;
    }
    return _tcsicmp(&firstFull[0], &secondFull[0]) == 0;
}

static BOOL LoadManifestForEntryPoint(
    PCTSTR entryPointPath,
    CExtensionManifest& manifest)
{
    std::vector<TCHAR> directory(SAL_MAX_PATH);
    StringCchCopy(&directory[0], directory.size(), entryPointPath);
    if (!PathRemoveFileSpec(&directory[0]))
        return FALSE;

    for (int level = 0; level < 32; ++level)
    {
        std::vector<TCHAR> manifestPath(SAL_MAX_PATH);
        StringCchCopy(&manifestPath[0], manifestPath.size(), &directory[0]);
        if (!AppendSalPath(
                &manifestPath[0], _T("extension.json"),
                static_cast<int>(manifestPath.size())))
        {
            return FALSE;
        }

        std::string json;
        if (ReadManifestTextFile(&manifestPath[0], json))
        {
            CExtensionManifest candidate;
            CExtensionManifestError error;
            if (candidate.Parse(json.data(), json.size(), error))
            {
                std::vector<TCHAR> nativeEntryPoint(SAL_MAX_PATH);
                if (Utf8ToNative(
                        candidate.EntryPoint,
                        &nativeEntryPoint[0],
                        static_cast<int>(nativeEntryPoint.size())))
                {
                    std::vector<TCHAR> resolvedEntryPoint(SAL_MAX_PATH);
                    StringCchCopy(
                        &resolvedEntryPoint[0], resolvedEntryPoint.size(),
                        &directory[0]);
                    if (AppendSalPath(
                            &resolvedEntryPoint[0], &nativeEntryPoint[0],
                            static_cast<int>(resolvedEntryPoint.size())) &&
                        PathsEqual(&resolvedEntryPoint[0], entryPointPath))
                    {
                        manifest = candidate;
                        return TRUE;
                    }
                }
            }
        }

        std::vector<TCHAR> parent(SAL_MAX_PATH);
        StringCchCopy(&parent[0], parent.size(), &directory[0]);
        if (!PathRemoveFileSpec(&parent[0]) ||
            _tcsicmp(&parent[0], &directory[0]) == 0)
            break;
        StringCchCopy(&directory[0], directory.size(), &parent[0]);
    }
    return FALSE;
}

static BOOL ResolveManifestAssetPath(
    PCTSTR entryPointPath,
    const std::string& manifestEntryPoint,
    const std::string& asset,
    std::string& resolved)
{
    resolved.clear();
    if (asset.empty())
        return TRUE;

    std::vector<TCHAR> base(SAL_MAX_PATH);
    if (StringCchCopy(&base[0], base.size(), entryPointPath) != S_OK ||
        !PathRemoveFileSpec(&base[0]))
        return FALSE;

    std::vector<TCHAR> nativeEntryPoint(SAL_MAX_PATH);
    if (!Utf8ToNative(manifestEntryPoint, &nativeEntryPoint[0],
                      static_cast<int>(nativeEntryPoint.size())))
        return FALSE;

    // entryPointPath currently points at the entry point. Move from its
    // directory to the package root represented by manifest.EntryPoint.
    for (const TCHAR* p = &nativeEntryPoint[0]; *p != _T('\0'); ++p)
    {
        if (*p == _T('\\') || *p == _T('/'))
        {
            if (!PathRemoveFileSpec(&base[0]))
                return FALSE;
        }
    }

    std::vector<TCHAR> nativeAsset(SAL_MAX_PATH);
    if (!Utf8ToNative(asset, &nativeAsset[0],
                      static_cast<int>(nativeAsset.size())) ||
        !AppendSalPath(&base[0], &nativeAsset[0],
                       static_cast<int>(base.size())))
        return FALSE;

    std::vector<char> utf8Path(SAL_MAX_PATH * 3);
    if (!NativeToUtf8(&base[0], &utf8Path[0],
                      static_cast<int>(utf8Path.size())))
        return FALSE;
    resolved.assign(&utf8Path[0]);
    return TRUE;
}

static std::string GetPreferredManifestLocale()
{
    WCHAR localeName[LOCALE_NAME_MAX_LENGTH];
    if (GetUserDefaultLocaleName(localeName, _countof(localeName)) == 0)
        return std::string();
    int length = WideCharToMultiByte(
        CP_UTF8, 0, localeName, -1, NULL, 0, NULL, NULL);
    if (length <= 1)
        return std::string();
    std::vector<char> utf8(static_cast<size_t>(length));
    if (WideCharToMultiByte(
            CP_UTF8, 0, localeName, -1, &utf8[0], length, NULL, NULL) == 0)
    {
        return std::string();
    }
    return std::string(&utf8[0]);
}

static int GetManifestLocaleMatchScore(
    const std::string& available,
    const std::string& preferred)
{
    const size_t availableSeparator = available.find('-');
    const std::string availablePrimary = available.substr(0, availableSeparator);
    if (preferred.empty())
        return _stricmp(availablePrimary.c_str(), "en") == 0 ? 1 : 0;
    if (_stricmp(available.c_str(), preferred.c_str()) == 0)
        return 4;

    const size_t preferredSeparator = preferred.find('-');
    const std::string preferredPrimary = preferred.substr(0, preferredSeparator);
    if (_stricmp(availablePrimary.c_str(), preferredPrimary.c_str()) == 0)
    {
        // Prefer the neutral language over a different regional variant.
        return availableSeparator == std::string::npos ? 3 : 2;
    }
    return _stricmp(availablePrimary.c_str(), "en") == 0 ? 1 : 0;
}

static BOOL LoadManifestLocaleText(
    PCTSTR entryPointPath,
    const CExtensionManifest& manifest,
    CExtensionManifestLocaleText& localized)
{
    if (manifest.Locales.empty())
        return FALSE;

    const std::string preferred = GetPreferredManifestLocale();
    int selected = -1;
    int bestScore = 0;
    for (size_t index = 0; index < manifest.Locales.size(); ++index)
    {
        const int score = GetManifestLocaleMatchScore(
            manifest.Locales[index].Language, preferred);
        if (score > bestScore)
        {
            bestScore = score;
            selected = static_cast<int>(index);
        }
    }
    if (selected < 0)
        return FALSE;

    std::string resolvedPath;
    if (!ResolveManifestAssetPath(
            entryPointPath, manifest.EntryPoint,
            manifest.Locales[selected].File, resolvedPath))
    {
        return FALSE;
    }
    std::vector<TCHAR> nativePath(resolvedPath.size() * 2 + 2);
    if (!Utf8ToNative(
            resolvedPath, &nativePath[0],
            static_cast<int>(nativePath.size())))
    {
        return FALSE;
    }
    std::string json;
    if (!ReadManifestTextFile(&nativePath[0], json))
        return FALSE;
    CExtensionManifestError error;
    return CExtensionManifest::ParseLocaleText(
               json.data(), json.size(), localized, error)
               ? TRUE
               : FALSE;
}

static const char* FindLocalizedManifestCommandTitle(
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

CScriptInfo::CScriptInfo(
    PCTSTR pszFileName,
    CScriptContainer* pContainer)
{
    PCTSTR pszNameStart, pszNameEnd;

    StringCchCopy(m_szFileName, _countof(m_szFileName), pszFileName);

    pszNameStart = PathFindFileName(pszFileName);
    pszNameEnd = PathFindExtension(pszNameStart);
    StringCchCopyN(m_szDisplayName, _countof(m_szDisplayName), pszNameStart, pszNameEnd - pszNameStart);
    m_szSalamatrixCommandId[0] = _T('\0');
    m_szRuntimeCommandId[0] = '\0';
    m_szSalamatrixExtensionId[0] = '\0';
    m_szSalamatrixRuntimeId[0] = '\0';
    m_dwSalamatrixMinimumRuntimeVersion = 0;
    m_bSalamatrixEventsDeclared = false;
    m_salamatrixDependencies.clear();
    m_salamatrixSettings.clear();
    m_salamatrixEvents.clear();
    m_salamatrixManifestCommands.clear();
    m_bSalamatrixManifestCommandsPublished = false;
    m_bShowInPluginMenu = true;
    m_bShowInContextMenu = false;
    m_bManifestToolbar = false;
    m_salamatrixIconPath.clear();
    m_salamatrixIconDarkPath.clear();
    m_bRuntimeCommandOwned = false;
    memset(m_runtimeCommands, 0, sizeof(m_runtimeCommands));
    m_nRuntimeCommands = 0;
    m_dwMenuEventOrMask = MENU_EVENT_TRUE;
    m_dwMenuEventAndMask = MENU_EVENT_TRUE;
    LoadSalamatrixMetadata();
    LoadSalamatrixManifestMetadata();

    m_clsidEngine = CLSID_NULL;

    m_pScript = NULL;
    m_pSite = NULL;
    m_pHardError = NULL;

    m_cExecuted = 0;

    m_pExecInfo = NULL;

    m_bAbortPending = false;
    m_hAbortEvent = NULL;
    m_hwndAbortTarget = NULL;
    m_pRuntimeSession = NULL;
    m_hRuntimePumpThread = NULL;
    memset(m_runtimeEventSubscriptions, 0, sizeof(m_runtimeEventSubscriptions));
    m_nRuntimeEventSubscriptions = 0;
    memset(m_runtimeDialogs, 0, sizeof(m_runtimeDialogs));
    m_nRuntimeDialogs = 0;
    m_nextRuntimeDialogId = 1;
    memset(&m_runtimeProgress, 0, sizeof(m_runtimeProgress));
    m_nextRuntimeProgressId = 1;

    m_pNext = NULL;
    m_pNextHash = NULL;
    m_pContainer = pContainer;
    m_nId = 0;

    m_bDirty = false;

    m_pShim = NULL;
}

CScriptInfo::~CScriptInfo()
{
    ReleaseRuntimeSession();
    if (m_pScript != NULL)
    {
        m_pScript->Release();
    }
}

void CScriptInfo::ApplySalamatrixPlacement(PCTSTR pszValue)
{
    if (_tcsicmp(pszValue, _T("plugin")) == 0)
    {
        m_bShowInPluginMenu = true;
        m_bShowInContextMenu = false;
    }
    else if (_tcsicmp(pszValue, _T("context")) == 0)
    {
        m_bShowInPluginMenu = false;
        m_bShowInContextMenu = true;
    }
    else if (_tcsicmp(pszValue, _T("both")) == 0)
    {
        m_bShowInPluginMenu = true;
        m_bShowInContextMenu = true;
    }
    else if (_tcsicmp(pszValue, _T("none")) == 0)
    {
        m_bShowInPluginMenu = false;
        m_bShowInContextMenu = false;
    }
}

void CScriptInfo::ApplySalamatrixContextMenu(bool value)
{
    m_bShowInContextMenu = value;
}

void CScriptInfo::ApplySalamatrixRequires(PCTSTR pszValue)
{
    if (_tcsicmp(pszValue, _T("any")) == 0)
    {
        m_dwMenuEventOrMask = MENU_EVENT_TRUE;
        m_dwMenuEventAndMask = MENU_EVENT_TRUE;
    }
    else if (_tcsicmp(pszValue, _T("disk")) == 0)
    {
        m_dwMenuEventOrMask = MENU_EVENT_TRUE;
        m_dwMenuEventAndMask = MENU_EVENT_DISK;
    }
    else if (_tcsicmp(pszValue, _T("focused")) == 0)
    {
        m_dwMenuEventOrMask = MENU_EVENT_FILE_FOCUSED | MENU_EVENT_DIR_FOCUSED;
        m_dwMenuEventAndMask = MENU_EVENT_DISK;
    }
    else if (_tcsicmp(pszValue, _T("file")) == 0)
    {
        m_dwMenuEventOrMask = MENU_EVENT_FILE_FOCUSED | MENU_EVENT_FILES_SELECTED;
        m_dwMenuEventAndMask = MENU_EVENT_DISK;
    }
    else if (_tcsicmp(pszValue, _T("selection")) == 0)
    {
        m_dwMenuEventOrMask = MENU_EVENT_FILES_SELECTED | MENU_EVENT_DIRS_SELECTED;
        m_dwMenuEventAndMask = MENU_EVENT_DISK;
    }
}

void CScriptInfo::ApplySalamatrixMetadataLine(PCTSTR pszLine)
{
    static const TCHAR COMMAND_ID_PREFIX[] = _T("// Salamatrix.CommandId:");
    static const TCHAR COMMAND_TITLE_PREFIX[] = _T("// Salamatrix.CommandTitle:");
    static const TCHAR COMMAND_MENU_PREFIX[] = _T("// Salamatrix.CommandMenu:");
    static const TCHAR COMMAND_CONTEXT_MENU_PREFIX[] = _T("// Salamatrix.CommandContextMenu:");
    static const TCHAR COMMAND_REQUIRES_PREFIX[] = _T("// Salamatrix.CommandRequires:");

    while (*pszLine == _T(' ') || *pszLine == _T('\t'))
        ++pszLine;

    if (_tcsnicmp(pszLine, COMMAND_ID_PREFIX, _countof(COMMAND_ID_PREFIX) - 1) == 0)
    {
        pszLine += _countof(COMMAND_ID_PREFIX) - 1;
        while (*pszLine == _T(' ') || *pszLine == _T('\t'))
            ++pszLine;
        StringCchCopy(m_szSalamatrixCommandId, _countof(m_szSalamatrixCommandId), pszLine);
    }
    else if (_tcsnicmp(pszLine, COMMAND_TITLE_PREFIX, _countof(COMMAND_TITLE_PREFIX) - 1) == 0)
    {
        pszLine += _countof(COMMAND_TITLE_PREFIX) - 1;
        while (*pszLine == _T(' ') || *pszLine == _T('\t'))
            ++pszLine;
        StringCchCopy(m_szDisplayName, _countof(m_szDisplayName), pszLine);
    }
    else if (_tcsnicmp(pszLine, COMMAND_MENU_PREFIX, _countof(COMMAND_MENU_PREFIX) - 1) == 0)
    {
        pszLine += _countof(COMMAND_MENU_PREFIX) - 1;
        while (*pszLine == _T(' ') || *pszLine == _T('\t'))
            ++pszLine;
        ApplySalamatrixPlacement(pszLine);
    }
    else if (_tcsnicmp(pszLine, COMMAND_CONTEXT_MENU_PREFIX, _countof(COMMAND_CONTEXT_MENU_PREFIX) - 1) == 0)
    {
        pszLine += _countof(COMMAND_CONTEXT_MENU_PREFIX) - 1;
        while (*pszLine == _T(' ') || *pszLine == _T('\t'))
            ++pszLine;
        ApplySalamatrixContextMenu(_tcsicmp(pszLine, _T("true")) == 0);
    }
    else if (_tcsnicmp(pszLine, COMMAND_REQUIRES_PREFIX, _countof(COMMAND_REQUIRES_PREFIX) - 1) == 0)
    {
        pszLine += _countof(COMMAND_REQUIRES_PREFIX) - 1;
        while (*pszLine == _T(' ') || *pszLine == _T('\t'))
            ++pszLine;
        ApplySalamatrixRequires(pszLine);
    }
}

void CScriptInfo::ApplySalamatrixManifestValue(const char* key, const char* value)
{
    if (value == NULL || value[0] == 0)
        return;

    TCHAR converted[256];
    if (!Utf8ToNative(value, converted, _countof(converted)))
        return;

    if (strcmp(key, "id") == 0)
    {
        StringCchCopy(m_szSalamatrixCommandId, _countof(m_szSalamatrixCommandId), converted);
    }
    else if (strcmp(key, "title") == 0 || strcmp(key, "name") == 0)
    {
        StringCchCopy(m_szDisplayName, _countof(m_szDisplayName), converted);
    }
    else if (strcmp(key, "menu") == 0 || strcmp(key, "placement") == 0)
    {
        ApplySalamatrixPlacement(converted);
    }
    else if (strcmp(key, "requires") == 0)
    {
        ApplySalamatrixRequires(converted);
    }
}

void CScriptInfo::LoadSalamatrixManifestMetadata()
{
    CExtensionManifest manifest;
    if (!LoadManifestForEntryPoint(m_szFileName, manifest))
        return;

    // Locale resources are package data, resolved only through a manifest
    // entry whose safe path was validated above. A broken optional translation
    // never prevents the base manifest from being discovered.
    const std::string baseManifestName = manifest.Name;
    CExtensionManifestLocaleText localized;
    if (LoadManifestLocaleText(m_szFileName, manifest, localized))
    {
        if (!localized.Name.empty())
            manifest.Name = localized.Name;
        for (size_t commandIndex = 0;
             commandIndex < manifest.Commands.size();
             ++commandIndex)
        {
            const char* title = FindLocalizedManifestCommandTitle(
                localized, manifest.Commands[commandIndex].Id);
            if (title != NULL)
            {
                manifest.Commands[commandIndex].Title = title;
            }
            else if (manifest.Commands[commandIndex].Title == baseManifestName)
            {
                // The default command title follows a translated extension
                // name even when the locale file does not list it separately.
                manifest.Commands[commandIndex].Title = manifest.Name;
            }
        }
    }

    ApplySalamatrixManifestValue("title", manifest.Name.c_str());

    StringCchCopyA(
        m_szSalamatrixExtensionId,
        _countof(m_szSalamatrixExtensionId),
        manifest.Id.c_str());
    StringCchCopyA(
        m_szSalamatrixRuntimeId,
        _countof(m_szSalamatrixRuntimeId),
        manifest.RuntimeId.c_str());
    m_dwSalamatrixMinimumRuntimeVersion = manifest.MinimumRuntimeVersion;
    m_salamatrixCapabilities = manifest.Capabilities;
    m_salamatrixDependencies = manifest.Dependencies;
    m_salamatrixSettings = manifest.Settings;
    m_bSalamatrixEventsDeclared = manifest.EventsDeclared;
    m_salamatrixEvents = manifest.Events;
    ResolveManifestAssetPath(
        m_szFileName, manifest.EntryPoint, manifest.Icon,
        m_salamatrixIconPath);
    ResolveManifestAssetPath(
        m_szFileName, manifest.EntryPoint, manifest.IconDark,
        m_salamatrixIconDarkPath);

    m_salamatrixManifestCommands.clear();
    const size_t maxManifestCommands = 16;
    for (size_t index = 0;
         index < manifest.Commands.size() &&
         m_salamatrixManifestCommands.size() < maxManifestCommands;
         ++index)
    {
        const CExtensionManifestCommand& command = manifest.Commands[index];
        if (command.Id.empty())
            continue;

        SALAMATRIX_MANIFEST_COMMAND published;
        published.Id = command.Id;
        published.Handler = command.Handler;
        published.Title = command.Title.empty() ? command.Id : command.Title;
        published.Menu = command.Menu.empty() ? "plugin" : command.Menu;
        published.Requires = command.Requires.empty() ? "any" : command.Requires;
        ResolveManifestAssetPath(
            m_szFileName, manifest.EntryPoint, command.Icon,
            published.IconPath);
        ResolveManifestAssetPath(
            m_szFileName, manifest.EntryPoint, command.IconDark,
            published.IconDarkPath);
        published.ContextMenu = command.ContextMenu;
        published.Toolbar = command.Toolbar;

        const bool firstCommand = m_salamatrixManifestCommands.empty();
        if (firstCommand)
        {
            ApplySalamatrixManifestValue("id", command.Id.c_str());
            ApplySalamatrixManifestValue("title", published.Title.c_str());
            ApplySalamatrixManifestValue("menu", published.Menu.c_str());
            ApplySalamatrixContextMenu(command.ContextMenu);
            ApplySalamatrixManifestValue("requires", published.Requires.c_str());
            m_bManifestToolbar = command.Toolbar;
            published.MenuEventOrMask = m_dwMenuEventOrMask;
            published.MenuEventAndMask = m_dwMenuEventAndMask;
        }
        else
        {
            DWORD savedOrMask = m_dwMenuEventOrMask;
            DWORD savedAndMask = m_dwMenuEventAndMask;
            ApplySalamatrixManifestValue("requires", published.Requires.c_str());
            published.MenuEventOrMask = m_dwMenuEventOrMask;
            published.MenuEventAndMask = m_dwMenuEventAndMask;
            m_dwMenuEventOrMask = savedOrMask;
            m_dwMenuEventAndMask = savedAndMask;
        }
        m_salamatrixManifestCommands.push_back(published);
    }
}

void CScriptInfo::InitializeSalamatrixSettings(
    Salamatrix::Storage::IStorageService* storage)
{
    if (storage == NULL || m_szSalamatrixExtensionId[0] == '\0')
        return;

    for (size_t index = 0; index < m_salamatrixSettings.size(); ++index)
    {
        const CExtensionManifestSetting& setting = m_salamatrixSettings[index];
        if (!setting.HasDefault ||
            storage->GetValueType(
                m_szSalamatrixExtensionId, setting.Key.c_str()) !=
                Salamatrix::Storage::StorageValueMissing)
            continue;

        switch (setting.Type)
        {
        case ExtensionManifestSettingString:
            storage->SetString(
                m_szSalamatrixExtensionId,
                setting.Key.c_str(),
                setting.StringDefault.c_str());
            break;
        case ExtensionManifestSettingInteger:
            storage->SetInteger(
                m_szSalamatrixExtensionId,
                setting.Key.c_str(),
                static_cast<LONGLONG>(setting.IntegerDefault));
            break;
        case ExtensionManifestSettingBoolean:
            storage->SetBoolean(
                m_szSalamatrixExtensionId,
                setting.Key.c_str(),
                setting.BooleanDefault ? TRUE : FALSE);
            break;
        default:
            break;
        }
    }
}

void CScriptInfo::LoadSalamatrixMetadata()
{
    char buffer[4097];
    if (!ReadSmallTextFile(m_szFileName, buffer, sizeof(buffer)))
        return;

    char* line = buffer;
    while (line != NULL && *line != 0)
    {
        char* next = strchr(line, '\n');
        if (next != NULL)
        {
            *next = 0;
            ++next;
        }

        char* end = line + strlen(line);
        while (end > line && (end[-1] == '\r' || end[-1] == '\n'))
        {
            --end;
            *end = 0;
        }

#ifdef UNICODE
        TCHAR wideLine[512];
        MultiByteToWideChar(CP_ACP, 0, line, -1, wideLine, _countof(wideLine));
        wideLine[_countof(wideLine) - 1] = 0;
        ApplySalamatrixMetadataLine(wideLine);
#else
        ApplySalamatrixMetadataLine(line);
#endif

        line = next;
    }
}

PCTSTR CScriptInfo::GetFileExt() const
{
    return PathFindExtension(m_szFileName);
}

bool CScriptInfo::Execute(__inout EXECUTION_INFO& info)
{
    if (m_szSalamatrixRuntimeId[0] != '\0')
        return ExecuteThroughRuntime(info);

    return ExecuteLegacy(info);
}

bool CScriptInfo::ExecuteLegacy(__inout EXECUTION_INFO& info)
{
    if (!EnsureEngineAssociation())
    {
        return false;
    }

    //return ExecuteInSeparateThread(&info);
    return ExecuteWorker(&info);
}

struct CCompatibilityExecutionContext
{
    CScriptInfo* Script;
    CScriptInfo::EXECUTION_INFO* Info;
};

BOOL WINAPI CScriptInfo::ExecuteCompatibilityRuntime(
    void* context,
    Salamatrix::Runtime::RuntimeExecutionResult* result)
{
    CCompatibilityExecutionContext* execution =
        static_cast<CCompatibilityExecutionContext*>(context);
    if (execution == NULL || execution->Script == NULL ||
        execution->Info == NULL || result == NULL)
    {
        return FALSE;
    }

    bool succeeded = execution->Script->ExecuteLegacy(*execution->Info);
    result->Status = succeeded
                         ? Salamatrix::Runtime::RuntimeExecutionStatusSucceeded
                         : Salamatrix::Runtime::RuntimeExecutionStatusFailed;
    result->ErrorCode = succeeded ? S_OK : E_FAIL;
    return succeeded ? TRUE : FALSE;
}

bool CScriptInfo::ExecuteThroughRuntime(__inout EXECUTION_INFO& info)
{
    CAutomationSalamatrixBridge* bridge = g_oAutomationPlugin.GetSalamatrixBridge();
    if (!bridge->HasRuntimeBroker())
    {
        g_oAutomationPlugin.RefreshSalamatrixServices();
        bridge = g_oAutomationPlugin.GetSalamatrixBridge();
    }

    Salamatrix::Runtime::IRuntimeService* runtimeService = bridge->GetRuntimeService();
    Salamatrix::Runtime::IRuntimeAdapter* adapter =
        runtimeService != NULL
            ? runtimeService->FindAdapter(
                  m_szSalamatrixRuntimeId,
                  m_dwSalamatrixMinimumRuntimeVersion)
            : NULL;

    std::vector<char> entryPointUtf8(SAL_MAX_PATH * 3);
    if (adapter == NULL ||
        !NativeToUtf8(
            m_szFileName,
            &entryPointUtf8[0],
            static_cast<int>(entryPointUtf8.size())) ||
        !adapter->IsAvailable() ||
        !adapter->SupportsEntryPoint(&entryPointUtf8[0]))
    {
        TCHAR message[512];
        TCHAR runtimeId[128];
        Utf8ToNative(m_szSalamatrixRuntimeId, runtimeId, _countof(runtimeId));
        StringCchPrintf(
            message,
            _countof(message),
            _T("The extension requires runtime '%s', but no compatible and available adapter is registered."),
            runtimeId);
        SalamanderGeneral->SalMessageBox(
            SalamanderGeneral->GetMsgBoxParent(),
            message,
            SalamanderGeneral->LoadStr(g_hLangInst, IDS_PLUGINNAME),
            MB_OK | MB_ICONERROR);
        return false;
    }

    std::vector<wchar_t> entryPointWide(SAL_MAX_PATH);
#ifdef UNICODE
    if (StringCchCopyW(
            &entryPointWide[0], entryPointWide.size(), m_szFileName) != S_OK)
        return false;
#else
    if (MultiByteToWideChar(
            CP_ACP, 0, m_szFileName, -1,
            &entryPointWide[0], static_cast<int>(entryPointWide.size())) == 0)
    {
        return false;
    }
#endif

    CCompatibilityExecutionContext compatibilityContext = {this, &info};
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.ExtensionId = m_szSalamatrixExtensionId;
    request.CommandId = info.SalamatrixCommandId[0] != '\0'
                            ? info.SalamatrixCommandId
                            : m_szSalamatrixCommandId;
    request.CommandHandler = FindRuntimeCommandHandler(request.CommandId);
    request.EntryPoint = &entryPointWide[0];
    request.ParentWindow = SalamanderGeneral->GetMsgBoxParent();
    request.Flags = Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap |
                    Salamatrix::Runtime::RuntimeExecutionFlagOneShotWorker;
    request.HostDispatch = CScriptInfo::RuntimeHostDispatch;
    request.HostDispatchContext = this;
    request.CompatibilityExecute = ExecuteCompatibilityRuntime;
    request.CompatibilityContext = &compatibilityContext;

    // Modern workers may use Salamander.UI progress dialogs. Keep the
    // operation context valid for the complete worker lifetime; persistent
    // extensions activated without an execution context receive a clear
    // "unavailable" response from RuntimeHostDispatch instead.
    m_pExecInfo = &info;

    Salamatrix::Runtime::RuntimeExecutionResult result;
    BOOL executed = FALSE;
    Salamatrix::Runtime::IRuntimeSession* session = NULL;
    if (adapter->StartPersistent(&request, &session) && session != NULL)
    {
        const ULONGLONG startedAt = GetTickCount64();
        while (session->IsAlive())
        {
            if (GetTickCount64() - startedAt >= request.TimeoutMs)
                break;
            session->Pump(250);
        }
        DWORD exitCode = 1;
        executed = session->GetExitCode(&exitCode) && exitCode == 0;
        result.ExitCode = exitCode;
        result.Status = executed
                            ? Salamatrix::Runtime::RuntimeExecutionStatusSucceeded
                            : Salamatrix::Runtime::RuntimeExecutionStatusFailed;
        result.ErrorCode = executed ? S_OK : E_FAIL;
        if (session->IsAlive())
            session->Stop();
        session->Release();
    }
    else if (m_szSalamatrixRuntimeId[0] != '\0' &&
             _strnicmp(m_szSalamatrixRuntimeId, "Automation.", 10) == 0)
    {
        request.Flags = Salamatrix::Runtime::RuntimeExecutionFlagNone;
        executed = adapter->Execute(&request, &result);
    }
    else
    {
        result.Status = Salamatrix::Runtime::RuntimeExecutionStatusFailed;
        result.ErrorCode = E_FAIL;
        StringCchCopyW(
            result.Message,
            _countof(result.Message),
            L"The runtime worker could not be started.");
    }
    if (!executed &&
        result.Status != Salamatrix::Runtime::RuntimeExecutionStatusCancelled &&
        result.Message[0] != L'\0')
    {
        TCHAR message[_countof(result.Message)];
#ifdef UNICODE
        StringCchCopyW(message, _countof(message), result.Message);
#else
        WideCharToMultiByte(
            CP_ACP, 0, result.Message, -1, message, _countof(message), NULL, NULL);
        message[_countof(message) - 1] = '\0';
#endif
        SalamanderGeneral->SalMessageBox(
            request.ParentWindow,
            message,
            SalamanderGeneral->LoadStr(g_hLangInst, IDS_PLUGINNAME),
            MB_OK | MB_ICONERROR);
    }
    ReleaseRuntimeProgress();
    m_pExecInfo = NULL;
    return executed != FALSE &&
           result.Status == Salamatrix::Runtime::RuntimeExecutionStatusSucceeded;
}

static std::string JsonEscapeRuntimeText(const char* value)
{
    std::string escaped;
    if (value == NULL)
        return escaped;
    for (const unsigned char* character =
             reinterpret_cast<const unsigned char*>(value);
         *character != 0;
         ++character)
    {
        switch (*character)
        {
        case '"':
            escaped.append("\\\"");
            break;
        case '\\':
            escaped.append("\\\\");
            break;
        case '\b':
            escaped.append("\\b");
            break;
        case '\f':
            escaped.append("\\f");
            break;
        case '\n':
            escaped.append("\\n");
            break;
        case '\r':
            escaped.append("\\r");
            break;
        case '\t':
            escaped.append("\\t");
            break;
        default:
            if (*character < 0x20)
                escaped.push_back(' ');
            else
                escaped.push_back(static_cast<char>(*character));
            break;
        }
    }
    return escaped;
}

static BOOL CopyRuntimeHostResult(
    const std::string& value,
    char* output,
    DWORD outputCapacity,
    DWORD* outputLength)
{
    if (output == NULL || outputLength == NULL ||
        value.size() + 1 > outputCapacity)
        return FALSE;
    memcpy(output, value.c_str(), value.size());
    output[value.size()] = '\0';
    *outputLength = static_cast<DWORD>(value.size());
    return TRUE;
}

static const char* RuntimeEventName(
    Salamatrix::Events::EventKind kind)
{
    switch (kind)
    {
    case Salamatrix::Events::EventKindHostStartup:
        return "hostStartup";
    case Salamatrix::Events::EventKindHostShutdown:
        return "hostShutdown";
    case Salamatrix::Events::EventKindSettingsChanged:
        return "settingsChanged";
    case Salamatrix::Events::EventKindConfigurationChanged:
        return "configurationChanged";
    case Salamatrix::Events::EventKindColorsChanged:
        return "colorsChanged";
    case Salamatrix::Events::EventKindPanelsSwapped:
        return "panelsSwapped";
    case Salamatrix::Events::EventKindActivePanelChanged:
        return "activePanelChanged";
    case Salamatrix::Events::EventKindSidePathChanged:
        return "sidePathChanged";
    case Salamatrix::Events::EventKindSideSelectionChanged:
        return "sideSelectionChanged";
    case Salamatrix::Events::EventKindSideTabChanged:
        return "sideTabChanged";
    case Salamatrix::Events::EventKindSideRefreshed:
        return "sideRefreshed";
    case Salamatrix::Events::EventKindPathChanged:
        return "pathChanged";
    case Salamatrix::Events::EventKindSelectionChanged:
        return "selectionChanged";
    case Salamatrix::Events::EventKindTabChanged:
        return "tabChanged";
    case Salamatrix::Events::EventKindFileChanged:
        return "fileChanged";
    default:
        return NULL;
    }
}

static BOOL RuntimeEventKindFromName(
    const std::string& name,
    Salamatrix::Events::EventKind* kind)
{
    if (kind == NULL)
        return FALSE;
    for (int value = Salamatrix::Events::EventKindHostStartup;
         value <= Salamatrix::Events::EventKindFileChanged;
         ++value)
    {
        Salamatrix::Events::EventKind candidate =
            static_cast<Salamatrix::Events::EventKind>(value);
        const char* candidateName = RuntimeEventName(candidate);
        if (candidateName != NULL && _stricmp(name.c_str(), candidateName) == 0)
        {
            *kind = candidate;
            return TRUE;
        }
    }
    return FALSE;
}

static Salamatrix::Sides::SideReference RuntimeSideFromName(
    const std::string& name)
{
    if (_stricmp(name.c_str(), "left") == 0)
        return Salamatrix::Sides::SideReferenceLeft;
    if (_stricmp(name.c_str(), "right") == 0)
        return Salamatrix::Sides::SideReferenceRight;
    if (_stricmp(name.c_str(), "target") == 0)
        return Salamatrix::Sides::SideReferenceTarget;
    return Salamatrix::Sides::SideReferenceSource;
}

static std::string RuntimeItemInfoJson(
    const Salamatrix::Sides::ItemInfo& item)
{
    std::string path(item.Path);
    if (path.size() > 2048)
        path.resize(2048);
    ULARGE_INTEGER lastWrite;
    lastWrite.LowPart = item.LastWriteUtc.dwLowDateTime;
    lastWrite.HighPart = item.LastWriteUtc.dwHighDateTime;
    return std::string("{\"name\":\"") +
           JsonEscapeRuntimeText(item.Name) +
           "\",\"path\":\"" + JsonEscapeRuntimeText(path.c_str()) +
           "\",\"size\":\"" +
           std::to_string(static_cast<unsigned long long>(item.Size.Value)) +
           "\",\"attributes\":" + std::to_string(item.Attributes) +
           ",\"isDirectory\":" +
           (item.IsDirectory ? "true" : "false") +
           ",\"extension\":\"" +
           JsonEscapeRuntimeText(item.Extension) +
           "\",\"lastWriteUtc\":\"" +
           std::to_string(static_cast<unsigned long long>(lastWrite.QuadPart)) +
           "\",\"sizeValid\":" +
           (item.SizeValid ? "true" : "false") +
           ",\"hidden\":" + (item.Hidden ? "true" : "false") +
           ",\"link\":" + (item.IsLink ? "true" : "false") +
           ",\"offline\":" + (item.IsOffline ? "true" : "false") + "}";
}

BOOL CScriptInfo::ConfigureGeneratedRuntime(
    const char* runtimeId,
    const char* extensionId)
{
    if (runtimeId == NULL || runtimeId[0] == '\0' ||
        extensionId == NULL || extensionId[0] == '\0')
        return FALSE;
    if (StringCchCopyA(m_szSalamatrixRuntimeId,
                       _countof(m_szSalamatrixRuntimeId), runtimeId) != S_OK ||
        StringCchCopyA(m_szSalamatrixExtensionId,
                       _countof(m_szSalamatrixExtensionId), extensionId) != S_OK)
        return FALSE;
    m_dwSalamatrixMinimumRuntimeVersion = 0;
    m_salamatrixCapabilities.clear();
    m_salamatrixCapabilities.push_back("*");
    m_bSalamatrixEventsDeclared = false;
    m_salamatrixEvents.clear();
    m_szSalamatrixCommandId[0] = _T('\0');
    m_szRuntimeCommandId[0] = '\0';
    m_bShowInPluginMenu = false;
    m_bShowInContextMenu = false;
    return TRUE;
}

static BOOL FindRuntimeQuadWord(
    const char* jsonText,
    const char* member,
    CQuadWord* value)
{
    if (value == NULL)
        return FALSE;
    std::string raw;
    if (!Salamatrix::Runtime::Protocol::Json::FindRawMember(
            jsonText, member, &raw) || raw.empty())
        return FALSE;
    const char* begin = raw.c_str();
    while (*begin == ' ' || *begin == '\t' || *begin == '\r' || *begin == '\n')
        ++begin;
    if (*begin == '-' || *begin == '\0')
        return FALSE;
    char* end = NULL;
    unsigned __int64 parsed = _strtoui64(begin, &end, 10);
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
        ++end;
    if (end == begin || *end != '\0')
        return FALSE;
    value->SetUI64(parsed);
    return TRUE;
}

static const char* RuntimeCapabilityForMethod(
    const std::string& method)
{
    if (method == "salamander.sides.changePath" ||
        method == "salamander.sides.refresh" ||
        method == "salamander.sides.selectItem" ||
        method == "salamander.sides.selectAll" ||
        method == "salamander.sides.focusItem")
        return "panels.write";
    if (method.compare(0, 16, "salamander.sides.") == 0)
        return "panels.read";
    if (method.compare(0, 13, "salamander.ui.") == 0 ||
        method == "salamander.clipboard.copyText")
        return "ui.dialogs";
    if (method.compare(0, 19, "salamander.commands.") == 0)
        return "commands";
    if (method.compare(0, 25, "salamander.fileOperations.") == 0)
        return "file-operations";
    if (method.compare(0, 18, "salamander.storage.") == 0)
        return "storage";
    if (method.compare(0, 17, "salamander.events.") == 0)
        return "events";
    if (method.compare(0, 13, "salamander.ai.") == 0)
        return "ai";
    return NULL;
}

BOOL WINAPI CScriptInfo::RuntimeEventCallback(
    void* context,
    const Salamatrix::Events::EventPayload* payload)
{
    CScriptInfo* script = static_cast<CScriptInfo*>(context);
    if (script == NULL || payload == NULL || script->m_pRuntimeSession == NULL)
        return FALSE;

    const char* name = RuntimeEventName(payload->Kind);
    if (name == NULL)
        return FALSE;
    char tabId[32];
    _ui64toa_s(payload->ActiveTabId, tabId, _countof(tabId), 10);
    std::string eventJson =
        std::string("{\"event\":\"") + name +
        "\",\"parameter\":" + std::to_string(payload->Parameter) +
        ",\"activePanel\":" + std::to_string(payload->ActivePanel) +
        ",\"tabId\":\"" + tabId +
        "\",\"pathType\":" + std::to_string(payload->PathType) +
        ",\"path\":\"" + JsonEscapeRuntimeText(payload->Path) + "\"}";
    std::string frame;
    if (!Salamatrix::Runtime::Protocol::LineCodec::Encode(
            Salamatrix::Runtime::Protocol::MessageEvent,
            0,
            eventJson,
            &frame))
        return FALSE;
    return script->m_pRuntimeSession->QueueFrame(
        frame.c_str(), static_cast<DWORD>(frame.size()));
}

BOOL WINAPI CScriptInfo::RuntimeDialogEventCallback(
    void* context,
    const Salamatrix::UI::DialogEvent* event)
{
    RUNTIME_DIALOG* binding = static_cast<RUNTIME_DIALOG*>(context);
    if (binding == NULL || binding->Owner == NULL || event == NULL ||
        !binding->EventsEnabled || binding->EventName[0] == '\0' ||
        binding->Owner->m_pRuntimeSession == NULL)
        return FALSE;

    char dialogId[32];
    _ui64toa_s(binding->Id, dialogId, _countof(dialogId), 10);
    std::string eventJson =
        std::string("{\"event\":\"") +
        JsonEscapeRuntimeText(binding->EventName) +
        "\",\"dialogId\":\"" + dialogId +
        "\",\"controlId\":\"" +
        JsonEscapeRuntimeText(event->ControlId) +
        "\",\"kind\":" +
        std::to_string(static_cast<int>(event->Control)) +
        ",\"text\":\"" + JsonEscapeRuntimeText(event->Text) +
        "\",\"checked\":" +
        (event->Checked ? "true" : "false") +
        ",\"selectedIndex\":" +
        std::to_string(event->SelectedIndex) + "}";
    std::string frame;
    if (!Salamatrix::Runtime::Protocol::LineCodec::Encode(
            Salamatrix::Runtime::Protocol::MessageEvent,
            0,
            eventJson,
            &frame))
        return FALSE;
    return binding->Owner->m_pRuntimeSession->QueueFrame(
        frame.c_str(), static_cast<DWORD>(frame.size()));
}

namespace
{
struct RuntimeMainThreadDispatch
{
    void* Context;
    Salamatrix::Runtime::Protocol::MessageType Type;
    ULONGLONG RequestId;
    const char* PayloadJson;
    char* ResultJson;
    DWORD ResultCapacity;
    DWORD* ResultLength;
};

static __declspec(thread) RuntimeMainThreadDispatch*
    g_runtimeHostDispatchMainCall = NULL;

BOOL WINAPI DispatchRuntimeHostOnMain(void* raw)
{
    RuntimeMainThreadDispatch* call =
        static_cast<RuntimeMainThreadDispatch*>(raw);
    if (call == NULL)
        return FALSE;
    RuntimeMainThreadDispatch* previous = g_runtimeHostDispatchMainCall;
    g_runtimeHostDispatchMainCall = call;
    BOOL result = CScriptInfo::DispatchRuntimeHostCall(
        call->Context, call->Type, call->RequestId, call->PayloadJson,
        call->ResultJson, call->ResultCapacity, call->ResultLength);
    g_runtimeHostDispatchMainCall = previous;
    return result;
}
}

BOOL WINAPI CScriptInfo::RuntimeHostDispatch(
    void* context,
    Salamatrix::Runtime::Protocol::MessageType type,
    ULONGLONG requestId,
    const char* payloadJson,
    char* resultJson,
    DWORD resultCapacity,
    DWORD* resultLength)
{
    (void)requestId;
    CScriptInfo* script = static_cast<CScriptInfo*>(context);
    if (script == NULL || resultJson == NULL || resultLength == NULL)
        return FALSE;

    // Persistent runtime workers receive requests on their own pump thread,
    // while Salamander's panel/UI services are main-thread-only. Marshal the
    // complete dispatch synchronously; the callback marker lets the main-thread
    // invocation enter the real implementation without recursing back into
    // SendMessage (TLS is per thread, so the marker is set by the callback).
    if (g_runtimeHostDispatchMainCall == NULL && SalamanderGeneral != NULL)
    {
        RuntimeMainThreadDispatch call = {
            context,
            type,
            requestId,
            payloadJson,
            resultJson,
            resultCapacity,
            resultLength};
        BOOL dispatched = SalamanderGeneral->InvokeOnMainThread(
            DispatchRuntimeHostOnMain, &call, 120000);
        return dispatched;
    }

    *resultLength = 0;

    CAutomationSalamatrixBridge* bridge =
        g_oAutomationPlugin.GetSalamatrixBridge();
    if (!bridge->WasQueried())
        g_oAutomationPlugin.RefreshSalamatrixServices();

    class CSalamatrixServiceLeaseGuard
    {
    private:
        CSalamanderGeneralAbstract* m_pGeneral;
        const char* m_pServiceId;
        void* m_pInterface;
        void* m_pConsumer;
        BOOL m_bAcquired;

    public:
        CSalamatrixServiceLeaseGuard(
            CSalamanderGeneralAbstract* general,
            const char* serviceId,
            void* serviceInterface,
            void* consumer)
            : m_pGeneral(general),
              m_pServiceId(serviceId),
              m_pInterface(serviceInterface),
              m_pConsumer(consumer),
              m_bAcquired(FALSE)
        {
            if (m_pGeneral != NULL && m_pServiceId != NULL &&
                m_pInterface != NULL && m_pConsumer != NULL)
                m_bAcquired = m_pGeneral->AcquireService(
                    m_pServiceId, m_pInterface, m_pConsumer);
        }

        ~CSalamatrixServiceLeaseGuard()
        {
            if (m_bAcquired)
                m_pGeneral->ReleaseService(
                    m_pServiceId, m_pInterface, m_pConsumer);
        }

        BOOL IsBlocked() const
        {
            return m_pInterface != NULL && !m_bAcquired;
        }
    };

    // The extension catalog itself is a shared provider service. Acquire its
    // lease before asking it for the extension callback lease below.
    CSalamatrixServiceLeaseGuard extensionServiceLease(
        SalamanderGeneral,
        SALAMATRIX_SERVICE_EXTENSIONS,
        bridge->GetExtensionsService(),
        script);
    if (extensionServiceLease.IsBlocked())
        return CopyRuntimeHostResult(
            "{\"ok\":false,\"error\":\"service is unloading\"}",
            resultJson, resultCapacity, resultLength);

    class CExtensionLeaseGuard
    {
    private:
        Salamatrix::Extensions::IExtensionsService* m_pService;
        const char* m_pExtensionId;
        void* m_pOwner;
        BOOL m_bRegistered;
        BOOL m_bAcquired;

    public:
        CExtensionLeaseGuard(
            Salamatrix::Extensions::IExtensionsService* service,
            const char* extensionId,
            void* owner)
            : m_pService(service),
              m_pExtensionId(extensionId),
              m_pOwner(owner),
              m_bRegistered(FALSE),
              m_bAcquired(FALSE)
        {
            if (m_pService != NULL && m_pExtensionId != NULL &&
                m_pExtensionId[0] != '\0')
            {
                Salamatrix::Extensions::ExtensionInfo info;
                m_bRegistered = m_pService->FindExtension(
                    m_pExtensionId, &info);
                if (m_bRegistered)
                    m_bAcquired = m_pService->AcquireExtension(
                        m_pExtensionId, m_pOwner);
            }
        }

        ~CExtensionLeaseGuard()
        {
            if (m_bAcquired)
                m_pService->ReleaseExtension(m_pExtensionId, m_pOwner);
        }

        BOOL IsBlocked() const
        {
            return m_bRegistered && !m_bAcquired;
        }
    };

    CExtensionLeaseGuard lease(
        bridge->GetExtensionsService(),
        script->m_szSalamatrixExtensionId,
        script);
    if (lease.IsBlocked())
        return CopyRuntimeHostResult(
            "{\"ok\":false,\"error\":\"extension is unloading\"}",
            resultJson, resultCapacity, resultLength);

    if (type == Salamatrix::Runtime::Protocol::MessageHello)
    {
        std::string response =
            "{\"ok\":true,\"protocol\":1,\"extensionId\":\"" +
            JsonEscapeRuntimeText(script->m_szSalamatrixExtensionId) +
            "\",\"services\":[\"commands\",\"fileOperations\",\"sides\",\"storage\",\"ui\",\"ai\",\"runtimes\"]}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (type != Salamatrix::Runtime::Protocol::MessageCall ||
        payloadJson == NULL)
        return FALSE;

    std::string method;
    if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "method", &method))
        return FALSE;

    const char* requiredCapability = RuntimeCapabilityForMethod(method);
    if (requiredCapability != NULL &&
        script->HasDeclaredSalamatrixCapabilities() &&
        !script->HasSalamatrixCapability(requiredCapability))
    {
        std::string response =
            std::string("{\"ok\":false,\"error\":\"capability denied\",\"capability\":\"") +
            JsonEscapeRuntimeText(requiredCapability) + "\"}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    const char* leasedServiceId = NULL;
    void* leasedService = NULL;
    if (method == "salamander.clipboard.copyText" ||
        method.find("salamander.ui.") == 0)
    {
        leasedServiceId = SALAMATRIX_SERVICE_UI;
        leasedService = bridge->GetUIService();
    }
    else if (method.find("salamander.commands.") == 0)
    {
        leasedServiceId = SALAMATRIX_SERVICE_COMMANDS;
        leasedService = bridge->GetCommandService();
    }
    else if (method.find("salamander.fileOperations.") == 0)
    {
        leasedServiceId = SALAMATRIX_SERVICE_FILEOPERATIONS;
        leasedService = bridge->GetFileOperationsService();
    }
    else if (method.find("salamander.sides.") == 0)
    {
        leasedServiceId = SALAMATRIX_SERVICE_SIDES;
        leasedService = bridge->GetSidesService();
    }
    else if (method.find("salamander.events.") == 0)
    {
        leasedServiceId = SALAMATRIX_SERVICE_EVENTS;
        leasedService = bridge->GetEventsService();
    }
    else if (method.find("salamander.storage.") == 0)
    {
        leasedServiceId = SALAMATRIX_SERVICE_STORAGE;
        leasedService = bridge->GetStorageService();
    }
    else if (method.find("salamander.ai.") == 0)
    {
        leasedServiceId = SALAMATRIX_SERVICE_AI;
        leasedService = bridge->GetAssistantService();
    }
    else if (method == "salamander.runtimes.list")
    {
        leasedServiceId = SALAMATRIX_SERVICE_RUNTIME;
        leasedService = bridge->GetRuntimeService();
    }
    CSalamatrixServiceLeaseGuard serviceLease(
        SalamanderGeneral, leasedServiceId, leasedService, script);
    if (serviceLease.IsBlocked())
        return CopyRuntimeHostResult(
            "{\"ok\":false,\"error\":\"service is unloading\"}",
            resultJson, resultCapacity, resultLength);

    if (method == "salamander.runtimes.list")
    {
        Salamatrix::Runtime::IRuntimeService* runtimes =
            bridge->GetRuntimeService();
        if (runtimes == NULL)
            return FALSE;
        std::string response = "{\"ok\":true,\"runtimes\":[";
        for (int index = 0; index < runtimes->GetAdapterCount(); ++index)
        {
            Salamatrix::Runtime::IRuntimeAdapter* adapter =
                runtimes->GetAdapter(index);
            const Salamatrix::Runtime::RuntimeAdapterDescriptor* descriptor =
                adapter != NULL ? adapter->GetDescriptor() : NULL;
            if (descriptor == NULL || descriptor->RuntimeId == NULL)
                continue;
            if (response[response.size() - 1] != '[')
                response += ",";
            response +=
                std::string("{\"id\":\"") +
                JsonEscapeRuntimeText(descriptor->RuntimeId) +
                "\",\"name\":\"" +
                JsonEscapeRuntimeText(descriptor->DisplayName != NULL
                                          ? descriptor->DisplayName
                                          : "") +
                "\",\"language\":\"" +
                JsonEscapeRuntimeText(descriptor->LanguageId != NULL
                                          ? descriptor->LanguageId
                                          : "") +
                "\",\"extensions\":\"" +
                JsonEscapeRuntimeText(descriptor->FileExtensions != NULL
                                          ? descriptor->FileExtensions
                                          : "") +
                "\",\"version\":" +
                std::to_string(descriptor->RuntimeVersion) +
                ",\"available\":" +
                (adapter->IsAvailable() ? "true}" : "false}");
        }
        response += "]}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "runtime.ready")
    {
        return CopyRuntimeHostResult(
            "{\"ok\":true,\"ready\":true}",
            resultJson,
            resultCapacity,
            resultLength);
    }

    if (method == "salamander.clipboard.copyText")
    {
        std::string text;
        BOOL showEcho = FALSE;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "text", &text))
            return FALSE;
        Salamatrix::Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "showEcho", &showEcho);
        Salamatrix::UI::IUIService* ui = bridge->GetUIService();
        if (ui == NULL || !ui->CopyTextToClipboard(
                              text.c_str(),
                              showEcho,
                              SalamanderGeneral->GetMsgBoxParent()))
            return FALSE;
        return CopyRuntimeHostResult(
            "{\"ok\":true,\"copied\":true}",
            resultJson,
            resultCapacity,
            resultLength);
    }

    if (method == "salamander.ui.messageBox")
    {
        std::string message;
        std::string title;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "message", &message))
            return FALSE;
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "title", &title);
        if (title.empty())
            title = "Salamatrix";
        Salamatrix::UI::IUIService* ui = bridge->GetUIService();
        int result;
        if (ui != NULL)
        {
            result = ui->ShowMessageBox(
                SalamanderGeneral->GetMsgBoxParent(),
                message.c_str(),
                title.c_str(),
                MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            std::wstring messageWide;
            std::wstring titleWide;
            if (!Utf8ToWideText(message, messageWide) ||
                !Utf8ToWideText(title, titleWide))
                return FALSE;
            result = MessageBoxW(
                SalamanderGeneral->GetMsgBoxParent(),
                messageWide.c_str(),
                titleWide.c_str(),
                MB_OK | MB_ICONINFORMATION);
        }
        std::string response =
            "{\"ok\":true,\"result\":" + std::to_string(result) + "}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.ui.notify")
    {
        std::string title;
        std::string message;
        int timeoutValue = 5000;
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "title", &title);
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "message", &message);
        Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
            payloadJson, "timeoutMs", &timeoutValue);
        if (title.empty())
            title = "Salamander";
        if (timeoutValue < 0)
            timeoutValue = 0;
        Salamatrix::UI::IUIService* ui = bridge->GetUIService();
        BOOL shown = ui != NULL &&
                     ui->GetVersion() >= SALAMATRIX_UI_VERSION_1_1 &&
                     ui->ShowNotification(
                         SalamanderGeneral->GetMainWindowHWND(),
                         title.c_str(),
                         message.c_str(),
                         static_cast<DWORD>(timeoutValue));
        return CopyRuntimeHostResult(
            std::string("{\"ok\":true,\"shown\":") +
                (shown ? "true}" : "false}"),
            resultJson,
            resultCapacity,
            resultLength);
    }

    if (method == "salamander.ui.inputBox")
    {
        std::string prompt;
        std::string title;
        std::string initial;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "prompt", &prompt))
            return FALSE;
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "title", &title);
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "initial", &initial);
        if (title.empty())
            title = "Salamatrix";
        char value[4096];
        value[0] = '\0';
        BOOL accepted = FALSE;
        Salamatrix::UI::IUIService* ui = bridge->GetUIService();
        if (ui != NULL)
        {
            Salamatrix::UI::DialogOptions options;
            options.Title = title.c_str();
            options.Parent = SalamanderGeneral->GetMsgBoxParent();
            Salamatrix::UI::IDialog* dialog = ui->CreateSalamatrixDialog(options);
            if (dialog != NULL)
            {
                Salamatrix::UI::ControlOptions promptOptions;
                promptOptions.Id = "prompt";
                promptOptions.Text = prompt.c_str();
                dialog->AddControl(
                    Salamatrix::UI::ControlKindLabel, promptOptions);
                Salamatrix::UI::ControlOptions valueOptions;
                valueOptions.Id = "value";
                valueOptions.Text = initial.c_str();
                Salamatrix::UI::IControl* valueControl = dialog->AddControl(
                    Salamatrix::UI::ControlKindTextBox, valueOptions);
                Salamatrix::UI::ControlOptions okOptions;
                okOptions.Id = "ok";
                okOptions.Text = "OK";
                okOptions.DialogResult = IDOK;
                dialog->AddControl(
                    Salamatrix::UI::ControlKindButton, okOptions);
                Salamatrix::UI::ControlOptions cancelOptions;
                cancelOptions.Id = "cancel";
                cancelOptions.Text = "Cancel";
                cancelOptions.DialogResult = IDCANCEL;
                dialog->AddControl(
                    Salamatrix::UI::ControlKindButton, cancelOptions);
                accepted = dialog->ShowModal() == IDOK;
                if (accepted && valueControl != NULL)
                    valueControl->GetText(value, _countof(value));
                ui->DestroyDialog(dialog);
            }
        }
        else
        {
            accepted = ShowRuntimeInputBox(
                SalamanderGeneral->GetMsgBoxParent(),
                title,
                prompt,
                initial,
                value,
                _countof(value));
        }
        std::string response =
            std::string("{\"ok\":true,\"accepted\":") +
            (accepted ? "true" : "false") +
            ",\"value\":\"" + JsonEscapeRuntimeText(value) + "\"}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.ui.pickFile")
    {
        BOOL save = FALSE;
        std::string title;
        std::string filter;
        std::string initialPath;
        Salamatrix::Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "save", &save);
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "title", &title);
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "filter", &filter);
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "initial", &initialPath);
        if (title.empty())
            title = save ? "Save file" : "Open file";

        std::vector<char> selectedPath(SAL_MAX_PATH * 3);
        Salamatrix::UI::IUIService* ui = bridge->GetUIService();
        if (ui == NULL)
            return FALSE;
        BOOL selected = ui->PickFile(
            SalamanderGeneral->GetMsgBoxParent(),
            save,
            title.c_str(),
            filter.c_str(),
            initialPath.c_str(),
            &selectedPath[0],
            static_cast<DWORD>(selectedPath.size()));
        std::string response =
            std::string("{\"ok\":true,\"selected\":") +
            (selected ? "true" : "false") +
            ",\"path\":\"" +
            JsonEscapeRuntimeText(selected ? &selectedPath[0] : "") +
            "\"}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.ui.pickFolder")
    {
        std::string title;
        std::string initialPath;
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "title", &title);
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "initial", &initialPath);
        if (title.empty())
            title = "Select folder";

        std::vector<char> selectedPath(SAL_MAX_PATH * 3);
        Salamatrix::UI::IUIService* ui = bridge->GetUIService();
        if (ui == NULL)
            return FALSE;
        BOOL selected = ui->PickFolder(
            SalamanderGeneral->GetMsgBoxParent(),
            title.c_str(),
            initialPath.c_str(),
            &selectedPath[0],
            static_cast<DWORD>(selectedPath.size()));
        std::string response =
            std::string("{\"ok\":true,\"selected\":") +
            (selected ? "true" : "false") +
            ",\"path\":\"" +
            JsonEscapeRuntimeText(selected ? &selectedPath[0] : "") +
            "\"}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
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
        Salamatrix::UI::IUIService* ui = bridge->GetUIService();
        if (ui == NULL)
            return FALSE;

        if (method == "salamander.ui.progress.create")
        {
            if (script->m_pExecInfo == NULL ||
                script->m_pExecInfo->pOperation == NULL)
            {
                return CopyRuntimeHostResult(
                    "{\"ok\":false,\"error\":\"progress requires an execution operation context\"}",
                    resultJson, resultCapacity, resultLength);
            }
            script->ReleaseRuntimeProgress();
            std::string title;
            CQuadWord total;
            CQuadWord total2;
            BOOL hasTotal = FALSE;
            BOOL hasTotal2 = FALSE;
            BOOL twoProgressBars = FALSE;
            BOOL fileProgress = FALSE;
            BOOL cancelEnabled = TRUE;
            Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "title", &title);
            hasTotal = FindRuntimeQuadWord(payloadJson, "total", &total);
            hasTotal2 = FindRuntimeQuadWord(payloadJson, "total2", &total2);
            Salamatrix::Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "twoProgressBars", &twoProgressBars);
            Salamatrix::Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "fileProgress", &fileProgress);
            Salamatrix::Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "cancelEnabled", &cancelEnabled);
            Salamatrix::UI::ProgressDialogOptions options;
            options.Title = title.empty() ? "Salamatrix" : title.c_str();
            options.Parent = SalamanderGeneral->GetMsgBoxParent();
            options.TwoProgressBars = twoProgressBars;
            options.FileProgress = fileProgress;
            options.CancelEnabled = cancelEnabled;
            Salamatrix::UI::IProgressDialog* progress =
                ui->CreateProgressDialog(script->m_pExecInfo->pOperation);
            if (progress == NULL)
                return CopyRuntimeHostResult(
                    "{\"ok\":false,\"error\":\"progress dialog unavailable\"}",
                    resultJson, resultCapacity, resultLength);
            progress->Open(options);
            if (hasTotal && hasTotal2)
                progress->SetTotals(total, total2);
            else if (hasTotal)
                progress->SetTotal(total);
            ULONGLONG id = script->m_nextRuntimeProgressId++;
            if (id == 0)
                id = script->m_nextRuntimeProgressId++;
            script->m_runtimeProgress.Owner = script;
            script->m_runtimeProgress.Id = id;
            script->m_runtimeProgress.Dialog = progress;
            char idText[32];
            _ui64toa_s(id, idText, _countof(idText), 10);
            std::string response =
                std::string("{\"ok\":true,\"progressId\":\"") +
                idText + "\"}";
            return CopyRuntimeHostResult(
                response, resultJson, resultCapacity, resultLength);
        }

        std::string idText;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "progressId", &idText))
            return FALSE;
        char* idEnd = NULL;
        ULONGLONG progressId = _strtoui64(idText.c_str(), &idEnd, 10);
        if (idEnd == idText.c_str() || *idEnd != '\0' ||
            script->m_runtimeProgress.Dialog == NULL ||
            script->m_runtimeProgress.Id != progressId)
            return FALSE;
        Salamatrix::UI::IProgressDialog* progress =
            script->m_runtimeProgress.Dialog;
        if (method == "salamander.ui.progress.close")
        {
            script->ReleaseRuntimeProgress();
            return CopyRuntimeHostResult(
                "{\"ok\":true,\"closed\":true}",
                resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.progress.cancelled")
        {
            std::string response =
                std::string("{\"ok\":true,\"cancelled\":") +
                (progress->IsCancelled() ? "true}" : "false}");
            return CopyRuntimeHostResult(
                response, resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.progress.setTitle")
        {
            std::string title;
            if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "title", &title))
                return FALSE;
            progress->SetTitle(title.c_str());
            return CopyRuntimeHostResult(
                "{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.progress.setCancelEnabled")
        {
            BOOL enabled = TRUE;
            Salamatrix::Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "enabled", &enabled);
            progress->SetCancelEnabled(enabled);
            return CopyRuntimeHostResult(
                "{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.progress.setTotals")
        {
            CQuadWord firstTotal;
            CQuadWord secondTotal;
            if (!FindRuntimeQuadWord(payloadJson, "total", &firstTotal) ||
                !FindRuntimeQuadWord(payloadJson, "total2", &secondTotal))
                return FALSE;
            progress->SetTotals(firstTotal, secondTotal);
            return CopyRuntimeHostResult(
                "{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.ui.progress.setPositions")
        {
            CQuadWord firstPosition;
            CQuadWord secondPosition;
            if (!FindRuntimeQuadWord(payloadJson, "position", &firstPosition) ||
                !FindRuntimeQuadWord(payloadJson, "position2", &secondPosition))
                return FALSE;
            BOOL delayedPaint = TRUE;
            Salamatrix::Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "delayedPaint", &delayedPaint);
            BOOL continued = progress->SetPositions(
                firstPosition, secondPosition, delayedPaint);
            std::string response =
                std::string("{\"ok\":true,\"continued\":") +
                (continued && !progress->IsCancelled() ? "true}" : "false}");
            return CopyRuntimeHostResult(
                response, resultJson, resultCapacity, resultLength);
        }
        BOOL delayedPaint = TRUE;
        Salamatrix::Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "delayedPaint", &delayedPaint);
        BOOL continued = TRUE;
        if (method == "salamander.ui.progress.step")
        {
            int amount = 1;
            Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
                payloadJson, "amount", &amount);
            continued = progress->Step(amount, delayedPaint);
        }
        else
        {
            CQuadWord position;
            CQuadWord total;
            CQuadWord position2;
            CQuadWord total2;
            std::string text;
            if (!FindRuntimeQuadWord(payloadJson, "position", &position))
                return FALSE;
            BOOL hasTotal = FindRuntimeQuadWord(payloadJson, "total", &total);
            BOOL hasPosition2 = FindRuntimeQuadWord(
                payloadJson, "position2", &position2);
            BOOL hasTotal2 = FindRuntimeQuadWord(
                payloadJson, "total2", &total2);
            Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "text", &text);
            if (hasTotal && hasTotal2)
                progress->SetTotals(total, total2);
            else if (hasTotal)
                progress->SetTotal(total);
            if (!text.empty())
                progress->AddText(text.c_str(), delayedPaint);
            continued = hasPosition2
                            ? progress->SetPositions(
                                  position, position2, delayedPaint)
                            : progress->SetPosition(position, delayedPaint);
        }
        std::string response =
            std::string("{\"ok\":true,\"continued\":") +
            (continued && !progress->IsCancelled() ? "true}" : "false}");
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.ui.dialog.create" ||
        method == "salamander.ui.dialog.add" ||
        method == "salamander.ui.dialog.item" ||
        method == "salamander.ui.dialog.column" ||
        method == "salamander.ui.dialog.selection" ||
        method == "salamander.ui.dialog.clearItems" ||
        method == "salamander.ui.dialog.validation" ||
        method == "salamander.ui.dialog.events" ||
        method == "salamander.ui.dialog.show" ||
        method == "salamander.ui.dialog.get" ||
        method == "salamander.ui.dialog.set" ||
        method == "salamander.ui.dialog.close" ||
        method == "salamander.ui.dialog.destroy")
    {
        Salamatrix::UI::IUIService* ui = bridge->GetUIService();
        if (ui == NULL)
            return FALSE;

        if (method == "salamander.ui.dialog.create")
        {
            if (script->m_nRuntimeDialogs >=
                static_cast<int>(_countof(script->m_runtimeDialogs)))
                return FALSE;
            std::string title;
            int width = 320;
            int height = 180;
            Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "title", &title);
            if (title.empty())
                title = "Salamatrix";
            Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
                payloadJson, "width", &width);
            Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
                payloadJson, "height", &height);
            Salamatrix::UI::DialogOptions options;
            options.Title = title.c_str();
            options.Parent = SalamanderGeneral->GetMsgBoxParent();
            options.Width = static_cast<short>(width < 160 ? 160 : (width > 1200 ? 1200 : width));
            options.Height = static_cast<short>(height < 100 ? 100 : (height > 900 ? 900 : height));
            Salamatrix::UI::IDialog* dialog = ui->CreateSalamatrixDialog(options);
            if (dialog == NULL)
                return FALSE;
            ULONGLONG id = script->m_nextRuntimeDialogId++;
            if (id == 0)
                id = script->m_nextRuntimeDialogId++;
            CScriptInfo::RUNTIME_DIALOG& runtimeDialog =
                script->m_runtimeDialogs[script->m_nRuntimeDialogs];
            runtimeDialog.Owner = script;
            runtimeDialog.Id = id;
            runtimeDialog.Dialog = dialog;
            runtimeDialog.EventsEnabled = FALSE;
            runtimeDialog.EventName[0] = '\0';
            ++script->m_nRuntimeDialogs;
            char idText[32];
            _ui64toa_s(id, idText, _countof(idText), 10);
            std::string response =
                std::string("{\"ok\":true,\"dialogId\":\"") + idText + "\"}";
            return CopyRuntimeHostResult(
                response, resultJson, resultCapacity, resultLength);
        }

        std::string idText;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "dialogId", &idText))
            return FALSE;
        char* end = NULL;
        ULONGLONG dialogId = _strtoui64(idText.c_str(), &end, 10);
        if (end == idText.c_str() || *end != '\0')
            return FALSE;
        int dialogIndex = -1;
        for (int index = 0; index < script->m_nRuntimeDialogs; ++index)
        {
            if (script->m_runtimeDialogs[index].Id == dialogId)
            {
                dialogIndex = index;
                break;
            }
        }
        if (dialogIndex < 0 || script->m_runtimeDialogs[dialogIndex].Dialog == NULL)
            return FALSE;
        Salamatrix::UI::IDialog* dialog =
            script->m_runtimeDialogs[dialogIndex].Dialog;

        if (method == "salamander.ui.dialog.add")
        {
            std::string kindName;
            std::string controlId;
            std::string text;
            BOOL readOnly = FALSE;
            BOOL checked = FALSE;
            BOOL keepOpen = FALSE;
            BOOL multiline = FALSE;
            int dialogResult = 0;
            int x = 0;
            int y = 0;
            int width = 0;
            int height = 0;
            BOOL hasBounds = FALSE;
            if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "kind", &kindName) ||
                !Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "controlId", &controlId))
                return FALSE;
            Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "text", &text);
            Salamatrix::Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "readOnly", &readOnly);
            Salamatrix::Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "checked", &checked);
            Salamatrix::Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "keepOpen", &keepOpen);
            Salamatrix::Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "multiline", &multiline);
            std::string rawCoordinate;
            if (Salamatrix::Runtime::Protocol::Json::FindRawMember(
                    payloadJson, "x", &rawCoordinate))
            {
                if (!Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
                        payloadJson, "x", &x))
                    return FALSE;
                hasBounds = TRUE;
            }
            if (Salamatrix::Runtime::Protocol::Json::FindRawMember(
                    payloadJson, "y", &rawCoordinate))
            {
                if (!Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
                        payloadJson, "y", &y))
                    return FALSE;
                hasBounds = TRUE;
            }
            if (Salamatrix::Runtime::Protocol::Json::FindRawMember(
                    payloadJson, "width", &rawCoordinate))
            {
                if (!Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
                        payloadJson, "width", &width))
                    return FALSE;
                hasBounds = TRUE;
            }
            if (Salamatrix::Runtime::Protocol::Json::FindRawMember(
                    payloadJson, "height", &rawCoordinate))
            {
                if (!Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
                        payloadJson, "height", &height))
                    return FALSE;
                hasBounds = TRUE;
            }
            std::string rawResult;
            if (Salamatrix::Runtime::Protocol::Json::FindRawMember(
                    payloadJson, "dialogResult", &rawResult))
            {
                char* resultEnd = NULL;
                unsigned long parsed = strtoul(rawResult.c_str(), &resultEnd, 10);
                if (resultEnd != rawResult.c_str() && *resultEnd == '\0')
                    dialogResult = static_cast<int>(parsed);
            }
            Salamatrix::UI::ControlKind kind;
            if (_stricmp(kindName.c_str(), "label") == 0)
                kind = Salamatrix::UI::ControlKindLabel;
            else if (_stricmp(kindName.c_str(), "textbox") == 0)
                kind = Salamatrix::UI::ControlKindTextBox;
            else if (_stricmp(kindName.c_str(), "checkbox") == 0)
                kind = Salamatrix::UI::ControlKindCheckBox;
            else if (_stricmp(kindName.c_str(), "radio") == 0)
                kind = Salamatrix::UI::ControlKindRadioButton;
            else if (_stricmp(kindName.c_str(), "combobox") == 0)
                kind = Salamatrix::UI::ControlKindComboBox;
            else if (_stricmp(kindName.c_str(), "button") == 0)
                kind = Salamatrix::UI::ControlKindButton;
            else if (_stricmp(kindName.c_str(), "listview") == 0)
                kind = Salamatrix::UI::ControlKindListView;
            else if (_stricmp(kindName.c_str(), "treeview") == 0)
                kind = Salamatrix::UI::ControlKindTreeView;
            else if (_stricmp(kindName.c_str(), "tabcontrol") == 0)
                kind = Salamatrix::UI::ControlKindTabControl;
            else if (_stricmp(kindName.c_str(), "folderpicker") == 0)
                kind = Salamatrix::UI::ControlKindFolderPicker;
            else
                return FALSE;
            Salamatrix::UI::ControlOptions options;
            options.Id = controlId.c_str();
            options.Text = text.c_str();
            options.ReadOnly = readOnly;
            options.Checked = checked;
            options.DialogResult = dialogResult;
            options.KeepOpen = keepOpen;
            options.Multiline = multiline;
            Salamatrix::UI::ControlLayout layout;
            layout.HasBounds = hasBounds;
            layout.X = x;
            layout.Y = y;
            layout.Width = width;
            layout.Height = height;
            Salamatrix::UI::IControl* control = dialog->AddControlEx(
                kind, options, layout);
            if (control == NULL)
                return FALSE;
            return CopyRuntimeHostResult(
                "{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }

        if (method == "salamander.ui.dialog.validation")
        {
            std::string controlId;
            std::string message;
            BOOL required = FALSE;
            if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "controlId", &controlId))
                return FALSE;
            Salamatrix::Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "required", &required);
            Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "message", &message);
            Salamatrix::UI::IControl* control =
                dialog->FindControl(controlId.c_str());
            if (control == NULL || !control->SetRequired(required) ||
                !control->SetValidationMessage(message.c_str()))
                return FALSE;
            return CopyRuntimeHostResult(
                "{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }

        if (method == "salamander.ui.dialog.events")
        {
            BOOL enabled = FALSE;
            std::string eventName;
            Salamatrix::Runtime::Protocol::Json::FindBoolMember(
                payloadJson, "enabled", &enabled);
            Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "event", &eventName);
            CScriptInfo::RUNTIME_DIALOG& binding =
                script->m_runtimeDialogs[dialogIndex];
            if (enabled)
            {
                if (eventName.empty() ||
                    eventName.size() >= _countof(binding.EventName) ||
                    StringCchCopyA(
                        binding.EventName,
                        _countof(binding.EventName),
                        eventName.c_str()) != S_OK ||
                    !dialog->SetEventCallback(
                        CScriptInfo::RuntimeDialogEventCallback,
                        &binding))
                    return FALSE;
                binding.EventsEnabled = TRUE;
            }
            else
            {
                if (!dialog->SetEventCallback(NULL, NULL))
                    return FALSE;
                binding.EventsEnabled = FALSE;
                binding.EventName[0] = '\0';
            }
            return CopyRuntimeHostResult(
                std::string("{\"ok\":true,\"enabled\":") +
                    (enabled ? "true}" : "false}"),
                resultJson,
                resultCapacity,
                resultLength);
        }

        if (method == "salamander.ui.dialog.item")
        {
            std::string controlId;
            std::string text;
            int parentIndex = -1;
            if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "controlId", &controlId) ||
                !Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "text", &text))
                return FALSE;
            std::string rawParent;
            if (Salamatrix::Runtime::Protocol::Json::FindRawMember(
                    payloadJson, "parentIndex", &rawParent))
            {
                char* parentEnd = NULL;
                long parsedParent = strtol(rawParent.c_str(), &parentEnd, 10);
                if (parentEnd != rawParent.c_str() && *parentEnd == '\0')
                    parentIndex = static_cast<int>(parsedParent);
            }
            Salamatrix::UI::IControl* control =
                dialog->FindControl(controlId.c_str());
            if (control == NULL || !control->AddItem(text.c_str(), parentIndex))
                return FALSE;
            return CopyRuntimeHostResult(
                std::string("{\"ok\":true,\"itemCount\":") +
                    std::to_string(control->GetItemCount()) + "}",
                resultJson,
                resultCapacity,
                resultLength);
        }

        if (method == "salamander.ui.dialog.column")
        {
            std::string controlId;
            std::string title;
            int width = 180;
            if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "controlId", &controlId) ||
                !Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "title", &title))
                return FALSE;
            Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
                payloadJson, "width", &width);
            Salamatrix::UI::IControl* control =
                dialog->FindControl(controlId.c_str());
            if (control == NULL || !control->AddColumn(title.c_str(), width))
                return FALSE;
            return CopyRuntimeHostResult(
                "{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }

        if (method == "salamander.ui.dialog.selection")
        {
            std::string controlId;
            int index = -1;
            if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "controlId", &controlId) ||
                !Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
                    payloadJson, "index", &index))
                return FALSE;
            Salamatrix::UI::IControl* control =
                dialog->FindControl(controlId.c_str());
            if (control == NULL || !control->SetSelectedIndex(index))
                return FALSE;
            return CopyRuntimeHostResult(
                std::string("{\"ok\":true,\"selectedIndex\":") +
                    std::to_string(control->GetSelectedIndex()) + "}",
                resultJson,
                resultCapacity,
                resultLength);
        }

        if (method == "salamander.ui.dialog.clearItems")
        {
            std::string controlId;
            if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "controlId", &controlId))
                return FALSE;
            Salamatrix::UI::IControl* control =
                dialog->FindControl(controlId.c_str());
            if (control == NULL || !control->ClearItems())
                return FALSE;
            return CopyRuntimeHostResult(
                "{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }

        if (method == "salamander.ui.dialog.show")
        {
            int result = dialog->ShowModal();
            return CopyRuntimeHostResult(
                std::string("{\"ok\":true,\"result\":") +
                    std::to_string(result) + "}",
                resultJson,
                resultCapacity,
                resultLength);
        }

        if (method == "salamander.ui.dialog.get")
        {
            std::string controlId;
            if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "controlId", &controlId))
                return FALSE;
            Salamatrix::UI::IControl* control = dialog->FindControl(controlId.c_str());
            if (control == NULL)
                return FALSE;
            char value[4096];
            value[0] = '\0';
            control->GetText(value, _countof(value));
            std::string response =
                std::string("{\"ok\":true,\"text\":\"") +
                JsonEscapeRuntimeText(value) +
                "\",\"checked\":" +
                (control->GetChecked() ? "true" : "false") +
                ",\"itemCount\":" +
                std::to_string(control->GetItemCount()) +
                ",\"selectedIndex\":" +
                std::to_string(control->GetSelectedIndex()) + "}";
            return CopyRuntimeHostResult(
                response, resultJson, resultCapacity, resultLength);
        }

        if (method == "salamander.ui.dialog.set")
        {
            std::string controlId;
            std::string value;
            if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "controlId", &controlId) ||
                !Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "value", &value))
                return FALSE;
            Salamatrix::UI::IControl* control =
                dialog->FindControl(controlId.c_str());
            if (control == NULL || !control->SetText(value.c_str()))
                return FALSE;
            return CopyRuntimeHostResult(
                "{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }

        dialog->SetEventCallback(NULL, NULL);
        ui->DestroyDialog(dialog);
        for (int move = dialogIndex; move + 1 < script->m_nRuntimeDialogs; ++move)
            script->m_runtimeDialogs[move] = script->m_runtimeDialogs[move + 1];
        --script->m_nRuntimeDialogs;
        script->m_runtimeDialogs[script->m_nRuntimeDialogs].Owner = NULL;
        script->m_runtimeDialogs[script->m_nRuntimeDialogs].Id = 0;
        script->m_runtimeDialogs[script->m_nRuntimeDialogs].Dialog = NULL;
        script->m_runtimeDialogs[script->m_nRuntimeDialogs].EventsEnabled = FALSE;
        script->m_runtimeDialogs[script->m_nRuntimeDialogs].EventName[0] = '\0';
        return CopyRuntimeHostResult(
            "{\"ok\":true}", resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.ai.api" ||
        method == "salamander.ai.apiDescription")
    {
        std::string topic;
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "topic", &topic);
        Salamatrix::AI::IAssistantService* assistant =
            bridge->GetAssistantService();
        if (assistant == NULL)
            return FALSE;
        const char* description = assistant->GetApiDescriptionSlice(
            topic.empty() ? NULL : topic.c_str());
        if (description == NULL)
            return FALSE;
        std::string response =
            std::string("{\"ok\":true,\"description\":") +
            description + "}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.ai.generate" ||
        method == "salamander.ai.preview")
    {
        std::string prompt;
        std::string provider;
        std::string contextJson;
        std::string runtimeId;
        std::string existingScript;
        std::string feedback;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "prompt", &prompt))
            return FALSE;
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "provider", &provider);
        if (!Salamatrix::Runtime::Protocol::Json::FindRawMember(
                payloadJson, "context", &contextJson))
            contextJson = "{}";
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "runtime", &runtimeId);
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "existingScript", &existingScript);
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "feedback", &feedback);

        Salamatrix::AI::IAssistantService* assistant =
            bridge->GetAssistantService();
        if (assistant == NULL)
            return FALSE;
        Salamatrix::AI::AssistantRequest request;
        request.Prompt = prompt.c_str();
        request.ContextJson = contextJson.c_str();
        request.RuntimeId = runtimeId.empty() ? NULL : runtimeId.c_str();
        request.ExistingScript =
            existingScript.empty() ? NULL : existingScript.c_str();
        request.Feedback = feedback.empty() ? NULL : feedback.c_str();
        Salamatrix::AI::AssistantResponse responseData;
        BOOL generated = assistant->GenerateWithRepair(
            provider.empty() ? NULL : provider.c_str(),
            &request,
            &responseData,
            2);
        const char* status =
            responseData.Status == Salamatrix::AI::AssistantStatusSucceeded
                ? "succeeded"
                : responseData.Status == Salamatrix::AI::AssistantStatusUnavailable
                      ? "unavailable"
                      : responseData.Status == Salamatrix::AI::AssistantStatusCancelled
                            ? "cancelled"
                            : responseData.Status == Salamatrix::AI::AssistantStatusInvalidResponse
                                  ? "invalid_response"
                                  : "failed";
        bool preview = method == "salamander.ai.preview";
        bool canRun = generated &&
                      Salamatrix::AI::IsSafeToRun(responseData.Summary);
        std::string response =
            std::string("{\"ok\":") + (generated ? "true" : "false") +
            ",\"status\":\"" + status + "\",\"response\":" +
            (responseData.OutputLength != 0 ? responseData.ResponseJson : "null") +
            (preview ? std::string(",\"preview\":true,\"canRun\":") +
                           (canRun ? "true" : "false")
                     : std::string()) +
            "}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.events.subscribe")
    {
        std::string eventName;
        Salamatrix::Events::EventKind kind;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "event", &eventName) ||
            !RuntimeEventKindFromName(eventName, &kind) ||
            !script->AllowsSalamatrixEvent(eventName.c_str()) ||
            script->m_nRuntimeEventSubscriptions >=
                static_cast<int>(_countof(script->m_runtimeEventSubscriptions)))
            return FALSE;
        Salamatrix::Events::IEventsService* events = bridge->GetEventsService();
        if (events == NULL)
            return FALSE;
        ULONGLONG subscriptionId = 0;
        if (!events->Subscribe(
                kind,
                CScriptInfo::RuntimeEventCallback,
                script,
                &subscriptionId))
            return FALSE;
        script->m_runtimeEventSubscriptions[
            script->m_nRuntimeEventSubscriptions++] = subscriptionId;
        char id[32];
        _ui64toa_s(subscriptionId, id, _countof(id), 10);
        std::string response =
            std::string("{\"ok\":true,\"subscriptionId\":\"") + id + "\"}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.events.unsubscribe")
    {
        std::string idText;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "subscriptionId", &idText))
            return FALSE;
        char* end = NULL;
        ULONGLONG subscriptionId = _strtoui64(idText.c_str(), &end, 10);
        if (end == idText.c_str() || *end != '\0')
            return FALSE;
        Salamatrix::Events::IEventsService* events = bridge->GetEventsService();
        if (events == NULL || !events->Unsubscribe(subscriptionId))
            return FALSE;
        for (int index = 0; index < script->m_nRuntimeEventSubscriptions; ++index)
        {
            if (script->m_runtimeEventSubscriptions[index] == subscriptionId)
            {
                for (int move = index;
                     move + 1 < script->m_nRuntimeEventSubscriptions;
                     ++move)
                {
                    script->m_runtimeEventSubscriptions[move] =
                        script->m_runtimeEventSubscriptions[move + 1];
                }
                --script->m_nRuntimeEventSubscriptions;
                break;
            }
        }
        return CopyRuntimeHostResult(
            "{\"ok\":true}", resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.commands.execute")
    {
        std::string commandId;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "commandId", &commandId))
            return FALSE;
        Salamatrix::Commands::ICommandService* commands =
            bridge->GetCommandService();
        if (commands == NULL)
            return FALSE;
        Salamatrix::Commands::ExecuteOptions options;
        options.Parent = SalamanderGeneral->GetMsgBoxParent();
        Salamatrix::Runtime::OperationResult operation =
            commands->Execute(commandId.c_str(), options);
        const char* resultName =
            operation == Salamatrix::Runtime::OperationResultOk
                ? "ok"
                : operation == Salamatrix::Runtime::OperationResultNotAvailable
                      ? "not_available"
                      : "error";
        std::string response =
            "{\"ok\":" +
            std::string(operation == Salamatrix::Runtime::OperationResultOk
                            ? "true"
                            : "false") +
            ",\"result\":\"" + resultName + "\"}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.commands.register")
    {
        std::string commandId;
        std::string title;
        std::string handler;
        BOOL pluginMenu = TRUE;
        BOOL contextMenu = FALSE;
        BOOL toolbar = script->m_bManifestToolbar ? TRUE : FALSE;
        int hotKeyValue = 0;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "commandId", &commandId) || commandId.empty())
            return FALSE;
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "title", &title);
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "handler", &handler);
        if (title.empty())
            title = commandId;
        Salamatrix::Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "pluginMenu", &pluginMenu);
        Salamatrix::Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "contextMenu", &contextMenu);
        Salamatrix::Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "toolbar", &toolbar);
        Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
            payloadJson, "hotKey", &hotKeyValue);
        if (!script->m_bRuntimeCommandOwned &&
            script->m_salamatrixManifestCommands.empty() &&
            script->m_szSalamatrixCommandId[0] != _T('\0'))
            return FALSE;
        if (!script->RegisterRuntimeCommand(
                commandId.c_str(),
                title.c_str(),
                handler.empty() ? NULL : handler.c_str(),
                pluginMenu != FALSE,
                contextMenu != FALSE,
                toolbar != FALSE,
                static_cast<DWORD>(hotKeyValue),
                script->m_dwMenuEventOrMask,
                script->m_dwMenuEventAndMask))
            return FALSE;
        if (!script->m_bRuntimeCommandOwned &&
            script->m_salamatrixManifestCommands.empty())
        {
#ifdef UNICODE
            wchar_t nativeCommandId[128];
            wchar_t nativeTitle[256];
            if (!Utf8ToNative(commandId, nativeCommandId, _countof(nativeCommandId)) ||
                !Utf8ToNative(title, nativeTitle, _countof(nativeTitle)) ||
                StringCchCopy(script->m_szSalamatrixCommandId,
                              _countof(script->m_szSalamatrixCommandId),
                              nativeCommandId) != S_OK ||
                StringCchCopy(script->m_szDisplayName,
                              _countof(script->m_szDisplayName),
                              nativeTitle) != S_OK)
            {
                script->UnregisterRuntimeCommand(commandId.c_str());
                return FALSE;
            }
#else
            if (StringCchCopyA(script->m_szSalamatrixCommandId,
                               _countof(script->m_szSalamatrixCommandId),
                               commandId.c_str()) != S_OK ||
                StringCchCopyA(script->m_szDisplayName,
                               _countof(script->m_szDisplayName),
                               title.c_str()) != S_OK)
            {
                script->UnregisterRuntimeCommand(commandId.c_str());
                return FALSE;
            }
#endif
            StringCchCopyA(script->m_szRuntimeCommandId,
                           _countof(script->m_szRuntimeCommandId),
                           commandId.c_str());
        }
        script->m_bShowInPluginMenu = true;
        script->m_bShowInContextMenu = true;
        script->m_bRuntimeCommandOwned = true;
        SalamanderGeneral->PostPluginMenuChanged();
        return CopyRuntimeHostResult(
            "{\"ok\":true,\"registered\":true}",
            resultJson,
            resultCapacity,
            resultLength);
    }

    if (method == "salamander.commands.unregister")
    {
        std::string commandId;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "commandId", &commandId) ||
            !script->m_bRuntimeCommandOwned ||
            !script->UnregisterRuntimeCommand(commandId.c_str()))
            return FALSE;
        if (script->m_nRuntimeCommands == 0)
            script->ReleaseRuntimeCommand();
        return CopyRuntimeHostResult(
            "{\"ok\":true,\"unregistered\":true}",
            resultJson,
            resultCapacity,
            resultLength);
    }

    if (method == "salamander.fileOperations.rename" ||
        method == "salamander.fileOperations.copy" ||
        method == "salamander.fileOperations.move" ||
        method == "salamander.fileOperations.delete" ||
        method == "salamander.fileOperations.createDirectory" ||
        method == "salamander.fileOperations.refresh" ||
        method == "salamander.fileOperations.properties")
    {
        Salamatrix::FileOperations::IFileOperationsService* operations =
            bridge->GetFileOperationsService();
        if (operations == NULL)
            return FALSE;
        Salamatrix::FileOperations::InteractiveOptions options;
        options.Parent = SalamanderGeneral->GetMsgBoxParent();
        Salamatrix::Runtime::OperationResult operation =
            Salamatrix::Runtime::OperationResultError;
        if (method == "salamander.fileOperations.rename")
            operation = operations->RenameInteractive(options);
        else if (method == "salamander.fileOperations.copy")
            operation = operations->CopyInteractive(options);
        else if (method == "salamander.fileOperations.move")
            operation = operations->MoveInteractive(options);
        else if (method == "salamander.fileOperations.delete")
            operation = operations->DeleteInteractive(options);
        else if (method == "salamander.fileOperations.createDirectory")
            operation = operations->CreateDirectoryInteractive(options);
        else if (method == "salamander.fileOperations.refresh")
            operation = operations->Refresh(options);
        else if (method == "salamander.fileOperations.properties")
            operation = operations->ShowProperties(options);
        const char* name =
            operation == Salamatrix::Runtime::OperationResultOk
                ? "ok"
                : operation == Salamatrix::Runtime::OperationResultCancel
                      ? "cancel"
                      : operation == Salamatrix::Runtime::OperationResultNotAvailable
                            ? "not_available"
                            : "error";
        std::string response =
            std::string("{\"ok\":") +
            (operation == Salamatrix::Runtime::OperationResultOk ? "true" : "false") +
            ",\"result\":\"" + name + "\"}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.sides.context")
    {
        std::string sideName;
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "side", &sideName);
        Salamatrix::Sides::SideReference side =
            RuntimeSideFromName(sideName);
        Salamatrix::Sides::ISidesService* sides = bridge->GetSidesService();
        if (sides == NULL)
            return FALSE;
        char panelPath[SALAMATRIX_SIDE_ITEM_PATH_CAPACITY];
        panelPath[0] = '\0';
        int pathType = 0;
        if (!sides->GetPath(
                side,
                panelPath,
                _countof(panelPath),
                &pathType))
            return FALSE;
        int selectedCount = sides->GetSelectedItemCount(side);
        if (selectedCount < 0)
            selectedCount = 0;
        int returnedCount = selectedCount > 64 ? 64 : selectedCount;
        std::string response =
            std::string("{\"ok\":true,\"path\":\"") +
            JsonEscapeRuntimeText(panelPath) +
            "\",\"pathType\":" + std::to_string(pathType) +
            ",\"selectedCount\":" + std::to_string(selectedCount) +
            ",\"selectedItems\":[";
        for (int index = 0; index < returnedCount; ++index)
        {
            Salamatrix::Sides::ItemInfo item;
            if (!sides->GetSelectedItem(side, index, &item))
                continue;
            if (response[response.size() - 1] != '[')
                response.push_back(',');
            response += RuntimeItemInfoJson(item);
        }
        response += "],\"focusedItem\":";
        Salamatrix::Sides::ItemInfo focused;
        if (sides->GetFocusedItem(side, &focused))
            response += RuntimeItemInfoJson(focused);
        else
            response += "null";
        response += "}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.sides.activeTab")
    {
        std::string sideName;
        Salamatrix::Sides::SideReference side =
            Salamatrix::Sides::SideReferenceSource;
        if (Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "side", &sideName))
        {
            if (_stricmp(sideName.c_str(), "left") == 0)
                side = Salamatrix::Sides::SideReferenceLeft;
            else if (_stricmp(sideName.c_str(), "right") == 0)
                side = Salamatrix::Sides::SideReferenceRight;
            else if (_stricmp(sideName.c_str(), "target") == 0)
                side = Salamatrix::Sides::SideReferenceTarget;
        }
        Salamatrix::Sides::ISidesService* sides = bridge->GetSidesService();
        if (sides == NULL)
            return FALSE;
        Salamatrix::Sides::TabInfo info;
        if (!sides->GetActiveTabInfo(side, &info))
            return FALSE;
        char path[32768];
        int pathType = info.PathType;
        if (!sides->GetTabPath(info.TabId, path, _countof(path), &pathType))
            path[0] = '\0';
        char id[32];
        _ui64toa_s(info.TabId, id, _countof(id), 10);
        std::string response =
            std::string("{\"ok\":true,\"id\":\"") + id +
            "\",\"index\":" + std::to_string(info.Index) +
            ",\"pathType\":" + std::to_string(pathType) +
            ",\"path\":\"" + JsonEscapeRuntimeText(path) + "\"}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.sides.tabs")
    {
        std::string sideName;
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "side", &sideName);
        Salamatrix::Sides::SideReference side =
            RuntimeSideFromName(sideName);
        Salamatrix::Sides::ISidesService* sides = bridge->GetSidesService();
        if (sides == NULL)
            return FALSE;

        int tabCount = sides->GetTabCount(side);
        if (tabCount < 0)
            tabCount = 0;
        // Keep a single response bounded even if a future core exposes many
        // detached tabs; callers can request another side explicitly.
        int returnedCount = tabCount > 128 ? 128 : tabCount;
        std::string response = "{\"ok\":true,\"tabs\":[";
        for (int index = 0; index < returnedCount; ++index)
        {
            Salamatrix::Sides::TabInfo info;
            if (!sides->GetTabInfo(side, index, &info))
                continue;
            char tabId[32];
            _ui64toa_s(info.TabId, tabId, _countof(tabId), 10);
            char path[SALAMATRIX_SIDE_ITEM_PATH_CAPACITY];
            path[0] = '\0';
            int pathType = info.PathType;
            if (!sides->GetTabPath(
                    info.TabId, path, _countof(path), &pathType))
                path[0] = '\0';
            if (response[response.size() - 1] != '[')
                response.push_back(',');
            response += std::string("{\"id\":\"") + tabId +
                        "\",\"index\":" + std::to_string(info.Index) +
                        ",\"side\":" +
                        std::to_string(static_cast<int>(info.PhysicalSide)) +
                        ",\"pathType\":" + std::to_string(pathType) +
                        ",\"flags\":" + std::to_string(info.Flags) +
                        ",\"path\":\"" +
                        JsonEscapeRuntimeText(path) + "\"}";
        }
        response += "]}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.sides.activateTab")
    {
        CQuadWord tabId;
        if (!FindRuntimeQuadWord(payloadJson, "tabId", &tabId))
            return FALSE;
        BOOL focus = TRUE;
        Salamatrix::Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "focus", &focus);
        Salamatrix::Sides::ISidesService* sides = bridge->GetSidesService();
        if (sides == NULL)
            return FALSE;
        BOOL activated = sides->ActivateTab(tabId.Value, focus);
        if (activated)
        {
            Salamatrix::Sides::TabInfo tab;
            Salamatrix::Sides::SideReference side =
                Salamatrix::Sides::SideReferenceSource;
            if (sides->GetTabInfoById(tabId.Value, &tab))
                side = tab.PhysicalSide;
            Salamatrix::Events::PublishSideOperation(
                bridge->GetEventsService(),
                sides,
                Salamatrix::Events::EventKindSideTabChanged,
                side,
                0);
        }
        std::string response =
            std::string("{\"ok\":") +
            (activated ? "true" : "false") +
            ",\"activated\":" + (activated ? "true" : "false") + "}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.sides.changePath")
    {
        std::string sideName;
        std::string path;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "path", &path) || path.empty())
            return FALSE;
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "side", &sideName);
        Salamatrix::Sides::ISidesService* sides = bridge->GetSidesService();
        if (sides == NULL)
            return FALSE;
        int failReason = 0;
        Salamatrix::Sides::SideReference side = RuntimeSideFromName(sideName);
        BOOL changed = sides->ChangeActiveTabPath(
            side, path.c_str(), &failReason);
        if (changed)
            Salamatrix::Events::PublishSideOperation(
                bridge->GetEventsService(),
                sides,
                Salamatrix::Events::EventKindSidePathChanged,
                side,
                0);
        std::string response =
            std::string("{\"ok\":") +
            (changed ? "true" : "false") +
            ",\"changed\":" + (changed ? "true" : "false") +
            ",\"failReason\":" + std::to_string(failReason) + "}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.sides.refresh")
    {
        std::string sideName;
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "side", &sideName);
        BOOL forceRefresh = FALSE;
        BOOL focusFirstNewItem = FALSE;
        Salamatrix::Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "force", &forceRefresh);
        Salamatrix::Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "focusFirstNewItem", &focusFirstNewItem);
        Salamatrix::Sides::ISidesService* sides = bridge->GetSidesService();
        if (sides == NULL)
            return FALSE;
        Salamatrix::Sides::SideReference side = RuntimeSideFromName(sideName);
        BOOL refreshed = sides->Refresh(
            side, forceRefresh, focusFirstNewItem);
        if (refreshed)
            Salamatrix::Events::PublishSideOperation(
                bridge->GetEventsService(),
                sides,
                Salamatrix::Events::EventKindSideRefreshed,
                side,
                0);
        return CopyRuntimeHostResult(
            std::string("{\"ok\":") +
                (refreshed ? "true}" : "false}"),
            resultJson,
            resultCapacity,
            resultLength);
    }

    if (method == "salamander.sides.selectItem" ||
        method == "salamander.sides.selectAll" ||
        method == "salamander.sides.focusItem")
    {
        std::string sideName;
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "side", &sideName);
        Salamatrix::Sides::ISidesService* sides = bridge->GetSidesService();
        if (sides == NULL)
            return FALSE;
        BOOL select = TRUE;
        BOOL repaint = TRUE;
        BOOL partVisible = TRUE;
        Salamatrix::Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "select", &select);
        Salamatrix::Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "repaint", &repaint);
        Salamatrix::Runtime::Protocol::Json::FindBoolMember(
            payloadJson, "partVisible", &partVisible);

        Salamatrix::Sides::SideReference side = RuntimeSideFromName(sideName);
        BOOL changed = FALSE;
        DWORD eventParameter = 0;
        if (method == "salamander.sides.selectAll")
        {
            changed = sides->SelectAll(
                side, select, repaint);
            eventParameter = select ? 1 : 0;
        }
        else
        {
            int index = -1;
            if (!Salamatrix::Runtime::Protocol::Json::FindIntegerMember(
                    payloadJson, "index", &index) || index < 0)
                return FALSE;
            if (method == "salamander.sides.selectItem")
                changed = sides->SetItemSelected(
                    side, index, select, repaint);
            else
                changed = sides->FocusItem(
                    side, index, partVisible);
            eventParameter = static_cast<DWORD>(index);
        }
        if (changed)
            Salamatrix::Events::PublishSideOperation(
                bridge->GetEventsService(),
                sides,
                Salamatrix::Events::EventKindSideSelectionChanged,
                side,
                eventParameter);
        return CopyRuntimeHostResult(
            std::string("{\"ok\":") + (changed ? "true" : "false") +
                ",\"changed\":" + (changed ? "true}" : "false}"),
            resultJson,
            resultCapacity,
            resultLength);
    }

    if (method == "salamander.storage.get" ||
        method == "salamander.storage.set" ||
        method == "salamander.storage.remove" ||
        method == "salamander.storage.clear" ||
        method == "salamander.storage.schema")
    {
        Salamatrix::Storage::IStorageService* storage =
            bridge->GetStorageService();
        if (storage == NULL || script->m_szSalamatrixExtensionId[0] == '\0')
            return FALSE;
        if (method == "salamander.storage.schema")
        {
            std::string response = "{\"ok\":true,\"settings\":[";
            for (size_t index = 0;
                 index < script->m_salamatrixSettings.size();
                 ++index)
            {
                const CExtensionManifestSetting& setting =
                    script->m_salamatrixSettings[index];
                if (index != 0)
                    response += ",";
                response += "{\"key\":\"" +
                            JsonEscapeRuntimeText(setting.Key.c_str()) +
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
                    {
                        response += "\"" +
                                    JsonEscapeRuntimeText(
                                        setting.StringDefault.c_str()) +
                                    "\"";
                    }
                    else if (setting.Type == ExtensionManifestSettingInteger)
                    {
                        response += std::to_string(setting.IntegerDefault);
                    }
                    else
                    {
                        response += setting.BooleanDefault ? "true" : "false";
                    }
                }
                else
                {
                    response += ",\"hasDefault\":false";
                }
                response += "}";
            }
            response += "]}";
            return CopyRuntimeHostResult(
                response, resultJson, resultCapacity, resultLength);
        }
        if (method == "salamander.storage.clear")
        {
            BOOL cleared = storage->ClearExtension(
                script->m_szSalamatrixExtensionId);
            return CopyRuntimeHostResult(
                std::string("{\"ok\":") + (cleared ? "true}" : "false}"),
                resultJson,
                resultCapacity,
                resultLength);
        }
        std::string key;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "key", &key))
            return FALSE;
        if (method == "salamander.storage.remove")
        {
            BOOL removed = storage->DeleteValue(
                script->m_szSalamatrixExtensionId, key.c_str());
            return CopyRuntimeHostResult(
                std::string("{\"ok\":true,\"removed\":") +
                    (removed ? "true}" : "false}"),
                resultJson,
                resultCapacity,
                resultLength);
        }
        if (method == "salamander.storage.set")
        {
            std::string rawValue;
            if (!Salamatrix::Runtime::Protocol::Json::FindRawMember(
                    payloadJson, "value", &rawValue))
                return FALSE;

            BOOL stored = FALSE;
            if (!rawValue.empty() && rawValue[0] == '"')
            {
                std::string value;
                stored =
                    Salamatrix::Runtime::Protocol::Json::FindStringMember(
                        payloadJson, "value", &value) &&
                    storage->SetString(
                        script->m_szSalamatrixExtensionId,
                        key.c_str(),
                        value.c_str());
            }
            else if (rawValue == "true" || rawValue == "false")
            {
                BOOL value = FALSE;
                stored =
                    Salamatrix::Runtime::Protocol::Json::FindBoolMember(
                        payloadJson, "value", &value) &&
                    storage->SetBoolean(
                        script->m_szSalamatrixExtensionId,
                        key.c_str(),
                        value);
            }
            else
            {
                LONGLONG value = 0;
                stored =
                    Salamatrix::Runtime::Protocol::Json::FindInteger64Member(
                        payloadJson, "value", &value) &&
                    storage->SetInteger(
                        script->m_szSalamatrixExtensionId,
                        key.c_str(),
                        value);
            }
            if (!stored)
                return FALSE;
            return CopyRuntimeHostResult(
                "{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        Salamatrix::Storage::StorageValueType valueType =
            storage->GetValueType(
                script->m_szSalamatrixExtensionId, key.c_str());
        if (valueType == Salamatrix::Storage::StorageValueString)
        {
            std::vector<char> value(16385);
            int required = 0;
            if (!storage->GetString(
                    script->m_szSalamatrixExtensionId,
                    key.c_str(),
                    &value[0],
                    static_cast<int>(value.size()),
                    &required))
                return FALSE;
            std::string response =
                "{\"ok\":true,\"type\":\"string\",\"value\":\"" +
                JsonEscapeRuntimeText(&value[0]) + "\"}";
            return CopyRuntimeHostResult(
                response, resultJson, resultCapacity, resultLength);
        }
        if (valueType == Salamatrix::Storage::StorageValueInteger)
        {
            LONGLONG value = 0;
            if (!storage->GetInteger(
                    script->m_szSalamatrixExtensionId,
                    key.c_str(),
                    &value))
                return FALSE;
            std::string response =
                "{\"ok\":true,\"type\":\"integer\",\"value\":" +
                std::to_string(static_cast<long long>(value)) + "}";
            return CopyRuntimeHostResult(
                response, resultJson, resultCapacity, resultLength);
        }
        if (valueType == Salamatrix::Storage::StorageValueBoolean)
        {
            BOOL value = FALSE;
            if (!storage->GetBoolean(
                    script->m_szSalamatrixExtensionId,
                    key.c_str(),
                    &value))
                return FALSE;
            std::string response =
                std::string("{\"ok\":true,\"type\":\"boolean\",\"value\":") +
                (value ? "true}" : "false}");
            return CopyRuntimeHostResult(
                response, resultJson, resultCapacity, resultLength);
        }
        return CopyRuntimeHostResult(
            "{\"ok\":true,\"type\":\"missing\"}",
            resultJson,
            resultCapacity,
            resultLength);
    }

    return FALSE;
}

BOOL WINAPI CScriptInfo::RuntimeLifecycleCallback(
    void* context,
    Salamatrix::Extensions::ExtensionAction action,
    const Salamatrix::Extensions::ExtensionInfo* info)
{
    CScriptInfo* script = static_cast<CScriptInfo*>(context);
    if (script == NULL || info == NULL)
        return FALSE;

    if (action == Salamatrix::Extensions::ExtensionActionDeactivate)
    {
        script->ReleaseRuntimeSession();
        return TRUE;
    }

    if (action != Salamatrix::Extensions::ExtensionActionActivate)
        return FALSE;
    if (script->m_pRuntimeSession != NULL)
    {
        if (script->m_pRuntimeSession->IsAlive())
            return TRUE;
        script->ReleaseRuntimeSession();
    }

    CAutomationSalamatrixBridge* bridge =
        g_oAutomationPlugin.GetSalamatrixBridge();
    if (!bridge->HasRuntimeBroker())
    {
        g_oAutomationPlugin.RefreshSalamatrixServices();
        bridge = g_oAutomationPlugin.GetSalamatrixBridge();
    }
    Salamatrix::Runtime::IRuntimeService* runtime =
        bridge->GetRuntimeService();
    Salamatrix::Runtime::IRuntimeAdapter* adapter =
        runtime != NULL
            ? runtime->FindAdapter(
                  script->m_szSalamatrixRuntimeId,
                  script->m_dwSalamatrixMinimumRuntimeVersion)
            : NULL;
    if (adapter == NULL)
        return FALSE;

    if (!script->PublishSalamatrixManifestCommands())
    {
        script->ReleaseRuntimeCommand();
        return FALSE;
    }

    std::vector<wchar_t> entryPoint(SAL_MAX_PATH);
#ifdef UNICODE
    if (StringCchCopyW(
            &entryPoint[0], entryPoint.size(), script->m_szFileName) != S_OK)
        return FALSE;
#else
    if (MultiByteToWideChar(
            CP_ACP, 0, script->m_szFileName, -1,
            &entryPoint[0], static_cast<int>(entryPoint.size())) == 0)
        return FALSE;
#endif

    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.ExtensionId = script->m_szSalamatrixExtensionId;
    request.EntryPoint = &entryPoint[0];
    request.ParentWindow = SalamanderGeneral->GetMsgBoxParent();
    request.Flags =
        Salamatrix::Runtime::RuntimeExecutionFlagPersistentWorker |
        Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap;
    request.HostDispatch = CScriptInfo::RuntimeHostDispatch;
    request.HostDispatchContext = script;

    Salamatrix::Runtime::IRuntimeSession* session = NULL;
    if (!adapter->StartPersistent(&request, &session) || session == NULL)
    {
        script->ReleaseRuntimeCommand();
        return FALSE;
    }
    if (!session->IsAlive())
    {
        session->Release();
        script->ReleaseRuntimeCommand();
        return FALSE;
    }
    script->m_pRuntimeSession = session;
    script->m_hRuntimePumpThread = CreateThread(
        NULL, 0, CScriptInfo::RuntimePumpProc, script, 0, NULL);
    if (script->m_hRuntimePumpThread == NULL)
    {
        script->ReleaseRuntimeSession();
        return FALSE;
    }
    return TRUE;
}

bool CScriptInfo::EnsureEngineAssociation()
{
    if (IsEqualGUID(m_clsidEngine, GUID_NULL))
    {
        if (!g_oScriptAssociations.FindEngineByExt(
                PathFindExtension(m_szFileName),
                &m_clsidEngine))
        {
            // TODO: report a message
            return false;
        }
    }

    return true;
}

bool CScriptInfo::CreateEngine(EXECUTION_INFO* info)
{
    HRESULT hr;
    IActiveScriptParse* pParse;

    if (m_pScript)
    {
        return true;
    }

    hr = CoCreateInstance(m_clsidEngine, NULL, CLSCTX_INPROC_SERVER,
                          __uuidof(m_pScript), (void**)&m_pScript);
    if (SUCCEEDED(hr))
    {
        _ASSERTE(m_pSite == NULL);
        m_pSite = new CScriptSite(this);
        _ASSERTE(m_pSite);

        m_pShim = CScriptEngineShim::Create(this);

        if (info->bEnableDebugger)
        {
            InitializeDebugger(&info->dbgInfo);
        }

        m_hAbortEvent = HANDLES(CreateEvent(NULL, TRUE, FALSE, NULL));

        // BUG 103: ActiveRuby calls OnEnterScript very early
        // during parsing.
        m_pSite->SetExecutionInfo(info);

        hr = m_pScript->SetScriptSite(m_pSite);

        if (SUCCEEDED(hr))
        {
            hr = m_pScript->QueryInterface(&pParse);
            if (SUCCEEDED(hr))
            {
                hr = pParse->InitNew();

                if (SUCCEEDED(hr))
                {
                    hr = m_pScript->AddNamedItem(
                        L"Salamander",
                        SCRIPTITEM_ISVISIBLE); // These are the flags cscript.exe uses.

                    if (SUCCEEDED(hr))
                    {
                        hr = LoadScript(pParse, info);
                    }
                }

                pParse->Release();
            }
        }
    }

    if (FAILED(hr))
    {
        delete m_pShim;

        if (m_pScript != NULL)
        {
            m_pScript->Release();
            m_pScript = NULL;
        }

        if (m_pSite != NULL)
        {
            m_pSite->Release();
            m_pSite = NULL;
        }

        HANDLES(CloseHandle(m_hAbortEvent));
        m_hAbortEvent = NULL;

        TRACE_E("CreateEngine failed with error " << std::hex << hr);

        return false;
    }

    return true;
}

HRESULT CScriptInfo::LoadScript(IActiveScriptParse* pParse, EXECUTION_INFO* info)
{
    HRESULT hr;
    LPOLESTR pszCode;
    EXCEPINFO ei;
    ULONG cch;
    std::vector<TCHAR> szExpanded(SAL_MAX_PATH);

    if (!g_oAutomationPlugin.ExpandPath(
            m_szFileName, &szExpanded[0], static_cast<int>(szExpanded.size())))
    {
        return HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND);
    }

    hr = LoadOleStringFromFile(&szExpanded[0], pszCode, &cch);
    if (FAILED(hr))
    {
        TCHAR szMessage[256];
        TCHAR szError[192];

        FormatErrorText(hr, szError, _countof(szError));
        StringCchPrintf(szMessage, _countof(szMessage),
                        SalamanderGeneral->LoadStr(g_hLangInst, IDS_LOADERRFMT),
                        PathFindFileName(m_szFileName), szError);

        SalamanderGeneral->ShowMessageBox(
            szMessage,
            SalamanderGeneral->LoadStr(g_hLangInst, IDS_PLUGINNAME),
            MSGBOX_ERROR);

        return hr;
    }

    if (SUCCEEDED(hr) && info->dbgInfo.pDbgDocHelper != NULL)
    {
        HRESULT hrdbg;
        IDebugDocumentHelper* pddh = info->dbgInfo.pDbgDocHelper;

        IDebugDocumentHost* phost;
        hrdbg = m_pSite->QueryInterface(__uuidof(phost), (void**)&phost);
        if (SUCCEEDED(hrdbg))
        {
            hrdbg = pddh->SetDebugDocumentHost(phost);
            phost->Release();
        }

        hrdbg = pddh->AddUnicodeText(pszCode);
        if (SUCCEEDED(hrdbg))
        {
            hrdbg = pddh->DefineScriptBlock(0, cch, m_pScript, FALSE, &info->dbgInfo.dwSourceContext);
        }
    }

    hr = pParse->ParseScriptText(pszCode, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_HOSTMANAGESSOURCE, NULL, &ei);

    FreeOleString(pszCode);

    if (FAILED(hr) && hr != SCRIPT_E_REPORTED)
    {
        DisplayException(ei);
    }

    return hr;
}

HRESULT CScriptInfo::LoadOleStringFromFile(PCTSTR pszFileName, __out LPOLESTR& s, __out_opt ULONG* cch)
{
    HANDLE hFile, hMapping;
    DWORD cbSize;
    char* pszCodeA;
    HRESULT hr;
    int cchRequired, cchConverted;

    hFile = OpenReadFilePath(pszFileName);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    cbSize = GetFileSize(hFile, NULL);
    if (cbSize == 0)
    {
        CloseHandle(hFile);
        s = (LPOLESTR)malloc(sizeof(WCHAR));
        if (s == NULL)
        {
            return E_OUTOFMEMORY;
        }

        *s = L'\0';
        return S_OK;
    }

    hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    hr = HRESULT_FROM_WIN32(GetLastError());
    CloseHandle(hFile);
    if (hMapping == NULL)
    {
        return hr;
    }

    pszCodeA = (char*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    hr = HRESULT_FROM_WIN32(GetLastError());
    CloseHandle(hMapping);
    if (pszCodeA == NULL)
    {
        return hr;
    }

    cchRequired = MultiByteToWideChar(CP_ACP, 0, pszCodeA, cbSize, NULL, 0);
    if (cchRequired <= 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        UnmapViewOfFile(pszCodeA);
        return hr;
    }

    s = (LPOLESTR)malloc((cchRequired + 1) * sizeof(WCHAR));
    if (s == NULL)
    {
        UnmapViewOfFile(pszCodeA);
        return E_OUTOFMEMORY;
    }

    cchConverted = MultiByteToWideChar(CP_ACP, 0, pszCodeA, cbSize, s, cchRequired);
    hr = HRESULT_FROM_WIN32(GetLastError());
    UnmapViewOfFile(pszCodeA);
    if (cchConverted <= 0)
    {
        free(s);
        return hr;
    }

    // nul terminate
    s[cchConverted] = L'\0';

    if (cch != NULL)
    {
        *cch = cchConverted;
    }

    return S_OK;
}

bool CScriptInfo::ExecuteWorker(EXECUTION_INFO* info)
{
    HRESULT hr = S_OK;

    CALL_STACK_MESSAGE2("CScriptInfo::ExecuteWorker() (file name = \"%s\")", m_szFileName);

    info->bDeselect = false;

    m_pExecInfo = info;
    m_bAbortPending = false;

    _ASSERTE(m_pHardError == NULL);

    m_bSiteErrorDisplayed = false;
    if (!CreateEngine(info))
    {
        // If the error occured during parsing and was already displayed
        // through the site's OnScriptError event, don't display it here.
        // Otherwise display generic error message here.
        if (!m_bSiteErrorDisplayed)
        {
            SalamanderGeneral->SalMessageBox(
                SalamanderGeneral->GetMsgBoxParent(),
                SalamanderGeneral->LoadStr(g_hLangInst, IDS_ENGINECREATEFAIL),
                SalamanderGeneral->LoadStr(g_hLangInst, IDS_PLUGINNAME),
                MB_OK | MB_ICONERROR);
        }

        return false;
    }

    // workaround for WshShell.SendKeys to work properly (by john):
    // Salamander is not responding properly on SendKeys (http://msdn.microsoft.com/en-us/library/8c6yea83%28VS.85%29.aspx)
    // when Ctrl/Shift/Alt is still pressed (for example when script was started using Ctrl+Shift+Z hot key).
    // Miranda global hot key (Ctrl+Shift+A) is triggered on (salamander ->) Ctrl+Shift+Z -> (script started ->) SendKeys("a").
    // Attempt to release pressed Ctrl/Shift/Alt using SetKeyboardState() doesn't work.
    // Note: it seems that SendKeys is using API ::SendInput() beacause it doesn't work too (Miranda is activated).
    // Fortunately, following hack works pretty well.
    ResetKeyboardState();

    m_pShim->BeginExecution();

    // Run the script.
    // We used to have only SetScriptState(SCRIPTSTATE_CONNECTED) here, but tracing
    // the cscript.exe revealed, that it calls SCRIPTSTATE_INITIALIZED immediately
    // followed by SCRIPTSTATE_STARTED. We changed the logic here to match the one
    // of cscript.exe more closely. This was done primarily because of RScript22
    // won't execute a script without SCRIPTSTATE_STARTED. Hopefully, this won't
    // break other engines (JScript and VBScript seems fine).
    hr = m_pScript->SetScriptState(SCRIPTSTATE_INITIALIZED);
    if (SUCCEEDED(hr))
    {
        hr = m_pScript->SetScriptState(SCRIPTSTATE_STARTED);
    }

    // ActivePython returns SCRIPT_E_REPORTED if there was parse error.
    // If debugging is enabled, the SCRIPT_E_PROPAGATE may be returned
    // if debugger is detached while an exception is being debugged.
    _ASSERTE(SUCCEEDED(hr) || hr == SCRIPT_E_REPORTED || hr == SCRIPT_E_PROPAGATE || m_pHardError);

    m_pSite->SetExecutionInfo(NULL);
    UninitializeDebugger(&info->dbgInfo);
    m_pShim->EndExecution();

    // cscript.exe doesn't call SetScriptState at all at the end of the execution. But it can
    // afford not calling it because the process ends afterwards. We must deal with buggy
    // engines and do need to uninitialize it.
    hr = m_pScript->SetScriptState(SCRIPTSTATE_INITIALIZED);
    _ASSERTE(SUCCEEDED(hr));

    if (m_pHardError)
    {
        DisplayException(m_pHardError, m_pShim);
        m_pHardError->Release();
        m_pHardError = NULL;
    }

    hr = m_pShim->ReleaseEngine(m_pScript);
    _ASSERTE(SUCCEEDED(hr));

    m_pScript = NULL;

    m_pSite->Release();
    m_pSite = NULL;

    m_pExecInfo = NULL;

    HANDLES(CloseHandle(m_hAbortEvent));
    m_hAbortEvent = NULL;

    delete m_pShim;
    m_pShim = NULL;

    return SUCCEEDED(hr);
}

bool CScriptInfo::ExecuteInSeparateThread(EXECUTION_INFO* info)
{
    HANDLE hThread;

    info->pInstance = this;
    info->bAsyncResult = false;

    hThread = CreateThread(NULL, 0, ExecuteEntryProc, info, 0, NULL);
    if (hThread != NULL)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    return info->bAsyncResult;
}

DWORD WINAPI CScriptInfo::ExecuteEntryProc(void* arg)
{
    HRESULT hr;
    CScriptInfo* that;
    EXECUTION_INFO* info;

    info = reinterpret_cast<EXECUTION_INFO*>(arg);
    that = reinterpret_cast<CScriptInfo*>(info->pInstance);

    hr = CoInitializeEx(NULL, /*COINIT_MULTITHREADED*/ COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        info->bAsyncResult = that->ExecuteWorker(info);

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        CoUninitialize();
    }

    return 0;
}

void CScriptInfo::InitializeDebugger(DEBUG_INFO* dbgInfo)
{
    HRESULT hr;

    hr = CoCreateInstance(
        CLSID_ProcessDebugManager,
        NULL,
        CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
        __uuidof(IProcessDebugManager),
        (void**)&dbgInfo->pProcDbgMgr);
    if (FAILED(hr))
        return;

    hr = dbgInfo->pProcDbgMgr->GetDefaultApplication(&dbgInfo->pDbgApp);
    if (FAILED(hr))
        return;

    hr = dbgInfo->pProcDbgMgr->CreateDebugDocumentHelper(NULL, &dbgInfo->pDbgDocHelper);
    if (FAILED(hr))
        return;

    std::vector<OLECHAR> szUrl(SAL_MAX_PATH * 2);
    DWORD cchUrl = static_cast<DWORD>(szUrl.size());
    A2OLE sFileNameW(m_szFileName);
    if (FAILED(UrlCreateFromPathW(
            A2OLE(sFileNameW), &szUrl[0], &cchUrl, 0)))
    {
        StringCchCopyW(&szUrl[0], szUrl.size(), sFileNameW);
    }
    hr = dbgInfo->pDbgDocHelper->Init(dbgInfo->pDbgApp,
                                      A2OLE(GetDisplayName()), &szUrl[0], TEXT_DOC_ATTR_READONLY);
    if (FAILED(hr))
        return;

    hr = dbgInfo->pDbgDocHelper->Attach(NULL);
}

void CScriptInfo::UninitializeDebugger(DEBUG_INFO* dbgInfo)
{
    if (dbgInfo->pDbgDocHelper)
    {
        dbgInfo->pDbgDocHelper->Detach();
        dbgInfo->pDbgDocHelper->Release();
    }

    if (dbgInfo->pDbgApp)
    {
        dbgInfo->pDbgApp->Release();
    }

    if (dbgInfo->pProcDbgMgr)
    {
        dbgInfo->pProcDbgMgr->Release();
    }

    memset(dbgInfo, 0, sizeof(DEBUG_INFO));
}

HRESULT CScriptInfo::AbortScript()
{
    HRESULT hr;

    _ASSERTE(m_pScript);
    _ASSERTE(m_pShim);

    hr = m_pShim->InterruptScript(m_pScript);

    if (!m_bAbortPending)
    {
        m_bAbortPending = true;

        // Cooperatively abort native execution (e.g. quit modal
        // message boxes, exit GUI loops, cancel sleeps, break
        // enumerators etc.).

        // Set the abort event, so the kernel waits can exit (e.g.
        // Salamander.Sleep()).
        _ASSERTE(m_hAbortEvent != NULL);
        SetEvent(m_hAbortEvent);

        HWND hwndAbortTarget = *(volatile HWND*)&m_hwndAbortTarget;
        if (hwndAbortTarget != NULL)
        {
            PostMessage(hwndAbortTarget, WM_SAL_ABORTMODAL, 0, 0);
        }
    }

    return hr;
}

void CScriptInfo::ScriptEnter()
{
    HWND hwndPalette;

    _ASSERTE(m_pExecInfo);
    _ASSERTE(m_pExecInfo->pAbortPalette == NULL);

    if (m_pExecInfo->pAbortPalette == NULL)
    {
        m_pExecInfo->pAbortPalette = new CScriptAbortPalette(this);
        hwndPalette = m_pExecInfo->pAbortPalette->GetHwnd();
        _ASSERTE(IsWindow(hwndPalette));

        // Make the main window inaccessible, since the script may display
        // modeless window and the user can unload the whole plugin from
        // the main window in the mean time.
        // NOTE: 'hwndPalette' mechanism is not used because WS_EX_TOPMOST with process tree
        // checking using WindowBelongsToProcessID() look like better solution for now.
        // For example unpack script is starting several command prompt windows
        // so WS_EX_TOPMOST toolbar is better accessible.
        SalamanderGeneral->LockMainWindow(TRUE, NULL, SalamanderGeneral->LoadStr(g_hLangInst, IDS_MAINWINDOWLOCKED));
    }
}

void CScriptInfo::ScriptLeave()
{
    if (m_pExecInfo != NULL)
    {
        // Enable the main window again.
        SalamanderGeneral->LockMainWindow(FALSE, NULL, NULL);

        delete m_pExecInfo->pAbortPalette;
        m_pExecInfo->pAbortPalette = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////

CScriptContainer::CScriptContainer()
{
    m_pParent = NULL;
    m_pSibling = NULL;
    m_pChild = NULL;
    m_pScripts = NULL;
    m_szPath[0] = _T('\0');
    m_pszName = NULL;
}

CScriptContainer::CScriptContainer(
    CScriptContainer* pParent,
    PCTSTR pszPath,
    bool bFullPath)
{
    m_pParent = pParent;
    m_pSibling = NULL;
    m_pChild = NULL;
    m_pScripts = NULL;

    if (bFullPath)
    {
        StringCchCopy(m_szPath, _countof(m_szPath), pszPath);
    }
    else
    {
        _ASSERTE(pParent);
        StringCchCopy(m_szPath, _countof(m_szPath), pParent->m_szPath);
        AppendSalPath(m_szPath, pszPath, _countof(m_szPath));
    }

    PathRemoveBackslash(m_szPath);
    m_pszName = PathFindFileName(m_szPath);
}

CScriptContainer::~CScriptContainer()
{
}

CScriptContainer* CScriptContainer::FirstChild(
    PCTSTR pszPath,
    bool bFullPath)
{
    CScriptContainer* pIter;
    std::vector<TCHAR> szFullPath(SAL_MAX_PATH);

    if (m_pChild == NULL)
    {
        return NULL;
    }

    pIter = m_pChild;

    if (bFullPath)
    {
        StringCchCopy(&szFullPath[0], szFullPath.size(), pszPath);
    }
    else
    {
        StringCchCopy(&szFullPath[0], szFullPath.size(), m_szPath);
        AppendSalPath(
            &szFullPath[0], pszPath, static_cast<int>(szFullPath.size()));
    }

    while (pIter)
    {
        if (_tcsicmp(pIter->m_szPath, &szFullPath[0]) == 0)
        {
            return pIter;
        }

        pIter = pIter->m_pSibling;
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////

CScriptLookup::CScriptLookup()
{
    m_pRootContainer = NULL;
    memset(m_apHashBins, 0, sizeof(m_apHashBins));
    m_cScriptsTotal = 0;
    m_bModified = false;
    m_dwLastRefreshTime = 0;
}

CScriptLookup::~CScriptLookup()
{
    if (m_pRootContainer)
    {
        CascadeDeleteContainer(m_pRootContainer);
    }

    CScriptInfo *pIter, *pTmp;
    for (int iBin = 0; iBin < _countof(m_apHashBins); iBin++)
    {
        pIter = m_apHashBins[iBin];
        while (pIter)
        {
            pTmp = pIter->m_pNextHash;
            delete pIter;
            pIter = pTmp;
        }
    }
}

bool CScriptLookup::Load(HKEY hKey, CSalamanderRegistryAbstract* registry)
{
    int cDirs;
    int iDir;

    if (m_pRootContainer == NULL)
    {
        m_pRootContainer = new CScriptContainer();
    }

    if (hKey != INVALID_HANDLE_VALUE)
    {
        // not a refresh
        m_bModified = false;
    }

    cDirs = g_oAutomationPlugin.GetScriptDirectoryCount();
    for (iDir = 0; iDir < cDirs; iDir++)
    {
        CScriptContainer* pContainer;
        bool bExisting;

        pContainer = m_pRootContainer->FirstChild(
            g_oAutomationPlugin.GetScriptDirectoryRaw(iDir), true);
        bExisting = (pContainer != NULL);

        if (!bExisting)
        {
            pContainer = new CScriptContainer(m_pRootContainer,
                                              g_oAutomationPlugin.GetScriptDirectoryRaw(iDir), true);
        }

        if (FillContainer(pContainer, hKey, registry) > 0)
        {
            if (!bExisting)
            {
                LinkContainer(pContainer, m_pRootContainer);
            }
        }
        else if (!bExisting)
        {
            delete pContainer;
        }
    }

    m_dwLastRefreshTime = GetTickCount();

    return true;
}

bool CScriptLookup::Save(HKEY hKey, CSalamanderRegistryAbstract* registry)
{
    _ASSERTE(hKey != NULL);
    _ASSERTE(registry != NULL);

    if (!registry->ClearKey(hKey))
    {
        return false;
    }

    for (int iBin = 0; iBin < _countof(m_apHashBins); iBin++)
    {
        if (m_apHashBins[iBin] != NULL)
        {
            SaveBin(m_apHashBins[iBin], hKey, registry);
        }
    }

    m_bModified = false;

    return true;
}

bool CScriptLookup::SaveBin(
    CScriptInfo* pFirst,
    HKEY hKey,
    CSalamanderRegistryAbstract* registry)
{
    TCHAR szName[8];
    HKEY hkSub = NULL;
    UINT nPrevHash = 0;
    UINT nHash;
    CScriptInfo* pIter;

    for (pIter = pFirst; pIter; pIter = pIter->m_pNextHash)
    {
        nHash = HashFromId(pIter->m_nId);
        if (nHash != nPrevHash)
        {
            if (hkSub != NULL)
            {
                registry->CloseKey(hkSub);
                hkSub = NULL;
            }

            StringCchPrintf(szName, _countof(szName), "%06X", nHash);
            if (!registry->CreateKey(hKey, szName, hkSub))
            {
                return false;
            }

            nPrevHash = nHash;
        }

        StringCchPrintf(szName, _countof(szName), "%02X", UniquierFromId(pIter->m_nId));
        registry->SetValue(hkSub, szName, REG_SZ, pIter->GetFileName(), -1);
    }

    if (hkSub != NULL)
    {
        registry->CloseKey(hkSub);
    }

    return true;
}

CScriptInfo* CScriptLookup::LookupScript(int nId)
{
    int iBin;
    CScriptInfo* pScript;

    iBin = HashFromId(nId) % _countof(m_apHashBins);
    pScript = m_apHashBins[iBin];
    while (pScript)
    {
        if (pScript->m_nId == nId)
        {
            return pScript;
        }
        else if (pScript->m_nId > nId)
        {
            break;
        }

        pScript = pScript->m_pNextHash;
    }

    return NULL;
}

CScriptInfo* CScriptLookup::LookupRuntimeCommand(int menuId)
{
    for (int bin = 0; bin < _countof(m_apHashBins); ++bin)
    {
        CScriptInfo* script = m_apHashBins[bin];
        while (script != NULL)
        {
            if (script->FindRuntimeCommandIndexByMenuId(menuId) >= 0)
                return script;
            script = script->m_pNextHash;
        }
    }
    return NULL;
}

int CScriptLookup::FillContainer(
    CScriptContainer* pContainer,
    HKEY hKey,
    CSalamanderRegistryAbstract* registry)
{
    HANDLE hFind;
    std::vector<TCHAR> szPattern(SAL_MAX_PATH);
    WIN32_FIND_DATA fd;
    int cScripts = 0;

    g_oAutomationPlugin.ExpandPath(
        pContainer->GetPath(), &szPattern[0], static_cast<int>(szPattern.size()));
    AppendSalPath(
        &szPattern[0], _T("*"), static_cast<int>(szPattern.size()));

    hFind = FindFirstFile(&szPattern[0], &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    do
    {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        {
            continue;
        }

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (fd.cFileName[0] != _T('.')) // exclude . and .. as well as unix style hidden dirs
            {
                int cSubScripts = 0;
                CScriptContainer* pSubContainer;
                bool bExisting;

                pSubContainer = pContainer->FirstChild(fd.cFileName, false);
                bExisting = (pSubContainer != NULL);
                if (!bExisting)
                {
                    pSubContainer = new CScriptContainer(pContainer, fd.cFileName, false);
                }

                cSubScripts = FillContainer(pSubContainer, hKey, registry);
                if (cSubScripts > 0)
                {
                    if (!bExisting)
                    {
                        LinkContainer(pSubContainer, pContainer);
                    }
                    cScripts += cSubScripts;
                }
                else if (!bExisting)
                {
                    delete pSubContainer;
                }
            }
        }
        else
        {
            PTSTR pszExt = PathFindExtension(fd.cFileName);
            if (pszExt && *pszExt)
            {
                std::vector<TCHAR> szFullPath(SAL_MAX_PATH);
                StringCchCopy(
                    &szFullPath[0], szFullPath.size(), pContainer->GetPath());
                AppendSalPath(
                    &szFullPath[0], fd.cFileName,
                    static_cast<int>(szFullPath.size()));

                bool supported = g_oScriptAssociations.FindEngineByExt(pszExt);
                if (!supported)
                {
                    CExtensionManifest manifest;
                    supported = LoadManifestForEntryPoint(
                        &szFullPath[0], manifest) != FALSE;
                }
                if (!supported)
                {
                    const CAutomationSalamatrixBridge* bridge =
                        g_oAutomationPlugin.GetSalamatrixBridge();
                    Salamatrix::Runtime::IRuntimeService* runtimes =
                        bridge->GetRuntimeService();
                    std::vector<char> entryPointUtf8(SAL_MAX_PATH * 3);
                    supported =
                        runtimes != NULL &&
                        NativeToUtf8(
                            &szFullPath[0], &entryPointUtf8[0],
                            static_cast<int>(entryPointUtf8.size())) &&
                        runtimes->FindAdapterForEntryPoint(&entryPointUtf8[0]) != NULL;
                }

                if (supported &&
                    AddScriptFromFile(
                        pContainer, &szFullPath[0], hKey, registry))
                {
                    ++cScripts;
                    ++m_cScriptsTotal;
                }
            }
        }
    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);

    return (cScripts > 0);
}

void CScriptLookup::LinkContainer(CScriptContainer* pContainer, CScriptContainer* pParent)
{
    _ASSERTE(pParent != NULL);

    if (pParent->m_pChild == NULL)
    {
        pParent->m_pChild = pContainer;
    }
    else
    {
        CScriptContainer* pIter = pParent->m_pChild;
        CScriptContainer* pPrev = NULL;

        while (pIter && _tcsicmp(pContainer->m_pszName, pIter->m_pszName) >= 0)
        {
            pPrev = pIter;
            pIter = pIter->m_pSibling;
        }

        if (pPrev)
        {
            pContainer->m_pSibling = pPrev->m_pSibling;
            pPrev->m_pSibling = pContainer;
        }
        else
        {
            pContainer->m_pSibling = pParent->m_pChild;
            pParent->m_pChild = pContainer;
        }
    }
}

void CScriptLookup::UnlinkContainer(CScriptContainer* pContainer)
{
    _ASSERTE(pContainer != NULL);
    _ASSERTE(pContainer->m_pChild == NULL);
    _ASSERTE(pContainer->m_pScripts == NULL);

    CScriptContainer* pParent = pContainer->m_pParent;
    CScriptContainer *pIter, *pPrev = NULL;

    pIter = pParent->m_pChild;
    while (pIter != pContainer)
    {
        pPrev = pIter;
        pIter = pIter->m_pSibling;
    }

    if (pPrev == NULL)
    {
        pParent->m_pChild = pContainer->m_pSibling;
    }
    else
    {
        pPrev->m_pSibling = pContainer->m_pSibling;
    }

    pContainer->m_pSibling = NULL;
    pContainer->m_pParent = NULL;
}

void CScriptLookup::LinkScript(
    CScriptInfo* pScript)
{
    CScriptContainer* pParent = pScript->m_pContainer;

    if (pParent->m_pScripts == NULL)
    {
        pParent->m_pScripts = pScript;
    }
    else
    {
        CScriptInfo* pIter = pParent->m_pScripts;
        CScriptInfo* pPrev = NULL;

        while (pIter && _tcsicmp(pScript->GetDisplayName(), pIter->GetDisplayName()) >= 0)
        {
            pPrev = pIter;
            pIter = pIter->m_pNext;
        }

        if (pPrev)
        {
            pScript->m_pNext = pPrev->m_pNext;
            pPrev->m_pNext = pScript;
        }
        else
        {
            pScript->m_pNext = pParent->m_pScripts;
            pParent->m_pScripts = pScript;
        }
    }
}

void CScriptLookup::UnlinkScript(
    CScriptInfo* pScript)
{
    CScriptContainer* pParent = pScript->m_pContainer;
    CScriptInfo *pIter, *pPrev = NULL;

    pIter = pParent->m_pScripts;
    while (pIter != pScript)
    {
        pPrev = pIter;
        pIter = pIter->m_pNext;
    }

    if (pPrev == NULL)
    {
        pParent->m_pScripts = pScript->m_pNext;
    }
    else
    {
        pPrev->m_pNext = pScript->m_pNext;
    }

    pScript->m_pNext = NULL;
    pScript->m_pContainer = NULL;
}

void CScriptLookup::LinkScriptHash(CScriptInfo* pScript)
{
    int iBin;

    iBin = HashFromId(pScript->m_nId) % _countof(m_apHashBins);
    if (m_apHashBins[iBin] == NULL)
    {
        m_apHashBins[iBin] = pScript;
    }
    else
    {
        CScriptInfo* pIter = m_apHashBins[iBin];
        CScriptInfo* pPrev = NULL;

        while (pIter && pIter->m_nId <= pScript->m_nId)
        {
            pPrev = pIter;
            pIter = pIter->m_pNextHash;
        }

        if (pPrev)
        {
            pScript->m_pNextHash = pPrev->m_pNextHash;
            pPrev->m_pNextHash = pScript;
        }
        else
        {
            pScript->m_pNextHash = m_apHashBins[iBin];
            m_apHashBins[iBin] = pScript;
        }
    }
}

CScriptInfo* CScriptLookup::AddScriptFromFile(
    CScriptContainer* pContainer,
    PCTSTR pszFullPath,
    HKEY hKey,
    CSalamanderRegistryAbstract* registry)
{
    UINT nHash;
    int nUniquier;
    CScriptInfo* pScript;

    nHash = HashPath(pszFullPath);
    if (nHash == 0)
    {
        _ASSERTE(0);
        return NULL;
    }

    pScript = LookupScriptByPath(nHash, pszFullPath);
    if (pScript)
    {
        // script already in the hash map,
        // clear the dirty flag
        pScript->ResetDirty();
        return pScript;
    }

    nUniquier = GetUniquier(nHash, pszFullPath, hKey, registry);
    if (nUniquier < 0)
    {
        return NULL;
    }

    pScript = new CScriptInfo(pszFullPath, pContainer);
    pScript->m_nId = MakeId(nHash, nUniquier);
    LinkScript(pScript);
    LinkScriptHash(pScript);

    return pScript;
}

int CScriptLookup::GetUniquier(
    UINT nHash,
    __in_z PCTSTR pszPath,
    HKEY hKey,
    CSalamanderRegistryAbstract* registry)
{
    int iBin;
    CScriptInfo* pScript;
    int nUniquier;
    CUniquierBitmap bitmap;

    if (hKey != NULL && hKey != INVALID_HANDLE_VALUE)
    {
        HKEY hkSub;
        TCHAR szName[8];
        StringCchPrintf(szName, _countof(szName), "%06X", HashFromId(nHash));
        if (!registry->OpenKey(hKey, szName, hkSub))
        {
            // the hash key does not even exist in the registry,
            // return 1st available uniquier
            m_bModified = true;
            return 0;
        }

        LONG res = NO_ERROR;
        DWORD dwIndex = 0;
        DWORD cchName;
        DWORD dwType;
        std::vector<TCHAR> szPathRead(SAL_MAX_PATH);
        DWORD cbData;

        for (; res == NO_ERROR; dwIndex++)
        {
            cchName = _countof(szName);
            cbData = static_cast<DWORD>(szPathRead.size() * sizeof(TCHAR));
            res = RegEnumValue(hkSub, dwIndex, szName, &cchName,
                               NULL, &dwType, (LPBYTE)&szPathRead[0], &cbData);
            if (res == NO_ERROR && dwType == REG_SZ)
            {
                nUniquier = _tcstol(szName, NULL, 16);

                // mark this uniquier as used in the free bitmap
                bitmap.MarkBusy(nUniquier);

                if (_tcsicmp(pszPath, &szPathRead[0]) == 0)
                {
                    // we found exact uniquier for this script
                    registry->CloseKey(hkSub);
                    return nUniquier;
                }
            }
        }

        registry->CloseKey(hkSub);

        // the script path was not found in the registry,
        // look if we can allocate a new uniquier for this path
        m_bModified = true;
        return bitmap.Alloc();
    }

    m_bModified = true;
    iBin = nHash % _countof(m_apHashBins);
    pScript = m_apHashBins[iBin];
    CScriptInfo* pFirstScript = NULL;
    while (pScript)
    {
        if (HashFromId(pScript->m_nId) == HashFromId(nHash))
        {
            pFirstScript = pScript;
            break;
        }
        else if (HashFromId(pScript->m_nId) > HashFromId(nHash))
        {
            break;
        }

        pScript = pScript->m_pNextHash;
    }

    if (pFirstScript != NULL)
    {
        while (pFirstScript && HashFromId(pFirstScript->m_nId) == HashFromId(nHash))
        {
            bitmap.MarkBusy(UniquierFromId(pFirstScript->m_nId));
            pFirstScript = pFirstScript->m_pNextHash;
        }

        return bitmap.Alloc();
    }

    // no uniquier for this hash yet, start counting at zero
    return 0;
}

UINT CScriptLookup::HashPath(__in_z PCTSTR pszPath)
{
    UINT nHash;
    std::vector<TCHAR> szCanonicalPath(SAL_MAX_PATH);

    StringCchCopy(&szCanonicalPath[0], szCanonicalPath.size(), pszPath);
    CharLower(&szCanonicalPath[0]);
    nHash = HashString(&szCanonicalPath[0]);
    if (nHash == 0)
    {
        nHash = ~0u;
    }

    nHash <<= HASH_SHIFT;
    nHash &= HASH_MASK;

    return nHash;
}

void CScriptLookup::CascadeDeleteContainer(
    CScriptContainer* pContainer)
{
    if (pContainer)
    {
        CascadeDeleteContainer(pContainer->m_pChild);

        while (pContainer->m_pSibling)
        {
            CScriptContainer* pTmp;
            pTmp = pContainer->m_pSibling->m_pSibling;
            CascadeDeleteContainer(pContainer->m_pSibling->m_pChild);
            delete pContainer->m_pSibling;
            pContainer->m_pSibling = pTmp;
        }

        delete pContainer;
    }
}

bool CScriptLookup::Refresh(bool bForce)
{
    bool res;

    if (GetTickCount() - m_dwLastRefreshTime < 5000 && !bForce)
    {
        // ignore refresh request if it comes too early
        // since the last one (this prevents excessive
        // disk activity when user repeatedly runs a script)
        return true;
    }

    // Keep the lifecycle registry in sync with the objects that are about to
    // be replaced by the refresh. Ownership is the CScriptInfo address, so
    // unregister before RemoveDirtyScripts deletes anything.
    UnpublishSalamatrixExtensions();

    // assume all existing scripts dirty
    MarkAllScriptsDirty();

    res = Load((HKEY)INVALID_HANDLE_VALUE, NULL);

    // remove scripts that remained dirty after the refresh
    RemoveDirtyScripts();

    // remove containers that remained empty
    RemoveEmptyContainers(m_pRootContainer);

    PublishSalamatrixExtensions();

    return res;
}

void CScriptLookup::UnpublishSalamatrixExtensions()
{
    CAutomationSalamatrixBridge* bridge =
        g_oAutomationPlugin.GetSalamatrixBridge();
    if (!bridge->HasExtensions())
    {
        g_oAutomationPlugin.RefreshSalamatrixServices();
        bridge = g_oAutomationPlugin.GetSalamatrixBridge();
    }

    Salamatrix::Extensions::IExtensionsService* service =
        bridge->GetExtensionsService();
    if (service == NULL)
        return;

    for (int iBin = 0; iBin < _countof(m_apHashBins); iBin++)
    {
        for (CScriptInfo* pScript = m_apHashBins[iBin];
             pScript != NULL;
             pScript = pScript->m_pNextHash)
        {
            if (pScript->GetSalamatrixExtensionId()[0] != '\0')
            {
                service->UnregisterExtension(
                    pScript->GetSalamatrixExtensionId(), pScript);
            }
        }
    }
}

DWORD WINAPI CScriptInfo::RuntimePumpProc(void* arg)
{
    CScriptInfo* script = static_cast<CScriptInfo*>(arg);
    if (script == NULL)
        return 0;

    Salamatrix::Runtime::IRuntimeSession* session = script->m_pRuntimeSession;
    if (session == NULL)
        return 0;

    while (session->IsAlive())
    {
        if (!session->Pump(250) && !session->IsAlive())
            break;
    }
    return 0;
}

void CScriptInfo::ReleaseRuntimeEventSubscriptions()
{
    if (m_nRuntimeEventSubscriptions <= 0)
        return;
    CAutomationSalamatrixBridge* bridge =
        g_oAutomationPlugin.GetSalamatrixBridge();
    Salamatrix::Events::IEventsService* events =
        bridge != NULL ? bridge->GetEventsService() : NULL;
    if (events != NULL)
    {
        for (int index = 0; index < m_nRuntimeEventSubscriptions; ++index)
            events->Unsubscribe(m_runtimeEventSubscriptions[index]);
    }
    memset(m_runtimeEventSubscriptions, 0, sizeof(m_runtimeEventSubscriptions));
    m_nRuntimeEventSubscriptions = 0;
}

bool CScriptInfo::RegisterRuntimeCommand(
    const char* commandId,
    const char* title,
    const char* handler,
    bool pluginMenu,
    bool contextMenu,
    bool toolbar,
    DWORD hotKey,
    DWORD menuEventOrMask,
    DWORD menuEventAndMask,
    const char* iconPath,
    const char* iconDarkPath)
{
    if (commandId == NULL || commandId[0] == '\0')
        return false;
    for (int index = 0; index < m_nRuntimeCommands; ++index)
    {
        if (_stricmp(m_runtimeCommands[index].Id, commandId) == 0)
            return true;
    }
    if (m_nRuntimeCommands >= _countof(m_runtimeCommands))
        return false;
    RUNTIME_COMMAND_INFO& command = m_runtimeCommands[m_nRuntimeCommands];
    if (StringCchCopyA(command.Id, _countof(command.Id), commandId) != S_OK)
        return false;
    if (handler != NULL && handler[0] != '\0' &&
        StringCchCopyA(command.Handler, _countof(command.Handler), handler) != S_OK)
        return false;
    char fallbackTitle[256];
    if (title == NULL || title[0] == '\0')
        title = commandId;
    if (StringCchCopyA(fallbackTitle, _countof(fallbackTitle), title) != S_OK)
        return false;
#ifdef UNICODE
    if (!Utf8ToNative(std::string(fallbackTitle), command.Title, _countof(command.Title)))
        return false;
#else
    if (StringCchCopyA(command.Title, _countof(command.Title), fallbackTitle) != S_OK)
        return false;
#endif
    LONG menuId = InterlockedIncrement(&g_nextRuntimeCommandMenuId);
    if (menuId <= 0)
        return false;
    command.MenuId = static_cast<int>(menuId);
    command.PluginMenu = pluginMenu;
    command.ContextMenu = contextMenu;
    command.Toolbar = toolbar;
    command.HotKey = hotKey;
    command.MenuEventOrMask = menuEventOrMask;
    command.MenuEventAndMask = menuEventAndMask;
    if (toolbar)
    {
        CSalamanderToolbarButton toolbarButton;
        toolbarButton.CommandId = command.MenuId;
        toolbarButton.Title = fallbackTitle;
        toolbarButton.IconPath = iconPath != NULL && iconPath[0] != '\0'
                                     ? iconPath
                                     : (m_salamatrixIconPath.empty()
                                            ? NULL
                                            : m_salamatrixIconPath.c_str());
        toolbarButton.IconDarkPath = iconDarkPath != NULL && iconDarkPath[0] != '\0'
                                         ? iconDarkPath
                                         : (m_salamatrixIconDarkPath.empty()
                                                ? NULL
                                                : m_salamatrixIconDarkPath.c_str());
        char stableId[512];
        stableId[0] = '\0';
        if (m_szSalamatrixExtensionId[0] != '\0')
            StringCchPrintfA(stableId, _countof(stableId),
                             "ext:%s:%s", m_szSalamatrixExtensionId,
                             commandId);
        toolbarButton.StableId = stableId[0] != '\0' ? stableId : NULL;
        if (SalamanderGeneral == NULL ||
            !SalamanderGeneral->RegisterToolbarButton(&toolbarButton))
            return false;
    }
    ++m_nRuntimeCommands;
    return true;
}

bool CScriptInfo::PublishSalamatrixManifestCommands()
{
    if (m_bSalamatrixManifestCommandsPublished)
        return true;
    if (m_salamatrixManifestCommands.empty())
        return true;

    for (size_t index = 0;
         index < m_salamatrixManifestCommands.size();
         ++index)
    {
        const SALAMATRIX_MANIFEST_COMMAND& manifestCommand =
            m_salamatrixManifestCommands[index];
        bool pluginMenu =
            _stricmp(manifestCommand.Menu.c_str(), "plugin") == 0 ||
            _stricmp(manifestCommand.Menu.c_str(), "both") == 0;
        bool contextMenu =
            _stricmp(manifestCommand.Menu.c_str(), "context") == 0 ||
            _stricmp(manifestCommand.Menu.c_str(), "both") == 0 ||
            manifestCommand.ContextMenu;
        if (_stricmp(manifestCommand.Menu.c_str(), "none") == 0)
        {
            pluginMenu = false;
            contextMenu = false;
        }
        if (!RegisterRuntimeCommand(
                manifestCommand.Id.c_str(),
                manifestCommand.Title.c_str(),
                manifestCommand.Handler.empty()
                    ? NULL
                    : manifestCommand.Handler.c_str(),
                pluginMenu,
                contextMenu,
                manifestCommand.Toolbar,
                0,
                manifestCommand.MenuEventOrMask,
                manifestCommand.MenuEventAndMask,
                manifestCommand.IconPath.empty()
                    ? NULL
                    : manifestCommand.IconPath.c_str(),
                manifestCommand.IconDarkPath.empty()
                    ? NULL
                    : manifestCommand.IconDarkPath.c_str()))
        {
            ReleaseRuntimeCommands();
            return false;
        }
    }
    m_bSalamatrixManifestCommandsPublished = true;
    if (SalamanderGeneral != NULL)
        SalamanderGeneral->PostPluginMenuChanged();
    return true;
}

const char* CScriptInfo::FindRuntimeCommandHandler(
    const char* commandId) const
{
    if (commandId == NULL || commandId[0] == '\0')
        return NULL;
    for (size_t index = 0;
         index < m_salamatrixManifestCommands.size();
         ++index)
    {
        const SALAMATRIX_MANIFEST_COMMAND& command =
            m_salamatrixManifestCommands[index];
        if (_stricmp(command.Id.c_str(), commandId) == 0)
            return command.Handler.empty() ? NULL : command.Handler.c_str();
    }
    for (int index = 0; index < m_nRuntimeCommands; ++index)
    {
        if (_stricmp(m_runtimeCommands[index].Id, commandId) == 0 &&
            m_runtimeCommands[index].Handler[0] != '\0')
            return m_runtimeCommands[index].Handler;
    }
    return NULL;
}

bool CScriptInfo::UnregisterRuntimeCommand(const char* commandId)
{
    if (commandId == NULL)
        return false;
    for (int index = 0; index < m_nRuntimeCommands; ++index)
    {
        if (_stricmp(m_runtimeCommands[index].Id, commandId) != 0)
            continue;
        if (m_runtimeCommands[index].Toolbar && SalamanderGeneral != NULL)
            SalamanderGeneral->UnregisterToolbarButton(
                m_runtimeCommands[index].MenuId);
        for (int move = index; move + 1 < m_nRuntimeCommands; ++move)
            m_runtimeCommands[move] = m_runtimeCommands[move + 1];
        m_runtimeCommands[m_nRuntimeCommands - 1] = RUNTIME_COMMAND_INFO();
        --m_nRuntimeCommands;
        return true;
    }
    return false;
}

void CScriptInfo::ReleaseRuntimeCommands()
{
    if (SalamanderGeneral != NULL)
    {
        for (int index = 0; index < m_nRuntimeCommands; ++index)
        {
            if (m_runtimeCommands[index].Toolbar)
                SalamanderGeneral->UnregisterToolbarButton(
                    m_runtimeCommands[index].MenuId);
        }
    }
    memset(m_runtimeCommands, 0, sizeof(m_runtimeCommands));
    m_nRuntimeCommands = 0;
}

int CScriptInfo::FindRuntimeCommandIndexByMenuId(int menuId) const
{
    for (int index = 0; index < m_nRuntimeCommands; ++index)
    {
        if (m_runtimeCommands[index].MenuId == menuId)
            return index;
    }
    return -1;
}

void CScriptInfo::ReleaseRuntimeCommand()
{
    if (!m_bRuntimeCommandOwned && !m_bSalamatrixManifestCommandsPublished)
        return;
    ReleaseRuntimeCommands();
    m_szRuntimeCommandId[0] = '\0';
    m_bSalamatrixManifestCommandsPublished = false;
    if (!m_salamatrixManifestCommands.empty())
    {
        // Restore the manifest's default command metadata after a persistent
        // worker (which may have added dynamic commands) is torn down.
        LoadSalamatrixManifestMetadata();
    }
    else
    {
        m_szSalamatrixCommandId[0] = _T('\0');
        m_bShowInPluginMenu = false;
        m_bShowInContextMenu = false;
    }
    m_bRuntimeCommandOwned = false;
    if (SalamanderGeneral != NULL)
        SalamanderGeneral->PostPluginMenuChanged();
}

void CScriptInfo::ReleaseRuntimeDialogs()
{
    CAutomationSalamatrixBridge* bridge =
        g_oAutomationPlugin.GetSalamatrixBridge();
    Salamatrix::UI::IUIService* ui =
        bridge != NULL ? bridge->GetUIService() : NULL;
    for (int index = 0; index < m_nRuntimeDialogs; ++index)
    {
        if (m_runtimeDialogs[index].Dialog == NULL)
            continue;
        m_runtimeDialogs[index].Dialog->SetEventCallback(NULL, NULL);
        if (ui != NULL)
            ui->DestroyDialog(m_runtimeDialogs[index].Dialog);
        else
            m_runtimeDialogs[index].Dialog->Release();
        m_runtimeDialogs[index].Dialog = NULL;
        m_runtimeDialogs[index].Id = 0;
    }
    memset(m_runtimeDialogs, 0, sizeof(m_runtimeDialogs));
    m_nRuntimeDialogs = 0;
}

void CScriptInfo::ReleaseRuntimeProgress()
{
    if (m_runtimeProgress.Dialog == NULL)
        return;
    CAutomationSalamatrixBridge* bridge =
        g_oAutomationPlugin.GetSalamatrixBridge();
    Salamatrix::UI::IUIService* ui =
        bridge != NULL ? bridge->GetUIService() : NULL;
    if (ui != NULL)
        ui->DestroyProgressDialog(m_runtimeProgress.Dialog);
    else
        m_runtimeProgress.Dialog->Close();
    memset(&m_runtimeProgress, 0, sizeof(m_runtimeProgress));
}

void CScriptInfo::ReleaseRuntimeSession()
{
    ReleaseRuntimeCommand();
    ReleaseRuntimeDialogs();
    ReleaseRuntimeProgress();
    if (m_pRuntimeSession == NULL)
        return;
    ReleaseRuntimeEventSubscriptions();
    m_pRuntimeSession->Stop();
    if (m_hRuntimePumpThread != NULL)
    {
        // A host callback may be blocked in a modal UI call. Never let
        // extension teardown wait forever; Stop() has already terminated the
        // child process, so the thread is safe to terminate as a last resort.
        DWORD wait = WaitForSingleObject(m_hRuntimePumpThread, 5000);
        if (wait == WAIT_TIMEOUT)
            TerminateThread(m_hRuntimePumpThread, 1);
        CloseHandle(m_hRuntimePumpThread);
        m_hRuntimePumpThread = NULL;
    }
    m_pRuntimeSession->Release();
    m_pRuntimeSession = NULL;
}

void CScriptLookup::PublishSalamatrixExtensions()
{
    CAutomationSalamatrixBridge* bridge =
        g_oAutomationPlugin.GetSalamatrixBridge();
    if (!bridge->HasExtensions())
    {
        g_oAutomationPlugin.RefreshSalamatrixServices();
        bridge = g_oAutomationPlugin.GetSalamatrixBridge();
    }

    Salamatrix::Extensions::IExtensionsService* service =
        bridge->GetExtensionsService();
    if (service == NULL)
        return;
    Salamatrix::Storage::IStorageService* storage =
        bridge->GetStorageService();

    for (int iBin = 0; iBin < _countof(m_apHashBins); iBin++)
    {
        for (CScriptInfo* pScript = m_apHashBins[iBin];
             pScript != NULL;
             pScript = pScript->m_pNextHash)
        {
            const char* extensionId = pScript->GetSalamatrixExtensionId();
            if (extensionId[0] == '\0')
                continue;

            Salamatrix::Extensions::ExtensionDescriptor descriptor;
            memset(&descriptor, 0, sizeof(descriptor));
            descriptor.StructSize = sizeof(descriptor);
            StringCchCopyA(
                descriptor.Id, _countof(descriptor.Id), extensionId);
            NativeToUtf8(
                pScript->GetDisplayName(),
                descriptor.Name,
                _countof(descriptor.Name));
            StringCchCopyA(
                descriptor.RuntimeId,
                _countof(descriptor.RuntimeId),
                pScript->GetSalamatrixRuntimeId());
            NativeToUtf8(
                pScript->GetFileName(),
                descriptor.EntryPoint,
                _countof(descriptor.EntryPoint));
            StringCchCopyA(
                descriptor.IconPath,
                _countof(descriptor.IconPath),
                pScript->GetSalamatrixIconPath());
            StringCchCopyA(
                descriptor.IconDarkPath,
                _countof(descriptor.IconDarkPath),
                pScript->GetSalamatrixIconDarkPath());
            descriptor.Flags = Salamatrix::Extensions::ExtensionFlagManifest |
                               Salamatrix::Extensions::ExtensionFlagPersistent;
            CAutomationSalamatrixBridge* availabilityBridge =
                g_oAutomationPlugin.GetSalamatrixBridge();
            Salamatrix::Runtime::IRuntimeService* runtimeService =
                availabilityBridge != NULL
                    ? availabilityBridge->GetRuntimeService()
                    : NULL;
            Salamatrix::Runtime::IRuntimeAdapter* runtimeAdapter =
                runtimeService != NULL
                    ? runtimeService->FindAdapter(
                          pScript->GetSalamatrixRuntimeId(),
                          pScript->GetSalamatrixMinimumRuntimeVersion())
                    : NULL;
            if (runtimeAdapter == NULL || !runtimeAdapter->IsAvailable())
                descriptor.Flags |=
                    Salamatrix::Extensions::ExtensionFlagRuntimeUnavailable;

            BOOL missingDependency = FALSE;
            for (size_t dependencyIndex = 0;
                 dependencyIndex < pScript->m_salamatrixDependencies.size();
                 ++dependencyIndex)
            {
                Salamatrix::Extensions::ExtensionInfo dependencyInfo;
                if (!service->FindExtension(
                        pScript->m_salamatrixDependencies[dependencyIndex].c_str(),
                        &dependencyInfo) ||
                    (dependencyInfo.State !=
                         Salamatrix::Extensions::ExtensionStateActive &&
                     dependencyInfo.State !=
                         Salamatrix::Extensions::ExtensionStateActivating))
                {
                    missingDependency = TRUE;
                    break;
                }
            }
            if (missingDependency)
                descriptor.Flags |=
                    Salamatrix::Extensions::ExtensionFlagDependencyUnavailable;

            // Materialize declared defaults before activation. Existing user
            // values always win, so re-discovery never resets an extension's
            // settings.
            pScript->InitializeSalamatrixSettings(storage);

            // Plugin Manager persists the user's enabled/disabled choice in
            // the extension's isolated namespace. Absence deliberately means
            // enabled so existing packages keep their current behaviour.
            if (storage != NULL &&
                storage->GetValueType(
                    extensionId, "salamatrix.enabled") ==
                    Salamatrix::Storage::StorageValueBoolean)
            {
                BOOL enabled = TRUE;
                if (storage->GetBoolean(
                        extensionId, "salamatrix.enabled", &enabled) &&
                    !enabled)
                {
                    descriptor.Flags |=
                        Salamatrix::Extensions::ExtensionFlagDisabled;
                }
            }

            Salamatrix::Extensions::ExtensionSettingInfo settingSchema[64];
            int settingCount = 0;
            for (size_t settingIndex = 0;
                 settingIndex < pScript->m_salamatrixSettings.size() &&
                 settingCount < _countof(settingSchema);
                 ++settingIndex)
            {
                const CExtensionManifestSetting& setting =
                    pScript->m_salamatrixSettings[settingIndex];
                Salamatrix::Extensions::ExtensionSettingInfo& published =
                    settingSchema[settingCount++];
                StringCchCopyA(
                    published.Key, _countof(published.Key),
                    setting.Key.c_str());
                published.Type =
                    setting.Type == ExtensionManifestSettingInteger
                        ? Salamatrix::Extensions::ExtensionSettingInteger
                        : setting.Type == ExtensionManifestSettingBoolean
                              ? Salamatrix::Extensions::ExtensionSettingBoolean
                              : Salamatrix::Extensions::ExtensionSettingString;
            }

            // A failed registration (for example a duplicate manifest id)
            // is intentionally ignored here. The host registry remains
            // authoritative and malformed/duplicate entries never become
            // executable by accident. Activation is performed in the
            // dependency-resolution pass below, after every manifest in this
            // refresh is visible. Missing runtimes and dependencies remain
            // explicit waiting states and are retried on the next refresh.
            if (service->RegisterExtension(
                    &descriptor,
                    CScriptInfo::RuntimeLifecycleCallback,
                    pScript))
            {
                service->SetExtensionSettingsSchema(
                    extensionId, settingSchema, settingCount);
            }
            // Activation is deferred until every manifest has been
            // registered, allowing dependencies that appear later in the
            // discovery order to resolve in the same refresh.
        }
    }

    // Resolve dependency chains after the complete registry is visible. A
    // bounded number of passes is sufficient for the registry's 256-entry
    // capacity and prevents malformed cycles from spinning forever.
    for (int pass = 0; pass < 256; ++pass)
    {
        BOOL changed = FALSE;
        for (int iBin = 0; iBin < _countof(m_apHashBins); iBin++)
        {
            for (CScriptInfo* pScript = m_apHashBins[iBin];
                 pScript != NULL;
                 pScript = pScript->m_pNextHash)
            {
                const char* extensionId =
                    pScript->GetSalamatrixExtensionId();
                if (extensionId[0] == '\0')
                    continue;

                Salamatrix::Extensions::ExtensionInfo info;
                if (!service->FindExtension(extensionId, &info))
                    continue;
                BOOL missingDependency = FALSE;
                for (size_t dependencyIndex = 0;
                     dependencyIndex < pScript->m_salamatrixDependencies.size();
                     ++dependencyIndex)
                {
                    Salamatrix::Extensions::ExtensionInfo dependencyInfo;
                    if (!service->FindExtension(
                            pScript->m_salamatrixDependencies[dependencyIndex].c_str(),
                            &dependencyInfo) ||
                        (dependencyInfo.State !=
                             Salamatrix::Extensions::ExtensionStateActive &&
                         dependencyInfo.State !=
                             Salamatrix::Extensions::ExtensionStateActivating))
                    {
                        missingDependency = TRUE;
                        break;
                    }
                }
                if (missingDependency)
                    continue;

                if ((info.Descriptor.Flags &
                     Salamatrix::Extensions::ExtensionFlagDependencyUnavailable) != 0)
                {
                    Salamatrix::Extensions::ExtensionDescriptor refreshed =
                        info.Descriptor;
                    refreshed.Flags &=
                        ~Salamatrix::Extensions::ExtensionFlagDependencyUnavailable;
                    if (service->RegisterExtension(
                            &refreshed,
                            CScriptInfo::RuntimeLifecycleCallback,
                            pScript))
                        changed = TRUE;
                }
                if (info.State != Salamatrix::Extensions::ExtensionStateActive &&
                    info.State != Salamatrix::Extensions::ExtensionStateActivating &&
                    info.State != Salamatrix::Extensions::ExtensionStateWaitingForRuntime)
                {
                    Salamatrix::Extensions::ExtensionState stateBefore =
                        info.State;
                    if (service->ActivateExtension(extensionId))
                    {
                        Salamatrix::Extensions::ExtensionInfo after;
                        if (service->FindExtension(extensionId, &after) &&
                            after.State != stateBefore)
                            changed = TRUE;
                    }
                }
            }
        }
        if (!changed)
            break;
    }
}

void CScriptLookup::MarkAllScriptsDirty()
{
    for (int iBin = 0; iBin < _countof(m_apHashBins); iBin++)
    {
        for (CScriptInfo* pIter = m_apHashBins[iBin];
             pIter != NULL;
             pIter = pIter->m_pNextHash)
        {
            pIter->SetDirty();
        }
    }
}

void CScriptLookup::RemoveDirtyScripts()
{
    for (int iBin = 0; iBin < _countof(m_apHashBins); iBin++)
    {
    restart:
        CScriptInfo* pIter = m_apHashBins[iBin];
        CScriptInfo* pPrev = NULL;

        while (pIter != NULL)
        {
            CScriptInfo* pNext = pIter->m_pNextHash;

            if (pIter->IsDirty())
            {
                // unlink from hash map
                if (pPrev)
                {
                    pPrev->m_pNextHash = pNext;
                }
                else
                {
                    m_apHashBins[iBin] = pNext;
                }

                // unlink from container
                UnlinkScript(pIter);
                m_bModified = true;

                delete pIter;

                goto restart;
            }

            pPrev = pIter;
            pIter = pNext;
        }
    }
}

void CScriptLookup::RemoveEmptyContainers(CScriptContainer* pContainer)
{
    _ASSERTE(pContainer != NULL);

    if (pContainer->m_pChild != NULL)
    {
        RemoveEmptyContainers(pContainer->m_pChild);
    }

    while (pContainer != NULL)
    {
        CScriptContainer* pNext = pContainer->m_pSibling;

        if (pContainer->m_pChild == NULL && pContainer->m_pScripts == NULL)
        {
            if (pContainer != m_pRootContainer)
            {
                UnlinkContainer(pContainer);
                delete pContainer;
            }
        }

        pContainer = pNext;
    }
}

CScriptInfo* CScriptLookup::LookupScriptByPath(
    UINT nHash,
    PCTSTR pszFullPath)
{
    _ASSERTE(nHash != 0);

    int iBin = HashFromId(nHash) % _countof(m_apHashBins);
    for (CScriptInfo* pIter = m_apHashBins[iBin];
         pIter != NULL;
         pIter = pIter->m_pNextHash)
    {
        if (_tcsicmp(pIter->GetFileName(), pszFullPath) == 0)
        {
            return pIter;
        }
    }

    return NULL;
}
