// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "configstorage.h"
#include "consts.h"

CConfigurationStorage ConfigurationStorage;

CConfigurationStorage::CConfigurationStorage()
{
    StorageType = cstRegistry;
    Registry = NULL;
    FilePath[0] = 0;
}

CConfigurationStorage::~CConfigurationStorage()
{
    Release();
}

CSalamanderRegistryExAbstract* CConfigurationStorage::CreateRegistry(CConfigurationStorageType type)
{
    return type == cstRegistry ? REG_SysRegistryFactory() : REG_MemRegistryFactory();
}

BOOL CConfigurationStorage::BuildRootKeyName(char* keyName, int keyNameSize)
{
    if (SALAMANDER_ROOT_REG == NULL || keyName == NULL || keyNameSize <= 0)
        return FALSE;

    _snprintf_s(keyName, keyNameSize, _TRUNCATE, "HKEY_CURRENT_USER\\%s", SALAMANDER_ROOT_REG);
    return TRUE;
}

BOOL CConfigurationStorage::LoadRegFile(CSalamanderRegistryExAbstract* registry)
{
    if (registry == NULL || FilePath[0] == 0)
        return FALSE;

    HANDLE file = HANDLES_Q(CreateFile(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                                       OPEN_EXISTING, 0, 0));
    if (file == INVALID_HANDLE_VALUE)
        return GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND;

    BOOL ret = FALSE;
    DWORD size = GetFileSize(file, NULL);
    if (size != INVALID_FILE_SIZE && size > 0)
    {
        LPTSTR buf = (LPTSTR)malloc(size + sizeof(WCHAR));
        if (buf != NULL)
        {
            DWORD bytesRead;
            if (ReadFile(file, buf, size, &bytesRead, NULL))
            {
                *(WCHAR*)((LPBYTE)buf + bytesRead) = 0;
                if (ConvertIfNeeded(&buf, bytesRead) != 0)
                    ret = Parse(buf, registry, TRUE) == RPE_OK;
            }
            free(buf);
        }
    }
    else
        ret = size == 0;

    HANDLES(CloseHandle(file));
    return ret;
}

BOOL CConfigurationStorage::Initialize(CConfigurationStorageType type, const char* filePath)
{
    Release();

    StorageType = type;
    if (filePath != NULL)
        strncpy_s(FilePath, filePath, _TRUNCATE);
    else
        FilePath[0] = 0;

    Registry = CreateRegistry(StorageType);
    if (Registry == NULL)
        return FALSE;

    return Load();
}

CSalamanderRegistryExAbstract* CConfigurationStorage::GetRegistry()
{
    return Registry;
}

CConfigurationStorageType CConfigurationStorage::GetStorageType() const
{
    return StorageType;
}

BOOL CConfigurationStorage::SwitchStorageType(CConfigurationStorageType newType, BOOL migrateCurrentData)
{
    if (newType == StorageType)
        return TRUE;

    CSalamanderRegistryExAbstract* newRegistry = CreateRegistry(newType);
    if (newRegistry == NULL)
        return FALSE;

    BOOL ret = TRUE;
    if (migrateCurrentData && Registry != NULL)
    {
        char keyName[MAX_PATH];
        if (BuildRootKeyName(keyName, SizeOf(keyName)))
            ret = CopyBranch(keyName, Registry, newRegistry) == RPE_OK;
    }
    else if (newType == cstRegFile)
    {
        ret = LoadRegFile(newRegistry);
    }

    if (!ret)
    {
        newRegistry->Release();
        return FALSE;
    }

    if (Registry != NULL)
        Registry->Release();
    Registry = newRegistry;
    StorageType = newType;

    return TRUE;
}

BOOL CConfigurationStorage::Load()
{
    if (Registry == NULL)
        return FALSE;

    if (StorageType == cstRegistry)
        return TRUE;

    return LoadRegFile(Registry);
}

BOOL CConfigurationStorage::Save()
{
    if (Registry == NULL)
        return FALSE;

    if (StorageType == cstRegistry)
        return TRUE;

    if (FilePath[0] == 0)
        return FALSE;

    return Registry->Dump(FilePath, NULL);
}

BOOL CConfigurationStorage::Flush()
{
    return Save();
}

void CConfigurationStorage::Release()
{
    if (Registry != NULL)
    {
        if (StorageType == cstRegFile)
            Save();
        Registry->Release();
        Registry = NULL;
    }
}
