// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

extern CSalamanderGeneralAbstract* SalamanderGeneral;
extern CSalamanderSafeFileAbstract* SalamanderSafeFile;
extern CSalamanderGUIAbstract* SalamanderGUI;

// Close host-owned handles/progress even if an owned string allocation fails.
class CSplitCBNSafeFileScope
{
    SAFE_FILE& File;
public:
    explicit CSplitCBNSafeFileScope(SAFE_FILE& file) : File(file) { ZeroMemory(&File, sizeof(File)); }
    ~CSplitCBNSafeFileScope()
    {
        if (File.HFile != NULL && File.HFile != INVALID_HANDLE_VALUE)
            SalamanderSafeFile->SafeFileClose(&File);
    }
    CSplitCBNSafeFileScope(const CSplitCBNSafeFileScope&) = delete;
    CSplitCBNSafeFileScope& operator=(const CSplitCBNSafeFileScope&) = delete;
};
// Keep the input read handles open across target validation/creation. Their
// sharing mode also prevents a target alias race from truncating an input.
class CSplitCBNInputFiles
{
    std::vector<SAFE_FILE> Files;
public:
    explicit CSplitCBNInputFiles(int count) : Files(static_cast<size_t>(count)) {}
    ~CSplitCBNInputFiles()
    {
        for (size_t i = 0; i < Files.size(); ++i)
            if (Files[i].HFile != NULL && Files[i].HFile != INVALID_HANDLE_VALUE)
                SalamanderSafeFile->SafeFileClose(&Files[i]);
    }
    SAFE_FILE& Get(int index) { return Files[static_cast<size_t>(index)]; }
    CSplitCBNInputFiles(const CSplitCBNInputFiles&) = delete;
    CSplitCBNInputFiles& operator=(const CSplitCBNInputFiles&) = delete;
};
class CSplitCBNProgressScope
{
    CSalamanderForOperationsAbstract* Operations;
    bool Open;
public:
    explicit CSplitCBNProgressScope(CSalamanderForOperationsAbstract* operations) : Operations(operations), Open(false) {}
    ~CSplitCBNProgressScope() { Close(); }
    void Started() { Open = true; }
    void Close() { if (Open) { Open = false; Operations->CloseProgressDialog(); } }
    CSplitCBNProgressScope(const CSplitCBNProgressScope&) = delete;
    CSplitCBNProgressScope& operator=(const CSplitCBNProgressScope&) = delete;
};


class CPluginInterface : public CPluginInterfaceAbstract
{
public:
    virtual void WINAPI About(HWND parent);

    virtual BOOL WINAPI Release(HWND parent, BOOL force);

    virtual void WINAPI LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry);
    virtual void WINAPI SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry);
    virtual void WINAPI Configuration(HWND parent);

    virtual void WINAPI Connect(HWND parent, CSalamanderConnectAbstract* salamander);

    virtual void WINAPI ReleasePluginDataInterface(CPluginDataInterfaceAbstract* pluginData) { return; };

    virtual CPluginInterfaceForArchiverAbstract* WINAPI GetInterfaceForArchiver() { return NULL; };
    virtual CPluginInterfaceForViewerAbstract* WINAPI GetInterfaceForViewer() { return NULL; };
    virtual CPluginInterfaceForMenuExtAbstract* WINAPI GetInterfaceForMenuExt();
    virtual CPluginInterfaceForFSAbstract* WINAPI GetInterfaceForFS() { return NULL; };
    virtual CPluginInterfaceForThumbLoaderAbstract* WINAPI GetInterfaceForThumbLoader() { return NULL; }

    virtual void WINAPI Event(int event, DWORD param) {}
    virtual void WINAPI ClearHistory(HWND parent) {}
    virtual void WINAPI AcceptChangeOnPathNotification(const char* path, BOOL includingSubdirs) {}

    virtual void WINAPI PasswordManagerEvent(HWND parent, int event) {}
};

class CPluginInterfaceForMenuExt : public CPluginInterfaceForMenuExtAbstract
{
public:
    virtual DWORD WINAPI GetMenuItemState(int id, DWORD eventMask) { return 0; };
    virtual BOOL WINAPI ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander, HWND parent,
                                        int id, DWORD eventMask);
    virtual BOOL WINAPI HelpForMenuItem(HWND parent, int id);
    virtual void WINAPI BuildMenu(HWND parent, CSalamanderBuildMenuAbstract* salamander) {}
};

extern BOOL configIncludeFileExt;
extern BOOL configCreateBatchFile;
extern BOOL configSplitToOther;
extern BOOL configCombineToOther;
extern BOOL configSplitToSubdir;

extern HINSTANCE DLLInstance; // handle to SPL - language-independent resources
extern HINSTANCE HLanguage;   // handle to SLG - language-dependent resources
char* LoadStr(int resID);
void RefreshSplitCBNDarkModeFromHost();
void ConfigureSplitCBNDarkModeFromHost();
void ApplySplitCBNDarkMode(HWND hwnd);
BOOL HandleSplitCBNDarkCtlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, INT_PTR* result);
void CenterWindow(HWND hWnd);
void GetInfo(char* buffer, CQuadWord& size);
void StripExtension(LPTSTR fileName);
BOOL Error(int title, int error, ...);
BOOL Error2(HWND hParent, int title, int error, ...);
BOOL TestTargetSpace(HWND parent, const char* path, const CQuadWord& size, int title);
std::string GetTargetDir(const char* sourceDirectory, const char* subdirName, BOOL bSplit);
BOOL MakePathAbsolute(std::string& path, BOOL pathIsDir, const char* absRoot, BOOL activePreferred, int errorTitle);

#define GETPARENT SalamanderGeneral->GetMsgBoxParent()

#define DUMP_MEM_OBJECTS

#if defined(DUMP_MEM_OBJECTS) && defined(_DEBUG)
#define CRT_MEM_CHECKPOINT \
    _CrtMemState ___CrtMemState; \
    _CrtMemCheckpoint(&___CrtMemState);
#define CRT_MEM_DUMP_ALL_OBJECTS_SINCE _CrtMemDumpAllObjectsSince(&___CrtMemState);

#else //DUMP_MEM_OBJECTS
#define CRT_MEM_CHECKPOINT ;
#define CRT_MEM_DUMP_ALL_OBJECTS_SINCE ;

#endif //DUMP_MEM_OBJECTS
