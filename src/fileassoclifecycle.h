// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Expose transactional canonical model editing. | Allow the configuration page to replace validated associations without mutating legacy runtime state on failure. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-08 | Add guarded canonical association lifecycle hooks. | Keep legacy runtime configuration active until a complete projection adapter is registered. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |

#pragma once

#include "fileassocprojection.h"

class CConfigurationStorage;

namespace Salamander
{
namespace FileAssociations
{

void RegisterCanonicalRuntimeProjection(const RuntimeProjectionOperations& operations);
void ClearCanonicalRuntimeProjection();

// A malformed canonical root is reported as a load failure. A missing or
// unprojectable root is retained internally but does not replace legacy runtime
// structures or fail the surrounding configuration load.
BOOL LoadCanonicalAssociationsForLifecycle(CConfigurationStorage& storage,
                                           std::string* error = NULL);

// Saving is deliberately a no-op until the canonical model has been loaded
// and projected successfully. This prevents an empty or blocked state from
// creating or overwriting the canonical root.
BOOL SaveCanonicalAssociationsForLifecycle(CConfigurationStorage& storage,
                                           std::string* error = NULL);

BOOL CopyCanonicalAssociationsForConfiguration(CFileAssociationModel* model,
                                                std::string* error = NULL);
BOOL ReplaceCanonicalAssociationsForConfiguration(const CFileAssociationModel& model,
                                                   std::string* error = NULL);

bool IsCanonicalRuntimeLoaded();
bool IsCanonicalRuntimeProjectionBlocked();
const RuntimeProjectionReport& GetCanonicalRuntimeProjectionReport();

} // namespace FileAssociations
} // namespace Salamander
