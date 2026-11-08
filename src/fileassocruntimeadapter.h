// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Add the stable plugin runtime resolver. | Map canonical plugin targets to current runtime indexes only when plugin capabilities support the requested association. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-08 | Stage and commit canonical runtime associations. | Replace viewer and archive runtime rows only after every target resolves and archive lookup data rebuilds successfully. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |

#pragma once

#include "fileassocprojection.h"

class CPlugins;
class CMainWindow;

namespace Salamander
{
namespace FileAssociations
{

struct PluginRuntimeResolution
{
    int PluginIndex;
    int RuntimeViewerType;
    int RuntimeArchiveIndex;

    PluginRuntimeResolution()
        : PluginIndex(-1), RuntimeViewerType(0), RuntimeArchiveIndex(-1) {}
};

struct PluginRuntimeResolverContext
{
    CPlugins* Plugins;
    PluginRuntimeResolution LastResolution;

    PluginRuntimeResolverContext(CPlugins* plugins = NULL)
        : Plugins(plugins) {}
};

// Resolves only plugin targets. Built-in, preset, editor-command, and missing
// metadata targets remain blocked instead of being assigned guessed encodings.
bool ResolvePluginRuntimeAssociation(const Association& association, void* context);

// Adds the resolver to an existing transactional projection operation set. The
// caller still supplies Begin/Apply/Rollback/Commit for the legacy structures.
void AttachPluginRuntimeResolver(RuntimeProjectionOperations* operations,
                                 PluginRuntimeResolverContext* context);

// Installs the complete adapter. It stages new viewer/archive rows and leaves
// editor rows unchanged because canonical editor command metadata is unavailable.
void AttachPluginRuntimeAdapter(RuntimeProjectionOperations* operations,
                                CMainWindow* mainWindow, CPlugins* plugins);

} // namespace FileAssociations
} // namespace Salamander
