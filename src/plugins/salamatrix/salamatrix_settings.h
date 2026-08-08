// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <limits>
#include <vector>

#include "salamatrix_manifest.h"
#include "salamatrix_storage.h"

namespace Salamatrix
{
namespace Settings
{

static const char VersionKey[] = "salamatrix.settings.version";
static const size_t MaxMigrationRecords = 32;

inline BOOL ReadString(
    Storage::IStorageService* storage,
    const char* extensionId,
    const char* key,
    std::vector<char>* value)
{
    if (storage == NULL || value == NULL)
        return FALSE;
    int required = 0;
    storage->GetString(extensionId, key, NULL, 0, &required);
    if (required <= 0)
        return FALSE;
    value->assign(static_cast<size_t>(required), '\0');
    return storage->GetString(
        extensionId, key, &(*value)[0], required, &required);
}

inline BOOL CopyTypedValue(
    Storage::IStorageService* storage,
    const char* extensionId,
    const char* source,
    const char* destination)
{
    switch (storage->GetValueType(extensionId, source))
    {
    case Storage::StorageValueString:
    {
        std::vector<char> value;
        return ReadString(storage, extensionId, source, &value) &&
               storage->SetString(extensionId, destination, &value[0]);
    }
    case Storage::StorageValueInteger:
    {
        LONGLONG value = 0;
        return storage->GetInteger(extensionId, source, &value) &&
               storage->SetInteger(extensionId, destination, value);
    }
    case Storage::StorageValueBoolean:
    {
        BOOL value = FALSE;
        return storage->GetBoolean(extensionId, source, &value) &&
               storage->SetBoolean(extensionId, destination, value);
    }
    case Storage::StorageValueMissing:
        return TRUE;
    default:
        return FALSE;
    }
}

inline BOOL ApplyMigrationRecord(
    Storage::IStorageService* storage,
    const char* extensionId,
    const CExtensionManifestSettingMigration& migration)
{
    if (migration.FromVersion >= migration.ToVersion ||
        migration.Operations.empty())
        return FALSE;

    for (size_t index = 0; index < migration.Operations.size(); ++index)
    {
        const CExtensionManifestSettingMigrationOperation& operation =
            migration.Operations[index];
        if (operation.FromKey.empty() ||
            (!operation.Remove && operation.ToKey.empty()))
            return FALSE;

        if (operation.Remove)
        {
            if (!storage->DeleteValue(
                    extensionId, operation.FromKey.c_str()) &&
                storage->GetValueType(
                    extensionId, operation.FromKey.c_str()) !=
                    Storage::StorageValueMissing)
                return FALSE;
            continue;
        }

        if (storage->GetValueType(
                extensionId, operation.FromKey.c_str()) ==
            Storage::StorageValueMissing)
            continue;
        if (storage->GetValueType(
                extensionId, operation.ToKey.c_str()) ==
                Storage::StorageValueMissing &&
            !CopyTypedValue(
                storage, extensionId, operation.FromKey.c_str(),
                operation.ToKey.c_str()))
            return FALSE;
        if (!storage->DeleteValue(extensionId, operation.FromKey.c_str()) &&
            storage->GetValueType(
                extensionId, operation.FromKey.c_str()) !=
                Storage::StorageValueMissing)
            return FALSE;
    }
    return TRUE;
}

inline BOOL ApplyMigrations(
    Storage::IStorageService* storage,
    const char* extensionId,
    unsigned int targetVersion,
    const std::vector<CExtensionManifestSettingMigration>& migrations)
{
    if (storage == NULL || extensionId == NULL || extensionId[0] == '\0' ||
        targetVersion > 65535 || migrations.size() > MaxMigrationRecords)
        return FALSE;

    unsigned int currentVersion = 0;
    const Storage::StorageValueType versionType =
        storage->GetValueType(extensionId, VersionKey);
    if (versionType != Storage::StorageValueMissing)
    {
        LONGLONG stored = 0;
        if (versionType != Storage::StorageValueInteger ||
            !storage->GetInteger(extensionId, VersionKey, &stored) ||
            stored < 0 ||
            static_cast<unsigned long long>(stored) >
                static_cast<unsigned long long>(UINT_MAX))
            return FALSE;
        currentVersion = static_cast<unsigned int>(stored);
    }
    if (currentVersion >= targetVersion)
        return TRUE;

    if (migrations.empty())
        return storage->SetInteger(
            extensionId, VersionKey, static_cast<LONGLONG>(targetVersion));

    for (size_t count = 0; currentVersion < targetVersion; ++count)
    {
        if (count >= MaxMigrationRecords)
            return FALSE;
        const CExtensionManifestSettingMigration* next = NULL;
        for (size_t index = 0; index < migrations.size(); ++index)
        {
            if (migrations[index].FromVersion == currentVersion)
            {
                if (next != NULL)
                    return FALSE;
                next = &migrations[index];
            }
        }
        if (next == NULL || next->ToVersion > targetVersion ||
            next->ToVersion <= currentVersion ||
            !ApplyMigrationRecord(storage, extensionId, *next) ||
            !storage->SetInteger(
                extensionId, VersionKey,
                static_cast<LONGLONG>(next->ToVersion)))
            return FALSE;
        currentVersion = next->ToVersion;
    }
    return TRUE;
}

inline BOOL MaterializeDefaults(
    Storage::IStorageService* storage,
    const char* extensionId,
    const std::vector<CExtensionManifestSetting>& settings)
{
    if (storage == NULL || extensionId == NULL || extensionId[0] == '\0')
        return FALSE;
    for (size_t index = 0; index < settings.size(); ++index)
    {
        const CExtensionManifestSetting& setting = settings[index];
        if (!setting.HasDefault ||
            storage->GetValueType(extensionId, setting.Key.c_str()) !=
                Storage::StorageValueMissing)
            continue;
        BOOL stored = FALSE;
        if (setting.Type == ExtensionManifestSettingString)
            stored = storage->SetString(
                extensionId, setting.Key.c_str(),
                setting.StringDefault.c_str());
        else if (setting.Type == ExtensionManifestSettingInteger)
            stored = storage->SetInteger(
                extensionId, setting.Key.c_str(), setting.IntegerDefault);
        else if (setting.Type == ExtensionManifestSettingBoolean)
            stored = storage->SetBoolean(
                extensionId, setting.Key.c_str(),
                setting.BooleanDefault ? TRUE : FALSE);
        if (!stored)
            return FALSE;
    }
    return TRUE;
}

} // namespace Settings
} // namespace Salamatrix
