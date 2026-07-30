// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define IDD_DISKDIR_PACK 101
#define IDC_DD_ARCHIVE 1001
#define IDC_DD_PACK_PATHS 1002
#define IDC_DD_RECURSE 1003

class CDiskDirArchiver : public CPluginInterfaceForArchiverAbstract
{
public:
    BOOL WINAPI ListArchive(CSalamanderForOperationsAbstract* salamander, const char* fileName,
                            CSalamanderDirectoryAbstract* dir,
                            CPluginDataInterfaceAbstract*& pluginData) override;
    BOOL WINAPI UnpackArchive(CSalamanderForOperationsAbstract* salamander, const char* fileName,
                              CPluginDataInterfaceAbstract* pluginData, const char* targetDir,
                              const char* archiveRoot, SalEnumSelection next,
                              void* nextParam) override;
    BOOL WINAPI UnpackOneFile(CSalamanderForOperationsAbstract* salamander, const char* fileName,
                              CPluginDataInterfaceAbstract* pluginData, const char* nameInArchive,
                              const CFileData* fileData, const char* targetDir,
                              const char* newFileName, BOOL* renamingNotSupported) override;
    BOOL WINAPI PackToArchive(CSalamanderForOperationsAbstract* salamander, const char* fileName,
                              const char* archiveRoot, BOOL move, const char* sourcePath,
                              SalEnumSelection2 next, void* nextParam) override;
    BOOL WINAPI DeleteFromArchive(CSalamanderForOperationsAbstract*, const char*,
                                  CPluginDataInterfaceAbstract*, const char*,
                                  SalEnumSelection, void*) override { return FALSE; }
    BOOL WINAPI UnpackWholeArchive(CSalamanderForOperationsAbstract* salamander,
                                   const char* fileName, const char* mask,
                                   const char* targetDir, BOOL delArchiveWhenDone,
                                   CDynamicString* archiveVolumes) override;
    BOOL WINAPI CanCloseArchive(CSalamanderForOperationsAbstract*, const char*,
                                BOOL, int) override { return TRUE; }
    BOOL WINAPI GetCacheInfo(char*, BOOL*, BOOL*) override { return FALSE; }
    void WINAPI DeleteTmpCopy(const char*, BOOL) override {}
    BOOL WINAPI PrematureDeleteTmpCopy(HWND, int) override { return FALSE; }
};

class CDiskDirPlugin : public CPluginInterfaceAbstract
{
public:
    void WINAPI About(HWND parent) override;
    BOOL WINAPI Release(HWND, BOOL) override { return TRUE; }
    void WINAPI LoadConfiguration(HWND, HKEY, CSalamanderRegistryAbstract*) override;
    void WINAPI SaveConfiguration(HWND, HKEY, CSalamanderRegistryAbstract*) override;
    void WINAPI Configuration(HWND) override {}
    void WINAPI Connect(HWND parent, CSalamanderConnectAbstract* salamander) override;
    void WINAPI ReleasePluginDataInterface(CPluginDataInterfaceAbstract*) override {}
    CPluginInterfaceForArchiverAbstract* WINAPI GetInterfaceForArchiver() override;
    CPluginInterfaceForViewerAbstract* WINAPI GetInterfaceForViewer() override { return NULL; }
    CPluginInterfaceForMenuExtAbstract* WINAPI GetInterfaceForMenuExt() override { return NULL; }
    CPluginInterfaceForFSAbstract* WINAPI GetInterfaceForFS() override { return NULL; }
    CPluginInterfaceForThumbLoaderAbstract* WINAPI GetInterfaceForThumbLoader() override { return NULL; }
    void WINAPI Event(int, DWORD) override {}
    void WINAPI ClearHistory(HWND) override {}
    void WINAPI AcceptChangeOnPathNotification(const char*, BOOL) override {}
    void WINAPI PasswordManagerEvent(HWND, int) override {}
};

extern HINSTANCE DLLInstance;
extern CSalamanderGeneralAbstract* SalamanderGeneral;
extern CSalamanderDebugAbstract* SalamanderDebug;
extern int SalamanderVersion;
