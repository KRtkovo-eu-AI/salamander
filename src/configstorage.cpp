// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "configstorage.h"
#include "consts.h"
#include "cfgdlg.h"

CConfigurationStorage ConfigurationStorage;

CConfigurationStorage::CConfigurationStorage()
    : ActiveRegistryKeys(32, 16)
{
    StorageType = cstRegistry;
    Registry = NULL;
    FilePath[0] = 0;
    DoNotDeleteHiddenKeysAndValues = TRUE;
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


void CConfigurationStorage::DeleteStoredRegistryConfiguration(const char* keyName)
{
    HKEY key;
    if (keyName != NULL && OpenKey(HKEY_CURRENT_USER, keyName, key))
    {
        ClearKey(key);
        CloseKey(key);
        DeleteKey(HKEY_CURRENT_USER, keyName);
    }
}


BOOL CConfigurationStorage::GetStorageTypeBootstrapFilePath(char* filePath, int filePathSize)
{
    if (filePath == NULL || filePathSize <= 0)
        return FALSE;

    DWORD len = GetModuleFileName(NULL, filePath, filePathSize);
    if (len == 0 || len >= (DWORD)filePathSize)
        return FALSE;

    char* slash = strrchr(filePath, '\\');
    if (slash != NULL)
        slash++;
    else
        slash = filePath;

    strcpy_s(slash, filePathSize - (int)(slash - filePath), "configstorage.ini");
    return TRUE;
}

BOOL CConfigurationStorage::LoadStorageTypeBootstrap(CConfigurationStorageType& type)
{
    char fileName[MAX_PATH];
    if (!GetStorageTypeBootstrapFilePath(fileName, SizeOf(fileName)))
        return FALSE;

    char value[40];
    DWORD read = GetPrivateProfileString("Configuration", "StorageType", "", value, SizeOf(value), fileName);
    if (read == 0)
        return FALSE;

    if (_stricmp(value, "RegFile") == 0)
        type = cstRegFile;
    else
        type = cstRegistry;
    return TRUE;
}

BOOL CConfigurationStorage::SaveStorageTypeBootstrap(CConfigurationStorageType type)
{
    char fileName[MAX_PATH];
    if (!GetStorageTypeBootstrapFilePath(fileName, SizeOf(fileName)))
        return FALSE;

    return WritePrivateProfileString("Configuration", "StorageType",
                                     type == cstRegFile ? "RegFile" : "Registry", fileName);
}


BOOL CConfigurationStorage::UseActiveRegistryForKey(HKEY key, const char* name)
{
    if (Registry == NULL || StorageType != cstRegFile)
        return FALSE;

    if (key == HKEY_CURRENT_USER)
        return SALAMANDER_ROOT_REG != NULL && name != NULL && strcmp(name, SALAMANDER_ROOT_REG) == 0;

    if (key == HKEY_LOCAL_MACHINE || key == HKEY_CLASSES_ROOT || key == HKEY_USERS || key == HKEY_CURRENT_CONFIG)
        return FALSE;

    for (int i = 0; i < ActiveRegistryKeys.Count; i++)
        if (ActiveRegistryKeys[i] == key)
            return TRUE;

    return FALSE;
}

BOOL CConfigurationStorage::OpenConfigurationRootKey(HKEY& key, BOOL createKey)
{
    if (SALAMANDER_ROOT_REG == NULL)
        return FALSE;

    CSalamanderRegistryExAbstract* registry = GetRegistry();
    if (registry != NULL)
    {
        if (createKey)
        {
            if (registry->CreateKey(HKEY_CURRENT_USER, SALAMANDER_ROOT_REG, key))
            {
                RegisterActiveRegistryKey(key);
                return TRUE;
            }
            return FALSE;
        }
        if (registry->OpenKey(HKEY_CURRENT_USER, SALAMANDER_ROOT_REG, key))
        {
            RegisterActiveRegistryKey(key);
            return TRUE;
        }
        return FALSE;
    }

    if (createKey)
        return ::CreateKey(HKEY_CURRENT_USER, SALAMANDER_ROOT_REG, key);
    return ::OpenKey(HKEY_CURRENT_USER, SALAMANDER_ROOT_REG, key);
}

BOOL CConfigurationStorage::GetPortableConfigFilePath(char* filePath, int filePathSize)
{
    if (filePath == NULL || filePathSize <= 0)
        return FALSE;

    DWORD len = GetModuleFileName(NULL, filePath, filePathSize);
    if (len == 0 || len >= (DWORD)filePathSize)
        return FALSE;

    char* slash = strrchr(filePath, '\\');
    if (slash != NULL)
        slash++;
    else
        slash = filePath;

    strcpy_s(slash, filePathSize - (int)(slash - filePath), "config.reg");
    return TRUE;
}

void CConfigurationStorage::ShowRegFileLoadError(const char* fileName, eRPE_ERROR err)
{
    char text[MAX_PATH + 300];
    _snprintf_s(text, _TRUNCATE,
                "Unable to load portable configuration file:\n\n%s\n\nThe file is not a valid Registry configuration file or contains unsupported data (parser error %d). The original file was left unchanged.",
                fileName != NULL ? fileName : "config.reg", err);
    SalMessageBox(NULL, text, LoadStr(IDS_ERRORLOADCONFIG), MB_OK | MB_ICONEXCLAMATION);
}

void CConfigurationStorage::ShowRegFileSaveError(const char* fileName, DWORD err)
{
    char text[MAX_PATH + 300];
    _snprintf_s(text, _TRUNCATE,
                "Unable to save portable configuration file:\n\n%s\n\nThe previous configuration file was left unchanged.\n\n%s",
                fileName != NULL ? fileName : "config.reg", err != 0 ? GetErrorText(err) : "");
    SalMessageBox(NULL, text, LoadStr(IDS_ERRORSAVECONFIG), MB_OK | MB_ICONEXCLAMATION);
}

BOOL CConfigurationStorage::LoadRegFile(CSalamanderRegistryExAbstract* registry)
{
    if (registry == NULL)
        return FALSE;

    if (FilePath[0] == 0 && !GetPortableConfigFilePath(FilePath, SizeOf(FilePath)))
        return FALSE;

    DWORD attrs = GetFileAttributes(FilePath);
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        DWORD err = GetLastError();
        return err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND;
    }
    if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
        ShowRegFileLoadError(FilePath, RPE_NOT_REG_FILE);
        return FALSE;
    }

    HANDLE file = HANDLES_Q(CreateFile(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                                       OPEN_EXISTING, 0, 0));
    if (file == INVALID_HANDLE_VALUE)
    {
        ShowRegFileLoadError(FilePath, RPE_KEY_OPEN);
        return FALSE;
    }

    BOOL ret = FALSE;
    LARGE_INTEGER size;
    size.QuadPart = 0;
    if (GetFileSizeEx(file, &size) && size.QuadPart == 0)
        ret = TRUE; // empty portable config means clean/default configuration, same as a missing Registry key
    else if (size.QuadPart > 0 && size.HighPart == 0)
    {
        LPTSTR buf = (LPTSTR)malloc(size.LowPart + sizeof(WCHAR));
        if (buf != NULL)
        {
            DWORD bytesRead;
            if (ReadFile(file, buf, size.LowPart, &bytesRead, NULL))
            {
                *(WCHAR*)((LPBYTE)buf + bytesRead) = 0;
                if (ConvertIfNeeded(&buf, bytesRead) != 0)
                {
                    eRPE_ERROR regerr = Parse(buf, registry, DoNotDeleteHiddenKeysAndValues);
                    ret = regerr == RPE_OK;
                    if (!ret)
                        ShowRegFileLoadError(FilePath, regerr);
                }
                else
                    ShowRegFileLoadError(FilePath, RPE_OUT_OF_MEMORY);
            }
            else
                ShowRegFileLoadError(FilePath, RPE_KEY_OPEN);
            free(buf);
        }
        else
            ShowRegFileLoadError(FilePath, RPE_OUT_OF_MEMORY);
    }
    else
        ShowRegFileLoadError(FilePath, RPE_INVALID_FORMAT);

    HANDLES(CloseHandle(file));
    return ret;
}

BOOL CConfigurationStorage::SaveRegFile()
{
    if (Registry == NULL)
        return FALSE;

    if (FilePath[0] == 0 && !GetPortableConfigFilePath(FilePath, SizeOf(FilePath)))
        return FALSE;

    char tmpFileName[MAX_PATH];
    _snprintf_s(tmpFileName, _TRUNCATE, "%s.tmp", FilePath);

    char clearKeyName[MAX_PATH];
    const char* clearKeyNamePtr = BuildRootKeyName(clearKeyName, SizeOf(clearKeyName)) ? clearKeyName : NULL;

    LoadSaveToRegistryMutex.Enter();
    DeleteFile(tmpFileName);
    // Portable config is the live configuration store, so preserve hidden entries.
    // ExportConfiguration() still removes hidden entries for user-visible exports.
    BOOL dumped = Registry->Dump(tmpFileName, clearKeyNamePtr);
    DWORD err = dumped ? ERROR_SUCCESS : GetLastError();
    if (dumped && !MoveFileEx(tmpFileName, FilePath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        err = GetLastError();
        dumped = FALSE;
    }
    LoadSaveToRegistryMutex.Leave();

    if (!dumped)
    {
        DeleteFile(tmpFileName);
        ShowRegFileSaveError(FilePath, err);
        return FALSE;
    }

    return TRUE;
}

BOOL CConfigurationStorage::Initialize(CConfigurationStorageType type, const char* filePath)
{
    Release();
    ActiveRegistryKeys.ResetState();

    StorageType = type;
    if (filePath != NULL && filePath[0] != 0)
        strncpy_s(FilePath, filePath, _TRUNCATE);
    else
        FilePath[0] = 0;

    if (StorageType == cstRegFile && FilePath[0] == 0 && !GetPortableConfigFilePath(FilePath, SizeOf(FilePath)))
        return FALSE;

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

    CSalamanderRegistryExAbstract* oldRegistry = Registry;
    CConfigurationStorageType oldType = StorageType;
    Registry = newRegistry;
    StorageType = newType;
    ActiveRegistryKeys.ResetState();

    if (StorageType == cstRegFile && migrateCurrentData && !SaveRegFile())
    {
        Registry = oldRegistry;
        StorageType = oldType;
        newRegistry->Release();
        return FALSE;
    }

    if (oldType == cstRegistry && StorageType == cstRegFile && migrateCurrentData)
        DeleteStoredRegistryConfiguration(SALAMANDER_ROOT_REG);

    if (oldRegistry != NULL)
        oldRegistry->Release();

    if (StorageType == cstRegistry)
    {
        char portablePath[MAX_PATH];
        if (GetPortableConfigFilePath(portablePath, SizeOf(portablePath)))
            DeleteFile(portablePath);
    }

    SaveStorageTypeBootstrap(StorageType);
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

    return SaveRegFile();
}

BOOL CConfigurationStorage::Flush()
{
    return Save();
}

void CConfigurationStorage::Release()
{
    ActiveRegistryKeys.ResetState();
    if (Registry != NULL)
    {
        if (StorageType == cstRegFile)
            Save();
        Registry->Release();
        Registry = NULL;
    }
}

void CConfigurationStorage::RegisterActiveRegistryKey(HKEY key)
{
    if (key == NULL || StorageType != cstRegFile)
        return;

    for (int i = 0; i < ActiveRegistryKeys.Count; i++)
        if (ActiveRegistryKeys[i] == key)
            return;

    ActiveRegistryKeys.Add(key);
}

void CConfigurationStorage::UnregisterActiveRegistryKey(HKEY key)
{
    for (int i = 0; i < ActiveRegistryKeys.Count; i++)
    {
        if (ActiveRegistryKeys[i] == key)
        {
            ActiveRegistryKeys.Delete(i);
            return;
        }
    }
}
