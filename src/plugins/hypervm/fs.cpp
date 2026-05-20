#include "precomp.h"
#include <string>
#include <vector>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

static HICON LoadPluginIconResource(int resourceId, int size)
{
    return (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(resourceId), IMAGE_ICON, size, size, SalamanderGeneral->GetIconLRFlags());
}

static HBITMAP CreateMenuBitmapFromIcon(HICON icon)
{
    if (icon == NULL)
        return NULL;

    HDC screenDc = GetDC(NULL);
    if (screenDc == NULL)
        return NULL;
    HDC memDc = CreateCompatibleDC(screenDc);
    HBITMAP bmp = CreateCompatibleBitmap(screenDc, 16, 16);
    HGDIOBJ oldBmp = bmp ? SelectObject(memDc, bmp) : NULL;
    if (bmp != NULL)
    {
        RECT r = {0, 0, 16, 16};
        FillRect(memDc, &r, (HBRUSH)(COLOR_MENU + 1));
        DrawIconEx(memDc, 0, 0, icon, 16, 16, 0, NULL, DI_NORMAL);
    }
    if (oldBmp != NULL)
        SelectObject(memDc, oldBmp);
    if (memDc != NULL)
        DeleteDC(memDc);
    ReleaseDC(NULL, screenDc);
    return bmp;
}

static void SetMenuItemIcon(HMENU menu, UINT cmdId, int iconResourceId)
{
    HICON icon = LoadPluginIconResource(iconResourceId, 16);
    HBITMAP bmp = CreateMenuBitmapFromIcon(icon);
    if (bmp != NULL)
    {
        MENUITEMINFOA mi = {0};
        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_BITMAP;
        mi.hbmpItem = bmp;
        SetMenuItemInfoA(menu, cmdId, FALSE, &mi);
    }
    if (icon != NULL)
        DestroyIcon(icon);
}

static std::string EscapePsSingleQuoted(const char* text)
{
    std::string src = text ? text : "";
    std::string out;
    out.reserve(src.size() + 8);
    for (char c : src)
    {
        if (c == '\'')
            out += "''";
        else
            out.push_back(c);
    }
    return out;
}

static bool RunHiddenPowerShell(const std::string& script)
{
    std::string cmdLine = "powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"" + script + "\"";
    std::vector<char> mutableCmd(cmdLine.begin(), cmdLine.end());
    mutableCmd.push_back('\0');

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {0};
    BOOL created = CreateProcessA(NULL, mutableCmd.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (!created)
        return false;

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return exitCode == 0;
}

static bool RunDetachedProcessAndWaitForInputIdle(const std::string& commandLine, HWND parent, DWORD timeoutMs)
{
    std::vector<char> mutableCmd(commandLine.begin(), commandLine.end());
    mutableCmd.push_back('\0');

    HCURSOR oldCursor = SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
    if (parent != NULL)
        SetCapture(parent);

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};
    BOOL created = CreateProcessA(NULL, mutableCmd.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (!created)
    {
        if (parent != NULL)
            ReleaseCapture();
        SetCursor(oldCursor);
        return false;
    }

    WaitForInputIdle(pi.hProcess, timeoutMs);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (parent != NULL)
        ReleaseCapture();
    SetCursor(oldCursor);
    return true;
}

static bool LaunchVmCreate(HWND parent)
{
    return ShellExecuteA(parent, "open", "C:\\Program Files\\Hyper-V\\VMCreate.exe", "", "", SW_SHOWNORMAL) > (HINSTANCE)32;
}

static bool LaunchVmConnect(const char* vmName, HWND parent)
{
    std::string vm = vmName ? vmName : "";
    std::string cmdLine = "vmconnect.exe localhost \"" + vm + "\"";
    return RunDetachedProcessAndWaitForInputIdle(cmdLine, parent, 10000);
}


struct CHyperVItemData
{
    bool Running;
};

static int WINAPI HyperVGetSimpleIconIndex()
{
    return 0;
}

class CHyperVPluginDataInterface : public CPluginDataInterfaceAbstract
{
public:
    HIMAGELIST ImageList;

    CHyperVPluginDataInterface() : ImageList(NULL) {}
    virtual ~CHyperVPluginDataInterface()
    {
        if (ImageList != NULL)
            ImageList_Destroy(ImageList);
    }

    virtual BOOL WINAPI CallReleaseForFiles() { return TRUE; }
    virtual BOOL WINAPI CallReleaseForDirs() { return TRUE; }
    virtual void WINAPI ReleasePluginData(CFileData& file, BOOL isDir) { (void)isDir; CHyperVItemData* ext = (CHyperVItemData*)file.PluginData; if (ext != NULL) delete ext; file.PluginData = 0; }
    virtual void WINAPI GetFileDataForUpDir(const char* archivePath, CFileData& upDir) { (void)archivePath; (void)upDir; }
    virtual BOOL WINAPI GetFileDataForNewDir(const char* dirName, CFileData& dir) { (void)dirName; (void)dir; return TRUE; }
    virtual HIMAGELIST WINAPI GetSimplePluginIcons(int iconSize)
    {
        int size = iconSize == SALICONSIZE_32 ? 32 : 16;
        if (ImageList != NULL)
        {
            ImageList_Destroy(ImageList);
            ImageList = NULL;
        }
        ImageList = ImageList_Create(size, size, ILC_COLOR32 | ILC_MASK, 1, 1);
        if (ImageList == NULL)
            return NULL;
        HICON icon = LoadPluginIconResource(IDI_VM_ITEM, size);
        if (icon != NULL)
        {
            ImageList_ReplaceIcon(ImageList, -1, icon);
            DestroyIcon(icon);
        }
        return ImageList;
    }
    virtual BOOL WINAPI HasSimplePluginIcon(CFileData& file, BOOL isDir) { (void)file; (void)isDir; return TRUE; }
    virtual HICON WINAPI GetPluginIcon(const CFileData* file, int iconSize, BOOL& destroyIcon) { (void)file; destroyIcon = TRUE; return LoadPluginIconResource(IDI_VM_ITEM, iconSize == SALICONSIZE_32 ? 32 : 16); }
    virtual int WINAPI CompareFilesFromFS(const CFileData* file1, const CFileData* file2) { return lstrcmpiA(file1->Name, file2->Name); }
    virtual void WINAPI SetupView(BOOL leftPanel, CSalamanderViewAbstract* view, const char* archivePath, const CFileData* upperDir)
    { (void)leftPanel; (void)archivePath; (void)upperDir; view->SetPluginSimpleIconCallback(HyperVGetSimpleIconIndex); }
    virtual void WINAPI ColumnFixedWidthShouldChange(BOOL leftPanel, const CColumn* column, int newFixedWidth) { (void)leftPanel; (void)column; (void)newFixedWidth; }
    virtual void WINAPI ColumnWidthWasChanged(BOOL leftPanel, const CColumn* column, int newWidth) { (void)leftPanel; (void)column; (void)newWidth; }
    virtual void WINAPI GetInfoLineContent(int panel, const CFileData* file, BOOL isDir, int selectedFiles, int selectedDirs, BOOL displaySize, const CQuadWord& selectedSize, char* buffer, DWORD* hotTexts, int& hotTextsCount) { (void)panel; (void)file; (void)isDir; (void)selectedFiles; (void)selectedDirs; (void)displaySize; (void)selectedSize; (void)hotTexts; hotTextsCount = 0; if (buffer) buffer[0] = 0; }
    virtual BOOL WINAPI CanBeCopiedToClipboard() { return FALSE; }
    virtual void WINAPI GetByteSize(const CFileData* file, BOOL isDir, CQuadWord* size) { (void)file; (void)isDir; if (size) size->SetUI64(0); }
};

static bool QueryVmState(const std::string& vmNameEscaped, std::string& state)
{
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    bool coInit = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
    if (!coInit)
        return false;

    IWbemLocator* pLoc = NULL;
    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hr) || pLoc == NULL)
    {
        CoUninitialize();
        return false;
    }

    IWbemServices* pSvc = NULL;
    hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\virtualization\\v2"), NULL, NULL, 0, 0, 0, 0, &pSvc);
    if (FAILED(hr) || pSvc == NULL)
    {
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hr))
    {
        pSvc->Release(); pLoc->Release(); CoUninitialize(); return false;
    }

    std::wstring wvm(vmNameEscaped.begin(), vmNameEscaped.end());
    std::wstring query = L"SELECT EnabledState FROM Msvm_ComputerSystem WHERE Caption='Virtual Machine' AND ElementName='" + wvm + L"'";

    IEnumWbemClassObject* pEnumerator = NULL;
    hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    bool ok = false;
    if (SUCCEEDED(hr) && pEnumerator != NULL)
    {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn > 0)
        {
            VARIANT vtState; VariantInit(&vtState);
            pclsObj->Get(L"EnabledState", 0, &vtState, 0, 0);
            long st = 0;
            if (vtState.vt == VT_I4) st = vtState.lVal;
            else if (vtState.vt == VT_UI4) st = (long)vtState.ulVal;
            state = (st == 2) ? "Running" : "Off";
            VariantClear(&vtState);
            pclsObj->Release();
            ok = true;
        }
        pEnumerator->Release();
    }

    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return ok;
}

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
        pluginData = new CHyperVPluginDataInterface();
        iconsType = pitFromPlugin;
        dir->SetValidData(VALID_DATA_NONE);

        HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
        bool coInit = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
        if (!coInit)
            return TRUE;

        hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
        (void)hr;

        IWbemLocator* pLoc = NULL;
        hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                              IID_IWbemLocator, (LPVOID*)&pLoc);
        if (FAILED(hr) || pLoc == NULL)
        {
            CoUninitialize();
            return TRUE;
        }

        IWbemServices* pSvc = NULL;
        hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\virtualization\\v2"), NULL, NULL, 0, 0, 0, 0, &pSvc);
        if (FAILED(hr) || pSvc == NULL)
        {
            pLoc->Release();
            CoUninitialize();
            return TRUE;
        }

        hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                               RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
        if (FAILED(hr))
        {
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return TRUE;
        }

        IEnumWbemClassObject* pEnumerator = NULL;
        hr = pSvc->ExecQuery(bstr_t("WQL"),
                             bstr_t("SELECT ElementName, EnabledState FROM Msvm_ComputerSystem WHERE Caption='Virtual Machine'"),
                             WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                             NULL, &pEnumerator);

        if (SUCCEEDED(hr) && pEnumerator != NULL)
        {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;
            while (pEnumerator)
            {
                hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (uReturn == 0)
                    break;

                VARIANT vtName;
                VariantInit(&vtName);
                VARIANT vtState;
                VariantInit(&vtState);

                pclsObj->Get(L"ElementName", 0, &vtName, 0, 0);
                pclsObj->Get(L"EnabledState", 0, &vtState, 0, 0);

                if ((vtName.vt == VT_BSTR) && vtName.bstrVal != NULL)
                {
                    char line[512] = {0};
                    WideCharToMultiByte(CP_ACP, 0, vtName.bstrVal, -1, line, (int)sizeof(line), NULL, NULL);

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

                    CHyperVItemData* ext = new CHyperVItemData();
                    long st = 0;
                    if (vtState.vt == VT_I4) st = vtState.lVal;
                    else if (vtState.vt == VT_UI4) st = (long)vtState.ulVal;
                    ext->Running = (st == 2);
                    file.PluginData = reinterpret_cast<DWORD_PTR>(ext);
                    dir->AddFile(NULL, file, pluginData);
                }

                VariantClear(&vtName);
                VariantClear(&vtState);
                pclsObj->Release();
            }
            pEnumerator->Release();
        }

        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return TRUE;
    }

    virtual BOOL WINAPI TryCloseOrDetach(BOOL forceClose, BOOL canDetach, BOOL& detach, int reason) { (void)forceClose; (void)canDetach; (void)reason; detach = FALSE; return TRUE; }
    virtual void WINAPI Event(int event, DWORD param)
    {
        (void)param;
        if (event == FSE_ACTIVATEREFRESH || event == FSE_TIMER)
            SalamanderGeneral->PostRefreshPanelFS(this);
        if (event == FSE_OPENED || event == FSE_ATTACHED || event == FSE_TIMER)
            SalamanderGeneral->AddPluginFSTimer(3000, this, 1);
    }
    virtual void WINAPI ReleaseObject(HWND parent) { (void)parent; }
    virtual DWORD WINAPI GetSupportedServices() { return FS_SERVICE_CONTEXTMENU | FS_SERVICE_GETFSICON; }
    virtual BOOL WINAPI GetChangeDriveOrDisconnectItem(const char* fsName, char*& title, HICON& icon, BOOL& destroyIcon)
    {
        char text[2 * MAX_PATH + 32];
        text[0] = '\t';
        lstrcpynA(text + 1, fsName, _countof(text) - 1);

        if (Path[0] != '\0')
        {
            size_t currentLength = strlen(text);
            if (currentLength < _countof(text) - 1)
            {
                text[currentLength++] = ':';
                text[currentLength] = '\0';
            }
            size_t remaining = _countof(text) - currentLength;
            int copyLimit = remaining > static_cast<size_t>(INT_MAX) ? INT_MAX : static_cast<int>(remaining);
            lstrcpynA(text + currentLength, Path, copyLimit);
        }

        SalamanderGeneral->DuplicateAmpersands(text, _countof(text));
        title = SalamanderGeneral->DupStr(text);
        if (title == NULL)
            return FALSE;

        icon = LoadPluginIconResource(IDI_PLUGIN_MAIN, 16);
        destroyIcon = (icon != NULL);
        return TRUE;
    }
    virtual HICON WINAPI GetFSIcon(BOOL& destroyIcon) { destroyIcon = TRUE; return LoadPluginIconResource(IDI_PLUGIN_MAIN, 16); }
    virtual void WINAPI GetDropEffect(const char* srcFSPath, const char* tgtFSPath, DWORD allowedEffects, DWORD keyState, DWORD* dropEffect) { (void)srcFSPath; (void)tgtFSPath; (void)keyState; *dropEffect = allowedEffects & DROPEFFECT_COPY; }
    virtual void WINAPI GetFSFreeSpace(CQuadWord* retValue) { retValue->SetUI64(0); }
    virtual BOOL WINAPI GetNextDirectoryLineHotPath(const char* text, int pathLen, int& offset) { (void)text; (void)pathLen; (void)offset; return FALSE; }
    virtual void WINAPI CompleteDirectoryLineHotPath(char* path, int pathBufSize) { (void)path; (void)pathBufSize; }
    virtual BOOL WINAPI GetPathForMainWindowTitle(const char* fsName, int mode, char* buf, int bufSize) { (void)mode; _snprintf(buf, bufSize, "%s:%s", fsName, Path); return TRUE; }
    virtual void WINAPI ShowInfoDialog(const char* fsName, HWND parent) { (void)fsName; (void)parent; }
    virtual BOOL WINAPI ExecuteCommandLine(HWND parent, char* command, int& selFrom, int& selTo) { (void)parent; (void)command; (void)selFrom; (void)selTo; return FALSE; }
    virtual BOOL WINAPI QuickRename(const char* fsName, int mode, HWND parent, CFileData& file, BOOL isDir, char* newName, BOOL& cancel) { (void)fsName; (void)mode; (void)parent; (void)file; (void)isDir; (void)newName; cancel = FALSE; return FALSE; }
    virtual void WINAPI AcceptChangeOnPathNotification(const char* fsName, const char* path, BOOL includingSubdirs) { (void)fsName; (void)path; (void)includingSubdirs; SalamanderGeneral->PostRefreshPanelFS(this); }
    virtual BOOL WINAPI CreateDir(const char* fsName, int mode, HWND parent, char* newName, BOOL& cancel) { (void)fsName; (void)mode; (void)parent; (void)newName; cancel = FALSE; return FALSE; }
    virtual void WINAPI ViewFile(const char* fsName, HWND parent, CSalamanderForViewFileOnFSAbstract* salamander, CFileData& file) { (void)fsName; (void)parent; (void)salamander; (void)file; }
    virtual BOOL WINAPI Delete(const char* fsName, int mode, HWND parent, int panel, int selectedFiles, int selectedDirs, BOOL& cancelOrError) { (void)fsName; (void)mode; (void)parent; (void)panel; (void)selectedFiles; (void)selectedDirs; cancelOrError = FALSE; return FALSE; }
    virtual BOOL WINAPI CopyOrMoveFromFS(BOOL copy, int mode, const char* fsName, HWND parent, int panel, int selectedFiles, int selectedDirs, char* targetPath, BOOL& operationMask, BOOL& cancelOrHandlePath, HWND dropTarget) { (void)copy; (void)mode; (void)fsName; (void)parent; (void)panel; (void)selectedFiles; (void)selectedDirs; (void)targetPath; (void)operationMask; (void)dropTarget; cancelOrHandlePath = FALSE; return FALSE; }
    virtual BOOL WINAPI CopyOrMoveFromDiskToFS(BOOL copy, int mode, const char* fsName, HWND parent, const char* sourcePath, SalEnumSelection2 next, void* nextParam, int sourceFiles, int sourceDirs, char* targetPath, BOOL* invalidPathOrCancel) { (void)copy; (void)mode; (void)fsName; (void)parent; (void)sourcePath; (void)next; (void)nextParam; (void)sourceFiles; (void)sourceDirs; (void)targetPath; if (invalidPathOrCancel) *invalidPathOrCancel = FALSE; return FALSE; }
    virtual BOOL WINAPI ChangeAttributes(const char* fsName, HWND parent, int panel, int selectedFiles, int selectedDirs) { (void)fsName; (void)parent; (void)panel; (void)selectedFiles; (void)selectedDirs; return FALSE; }
    virtual void WINAPI ShowProperties(const char* fsName, HWND parent, int panel, int selectedFiles, int selectedDirs) { (void)fsName; (void)parent; (void)panel; (void)selectedFiles; (void)selectedDirs; }
    virtual void WINAPI ContextMenu(const char* fsName, HWND parent, int menuX, int menuY, int type, int panel, int selectedFiles, int selectedDirs)
    {
        (void)fsName;

        HMENU menu = CreatePopupMenu();
        if (menu == NULL)
            return;

        if (type == fscmPanel || type == fscmPathInPanel)
        {
            AppendMenuA(menu, MF_STRING, 2005, "Create New Machine");
            SetMenuItemIcon(menu, 2005, IDI_MENU_NEW_VM);
            UINT cmdIdOnly = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, menuX, menuY, 0, parent, NULL);
            DestroyMenu(menu);
            if (cmdIdOnly == 2005)
            {
                if (!LaunchVmCreate(parent))
                    SalamanderGeneral->SalMessageBox(parent, "Unable to start VMCreate.exe.", "Hyper-V Machines", MB_OK | MB_ICONERROR);
                SalamanderGeneral->PostRefreshPanelFS(this);
            }
            return;
        }

        if (type != fscmItemsInPanel)
        {
            DestroyMenu(menu);
            return;
        }

        int isDir = 0;
        const CFileData* item = NULL;
        if (selectedFiles == 0 && selectedDirs == 0)
            item = SalamanderGeneral->GetPanelFocusedItem(panel, &isDir);
        else
        {
            int idx = 0;
            item = SalamanderGeneral->GetPanelSelectedItem(panel, &idx, &isDir);
        }
        if (item == NULL || item->Name == NULL || item->Name[0] == 0)
            return;

        std::string vm = EscapePsSingleQuoted(item->Name);
        std::string state;
        QueryVmState(vm, state);
        bool running = (_stricmp(state.c_str(), "Running") == 0);

        const UINT ID_CONNECT = 2001;
        const UINT ID_START = 2002;
        const UINT ID_TURNOFF = 2003;
        const UINT ID_SHUTDOWN = 2004;
        const UINT ID_CREATE = 2005;

        AppendMenuA(menu, MF_STRING, ID_CONNECT, "Connect");
        SetMenuItemIcon(menu, ID_CONNECT, IDI_MENU_CONNECT);
        SetMenuDefaultItem(menu, ID_CONNECT, FALSE);
        AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
        if (running)
        {
            AppendMenuA(menu, MF_STRING, ID_TURNOFF, "Turn Off");
            AppendMenuA(menu, MF_STRING, ID_SHUTDOWN, "Shut Down");
        }
        else
        {
            AppendMenuA(menu, MF_STRING, ID_START, "Start");
        }
        AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
        AppendMenuA(menu, MF_STRING, ID_CREATE, "Create New Machine");
        SetMenuItemIcon(menu, ID_CREATE, IDI_MENU_NEW_VM);

        UINT cmdId = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, menuX, menuY, 0, parent, NULL);
        DestroyMenu(menu);
        if (cmdId == 0)
            return;

        bool success = false;
        if (cmdId == ID_CONNECT)
        {
            success = LaunchVmConnect(item->Name, parent);
        }
        else if (cmdId == ID_START)
        {
            success = RunHiddenPowerShell("$ErrorActionPreference='Stop'; Import-Module Hyper-V -ErrorAction Stop; Start-VM -Name '" + vm + "' -ErrorAction Stop");
        }
        else if (cmdId == ID_TURNOFF)
        {
            success = RunHiddenPowerShell("$ErrorActionPreference='Stop'; Import-Module Hyper-V -ErrorAction Stop; Stop-VM -Name '" + vm + "' -TurnOff -Force -ErrorAction Stop");
        }
        else if (cmdId == ID_SHUTDOWN)
        {
            success = RunHiddenPowerShell("$ErrorActionPreference='Stop'; Import-Module Hyper-V -ErrorAction Stop; Stop-VM -Name '" + vm + "' -ErrorAction Stop");
        }
        else if (cmdId == ID_CREATE)
        {
            success = LaunchVmCreate(parent);
        }

        if (!success)
        {
            SalamanderGeneral->SalMessageBox(parent, "Hyper-V command failed.", "Hyper-V Machines", MB_OK | MB_ICONERROR);
        }

        SalamanderGeneral->PostRefreshPanelFS(this);
    }
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
    virtual void WINAPI ExecuteOnFS(int panel, CPluginFSInterfaceAbstract* pluginFS, const char* pluginFSName, int pluginFSNameIndex, CFileData& file, int isDir)
    {
        (void)panel;
        (void)pluginFS;
        (void)pluginFSName;
        (void)pluginFSNameIndex;

        if (isDir != 0 || file.Name == NULL || file.Name[0] == 0)
            return;

        bool success = LaunchVmConnect(file.Name, SalamanderGeneral->GetMainWindowHWND());
        if (!success)
        {
            SalamanderGeneral->SalMessageBox(SalamanderGeneral->GetMainWindowHWND(),
                                             "Unable to open VM desktop session.",
                                             "Hyper-V Machines",
                                             MB_OK | MB_ICONERROR);
        }
    }
    virtual BOOL WINAPI DisconnectFS(HWND parent, BOOL isInPanel, int panel, CPluginFSInterfaceAbstract* pluginFS, const char* pluginFSName, int pluginFSNameIndex) { (void)isInPanel; (void)panel; (void)pluginFSName; (void)pluginFSNameIndex; SalamanderGeneral->CloseDetachedFS(parent, pluginFS); return TRUE; }
    virtual void WINAPI ConvertPathToInternal(const char* fsName, int fsNameIndex, char* fsUserPart) { (void)fsName; (void)fsNameIndex; (void)fsUserPart; }
    virtual void WINAPI ConvertPathToExternal(const char* fsName, int fsNameIndex, char* fsUserPart) { (void)fsName; (void)fsNameIndex; (void)fsUserPart; }
    virtual void WINAPI EnsureShareExistsOnServer(int panel, const char* server, const char* share) { (void)panel; (void)server; (void)share; }
};

CHyperVFSInterface gHyperVFSInterface;
CPluginInterfaceForFSAbstract* gHyperVFSInterfacePtr = &gHyperVFSInterface;
