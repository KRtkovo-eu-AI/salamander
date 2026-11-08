// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Add guarded canonical association lifecycle hooks. | Keep legacy runtime configuration active until a complete projection adapter is registered. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |

#include "precomp.h"
#include "fileassoclifecycle.h"
#include "configstorage.h"

namespace Salamander
{
namespace FileAssociations
{

namespace
{

const char* const CanonicalAssociationKey = "File Associations";

struct LifecycleState
{
    CFileAssociationModel Model;
    RuntimeProjectionReport ProjectionReport;
    bool Loaded;
    bool ProjectionSucceeded;
    bool ProjectionBlocked;

    LifecycleState()
        : Loaded(false), ProjectionSucceeded(false), ProjectionBlocked(false) {}
};

LifecycleState& State()
{
    static LifecycleState state;
    return state;
}

RuntimeProjectionOperations& RegisteredOperations()
{
    static RuntimeProjectionOperations operations;
    return operations;
}

bool& HasRegisteredOperations()
{
    static bool registered = false;
    return registered;
}

void SetError(std::string* error, const char* message)
{
    if (error != NULL && error->empty())
        *error = message;
}

void MarkProjectionBlocked(size_t associationCount, const char* message,
                           std::string* error)
{
    LifecycleState& state = State();
    state.ProjectionSucceeded = false;
    state.ProjectionBlocked = true;
    state.ProjectionReport.CommittedCount = 0;
    state.ProjectionReport.BlockedCount = associationCount;
    state.ProjectionReport.RuntimeProjectionBlocked = true;
    SetError(error, message);
}

} // namespace

void RegisterCanonicalRuntimeProjection(const RuntimeProjectionOperations& operations)
{
    RegisteredOperations() = operations;
    HasRegisteredOperations() = true;
}

void ClearCanonicalRuntimeProjection()
{
    RegisteredOperations() = RuntimeProjectionOperations();
    HasRegisteredOperations() = false;
}

BOOL LoadCanonicalAssociationsForLifecycle(CConfigurationStorage& storage,
                                           std::string* error)
{
    if (error != NULL)
        error->clear();

    CFileAssociationModel loaded;
    if (!loaded.LoadFromConfigurationStorage(storage, CanonicalAssociationKey, error))
        return FALSE;

    LifecycleState& state = State();
    state.Model = loaded;
    state.Loaded = true;
    state.ProjectionReport = RuntimeProjectionReport();
    state.ProjectionSucceeded = loaded.Associations().empty();
    state.ProjectionBlocked = false;

    if (loaded.Associations().empty())
        return TRUE;

    if (!HasRegisteredOperations())
    {
        MarkProjectionBlocked(loaded.Associations().size(),
                              "runtime association projection adapter is not registered", error);
        return TRUE;
    }

    if (!ProjectRuntimeAssociations(loaded, RegisteredOperations(),
                                    &state.ProjectionReport, error))
    {
        state.ProjectionBlocked = true;
        state.ProjectionSucceeded = false;
        return TRUE;
    }

    state.ProjectionSucceeded = true;
    state.ProjectionBlocked = false;
    return TRUE;
}

BOOL SaveCanonicalAssociationsForLifecycle(CConfigurationStorage& storage,
                                           std::string* error)
{
    if (error != NULL)
        error->clear();

    LifecycleState& state = State();
    if (!state.Loaded || state.Model.Associations().empty() ||
        !HasRegisteredOperations() || !state.ProjectionSucceeded ||
        state.ProjectionBlocked)
        return TRUE;

    return state.Model.SaveToConfigurationStorage(storage, CanonicalAssociationKey, error);
}

bool IsCanonicalRuntimeLoaded()
{
    return State().Loaded;
}

bool IsCanonicalRuntimeProjectionBlocked()
{
    return State().ProjectionBlocked;
}

const RuntimeProjectionReport& GetCanonicalRuntimeProjectionReport()
{
    return State().ProjectionReport;
}

} // namespace FileAssociations
} // namespace Salamander
