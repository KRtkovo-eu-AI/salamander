#ifndef __PLUGIN_H
#define __PLUGIN_H



char *LoadStr(int resID);

extern CSalamanderGeneralAbstract	*SalamanderGeneral;
extern CSalamanderGUIAbstract		*SalamanderGUI;

// FS-name pridelene Salamanderem po loadu pluginu
extern char AssignedFSName[MAX_PATH];
extern int  AssignedFSNameLen;

extern HINSTANCE DLLInstance;  // handle @ SPL
extern HINSTANCE HLanguage;    // handle @ SLG

HINSTANCE GetLanguageResourceHandle();

extern HIMAGELIST DFSImageList ;

BOOL InitFS();

// Menu
#define MENUCMD_DLG              1


// menu commands
#define MENUCMD_START											1
#define MENUCMD_STOP											2
#define MENUCMD_PAUSE											3
#define MENUCMD_RESUME										4
#define MENUCMD_RESTART										5
#define MENUCMD_PROPERTIES								6
#define MENUCMD_SCM												7
#define MENUCMD_DELETE										8
#define MENUCMD_REGISTER										9
// Sub Interfaces
// -----------------------------------------------------------------------------------------
#define TOP_INDEX_MEM_SIZE 50    // pamatovanych number of top-index (level), at least 1

class CTopIndexMem
{
  protected:
    // way for the last remembered by top-index
    char Path[MAX_PATH];
    int  TopIndexes[TOP_INDEX_MEM_SIZE];  // cached top indices
    int  TopIndexesCount;                 // memorized number of top-index

  public:
    CTopIndexMem() {Clear();}
    void Clear() {Path[0] = 0; TopIndexesCount = 0;} // clear the memory
    void Push(const char *path, int topIndex);       // impose top-index for the path
    BOOL FindAndPop(const char *path, int &topIndex);// top-looking index for the path, FALSE-> found
};

//structure for transmitting data from the "Connect" dialogue to the newly created FS
struct CConnectData
{
  BOOL UseConnectData;
  char UserPart[MAX_PATH];

  CConnectData() {UseConnectData = FALSE; UserPart[0] = 0;}
};

extern CConnectData ConnectData;
// -----------------------------------------------------------------------------------------
// Generic PluginInterface
class CPluginInterface: public CPluginInterfaceAbstract
{
  public:
    virtual int WINAPI GetVersion() {return LAST_VERSION_OF_SALAMANDER;}

    virtual void WINAPI About(HWND parent);

    virtual BOOL WINAPI Release(HWND parent, BOOL force);

    virtual void WINAPI LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract *registry);
    virtual void WINAPI SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract *registry);
    virtual void WINAPI Configuration(HWND parent);

    virtual void WINAPI Connect(HWND parent, CSalamanderConnectAbstract *salamander);

    virtual void WINAPI ReleasePluginDataInterface(CPluginDataInterfaceAbstract *pluginData);

    virtual CPluginInterfaceForArchiverAbstract * WINAPI GetInterfaceForArchiver();
    virtual CPluginInterfaceForViewerAbstract * WINAPI GetInterfaceForViewer();
    virtual CPluginInterfaceForMenuExtAbstract * WINAPI GetInterfaceForMenuExt();
    virtual CPluginInterfaceForFSAbstract * WINAPI GetInterfaceForFS();
    virtual CPluginInterfaceForThumbLoaderAbstract * WINAPI GetInterfaceForThumbLoader();

    virtual void WINAPI Event(int event, DWORD param);
    virtual void WINAPI ClearHistory(HWND parent);
    virtual void WINAPI AcceptChangeOnPathNotification(const char *path, BOOL includingSubdirs) {}
    virtual void WINAPI PasswordManagerEvent(HWND parent, int event) {}

    virtual BOOL WINAPI UninstallUnregisteredComponents(HWND parent, char *componentsDescr, BOOL *uninstallSPL,
                                                        BOOL *uninstallLangDir, const char *pluginDir,
                                                        CDynamicString *deleteFileList) {return FALSE;}
};

//Sub Interfaces
class CPluginInterfaceForArchiver: public CPluginInterfaceForArchiverAbstract
{
  public:
    virtual BOOL WINAPI ListArchive(CSalamanderForOperationsAbstract *salamander, const char *fileName,
                                    CSalamanderDirectoryAbstract *dir,
                                    CPluginDataInterfaceAbstract *&pluginData);
    virtual BOOL WINAPI UnpackArchive(CSalamanderForOperationsAbstract *salamander, const char *fileName,
                                      CPluginDataInterfaceAbstract *pluginData, const char *targetDir,
                                      const char *archiveRoot, SalEnumSelection next, void *nextParam);
    virtual BOOL WINAPI UnpackOneFile(CSalamanderForOperationsAbstract *salamander, const char *fileName,
                                      CPluginDataInterfaceAbstract *pluginData, const char *nameInArchive,
                                      const CFileData *fileData, const char *targetDir,
                                      const char *newFileName, BOOL *renamingNotSupported);
    virtual BOOL WINAPI PackToArchive(CSalamanderForOperationsAbstract *salamander, const char *fileName,
                                      const char *archiveRoot, BOOL move, const char *sourcePath,
                                      SalEnumSelection2 next, void *nextParam);
    virtual BOOL WINAPI DeleteFromArchive(CSalamanderForOperationsAbstract *salamander, const char *fileName,
                                          CPluginDataInterfaceAbstract *pluginData, const char *archiveRoot,
                                          SalEnumSelection next, void *nextParam);
    virtual BOOL WINAPI UnpackWholeArchive(CSalamanderForOperationsAbstract *salamander, const char *fileName,
                                           const char *mask, const char *targetDir, BOOL delArchiveWhenDone,
                                           CDynamicString *archiveVolumes);
    virtual BOOL WINAPI CanCloseArchive(CSalamanderForOperationsAbstract *salamander, const char *fileName,
                                        BOOL force, int panel);

    virtual BOOL WINAPI GetCacheInfo(char *tempPath, BOOL *ownDelete, BOOL *cacheCopies);
    virtual void WINAPI DeleteTmpCopy(const char *fileName, BOOL firstFile);
    virtual BOOL WINAPI PrematureDeleteTmpCopy(HWND parent, int copiesCount);
};

class CPluginInterfaceForViewer: public CPluginInterfaceForViewerAbstract
{
  public:
    virtual BOOL WINAPI ViewFile(const char *name, int left, int top, int width, int height,
                                 UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE *lock,
                                 BOOL *lockOwner, CSalamanderPluginViewerData *viewerData,
                                 int enumFilesSourceUID, int enumFilesCurrentIndex);
    virtual BOOL WINAPI CanViewFile(const char *name) {return TRUE;}
};

class CPluginInterfaceForMenuExt: public CPluginInterfaceForMenuExtAbstract
{
  public:
    virtual DWORD WINAPI GetMenuItemState(int id, DWORD eventMask);
    virtual BOOL WINAPI ExecuteMenuItem(CSalamanderForOperationsAbstract *salamander, HWND parent,
                                        int id, DWORD eventMask);
    virtual BOOL WINAPI HelpForMenuItem(HWND parent, int id);
    virtual void WINAPI BuildMenu(HWND parent, CSalamanderBuildMenuAbstract *salamander);
};

class CPluginInterfaceForFS: public CPluginInterfaceForFSAbstract
{
  protected:
    int ActiveFSCount;

  public:
    CPluginInterfaceForFS() {ActiveFSCount = 0;}
    int GetActiveFSCount() {return ActiveFSCount;}

    virtual CPluginFSInterfaceAbstract * WINAPI OpenFS(const char *fsName, int fsNameIndex);
    virtual void WINAPI CloseFS(CPluginFSInterfaceAbstract *fs);

    virtual void WINAPI ExecuteChangeDriveMenuItem(int panel);
    virtual BOOL WINAPI ChangeDriveMenuItemContextMenu(HWND parent, int panel, int x, int y,
                                                       CPluginFSInterfaceAbstract *pluginFS,
                                                       const char *pluginFSName, int pluginFSNameIndex,
                                                       BOOL isDetachedFS, BOOL &refreshMenu,
                                                       BOOL &closeMenu, int &postCmd, void *&postCmdParam);
    virtual void WINAPI ExecuteChangeDrivePostCommand(int panel, int postCmd, void *postCmdParam);
    virtual void WINAPI ExecuteOnFS(int panel, CPluginFSInterfaceAbstract *pluginFS,
                                    const char *pluginFSName, int pluginFSNameIndex,
                                    CFileData &file, int isDir);
    virtual BOOL WINAPI DisconnectFS(HWND parent, BOOL isInPanel, int panel,
                                     CPluginFSInterfaceAbstract *pluginFS,
                                     const char *pluginFSName, int pluginFSNameIndex);

    virtual void WINAPI ConvertPathToInternal(const char *fsName, int fsNameIndex,
                                              char *fsUserPart) {}
    virtual void WINAPI ConvertPathToExternal(const char *fsName, int fsNameIndex,
                                              char *fsUserPart) {}
    virtual BOOL WINAPI GetNoItemsInPanelText(char *textBuf, int textBufSize) {return FALSE;}
    virtual void WINAPI ShowSecurityInfo(HWND parent) {}
    virtual void WINAPI EnsureShareExistsOnServer(int panel, const char *server, const char *share) {}
};




class CPluginFSInterface: public CPluginFSInterfaceAbstract
{
  public:
    char Path[MAX_PATH];      // current path
    BOOL PathError;           // TRUE if failed ListCurrentPath (path error), will call ChangePath
    BOOL FatalError;          // TRUE if ListCurrentPath failed (fatal error), will call ChangePath
    CTopIndexMem TopIndexMem; // top-memory index ExecuteOnFS ()
    BOOL CalledFromDisconnectDialog; // TRUE = user wants disconnectnout the FS from Disconnect dialog (F12)

  public:
    CPluginFSInterface();
    ~CPluginFSInterface() {}

    virtual BOOL WINAPI GetCurrentPath(char *userPart);
    virtual BOOL WINAPI GetFullName(CFileData &file, int isDir, char *buf, int bufSize);
    virtual BOOL WINAPI GetFullFSPath(HWND parent, const char *fsName, char *path, int pathSize, BOOL &success);
    virtual BOOL WINAPI GetRootPath(char *userPart);

    virtual BOOL WINAPI IsCurrentPath(int currentFSNameIndex, int fsNameIndex, const char *userPart);
    virtual BOOL WINAPI IsOurPath(int currentFSNameIndex, int fsNameIndex, const char *userPart);

    virtual BOOL WINAPI ChangePath(int currentFSNameIndex, char *fsName, int fsNameIndex, const char *userPart,
                                   char *cutFileName, BOOL *pathWasCut, BOOL forceRefresh, int mode);
    virtual BOOL WINAPI ListCurrentPath(CSalamanderDirectoryAbstract *dir,
                                        CPluginDataInterfaceAbstract *&pluginData,
                                        int &iconsType, BOOL forceRefresh);

    virtual BOOL WINAPI TryCloseOrDetach(BOOL forceClose, BOOL canDetach, BOOL &detach, int reason);

    virtual void WINAPI Event(int event, DWORD param);

    virtual void WINAPI ReleaseObject(HWND parent);

    virtual DWORD WINAPI GetSupportedServices();

    virtual BOOL WINAPI GetChangeDriveOrDisconnectItem(const char *fsName, char *&title, HICON &icon, BOOL &destroyIcon);
    virtual HICON WINAPI GetFSIcon(BOOL &destroyIcon);
    virtual void WINAPI GetDropEffect(const char *srcFSPath, const char *tgtFSPath,
                                      DWORD allowedEffects, DWORD keyState,
                                      DWORD *dropEffect);
    virtual void WINAPI GetFSFreeSpace(CQuadWord *retValue);
    virtual BOOL WINAPI GetNextDirectoryLineHotPath(const char *text, int pathLen, int &offset);
    virtual void WINAPI CompleteDirectoryLineHotPath(char *path, int pathBufSize) {}
    virtual BOOL WINAPI GetPathForMainWindowTitle(const char *fsName, int mode, char *buf, int bufSize) {return FALSE;}
    virtual void WINAPI ShowInfoDialog(const char *fsName, HWND parent);
    virtual BOOL WINAPI ExecuteCommandLine(HWND parent, char *command, int &selFrom, int &selTo);
    virtual BOOL WINAPI QuickRename(const char *fsName, int mode, HWND parent, CFileData &file, BOOL isDir,
                                    char *newName, BOOL &cancel);
    virtual void WINAPI AcceptChangeOnPathNotification(const char *fsName, const char *path, BOOL includingSubdirs);
    virtual BOOL WINAPI CreateDir(const char *fsName, int mode, HWND parent, char *newName, BOOL &cancel);
    virtual void WINAPI ViewFile(const char *fsName, HWND parent,
                                 CSalamanderForViewFileOnFSAbstract *salamander,
                                 CFileData &file);
    virtual BOOL WINAPI Delete(const char *fsName, int mode, HWND parent, int panel,
                               int selectedFiles, int selectedDirs, BOOL &cancelOrError);
    virtual BOOL WINAPI CopyOrMoveFromFS(BOOL copy, int mode, const char *fsName, HWND parent,
                                         int panel, int selectedFiles, int selectedDirs,
                                         char *targetPath, BOOL &operationMask,
                                         BOOL &cancelOrHandlePath, HWND dropTarget);
    virtual BOOL WINAPI CopyOrMoveFromDiskToFS(BOOL copy, int mode, const char *fsName, HWND parent,
                                               const char *sourcePath, SalEnumSelection2 next,
                                               void *nextParam, int sourceFiles, int sourceDirs,
                                               char *targetPath, BOOL *invalidPathOrCancel);
    virtual BOOL WINAPI ChangeAttributes(const char *fsName, HWND parent, int panel,
                                         int selectedFiles, int selectedDirs);
    virtual void WINAPI ShowProperties(const char *fsName, HWND parent, int panel,
                                       int selectedFiles, int selectedDirs);
    virtual void WINAPI ContextMenu(const char *fsName, HWND parent, int menuX, int menuY, int type,
                                    int panel, int selectedFiles, int selectedDirs);
    virtual BOOL WINAPI HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult) {return FALSE;}
    virtual BOOL WINAPI OpenFindDialog(const char *fsName, int panel) {return FALSE;}
    virtual void WINAPI OpenActiveFolder(const char *fsName, HWND parent) {}
    virtual void WINAPI GetAllowedDropEffects(int mode, const char *tgtFSPath, DWORD *allowedEffects) {}
    virtual BOOL WINAPI GetNoItemsInPanelText(char *textBuf, int textBufSize) {return FALSE;}
    virtual void WINAPI ShowSecurityInfo(HWND parent) {}
    virtual void WINAPI EnsureShareExistsOnServer(int panel, const char *server, const char *share) {}
};
class CPluginInterfaceForThumbLoader: public CPluginInterfaceForThumbLoaderAbstract
{
  public:
    virtual BOOL WINAPI LoadThumbnail(const char *filename, int thumbWidth, int thumbHeight,
                                      CSalamanderThumbnailMakerAbstract *thumbMaker,
                                      BOOL fastThumbnail);
};

class CArcPluginDataInterface: public CPluginDataInterfaceAbstract
{
  public:
    virtual BOOL WINAPI CallReleaseForFiles() {return FALSE;}
    virtual BOOL WINAPI CallReleaseForDirs() {return FALSE;}
    virtual void WINAPI ReleasePluginData(CFileData &file, BOOL isDir) {}

    virtual void WINAPI GetFileDataForUpDir(const char *archivePath, CFileData &upDir)
    {
      // zadna vlastni data plugin v CFileData nema, takze neni potreba nic menit/alokovat
    }
    virtual BOOL WINAPI GetFileDataForNewDir(const char *dirName, CFileData &dir)
    {
      // zadna vlastni data plugin v CFileData nema, takze neni potreba nic menit/alokovat
      return TRUE;  // vracime uspech
    }

    virtual HIMAGELIST WINAPI GetSimplePluginIcons(int iconSize) {return NULL;}
    virtual BOOL WINAPI HasSimplePluginIcon(CFileData &file, BOOL isDir) {return TRUE;}
    virtual HICON WINAPI GetPluginIcon(const CFileData *file, int iconSize, BOOL &destroyIcon) {return NULL;}
    virtual int WINAPI CompareFilesFromFS(const CFileData *file1, const CFileData *file2) {return 0;}

    virtual void WINAPI SetupView(BOOL leftPanel, CSalamanderViewAbstract *view, const char *archivePath,
                                  const CFileData *upperDir);
    virtual void WINAPI ColumnFixedWidthShouldChange(BOOL leftPanel, const CColumn *column, int newFixedWidth);
    virtual void WINAPI ColumnWidthWasChanged(BOOL leftPanel, const CColumn *column, int newWidth);

    virtual BOOL WINAPI GetInfoLineContent(int panel, const CFileData *file, BOOL isDir, int selectedFiles,
                                           int selectedDirs, BOOL displaySize, const CQuadWord &selectedSize,
                                           char *buffer, DWORD *hotTexts, int &hotTextsCount) {return FALSE;}

    virtual BOOL WINAPI CanBeCopiedToClipboard() {return TRUE;}

    virtual BOOL WINAPI GetByteSize(const CFileData *file, BOOL isDir, CQuadWord *size) {return FALSE;}
    virtual BOOL WINAPI GetLastWriteDate(const CFileData *file, BOOL isDir, SYSTEMTIME *date) {return FALSE;}
    virtual BOOL WINAPI GetLastWriteTime(const CFileData *file, BOOL isDir, SYSTEMTIME *time) {return FALSE;}
};

// ****************************************************************************
// CPluginFSDataInterface
//

//struct CFSData
//{
//  FILETIME CreationTime;
//  FILETIME LastAccessTime;
//  char *TypeName;
//
//  //CFSData(const FILETIME &creationTime, const FILETIME &lastAccessTime, const char *type);
//  //~CFSData() {if (TypeName != NULL) SalamanderGeneral->Free(TypeName);}
//  BOOL IsGood() {return TypeName != NULL;}
//};

struct CFSData
{
        CFSData()
            : Description(NULL)
            , Status(0)
            , StartupType(0)
            , LogOnAs(NULL)
            , ServiceName(NULL)
            , DisplayName(NULL)
            , ExecuteablePath(NULL)
        {
        }

        ~CFSData()
        {
            FreeString(Description);
            FreeString(LogOnAs);
            FreeString(ServiceName);
            FreeString(DisplayName);
            FreeString(ExecuteablePath);
        }

        void FreeString(char *&ptr)
        {
            if (ptr != NULL)
            {
                SalamanderGeneral->Free(ptr);
                ptr = NULL;
            }
        }

        char *Description;
        int Status;
        int StartupType;
        char *LogOnAs;
        char *ServiceName;
        char *DisplayName;
        char *ExecuteablePath;
};


class CPluginFSDataInterface: public CPluginDataInterfaceAbstract
{
  protected:
    char Path[MAX_PATH];   // buffer pro plne jmeno souboru/adresare pouzivane pri nacitani ikon
    char *Name;            // ukazatel do Path za posledni backslash cesty (na jmeno)

  public:
    CPluginFSDataInterface(const char *path);

    virtual BOOL WINAPI CallReleaseForFiles() {return TRUE;}
    virtual BOOL WINAPI CallReleaseForDirs() {return TRUE;}

    virtual void WINAPI ReleasePluginData(CFileData &file, BOOL isDir)
    {
      (void)isDir;
      if (file.PluginData != 0)
      {
        delete reinterpret_cast<CFSData *>(file.PluginData);
        file.PluginData = 0;
      }
    }

    virtual void WINAPI GetFileDataForUpDir(const char *archivePath, CFileData &upDir) {}
    virtual BOOL WINAPI GetFileDataForNewDir(const char *dirName, CFileData &dir) {return FALSE;}

    virtual HIMAGELIST WINAPI GetSimplePluginIcons(int iconSize);
    virtual BOOL WINAPI HasSimplePluginIcon(CFileData &file, BOOL isDir); 
    virtual HICON WINAPI GetPluginIcon(const CFileData *file, int iconSize, BOOL &destroyIcon);

    virtual int WINAPI CompareFilesFromFS(const CFileData *file1, const CFileData *file2);

    virtual void WINAPI SetupView(BOOL leftPanel, CSalamanderViewAbstract *view, const char *archivePath,
                                  const CFileData *upperDir);
    virtual void WINAPI ColumnFixedWidthShouldChange(BOOL leftPanel, const CColumn *column, int newFixedWidth);
    virtual void WINAPI ColumnWidthWasChanged(BOOL leftPanel, const CColumn *column, int newWidth);

    virtual BOOL WINAPI GetInfoLineContent(int panel, const CFileData *file, BOOL isDir, int selectedFiles,
                                           int selectedDirs, BOOL displaySize, const CQuadWord &selectedSize,
                                           char *buffer, DWORD *hotTexts, int &hotTextsCount);

    virtual BOOL WINAPI CanBeCopiedToClipboard() {return TRUE;}

    virtual BOOL WINAPI GetByteSize(const CFileData *file, BOOL isDir, CQuadWord *size) {return FALSE;}
    virtual BOOL WINAPI GetLastWriteDate(const CFileData *file, BOOL isDir, SYSTEMTIME *date) {return FALSE;}
    virtual BOOL WINAPI GetLastWriteTime(const CFileData *file, BOOL isDir, SYSTEMTIME *time) {return FALSE;}
};


// External 
extern CPluginInterface PluginInterface;

// Events
void OnAbout(HWND hParent);
void OnConfiguration(HWND hParent);

#endif
