// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Declare a force-release path that still joins the native worker. | Avoid unmapping Samandarin code while a canceled worker is executing. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Declare native process-close recovery without changing the SDK ABI. | Restore updater service after a global unload is refused. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Replace managed updater entry points with native updater APIs. | Keep plugin settings and lifecycle in C++ without changing the Salamander SDK. | | openrouter/openai/gpt-6-luna-pro |

//****************************************************************************
//
// Copyright (c) 2023 Open Salamander Authors
//
// This is a part of the Open Salamander SDK library.
//
//****************************************************************************

#pragma once

// globalni data
extern HINSTANCE DLLInstance; // handle k SPL-ku - jazykove nezavisle resourcy
extern HINSTANCE HLanguage;   // handle k SLG-cku - jazykove zavisle resourcy

// obecne rozhrani Salamandera - platne od startu az do ukonceni pluginu
extern CSalamanderGeneralAbstract* SalamanderGeneral;

char* LoadStr(int resID);

enum class NativeUpdateFrequency
{
    Disabled = 0,
    Daily = 1,
    Weekly = 2,
    Monthly = 3,
};

struct NativeUpdateSettings
{
    int CheckOnStartup;
    int Frequency;
    int HasLastCheckUtc;
    LONGLONG LastCheckUtcTicks;
    char LastPromptedVersion[128];
    char LastKnownRemoteVersion[128];
    char PluginCatalogSources[4096];
};

namespace SamandarinNative
{
void NativeUpdater_Initialize(HWND parent);
BOOL NativeUpdater_Shutdown(DWORD timeoutMs);
BOOL NativeUpdater_ShutdownForce();
BOOL NativeUpdater_ShowConfiguration(HWND parent);
BOOL NativeUpdater_CheckNow(HWND parent);
BOOL NativeUpdater_ShowPluginUpdates(HWND parent);
void NativeUpdater_NotifyColorsChanged();
BOOL NativeUpdater_IsProcessExiting();
BOOL NativeUpdater_PrepareForProcessExit(DWORD timeoutMs, BOOL finalize);
void NativeUpdater_ResumeAfterProcessExit();
}

extern "C" __declspec(dllexport) BOOL __stdcall Samandarin_LoadSettings(NativeUpdateSettings* settings);
extern "C" __declspec(dllexport) BOOL __stdcall Samandarin_SaveSettings(const NativeUpdateSettings* settings);

// prikazy pluginoveho menu
#define MENUCMD_CHECKNOW 1
#define MENUCMD_PLUGIN_UPDATES 2

class CPluginInterfaceForMenuExt : public CPluginInterfaceForMenuExtAbstract
{
public:
    virtual DWORD WINAPI GetMenuItemState(int id, DWORD eventMask) { return 0; }
    virtual BOOL WINAPI ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander, HWND parent,
                                        int id, DWORD eventMask);
    virtual BOOL WINAPI HelpForMenuItem(HWND parent, int id);
    virtual void WINAPI BuildMenu(HWND parent, CSalamanderBuildMenuAbstract* salamander) {}
};

class CPluginInterface : public CPluginInterfaceAbstract
{
public:
    virtual void WINAPI About(HWND parent);

    virtual BOOL WINAPI Release(HWND parent, BOOL force);

    virtual void WINAPI LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry) {}
    virtual void WINAPI SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry) {}
    virtual void WINAPI Configuration(HWND parent);

    virtual void WINAPI Connect(HWND parent, CSalamanderConnectAbstract* salamander);

    virtual void WINAPI ReleasePluginDataInterface(CPluginDataInterfaceAbstract* pluginData) {}

    virtual CPluginInterfaceForArchiverAbstract* WINAPI GetInterfaceForArchiver() { return NULL; }
    virtual CPluginInterfaceForViewerAbstract* WINAPI GetInterfaceForViewer() { return NULL; }
    virtual CPluginInterfaceForMenuExtAbstract* WINAPI GetInterfaceForMenuExt();
    virtual CPluginInterfaceForFSAbstract* WINAPI GetInterfaceForFS() { return NULL; }
    virtual CPluginInterfaceForThumbLoaderAbstract* WINAPI GetInterfaceForThumbLoader() { return NULL; }

    virtual void WINAPI Event(int event, DWORD param);
    virtual void WINAPI ClearHistory(HWND parent) {}
    virtual void WINAPI AcceptChangeOnPathNotification(const char* path, BOOL includingSubdirs) {}

    virtual void WINAPI PasswordManagerEvent(HWND parent, int event) {}
};

// rozhrani pluginu poskytnute Salamanderovi
extern CPluginInterface PluginInterface;
extern CPluginInterfaceForMenuExt InterfaceForMenuExt;

void ShowInitializationError(HWND parent);
