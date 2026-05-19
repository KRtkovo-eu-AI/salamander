#include "precomp.h"

class CHyperVFSInterface : public CPluginInterfaceForFSAbstract
{
public:
    virtual CPluginFSInterfaceAbstract* WINAPI OpenFS(const char* fsName, int fsNameIndex)
    {
        (void)fsName;
        (void)fsNameIndex;
        return NULL;
    }

    virtual void WINAPI CloseFS(CPluginFSInterfaceAbstract* fs)
    {
        (void)fs;
    }

    virtual void WINAPI ExecuteChangeDriveMenuItem(int panel)
    {
        (void)panel;
        int failReason = 0;
        SalamanderGeneral->ChangePanelPathToPluginFS(PANEL_SOURCE, AssignedFSName, "", &failReason);
    }

    virtual BOOL WINAPI ChangeDriveMenuItemContextMenu(HWND parent, int panel, int x, int y,
                                                       CPluginFSInterfaceAbstract* pluginFS,
                                                       const char* pluginFSName, int pluginFSNameIndex,
                                                       BOOL isDetachedFS, BOOL& refreshMenu,
                                                       BOOL& closeMenu, int& postCmd, void*& postCmdParam)
    {
        (void)parent; (void)panel; (void)x; (void)y; (void)pluginFS; (void)pluginFSName; (void)pluginFSNameIndex;
        (void)isDetachedFS; (void)refreshMenu; (void)closeMenu; (void)postCmd; (void)postCmdParam;
        return FALSE;
    }

    virtual void WINAPI ExecuteChangeDrivePostCommand(int panel, int postCmd, void* postCmdParam)
    {
        (void)panel; (void)postCmd; (void)postCmdParam;
    }

    virtual void WINAPI ExecuteOnFS(int panel, CPluginFSInterfaceAbstract* pluginFS,
                                    const char* pluginFSName, int pluginFSNameIndex,
                                    CFileData& file, int isDir)
    {
        (void)panel; (void)pluginFS; (void)pluginFSName; (void)pluginFSNameIndex; (void)file; (void)isDir;
    }

    virtual BOOL WINAPI DisconnectFS(HWND parent, BOOL isInPanel, int panel,
                                     CPluginFSInterfaceAbstract* pluginFS,
                                     const char* pluginFSName, int pluginFSNameIndex)
    {
        (void)isInPanel; (void)panel; (void)pluginFSName; (void)pluginFSNameIndex;
        SalamanderGeneral->CloseDetachedFS(parent, pluginFS);
        return TRUE;
    }
};

CHyperVFSInterface gHyperVFSInterface;
CPluginInterfaceForFSAbstract* gHyperVFSInterfacePtr = &gHyperVFSInterface;
