// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Collect legacy viewer, editor, and archive defaults. | Populate the canonical page from the existing runtime configuration instead of asking users to retype every pattern. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Report the runtime projection gate. | Prevent target-only migrated rows from being mistaken for a complete legacy runtime projection. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Add loss-aware legacy association migration. | Prevent grouped masks, exclusions, malformed masks, and unstable targets from entering the canonical model. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |

#include "precomp.h"
#include "fileassocmigration.h"
#include "masks.h"
#include "cfgdlg.h"
#include "mainwnd.h"
#include "pack.h"
#include "plugins.h"

#include <cstring>
#include <string>

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

void AddLegacyMaskList(CFileAssociationModel* model, const char* masks,
                       Category category, PanelOperation operation,
                       const Target& target, LegacyMigrationReport* report,
                       std::string* error)
{
    if (model == NULL || masks == NULL)
        return;

    const char* begin = masks;
    for (const char* p = masks;; ++p)
    {
        if (*p != ';' && *p != 0)
            continue;

        std::string one(begin, static_cast<size_t>(p - begin));
        while (!one.empty() && (one[0] == ' ' || one[0] == '\t'))
            one.erase(one.begin());
        while (!one.empty() && (one[one.size() - 1] == ' ' || one[one.size() - 1] == '\t'))
            one.erase(one.size() - 1);

        if (!one.empty())
        {
            std::string localError;
            if (model->AddLegacy(one.c_str(), category, operation, target, &localError))
            {
                if (report != NULL)
                    ++report->ImportedCount;
            }
            else
            {
                if (report != NULL)
                    ++report->InvalidCount;
                if (error != NULL && error->empty())
                    *error = localError;
            }
        }
        if (*p == 0)
            break;
        begin = p + 1;
    }
}

Target LegacyRuntimeTarget(int runtimeIndex, bool builtInNegative = false)
{
    if (runtimeIndex >= 0 || builtInNegative)
        return Target(TargetKind::BuiltIn, "legacy");

    const int pluginIndex = -runtimeIndex - 1;
    CPluginData* plugin = pluginIndex >= 0 && pluginIndex < Plugins.GetCount()
                              ? Plugins.Get(pluginIndex)
                              : NULL;
    if (plugin != NULL && plugin->GetStableId()[0] != 0)
        return Target(TargetKind::Plugin, plugin->GetStableId());
    return Target(TargetKind::Unassigned);
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

bool BuildLegacyAssociationDefaults(CFileAssociationModel* model,
                                    LegacyMigrationReport* report,
                                    std::string* error)
{
    if (error != NULL)
        error->clear();
    if (report != NULL)
        *report = LegacyMigrationReport();
    if (model == NULL)
    {
        if (error != NULL)
            *error = "legacy association model output is required";
        return false;
    }

    CFileAssociationModel staged;
    if (MainWindow != NULL)
    {
        CViewerMasks* viewerTables[2] = {MainWindow->ViewerMasks, MainWindow->AltViewerMasks};
        const Category categories[2] = {Category::View, Category::AlternateView};
        for (int table = 0; table < 2; ++table)
        {
            CViewerMasks* source = viewerTables[table];
            if (source == NULL)
                continue;
            for (int i = 0; i < source->Count; ++i)
            {
                CViewerMasksItem* item = source->At(i);
                if (item == NULL || item->Masks == NULL)
                    continue;
                AddLegacyMaskList(&staged, item->Masks->GetMasksString(), categories[table],
                                  PanelOperation::NotApplicable,
                                  LegacyRuntimeTarget(item->ViewerType), report, error);
            }
        }

        if (MainWindow->EditorMasks != NULL)
        {
            for (int i = 0; i < MainWindow->EditorMasks->Count; ++i)
            {
                CEditorMasksItem* item = MainWindow->EditorMasks->At(i);
                if (item == NULL || item->Masks == NULL)
                    continue;
                AddLegacyMaskList(&staged, item->Masks->GetMasksString(), Category::Edit,
                                  PanelOperation::NotApplicable,
                                  Target(TargetKind::Preset, "legacy-editor"), report, error);
            }
        }
    }

    for (int i = 0; i < PackerFormatConfig.GetFormatsCount(); ++i)
    {
        AddLegacyMaskList(&staged, PackerFormatConfig.GetExt(i), Category::PanelArchive,
                          PanelOperation::ListUnpack,
                          LegacyRuntimeTarget(PackerFormatConfig.GetUnpackerIndex(i),
                                              PackerFormatConfig.GetOldType(i) != FALSE), report, error);
        if (PackerFormatConfig.GetUsePacker(i))
            AddLegacyMaskList(&staged, PackerFormatConfig.GetExt(i), Category::PanelArchive,
                              PanelOperation::DeletePack,
                              LegacyRuntimeTarget(PackerFormatConfig.GetPackerIndex(i),
                                                  PackerFormatConfig.GetOldType(i) != FALSE), report, error);
    }

    if (report != NULL)
        report->RuntimeProjectionBlocked = report->InvalidCount != 0;
    *model = staged;
    return report == NULL || report->InvalidCount == 0;
}

} // namespace FileAssociations
} // namespace Salamander
