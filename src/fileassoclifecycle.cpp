// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Save valid association edits when runtime projection is blocked. | Configuration changes must be retained while plug-in handlers are temporarily unavailable. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 (Codex) |
// | 2026-11-09 | Defer default association import until Configuration opens. | Prevent early startup from persisting an incomplete clean-configuration snapshot before plugin defaults finish loading. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-09 | Materialize migrated associations without restarting. | Let the configuration page display clean/default and migrated rows immediately after lifecycle load. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-08 | Import legacy associations on first canonical load. | Show existing viewer, editor, and archive defaults in the new matrix without forcing manual pattern entry. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-08 | Expose transactional canonical model editing. | Project a candidate before publishing it to lifecycle state so the new configuration page is atomic. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-08 | Add guarded canonical association lifecycle hooks. | Keep legacy runtime configuration active until a complete projection adapter is registered. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |

#include "precomp.h"
#include "fileassoclifecycle.h"
#include "fileassocmigration.h"
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
    bool Materialized;
    bool ProjectionSucceeded;
    bool ProjectionBlocked;

    LifecycleState()
        : Loaded(false), Materialized(false), ProjectionSucceeded(false), ProjectionBlocked(false) {}
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

BOOL ProjectLoadedState(LifecycleState& state, std::string* error)
{
    state.ProjectionReport = RuntimeProjectionReport();
    state.ProjectionSucceeded = state.Model.Associations().empty();
    state.ProjectionBlocked = false;

    if (state.Model.Associations().empty())
        return TRUE;

    if (!HasRegisteredOperations())
    {
        MarkProjectionBlocked(state.Model.Associations().size(),
                              "runtime association projection adapter is not registered", error);
        return TRUE;
    }

    if (!ProjectRuntimeAssociations(state.Model, RegisteredOperations(),
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

BOOL MaterializeLegacyDefaults(LifecycleState& state, std::string* error)
{
    if (!state.Model.Associations().empty())
        return TRUE;

    CFileAssociationModel legacy;
    LegacyMigrationReport migrationReport;
    std::string migrationError;
    const BOOL migrationSucceeded = BuildLegacyAssociationDefaults(
        &legacy, &migrationReport, &migrationError) ? TRUE : FALSE;
    if (!legacy.Associations().empty())
        state.Model = legacy;

    // A partial result is still useful to the editor: unsupported grouped
    // masks must not hide the representable rows from the user.
    if (!migrationSucceeded && legacy.Associations().empty())
    {
        SetError(error, migrationError.empty()
                           ? "legacy association migration produced no rows"
                           : migrationError.c_str());
        // Missing or wholly unrepresentable legacy defaults must not abort the
        // surrounding configuration load. The editor can still open empty and
        // show the migration error through its status path when requested.
        return TRUE;
    }
    return TRUE;
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
    state.Materialized = !state.Model.Associations().empty();

    if (!ProjectLoadedState(state, error))
        return FALSE;
    return TRUE;
}

BOOL SaveCanonicalAssociationsForLifecycle(CConfigurationStorage& storage,
                                           std::string* error)
{
    if (error != NULL)
        error->clear();

    LifecycleState& state = State();
    if (!state.Loaded || !state.Materialized)
        return TRUE;

    return state.Model.SaveToConfigurationStorage(storage, CanonicalAssociationKey, error);
}

BOOL CopyCanonicalAssociationsForConfiguration(CFileAssociationModel* model,
                                               std::string* error)
{
    if (error != NULL)
        error->clear();
    if (model == NULL)
    {
        SetError(error, "canonical association model output is required");
        return FALSE;
    }
    if (!State().Loaded)
    {
        SetError(error, "canonical association model is not loaded");
        return FALSE;
    }
    *model = State().Model;
    return TRUE;
}

BOOL EnsureCanonicalAssociationsMaterializedForConfiguration(std::string* error)
{
    if (error != NULL)
        error->clear();

    LifecycleState& state = State();
    if (!state.Loaded)
    {
        SetError(error, "canonical association model is not loaded");
        return FALSE;
    }
    if (!MaterializeLegacyDefaults(state, error))
        return FALSE;
    state.Materialized = true;
    return ProjectLoadedState(state, error);
}

BOOL ReplaceCanonicalAssociationsForConfiguration(const CFileAssociationModel& model,
                                                  std::string* error)
{
    if (error != NULL)
        error->clear();

    LifecycleState& state = State();
    if (!state.Loaded)
    {
        SetError(error, "canonical association model is not loaded");
        return FALSE;
    }

    CFileAssociationModel candidate;
    if (!candidate.ReplaceAssociations(model.Associations(), error))
        return FALSE;

    state.Model = candidate;
    state.Materialized = true;
    // Keep the canonical user configuration even when a runtime handler cannot
    // currently accept its projection. ProjectLoadedState records that condition
    // and the lifecycle can retry once its runtime adapter is available.
    return ProjectLoadedState(state, error);
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
