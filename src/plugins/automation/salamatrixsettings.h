// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>

#include "../salamatrix/salamatrix_manifest.h"
#include "../salamatrix/salamatrix_storage.h"

BOOL ApplySalamatrixSettingsMigrations(
    Salamatrix::Storage::IStorageService* storage,
    const char* extensionId,
    unsigned int targetVersion,
    const std::vector<CExtensionManifestSettingMigration>& migrations);
