#include "precomp.h"
#include "eventlogmodel.h"
#include <climits>

CPluginInterface PluginInterface;
CPluginInterfaceForFS InterfaceForFS;

HINSTANCE DLLInstance = NULL;
HINSTANCE HLanguage = NULL;

CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
CSalamanderGUIAbstract* SalamanderGUI = NULL;
CSalamanderDebugAbstract* SalamanderDebug = NULL;

int SalamanderVersion = 0;

char AssignedFSName[MAX_PATH] = "";

namespace
{
struct LogDefinition
{
    int DisplayNameResId;
    const wchar_t* LogName;
};

const LogDefinition kLogDefinitions[] = {
    {IDS_LOG_APPLICATION, L"Application"},
    {IDS_LOG_SECURITY, L"Security"},
    {IDS_LOG_SETUP, L"Setup"},
    {IDS_LOG_SYSTEM, L"System"},
    {IDS_LOG_FORWARD, L"ForwardedEvents"},
};

const size_t kMaxEventsToDisplay = 512;

HIMAGELIST gEventImageList = NULL;
int gCurrentImageListSize = 0;

int IconSizeToPixels(int iconSize)
{
    switch (iconSize)
    {
    case SALICONSIZE_48:
        return 48;
    case SALICONSIZE_32:
        return 32;
    default:
        return 16;
    }
}

BOOL EnsureEventImageList(int iconSize)
{
    if (gEventImageList != NULL && gCurrentImageListSize == iconSize)
        return TRUE;

    if (gEventImageList != NULL)
    {
        ImageList_Destroy(gEventImageList);
        gEventImageList = NULL;
        gCurrentImageListSize = 0;
    }

    const int pixels = IconSizeToPixels(iconSize);
    HIMAGELIST list = ImageList_Create(pixels, pixels, ILC_COLOR32 | ILC_MASK, 1, 0);
    if (list == NULL)
        return FALSE;

    ImageList_SetImageCount(list, 1);

    HICON icon = reinterpret_cast<HICON>(LoadImage(NULL, MAKEINTRESOURCE(IDI_INFORMATION), IMAGE_ICON, pixels, pixels,
                                                  LR_SHARED));
    if (icon == NULL)
    {
        ImageList_Destroy(list);
        return FALSE;
    }

    ImageList_ReplaceIcon(list, 0, icon);
    ImageList_SetBkColor(list, SalamanderGeneral != NULL ? SalamanderGeneral->GetCurrentColor(SALCOL_ITEM_BK_NORMAL)
                                                         : CLR_NONE);

    gEventImageList = list;
    gCurrentImageListSize = iconSize;
    return TRUE;
}

bool SetEventViewerIconList(CSalamanderConnectAbstract* salamander)
{
    if (SalamanderGUI == NULL)
        return false;

    CGUIIconListAbstract* iconList = SalamanderGUI->CreateIconList();
    if (iconList == NULL)
        return false;

    if (!iconList->Create(16, 16, 1))
    {
        SalamanderGUI->DestroyIconList(iconList);
        return false;
    }

    HICON icon = reinterpret_cast<HICON>(LoadImage(NULL, MAKEINTRESOURCE(IDI_INFORMATION), IMAGE_ICON, 16, 16, LR_SHARED));
    if (icon == NULL)
    {
        SalamanderGUI->DestroyIconList(iconList);
        return false;
    }

    iconList->ReplaceIcon(0, icon);
    salamander->SetIconListForGUI(iconList);
    return true;
}

struct EventItemData
{
    std::string LogName;
    EventLogRecord Record;
};

const EventItemData* GetCurrentEventItem(const CFileData* file)
{
    if (file == NULL || file->PluginData == 0)
        return NULL;
    return reinterpret_cast<const EventItemData*>(file->PluginData);
}

} // namespace

HINSTANCE GetLanguageResourceHandle()
{
    return HLanguage != NULL ? HLanguage : DLLInstance;
}

char* LoadStr(int resID)
{
    if (SalamanderGeneral == NULL)
        return (char*)"";

    char* text = SalamanderGeneral->LoadStr(GetLanguageResourceHandle(), resID);
    return text != NULL ? text : (char*)"";
}

BOOL InitFS()
{
    if (!EnsureEventImageList(SALICONSIZE_16))
    {
        if (SalamanderDebug != NULL)
        {
            SalamanderDebug->TraceI(__FILE__, __LINE__,
                                    "EventViewer: unable to initialize icon list for Event Logs.");
        }
        return FALSE;
    }

    return TRUE;
}

void ReleaseFS()
{
    if (gEventImageList != NULL)
    {
        ImageList_Destroy(gEventImageList);
        gEventImageList = NULL;
        gCurrentImageListSize = 0;
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DLLInstance = hinstDLL;

        INITCOMMONCONTROLSEX initCtrls;
        initCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
        initCtrls.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES;
        if (!InitCommonControlsEx(&initCtrls))
        {
            return FALSE;
        }
    }

    return TRUE;
}

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    SalamanderDebug = salamander->GetSalamanderDebug();
    SalamanderVersion = salamander->GetVersion();
    HANDLES_CAN_USE_TRACE();

    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
    {
        MessageBox(salamander->GetParentWindow(), REQUIRE_LAST_VERSION_OF_SALAMANDER, VERSINFO_PLUGINNAME,
                   MB_OK | MB_ICONERROR);
        return NULL;
    }

    SalamanderGeneral = salamander->GetSalamanderGeneral();
    SalamanderGUI = salamander->GetSalamanderGUI();

    HLanguage = salamander->LoadLanguageModule(salamander->GetParentWindow(), "EventViewer");
    if (HLanguage == NULL)
        return NULL;

    SalamanderGeneral->SetHelpFileName("eventviewer.chm");

    salamander->SetBasicPluginData(LoadStr(IDS_PLUGINNAME), FUNCTION_FILESYSTEM, VERSINFO_VERSION_NO_PLATFORM,
                                   VERSINFO_COPYRIGHT, LoadStr(IDS_PLUGIN_DESCRIPTION), "EVENTVIEWER", NULL, "evlog");

    salamander->SetPluginHomePageURL(LoadStr(IDS_PLUGIN_HOME));

    SalamanderGeneral->GetPluginFSName(AssignedFSName, 0);

    if (!InitFS())
        return NULL;

    return &PluginInterface;
}

int WINAPI SalamanderPluginGetReqVer()
{
    return LAST_VERSION_OF_SALAMANDER;
}

void WINAPI CPluginInterface::About(HWND parent)
{
    char buf[1024];
    _snprintf_s(buf, _TRUNCATE, "%s " VERSINFO_VERSION "\n\n" VERSINFO_COPYRIGHT "\n\n%s",
                LoadStr(IDS_PLUGINNAME), LoadStr(IDS_PLUGIN_DESCRIPTION));
    SalamanderGeneral->ShowMessageBox(buf, LoadStr(IDS_ABOUT), MSGBOX_INFO);
}

void WINAPI CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    char changeDriveLabel[256];
    _snprintf_s(changeDriveLabel, _TRUNCATE, "\t%s", LoadStr(IDS_EVENT_VIEWER_MENU));
    salamander->SetChangeDriveMenuItem(changeDriveLabel, 0);

    if (!SetEventViewerIconList(salamander))
    {
        if (SalamanderDebug != NULL)
        {
            SalamanderDebug->TraceI(__FILE__, __LINE__,
                                    "EventViewer: unable to assign custom icon list; using defaults.");
        }
    }

    salamander->SetPluginIcon(0);
    salamander->SetPluginMenuAndToolbarIcon(0);
}

void WINAPI CPluginInterface::Event(int event, DWORD param)
{
    UNREFERENCED_PARAMETER(event);
    UNREFERENCED_PARAMETER(param);
}

BOOL WINAPI CPluginInterface::Release(HWND parent, BOOL force)
{
    ReleaseFS();
    return TRUE;
}

void WINAPI CPluginInterface::LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(regKey);
    UNREFERENCED_PARAMETER(registry);
}

void WINAPI CPluginInterface::SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(regKey);
    UNREFERENCED_PARAMETER(registry);
}

void WINAPI CPluginInterface::Configuration(HWND parent)
{
    SalamanderGeneral->ShowMessageBox(LoadStr(IDS_NO_CONFIGURATION), LoadStr(IDS_PLUGINNAME), MSGBOX_INFO);
}

void WINAPI CPluginInterface::ClearHistory(HWND parent)
{
    UNREFERENCED_PARAMETER(parent);
}

void WINAPI CPluginInterface::ReleasePluginDataInterface(CPluginDataInterfaceAbstract* pluginData)
{
    delete pluginData;
}

CPluginInterfaceForFSAbstract* WINAPI CPluginInterface::GetInterfaceForFS()
{
    return &InterfaceForFS;
}

CPluginInterfaceForFS::CPluginInterfaceForFS()
{
}

CPluginFSInterfaceAbstract* WINAPI CPluginInterfaceForFS::OpenFS(const char* fsName, int fsNameIndex)
{
    UNREFERENCED_PARAMETER(fsName);
    UNREFERENCED_PARAMETER(fsNameIndex);
    return new CEventViewerFSInterface();
}

void WINAPI CPluginInterfaceForFS::CloseFS(CPluginFSInterfaceAbstract* fs)
{
    delete fs;
}

void WINAPI CPluginInterfaceForFS::ExecuteChangeDriveMenuItem(int panel)
{
    UNREFERENCED_PARAMETER(panel);

    int failReason = 0;
    SalamanderGeneral->ChangePanelPathToPluginFS(PANEL_SOURCE, AssignedFSName, "", &failReason);
}

BOOL WINAPI CPluginInterfaceForFS::ChangeDriveMenuItemContextMenu(HWND parent, int panel, int x, int y,
                                                                 CPluginFSInterfaceAbstract* pluginFS,
                                                                 const char* pluginFSName, int pluginFSNameIndex,
                                                                 BOOL isDetachedFS, BOOL& refreshMenu,
                                                                 BOOL& closeMenu, int& postCmd, void*& postCmdParam)
{
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(panel);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(pluginFS);
    UNREFERENCED_PARAMETER(pluginFSName);
    UNREFERENCED_PARAMETER(pluginFSNameIndex);
    UNREFERENCED_PARAMETER(isDetachedFS);
    UNREFERENCED_PARAMETER(refreshMenu);
    UNREFERENCED_PARAMETER(closeMenu);
    UNREFERENCED_PARAMETER(postCmd);
    UNREFERENCED_PARAMETER(postCmdParam);
    return FALSE;
}

void WINAPI CPluginInterfaceForFS::ExecuteChangeDrivePostCommand(int panel, int postCmd, void* postCmdParam)
{
    UNREFERENCED_PARAMETER(panel);
    UNREFERENCED_PARAMETER(postCmd);
    UNREFERENCED_PARAMETER(postCmdParam);
}

void WINAPI CPluginInterfaceForFS::ExecuteOnFS(int panel, CPluginFSInterfaceAbstract* pluginFS,
                                               const char* pluginFSName, int pluginFSNameIndex,
                                               CFileData& file, int isDir)
{
    UNREFERENCED_PARAMETER(panel);
    UNREFERENCED_PARAMETER(pluginFS);
    UNREFERENCED_PARAMETER(pluginFSName);
    UNREFERENCED_PARAMETER(pluginFSNameIndex);
    UNREFERENCED_PARAMETER(file);
    UNREFERENCED_PARAMETER(isDir);
}

BOOL WINAPI CPluginInterfaceForFS::DisconnectFS(HWND parent, BOOL isInPanel, int panel,
                                                CPluginFSInterfaceAbstract* pluginFS,
                                                const char* pluginFSName, int pluginFSNameIndex)
{
    UNREFERENCED_PARAMETER(isInPanel);
    UNREFERENCED_PARAMETER(panel);
    UNREFERENCED_PARAMETER(pluginFS);
    UNREFERENCED_PARAMETER(pluginFSName);
    UNREFERENCED_PARAMETER(pluginFSNameIndex);
    SalamanderGeneral->CloseDetachedFS(parent, pluginFS);
    return TRUE;
}

CEventViewerFSInterface::CEventViewerFSInterface()
    : PathError(false)
    , FatalError(false)
    , Reader(new EventLogReader())
{
    Path[0] = '\\';
    Path[1] = '\0';
}

CEventViewerFSInterface::~CEventViewerFSInterface()
{
    delete Reader;
    Reader = NULL;
}

void CEventViewerFSInterface::SetCurrentLog(const std::wstring& logName)
{
    CurrentLog = logName;

    Path[0] = '\\';
    Path[1] = '\0';
    if (!CurrentLog.empty())
    {
        WideCharToMultiByte(CP_ACP, 0, CurrentLog.c_str(), -1, Path + 1, static_cast<int>(sizeof(Path) - 1), NULL, NULL);
    }
}

BOOL WINAPI CEventViewerFSInterface::GetCurrentPath(char* userPart)
{
    if (userPart == NULL)
        return FALSE;

    if (CurrentLog.empty())
    {
        strcpy(userPart, "\\");
    }
    else
    {
        char buffer[MAX_PATH];
        buffer[0] = '\\';
        buffer[1] = '\0';
        WideCharToMultiByte(CP_ACP, 0, CurrentLog.c_str(), -1, buffer + 1, MAX_PATH - 1, NULL, NULL);
        strncpy(userPart, buffer, MAX_PATH);
        userPart[MAX_PATH - 1] = '\0';
    }
    return TRUE;
}

BOOL WINAPI CEventViewerFSInterface::GetFullName(CFileData& file, int isDir, char* buf, int bufSize)
{
    UNREFERENCED_PARAMETER(file);
    UNREFERENCED_PARAMETER(isDir);
    UNREFERENCED_PARAMETER(buf);
    UNREFERENCED_PARAMETER(bufSize);
    return TRUE;
}

BOOL WINAPI CEventViewerFSInterface::GetFullFSPath(HWND parent, const char* fsName, char* path, int pathSize, BOOL& success)
{
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(fsName);
    UNREFERENCED_PARAMETER(pathSize);

    if (path != NULL)
        path[0] = '\0';

    success = FALSE;
    return TRUE;
}

BOOL WINAPI CEventViewerFSInterface::GetRootPath(char* userPart)
{
    if (userPart == NULL)
        return FALSE;
    strcpy(userPart, "\\");
    return TRUE;
}

BOOL WINAPI CEventViewerFSInterface::IsCurrentPath(int currentFSNameIndex, int fsNameIndex, const char* userPart)
{
    UNREFERENCED_PARAMETER(currentFSNameIndex);
    UNREFERENCED_PARAMETER(fsNameIndex);

    if (userPart == NULL)
        return CurrentLog.empty();

    if (CurrentLog.empty())
        return strcmp(userPart, "\\") == 0 || userPart[0] == '\0';

    char buffer[MAX_PATH];
    buffer[0] = '\\';
    buffer[1] = '\0';
    WideCharToMultiByte(CP_ACP, 0, CurrentLog.c_str(), -1, buffer + 1, MAX_PATH - 1, NULL, NULL);
    return _stricmp(userPart, buffer) == 0;
}

BOOL WINAPI CEventViewerFSInterface::IsOurPath(int currentFSNameIndex, int fsNameIndex, const char* userPart)
{
    UNREFERENCED_PARAMETER(currentFSNameIndex);
    UNREFERENCED_PARAMETER(fsNameIndex);
    UNREFERENCED_PARAMETER(userPart);
    return TRUE;
}

BOOL WINAPI CEventViewerFSInterface::ChangePath(int currentFSNameIndex, char* fsName, int fsNameIndex,
                                                const char* userPart, char* cutFileName, BOOL* pathWasCut,
                                                BOOL forceRefresh, int mode)
{
    UNREFERENCED_PARAMETER(currentFSNameIndex);
    UNREFERENCED_PARAMETER(fsName);
    UNREFERENCED_PARAMETER(fsNameIndex);
    UNREFERENCED_PARAMETER(cutFileName);
    UNREFERENCED_PARAMETER(pathWasCut);
    UNREFERENCED_PARAMETER(forceRefresh);
    UNREFERENCED_PARAMETER(mode);

    std::wstring newLog;

    if (userPart != NULL)
    {
        const char* trimmed = userPart;
        while (*trimmed == '\\')
            ++trimmed;

        if (*trimmed != '\0')
        {
            int required = MultiByteToWideChar(CP_ACP, 0, trimmed, -1, NULL, 0);
            if (required > 0)
            {
                std::vector<wchar_t> buffer(static_cast<size_t>(required));
                if (MultiByteToWideChar(CP_ACP, 0, trimmed, -1, buffer.data(), required) > 0)
                {
                    newLog.assign(buffer.data());
                }
            }
        }
    }

    if (!newLog.empty())
    {
        bool known = false;
        for (const LogDefinition& def : kLogDefinitions)
        {
            if (_wcsicmp(def.LogName, newLog.c_str()) == 0)
            {
                known = true;
                break;
            }
        }
        if (!known)
        {
            PathError = true;
            return FALSE;
        }
    }

    SetCurrentLog(newLog);
    PathError = false;
    FatalError = false;
    CachedRecords.clear();
    return TRUE;
}

BOOL CEventViewerFSInterface::ListLogs(CSalamanderDirectoryAbstract* dir,
                                       CPluginDataInterfaceAbstract*& pluginData,
                                       int& iconsType)
{
    dir->Clear(NULL);
    dir->SetValidData(VALID_DATA_NONE);

    pluginData = new CPluginFSDataInterface(Path, std::wstring());

    CFileData file;
    memset(&file, 0, sizeof(file));
    file.Size = CQuadWord(0, 0);
    file.Hidden = 0;
    file.Attr = FILE_ATTRIBUTE_DIRECTORY;

    for (const LogDefinition& def : kLogDefinitions)
    {
        char* name = LoadStr(def.DisplayNameResId);
        if (name == NULL)
            continue;

        file.Name = SalamanderGeneral->DupStr(name);
        if (file.Name == NULL)
            continue;

        file.NameLen = static_cast<int>(strlen(file.Name));
        file.Ext = file.Name + file.NameLen;
        file.PluginData = 0;

        dir->AddDir(NULL, file, pluginData);
    }

    iconsType = pitFromPlugin;
    return TRUE;
}

BOOL CEventViewerFSInterface::EnsureEventDataLoaded(BOOL forceRefresh, std::string& errorMessage)
{
    if (CurrentLog.empty())
        return TRUE;

    if (!forceRefresh && !CachedRecords.empty())
        return TRUE;

    CachedRecords.clear();
    if (Reader == NULL)
        Reader = new EventLogReader();

    bool ok = Reader->Query(CurrentLog.c_str(), kMaxEventsToDisplay, CachedRecords, errorMessage);
    return ok;
}

BOOL CEventViewerFSInterface::ListLogRecords(CSalamanderDirectoryAbstract* dir,
                                             CPluginDataInterfaceAbstract*& pluginData,
                                             int& iconsType, BOOL forceRefresh)
{
    dir->Clear(NULL);
    dir->SetValidData(VALID_DATA_NONE);

    pluginData = new CPluginFSDataInterface(Path, CurrentLog);

    std::string errorMessage;
    if (!EnsureEventDataLoaded(forceRefresh, errorMessage))
    {
        if (!errorMessage.empty())
        {
            SalamanderGeneral->ShowMessageBox(errorMessage.c_str(), LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
        }
        return TRUE;
    }

    iconsType = pitFromPlugin;

    CFileData file;
    memset(&file, 0, sizeof(file));
    file.Size = CQuadWord(0, 0);
    file.Hidden = 0;
    file.Attr = 0;

    std::string currentLogAnsi = WideToAnsi(CurrentLog);

    for (size_t index = 0; index < CachedRecords.size(); ++index)
    {
        const EventLogRecord& record = CachedRecords[index];

        char displayName[256];
        if (!record.EventId.empty())
        {
            _snprintf_s(displayName, _TRUNCATE, "Event %s", record.EventId.c_str());
        }
        else
        {
            _snprintf_s(displayName, _TRUNCATE, "Event %zu", index + 1);
        }

        if (!record.Source.empty())
        {
            size_t len = strlen(displayName);
            _snprintf_s(displayName + len, sizeof(displayName) - len, _TRUNCATE, " - %s", record.Source.c_str());
        }

        file.Name = SalamanderGeneral->DupStr(displayName);
        if (file.Name == NULL)
            continue;

        file.NameLen = static_cast<int>(strlen(file.Name));
        file.Ext = file.Name + file.NameLen;

        EventItemData* data = new EventItemData();
        data->LogName = currentLogAnsi;
        data->Record = record;
        file.PluginData = reinterpret_cast<DWORD_PTR>(data);

        dir->AddFile(NULL, file, pluginData);
    }

    return TRUE;
}

BOOL WINAPI CEventViewerFSInterface::ListCurrentPath(CSalamanderDirectoryAbstract* dir,
                                                     CPluginDataInterfaceAbstract*& pluginData,
                                                     int& iconsType, BOOL forceRefresh)
{
    if (CurrentLog.empty())
        return ListLogs(dir, pluginData, iconsType);

    return ListLogRecords(dir, pluginData, iconsType, forceRefresh);
}

BOOL WINAPI CEventViewerFSInterface::TryCloseOrDetach(BOOL forceClose, BOOL canDetach, BOOL& detach, int reason)
{
    UNREFERENCED_PARAMETER(forceClose);
    UNREFERENCED_PARAMETER(canDetach);
    UNREFERENCED_PARAMETER(reason);
    detach = FALSE;
    return TRUE;
}

void WINAPI CEventViewerFSInterface::Event(int event, DWORD param)
{
    UNREFERENCED_PARAMETER(event);
    UNREFERENCED_PARAMETER(param);
}

void WINAPI CEventViewerFSInterface::ReleaseObject(HWND parent)
{
    UNREFERENCED_PARAMETER(parent);
}

DWORD WINAPI CEventViewerFSInterface::GetSupportedServices()
{
    return 0;
}

BOOL WINAPI CEventViewerFSInterface::GetChangeDriveOrDisconnectItem(const char* fsName, char*& title, HICON& icon,
                                                                    BOOL& destroyIcon)
{
    UNREFERENCED_PARAMETER(fsName);
    title = LoadStr(IDS_EVENT_VIEWER_MENU);
    icon = reinterpret_cast<HICON>(LoadImage(NULL, MAKEINTRESOURCE(IDI_INFORMATION), IMAGE_ICON, 16, 16, LR_SHARED));
    destroyIcon = FALSE;
    return TRUE;
}

HICON WINAPI CEventViewerFSInterface::GetFSIcon(BOOL& destroyIcon)
{
    destroyIcon = FALSE;
    return reinterpret_cast<HICON>(LoadImage(NULL, MAKEINTRESOURCE(IDI_INFORMATION), IMAGE_ICON, 16, 16, LR_SHARED));
}

void WINAPI CEventViewerFSInterface::GetDropEffect(const char* srcFSPath, const char* tgtFSPath,
                                                   DWORD allowedEffects, DWORD keyState, DWORD* dropEffect)
{
    UNREFERENCED_PARAMETER(srcFSPath);
    UNREFERENCED_PARAMETER(tgtFSPath);
    UNREFERENCED_PARAMETER(allowedEffects);
    UNREFERENCED_PARAMETER(keyState);
    if (dropEffect != NULL)
        *dropEffect = DROPEFFECT_NONE;
}

void WINAPI CEventViewerFSInterface::GetFSFreeSpace(CQuadWord* retValue)
{
    if (retValue != NULL)
        retValue->Set(0, 0);
}

BOOL WINAPI CEventViewerFSInterface::GetNextDirectoryLineHotPath(const char* text, int pathLen, int& offset)
{
    UNREFERENCED_PARAMETER(text);
    UNREFERENCED_PARAMETER(pathLen);
    UNREFERENCED_PARAMETER(offset);
    return FALSE;
}

BOOL WINAPI CEventViewerFSInterface::GetPathForMainWindowTitle(const char* fsName, int mode, char* buf, int bufSize)
{
    UNREFERENCED_PARAMETER(fsName);
    UNREFERENCED_PARAMETER(mode);

    if (buf != NULL && bufSize > 0)
    {
        if (CurrentLog.empty())
            strncpy(buf, LoadStr(IDS_EVENT_VIEWER_MENU), bufSize);
        else
        {
            char logBuffer[MAX_PATH];
            WideCharToMultiByte(CP_ACP, 0, CurrentLog.c_str(), -1, logBuffer, MAX_PATH, NULL, NULL);
            _snprintf_s(buf, bufSize, _TRUNCATE, "%s - %s", LoadStr(IDS_EVENT_VIEWER_MENU), logBuffer);
        }
        buf[bufSize - 1] = '\0';
    }
    return TRUE;
}

void WINAPI CEventViewerFSInterface::ShowInfoDialog(const char* fsName, HWND parent)
{
    UNREFERENCED_PARAMETER(fsName);
    SalamanderGeneral->ShowMessageBox(LoadStr(IDS_PLUGIN_DESCRIPTION), LoadStr(IDS_PLUGINNAME), MSGBOX_INFO);
}

BOOL WINAPI CEventViewerFSInterface::ExecuteCommandLine(HWND parent, char* command, int& selFrom, int& selTo)
{
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(command);
    UNREFERENCED_PARAMETER(selFrom);
    UNREFERENCED_PARAMETER(selTo);
    return FALSE;
}

BOOL WINAPI CEventViewerFSInterface::QuickRename(const char* fsName, int mode, HWND parent, CFileData& file, BOOL isDir,
                                                 char* newName, BOOL& cancel)
{
    UNREFERENCED_PARAMETER(fsName);
    UNREFERENCED_PARAMETER(mode);
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(file);
    UNREFERENCED_PARAMETER(isDir);
    UNREFERENCED_PARAMETER(newName);
    cancel = TRUE;
    return FALSE;
}

BOOL WINAPI CEventViewerFSInterface::CreateDir(const char* fsName, int mode, HWND parent, char* newName, BOOL& cancel)
{
    UNREFERENCED_PARAMETER(fsName);
    UNREFERENCED_PARAMETER(mode);
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(newName);
    cancel = TRUE;
    return FALSE;
}

void WINAPI CEventViewerFSInterface::ViewFile(const char* fsName, HWND parent,
                                              CSalamanderForViewFileOnFSAbstract* salamander,
                                              CFileData& file)
{
    UNREFERENCED_PARAMETER(fsName);
    UNREFERENCED_PARAMETER(salamander);

    const EventItemData* data = GetCurrentEventItem(&file);
    if (data == NULL)
        return;

    const std::string& details = data->Record.Details;
    const char* text = details.empty() ? LoadStr(IDS_EVENT_DETAILS_NOT_AVAILABLE) : details.c_str();
    SalamanderGeneral->ShowMessageBox(text, LoadStr(IDS_PLUGINNAME), MSGBOX_INFO);
}

BOOL WINAPI CEventViewerFSInterface::Delete(const char* fsName, int mode, HWND parent, int panel,
                                            int selectedFiles, int selectedDirs, BOOL& cancelOrError)
{
    UNREFERENCED_PARAMETER(fsName);
    UNREFERENCED_PARAMETER(mode);
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(panel);
    UNREFERENCED_PARAMETER(selectedFiles);
    UNREFERENCED_PARAMETER(selectedDirs);
    cancelOrError = TRUE;
    return FALSE;
}

BOOL WINAPI CEventViewerFSInterface::CopyOrMoveFromFS(BOOL copy, int mode, const char* fsName, HWND parent,
                                                      int panel, int selectedFiles, int selectedDirs,
                                                      char* targetPath, BOOL& operationMask,
                                                      BOOL& cancelOrHandlePath, HWND dropTarget)
{
    UNREFERENCED_PARAMETER(copy);
    UNREFERENCED_PARAMETER(mode);
    UNREFERENCED_PARAMETER(fsName);
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(panel);
    UNREFERENCED_PARAMETER(selectedFiles);
    UNREFERENCED_PARAMETER(selectedDirs);
    UNREFERENCED_PARAMETER(targetPath);
    UNREFERENCED_PARAMETER(operationMask);
    UNREFERENCED_PARAMETER(dropTarget);
    cancelOrHandlePath = TRUE;
    return FALSE;
}

BOOL WINAPI CEventViewerFSInterface::CopyOrMoveFromDiskToFS(BOOL copy, int mode, const char* fsName, HWND parent,
                                                            const char* sourcePath, SalEnumSelection2 next,
                                                            void* nextParam, int sourceFiles, int sourceDirs,
                                                            char* targetPath, BOOL* invalidPathOrCancel)
{
    UNREFERENCED_PARAMETER(copy);
    UNREFERENCED_PARAMETER(mode);
    UNREFERENCED_PARAMETER(fsName);
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(sourcePath);
    UNREFERENCED_PARAMETER(next);
    UNREFERENCED_PARAMETER(nextParam);
    UNREFERENCED_PARAMETER(sourceFiles);
    UNREFERENCED_PARAMETER(sourceDirs);
    UNREFERENCED_PARAMETER(targetPath);
    if (invalidPathOrCancel != NULL)
        *invalidPathOrCancel = TRUE;
    return FALSE;
}

BOOL WINAPI CEventViewerFSInterface::ChangeAttributes(const char* fsName, HWND parent, int panel,
                                                      int selectedFiles, int selectedDirs)
{
    UNREFERENCED_PARAMETER(fsName);
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(panel);
    UNREFERENCED_PARAMETER(selectedFiles);
    UNREFERENCED_PARAMETER(selectedDirs);
    return FALSE;
}

void WINAPI CEventViewerFSInterface::ShowProperties(const char* fsName, HWND parent, int panel,
                                                    int selectedFiles, int selectedDirs)
{
    UNREFERENCED_PARAMETER(fsName);
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(panel);
    UNREFERENCED_PARAMETER(selectedFiles);
    UNREFERENCED_PARAMETER(selectedDirs);
}

void WINAPI CEventViewerFSInterface::ContextMenu(const char* fsName, HWND parent, int menuX, int menuY, int type,
                                                 int panel, int selectedFiles, int selectedDirs)
{
    UNREFERENCED_PARAMETER(fsName);
    UNREFERENCED_PARAMETER(parent);
    UNREFERENCED_PARAMETER(menuX);
    UNREFERENCED_PARAMETER(menuY);
    UNREFERENCED_PARAMETER(type);
    UNREFERENCED_PARAMETER(panel);
    UNREFERENCED_PARAMETER(selectedFiles);
    UNREFERENCED_PARAMETER(selectedDirs);
}

namespace
{
const CFileData** gTransferFileData = NULL;
int* gTransferIsDir = NULL;
char* gTransferBuffer = NULL;
int* gTransferLen = NULL;
DWORD* gTransferRowData = NULL;
CPluginDataInterfaceAbstract** gTransferPluginDataIface = NULL;
DWORD* gTransferActCustomData = NULL;

enum ColumnId : DWORD
{
    kColumnLevel = 1,
    kColumnTime = 2,
    kColumnSource = 3,
    kColumnEventId = 4,
    kColumnTask = 5,
};

void CopyColumnText(const std::string& text)
{
    if (gTransferBuffer == NULL || gTransferLen == NULL)
        return;

    const size_t len = text.size();
    *gTransferLen = static_cast<int>(len > static_cast<size_t>(INT_MAX) ? INT_MAX : len);
    if (*gTransferLen > 0)
        memcpy(gTransferBuffer, text.data(), static_cast<size_t>(*gTransferLen));
}

void WINAPI GetLevelText()
{
    const EventItemData* data = GetCurrentEventItem(gTransferFileData != NULL ? *gTransferFileData : NULL);
    CopyColumnText(data != NULL ? data->Record.Level : std::string());
}

void WINAPI GetTimeText()
{
    const EventItemData* data = GetCurrentEventItem(gTransferFileData != NULL ? *gTransferFileData : NULL);
    CopyColumnText(data != NULL ? data->Record.TimeCreated : std::string());
}

void WINAPI GetSourceText()
{
    const EventItemData* data = GetCurrentEventItem(gTransferFileData != NULL ? *gTransferFileData : NULL);
    CopyColumnText(data != NULL ? data->Record.Source : std::string());
}

void WINAPI GetEventIdText()
{
    const EventItemData* data = GetCurrentEventItem(gTransferFileData != NULL ? *gTransferFileData : NULL);
    CopyColumnText(data != NULL ? data->Record.EventId : std::string());
}

void WINAPI GetTaskText()
{
    const EventItemData* data = GetCurrentEventItem(gTransferFileData != NULL ? *gTransferFileData : NULL);
    CopyColumnText(data != NULL ? data->Record.TaskCategory : std::string());
}

void AddColumn(BOOL leftPanel, CSalamanderViewAbstract* view, int& index, int resId,
               DWORD columnId, void (WINAPI* getText)())
{
    UNREFERENCED_PARAMETER(leftPanel);

    CColumn column = {};
    strcpy(column.Name, LoadStr(resId));
    strcpy(column.Description, LoadStr(resId));
    column.GetText = getText;
    column.CustomData = columnId;
    column.LeftAlignment = 1;
    column.SupportSorting = 1;
    column.ID = COLUMN_ID_CUSTOM;
    column.Width = 0;
    column.FixedWidth = 0;
    view->InsertColumn(++index, &column);
}
} // namespace

CPluginFSDataInterface::CPluginFSDataInterface(const char* path, const std::wstring& logName)
    : LogName(logName)
{
    if (path != NULL)
    {
        strncpy(Path, path, MAX_PATH);
        Path[MAX_PATH - 1] = '\0';
    }
    else
    {
        Path[0] = '\0';
    }

    SalamanderGeneral->SalPathAddBackslash(Path, MAX_PATH);
}

void WINAPI CPluginFSDataInterface::ReleasePluginData(CFileData& file, BOOL isDir)
{
    UNREFERENCED_PARAMETER(isDir);
    if (file.PluginData != 0)
    {
        delete reinterpret_cast<EventItemData*>(file.PluginData);
        file.PluginData = 0;
    }
}

HIMAGELIST WINAPI CPluginFSDataInterface::GetSimplePluginIcons(int iconSize)
{
    if (!EnsureEventImageList(iconSize))
        return NULL;
    return gEventImageList;
}

BOOL WINAPI CPluginFSDataInterface::HasSimplePluginIcon(CFileData& file, BOOL isDir)
{
    UNREFERENCED_PARAMETER(file);
    UNREFERENCED_PARAMETER(isDir);
    return TRUE;
}

HICON WINAPI CPluginFSDataInterface::GetPluginIcon(const CFileData* file, int iconSize, BOOL& destroyIcon)
{
    UNREFERENCED_PARAMETER(file);
    UNREFERENCED_PARAMETER(iconSize);
    destroyIcon = FALSE;
    return NULL;
}

int WINAPI CPluginFSDataInterface::CompareFilesFromFS(const CFileData* file1, const CFileData* file2)
{
    const EventItemData* data1 = GetCurrentEventItem(file1);
    const EventItemData* data2 = GetCurrentEventItem(file2);

    if (data1 == NULL || data2 == NULL)
        return 0;

    return _stricmp(data1->Record.TimeCreated.c_str(), data2->Record.TimeCreated.c_str());
}

void WINAPI CPluginFSDataInterface::SetupView(BOOL leftPanel, CSalamanderViewAbstract* view, const char* archivePath,
                                              const CFileData* upperDir)
{
    UNREFERENCED_PARAMETER(archivePath);
    UNREFERENCED_PARAMETER(upperDir);

    view->GetTransferVariables(gTransferFileData, gTransferIsDir, gTransferBuffer, gTransferLen, gTransferRowData,
                               gTransferPluginDataIface, gTransferActCustomData);

    view->SetPluginSimpleIconCallback(NULL);

    if (!LogName.empty() && view->GetViewMode() == VIEW_MODE_DETAILED)
    {
        int count = view->GetColumnsCount();
        for (int i = 0; i < count; ++i)
        {
            const CColumn* column = view->GetColumn(i);
            if (column->ID == COLUMN_ID_EXTENSION || column->ID == COLUMN_ID_SIZE || column->ID == COLUMN_ID_DATE ||
                column->ID == COLUMN_ID_TIME || column->ID == COLUMN_ID_ATTRIBUTES)
            {
                view->DeleteColumn(i);
                --i;
                count = view->GetColumnsCount();
            }
        }

        int insertIndex = view->GetColumnsCount() - 1;
        AddColumn(leftPanel, view, insertIndex, IDS_COLUMN_LEVEL, kColumnLevel, GetLevelText);
        AddColumn(leftPanel, view, insertIndex, IDS_COLUMN_TIME, kColumnTime, GetTimeText);
        AddColumn(leftPanel, view, insertIndex, IDS_COLUMN_SOURCE, kColumnSource, GetSourceText);
        AddColumn(leftPanel, view, insertIndex, IDS_COLUMN_EVENTID, kColumnEventId, GetEventIdText);
        AddColumn(leftPanel, view, insertIndex, IDS_COLUMN_TASK, kColumnTask, GetTaskText);
    }
}

BOOL WINAPI CPluginFSDataInterface::GetInfoLineContent(int panel, const CFileData* file, BOOL isDir, int selectedFiles,
                                                       int selectedDirs, BOOL displaySize,
                                                       const CQuadWord& selectedSize, char* buffer, DWORD* hotTexts,
                                                       int& hotTextsCount)
{
    UNREFERENCED_PARAMETER(panel);
    UNREFERENCED_PARAMETER(isDir);
    UNREFERENCED_PARAMETER(selectedFiles);
    UNREFERENCED_PARAMETER(selectedDirs);
    UNREFERENCED_PARAMETER(displaySize);
    UNREFERENCED_PARAMETER(selectedSize);
    UNREFERENCED_PARAMETER(hotTexts);

    if (file == NULL || buffer == NULL)
        return FALSE;

    const EventItemData* data = GetCurrentEventItem(file);
    if (data == NULL)
        return FALSE;

    _snprintf_s(buffer, 1000, _TRUNCATE, "%s | %s | %s | %s",
                data->Record.Level.c_str(), data->Record.TimeCreated.c_str(), data->Record.Source.c_str(),
                data->Record.EventId.c_str());
    hotTextsCount = 0;
    return TRUE;
}

