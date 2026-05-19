#include "precomp.h"

class CHyperVFS : public CPluginFSInterfaceAbstract
{
public:
    CHyperVFS() { Path[0] = 0; }
    char Path[MAX_PATH];

    virtual BOOL WINAPI GetCurrentPath(char* userPart) { lstrcpynA(userPart, Path, MAX_PATH); return TRUE; }
    virtual BOOL WINAPI GetFullName(CFileData& file, int isDir, char* buf, int bufSize) { (void)isDir; lstrcpynA(buf, file.Name, bufSize); return TRUE; }
    virtual BOOL WINAPI GetFullFSPath(HWND parent, const char* fsName, char* path, int pathSize, BOOL& success) { (void)parent; _snprintf(path, pathSize, "%s:%s", fsName, Path); success = TRUE; return TRUE; }
    virtual BOOL WINAPI GetRootPath(char* userPart) { userPart[0] = 0; return TRUE; }
    virtual BOOL WINAPI IsCurrentPath(int currentFSNameIndex, int fsNameIndex, const char* userPart) { (void)currentFSNameIndex; (void)fsNameIndex; return lstrcmpiA(Path, userPart ? userPart : "") == 0; }
    virtual BOOL WINAPI IsOurPath(int currentFSNameIndex, int fsNameIndex, const char* userPart) { (void)currentFSNameIndex; (void)fsNameIndex; (void)userPart; return TRUE; }
    virtual BOOL WINAPI ChangePath(int currentFSNameIndex, char* fsName, int fsNameIndex, const char* userPart, char* cutFileName, BOOL* pathWasCut, BOOL forceRefresh, int mode)
    { (void)currentFSNameIndex; (void)fsName; (void)fsNameIndex; (void)cutFileName; (void)pathWasCut; (void)forceRefresh; (void)mode; lstrcpynA(Path, userPart ? userPart : "", MAX_PATH); return TRUE; }

    virtual BOOL WINAPI ListCurrentPath(CSalamanderDirectoryAbstract* dir, CPluginDataInterfaceAbstract*& pluginData, int& iconsType, BOOL forceRefresh)
    {
        (void)forceRefresh;
        pluginData = NULL;
        iconsType = pitFromPlugin;
        dir->SetValidData(VALID_DATA_NONE);

        SECURITY_ATTRIBUTES sa = {0};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;

        HANDLE stdoutRead = NULL;
        HANDLE stdoutWrite = NULL;
        if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0))
            return TRUE;
        SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si = {0};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        si.hStdOutput = stdoutWrite;
        si.hStdError = stdoutWrite;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.wShowWindow = SW_HIDE;

        PROCESS_INFORMATION pi = {0};
        char cmdLine[] = "powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"$ErrorActionPreference='Stop'; Import-Module Hyper-V -ErrorAction Stop; Get-VM -ComputerName localhost -ErrorAction Stop | Select-Object -ExpandProperty Name\"";
        BOOL created = CreateProcessA(NULL, cmdLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
        CloseHandle(stdoutWrite);
        if (!created)
        {
            CloseHandle(stdoutRead);
            return TRUE;
        }

        char line[512];
        char chunk[256];
        DWORD bytesRead = 0;
        std::string pending;
        while (ReadFile(stdoutRead, chunk, sizeof(chunk) - 1, &bytesRead, NULL) && bytesRead > 0)
        {
            chunk[bytesRead] = 0;
            pending.append(chunk);

            size_t pos = 0;
            while (true)
            {
                size_t nl = pending.find('\n', pos);
                if (nl == std::string::npos)
                {
                    pending.erase(0, pos);
                    break;
                }

                std::string one = pending.substr(pos, nl - pos);
                if (!one.empty() && one.back() == '\r')
                    one.pop_back();
                pos = nl + 1;
                if (one.empty())
                    continue;

                lstrcpynA(line, one.c_str(), (int)sizeof(line));

                CFileData file;
                memset(&file, 0, sizeof(file));
                file.Name = SalamanderGeneral->DupStr(line);
                file.NameLen = static_cast<int>(strlen(file.Name));
                file.Ext = file.Name + file.NameLen;
                file.DosName = NULL;
                file.IsLink = 0;
                file.IsOffline = 0;
                file.Hidden = 0;
                file.Attr = 0;
                file.PluginData = 0;
                dir->AddFile(NULL, file, pluginData);
            }
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(stdoutRead);
        return TRUE;
    }

    virtual BOOL WINAPI TryCloseOrDetach(BOOL forceClose, BOOL canDetach, BOOL& detach, int reason) { (void)forceClose; (void)canDetach; (void)reason; detach = FALSE; return TRUE; }
    virtual void WINAPI Event(int event, DWORD param) { (void)event; (void)param; }
    virtual void WINAPI ReleaseObject(HWND parent) { (void)parent; }
    virtual DWORD WINAPI GetSupportedServices() { return 0; }
    virtual BOOL WINAPI GetChangeDriveOrDisconnectItem(const char* fsName, char*& title, HICON& icon, BOOL& destroyIcon) { (void)fsName; (void)title; icon = NULL; destroyIcon = FALSE; return FALSE; }
    virtual HICON WINAPI GetFSIcon(BOOL& destroyIcon) { destroyIcon = FALSE; return NULL; }
    virtual void WINAPI GetDropEffect(const char* srcFSPath, const char* tgtFSPath, DWORD allowedEffects, DWORD keyState, DWORD* dropEffect) { (void)srcFSPath; (void)tgtFSPath; (void)keyState; *dropEffect = allowedEffects & DROPEFFECT_COPY; }
    virtual void WINAPI GetFSFreeSpace(CQuadWord* retValue) { retValue->SetUI64(0); }
    virtual BOOL WINAPI GetNextDirectoryLineHotPath(const char* text, int pathLen, int& offset) { (void)text; (void)pathLen; (void)offset; return FALSE; }
    virtual void WINAPI CompleteDirectoryLineHotPath(char* path, int pathBufSize) { (void)path; (void)pathBufSize; }
    virtual BOOL WINAPI GetPathForMainWindowTitle(const char* fsName, int mode, char* buf, int bufSize) { (void)mode; _snprintf(buf, bufSize, "%s:%s", fsName, Path); return TRUE; }
    virtual void WINAPI ShowInfoDialog(const char* fsName, HWND parent) { (void)fsName; (void)parent; }
    virtual BOOL WINAPI ExecuteCommandLine(HWND parent, char* command, int& selFrom, int& selTo) { (void)parent; (void)command; (void)selFrom; (void)selTo; return FALSE; }
    virtual BOOL WINAPI QuickRename(const char* fsName, int mode, HWND parent, CFileData& file, BOOL isDir, char* newName, BOOL& cancel) { (void)fsName; (void)mode; (void)parent; (void)file; (void)isDir; (void)newName; cancel = FALSE; return FALSE; }
    virtual void WINAPI AcceptChangeOnPathNotification(const char* fsName, const char* path, BOOL includingSubdirs) { (void)fsName; (void)path; (void)includingSubdirs; }
    virtual BOOL WINAPI CreateDir(const char* fsName, int mode, HWND parent, char* newName, BOOL& cancel) { (void)fsName; (void)mode; (void)parent; (void)newName; cancel = FALSE; return FALSE; }
    virtual void WINAPI ViewFile(const char* fsName, HWND parent, CSalamanderForViewFileOnFSAbstract* salamander, CFileData& file) { (void)fsName; (void)parent; (void)salamander; (void)file; }
    virtual BOOL WINAPI Delete(const char* fsName, int mode, HWND parent, int panel, int selectedFiles, int selectedDirs, BOOL& cancelOrError) { (void)fsName; (void)mode; (void)parent; (void)panel; (void)selectedFiles; (void)selectedDirs; cancelOrError = FALSE; return FALSE; }
    virtual BOOL WINAPI CopyOrMoveFromFS(BOOL copy, int mode, const char* fsName, HWND parent, int panel, int selectedFiles, int selectedDirs, char* targetPath, BOOL& operationMask, BOOL& cancelOrHandlePath, HWND dropTarget) { (void)copy; (void)mode; (void)fsName; (void)parent; (void)panel; (void)selectedFiles; (void)selectedDirs; (void)targetPath; (void)operationMask; (void)dropTarget; cancelOrHandlePath = FALSE; return FALSE; }
    virtual BOOL WINAPI CopyOrMoveFromDiskToFS(BOOL copy, int mode, const char* fsName, HWND parent, const char* sourcePath, SalEnumSelection2 next, void* nextParam, int sourceFiles, int sourceDirs, char* targetPath, BOOL* invalidPathOrCancel) { (void)copy; (void)mode; (void)fsName; (void)parent; (void)sourcePath; (void)next; (void)nextParam; (void)sourceFiles; (void)sourceDirs; (void)targetPath; if (invalidPathOrCancel) *invalidPathOrCancel = FALSE; return FALSE; }
    virtual BOOL WINAPI ChangeAttributes(const char* fsName, HWND parent, int panel, int selectedFiles, int selectedDirs) { (void)fsName; (void)parent; (void)panel; (void)selectedFiles; (void)selectedDirs; return FALSE; }
    virtual void WINAPI ShowProperties(const char* fsName, HWND parent, int panel, int selectedFiles, int selectedDirs) { (void)fsName; (void)parent; (void)panel; (void)selectedFiles; (void)selectedDirs; }
    virtual void WINAPI ContextMenu(const char* fsName, HWND parent, int menuX, int menuY, int type, int panel, int selectedFiles, int selectedDirs) { (void)fsName; (void)parent; (void)menuX; (void)menuY; (void)type; (void)panel; (void)selectedFiles; (void)selectedDirs; }
    virtual BOOL WINAPI HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult) { (void)uMsg; (void)wParam; (void)lParam; (void)plResult; return FALSE; }
    virtual BOOL WINAPI OpenFindDialog(const char* fsName, int panel) { (void)fsName; (void)panel; return FALSE; }
    virtual void WINAPI OpenActiveFolder(const char* fsName, HWND parent) { (void)fsName; (void)parent; }
    virtual void WINAPI GetAllowedDropEffects(int mode, const char* tgtFSPath, DWORD* allowedEffects) { (void)mode; (void)tgtFSPath; *allowedEffects = 0; }
    virtual BOOL WINAPI GetNoItemsInPanelText(char* textBuf, int textBufSize) { lstrcpynA(textBuf, "No Hyper-V virtual machines found.", textBufSize); return TRUE; }
    virtual void WINAPI ShowSecurityInfo(HWND parent) { (void)parent; }
    virtual void WINAPI EnsureShareExistsOnServer(int panel, const char* server, const char* share) { (void)panel; (void)server; (void)share; }
};

class CHyperVFSInterface : public CPluginInterfaceForFSAbstract
{
public:
    virtual CPluginFSInterfaceAbstract* WINAPI OpenFS(const char* fsName, int fsNameIndex) { (void)fsName; (void)fsNameIndex; return new CHyperVFS(); }
    virtual void WINAPI CloseFS(CPluginFSInterfaceAbstract* fs) { if (fs) delete fs; }
    virtual void WINAPI ExecuteChangeDriveMenuItem(int panel) { int failReason = 0; (void)panel; SalamanderGeneral->ChangePanelPathToPluginFS(PANEL_SOURCE, AssignedFSName, "", &failReason); }
    virtual BOOL WINAPI ChangeDriveMenuItemContextMenu(HWND parent, int panel, int x, int y, CPluginFSInterfaceAbstract* pluginFS, const char* pluginFSName, int pluginFSNameIndex, BOOL isDetachedFS, BOOL& refreshMenu, BOOL& closeMenu, int& postCmd, void*& postCmdParam)
    { (void)parent; (void)panel; (void)x; (void)y; (void)pluginFS; (void)pluginFSName; (void)pluginFSNameIndex; (void)isDetachedFS; (void)refreshMenu; (void)closeMenu; (void)postCmd; (void)postCmdParam; return FALSE; }
    virtual void WINAPI ExecuteChangeDrivePostCommand(int panel, int postCmd, void* postCmdParam) { (void)panel; (void)postCmd; (void)postCmdParam; }
    virtual void WINAPI ExecuteOnFS(int panel, CPluginFSInterfaceAbstract* pluginFS, const char* pluginFSName, int pluginFSNameIndex, CFileData& file, int isDir) { (void)panel; (void)pluginFS; (void)pluginFSName; (void)pluginFSNameIndex; (void)file; (void)isDir; }
    virtual BOOL WINAPI DisconnectFS(HWND parent, BOOL isInPanel, int panel, CPluginFSInterfaceAbstract* pluginFS, const char* pluginFSName, int pluginFSNameIndex) { (void)isInPanel; (void)panel; (void)pluginFSName; (void)pluginFSNameIndex; SalamanderGeneral->CloseDetachedFS(parent, pluginFS); return TRUE; }
    virtual void WINAPI ConvertPathToInternal(const char* fsName, int fsNameIndex, char* fsUserPart) { (void)fsName; (void)fsNameIndex; (void)fsUserPart; }
    virtual void WINAPI ConvertPathToExternal(const char* fsName, int fsNameIndex, char* fsUserPart) { (void)fsName; (void)fsNameIndex; (void)fsUserPart; }
    virtual void WINAPI EnsureShareExistsOnServer(int panel, const char* server, const char* share) { (void)panel; (void)server; (void)share; }
};

CHyperVFSInterface gHyperVFSInterface;
CPluginInterfaceForFSAbstract* gHyperVFSInterfacePtr = &gHyperVFSInterface;
