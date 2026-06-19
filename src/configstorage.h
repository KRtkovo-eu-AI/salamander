// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "reglib\src\regparse.h"

enum CConfigurationStorageType
{
    cstRegistry,
    cstRegFile,
};

class CConfigurationStorage
{
protected:
    CConfigurationStorageType StorageType;
    CSalamanderRegistryExAbstract* Registry;
    char FilePath[MAX_PATH];
    TDirectArray<HKEY> ActiveRegistryKeys;
    BOOL DoNotDeleteHiddenKeysAndValues;

    CSalamanderRegistryExAbstract* CreateRegistry(CConfigurationStorageType type);
    BOOL BuildRootKeyName(char* keyName, int keyNameSize);
    BOOL LoadRegFile(CSalamanderRegistryExAbstract* registry);
    BOOL SaveRegFile();
    void ShowRegFileLoadError(const char* fileName, eRPE_ERROR err);
    void ShowRegFileSaveError(const char* fileName, DWORD err);
    void DeleteStoredRegistryConfiguration(CSalamanderRegistryExAbstract* registry, const char* keyName);

public:
    CConfigurationStorage();
    ~CConfigurationStorage();

    BOOL Initialize(CConfigurationStorageType type, const char* filePath);
    CSalamanderRegistryExAbstract* GetRegistry();
    CConfigurationStorageType GetStorageType() const;
    BOOL SwitchStorageType(CConfigurationStorageType newType, BOOL migrateCurrentData);
    BOOL Load();
    BOOL Save();
    BOOL Flush();
    void Release();

    BOOL GetPortableConfigFilePath(char* filePath, int filePathSize);
    BOOL GetStorageTypeBootstrapFilePath(char* filePath, int filePathSize);
    BOOL LoadStorageTypeBootstrap(CConfigurationStorageType& type);
    BOOL SaveStorageTypeBootstrap(CConfigurationStorageType type);
    BOOL OpenConfigurationRootKey(HKEY& key, BOOL createKey);
    void RegisterActiveRegistryKey(HKEY key);
    void UnregisterActiveRegistryKey(HKEY key);
    BOOL UseActiveRegistryForKey(HKEY key, const char* name = NULL);
};

extern CConfigurationStorage ConfigurationStorage;
