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


void CConfigurationStorage::DeleteStoredRegistryConfiguration(CSalamanderRegistryExAbstract* registry, const char* keyName)
{
    HKEY key;
    if (registry != NULL && keyName != NULL && registry->OpenKey(HKEY_CURRENT_USER, keyName, key))
    {
        registry->ClearKey(key);
        registry->CloseKey(key);
        registry->DeleteKey(HKEY_CURRENT_USER, keyName);
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

BOOL CConfigurationStorage::LoadStorageTypeBootstrap(CConfigurationStorageType& type, char* regFilePath, int regFilePathSize)
{
    char fileName[SAL_MAX_PATH];
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

    if (regFilePath != NULL && regFilePathSize > 0)
    {
        regFilePath[0] = 0;
        GetPrivateProfileString("Configuration", "RegFilePath", "", regFilePath, regFilePathSize, fileName);
        if (regFilePath[0] == 0)
            GetPortableConfigFilePath(regFilePath, regFilePathSize);
    }
    return TRUE;
}


BOOL CConfigurationStorage::CanSaveStorageTypeBootstrap()
{
    char fileName[SAL_MAX_PATH];
    if (!GetStorageTypeBootstrapFilePath(fileName, SizeOf(fileName)))
        return FALSE;

    DWORD attrs = GetFileAttributes(fileName);
    if (attrs != INVALID_FILE_ATTRIBUTES)
    {
        HANDLE file = HANDLES_Q(CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL, NULL));
        if (file == INVALID_HANDLE_VALUE)
            return FALSE;
        HANDLES(CloseHandle(file));
        return TRUE;
    }

    char tmpPath[SAL_MAX_PATH];
    _snprintf_s(tmpPath, _TRUNCATE, "%s.%lu.test", fileName, GetCurrentProcessId());
    HANDLE file = HANDLES_Q(CreateFile(tmpPath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, NULL));
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    HANDLES(CloseHandle(file));
    DeleteFile(tmpPath);
    return TRUE;
}

BOOL CConfigurationStorage::SaveStorageTypeBootstrap(CConfigurationStorageType type, const char* regFilePath)
{
    char fileName[SAL_MAX_PATH];
    if (!GetStorageTypeBootstrapFilePath(fileName, SizeOf(fileName)))
        return FALSE;

    if (type == cstRegFile && (regFilePath == NULL || regFilePath[0] == 0))
        return FALSE;

    BOOL ret = WritePrivateProfileString("Configuration", "StorageType",
                                          type == cstRegFile ? "RegFile" : "Registry", fileName);
    if (ret && type == cstRegFile)
    {
        // RegFilePath is meaningful only for file-backed configuration storage.
        ret = WritePrivateProfileString("Configuration", "RegFilePath", regFilePath, fileName);
        if (ret)
            ret = AddKnownFileStoragePath(regFilePath);
    }
    else if (ret)
    {
        // Registry storage must not keep a stale file target from a previous selection.
        ret = WritePrivateProfileString("Configuration", "RegFilePath", NULL, fileName);
    }
    return ret;
}


BOOL CConfigurationStorage::GetRegFilePath(char* filePath, int filePathSize) const
{
    if (filePath == NULL || filePathSize <= 0)
        return FALSE;

    if (FilePath[0] != 0)
    {
        strncpy_s(filePath, filePathSize, FilePath, _TRUNCATE);
        return TRUE;
    }

    return const_cast<CConfigurationStorage*>(this)->GetPortableConfigFilePath(filePath, filePathSize);
}


BOOL CConfigurationStorage::CanWriteRegFile() const
{
    char path[SAL_MAX_PATH];
    if (!GetRegFilePath(path, SizeOf(path)) || path[0] == 0)
        return FALSE;

    DWORD attrs = GetFileAttributes(path);
    if (attrs != INVALID_FILE_ATTRIBUTES)
    {
        if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
            return FALSE;

        HANDLE file = HANDLES_Q(CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL, NULL));
        if (file == INVALID_HANDLE_VALUE)
            return FALSE;

        HANDLES(CloseHandle(file));
        return TRUE;
    }

    char tmpPath[SAL_MAX_PATH];
    _snprintf_s(tmpPath, _TRUNCATE, "%s.%lu.test", path, GetCurrentProcessId());
    HANDLE file = HANDLES_Q(CreateFile(tmpPath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, NULL));
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    HANDLES(CloseHandle(file));
    DeleteFile(tmpPath);
    return TRUE;
}

BOOL CConfigurationStorage::SetRegFilePath(const char* filePath)
{
    if (filePath == NULL || filePath[0] == 0)
        return FALSE;

    strncpy_s(FilePath, filePath, _TRUNCATE);
    return TRUE;
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

BOOL CConfigurationStorage::SaveUseWindowsDarkMode(BOOL useDark)
{
    char fileName[SAL_MAX_PATH];
    if (!GetStorageTypeBootstrapFilePath(fileName, SizeOf(fileName)))
        return FALSE;

    return WritePrivateProfileString("SplashScreen", "UseWindowsDarkMode",
                                     useDark ? "1" : "0", fileName);
}

BOOL CConfigurationStorage::LoadUseWindowsDarkMode(BOOL& useDark)
{
    useDark = FALSE;
    char fileName[SAL_MAX_PATH];
    if (!GetStorageTypeBootstrapFilePath(fileName, SizeOf(fileName)))
        return FALSE;

    char value[4];
    DWORD read = GetPrivateProfileString("SplashScreen", "UseWindowsDarkMode", "", value, SizeOf(value), fileName);
    if (read == 0)
        return FALSE;

    useDark = (value[0] == '1');
    return TRUE;
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

void CConfigurationStorage::ShowRegFileSaveError(HWND parent)
{
    SalMessageBox(parent, LoadStr(IDS_CFGSTORAGE_FILEWRITEERR), LoadStr(IDS_ERRORTITLE),
                  MB_OK | MB_ICONEXCLAMATION);
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

BOOL CConfigurationStorage::SaveRegFile(BOOL showError)
{
    if (Registry == NULL)
        return FALSE;

    if (FilePath[0] == 0 && !GetPortableConfigFilePath(FilePath, SizeOf(FilePath)))
        return FALSE;

    char tmpFileName[SAL_MAX_PATH];
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
        TRACE_E("Unable to save portable configuration file: " << FilePath << ", error: " << err);
        if (showError)
            ShowRegFileSaveError(NULL);
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

BOOL CConfigurationStorage::SwitchStorageType(CConfigurationStorageType newType, BOOL migrateCurrentData, const char* filePath)
{
    if (newType == StorageType)
    {
        if (newType == cstRegFile && filePath != NULL && filePath[0] != 0 && strcmp(FilePath, filePath) != 0)
        {
            char oldFilePath[SAL_MAX_PATH];
            strcpy_s(oldFilePath, FilePath);
            BOOL movedFile = FALSE;
            DWORD oldAttrs = oldFilePath[0] != 0 ? GetFileAttributes(oldFilePath) : INVALID_FILE_ATTRIBUTES;
            if (oldAttrs != INVALID_FILE_ATTRIBUTES && (oldAttrs & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
                _stricmp(oldFilePath, filePath) != 0)
            {
                if (!MoveFileEx(oldFilePath, filePath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH))
                    return FALSE;
                movedFile = TRUE;
            }
            if (!SetRegFilePath(filePath) || (!movedFile && !SaveRegFile()) || !SaveStorageTypeBootstrap(StorageType, FilePath))
            {
                if (movedFile)
                    MoveFileEx(filePath, oldFilePath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
                strcpy_s(FilePath, oldFilePath);
                return FALSE;
            }
            return TRUE;
        }
        return TRUE;
    }

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

    char oldFilePath[SAL_MAX_PATH];
    strcpy_s(oldFilePath, FilePath);
    if (newType == cstRegFile && filePath != NULL && filePath[0] != 0)
        strncpy_s(FilePath, filePath, _TRUNCATE);
    else if (newType == cstRegFile && FilePath[0] == 0 && !GetPortableConfigFilePath(FilePath, SizeOf(FilePath)))
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
        strcpy_s(FilePath, oldFilePath);
        newRegistry->Release();
        return FALSE;
    }
    if (StorageType == cstRegFile && oldType == cstRegFile && oldFilePath[0] != 0 &&
        _stricmp(oldFilePath, FilePath) != 0)
    {
        DeleteFile(oldFilePath);
    }

    if (oldType == cstRegistry && StorageType == cstRegFile && migrateCurrentData)
        DeleteStoredRegistryConfiguration(oldRegistry, SALAMANDER_ROOT_REG);

    if (oldRegistry != NULL)
        oldRegistry->Release();

    if (StorageType == cstRegistry)
    {
        if (oldType == cstRegFile && oldFilePath[0] != 0)
            DeleteFile(oldFilePath);
        else
        {
            char portablePath[SAL_MAX_PATH];
            if (GetPortableConfigFilePath(portablePath, SizeOf(portablePath)))
                DeleteFile(portablePath);
        }
    }

    SaveStorageTypeBootstrap(StorageType, StorageType == cstRegFile ? FilePath : NULL);
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

BOOL CConfigurationStorage::Save(BOOL showError)
{
    if (Registry == NULL)
        return FALSE;

    if (StorageType == cstRegistry)
        return TRUE;

    return SaveRegFile(showError);
}

BOOL CConfigurationStorage::Flush(BOOL showError)
{
    return Save(showError);
}

void CConfigurationStorage::Release()
{
    ActiveRegistryKeys.ResetState();
    if (Registry != NULL)
    {
        if (StorageType == cstRegFile)
            Save(FALSE);
        Registry->Release();
        Registry = NULL;
    }
}

void CConfigurationStorage::RegisterActiveRegistryKey(HKEY key)
{
    if (key == NULL || StorageType != cstRegFile)
        return;

    // The in-memory registry returns the same CKey pointer when the same key is
    // opened more than once. Keep every open reference so closing one handle
    // does not accidentally route the remaining one to the Windows registry.
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

BOOL CConfigurationStorage::LoadKnownFileStoragePaths(char paths[][SAL_MAX_PATH], int* count, int maxCount)
{
    *count = 0;
    char fileName[SAL_MAX_PATH];
    if (!GetStorageTypeBootstrapFilePath(fileName, SizeOf(fileName)))
        return FALSE;

    for (int i = 0; i < maxCount; i++)
    {
        char key[20];
        _snprintf_s(key, _TRUNCATE, "Path%d", i);
        char path[SAL_MAX_PATH];
        path[0] = 0;
        GetPrivateProfileString("KnownFileStorage", key, "", path, SizeOf(path), fileName);
        if (path[0] != 0)
        {
            strncpy_s(paths[*count], path, _TRUNCATE);
            (*count)++;
        }
        else
            break;
    }
    return *count > 0;
}

BOOL CConfigurationStorage::AddKnownFileStoragePath(const char* path)
{
    if (path == NULL || path[0] == 0)
        return FALSE;

    char fileName[SAL_MAX_PATH];
    if (!GetStorageTypeBootstrapFilePath(fileName, SizeOf(fileName)))
        return FALSE;

    // Nejprve zkontrolovat jestli cesta uz existuje
    static char existingPaths[20][SAL_MAX_PATH];
    int existingCount = 0;
    LoadKnownFileStoragePaths(existingPaths, &existingCount, 20);

    for (int i = 0; i < existingCount; i++)
    {
        if (_stricmp(existingPaths[i], path) == 0)
            return TRUE; // uz existuje
    }

    // Pridat na konec
    char key[20];
    _snprintf_s(key, _TRUNCATE, "Path%d", existingCount);
    return WritePrivateProfileString("KnownFileStorage", key, path, fileName);
}

BOOL CConfigurationStorage::RemoveKnownFileStoragePath(const char* path)
{
    if (path == NULL || path[0] == 0)
        return FALSE;

    char fileName[SAL_MAX_PATH];
    if (!GetStorageTypeBootstrapFilePath(fileName, SizeOf(fileName)))
        return FALSE;

    static char existingPaths[20][SAL_MAX_PATH];
    int existingCount = 0;
    LoadKnownFileStoragePaths(existingPaths, &existingCount, 20);

    // Najit a odstranit
    int found = -1;
    for (int i = 0; i < existingCount; i++)
    {
        if (_stricmp(existingPaths[i], path) == 0)
        {
            found = i;
            break;
        }
    }
    if (found < 0)
        return FALSE;

    // Presunout zbyvajici cesty
    for (int i = found; i < existingCount - 1; i++)
    {
        strncpy_s(existingPaths[i], existingPaths[i + 1], _TRUNCATE);
    }
    existingCount--;

    // Prepsat vsechny klice
    WritePrivateProfileString("KnownFileStorage", NULL, NULL, fileName); // smazat celou sekci
    for (int i = 0; i < existingCount; i++)
    {
        char key[20];
        _snprintf_s(key, _TRUNCATE, "Path%d", i);
        WritePrivateProfileString("KnownFileStorage", key, existingPaths[i], fileName);
    }
    return TRUE;
}
