# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-10 | Guard association row badge provenance precedence. | Keep resolved user overrides visible in Status and Source without hiding remaining conflicts. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 |
# | 2026-11-10 | Regress in-place association pattern normalization. | Catch the input/output alias that made Apply's full-model replacement clear each row's pattern before validating it. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 |
# | 2026-11-10 | Guard pattern-key separation during Apply. | Catch regressions where reading action fields overwrites the selected matrix pattern with stale edit text. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 |
# | 2026-11-10 | Check association Apply input and persistence flow. | Guard the selected pattern fallback, user override commit, storage flush, and table reload against regressions. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 |
# | 2026-11-10 | Align lifecycle contract with materialized-state persistence. | Check the current storage guard instead of the removed projection-success flag. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 |
# | 2026-11-09 | Check association matrix selection and dark badge colors. | Keep neutral data cells, full-row selection, and readable status badges across themes. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 (Codex) |
# | 2026-11-09 | Check first-start plugin rows and extension owner labels. | Prevent startup ordering, migration, projection, and localized owner-display regressions for issue #733. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-09 | Check immediate clean/default materialization. | Keep migrated association rows visible without requiring a restart after lifecycle load or page refresh. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-09 | Check the property-holder minimum and list-view color contract. | Keep the canonical page below the parent buttons and make provenance colors effective in dark mode. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-09 | Check narrow association layouts and loss-aware mask groups. | Prevent localized action buttons from escaping the page and prevent legacy `|` exclusions from becoming fake extensions. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-08 | Check responsive matrix and Internal Viewer registration. | Prevent regressions in headers, scrolling, page navigation and access to the existing viewer settings. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-08 | Check matrix filters and localized target states. | Keep the redesigned association page usable for archive-only rows, disabled overrides, and missing targets. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-08 | Check resource-table separators. | Prevent rc.exe crashes caused by trailing commas after STRINGTABLE values. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-08 | Check canonical editor provenance and multi-row editing. | Prevent the new page from hiding row origin metadata or making Add / Update unable to create additional entries. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
# | 2026-11-08 | Check the canonical association configuration page. | Prevent the implementation from stopping at backend projection while the legacy pages remain the only UI. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
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
LIFECYCLE_HEADER = ROOT / "src" / "fileassoclifecycle.h"
LIFECYCLE_IMPLEMENTATION = ROOT / "src" / "fileassoclifecycle.cpp"
CFG_HEADER = ROOT / "src" / "cfgdlg.h"
DIALOGS4 = ROOT / "src" / "dialogs4.cpp"
DIALOGS5 = ROOT / "src" / "dialogs5.cpp"
LANG_HEADER = ROOT / "src" / "lang" / "lang.rh"
LANG_RESOURCE = ROOT / "src" / "lang" / "lang.rc"
TEXTS_HEADER = ROOT / "src" / "texts.rh2"
TEXTS_RESOURCE = ROOT / "src" / "lang" / "texts.rc2"
SHEETS = ROOT / "src" / "common" / "sheets.cpp"
SALAMDR2 = ROOT / "src" / "salamdr2.cpp"
SALAMATRIX = ROOT / "src" / "plugins" / "salamatrix" / "salamatrix.cpp"


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
    lifecycle_header = LIFECYCLE_HEADER.read_text(encoding="utf-8")
    lifecycle_implementation = LIFECYCLE_IMPLEMENTATION.read_text(encoding="utf-8")
    cfg_header = CFG_HEADER.read_text(encoding="utf-8")
    dialogs4 = DIALOGS4.read_text(encoding="utf-8")
    dialogs5 = DIALOGS5.read_text(encoding="utf-8")
    lang_header = LANG_HEADER.read_text(encoding="utf-8")
    lang_resource = LANG_RESOURCE.read_text(encoding="utf-8")
    texts_header = TEXTS_HEADER.read_text(encoding="utf-8")
    texts_resource = TEXTS_RESOURCE.read_text(encoding="utf-8")
    sheets = SHEETS.read_text(encoding="utf-8")
    salamdr2 = SALAMDR2.read_text(encoding="utf-8")
    salamatrix = SALAMATRIX.read_text(encoding="utf-8")

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
    normalize_start = implementation.index("bool CFileAssociationModel::NormalizePattern(")
    normalize_end = implementation.index("bool CFileAssociationModel::PatternsOverlap(", normalize_start)
    normalize_pattern = implementation[normalize_start:normalize_end]
    replace_start = implementation.index("bool CFileAssociationModel::ReplaceAssociations(")
    replace_end = implementation.index("bool CFileAssociationModel::Validate(", replace_start)
    replace_associations = implementation[replace_start:replace_end]
    require("NormalizePattern(item.Pattern.c_str(), &item.Pattern, error)" in replace_associations and
            "const std::string sourcePattern = pattern != NULL ? pattern : \"\";" in normalize_pattern and
            normalize_pattern.index("sourcePattern") < normalize_pattern.index("normalized->clear()") and
            "TrimPattern(sourcePattern.c_str())" in normalize_pattern,
            "in-place ReplaceAssociations normalization must preserve the source pattern before clearing output")
    status_start = dialogs5.index("std::string CCfgPageFileAssociations::MatrixStatusText(")
    status_end = dialogs5.index("int CCfgPageFileAssociations::MatrixSourceKind(", status_start)
    matrix_status = dialogs5[status_start:status_end]
    source_start = status_end
    source_end = dialogs5.index("std::string CCfgPageFileAssociations::MatrixSourceText(", source_start)
    matrix_source = dialogs5[source_start:source_end]
    require(matrix_status.index("if (row.Conflict)") <
            matrix_status.index("Provenance::User") <
            matrix_status.index("if (statusIndex < 0)"),
            "matrix status must keep unresolved conflicts first and prefer effective user overrides otherwise")
    require(matrix_source.index("SourceProvenance == Salamander::FileAssociations::Provenance::User") <
            matrix_source.index("for (int i = 0; i < 5; ++i)",
                                matrix_source.index("return 0;")),
            "matrix source must prefer a user override over earlier plugin/default columns")
    require('value = "*." + value' in implementation, "bare extensions must normalize to patterns")
    require("value.find(';')" in implementation and "value.find('|')" in implementation,
            "grouped masks must be rejected by the model")
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
    require("DisplayLabel" in header and
            'FileAssociationTargetLabel = "Target Label"' in implementation and
            "targetLabel.data()" in implementation and
            "AssociationTarget.DisplayLabel.c_str()" in implementation,
            "canonical targets must persist optional extension owner labels")
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
    require("if (!state.Loaded || !state.Materialized)" in lifecycle and
            "return TRUE;" in lifecycle,
            "unmaterialized canonical state must not create or overwrite persisted state")
    require("ReplaceAssociations" in header and
            "replacement.reserve(associations.size())" in implementation and
            "Items.swap(candidate.Items)" in implementation,
            "the model must expose atomic replacement for the configuration editor")
    require("CopyCanonicalAssociationsForConfiguration" in lifecycle_header and
            "ReplaceCanonicalAssociationsForConfiguration" in lifecycle_header and
            "EnsureCanonicalAssociationsMaterializedForConfiguration" in lifecycle_header and
            "candidate.ReplaceAssociations(model.Associations(), error)" in lifecycle_implementation and
            "MaterializeLegacyDefaults" in lifecycle_implementation and
            "state.Model.SaveToConfigurationStorage(storage, CanonicalAssociationKey, error)" in lifecycle_implementation and
            "ProjectRuntimeAssociations(state.Model, RegisteredOperations()" in lifecycle_implementation and
            "return ProjectLoadedState(state, error);" in lifecycle_implementation,
            "lifecycle must validate and project UI edits before publishing them")
    require("class CCfgPageFileAssociations" in cfg_header and
            "std::vector<Salamander::FileAssociations::Association> Rows" in cfg_header and
            "CopyCanonicalAssociationsForConfiguration" in dialogs5 and
            "ReplaceCanonicalAssociationsForConfiguration" in dialogs5 and
            "BuildCandidate" in dialogs5 and
            "ProvenanceText" in dialogs5 and
            "Rows.push_back(item)" in dialogs5 and
            "IDL_FILEASSOCIATIONS" in dialogs5,
            "the canonical association editor must load, edit, and commit model rows")
    apply_start = dialogs5.index("case IDB_FILEASSOCIATION_APPLY:")
    apply_end = dialogs5.index("case IDB_FILEASSOCIATION_DELETE:", apply_start)
    apply_handler = dialogs5[apply_start:apply_end]
    require("item.Pattern = focusedPattern;" in apply_handler and
            apply_handler.index("item.Pattern = focusedPattern;") <
                apply_handler.index("ReadControls(item, &error, &focusedPattern, FALSE)"),
            "Apply must seed the association pattern before reading action controls")
    require("if (!readPatternControl)" in dialogs5 and
            "item.Pattern = *fallbackPattern;" in dialogs5 and
            "ReadControls(item, &error, &focusedPattern, FALSE)" in apply_handler,
            "Apply must not reread the pattern edit field after resolving the selected matrix key")
    read_controls = apply_handler.index("ReadControls(item, &error, &focusedPattern, FALSE)")
    reassert_pattern = apply_handler.index("item.Pattern = focusedPattern;", read_controls)
    normalize_pattern = apply_handler.index("NormalizePattern(\n                    item.Pattern.c_str()", read_controls)
    require(read_controls < reassert_pattern < normalize_pattern,
            "Apply must restore its resolved selected pattern after reading action controls")
    require("appendPattern(MatrixRows[selectedItem.lParam].Pattern)" in apply_handler and
            "ListView_GetNextItem(matrix, -1, LVNI_FOCUSED)" in apply_handler and
            "GetDlgItemText(HWindow, IDE_FILEASSOCIATION_PATTERN" in apply_handler,
            "Apply must recover its pattern key from selection, focus, or the Pattern field")
    require("Provenance::User" in apply_handler and
            "candidate.ReplaceAssociations(Rows, &error)" in apply_handler and
            "ReplaceCanonicalAssociationsForConfiguration" in apply_handler and
            "SaveCanonicalAssociationsForLifecycle" in apply_handler and
            "ConfigurationStorage.Flush(FALSE)" in apply_handler and
            "CopyCanonicalAssociationsForConfiguration" in apply_handler and
            "Rows = savedModel.Associations();" in apply_handler,
            "Apply must persist a user override and reload the table from canonical state")
    apply_stages = (
        "candidate.ReplaceAssociations(Rows, &error)",
        "ReplaceCanonicalAssociationsForConfiguration(\n                    candidate",
        "SaveCanonicalAssociationsForLifecycle(\n                    ConfigurationStorage",
        "ConfigurationStorage.Flush(FALSE)",
        "CopyCanonicalAssociationsForConfiguration(\n                    &savedModel",
        "Rows = savedModel.Associations();",
    )
    apply_stage_positions = [apply_handler.index(stage) for stage in apply_stages]
    apply_stage_positions.append(apply_handler.index(
        "RefreshList();", apply_handler.index("Rows = savedModel.Associations();")))
    require(apply_stage_positions == sorted(apply_stage_positions),
            "Apply must validate, commit, persist, flush, reload, and refresh in order")
    require("EnsureCanonicalAssociationsMaterializedForConfiguration(&error)" in dialogs5 and
            dialogs5.index("EnsureCanonicalAssociationsMaterializedForConfiguration(&error)") <
                dialogs5.index("CopyCanonicalAssociationsForConfiguration(&model, &error)"),
            "the association page must materialize clean/default rows before reading lifecycle state")
    require("LegacyRuntimeTarget(item->ViewerType, false," in migration and
            "item->ViewerLabel" in migration and
            "SetViewerLabel(association.AssociationTarget.DisplayLabel.c_str())" in runtime_adapter,
            "viewer labels must survive migration and runtime projection")
    require("RegisteredDuringStartup" in cfg_header and
            "RegisteredDuringStartup = FALSE" in (ROOT / "src" / "dialogs2.cpp").read_text(encoding="utf-8") and
            "RegisteredDuringStartup = item.RegisteredDuringStartup" in (ROOT / "src" / "dialogs2.cpp").read_text(encoding="utf-8") and
            "if (force)\n                    item->RegisteredDuringStartup = TRUE;" in (ROOT / "src" / "plugins1.cpp").read_text(encoding="utf-8") and
            "!preloaded->RegisteredDuringStartup" in salamdr2,
            "viewer startup merging must preserve only first-registration rows")
    require("FindViewerMask" in salamdr2 and
            "MergePreloadedPluginViewerRows" in salamdr2 and
            "preloadedViewers.Load(*viewerMasks)" in salamdr2 and
            "viewerMasks->DestroyMembers()" in salamdr2 and
            "MergePreloadedPluginViewerRows(viewerMasks, &preloadedViewers)" in salamdr2,
            "persisted viewer loading must preserve plugin rows registered before configuration load")
    require("SalamatrixPackages->SetRefreshDeferred(FALSE);" in salamatrix and
            salamatrix.index("SalamatrixPackages->Refresh();") <
                salamatrix.index("SalamatrixPackages->RegisterViewerMasks(salamander);"),
            "Salamatrix package discovery must complete before viewer-mask registration")
    require("BuildLegacyAssociationDefaults" in migration_header and
             "viewerTables[2]" in migration and
             "PackerFormatConfig.GetFormatsCount" in migration and
             "one.find('|')" in migration and
             "report->BlockedCount" in migration,
             "the canonical editor must collect existing viewer, editor, and archive defaults")
    require("MatrixRow" in cfg_header and
            "FilterCategory" in cfg_header and
            "FilterProvenance" in cfg_header and
            "ConflictsOnly" in cfg_header and
            "IDC_FILEASSOCIATION_TARGETCATALOG" in dialogs5 and
            "void CCfgPageFileAssociations::PaintMatrix(HDC hdc)" in dialogs5 and
            "RoundRect(buffer, pill.left" in dialogs5 and
            "SupportPanelView" in dialogs5 and
            "SupportPanelEdit" in dialogs5,
            "the redesigned editor must expose matrix filtering, badges, and compatible target selection")
    require('"SysListView32"' in lang_resource and
            "LVS_REPORT" in lang_resource and
            "WS_HSCROLL" in lang_resource and
            "LVS_OWNERDRAWFIXED" not in lang_resource[lang_resource.index("IDD_CFGPAGE_FILEASSOCIATIONS"):lang_resource.index("IDD_CFGPAGE_VIEWERS")],
            "the association matrix must use a report list view with headers and scrolling")
    require("ListView_InsertColumn" in dialogs5 and
            "ListView_SetItemText" in dialogs5 and
            "LVN_ITEMCHANGED" in dialogs5 and
            "PaintMatrix(hdc)" in dialogs5 and
            "WinLibDPIToLogical(HWindow" in dialogs5 and
             "WinLibDPIFromLogical(HWindow" in dialogs5,
             "the association matrix must support selectable rows, colored cells, and DPI-aware layout")
    require("SetDlgItemText(HWindow, IDC_FILEASSOCIATION_HELP, LoadStr(IDS_BUTTON_HELP))" in dialogs5 and
            "std::string MatrixCellText" in cfg_header and
            "const int buttonIds[4]" in dialogs5 and
            "const ControlLayout controls[]" in dialogs5 and
            "GetClientRect(HWindow" in dialogs5 and
            "RoundRect(buffer, pill.left" in dialogs5 and
            "const COLORREF rowColor = selected" in dialogs5 and
            "SetTextColor(buffer, selected ? GetSysColor(COLOR_HIGHLIGHTTEXT)" in dialogs5,
            "the association page must keep compact help, complete display strings, and every action row inside the client area")
    require("#define _TPD_MIN_PAGE_H 300" in sheets and
            "Reserve the full canonical association page height" in sheets,
            "the property holder must keep the canonical page above its parent action buttons")
    association_validate = dialogs5[dialogs5.index("void CCfgPageFileAssociations::Validate"):dialogs5.index("INT_PTR CCfgPageFileAssociations::DialogProc")]
    require("BuildCandidate" not in association_validate and
            "NormalizePattern" in association_validate and
            "ReadControls" in association_validate,
            "page navigation validation must not reject unrelated whole-model conflicts")
    require("previousStableId" in dialogs5 and
            "previousTargetKind" in dialogs5,
            "unavailable plugin targets must not make page navigation impossible")
    require("DisplayLabel" in dialogs5 and
            "IsExtensionViewerLabel(item.AssociationTarget.DisplayLabel)" in dialogs5 and
            "LoadStr(IDS_FILEASSOCIATION_EXTENSION)" in dialogs5 and
            "IDS_FILEASSOCIATION_EXTENSION" in dialogs5 and
            "item.AssociationTarget.DisplayLabel.clear()" in dialogs5,
            "extension owner labels must be localized and cleared when the target changes")
    require("IDD_CFGPAGE_FILEASSOCIATIONS" in lang_header and
            "IDL_FILEASSOCIATIONS" in lang_resource and
            "IDE_FILEASSOCIATION_PATTERN" in lang_resource and
            "IDC_FILEASSOCIATION_TARGETCATALOG" in lang_resource and
            "IDS_FILEASSOCIATIONS" in texts_header and
            "IDS_FILEASSOCIATION_STATUS_CONFLICT" in texts_resource,
            "the canonical association editor must have resource-backed controls and text")
    require("IDS_FILEASSOCIATION_INTERNAL_VIEWER" in texts_header and
            "IDS_FILEASSOCIATION_LEGACY_RUNTIME" in texts_header and
            "IDS_FILEASSOCIATION_EXTENSION" in texts_header and
            'IDS_FILEASSOCIATION_EXTENSION, "Extension"' in texts_resource and
            "IDS_FILEASSOCIATION_NO_TARGETS" in texts_resource and
            "IDS_FILEASSOCIATION_PATTERN_EMPTY" in texts_resource,
            "new association UI states must use localized resource strings")
    require("IDD_CFGPAGE_FILEASSOCIATIONS DIALOGEX 50, 10, 420, 300" in lang_resource and
            "IDC_FILEASSOCIATION_TARGETCATALOG" in lang_resource and
            "IDC_FILEASSOCIATION_FILTER_PROVENANCE" in lang_resource,
            "the association page resource must contain the redesigned matrix controls")
    association_string_lines = [
        line.strip()
        for line in texts_resource.splitlines()
        if "IDS_FILEASSOCIATION" in line or "IDS_FILELAUNCH_LONGPATH_CAVEAT" in line
    ]
    require(all(not line.endswith('",') for line in association_string_lines),
            "association STRINGTABLE entries must not have trailing commas")
    page_tree = dialogs4[dialogs4.index("Add(&PageAppear"):dialogs4.index("void CConfigurationDlg::EnsureTabsPageVisibility")]
    require("Add(&PageFileAssociations" in page_tree and
            "Add(&PageViewer" in page_tree and
            "Add(&PagePP" in page_tree and
            "Add(&Page13" not in page_tree and
            "Add(&Page14" not in page_tree and
            "Add(&Page15" not in page_tree and
            "Add(&PageP4" not in page_tree,
            "the legacy association pages must not remain in the normal configuration tree")
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
