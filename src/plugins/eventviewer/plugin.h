#pragma once

char* LoadStr(int resID);

extern HINSTANCE DLLInstance;
extern HINSTANCE HLanguage;

extern CSalamanderGeneralAbstract* SalamanderGeneral;
extern CSalamanderGUIAbstract* SalamanderGUI;

HINSTANCE GetLanguageResourceHandle();

class CEventViewerWindow;

void EnsureEventViewerWindowClosed();
void ShowEventViewerWindow(HWND parent);

enum
{
    MENUCMD_EVENT_VIEWER = 1,
};

class CPluginInterface : public CPluginInterfaceAbstract
{
public:
    virtual int WINAPI GetVersion() { return LAST_VERSION_OF_SALAMANDER; }

    virtual void WINAPI About(HWND parent);

    virtual void WINAPI Connect(HWND parent, CSalamanderConnectAbstract* salamander);
    virtual void WINAPI Event(int event, DWORD param);
    virtual BOOL WINAPI Release(HWND parent, BOOL force);

    virtual void WINAPI LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry);
    virtual void WINAPI SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry);
    virtual void WINAPI Configuration(HWND parent);

    virtual void WINAPI ClearHistory(HWND parent);
    virtual void WINAPI AcceptChangeOnPathNotification(const char* path, BOOL includingSubdirs) {}
    virtual void WINAPI PasswordManagerEvent(HWND parent, int event) {}
    virtual BOOL WINAPI UninstallUnregisteredComponents(HWND parent, char* componentsDescr, BOOL* uninstallSPL,
                                                        BOOL* uninstallLangDir, const char* pluginDir,
                                                        CDynamicString* deleteFileList)
    {
        return FALSE;
    }

    virtual CPluginInterfaceForMenuExtAbstract* WINAPI GetInterfaceForMenuExt();
};

class CPluginInterfaceForMenuExt : public CPluginInterfaceForMenuExtAbstract
{
public:
    virtual DWORD WINAPI GetMenuItemState(int id, DWORD eventMask);
    virtual BOOL WINAPI ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander, HWND parent,
                                        int id, DWORD eventMask);
    virtual BOOL WINAPI HelpForMenuItem(HWND parent, int id);
    virtual void WINAPI BuildMenu(HWND parent, CSalamanderBuildMenuAbstract* salamander);
};
