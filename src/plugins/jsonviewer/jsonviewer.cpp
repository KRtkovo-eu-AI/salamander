// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "jsonviewer.h"

CPluginInterface PluginInterface;
CPluginInterfaceForViewer InterfaceForViewer;

const char* PluginNameEN = "JsonView";
const char* PluginNameShort = "JSONVIEW";

HINSTANCE DLLInstance = NULL;
HINSTANCE HLanguage = NULL;

CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
CSalamanderGUIAbstract* SalamanderGUI = NULL;
CSalamanderDebugAbstract* SalamanderDebug = NULL;
int SalamanderVersion = 0;

static CRITICAL_SECTION ViewerSection;
static bool ViewerSectionInitialized = false;
static std::vector<HWND> OpenViewers;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DLLInstance = hinstDLL;

        INITCOMMONCONTROLSEX icc{};
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES;
        if (!InitCommonControlsEx(&icc))
        {
            MessageBox(NULL, "InitCommonControlsEx failed!", "JsonView", MB_OK | MB_ICONERROR);
            return FALSE;
        }
    }

    return TRUE;
}

char* LoadStr(int resID)
{
    return SalamanderGeneral->LoadStr(HLanguage, resID);
}

BOOL InitViewer()
{
    if (!ViewerSectionInitialized)
    {
        InitializeCriticalSection(&ViewerSection);
        ViewerSectionInitialized = true;
    }
    return TRUE;
}

void ReleaseViewer()
{
    if (!ViewerSectionInitialized)
        return;

    std::vector<HWND> windows;
    EnterCriticalSection(&ViewerSection);
    windows = OpenViewers;
    LeaveCriticalSection(&ViewerSection);

    for (HWND hwnd : windows)
    {
        if (IsWindow(hwnd))
            PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
}

void RegisterViewerWindow(HWND hwnd)
{
    if (!ViewerSectionInitialized)
        return;
    EnterCriticalSection(&ViewerSection);
    OpenViewers.push_back(hwnd);
    LeaveCriticalSection(&ViewerSection);
}

void UnregisterViewerWindow(HWND hwnd)
{
    if (!ViewerSectionInitialized)
        return;
    EnterCriticalSection(&ViewerSection);
    OpenViewers.erase(std::remove(OpenViewers.begin(), OpenViewers.end(), hwnd), OpenViewers.end());
    LeaveCriticalSection(&ViewerSection);
}

void WINAPI
CPluginInterface::About(HWND parent)
{
    char buffer[512];
    _snprintf_s(buffer, _TRUNCATE,
                "%s\n\n%s",
                LoadStr(IDS_PLUGINNAME),
                LoadStr(IDS_PLUGIN_DESCRIPTION));
    SalamanderGeneral->SalMessageBox(parent, buffer, LoadStr(IDS_ABOUT), MB_OK | MB_ICONINFORMATION);
}

BOOL WINAPI
CPluginInterface::Release(HWND parent, BOOL force)
{
    ReleaseViewer();
    return TRUE;
}

void WINAPI
CPluginInterface::LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{
    // no persistent configuration yet
}

void WINAPI
CPluginInterface::SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{
    // no persistent configuration yet
}

void WINAPI
CPluginInterface::Configuration(HWND parent)
{
    SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_NO_CONFIGURATION), LoadStr(IDS_PLUGINNAME),
                                     MB_OK | MB_ICONINFORMATION);
}

void WINAPI
CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    salamander->AddViewer("*.json", FALSE);
    salamander->AddViewer("*.json5", FALSE);
}

void WINAPI
CPluginInterface::ClearHistory(HWND parent)
{
    // nothing to clear for now
}

CPluginInterfaceForViewerAbstract* WINAPI
CPluginInterface::GetInterfaceForViewer()
{
    return &InterfaceForViewer;
}

BOOL WINAPI
CPluginInterfaceForViewer::CanViewFile(const char* name)
{
    if (name == NULL)
        return FALSE;

    const char* ext = strrchr(name, '.');
    if (!ext)
        return FALSE;

    if (_stricmp(ext, ".json") == 0 || _stricmp(ext, ".json5") == 0)
        return TRUE;

    return FALSE;
}

BOOL WINAPI
CPluginInterfaceForViewer::ViewFile(const char* name, int left, int top, int width, int height,
                                    UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock,
                                    BOOL* lockOwner, CSalamanderPluginViewerData* viewerData,
                                    int enumFilesSourceUID, int enumFilesCurrentIndex)
{
    std::unique_ptr<CJsonViewerWindow> window(new CJsonViewerWindow());
    if (!window->Create(name, left, top, width, height, showCmd, alwaysOnTop, returnLock, lock, lockOwner))
        return FALSE;

    window.release();
    return TRUE;
}

void WINAPI
CPluginInterface::Event(int event, DWORD param)
{
    // no special events handled
}

int WINAPI SalamanderPluginGetReqVer()
{
    return LAST_VERSION_OF_SALAMANDER;
}

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    SalamanderDebug = salamander->GetSalamanderDebug();
    SalamanderVersion = salamander->GetVersion();
    HANDLES_CAN_USE_TRACE();

    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
    {
        MessageBox(salamander->GetParentWindow(), REQUIRE_LAST_VERSION_OF_SALAMANDER, PluginNameEN,
                   MB_OK | MB_ICONERROR);
        return NULL;
    }

    HLanguage = salamander->LoadLanguageModule(salamander->GetParentWindow(), PluginNameEN);
    if (HLanguage == NULL)
        return NULL;

    SalamanderGeneral = salamander->GetSalamanderGeneral();
    SalamanderGUI = salamander->GetSalamanderGUI();

    if (!InitViewer())
        return NULL;

    return &PluginInterface;
}

