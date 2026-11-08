# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-08 | Check qualified lifecycle adapter calls. | Keep source-contract checks aligned with the namespace-qualified C++ integration points. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-08 | Check stable plugin runtime resolution. | Ensure canonical plugin targets use persisted identities and capability checks instead of session ordinals. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-08 | Check transactional runtime projection callbacks. | Keep incomplete target resolution from mutating the legacy runtime projection. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-07 | Check the runtime projection gate. | Keep target-only migration explicit until legacy command and session-index projection is implemented. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-07 | Check migration target identity and commit boundaries. | Prevent unassigned or partially imported legacy records from entering canonical state. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-07 | Check storage adapters and loss-aware migration. | Keep portable configuration storage explicit and prevent lossy legacy masks from entering the canonical model. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-07 | Check complete association record persistence. | Prevent numeric registry gaps from being accepted as a truncated canonical model. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-07 | Check lossless canonical persistence loading. | Prevent serialized conflicts and provenance from being rewritten during transactional load. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-07 | Verify the canonical file-association model contract. | Guard normalization, provenance layering, and blocking pattern conflicts before registry/UI integration. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "src" / "fileassoc.h"
IMPLEMENTATION = ROOT / "src" / "fileassoc.cpp"
PROJECT = ROOT / "src" / "vcxproj" / "salamand.vcxproj"
FILTERS = ROOT / "src" / "vcxproj" / "salamand.vcxproj.filters"
ARRAY = ROOT / "src" / "common" / "array.h"
PACK_HEADER = ROOT / "src" / "pack.h"
PACK_IMPLEMENTATION = ROOT / "src" / "pack3.cpp"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    header = HEADER.read_text(encoding="utf-8")
    implementation = IMPLEMENTATION.read_text(encoding="utf-8")
    migration_header = (ROOT / "src" / "fileassocmigration.h").read_text(encoding="utf-8")
    migration = (ROOT / "src" / "fileassocmigration.cpp").read_text(encoding="utf-8")
    projection_header = (ROOT / "src" / "fileassocprojection.h").read_text(encoding="utf-8")
    projection = (ROOT / "src" / "fileassocprojection.cpp").read_text(encoding="utf-8")
    runtime_adapter_header = (ROOT / "src" / "fileassocruntimeadapter.h").read_text(encoding="utf-8")
    runtime_adapter = (ROOT / "src" / "fileassocruntimeadapter.cpp").read_text(encoding="utf-8")
    plugins_header = (ROOT / "src" / "plugins.h").read_text(encoding="utf-8")
    plugins_implementation = (ROOT / "src" / "plugins2.cpp").read_text(encoding="utf-8")
    lifecycle_header = (ROOT / "src" / "fileassoclifecycle.h").read_text(encoding="utf-8")
    lifecycle = (ROOT / "src" / "fileassoclifecycle.cpp").read_text(encoding="utf-8")
    mainwnd = (ROOT / "src" / "mainwnd2.cpp").read_text(encoding="utf-8")
    array = ARRAY.read_text(encoding="utf-8")
    pack_header = PACK_HEADER.read_text(encoding="utf-8")
    pack_implementation = PACK_IMPLEMENTATION.read_text(encoding="utf-8")
    project = PROJECT.read_text(encoding="utf-8")
    filters = FILTERS.read_text(encoding="utf-8")

    for enum_name in ("Category", "PanelOperation", "Provenance", "TargetKind"):
        require(f"enum class {enum_name}" in header, f"missing canonical {enum_name} enum")
    for method in (
        "NormalizePattern",
        "PatternsOverlap",
        "AddLegacy",
        "SetUserTarget",
        "SetUnassigned",
        "ResetUserOverride",
        "Validate",
        "FindExactEffective",
        "Load",
        "Save",
    ):
        require(method in header and method in implementation, f"missing model method: {method}")

    require('value = "*.*"' in implementation, "universal pattern normalization is missing")
    require('value = "*." + value' in implementation, "bare extensions must normalize to patterns")
    require('value.find(\';\')' in implementation, "grouped masks must be rejected by the model")
    require("IsExactOverridePair" in implementation, "default/user layering must be explicit")
    require("const std::vector<Association> previous = Items" in implementation and
            "Items = previous" in implementation,
            "failed user edits must leave the previous model unchanged")
    require("MarkLegacyConflicts" in implementation, "legacy conflicts must remain visible")
    require("FileAssociationsCurrentSchemaVersion = 1" in implementation,
            "canonical persistence must have an explicit schema version")
    require('const char* const FileAssociationsCount = "Association Count"' in implementation and
            "DWORD associationCount = 0" in implementation and
            "for (DWORD itemIndex = 0; itemIndex < associationCount; ++itemIndex)" in implementation and
            'SetError(error, "file-association record is missing")' in implementation,
            "canonical persistence must reject missing numeric records")
    require('const char* const FileAssociationPattern = "Pattern"' in implementation,
            "association records must persist the normalized pattern")
    require("CFileAssociationModel loaded" in implementation and
            "Items.swap(loaded.Items)" in implementation,
            "loading must replace the model only after all records are parsed")
    require("loaded.Items.push_back(item)" in implementation and
            "item.SourceProvenance = static_cast<Provenance>(sourceProvenance)" in implementation,
            "loading must preserve serialized association fields")
    require("Persisted conflicts are diagnostic rows" in implementation and
            "first.AssociationProvenance == Provenance::Conflict" in implementation,
            "persisted legacy conflicts must remain visible during load")
    require("if (error != NULL)\n        error->clear();" in implementation,
            "successful persistence calls must not expose a stale previous error")
    require('SetError(error, "malformed file-association record")' in implementation,
            "malformed records must report an actionable load error")
    require("if (!Validate(error))" in implementation,
            "saving must reject unresolved conflicts")
    require("LoadFromConfigurationStorage" in header and
            "SaveToConfigurationStorage" in header and
            "storage.OpenConfigurationRootKey" in implementation and
            "association storage key is invalid" in implementation,
            "canonical persistence must expose the configuration-storage adapter")
    require("LegacyAssociationCandidate" in migration_header and
            "MigrateLegacyAssociations" in migration_header and
            "CFileAssociationModel staged = *model" in migration and
            "legacy association migration would lose information" in migration,
            "legacy migration must be loss-aware and transactional")
    require("strchr(candidate.Masks, ';')" in migration and
            "strchr(candidate.Masks, '|')" in migration and
            "PrepareMasks" in migration and
            "candidate.AssociationTarget" in migration and
            "target.Kind == TargetKind::Unassigned" in migration,
            "legacy migration must reject grouped, excluded, and malformed masks")
    require("if (!HasStableTargetIdentity(candidate.AssociationTarget)" in migration,
            "legacy migration must require a stable target identity")
    require(migration.index("*model = staged") >
            migration.index("if (localReport.BlockedCount != 0 || localReport.InvalidCount != 0)"),
            "legacy migration must commit only after all candidates pass")
    require("bool RuntimeProjectionBlocked" in migration_header and
            "RuntimeProjectionBlocked(false)" in migration_header and
            "localReport.RuntimeProjectionBlocked = candidateCount != 0" in migration and
            "report->RuntimeProjectionBlocked = true" in migration,
            "legacy migration must expose the guarded runtime projection state")
    require("RuntimeProjectionOperations" in projection_header and
            "RuntimeProjectionResolve" in projection_header and
            "RuntimeProjectionBegin" in projection_header and
            "RuntimeProjectionApply" in projection_header and
            "RuntimeProjectionRollback" in projection_header and
            "RuntimeProjectionCommit" in projection_header and
            "ProjectRuntimeAssociations" in projection_header,
            "runtime projection must require explicit transactional operations")
    require("operations.Begin == NULL" in projection and
            "operations.Resolve == NULL" in projection and
            "operations.Apply == NULL" in projection and
            "operations.Rollback == NULL" in projection and
            "operations.Commit == NULL" in projection and
            "operations.Rollback(operations.Context)" in projection and
            "operations.Commit(operations.Context)" in projection,
            "runtime projection must block unavailable or failed operations transactionally")
    require("RegKeyName" in plugins_header and
            "GetStableId()" in plugins_header and
            "FindByStableId(const char* stableId) const" in plugins_header and
            "_stricmp(mutableThis->Data[i]->GetStableId(), stableId)" in plugins_implementation,
            "plugins must expose a stable identity lookup independent of session order")
    require("ResolvePluginRuntimeAssociation" in runtime_adapter_header and
            "AttachPluginRuntimeResolver" in runtime_adapter_header and
            "TargetKind::Plugin" in runtime_adapter and
            "SupportViewer" in runtime_adapter and
            "SupportPanelView" in runtime_adapter and
            "SupportPanelEdit" in runtime_adapter and
            "RuntimeViewerType = -pluginIndex - 1" in runtime_adapter and
            "RuntimeArchiveIndex = -pluginIndex - 1" in runtime_adapter,
             "plugin runtime adapter must resolve stable targets and capabilities")
    require("StagedViewers" in runtime_adapter and
            "StagedAlternateViewers" in runtime_adapter and
            "StagedEditors" in runtime_adapter and
            "StagedPacker" in runtime_adapter and
            "BeginRuntimeProjection" in runtime_adapter and
            "ApplyRuntimeProjection" in runtime_adapter and
            "RollbackRuntimeProjection" in runtime_adapter and
            "CommitRuntimeProjection" in runtime_adapter and
            "StagedPacker.BuildArray()" in runtime_adapter and
            "ViewerMasks->Swap" in runtime_adapter and
            "AltViewerMasks->Swap" in runtime_adapter and
            "EditorMasks->Swap" in runtime_adapter and
            "PackerFormatConfig.Swap" in runtime_adapter,
            "runtime adapter must stage and atomically commit legacy containers")
    require("void Swap(TDirectArray<DATA_TYPE>& source)" in array and
            "void Swap(CArray& source)" in array and
            "CDeleteType deleteType = DeleteType" in array,
            "array ownership swaps must include allocation state and delete policy")
    require("void Swap(CPackerFormatConfig& source)" in pack_header and
            "Formats.Swap(source.Formats)" in pack_implementation and
            "Extensions[i].Swap(source.Extensions[i])" in pack_implementation,
            "packer configuration must swap owning and derived arrays safely")
    require("runtime association target or metadata could not be resolved" in projection,
            "runtime projection must report unresolved target metadata")
    require("RegisterCanonicalRuntimeProjection" in lifecycle_header and
            "ClearCanonicalRuntimeProjection" in lifecycle_header and
            "LoadCanonicalAssociationsForLifecycle" in lifecycle_header and
            "SaveCanonicalAssociationsForLifecycle" in lifecycle_header,
            "canonical lifecycle must expose explicit projection registration and hooks")
    require("CFileAssociationModel loaded" in lifecycle and
            "LoadFromConfigurationStorage" in lifecycle and
            "ProjectRuntimeAssociations" in lifecycle and
            "runtime association projection adapter is not registered" in lifecycle,
            "canonical lifecycle must load temporary state and guard unavailable projection")
    require("if (!state.Loaded || state.Model.Associations().empty()" in lifecycle and
            "!state.ProjectionSucceeded" in lifecycle and
            "return TRUE;" in lifecycle,
            "blocked or empty canonical state must not create or overwrite persisted state")
    require("Salamander::FileAssociations::SaveCanonicalAssociationsForLifecycle" in mainwnd and
            "Salamander::FileAssociations::LoadCanonicalAssociationsForLifecycle" in mainwnd,
            "main lifecycle must invoke guarded canonical load and save hooks")
    require("Salamander::FileAssociations::AttachPluginRuntimeAdapter(&runtimeProjection, this, &Plugins)" in mainwnd and
            "Salamander::FileAssociations::RegisterCanonicalRuntimeProjection(runtimeProjection)" in mainwnd and
            mainwnd.index("Plugins.CheckData();") <
                mainwnd.index("Salamander::FileAssociations::AttachPluginRuntimeAdapter(&runtimeProjection, this, &Plugins)") <
                mainwnd.index("Salamander::FileAssociations::LoadCanonicalAssociationsForLifecycle"),
            "canonical projection must be registered after archive setup and before loading")
    require('Include="..\\fileassocmigration.cpp"' in project and
            'Include="..\\fileassocmigration.h"' in project and
            'Include="..\\fileassocmigration.cpp"' in filters and
            'Include="..\\fileassocmigration.h"' in filters,
            "legacy migration files must be compiled and visible in the project")
    require('Include="..\\fileassocprojection.cpp"' in project and
            'Include="..\\fileassocprojection.h"' in project and
            'Include="..\\fileassocprojection.cpp"' in filters and
            'Include="..\\fileassocprojection.h"' in filters,
            "runtime projection files must be compiled and visible in the project")
    require('Include="..\\fileassocruntimeadapter.cpp"' in project and
            'Include="..\\fileassocruntimeadapter.h"' in project and
            'Include="..\\fileassocruntimeadapter.cpp"' in filters and
            'Include="..\\fileassocruntimeadapter.h"' in filters,
            "plugin runtime adapter files must be compiled and visible in the project")
    require('Include="..\\fileassoclifecycle.cpp"' in project and
            'Include="..\\fileassoclifecycle.h"' in project and
            'Include="..\\fileassoclifecycle.cpp"' in filters and
            'Include="..\\fileassoclifecycle.h"' in filters,
            "canonical lifecycle files must be compiled and visible in the project")
    require("overlapping patterns in one association category are not allowed" in implementation,
            "overlapping patterns must be blocking conflicts")
    require("Provenance::Disabled" in implementation, "explicit Unassigned state is missing")
    require("TargetKind::Unassigned" in implementation, "typed unassigned target is missing")
    require('..\\fileassoc.cpp' in project, "canonical model must be compiled by salamand.vcxproj")
    require('..\\fileassoc.h' in project, "canonical model header must be in salamand.vcxproj")
    require('..\\fileassoc.cpp' in filters and '..\\fileassoc.h' in filters,
            "canonical model files must be visible in the Visual Studio filters")


if __name__ == "__main__":
    main()
