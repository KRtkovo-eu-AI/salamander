// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include <tchar.h>
#include "splitcbn.rh"
#include "splitcbn.rh2"
#include "lang\lang.rh"
#include "splitcbn.h"
#include "split.h"
#include "combine.h"
#include "dialogs.h"

// ****************************************************************************

HINSTANCE DLLInstance = NULL; // handle to SPL - language-independent resources
HINSTANCE HLanguage = NULL;   // handle to SLG - language-dependent resources

// plugin interface instance invoked directly by Salamander
CPluginInterface PluginInterface;
// portion of CPluginInterface that drives the extensions menu
CPluginInterfaceForMenuExt InterfaceForMenuExt;
// general Salamander interface, valid from startup until the plugin shuts down
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
// interface offering convenient file-handling helpers
CSalamanderSafeFileAbstract* SalamanderSafeFile = NULL;
// interface providing Salamander-specific custom Windows controls
CSalamanderGUIAbstract* SalamanderGUI = NULL;
// SalamanderDebug instance shared with "dbg.h"
CSalamanderDebugAbstract* SalamanderDebug = NULL;

BOOL configIncludeFileExt;
BOOL configCreateBatchFile;
BOOL configSplitToOther;
BOOL configCombineToOther;
BOOL configSplitToSubdir;

namespace
{
HBRUSH SplitCBNDarkModeDialogBrush = NULL;
COLORREF SplitCBNDarkModeDialogBrushColor = CLR_INVALID;
BOOL SplitCBNHostPolicyKnown = FALSE;
BOOL SplitCBNHostUseWindowsDarkMode = FALSE;
COLORREF SplitCBNHostSchemeText = CLR_INVALID;
COLORREF SplitCBNHostSchemeBackground = CLR_INVALID;
DWORD SplitCBNMainThreadId = 0;

HBRUSH SplitCBNGetDarkModeDialogBrush(COLORREF background)
{
    if (SplitCBNDarkModeDialogBrush == NULL || SplitCBNDarkModeDialogBrushColor != background)
    {
        if (SplitCBNDarkModeDialogBrush != NULL)
            DeleteObject(SplitCBNDarkModeDialogBrush);
        SplitCBNDarkModeDialogBrush = CreateSolidBrush(background);
        SplitCBNDarkModeDialogBrushColor = background;
    }
    return SplitCBNDarkModeDialogBrush;
}

void ReleaseSplitCBNDarkModeResources()
{
    if (SplitCBNDarkModeDialogBrush != NULL)
    {
        DeleteObject(SplitCBNDarkModeDialogBrush);
        SplitCBNDarkModeDialogBrush = NULL;
        SplitCBNDarkModeDialogBrushColor = CLR_INVALID;
    }
}

BOOL SplitCBNCanQueryHostDarkMode()
{
    return SalamanderGeneral != NULL && SplitCBNMainThreadId != 0 && GetCurrentThreadId() == SplitCBNMainThreadId;
}

BOOL SplitCBNShouldUseWindowsDarkMode()
{
    return SplitCBNHostPolicyKnown && SplitCBNHostUseWindowsDarkMode;
}
}

void RefreshSplitCBNDarkModeFromHost()
{
    if (!SplitCBNCanQueryHostDarkMode())
        return;

    BOOL useWindowsDarkMode = FALSE;
    if (SalamanderGeneral->GetConfigParameter(SALCFG_USEWINDOWSDARKMODE,
                                              &useWindowsDarkMode,
                                              sizeof(useWindowsDarkMode),
                                              NULL))
    {
        SplitCBNHostPolicyKnown = TRUE;
        SplitCBNHostUseWindowsDarkMode = useWindowsDarkMode;
    }

    if (SplitCBNHostPolicyKnown && SplitCBNHostUseWindowsDarkMode)
    {
        SplitCBNHostSchemeText = SalamanderGeneral->GetCurrentColor(SALCOL_ITEM_FG_NORMAL);
        SplitCBNHostSchemeBackground = SalamanderGeneral->GetCurrentColor(SALCOL_ITEM_BK_NORMAL);
    }
}

void ConfigureSplitCBNDarkModeFromHost()
{
    RefreshSplitCBNDarkModeFromHost();

    const BOOL useWindowsDarkMode = SplitCBNShouldUseWindowsDarkMode();
    const COLORREF fallbackText = GetSysColor(COLOR_BTNTEXT);
    const COLORREF fallbackBackground = GetSysColor(COLOR_BTNFACE);
    COLORREF text = fallbackText;
    COLORREF background = fallbackBackground;

    if (useWindowsDarkMode && SplitCBNHostSchemeText != CLR_INVALID && SplitCBNHostSchemeBackground != CLR_INVALID)
    {
        text = SplitCBNHostSchemeText;
        background = SplitCBNHostSchemeBackground;
    }

    const COLORREF readableText = DarkModeEnsureReadableForeground(text, background);
    HBRUSH dialogBrush = SplitCBNGetDarkModeDialogBrush(background);
    DarkModeSetConfiguredColors(text, background, fallbackText, fallbackBackground);
    DarkModeConfigureDialogColors(readableText, background, dialogBrush);
    DarkModeSetEnabled(useWindowsDarkMode != FALSE);
}

void ApplySplitCBNDarkMode(HWND hwnd)
{
    ConfigureSplitCBNDarkModeFromHost();
    if (hwnd != NULL)
    {
        DarkModeApplyWindow(hwnd);
        DarkModeRefreshTitleBar(hwnd);
        DarkModeApplyTree(hwnd);
    }
}

BOOL HandleSplitCBNDarkCtlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, INT_PTR* result)
{
    if (!SplitCBNShouldUseWindowsDarkMode())
        return FALSE;

    ConfigureSplitCBNDarkModeFromHost();
    LRESULT brush = 0;
    if (DarkModeHandleCtlColor(uMsg, wParam, lParam, brush))
    {
        *result = (INT_PTR)brush;
        return TRUE;
    }
    return FALSE;
}


static const char* KEY_INCLUDEFILEEXT = "Include Original Extension";
static const char* KEY_CREATEBATCHFILE = "Create Batch File";
static const char* KEY_SPLITTOOTHER = "Split To Other Panel";
static const char* KEY_COMBINETOOTHER = "Combine To Other Panel";
static const char* KEY_SPLITTOSUBDIR = "Split To Subdirectory";

// ****************************************************************************

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DLLInstance = hinstDLL;
        InitCommonControls();
    }
    return TRUE; // DLL can be loaded
}

char* LoadStr(int resID)
{
    return SalamanderGeneral->LoadStr(HLanguage, resID);
}

//****************************************************************************

int WINAPI SalamanderPluginGetReqVer()
{
    return LAST_VERSION_OF_SALAMANDER;
}

// ****************************************************************************

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    // set SalamanderDebug for "dbg.h"
    SalamanderDebug = salamander->GetSalamanderDebug();

    CALL_STACK_MESSAGE1("SalamanderPluginEntry()");

    // this plugin is built for the current version of Salamander and newer - perform a check
    if (salamander->GetVersion() < LAST_VERSION_OF_SALAMANDER)
    { // reject older versions
        MessageBox(salamander->GetParentWindow(),
                   REQUIRE_LAST_VERSION_OF_SALAMANDER,
                   "Split & Combine" /* do not translate! */, MB_OK | MB_ICONERROR);
        return NULL;
    }

    // ask Salamander to load the language module (.slg)
    HLanguage = salamander->LoadLanguageModule(salamander->GetParentWindow(), "Split & Combine" /* do not translate! */);
    if (HLanguage == NULL)
        return NULL;

    // obtain the general Salamander interface
    SalamanderGeneral = salamander->GetSalamanderGeneral();
    SplitCBNMainThreadId = GetCurrentThreadId();
    RefreshSplitCBNDarkModeFromHost();
    SalamanderSafeFile = salamander->GetSalamanderSafeFile();
    // obtain the interface providing customized Windows controls used in Salamander
    SalamanderGUI = salamander->GetSalamanderGUI();

    // set the help file name
    SalamanderGeneral->SetHelpFileName("splitcbn.chm");

    // set the basic information about the plugin
    salamander->SetBasicPluginData(LoadStr(IDS_PLUGINNAME),
                                   FUNCTION_CONFIGURATION | FUNCTION_LOADSAVECONFIGURATION,
                                   VERSINFO_VERSION_NO_PLATFORM,
                                   VERSINFO_COPYRIGHT,
                                   LoadStr(IDS_PLUGIN_DESCRIPTION),
                                   "SplitCombine");

    salamander->SetPluginHomePageURL("www.altap.cz");

    return &PluginInterface;
}

// ****************************************************************************
//
//  CPluginInterface
//
BOOL CPluginInterface::Release(HWND parent, BOOL force)
{
    CALL_STACK_MESSAGE2("CPluginInterface::Release(, %d)", force);
    (void)parent;
    ReleaseSplitCBNDarkModeResources();
    return TRUE;
}



void CPluginInterface::About(HWND parent)
{
    char buf[1000];
    _snprintf_s(buf, _TRUNCATE,
                "%s " VERSINFO_VERSION "\n\n" VERSINFO_COPYRIGHT "\n\n"
                "%s",
                LoadStr(IDS_PLUGINNAME),
                LoadStr(IDS_PLUGIN_DESCRIPTION));
    SalamanderGeneral->SalMessageBox(parent, buf, LoadStr(IDS_ABOUTTITLE), MB_OK | MB_ICONINFORMATION);
}

void CPluginInterface::LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{
    CALL_STACK_MESSAGE1("CPluginInterface::LoadConfiguration(, ,)");
    configIncludeFileExt = TRUE;
    configCreateBatchFile = TRUE;
    SalamanderGeneral->GetConfigParameter(SALCFG_ARCOTHERPANELFORUNPACK, &configSplitToOther, sizeof(BOOL), NULL);
    SalamanderGeneral->GetConfigParameter(SALCFG_ARCOTHERPANELFORPACK, &configCombineToOther, sizeof(BOOL), NULL);
    SalamanderGeneral->GetConfigParameter(SALCFG_ARCSUBDIRBYARCFORUNPACK, &configSplitToSubdir, sizeof(BOOL), NULL);
    if (regKey != NULL)
    {
        registry->GetValue(regKey, KEY_INCLUDEFILEEXT, REG_DWORD, &configIncludeFileExt, sizeof(DWORD));
        registry->GetValue(regKey, KEY_CREATEBATCHFILE, REG_DWORD, &configCreateBatchFile, sizeof(DWORD));
        registry->GetValue(regKey, KEY_SPLITTOOTHER, REG_DWORD, &configSplitToOther, sizeof(DWORD));
        registry->GetValue(regKey, KEY_COMBINETOOTHER, REG_DWORD, &configCombineToOther, sizeof(DWORD));
        registry->GetValue(regKey, KEY_SPLITTOSUBDIR, REG_DWORD, &configSplitToSubdir, sizeof(DWORD));
    }
    //if (!configSplitToOther) configSplitToSubdir = FALSE;
}

void CPluginInterface::SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{
    CALL_STACK_MESSAGE1("CPluginInterface::SaveConfiguration(, ,)");
    registry->SetValue(regKey, KEY_INCLUDEFILEEXT, REG_DWORD, &configIncludeFileExt, sizeof(DWORD));
    registry->SetValue(regKey, KEY_CREATEBATCHFILE, REG_DWORD, &configCreateBatchFile, sizeof(DWORD));
    registry->SetValue(regKey, KEY_SPLITTOOTHER, REG_DWORD, &configSplitToOther, sizeof(DWORD));
    registry->SetValue(regKey, KEY_COMBINETOOTHER, REG_DWORD, &configCombineToOther, sizeof(DWORD));
    registry->SetValue(regKey, KEY_SPLITTOSUBDIR, REG_DWORD, &configSplitToSubdir, sizeof(DWORD));
}

void CPluginInterface::Configuration(HWND parent)
{
    CALL_STACK_MESSAGE1("CPluginInterface::Configuration( )");
    ConfigDialog(parent);
}

void CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    CALL_STACK_MESSAGE1("CPluginInterface::Connect(,)");

    /* used by the script export_mnu.py, which generates salmenu.mnu for Translator
   keep synchronized with the salamander->AddMenuItem() calls below...
MENU_TEMPLATE_ITEM PluginMenu[] = 
{
	{MNTT_PB, 0
	{MNTT_IT, IDS_MENU1
	{MNTT_IT, IDS_MENU2
	{MNTT_PE, 0
};
*/

    salamander->AddMenuItem(-1, LoadStr(IDS_MENU1), 0, 1, FALSE, MENU_EVENT_TRUE,
                            MENU_EVENT_FILE_FOCUSED | MENU_EVENT_DISK, MENU_SKILLLEVEL_ALL);
    salamander->AddMenuItem(-1, LoadStr(IDS_MENU2), 0, 2, FALSE, MENU_EVENT_FILES_SELECTED | MENU_EVENT_FILE_FOCUSED, MENU_EVENT_DISK, MENU_SKILLLEVEL_ALL);

    // set the plugin icon
    HBITMAP hBmp = (HBITMAP)LoadImage(DLLInstance, MAKEINTRESOURCE(IDB_SPLIT),
                                      IMAGE_BITMAP, 16, 16, LR_DEFAULTCOLOR);
    salamander->SetBitmapWithIcons(hBmp);
    DeleteObject(hBmp);
    salamander->SetPluginIcon(0);
    salamander->SetPluginMenuAndToolbarIcon(0);
}

CPluginInterfaceForMenuExtAbstract*
CPluginInterface::GetInterfaceForMenuExt()
{
    return &InterfaceForMenuExt;
}

// ****************************************************************************
//
//  CPluginInterfaceForMenuExt
//

BOOL CPluginInterfaceForMenuExt::ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander,
                                                 HWND parent, int id, DWORD eventMask)
{
    CALL_STACK_MESSAGE3("CPluginInterfaceForMenuExt::ExecuteMenuItem( , , %ld, %X)", id, eventMask);

    SalamanderGeneral->SetUserWorkedOnPanelPath(PANEL_SOURCE); // treat all commands as working with the path (shown in Alt+F12)

    switch (id)
    {
    case 1:
    {
        return SplitCommand(parent, salamander);
    }

    case 2:
    {
        return CombineCommand(eventMask, parent, salamander);
    }
    }
    return FALSE;
}

BOOL CPluginInterfaceForMenuExt::HelpForMenuItem(HWND parent, int id)
{
    int helpID = 0;
    switch (id)
    {
    case 1:
        helpID = IDH_SPLIT;
        break;
    case 2:
        helpID = IDH_COMBINE;
        break;
    }
    if (helpID != 0)
        SalamanderGeneral->OpenHtmlHelp(parent, HHCDisplayContext, helpID, FALSE);
    return helpID != 0;
}

// ****************************************************************************
//
//  Helper functions
//

void CenterWindow(HWND hWnd)
{
    CALL_STACK_MESSAGE1("CenterWindow()");
    HWND hParent = GetParent(hWnd);
    if (hParent != NULL)
        SalamanderGeneral->MultiMonCenterWindow(hWnd, hParent, TRUE);
}

void GetInfo(char* buffer, CQuadWord& size)
{
    CALL_STACK_MESSAGE2("GetInfo(, %I64u)", size.Value);
    SYSTEMTIME st;
    GetLocalTime(&st);

    char date[50], time[50], number[50];
    if (GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, time, 50) == 0)
        sprintf(time, "%u:%02u:%02u", st.wHour, st.wMinute, st.wSecond);
    if (GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, date, 50) == 0)
        sprintf(date, "%u.%u.%u", st.wDay, st.wMonth, st.wYear);
    sprintf(buffer, "%s, %s, %s", SalamanderGeneral->NumberToStr(number, size), date, time);
}

void StripExtension(LPTSTR fileName)
{
    CALL_STACK_MESSAGE2("StripExtension(%s)", fileName);
    LPTSTR dot = _tcsrchr(fileName, '.');
    if (dot != NULL)
        *dot = 0; // ".cvspass" is treated as an extension in Windows
}

BOOL Error(int title, int error, ...)
{
    int lastErr = GetLastError();
    CALL_STACK_MESSAGE3("Error(%d, %d, ...)", title, error);
    std::vector<char> buffer;
    va_list arglist;
    va_start(arglist, error);
    const int length = _vscprintf(LoadStr(error), arglist);
    if (length < 0)
    {
        va_end(arglist);
        return FALSE;
    }
    buffer.resize(static_cast<size_t>(length) + 2048);
    char* buf = buffer.data();
    vsprintf_s(buf, buffer.size(), LoadStr(error), arglist);
    va_end(arglist);
    if (lastErr != ERROR_SUCCESS)
    {
        strcat(buf, " ");
        DWORD l = (DWORD)strlen(buf);
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, lastErr,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf + l, static_cast<DWORD>(buffer.size() - l), NULL);
    }
    SalamanderGeneral->ShowMessageBox(buf, LoadStr(title), MSGBOX_ERROR);

    return FALSE;
}

BOOL Error2(HWND hParent, int title, int error, ...)
{
    int lastErr = GetLastError();
    CALL_STACK_MESSAGE3("Error2( , %d, %d, ...)", title, error);
    std::vector<char> buffer;
    va_list arglist;
    va_start(arglist, error);
    const int length = _vscprintf(LoadStr(error), arglist);
    if (length < 0)
    {
        va_end(arglist);
        return FALSE;
    }
    buffer.resize(static_cast<size_t>(length) + 2048);
    char* buf = buffer.data();
    vsprintf_s(buf, buffer.size(), LoadStr(error), arglist);
    va_end(arglist);
    if (lastErr != ERROR_SUCCESS)
    {
        strcat(buf, " ");
        DWORD l = (DWORD)strlen(buf);
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, lastErr,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf + l, static_cast<DWORD>(buffer.size() - l), NULL);
    }
    SalamanderGeneral->SalMessageBox(hParent, buf, LoadStr(title), MSGBOXEX_OK | MSGBOXEX_ICONEXCLAMATION);

    return FALSE;
}

std::string GetTargetDir(const char* sourceDirectory, const char* subdirName, BOOL bSplit)
{
    std::string target = sourceDirectory;
    if (bSplit ? configSplitToOther : configCombineToOther)
    {
        SplitCBNPaths::Buffer panelPath(SplitCBNPaths::Capacity);
        int type = 0;
        if (SalamanderGeneral->GetPanelPath(PANEL_TARGET, panelPath.data(), static_cast<int>(panelPath.size()), &type, NULL) &&
            type == PATH_TYPE_WINDOWS)
            target = panelPath.data();
    }
    if (bSplit && configSplitToSubdir && subdirName != NULL)
        target = SplitCBNPaths::Join(target, SplitCBNPaths::Stem(subdirName));
    return target;
}

BOOL MakePathAbsolute(std::string& path, BOOL pathIsDir, const char* absRoot, BOOL activePreferred, int errorTitle)
{
    SalamanderGeneral->SalUpdateDefaultDir(activePreferred);
    std::string absolute;
    if (!SplitCBNPaths::Absolute(path, absRoot, absolute))
        return Error(errorTitle, IDS_PATHERROR);
    std::string directory = pathIsDir ? absolute : SplitCBNPaths::Parent(absolute);
    bool missing = false;
    for (;;)
    {
        const DWORD attributes = GetFileAttributesW(SplitCBNPaths::ApiPath(directory.c_str()).c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES)
        {
            if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                SetLastError(ERROR_DIRECTORY);
                return Error(errorTitle, IDS_WINPATH);
            }
            break;
        }
        const DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
            return Error(errorTitle, IDS_PATHERROR);
        missing = true;
        const std::string parent = SplitCBNPaths::Parent(directory);
        if (parent.empty() || parent == directory)
            return Error(errorTitle, IDS_PATHERROR);
        directory = parent;
    }
    if (missing && SalamanderGeneral->SalMessageBox(SalamanderGeneral->GetMsgBoxParent(),
              LoadStr(IDS_TARGETPATHEXIST), LoadStr(errorTitle), MB_YESNO | MB_ICONQUESTION) == IDNO)
        return FALSE;
    path.swap(absolute);
    return TRUE;
}

BOOL TestTargetSpace(HWND parent, const char* path, const CQuadWord& size, int title)
{
    CQuadWord freeSpace;
    SplitCBNPaths::DiskSpace(&freeSpace, path);
    if (freeSpace != CQuadWord(-1, -1) && freeSpace < size)
        return SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_OUTOFSPACE), LoadStr(title),
                                                MB_YESNO | MB_ICONQUESTION) == IDYES;
    return TRUE;
}
