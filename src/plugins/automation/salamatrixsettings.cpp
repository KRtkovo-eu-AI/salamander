// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "salamatrixsettings.h"

#include <limits>

namespace
{
const char kVersionKey[] = "salamatrix.settings.version";
const size_t kMaxMigrationRecords = 32;

BOOL ReadString(Salamatrix::Storage::IStorageService* storage,
                const char* extensionId, const char* key,
                std::vector<char>& value)
{
    int required = 0;
    storage->GetString(extensionId, key, NULL, 0, &required);
    if (required <= 0)
        return FALSE;
    value.assign(static_cast<size_t>(required), '\0');
    return storage->GetString(extensionId, key, &value[0], required, &required);
}

BOOL CopyTypedValue(Salamatrix::Storage::IStorageService* storage,
                    const char* extensionId, const char* source,
                    const char* destination)
{
    switch (storage->GetValueType(extensionId, source))
    {
    case Salamatrix::Storage::StorageValueString:
    {
        std::vector<char> value;
        return ReadString(storage, extensionId, source, value) &&
               storage->SetString(extensionId, destination, &value[0]);
    }
    case Salamatrix::Storage::StorageValueInteger:
    {
        LONGLONG value = 0;
        return storage->GetInteger(extensionId, source, &value) &&
               storage->SetInteger(extensionId, destination, value);
    }
    case Salamatrix::Storage::StorageValueBoolean:
    {
        BOOL value = FALSE;
        return storage->GetBoolean(extensionId, source, &value) &&
               storage->SetBoolean(extensionId, destination, value);
    }
    case Salamatrix::Storage::StorageValueMissing:
        return TRUE;
    default:
        return FALSE;
    }
}

BOOL ApplyRecord(Salamatrix::Storage::IStorageService* storage,
                 const char* extensionId,
                 const CExtensionManifestSettingMigration& migration)
{
    if (migration.FromVersion >= migration.ToVersion ||
        migration.Operations.empty())
        return FALSE;

    for (size_t i = 0; i < migration.Operations.size(); ++i)
    {
        const CExtensionManifestSettingMigrationOperation& operation =
            migration.Operations[i];
        if (operation.FromKey.empty() ||
            (!operation.Remove && operation.ToKey.empty()))
            return FALSE;

        if (operation.Remove)
        {
            if (!storage->DeleteValue(extensionId, operation.FromKey.c_str()) &&
                storage->GetValueType(extensionId, operation.FromKey.c_str()) !=
                    Salamatrix::Storage::StorageValueMissing)
                return FALSE;
            continue;
        }

        if (storage->GetValueType(extensionId, operation.FromKey.c_str()) ==
            Salamatrix::Storage::StorageValueMissing)
            continue;
        if (storage->GetValueType(extensionId, operation.ToKey.c_str()) ==
                Salamatrix::Storage::StorageValueMissing &&
            !CopyTypedValue(storage, extensionId, operation.FromKey.c_str(),
                            operation.ToKey.c_str()))
            return FALSE;
        if (!storage->DeleteValue(extensionId, operation.FromKey.c_str()) &&
            storage->GetValueType(extensionId, operation.FromKey.c_str()) !=
                Salamatrix::Storage::StorageValueMissing)
            return FALSE;
    }
    return TRUE;
}
}

BOOL ApplySalamatrixSettingsMigrations(
    Salamatrix::Storage::IStorageService* storage, const char* extensionId,
    unsigned int targetVersion,
    const std::vector<CExtensionManifestSettingMigration>& migrations)
{
    if (storage == NULL || extensionId == NULL || extensionId[0] == '\0' ||
        targetVersion > 65535 || migrations.size() > kMaxMigrationRecords)
        return FALSE;

    unsigned int currentVersion = 0;
    const Salamatrix::Storage::StorageValueType versionType =
        storage->GetValueType(extensionId, kVersionKey);
    if (versionType != Salamatrix::Storage::StorageValueMissing)
    {
        LONGLONG stored = 0;
        if (versionType != Salamatrix::Storage::StorageValueInteger ||
            !storage->GetInteger(extensionId, kVersionKey, &stored) ||
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
            extensionId, kVersionKey, static_cast<LONGLONG>(targetVersion));

    for (size_t count = 0; currentVersion < targetVersion; ++count)
    {
        if (count >= kMaxMigrationRecords)
            return FALSE;
        const CExtensionManifestSettingMigration* next = NULL;
        for (size_t i = 0; i < migrations.size(); ++i)
        {
            if (migrations[i].FromVersion == currentVersion)
            {
                if (next != NULL)
                    return FALSE;
                next = &migrations[i];
            }
        }
        if (next == NULL || next->ToVersion > targetVersion ||
            next->ToVersion <= currentVersion ||
            !ApplyRecord(storage, extensionId, *next) ||
            !storage->SetInteger(extensionId, kVersionKey,
                                 static_cast<LONGLONG>(next->ToVersion)))
            return FALSE;
        currentVersion = next->ToVersion;
    }
    return TRUE;
}
