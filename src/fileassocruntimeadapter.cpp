// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Restore migrated viewer owner labels during projection. | Keep extension ownership visible in the legacy viewer table and on subsequent saves. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-08 | Preserve imported legacy rows during projection. | Existing runtime rows already contain command metadata unavailable in the canonical record, so unchanged legacy rows must remain active while user edits are resolved. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-08 | Add the stable plugin runtime resolver. | Map canonical plugin targets to current runtime indexes only when plugin capabilities support the requested association. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |

#include "precomp.h"
#include "fileassocruntimeadapter.h"
#include "cfgdlg.h"
#include "mainwnd.h"
#include "pack.h"
#include "plugins.h"

namespace Salamander
{
namespace FileAssociations
{

namespace
{

struct RuntimeAdapterContext
{
    CMainWindow* MainWindow;
    PluginRuntimeResolverContext Resolver;
    CViewerMasks StagedViewers;
    CViewerMasks StagedAlternateViewers;
    CEditorMasks StagedEditors;
    CPackerFormatConfig StagedPacker;

    RuntimeAdapterContext()
        : MainWindow(NULL), Resolver(NULL), StagedViewers(10, 5),
          StagedAlternateViewers(10, 5), StagedEditors(10, 5)
    {
    }
};

RuntimeAdapterContext& AdapterContext()
{
    static RuntimeAdapterContext context;
    return context;
}

bool StageViewerAssociation(const Association& association, CViewerMasks* destination,
                            int viewerType)
{
    if (destination == NULL)
        return false;

    CViewerMasksItem* item = new CViewerMasksItem(association.Pattern.c_str(), "", "", "",
                                                   viewerType, FALSE);
    if (item == NULL || !item->IsGood())
    {
        delete item;
        return false;
    }
    if (!association.AssociationTarget.DisplayLabel.empty() &&
        !item->SetViewerLabel(association.AssociationTarget.DisplayLabel.c_str()))
    {
        delete item;
        return false;
    }

    destination->Add(item);
    if (!destination->IsGood())
    {
        delete item;
        return false;
    }
    return true;
}

bool GetArchiveExtension(const std::string& pattern, std::string* extension)
{
    if (extension == NULL || pattern.size() <= 2 || pattern.compare(0, 2, "*.") != 0)
        return false;

    const std::string value = pattern.substr(2);
    if (value.empty() || value.find_first_of("*?;|/") != std::string::npos)
        return false;

    *extension = value;
    return true;
}

int FindArchiveFormat(CPackerFormatConfig& config, const std::string& extension)
{
    for (int i = 0; i < config.GetFormatsCount(); ++i)
        if (_stricmp(config.GetExt(i), extension.c_str()) == 0)
            return i;
    return -1;
}

bool StageArchiveAssociation(const Association& association, RuntimeAdapterContext* context)
{
    std::string extension;
    if (context == NULL || !GetArchiveExtension(association.Pattern, &extension))
        return false;

    const int archiveIndex = context->Resolver.LastResolution.RuntimeArchiveIndex;
    int formatIndex = FindArchiveFormat(context->StagedPacker, extension);
    if (formatIndex < 0)
    {
        formatIndex = context->StagedPacker.AddFormat();
        if (formatIndex < 0)
            return false;

        if (association.Operation == PanelOperation::ListUnpack)
            return context->StagedPacker.SetFormat(formatIndex, extension.c_str(), FALSE, -1,
                                                   archiveIndex, FALSE) != FALSE;
        if (association.Operation == PanelOperation::DeletePack)
            return context->StagedPacker.SetFormat(formatIndex, extension.c_str(), TRUE,
                                                   archiveIndex, -1, FALSE) != FALSE;
        return false;
    }

    if (association.Operation == PanelOperation::ListUnpack)
    {
        context->StagedPacker.SetUnpackerIndex(formatIndex, archiveIndex);
        return true;
    }
    if (association.Operation == PanelOperation::DeletePack)
    {
        context->StagedPacker.SetUsePacker(formatIndex, TRUE);
        context->StagedPacker.SetPackerIndex(formatIndex, archiveIndex);
        return true;
    }
    return false;
}

bool BeginRuntimeProjection(void* opaque)
{
    RuntimeAdapterContext* context = static_cast<RuntimeAdapterContext*>(opaque);
    if (context == NULL || context->MainWindow == NULL || context->MainWindow->ViewerMasks == NULL ||
        context->MainWindow->AltViewerMasks == NULL || context->MainWindow->EditorMasks == NULL)
        return false;

    context->StagedViewers.DestroyMembers();
    context->StagedAlternateViewers.DestroyMembers();
    context->StagedEditors.DestroyMembers();
    context->StagedPacker.DeleteAllFormats();

    // Preserve complete legacy rows while the canonical record only carries
    // the target identity. User rows are layered onto these staged defaults.
    if (!context->StagedViewers.Load(*context->MainWindow->ViewerMasks) ||
        !context->StagedAlternateViewers.Load(*context->MainWindow->AltViewerMasks))
        return false;
    if (!context->StagedEditors.Load(*context->MainWindow->EditorMasks))
        return false;
    if (!context->StagedPacker.Load(PackerFormatConfig))
        return false;
    return true;
}

bool ApplyRuntimeProjection(const Association& association, void* opaque)
{
    RuntimeAdapterContext* context = static_cast<RuntimeAdapterContext*>(opaque);
    if (context == NULL)
        return false;

    if (association.AssociationProvenance == Provenance::Legacy)
        return true;

    switch (association.AssociationCategory)
    {
    case Category::View:
        return StageViewerAssociation(association, &context->StagedViewers,
                                       context->Resolver.LastResolution.RuntimeViewerType);
    case Category::AlternateView:
        return StageViewerAssociation(association, &context->StagedAlternateViewers,
                                       context->Resolver.LastResolution.RuntimeViewerType);
    case Category::PanelArchive:
        return StageArchiveAssociation(association, context);
    case Category::Edit:
    default:
        return false;
    }
}

void RollbackRuntimeProjection(void* opaque)
{
    RuntimeAdapterContext* context = static_cast<RuntimeAdapterContext*>(opaque);
    if (context == NULL)
        return;
    context->StagedViewers.DestroyMembers();
    context->StagedAlternateViewers.DestroyMembers();
    context->StagedEditors.DestroyMembers();
    context->StagedPacker.DeleteAllFormats();
}

bool CommitRuntimeProjection(void* opaque)
{
    RuntimeAdapterContext* context = static_cast<RuntimeAdapterContext*>(opaque);
    if (context == NULL || context->MainWindow == NULL || context->MainWindow->ViewerMasks == NULL ||
        context->MainWindow->AltViewerMasks == NULL || context->MainWindow->EditorMasks == NULL)
        return false;

    if (!context->StagedPacker.BuildArray())
        return false;

    context->MainWindow->ViewerMasks->Swap(context->StagedViewers);
    context->MainWindow->AltViewerMasks->Swap(context->StagedAlternateViewers);
    context->MainWindow->EditorMasks->Swap(context->StagedEditors);
    PackerFormatConfig.Swap(context->StagedPacker);
    return true;
}

} // namespace

bool ResolvePluginRuntimeAssociation(const Association& association, void* context)
{
    if (association.AssociationProvenance == Provenance::Legacy)
        return true;

    PluginRuntimeResolverContext* resolver =
        static_cast<PluginRuntimeResolverContext*>(context);
    if (resolver == NULL)
        return false;

    if (association.AssociationTarget.Kind == TargetKind::BuiltIn &&
        (association.AssociationCategory == Category::View ||
         association.AssociationCategory == Category::AlternateView) &&
        association.AssociationTarget.StableId == "internal")
    {
        resolver->LastResolution = PluginRuntimeResolution();
        resolver->LastResolution.RuntimeViewerType = VIEWER_INTERNAL;
        return true;
    }

    if (resolver->Plugins == NULL || association.AssociationTarget.Kind != TargetKind::Plugin ||
        association.AssociationTarget.StableId.empty())
        return false;

    const int pluginIndex = resolver->Plugins->FindByStableId(
        association.AssociationTarget.StableId.c_str());
    if (pluginIndex < 0)
        return false;

    CPluginData* plugin = resolver->Plugins->Get(pluginIndex);
    if (plugin == NULL)
        return false;

    resolver->LastResolution = PluginRuntimeResolution();
    resolver->LastResolution.PluginIndex = pluginIndex;

    switch (association.AssociationCategory)
    {
    case Category::View:
    case Category::AlternateView:
        if (!plugin->SupportViewer)
            return false;
        resolver->LastResolution.RuntimeViewerType = -pluginIndex - 1;
        return true;

    case Category::PanelArchive:
        if (association.Operation == PanelOperation::ListUnpack)
        {
            if (!plugin->SupportPanelView)
                return false;
        }
        else if (association.Operation == PanelOperation::DeletePack)
        {
            if (!plugin->SupportPanelEdit)
                return false;
        }
        else
            return false;

        resolver->LastResolution.RuntimeArchiveIndex = -pluginIndex - 1;
        return true;

    case Category::Edit:
    default:
        return false;
    }
}

void AttachPluginRuntimeResolver(RuntimeProjectionOperations* operations,
                                 PluginRuntimeResolverContext* context)
{
    if (operations == NULL)
        return;
    operations->Resolve = ResolvePluginRuntimeAssociation;
    operations->ResolveContext = context;
}

void AttachPluginRuntimeAdapter(RuntimeProjectionOperations* operations,
                                CMainWindow* mainWindow, CPlugins* plugins)
{
    if (operations == NULL)
        return;

    RuntimeAdapterContext& context = AdapterContext();
    context.MainWindow = mainWindow;
    context.Resolver.Plugins = plugins;
    context.Resolver.LastResolution = PluginRuntimeResolution();
    operations->Begin = BeginRuntimeProjection;
    operations->Resolve = ResolvePluginRuntimeAssociation;
    operations->Apply = ApplyRuntimeProjection;
    operations->Rollback = RollbackRuntimeProjection;
    operations->Commit = CommitRuntimeProjection;
    operations->Context = &context;
    operations->ResolveContext = &context.Resolver;
}

} // namespace FileAssociations
} // namespace Salamander
