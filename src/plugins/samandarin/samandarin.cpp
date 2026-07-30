// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

//****************************************************************************
//
// Copyright (c) 2023 Open Salamander Authors
//
// This is a part of the Open Salamander SDK library.
//
//****************************************************************************

#include "precomp.h"
#include <stdlib.h>
#include <string>

#include "../salamatrix/salamatrix_extensions.h"

// objekt interfacu pluginu, jeho metody se volaji ze Salamandera
CPluginInterface PluginInterface;
CPluginInterfaceForMenuExt InterfaceForMenuExt;

// globalni data
const char* PluginNameEN = "Samandarin Update Notifier";    // neprekladane jmeno pluginu, pouziti pred loadem jazykoveho modulu + pro debug veci
const char* PluginNameShort = "SAMANDARIN"; // jmeno pluginu (kratce, bez mezer)

HINSTANCE DLLInstance = NULL; // handle k SPL-ku - jazykove nezavisle resourcy
HINSTANCE HLanguage = NULL;   // handle k SLG-cku - jazykove zavisle resourcy

// obecne rozhrani Salamandera - platne od startu az do ukonceni pluginu
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;

// definice promenne pro "dbg.h"
CSalamanderDebugAbstract* SalamanderDebug = NULL;

// definice promenne pro "spl_com.h"
int SalamanderVersion = 0;

// rozhrani poskytujici upravene Windows controly pouzivane v Salamanderovi
CSalamanderGUIAbstract* SalamanderGUI = NULL;


typedef int(WINAPI* FSalamanderExportInstalledPlugins)(char* buffer, int cchBuffer);

extern "C" __declspec(dllexport) int __stdcall Samandarin_ExportInstalledPlugins(char* buffer, int cchBuffer)
{
    HMODULE host = GetModuleHandle(NULL);
    if (host == NULL)
        return 0;

    FSalamanderExportInstalledPlugins exportInstalledPlugins =
        (FSalamanderExportInstalledPlugins)GetProcAddress(host, "SalamanderExportInstalledPlugins");
    if (exportInstalledPlugins == NULL)
        return 0;

    return exportInstalledPlugins(buffer, cchBuffer);
}

namespace
{
void AppendUtf8ExportField(std::wstring& output, const char* text)
{
    if (text == NULL)
        return;
    int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0);
    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (required == 0)
    {
        codePage = CP_ACP;
        flags = 0;
        required = MultiByteToWideChar(codePage, flags, text, -1, NULL, 0);
    }
    if (required <= 1)
        return;
    std::wstring wide(static_cast<size_t>(required), L'\0');
    MultiByteToWideChar(codePage, flags, text, -1, &wide[0], required);
    wide.resize(static_cast<size_t>(required - 1));
    for (size_t index = 0; index < wide.size(); ++index)
    {
        wchar_t character = wide[index];
        output.push_back(character == L'\t' || character == L'\r' || character == L'\n'
                             ? L' '
                             : character);
    }
}
}

extern "C" __declspec(dllexport) int __stdcall Samandarin_ExportInstalledExtensions(wchar_t* buffer, int cchBuffer)
{
    if (SalamanderGeneral == NULL)
        return 0;

    CSalamanderServiceQuery query;
    memset(&query, 0, sizeof(query));
    query.ServiceId = SALAMATRIX_SERVICE_EXTENSIONS;
    query.MinimumVersion = SALAMATRIX_EXTENSIONS_VERSION_1_0;
    CSalamanderServiceResult result;
    memset(&result, 0, sizeof(result));
    if (!SalamanderGeneral->QueryService(&query, &result) || result.Interface == NULL)
        return 0;

    Salamatrix::Extensions::IExtensionsService* service =
        static_cast<Salamatrix::Extensions::IExtensionsService*>(result.Interface);
    std::wstring text;
    const int count = service->GetExtensionCount();
    for (int index = 0; index < count; ++index)
    {
        Salamatrix::Extensions::ExtensionInfo info;
        if (!service->GetExtensionInfo(index, &info) ||
            (info.Descriptor.Flags & Salamatrix::Extensions::ExtensionFlagPackage) == 0)
        {
            continue;
        }

        AppendUtf8ExportField(text, info.Descriptor.Id);
        text.push_back(L'\t');
        AppendUtf8ExportField(text, info.Descriptor.Name);
        text.push_back(L'\t');
        AppendUtf8ExportField(text, info.Descriptor.Version);
        text.push_back(L'\t');
        AppendUtf8ExportField(text, info.Descriptor.EntryPoint);
        text.push_back(L'\t');
        AppendUtf8ExportField(text, info.Descriptor.IconPath);
        text.push_back(L'\n');
    }

    const int required = static_cast<int>(text.size()) + 1;
    if (buffer != NULL && cchBuffer > 0)
        lstrcpynW(buffer, text.c_str(), cchBuffer);
    return required;
}

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

void ShowInitializationError(HWND parent)
{
    SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_INIT_MANAGED_ERROR), LoadStr(IDS_PLUGINNAME),
                                     MB_OK | MB_ICONERROR);
}

BOOL SynchronizeLoadOnStartFlagFromSettings();

BOOL WINAPI CPluginInterfaceForMenuExt::ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander,
                                                        HWND parent, int id, DWORD eventMask)
{
    (void)salamander;
    (void)eventMask;

    switch (id)
    {
    case MENUCMD_CHECKNOW:
        if (!ManagedBridge_CheckNow(parent))
        {
            SalamanderGeneral->SalMessageBox(parent,
                                             LoadStr(IDS_TRIGGER_CHECK_ERROR),
                                             LoadStr(IDS_PLUGINNAME), MB_OK | MB_ICONERROR);
        }
        break;

    case MENUCMD_PLUGIN_UPDATES:
        if (!ManagedBridge_ShowPluginUpdates(parent))
        {
            SalamanderGeneral->SalMessageBox(parent,
                                             LoadStr(IDS_TRIGGER_CHECK_ERROR),
                                             LoadStr(IDS_PLUGINNAME), MB_OK | MB_ICONERROR);
        }
        break;

    default:
        SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_UNKNOWN_COMMAND), LoadStr(IDS_PLUGINNAME), MB_OK | MB_ICONERROR);
        break;
    }

    return FALSE;
}

BOOL WINAPI CPluginInterfaceForMenuExt::HelpForMenuItem(HWND parent, int id)
{
    (void)parent;
    (void)id;
    return FALSE;
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

    // tento plugin je delany pro aktualni verzi Salamandera a vyssi - provedeme kontrolu
    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
    { // starsi verze odmitneme
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
    // ziskame rozhrani poskytujici upravene Windows controly pouzivane v Salamanderovi
    SalamanderGUI = salamander->GetSalamanderGUI();

    // nastavime zakladni informace o pluginu
    salamander->SetBasicPluginData(LoadStr(IDS_PLUGINNAME), FUNCTION_CONFIGURATION | FUNCTION_LOADSAVECONFIGURATION,
                                   VERSINFO_VERSION_NO_PLATFORM, VERSINFO_COPYRIGHT,
                                   LoadStr(IDS_PLUGIN_DESCRIPTION), PluginNameShort,
                                   NULL, NULL);

    SynchronizeLoadOnStartFlagFromSettings();

    // nastavime URL home-page pluginu
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

BOOL WINAPI CPluginInterface::Release(HWND parent, BOOL /*force*/)
{
    (void)parent;
    ManagedBridge_Shutdown();
    return TRUE;
}

void WINAPI CPluginInterface::Configuration(HWND parent)
{
    if (!ManagedBridge_ShowConfiguration(parent))
    {
        ShowInitializationError(parent);
    }
}

void WINAPI CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    CALL_STACK_MESSAGE1("CPluginInterface::Connect(,)");

    salamander->AddMenuItem(-1, LoadStr(IDS_MENU_CHECKNOW), 0, MENUCMD_CHECKNOW, FALSE,
                            MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);
    salamander->AddMenuItem(-1, LoadStr(IDS_MENU_PLUGIN_UPDATES), 0, MENUCMD_PLUGIN_UPDATES, FALSE,
                            MENU_EVENT_TRUE, MENU_EVENT_TRUE, MENU_SKILLLEVEL_ALL);

    if (!ManagedBridge_EnsureInitialized(parent))
    {
        ShowInitializationError(parent);
    }

    if (SalamanderGUI != NULL)
    {
        CGUIIconListAbstract* iconList = SalamanderGUI->CreateIconList();
        if (iconList != NULL)
        {
            if (iconList->Create(16, 16, 1))
            {
                UINT loadFlags = SalamanderGeneral != NULL ? SalamanderGeneral->GetIconLRFlags() : LR_DEFAULTCOLOR;
                HICON hIcon = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_PLUGINICON), IMAGE_ICON, 16, 16, loadFlags);
                if (hIcon != NULL)
                {
                    iconList->ReplaceIcon(0, hIcon);
                    DestroyIcon(hIcon);
                    salamander->SetIconListForGUI(iconList);
                    salamander->SetPluginIcon(0);
                    salamander->SetPluginMenuAndToolbarIcon(0);
                    iconList = NULL;
                }
            }

            if (iconList != NULL)
            {
                SalamanderGUI->DestroyIconList(iconList);
            }
        }
    }
}

void WINAPI CPluginInterface::Event(int event, DWORD /*param*/)
{
    if (event == PLUGINEVENT_COLORSCHANGED)
    {
        ManagedBridge_NotifyColorsChanged();
    }
}

CPluginInterfaceForMenuExtAbstract* WINAPI CPluginInterface::GetInterfaceForMenuExt()
{
    return &InterfaceForMenuExt;
}

namespace
{
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

    const char* const kConfigCheckOnStartup = "CheckOnStartup";
    const char* const kConfigFrequency = "Frequency";
    const char* const kConfigLastCheckUtcTicks = "LastCheckUtcTicks";
    const char* const kConfigLastPromptedVersion = "LastPromptedVersion";
    const char* const kConfigLastKnownRemoteVersion = "LastKnownRemoteVersion";
    const char* const kConfigPluginCatalogSources = "PluginCatalogSources";

    void InitializeDefaults(NativeUpdateSettings* settings)
    {
        if (settings == nullptr)
        {
            return;
        }

        settings->CheckOnStartup = TRUE;
        settings->Frequency = static_cast<int>(NativeUpdateFrequency::Weekly);
        settings->HasLastCheckUtc = FALSE;
        settings->LastCheckUtcTicks = 0;
        settings->LastPromptedVersion[0] = '\0';
        settings->LastKnownRemoteVersion[0] = '\0';
        settings->PluginCatalogSources[0] = '\0';
    }

    void WINAPI LoadOrSaveSettingsCallback(BOOL load, HKEY regKey, CSalamanderRegistryAbstract* registry, void* param)
    {
        auto* settings = reinterpret_cast<NativeUpdateSettings*>(param);
        if (load)
        {
            InitializeDefaults(settings);
            if (settings == nullptr || regKey == NULL)
            {
                return;
            }

            DWORD checkOnStartup = 0;
            if (registry->GetValue(regKey, kConfigCheckOnStartup, REG_DWORD, &checkOnStartup, sizeof(checkOnStartup)))
            {
                settings->CheckOnStartup = checkOnStartup != 0 ? TRUE : FALSE;
            }

            DWORD frequency = static_cast<DWORD>(NativeUpdateFrequency::Weekly);
            if (registry->GetValue(regKey, kConfigFrequency, REG_DWORD, &frequency, sizeof(frequency)) &&
                frequency <= static_cast<DWORD>(NativeUpdateFrequency::Monthly))
            {
                settings->Frequency = static_cast<int>(frequency);
            }

            char ticksBuffer[64];
            ticksBuffer[0] = '\0';
            if (registry->GetValue(regKey, kConfigLastCheckUtcTicks, REG_SZ, ticksBuffer, sizeof(ticksBuffer)))
            {
                ticksBuffer[sizeof(ticksBuffer) - 1] = '\0';
                char* end = nullptr;
                LONGLONG ticks = _strtoi64(ticksBuffer, &end, 10);
                if (end != ticksBuffer)
                {
                    settings->LastCheckUtcTicks = ticks;
                    settings->HasLastCheckUtc = TRUE;
                }
            }

            settings->LastPromptedVersion[0] = '\0';
            if (registry->GetValue(regKey, kConfigLastPromptedVersion, REG_SZ,
                                    settings->LastPromptedVersion, sizeof(settings->LastPromptedVersion)))
            {
                settings->LastPromptedVersion[sizeof(settings->LastPromptedVersion) - 1] = '\0';
            }

            settings->LastKnownRemoteVersion[0] = '\0';
            if (registry->GetValue(regKey, kConfigLastKnownRemoteVersion, REG_SZ,
                                    settings->LastKnownRemoteVersion, sizeof(settings->LastKnownRemoteVersion)))
            {
                settings->LastKnownRemoteVersion[sizeof(settings->LastKnownRemoteVersion) - 1] = '\0';
            }

            settings->PluginCatalogSources[0] = '\0';
            if (registry->GetValue(regKey, kConfigPluginCatalogSources, REG_SZ,
                                    settings->PluginCatalogSources, sizeof(settings->PluginCatalogSources)))
            {
                settings->PluginCatalogSources[sizeof(settings->PluginCatalogSources) - 1] = '\0';
            }
        }
        else
        {
            if (settings == nullptr || regKey == NULL)
            {
                return;
            }

            DWORD checkOnStartup = settings->CheckOnStartup != 0 ? 1U : 0U;
            registry->SetValue(regKey, kConfigCheckOnStartup, REG_DWORD, &checkOnStartup, sizeof(checkOnStartup));

            DWORD frequency = static_cast<DWORD>(settings->Frequency);
            registry->SetValue(regKey, kConfigFrequency, REG_DWORD, &frequency, sizeof(frequency));

            if (settings->HasLastCheckUtc != 0)
            {
                char buffer[64];
                _snprintf_s(buffer, _TRUNCATE, "%lld", settings->LastCheckUtcTicks);
                registry->SetValue(regKey, kConfigLastCheckUtcTicks, REG_SZ, buffer, -1);
            }
            else
            {
                registry->DeleteValue(regKey, kConfigLastCheckUtcTicks);
            }

            if (settings->LastPromptedVersion[0] != '\0')
            {
                registry->SetValue(regKey, kConfigLastPromptedVersion, REG_SZ,
                                   settings->LastPromptedVersion, -1);
            }
            else
            {
                registry->DeleteValue(regKey, kConfigLastPromptedVersion);
            }

            if (settings->LastKnownRemoteVersion[0] != '\0')
            {
                registry->SetValue(regKey, kConfigLastKnownRemoteVersion, REG_SZ,
                                   settings->LastKnownRemoteVersion, -1);
            }
            else
            {
                registry->DeleteValue(regKey, kConfigLastKnownRemoteVersion);
            }

            if (settings->PluginCatalogSources[0] != '\0')
            {
                registry->SetValue(regKey, kConfigPluginCatalogSources, REG_SZ,
                                   settings->PluginCatalogSources, -1);
            }
            else
            {
                registry->DeleteValue(regKey, kConfigPluginCatalogSources);
            }
        }
    }
} // namespace

BOOL SynchronizeLoadOnStartFlagFromSettings()
{
    if (SalamanderGeneral == NULL)
    {
        return FALSE;
    }

    NativeUpdateSettings settings;
    InitializeDefaults(&settings);
    SalamanderGeneral->CallLoadOrSaveConfiguration(TRUE, LoadOrSaveSettingsCallback, &settings);
    SalamanderGeneral->SetFlagLoadOnSalamanderStart(settings.CheckOnStartup != 0);
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL __stdcall Samandarin_LoadSettings(NativeUpdateSettings* settings)
{
    if (settings == nullptr || SalamanderGeneral == NULL)
    {
        return FALSE;
    }

    SalamanderGeneral->CallLoadOrSaveConfiguration(TRUE, LoadOrSaveSettingsCallback, settings);
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL __stdcall Samandarin_SaveSettings(const NativeUpdateSettings* settings)
{
    if (settings == nullptr || SalamanderGeneral == NULL)
    {
        return FALSE;
    }

    NativeUpdateSettings localCopy = *settings;
    SalamanderGeneral->CallLoadOrSaveConfiguration(FALSE, LoadOrSaveSettingsCallback, &localCopy);
    SalamanderGeneral->SetFlagLoadOnSalamanderStart(localCopy.CheckOnStartup != 0);
    return TRUE;
}
