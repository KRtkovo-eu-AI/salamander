<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Add stable plugin target resolution. | Resolve canonical plugin targets through persisted identities and capability checks before runtime projection. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
| 2026-11-08 | Connect guarded canonical lifecycle hooks. | Load and retain canonical associations without replacing legacy runtime state when projection is unavailable. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
| 2026-11-08 | Add the transactional runtime projection boundary. | Require explicit target resolution and legacy metadata before replacing runtime associations. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
| 2026-11-07 | Document the runtime projection gate. | Keep target-only migration separate from incomplete legacy runtime reconstruction. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
| 2026-11-07 | Add portable-storage and migration boundaries. | Make the first integration helpers loss-aware without wiring incomplete runtime projections. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
| 2026-11-07 | Document complete association record persistence. | Make malformed numeric record gaps detectable before canonical data is committed. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
| 2026-11-07 | Record the canonical persistence and migration boundary. | Keep the backend slice honest about its registry-handle adapter and prevent premature lifecycle integration. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
| 2026-11-07 | Plan a canonical file-association model and unified Configuration UI. | Replace the fragmented Viewers, Alternate Viewers, Editors, and panel archive association workflow with a provenance-aware per-pattern model. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
-->

# File Associations Plan

This document defines the implementation plan for issue #733. It combines the
current Viewers, Alternate Viewers, Editors, and Archives Associations in Panels
configuration into one provenance-aware model and one user-facing editor.

The plan starts with the backend. The existing runtime consumers and legacy
configuration formats will remain usable during the migration so that the new
UI does not introduce a second competing configuration source or silently
change existing assignments.

## Confirmed Decisions

- The first scope includes Viewers, Alternate Viewers, Editors, and Archives Associations in Panels.
- External Viewers, Editors, and panel archivers are represented by a shared preset catalog.
- Packers and Unpackers used only by the Pack and Unpack dialog are out of scope for the first phase.
- A new `File Associations` page becomes the main Configuration entry point.
- The old pages are hidden from the normal Configuration tree only after feature parity is demonstrated. Their loading and migration code remains temporarily.
- The canonical unit is one file pattern per row, such as `*.txt`, `*.tgz`, or `*.*`.
- A category and pattern have at most one active target.
- Overlapping patterns in the same category are blocking conflicts, not silent fallback rules.
- Panel archive associations expose two independent operations: `List/Unpack` and `Delete/Pack`.
- Plugin defaults are a dynamic layer derived from currently loaded plugins.
- User overrides, explicit disabled assignments, and presets are persisted in a new canonical configuration model.
- Stable plugin and handler identities are used in the canonical model. Runtime plugin indexes remain only a projection detail.
- `Unassigned` is the visible name for an explicit no-target state. It suppresses the dynamic default and is therefore disabled for that pattern and category.
- `Reset to default` removes the explicit user state and enables the current dynamic default, if one exists.
- Old configurations are preserved as `Legacy`; manual intent that was not historically stored must not be guessed.
- Legacy conflicts are preserved for runtime compatibility and shown as `Legacy Conflict` until the user resolves them explicitly.
- The implementation order is backend-first with a parity gate before hiding the old pages.

## Current Architecture

The current configuration is split across independent models:

| Area | Current model | Main runtime meaning |
| --- | --- | --- |
| Viewers | `ViewerMasks` / `CViewerMasksItem` | Ordered file-mask to viewer mappings |
| Alternate Viewers | `AltViewerMasks` / `CViewerMasksItem` | Ordered alternate-viewer mappings |
| Editors | `EditorMasks` / `CEditorMasksItem` | Ordered file-mask to editor mappings |
| Panel archives | `CPackerFormatConfig` / `CPackerFormatConfigData` | Extension to list/unpack and pack/delete targets |
| External panel archiver definitions | `CArchiverConfig` | External archiver executable/path definitions |

Important existing locations:

- Configuration page declarations: `src/cfgdlg.h`
- Configuration tree and page order: `src/dialogs4.cpp`
- Viewer and editor pages: `src/dialogs5.cpp`
- Panel archive association page and archiver pages: `src/dialogsp.cpp`
- Viewer/editor registry serialization: `src/salamdr2.cpp`
- Archive association serialization and helpers: `src/pack3.cpp`
- Archive association migration: `src/mainwnd2.cpp`
- Plugin viewer and panel archiver registration: `src/plugins1.cpp`
- Dialog resources: `src/lang/lang.rc`, `src/lang/lang.rh`, and `src/lang/texts.rc2`

The new backend must initially project its effective state back into these
existing structures because many runtime consumers use them directly. The
canonical model must not be implemented as a second independently editable
cache.

## Canonical Backend Model

The proposed internal model is a normalized association repository. The exact
class names are intentionally left open until the first implementation slice,
but the model should contain the following concepts.

### Association key

Each association is identified by:

- a normalized file pattern,
- an association category,
- for panel archives, an operation (`List/Unpack` or `Delete/Pack`).

The category values are conceptually:

- `view`,
- `alternate-view`,
- `edit`,
- `panel-archive`.

Patterns must retain enough original text for display while also having a
canonical comparison form for conflict detection. The normalization rules must
be explicit and tested. They must cover case, optional `*.` notation, wildcard
patterns, separators, and the existing special patterns such as `*.*`.

### Target reference

Targets must be typed rather than represented by overloaded integer indexes.
The target kinds should include:

- built-in internal viewer,
- plugin viewer or plugin archive handler,
- external viewer preset,
- external editor preset,
- external panel archiver preset,
- `Unassigned`.

Each plugin target must use a stable identity independent of the current plugin
load order. A handler label is part of the identity where one plugin exposes
multiple viewer or archive handlers. Existing negative runtime indexes remain
valid only when materializing the compatibility projection.

### Provenance and state

The effective state needs separate provenance from target identity. At minimum,
the model should distinguish:

- `Default`: target comes from the current plugin/default layer,
- `User`: target was explicitly selected or edited by the user,
- `Legacy`: target came from an old configuration without complete provenance,
- `Disabled`: explicit `Unassigned` user state,
- `Conflict`: more than one legacy or default candidate overlaps and requires repair.

`Conflict` is a diagnostic state and must not be silently converted into a
winner. A valid newly saved canonical configuration must contain no unresolved
conflicts.

### Dynamic defaults and user layer

The effective association is resolved from two layers:

1. The dynamic default layer is rebuilt from plugin registrations and built-in
   defaults on each startup or plugin refresh.
2. The persistent user layer contains explicit targets, explicit `Unassigned`,
   and references to external presets.

The user layer takes precedence over defaults. Removing a user override exposes
the current default again. An explicit `Unassigned` suppresses that default.

The model must not persist a stale copy of plugin defaults as if it were user
intent. This is necessary for plugin updates and for a reliable `Reset to
default` action.

### External preset catalog

The preset catalog must support external Viewers, Editors, and panel archivers.
Each preset should contain:

- stable preset ID,
- user-visible name,
- target kind,
- command or executable,
- arguments,
- initial directory,
- capability flags relevant to the target,
- reference count or reverse references for safe deletion.

Commands, arguments, initial directories, and executable paths must use the
existing long and Unicode path rules. New path handling must not introduce
`MAX_PATH`-sized path buffers or ANSI-only process creation paths.

## Persistence and Migration

### New configuration storage

Add a versioned canonical configuration root through the existing configuration
storage abstraction. The implementation must work with both the Windows
Registry-backed and file-backed configuration paths.

The schema should store:

- schema version,
- user association records,
- explicit `Unassigned` records,
- preset definitions,
- stable target references,
- migration metadata sufficient to make conversion idempotent.

The exact value names and hierarchy should be finalized before code is added.
They must be documented in the serialization tests and must not depend on
localized display names.

### Implemented backend boundary

The first backend slice is deliberately not connected to `LoadConfig` or
`SaveConfig`. `CFileAssociationModel::Load` and `Save` currently use the
project's registry-compatible `HKEY` helpers and a caller-provided root key.
Schema version 1 stores `Schema Version` and `Association Count` at the root,
an `Associations` child key, and one numeric child per association with
`Pattern`, `Category`, `Operation`, `Target Kind`, `Target ID`, `Provenance`,
and `Source Provenance` values. The count makes a missing numeric child a
malformed input instead of an accepted truncated model. This layout is
versioned and does not use localized labels.

Load treats an absent root as an unchanged model, parses into a temporary
model, preserves serialized provenance fields including `Conflict`, and
commits only after the complete input is accepted. Persisted conflict rows are
allowed so an unresolved legacy state remains inspectable; `Save` still rejects
unresolved conflicts. A present malformed schema, missing record fields, or an
unmarked overlapping pair is rejected without replacing the current model.

The application's `CConfigurationStorage` exposes
`CSalamanderRegistryExAbstract`, while the current model API uses global
`HKEY` functions. A registry/file-backed adapter or equivalent overload is
therefore required before this persistence is claimed to support both storage
modes. `LoadFromConfigurationStorage` and `SaveToConfigurationStorage` now
provide that boundary: they open the configuration root through
`CConfigurationStorage`, reuse the existing transactional serializer, and
close the active key through the registry-routing helpers. A missing root is
still treated as an unchanged model on load. The adapter does not alter the
legacy lifecycle or create a second registry path.

Legacy conversion is exposed separately through the loss-aware
`MigrateLegacyAssociations` batch helper. It stages the model and commits only
when every candidate has exactly one valid mask and a non-empty stable target
identity. Semicolon groups, `|` exclusions, malformed masks, unassigned legacy
targets, and missing target identities block the complete batch; the caller can
then keep the old runtime projection active. No canonical root is created by
this helper.

### Reading old configurations

When the canonical root is absent:

1. Load the current four legacy models exactly as today.
2. Load current plugin defaults and built-in defaults.
3. Expand grouped masks and archive extension lists into individual patterns.
4. Preserve original order and source information for diagnostics.
5. Use `User Extension List` for archive provenance where available.
6. Mark information that cannot be proven as `Legacy` rather than guessing.
7. Detect overlapping patterns and retain them as `Legacy Conflict`.
8. Keep the legacy runtime projection active until the user resolves and saves
   the new model.

Migration must not silently assign a new owner, delete a legacy row, or choose a
winner solely because one row appeared first.

The migration adapter has the following additional gate rules:

- A grouped viewer or editor mask is split only by the existing mask parser;
  raw text containing an exclusion or malformed token is retained as a legacy
  source item and is not silently converted to a different pattern.
- A legacy row is not projected to a canonical plugin or preset target until
  the target has a stable identity. A session plugin index, row ordinal, or
  localized label is never persisted as a target ID.
- Archive rows use `User Extension List` as evidence of explicit ownership.
  The evidence is preserved separately from the legacy source, and extensions
  absent from that list remain legacy/default candidates rather than being
  promoted to user intent.
- If a row cannot be represented without losing target identity, grouping, or
  conflict information, migration leaves the old runtime projection active and
  records the row as unresolved legacy data. It does not create a canonical
  root merely to discard that information.

These rules make migration idempotence and runtime projection testable before
any startup or save call is changed.

The migration report also exposes `RuntimeProjectionBlocked`. It is true for a
lossy batch and remains true after a successful target-only import until a
projection can reconstruct legacy command, argument, initial-directory, and
session-plugin-index fields. Callers must therefore keep the existing legacy
runtime structures active whenever this flag is set; this helper does not wire
startup or save lifecycle calls and does not claim runtime equivalence.

### Fallback and downgrade behavior

The canonical root becomes the preferred source after a successful migration.
Legacy keys remain available as fallback/import data during the transition.
The implementation must define what happens when:

- the canonical root is malformed,
- a referenced plugin or preset is missing,
- a newer schema version is encountered,
- the user starts a build that only understands the legacy format.

These cases must fail conservatively and preserve the last usable configuration.

## Conflict Rules

Conflicts are evaluated independently for each category and, for panel archives,
independently for each operation.

Examples that must be handled explicitly:

- `*.*` and `*.txt` in the View category,
- one extension assigned to two legacy viewer rows,
- a plugin default and a user target for the same normalized pattern,
- separate List/Unpack and Delete/Pack targets for one archive extension,
- a preset deleted while still referenced,
- a plugin handler renamed or removed between sessions.

New canonical writes must reject unresolved conflicts. The running application
may continue using the old runtime projection while a migrated legacy conflict
is awaiting user repair. No repair may happen implicitly during startup.

## Runtime Projection

The first implementation should provide a deterministic projection from the
canonical model into current runtime structures:

- `ViewerMasks`,
- `AltViewerMasks`,
- `EditorMasks`,
- `CPackerFormatConfig`.

The projection must:

- resolve stable plugin identities to current session indexes,
- preserve built-in target encodings,
- preserve panel archive operation separation,
- preserve user provenance for archive migrations,
- reject or report targets that cannot be resolved,
- rebuild lookup arrays only after a valid projection,
- remain transactional so a failed projection does not partially replace the
  running configuration.

The projection layer should initially be the only compatibility bridge. Runtime
call sites should not be rewritten until the new model has been exercised by
the complete UI and migration tests.

The current implementation intentionally stops before this layer. Stable
plugin IDs are persisted, but no resolver currently maps them to the session
indexes consumed by `ViewerType`, and canonical target rows do not carry the
legacy command/argument/initial-directory fields. `RuntimeProjectionBlocked`
is the explicit guard for this boundary; lifecycle integration remains
disabled rather than applying a partial projection.

The implemented boundary is `ProjectRuntimeAssociations`. It accepts explicit
`begin`, `apply`, `rollback`, and `commit` callbacks. The callback owner is
responsible for resolving stable targets and staging all legacy command,
argument, initial-directory, built-in, and session-index metadata. The bridge
does not mutate runtime structures when callbacks are incomplete, when a
target or its metadata cannot be resolved, or when commit fails. It rolls back
the staged projection on every failed apply or commit. A concrete runtime
adapter is still required before lifecycle call sites can use this boundary.

The plugin resolver is now concrete at the identity boundary. `CPluginData::RegKeyName` is
exposed as its stable ID and `CPlugins::FindByStableId` resolves it under the plugin data lock.
`ResolvePluginRuntimeAssociation` checks current plugin capabilities and derives the session
`ViewerType` or archive index only for supported View, Alternate View, List/Unpack, and
Delete/Pack associations. It rejects editor command metadata, built-in and preset targets,
missing plugins, and unsupported capabilities rather than guessing. A complete legacy adapter
must still provide transactional Begin/Apply/Rollback/Commit callbacks; the resolver is attached
through `AttachPluginRuntimeResolver`, and lifecycle registration remains blocked until that
callback owner can stage legacy rows atomically.

Guarded lifecycle hooks are now connected around the existing legacy configuration order. After
plugins and legacy viewer rows load, the lifecycle helper reads the canonical root into temporary
internal state and invokes only an explicitly registered projection adapter. With no adapter, an
unresolved target, or missing legacy metadata, it records a blocked projection and leaves the
legacy runtime arrays untouched. The save hook runs after the existing legacy viewer/editor saves,
but writes the canonical root only after a nonempty model has loaded and a complete projection has
committed; otherwise it is a successful no-op. This keeps optional canonical loading from failing
startup and prevents blocked lifecycle state from creating or overwriting persisted canonical data.

## Unified Configuration UI

### Main matrix

The main page should contain one searchable matrix with:

| Column | Meaning |
| --- | --- |
| Pattern | One normalized file pattern per row |
| View | Current View target and provenance |
| Alternate View | Current alternate-view target and provenance |
| Edit | Current Editor target and provenance |
| Panel Archive | Summary of List/Unpack and Delete/Pack targets |

Cells should show a short target name and a compact state badge. Long commands
and paths belong in the detail pane, not in the matrix.

### Detail editor

The detail editor must support:

- assigning a built-in or plugin target,
- selecting or editing an external preset,
- creating a new pattern,
- changing an existing pattern,
- setting explicit `Unassigned`,
- resetting to the current default,
- viewing provenance and source information,
- resolving a legacy conflict,
- editing both panel archive operations independently,
- validating command, argument, and initial-directory fields.

### Preset management

The page must make external presets discoverable without forcing users to
search several old pages. It should show:

- preset name,
- kind and capabilities,
- command summary,
- patterns using the preset,
- edit, duplicate, and delete actions.

Deleting an in-use preset must be rejected or require an explicit reassignment
before the delete can be committed.

### Filters and bulk actions

The first usable version should support:

- text search by pattern, target, or preset name,
- category filter,
- provenance filter,
- conflict-only filter,
- multi-select patterns,
- assign, unassign, reset, and conflict-resolution actions for compatible
  selections.

All visible strings, state badges, validation messages, and explanatory text
must use localized resources. The page must support dark mode, `MS Shell Dlg`,
keyboard navigation, resize, and existing property-sheet transaction rules.

## Implementation Phases

### Phase 0: Contract and inventory

- Document the canonical schema and stable identity rules.
- Enumerate every legacy field and its migration source.
- Define pattern normalization and conflict detection.
- Define missing-plugin, missing-preset, malformed-schema, and downgrade behavior.
- Decide the exact registry/configuration key layout.

Deliverable: backend contract document and representative migration fixtures.

### Phase 1: Canonical backend

- Add canonical association and preset data structures.
- Add versioned save/load through a `CConfigurationStorage` registry adapter.
  The current registry-handle implementation remains an isolated backend
  slice until that adapter is available.
- Add dynamic default registration and stable target identity mapping.
- Add user-layer resolution and explicit `Unassigned` handling.
- Add deterministic conflict reporting.

Deliverable: backend can load, validate, resolve, and persist data without UI
changes.

### Phase 2: Migration and runtime projection

- Convert Viewers, Alternate Viewers, Editors, and panel archive rows.
- Preserve `Legacy` and `Legacy Conflict` states.
- Reuse and extend archive provenance from commit `92243dee`.
- Materialize valid effective associations into current runtime structures.
- Keep legacy projection active for unresolved legacy conflicts.

Deliverable: old configurations continue to work and can be inspected through
the canonical model without silent ownership changes.

### Phase 3: External preset catalog

- Extract inline external Viewer and Editor command definitions into presets.
- Map external panel archiver definitions to the same target catalog.
- Preserve stable IDs and references during save/load and migration.
- Add safe edit, duplicate, delete, and in-use validation.

Deliverable: all external targets needed by the unified page are editable from
one place.

### Phase 4: Unified UI

- Add the new Configuration page and resources.
- Implement the matrix, search, filters, badges, and detail editor.
- Implement preset management and panel archive operation details.
- Add validation and transactional commit.
- Preserve existing Configuration page navigation until parity is verified.

Deliverable: the new page is a complete replacement for the four in-scope
pages.

### Phase 5: Parity gate and transition

- Compare every old-page operation with the new-page equivalent.
- Hide the old pages from the normal tree only after all parity checks pass.
- Retain old loaders and fallback import paths.
- Verify page-index and last-focused-page compatibility.
- Document migration and downgrade behavior.

Deliverable: one user-facing association workflow with safe fallback behavior.

## Test Plan

### Backend and persistence

- Canonical model round-trip preserves all fields and states.
- Save/load is idempotent.
- Malformed and newer schemas fail without destroying the last valid state.
- Stable plugin identities resolve correctly after plugin load-order changes.
- Missing plugins remain visible and do not silently reassign patterns.
- Preset references survive save/load and in-use deletion is blocked.

### Migration

- Viewer masks split into one-pattern records.
- Alternate Viewer masks migrate independently.
- Editor masks migrate independently.
- Archive extensions migrate with `User Extension List` provenance.
- Old rows without provenance become `Legacy`, not inferred `User` or `Default`.
- `*.*` plus `*.txt` becomes an explicit `Legacy Conflict`.
- Migration is repeatable and does not duplicate records.
- Legacy runtime behavior remains available while conflicts are unresolved.

### Resolution and projection

- User target overrides a dynamic default.
- `Unassigned` suppresses a dynamic default.
- `Reset to default` restores the current dynamic default.
- Plugin removal produces an unavailable target rather than a new owner.
- Panel List/Unpack and Delete/Pack targets remain independent.
- Runtime projections preserve existing target encodings.
- Failed projection leaves the previous runtime configuration intact.

### UI contract and manual coverage

- Matrix shows every pattern and category.
- Badges distinguish Default, User, Legacy, Legacy Conflict, and Unassigned.
- Search and filters work across patterns, targets, and presets.
- Detail editing covers all operations formerly available on the four old pages.
- Preset editing updates all referencing associations safely.
- Keyboard navigation, resizing, localization, and dark mode work.
- Manual path cases cover Unicode and long command/executable paths.
- Old Configuration page indices and last-focused-page behavior remain safe.

Source-contract tests are useful for wiring and migration invariants, but they
must be supplemented with executable model tests and at least one end-to-end
Configuration test before the old pages are hidden.

## Risks and Mitigations

| Risk | Mitigation |
| --- | --- |
| Existing runtime uses four unrelated structures | Keep a deterministic compatibility projection in the first implementation |
| Plugin indexes change between sessions | Persist stable plugin/handler identities and resolve indexes only at runtime |
| Old configurations contain ambiguous manual intent | Preserve `Legacy`; never infer provenance retroactively |
| Existing wildcard rows overlap | Preserve as `Legacy Conflict`; require explicit repair |
| New UI becomes another partial editor | Enforce a parity checklist before hiding old pages |
| External presets break paths or command expansion | Reuse long/Unicode path-safe helpers and add dedicated tests |
| Plugin SDK binary compatibility is broken | Do not insert existing virtual methods; prefer internal mappings or append-only API changes |
| Configuration tree indices change | Update startup mappings and last-focused-page handling together |
| Failed migration leaves partial state | Use validate-then-commit and retain the previous valid projection |

## Completion Criteria

The work is complete when:

- the new canonical model is the only editable source for in-scope associations,
- old configurations migrate without silent ownership changes,
- unresolved legacy conflicts are visible and actionable,
- dynamic plugin defaults and user overrides have distinct provenance,
- external Viewers, Editors, and panel archivers are managed through presets,
- the unified matrix replaces the four old pages without feature loss,
- runtime consumers continue to receive valid projections,
- save/load, migration, plugin-update, conflict, and UI parity tests pass,
- localization, dark mode, keyboard navigation, resize, and long/Unicode path
  requirements are covered.
