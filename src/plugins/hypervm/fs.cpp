#include "precomp.h"

class CHyperVFS : public CPluginFSInterfaceAbstract
{
public:
    CHyperVFS() { Path[0] = 0; }
    char Path[MAX_PATH];

    virtual BOOL WINAPI GetCurrentPath(char* userPart) { lstrcpynA(userPart, Path, MAX_PATH); return TRUE; }
    virtual BOOL WINAPI GetFullName(CFileData& file, int isDir, char* buf, int bufSize) { (void)isDir; lstrcpynA(buf, file.Name, bufSize); return TRUE; }
    virtual BOOL WINAPI GetFullFSPath(HWND parent, const char* fsName, char* path, int pathSize, BOOL& success) { (void)parent; _snprintf(path, pathSize, "%s:\\%s", fsName, Path); success = TRUE; return TRUE; }
    virtual BOOL WINAPI GetRootPath(char* userPart) { userPart[0] = 0; return TRUE; }
    virtual BOOL WINAPI IsCurrentPath(int currentFSNameIndex, int fsNameIndex, const char* userPart) { (void)currentFSNameIndex; (void)fsNameIndex; (void)userPart; return TRUE; }
    virtual BOOL WINAPI IsOurPath(int currentFSNameIndex, int fsNameIndex, const char* userPart) { (void)currentFSNameIndex; (void)fsNameIndex; (void)userPart; return TRUE; }
    virtual BOOL WINAPI ChangePath(int currentFSNameIndex, char* fsName, int fsNameIndex, const char* userPart, char* cutFileName, BOOL* pathWasCut, BOOL forceRefresh, int mode)
    { (void)currentFSNameIndex; (void)fsName; (void)fsNameIndex; (void)cutFileName; (void)pathWasCut; (void)forceRefresh; (void)mode; lstrcpynA(Path, userPart ? userPart : "", MAX_PATH); return TRUE; }
    virtual BOOL WINAPI ListCurrentPath(CSalamanderDirectoryAbstract* dir, CPluginDataInterfaceAbstract*& pluginData, int& iconsType, BOOL forceRefresh)
    {
        (void)forceRefresh;
        pluginData = NULL;
        dir->SetValidData(VALID_DATA_NONE);
        iconsType = pitSimple;

        FILE* pipe = _popen("powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"$ErrorActionPreference='Stop'; Import-Module Hyper-V -ErrorAction Stop; Get-VM -ComputerName localhost | Select-Object -ExpandProperty Name\"", "rt");
        if (!pipe) return TRUE;
        char line[512];
        while (fgets(line, sizeof(line), pipe) != NULL)
        {
            size_t len = strlen(line);
            while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = 0;
            if (len == 0) continue;

            CFileData file;
            memset(&file, 0, sizeof(file));
            file.Name = SalamanderGeneral->DupStr(line);
            file.NameLen = (int)strlen(file.Name);
            file.Ext = file.Name + file.NameLen;
            file.Attr = 0;
            file.PluginData = 0;
            dir->AddFile(NULL, file, pluginData);
        }
        _pclose(pipe);
        return TRUE;
    }
    virtual BOOL WINAPI TryCloseOrDetach(BOOL forceClose, BOOL canDetach, BOOL& detach, int reason) { (void)forceClose; (void)canDetach; (void)reason; detach = FALSE; return TRUE; }
    virtual void WINAPI Event(int event, DWORD param) { (void)event; (void)param; }
    virtual void WINAPI ReleaseObject(HWND parent) { (void)parent; delete this; }
    virtual DWORD WINAPI GetSupportedServices() { return 0; }
};

class CHyperVFSInterface : public CPluginInterfaceForFSAbstract
{
public:
    virtual CPluginFSInterfaceAbstract* WINAPI OpenFS(const char* fsName, int fsNameIndex) { (void)fsName; (void)fsNameIndex; return new CHyperVFS(); }
    virtual void WINAPI CloseFS(CPluginFSInterfaceAbstract* fs) { delete fs; }
    virtual void WINAPI ExecuteChangeDriveMenuItem(int panel)
    {
        (void)panel;
        int failReason = 0;
        SalamanderGeneral->ChangePanelPathToPluginFS(PANEL_SOURCE, AssignedFSName, "", &failReason);
    }
    virtual BOOL WINAPI ChangeDriveMenuItemContextMenu(HWND parent, int panel, int x, int y, CPluginFSInterfaceAbstract* pluginFS, const char* pluginFSName, int pluginFSNameIndex, BOOL isDetachedFS, BOOL& refreshMenu, BOOL& closeMenu, int& postCmd, void*& postCmdParam)
    { (void)parent; (void)panel; (void)x; (void)y; (void)pluginFS; (void)pluginFSName; (void)pluginFSNameIndex; (void)isDetachedFS; (void)refreshMenu; (void)closeMenu; (void)postCmd; (void)postCmdParam; return FALSE; }
    virtual void WINAPI ExecuteChangeDrivePostCommand(int panel, int postCmd, void* postCmdParam) { (void)panel; (void)postCmd; (void)postCmdParam; }
    virtual void WINAPI ExecuteOnFS(int panel, CPluginFSInterfaceAbstract* pluginFS, const char* pluginFSName, int pluginFSNameIndex, CFileData& file, int isDir) { (void)panel; (void)pluginFS; (void)pluginFSName; (void)pluginFSNameIndex; (void)file; (void)isDir; }
    virtual BOOL WINAPI DisconnectFS(HWND parent, BOOL isInPanel, int panel, CPluginFSInterfaceAbstract* pluginFS, const char* pluginFSName, int pluginFSNameIndex)
    { (void)isInPanel; (void)panel; (void)pluginFSName; (void)pluginFSNameIndex; SalamanderGeneral->CloseDetachedFS(parent, pluginFS); return TRUE; }
};

CHyperVFSInterface gHyperVFSInterface;
CPluginInterfaceForFSAbstract* gHyperVFSInterfacePtr = &gHyperVFSInterface;
