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

// objekt interfacu pluginu, jeho metody se volaji ze Salamandera
CPluginInterface PluginInterface;
// cast interfacu CPluginInterface pro viewer
CPluginInterfaceForViewer InterfaceForViewer;

// globalni data
const char* PluginNameEN = "JSON Viewer .NET"; // neprekladane jmeno pluginu
const char* PluginNameShort = "JSONVIEWER";    // jmeno pluginu (kratce, bez mezer)

HINSTANCE DLLInstance = NULL; // handle k SPL-ku - jazykove nezavisle resourcy
HINSTANCE HLanguage = NULL;   // handle k SLG-cku - jazykove zavisle resourcy

// obecne rozhrani Salamandera - platne od startu az do ukonceni pluginu
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
CSalamanderGUIAbstract* SalamanderGUI = NULL;

// definice promenne pro "dbg.h"
CSalamanderDebugAbstract* SalamanderDebug = NULL;

// maximum file size (in bytes) allowed for the managed viewer
static const ULONGLONG kMaxJsonFileSize = 8ULL * 1024ULL * 1024ULL; // 8 MB

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

static bool IsFileTooLarge(const char* path, ULONGLONG limit)
{
    if (path == NULL || path[0] == '\0')
        return false;

    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &attrs))
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

    salamander->AddViewer("*.json;*.pc;*.jbeam", FALSE);

    if (SalamanderGUI != NULL)
    {
        CGUIIconListAbstract* iconList = SalamanderGUI->CreateIconList();
        if (iconList != NULL)
        {
            if (iconList->Create(16, 16, 1))
            {
                UINT loadFlags = SalamanderGeneral != NULL ? SalamanderGeneral->GetIconLRFlags() : LR_DEFAULTCOLOR;
                HICON icon16 = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_JSONVIEWER), IMAGE_ICON, 16, 16, loadFlags);
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

//
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

    if (IsFileTooLarge(name, kMaxJsonFileSize))
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

        if (!ManagedBridge_ViewJsonFile(parent, name, placement, showCmd, alwaysOnTop, fileLock, true))
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

    return ManagedBridge_ViewJsonFile(parent, name, placement, showCmd, alwaysOnTop, NULL, false);
}

BOOL WINAPI CPluginInterfaceForViewer::CanViewFile(const char* name)
{
    if (name == NULL)
        return FALSE;

    const char* extension = strrchr(name, '.');
    if (extension == NULL)
        return FALSE;

    if (_stricmp(extension, ".json") == 0 || _stricmp(extension, ".pc") == 0 ||
        _stricmp(extension, ".jbeam") == 0)
        return TRUE;

    return FALSE;
}
