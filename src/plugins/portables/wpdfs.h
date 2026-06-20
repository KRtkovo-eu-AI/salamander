// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
	Windows Portable Devices Plugin for Open Salamander
	
	Copyright (c) 2015 Milan Kase <manison@manison.cz>
	Copyright (c) 2015 Open Salamander Authors
	
	wpdfs.h
	Salamander file system.
*/

#pragma once

#include "fx.h"
#include "fxfs.h"

class CWpdDevice;
class CWpdBaseContentItem;

class CWpdFS final : public TFxPluginFSInterface<CWpdFS>
{
protected:
    HRESULT WINAPI GetContentLocationForPath(PCSTR path, _Out_ CWpdDevice*& device, _Out_ CFxString& objectId);
    HRESULT WINAPI GetCurrentContentLocation(_Out_ CWpdDevice*& device, _Out_ CFxString& objectId);
    HRESULT WINAPI CreateWpdFolder(CWpdDevice* device, PCWSTR parentObjectId, PCSTR name);
    HRESULT WINAPI RenameWpdObject(CWpdBaseContentItem* item, PCSTR newName);
    HRESULT WINAPI RenameWpdObjectByCopy(CWpdBaseContentItem* item, PCSTR newName);
    HRESULT WINAPI AddWpdObjectId(IPortableDevicePropVariantCollection* objects, PCWSTR objectId);
    HRESULT WINAPI DeleteWpdObjects(CWpdDevice* device, IPortableDevicePropVariantCollection* objects);
    HRESULT WINAPI CopyOrMoveWpdObjects(
        CWpdDevice* device,
        IPortableDevicePropVariantCollection* objects,
        PCWSTR destinationObjectId,
        bool copy);
    HRESULT WINAPI DownloadWpdFile(CWpdBaseContentItem* item, PCSTR targetName);
    HRESULT WINAPI DownloadWpdObject(CWpdDevice* device, PCWSTR objectId, PCSTR targetName);
    HRESULT WINAPI UploadDiskFile(CWpdDevice* device, PCWSTR parentObjectId, PCSTR sourceName, PCSTR targetName);
    HRESULT WINAPI UploadDiskObject(CWpdDevice* device, PCWSTR parentObjectId, PCSTR sourceName, PCSTR targetName);
    HRESULT WINAPI FindWpdChildObject(CWpdDevice* device, PCWSTR parentObjectId, PCSTR childName, _Out_ PWSTR* childObjectId, _Out_opt_ DWORD* attributes);
    HRESULT WINAPI ConfirmAndDeleteExistingWpdObject(HWND parent, CWpdDevice* device, PCWSTR parentObjectId, PCSTR targetName, PCSTR sourceName, bool& overwriteAll, bool& skipAll, _Out_ bool& skip);

    /* CFxPluginFSInterface Overrides */

    virtual CFxPluginDataInterface* WINAPI CreatePluginData(CFxItemEnumerator* enumerator) override;

    virtual HRESULT WINAPI GetChildEnumerator(
        _Out_ CFxItemEnumerator*& enumerator,
        CFxItem* parentItem,
        int level,
        bool forceRefresh) override;

    virtual BOOL WINAPI QuickRename(
        const char* fsName,
        int mode,
        HWND parent,
        CFileData& file,
        BOOL isDir,
        char* newName,
        BOOL& cancel) override;

    virtual BOOL WINAPI GetPathForMainWindowTitle(
        const char* fsName,
        int mode,
        char* buf,
        int bufSize) override;

    virtual BOOL WINAPI CreateDir(
        const char* fsName,
        int mode,
        HWND parent,
        char* newName,
        BOOL& cancel) override;

    virtual void WINAPI ViewFile(
        const char* fsName,
        HWND parent,
        CSalamanderForViewFileOnFSAbstract* salamander,
        CFileData& file) override;

    virtual BOOL WINAPI Delete(
        const char* fsName,
        int mode,
        HWND parent,
        int panel,
        int selectedFiles,
        int selectedDirs,
        BOOL& cancelOrError) override;

    virtual BOOL WINAPI CopyOrMoveFromFS(
        BOOL copy,
        int mode,
        const char* fsName,
        HWND parent,
        int panel,
        int selectedFiles,
        int selectedDirs,
        char* targetPath,
        BOOL& operationMask,
        BOOL& cancelOrHandlePath,
        HWND dropTarget) override;

    virtual BOOL WINAPI CopyOrMoveFromDiskToFS(
        BOOL copy,
        int mode,
        const char* fsName,
        HWND parent,
        const char* sourcePath,
        SalEnumSelection2 next,
        void* nextParam,
        int sourceFiles,
        int sourceDirs,
        char* targetPath,
        BOOL* invalidPathOrCancel) override;

public:
    CWpdFS(CFxPluginInterfaceForFS& owner);

    enum _SupportedServices
    {
        SUPPORTED_SERVICES = FS_SERVICE_QUICKRENAME | FS_SERVICE_CREATEDIR | FS_SERVICE_DELETE |
                             FS_SERVICE_COPYFROMFS | FS_SERVICE_MOVEFROMFS |
                             FS_SERVICE_VIEWFILE | FS_SERVICE_GETPATHFORMAINWNDTITLE |
                             FS_SERVICE_COPYFROMDISKTOFS | FS_SERVICE_MOVEFROMDISKTOFS
    };

    static PCTSTR SUGGESTED_NAME;
};

typedef enum _WPDFS_LEVEL
{
    WPDFS_LEVEL_DEVICE = 1,
    WPDFS_LEVEL_STORAGE = 2,
    WPDFS_LEVEL_CONTENT = 3,
} WPDFS_LEVEL;

class CWpdType
{
protected:
    WPDFS_LEVEL m_level;

    CWpdType(WPDFS_LEVEL level)
        : m_level(level)
    {
    }

public:
    bool IsDevice() const
    {
        return m_level == WPDFS_LEVEL_DEVICE;
    }

    bool IsStorage() const
    {
        return m_level == WPDFS_LEVEL_STORAGE;
    }

    bool IsContent() const
    {
        return m_level == WPDFS_LEVEL_CONTENT;
    }
};

class CWpdItem : public CFxItem, public CWpdType
{
protected:
    CWpdItem(WPDFS_LEVEL level)
        : CWpdType(level)
    {
    }
};

class CWpdEnumerator : public CFxItemEnumerator, public CWpdType
{
protected:
    CWpdEnumerator(WPDFS_LEVEL level)
        : CWpdType(level)
    {
    }

public:
    virtual class CWpdPluginDataInterface* WINAPI CreatePluginData(CFxPluginFSInterface& owner) = 0;
};

class CWpdPluginDataInterface : public CFxPluginFSDataInterface, public CWpdType
{
protected:
    CWpdPluginDataInterface(CFxPluginFSInterface& owner, WPDFS_LEVEL level)
        : CFxPluginFSDataInterface(owner),
          CWpdType(level)
    {
    }
};
