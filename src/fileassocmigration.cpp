// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-07 | Report the runtime projection gate. | Prevent target-only migrated rows from being mistaken for a complete legacy runtime projection. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Add loss-aware legacy association migration. | Prevent grouped masks, exclusions, malformed masks, and unstable targets from entering the canonical model. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |

#include "precomp.h"
#include "fileassocmigration.h"
#include "masks.h"

#include <cstring>

namespace Salamander
{
namespace FileAssociations
{

namespace
{

bool HasStableTargetIdentity(const Target& target)
{
    if (target.Kind == TargetKind::Unassigned)
        return false;
    return !target.StableId.empty();
}

bool IsSingleValidMask(const char* masks)
{
    if (masks == NULL || masks[0] == 0 || strchr(masks, ';') != NULL ||
        strchr(masks, '|') != NULL)
        return false;

    CMaskGroup maskGroup(masks);
    int errorPosition = 0;
    return maskGroup.PrepareMasks(errorPosition);
}

} // namespace

bool MigrateLegacyAssociations(CFileAssociationModel* model,
                               const LegacyAssociationCandidate* candidates,
                               size_t candidateCount,
                               LegacyMigrationReport* report,
                               std::string* error)
{
    if (error != NULL)
        error->clear();
    if (report != NULL)
        *report = LegacyMigrationReport();

    if (model == NULL || (candidateCount != 0 && candidates == NULL))
    {
        if (error != NULL)
            *error = "legacy association migration arguments are invalid";
        return false;
    }

    CFileAssociationModel staged = *model;
    LegacyMigrationReport localReport;
    localReport.RuntimeProjectionBlocked = candidateCount != 0;
    for (size_t i = 0; i < candidateCount; ++i)
    {
        const LegacyAssociationCandidate& candidate = candidates[i];
        if (!HasStableTargetIdentity(candidate.AssociationTarget) ||
            !IsSingleValidMask(candidate.Masks))
        {
            if (candidate.Masks != NULL &&
                (strchr(candidate.Masks, ';') != NULL || strchr(candidate.Masks, '|') != NULL))
                ++localReport.BlockedCount;
            else
                ++localReport.InvalidCount;
            continue;
        }

        std::string candidateError;
        if (!staged.AddLegacy(candidate.Masks, candidate.AssociationCategory,
                              candidate.Operation, candidate.AssociationTarget,
                              &candidateError))
        {
            ++localReport.InvalidCount;
            if (error != NULL && error->empty())
                *error = candidateError;
        }
        else
            ++localReport.ImportedCount;
    }

    if (localReport.BlockedCount != 0 || localReport.InvalidCount != 0)
    {
        if (error != NULL && error->empty())
            *error = "legacy association migration would lose information";
        if (report != NULL)
        {
            report->BlockedCount = localReport.BlockedCount;
            report->InvalidCount = localReport.InvalidCount;
            report->RuntimeProjectionBlocked = true;
        }
        return false;
    }

    *model = staged;
    if (report != NULL)
        *report = localReport;
    return true;
}

} // namespace FileAssociations
} // namespace Salamander
