// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Project only effective user-overridden association targets. | Prevent shadowed plugin/default rows from defeating an explicit user choice in runtime lookup order. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 |
// | 2026-11-08 | Require explicit target resolution before projection. | Prevent runtime application of associations whose stable target or legacy metadata cannot be resolved. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-08 | Add a transactional runtime projection boundary. | Prevent incomplete target resolution from partially replacing legacy runtime associations. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |

#include "precomp.h"
#include "fileassocprojection.h"

namespace Salamander
{
namespace FileAssociations
{

namespace
{

void SetProjectionError(std::string* error, const char* message)
{
    if (error != NULL && error->empty())
        *error = message;
}

void SetBlockedReport(RuntimeProjectionReport* report, size_t blockedCount)
{
    if (report != NULL)
    {
        report->CommittedCount = 0;
        report->BlockedCount = blockedCount;
        report->RuntimeProjectionBlocked = true;
    }
}

bool IsShadowedByUserOverride(const std::vector<Association>& associations, size_t index)
{
    const Association& item = associations[index];
    if (item.AssociationProvenance == Provenance::Disabled)
        return true;
    if (item.AssociationProvenance == Provenance::User)
        return false;

    for (size_t i = 0; i < associations.size(); ++i)
    {
        const Association& candidate = associations[i];
        if (candidate.Pattern == item.Pattern &&
            candidate.AssociationCategory == item.AssociationCategory &&
            candidate.Operation == item.Operation &&
            (candidate.AssociationProvenance == Provenance::User ||
             candidate.AssociationProvenance == Provenance::Disabled))
            return true;
    }
    return false;
}

} // namespace

bool ProjectRuntimeAssociations(const CFileAssociationModel& model,
                                const RuntimeProjectionOperations& operations,
                                RuntimeProjectionReport* report,
                                std::string* error)
{
    if (error != NULL)
        error->clear();
    if (report != NULL)
        *report = RuntimeProjectionReport();

    if (!model.Validate(error))
    {
        SetBlockedReport(report, 1);
        return false;
    }

    const std::vector<Association>& associations = model.Associations();
    if (associations.empty())
        return true;

    if (operations.Begin == NULL || operations.Resolve == NULL || operations.Apply == NULL ||
        operations.Rollback == NULL || operations.Commit == NULL)
    {
        SetProjectionError(error, "runtime association projection operations are unavailable");
        SetBlockedReport(report, associations.size());
        return false;
    }

    if (!operations.Begin(operations.Context))
    {
        SetProjectionError(error, "runtime association projection could not begin");
        SetBlockedReport(report, associations.size());
        return false;
    }

    for (size_t i = 0; i < associations.size(); ++i)
    {
        if (IsShadowedByUserOverride(associations, i))
            continue;
        if (!operations.Resolve(associations[i], operations.ResolveContext))
        {
            operations.Rollback(operations.Context);
            SetProjectionError(error, "runtime association target or metadata could not be resolved");
            SetBlockedReport(report, associations.size() - i);
            return false;
        }
        if (!operations.Apply(associations[i], operations.Context))
        {
            operations.Rollback(operations.Context);
            SetProjectionError(error, "runtime association target or metadata could not be resolved");
            SetBlockedReport(report, associations.size() - i);
            return false;
        }
    }

    if (!operations.Commit(operations.Context))
    {
        operations.Rollback(operations.Context);
        SetProjectionError(error, "runtime association projection could not be committed");
        SetBlockedReport(report, associations.size());
        return false;
    }

    if (report != NULL)
        report->CommittedCount = associations.size();
    return true;
}

} // namespace FileAssociations
} // namespace Salamander
