// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-07 | Report the runtime projection gate. | Prevent target-only migrated rows from being mistaken for a complete legacy runtime projection. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Add loss-aware legacy association migration. | Prevent grouped masks, exclusions, malformed masks, and unstable targets from entering the canonical model. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |

#pragma once

#include "fileassoc.h"

#include <cstddef>

namespace Salamander
{
namespace FileAssociations
{

struct LegacyAssociationCandidate
{
    const char* Masks;
    Category AssociationCategory;
    PanelOperation Operation;
    Target AssociationTarget;
};

struct LegacyMigrationReport
{
    size_t ImportedCount;
    size_t BlockedCount;
    size_t InvalidCount;
    bool RuntimeProjectionBlocked;

    LegacyMigrationReport()
        : ImportedCount(0), BlockedCount(0), InvalidCount(0),
          RuntimeProjectionBlocked(false) {}
};

// Imports a complete legacy batch only when every candidate has one valid mask
// and a stable target identity. A blocked or invalid candidate leaves the model
// unchanged so callers can keep the legacy runtime projection active. Even a
// successful target-only import reports RuntimeProjectionBlocked because the
// current runtime still requires command metadata and session-local indexes.
bool MigrateLegacyAssociations(CFileAssociationModel* model,
                               const LegacyAssociationCandidate* candidates,
                               size_t candidateCount,
                               LegacyMigrationReport* report,
                               std::string* error = NULL);

} // namespace FileAssociations
} // namespace Salamander
