// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "salamatrixsettings.h"
#include "../salamatrix/salamatrix_settings.h"

BOOL ApplySalamatrixSettingsMigrations(
    Salamatrix::Storage::IStorageService* storage, const char* extensionId,
    unsigned int targetVersion,
    const std::vector<CExtensionManifestSettingMigration>& migrations)
{
    return Salamatrix::Settings::ApplyMigrations(
        storage, extensionId, targetVersion, migrations);
}
