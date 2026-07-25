# Salamatrix framework gap analysis

Status date: 2026-07-24

Source vision: <https://krtkovo.eu/salamander/framework.html>

This audit compares the framework vision with the implementation in this
repository. It distinguishes working code from API placeholders, samples, and
catalog metadata. The target is not to reproduce the complete native Plugin SDK
in scripting languages. It is the middle extensibility layer described by the
vision: persistent lightweight extensions with commands, UI, events, storage,
and a shared Salamander object model.

## Executive summary

The repository already contains a real Salamatrix foundation, not only a design:

- `SALAMATRIX.SPL` is a load-on-start Automation Framework provider.
- The core has a process-local, versioned service registry.
- Native and Automation callers share the first `Salamatrix.UI`,
  `Salamatrix.Commands`, `Salamatrix.FileOperations`, and `Salamatrix.Sides`
  services.
- `Salamatrix.Runtime` is a registered broker with versioned adapter
  descriptors, registration, enumeration, and lookup.
- `Salamander.UI.progress(...)`, `Salamander.Commands.execute(...)`, and the
  three interactive file-operation wrappers work through the Salamatrix bridge.
- Automation discovers recursive script directories and reads a minimal
  `extension.json` next to an entry point.
- `Salamander.Sides` exposes Left, Right, Source, and Target plus safe tab
  snapshots, paths, and activation to Automation scripts.
- `Salamatrix.Storage` and `Salamander.Storage` provide typed, synchronized,
  persistent namespaces isolated by manifest extension id.
- `Salamatrix.Extensions` now provides an owner-aware lifecycle registry, and
  Automation publishes/removes manifest descriptors during script-list load and
  refresh.
- The Plugin Manager has an enabled Extension Runtimes catalog source and
  Salamatrix has a distinct Automation Framework capability flag.

This is an MVP/PoC, not yet the framework described by the vision. In particular:

- Automation advertises and executes available legacy Active Scripting engines
  through the broker. Modern adapters, dependency environments, cancellation
  transport, persistent runtime lifecycle, worker bootstrap, and the first
  host-call/UI bindings are now present; richer values and long-lived event
  queues remain.
- JavaScript is legacy Windows JScript through `IActiveScript`, not a modern
  bundled JavaScript runtime. Python, PHP, Lua, and Ruby still depend on old
  third-party Active Scripting engines unless the new CPython/PHP CLI adapters
  are selected. PowerShell now has an optional CLI adapter, but no persistent
  runspace or Salamander object bridge yet.
- The first modern side/tab model is exposed through the native SDK,
  Salamatrix, and Automation without raw core pointers. Item collections, view
  settings, refresh, tab creation/closing, and change events are still missing.
- Modern manifest-backed scripts can run through the shared worker bootstrap and
  persistent extensions can subscribe/register dynamically; one-shot scripts
  still cannot remain alive after their execution ends.
- The strict schema-1 manifest parser validates package/runtime identity,
  minimum runtime version, safe entry points, capabilities, and multiple command
  records. Dependencies, icons, localization, settings/storage declarations,
  and dynamic publication of all handlers are still missing.
- The shared native UI service exposes progress, input-box, and a reusable
  `IDialog`/`IControl` contract for modern workers and native plugins. The
  native implementation now creates Label, TextBox, CheckBox, RadioButton,
  ComboBox, Button, ListView, TreeView, and TabControl controls with item/node binding;
  columns, layout, notifications, validation, and the legacy Forms migration
  remain.
- `Salamatrix.AI` now has a provider-neutral contract and Automation registers
  an optional local command provider through `SALAMATRIX_AI_COMMAND`. The
  provider exchanges one bounded JSON request/response and enforces a timeout.
  Automation now provides Ask-AI context capture, validated preview, clipboard,
  and explicit Save As; repair, runtime execution, package save, and bundled
  inference remain.

## Requirement matrix

| Vision area | Status | Current implementation | Missing work |
| --- | --- | --- | --- |
| Three extensibility levels | Partial | Native plugins and one-shot Automation scripts exist; manifest descriptors are now registered in an owner-aware lifecycle catalog, activation retains a persistent runtime session, and the host-call dispatcher binds commands, sides, storage, event subscriptions, and a message-box UI call. | Richer UI/value bindings, command ownership, worker bootstrap/library, and unload leases. |
| Shared cross-runtime API | Partial | The Salamatrix service layer and Automation bridge are real. | Runtime-neutral values/calls, complete object model, error model, threading rules, and bindings for every runtime. |
| Left/Right/Source/Target sides | Partial | `Salamatrix.Sides` and all modern workers resolve all four references and expose active tabs, bounded path/type, selected-item snapshots, focused-item metadata, and active/source/target/locked/detached flags without raw core pointers. | View mode/tree state, refresh, item change events, and main-thread marshaling for long-lived snapshots. |
| Tabs and detached windows | MVP | SDK snapshots and opaque process-local ids expose tab count, index, path/type, active/source/target/locked/detached flags, activation, and stale-handle-safe lookup without raw core pointers. | Create/close/reorder/detach APIs, colors, lifecycle events, richer detached-window operations, and persistence semantics. |
| Existing Salamander commands | Partial | The stable catalog now covers view/edit/open, rename, copy/move, email/delete/properties, case/attribute/space operations, refresh, directory creation, drive info, directory sizes, and disconnect; execution still uses Salamander's normal enablement and dialogs. | Parameterized command calls, richer synchronous/modal results, state/change notifications, and non-modal operation handles. |
| Programmatic file operations | Partial | `IFileOperationsService` and all modern worker bindings expose interactive rename/copy/move/delete/create-directory/refresh/properties workflows. | Typed source/target values, progress/cancellation handles, and final structured results without requiring the native dialog. |
| Extension command registration | Partial | Discovery-time metadata gives a script stable identity/caption/placement hints, and a persistent worker can register/unregister multiple owner-scoped commands with synthetic native menu ids and context masks; removal triggers a menu rebuild. | Enable/visible callbacks, icons, toolbar/hotkey binding, and richer ownership/unload leases. |
| Plugin and context menu placement | Partial | Metadata booleans and context masks exist. | The native menu-extension surface does not yet model independent placement cleanly; add explicit placement and dynamic command contribution services. |
| Toolbar and shortcuts | Missing | Normal native plugin menu items can participate in existing hotkey handling. | Manifest/API contributions, user assignment, icons, persistence, conflict handling. |
| Events | MVP | `Salamatrix.Events` maps host lifecycle/settings/configuration/color/panel events to unsubscribe-safe native callbacks; Automation exposes `subscribe/unsubscribe` with copied payloads. | Persistent extension instances, worker queues, UI-thread marshalling, coalescing, event replay, and unload-safe leases across modern runtimes. |
| Per-extension storage | MVP | `Salamatrix.Storage` persists isolated manifest-id namespaces with UTF-8 strings, signed 64-bit integers, booleans, delete/clear, validation, and synchronized access. Automation exposes `has/get/set/remove/clear`; legacy global persistence remains for compatibility. | Settings schemas/files, enumeration, quotas, migrations, transactional batches, package uninstall retention/deletion policy, and bindings in modern runtimes. |
| Shared UI framework | Partial | `Salamatrix.UI` now provides a reusable native `IDialog`/`IControl` model and `NativeDialog` implementation for labels, text boxes, check/radio buttons, combo boxes, buttons, native ListView/TreeView/TabControl controls with item binding, message boxes, input boxes, clipboard copy, dark-mode initialization, and localized Automation prompts; all workers expose the shared calls. | Columns/selection events, pickers, notifications, validation/events, layout, DPI/theme/accessibility, and migration of the legacy Automation Forms wrappers. |
| Manifest/package | Partial | Strict UTF-8 JSON parsing validates schema 1, package/runtime identity and minimum version, safe entry point, capabilities, and up to 64 command records. | Expose every parsed command, then add dependencies, icons, locales, settings, events, package install/uninstall, and richer diagnostics. |
| Runtime adapters | Partial | `Salamatrix.Runtime` registers/enumerates versioned adapters and executes manifest entry points through structured requests/results. Automation advertises legacy ActiveScript compatibility adapters plus optional out-of-process CPython, PowerShell, and PHP CLI adapters with timeout, exit-code, bounded output, `IRuntimeSession` persistent-worker support, and host calls for commands, sides, storage, events, AI generation/preview, input-box/dialog UI, clipboard, and dialog item binding. | Rich UI/full value binding, cancellation plumbing beyond process termination, debugging, dependency environments, and bundled modern JavaScript. |
| JavaScript runtime | Legacy only | Windows JScript is the hard-coded fallback for `.js`. | Supported bundled modern engine and compatibility adapter for existing JScript/VBScript scripts. |
| Python runtime | Process MVP | ActivePython COM compatibility remains; `Python.CPython` discovers `python.exe`/`python3.exe` or `SALAMATRIX_PYTHON`, runs `.py` entries out of process, and the optional worker bootstrap exposes the shared SMX1 `Salamander` API including multiple command registration. | Richer UI/value bindings, environment/dependency policy, and bundled runtime. |
| PowerShell runtime | Process MVP | `PowerShell` discovers `pwsh.exe`/`powershell.exe` or `SALAMATRIX_POWERSHELL`, runs `.ps1` entries out of process, and ships the same SMX1 worker bootstrap/API shape including multiple command registration. | Richer UI/value bindings, environment policy, cancellation/error mapping, and bundled runtime. |
| PHP runtime | Process MVP | Legacy PHPScript remains; `PHP.CLI` discovers `php.exe` or `SALAMATRIX_PHP`, runs `.php` entries out of process, and ships the same SMX1 worker bootstrap/API shape including multiple command registration. | Richer UI/value bindings, dependency policy, and support decision/bundled runtime. |
| Extension management | Partial | Plugin Manager supports sources and the Extension Runtimes catalog; `Salamatrix.Extensions` tracks manifest descriptors and lifecycle state. | Scripted-extension package type, installed-state model, enable/disable/configure/remove, dependency prompts, runtime status. |
| Permissions/capabilities | Missing | `requires` only controls whether a menu command is enabled in the current panel context. | Declared/granted capabilities, effect preview, runtime enforcement boundaries, network/process/filesystem policies, audit trail. |
| AI automation assistant | Partial | `Salamatrix.AI` exposes provider registration, structured generation validation, parsed effect flags, runtime/existing-script hints, and a safe `generate`/`preview` worker seam; Automation supplies an optional bounded local command provider selected by `SALAMATRIX_AI_COMMAND` plus an Ask-AI menu action that gathers source-panel context, shows a preview summary, copies the script for review, offers an explicit Run when the response names a safe runtime, and offers Save As. | Repair/conversation loop, save-as-extension packaging, bundled llama.cpp inference, and optional remote providers. |
| Testing | Initial | Standalone `/W4 /WX` tests cover the strict manifest parser, Storage persistence, Events subscribe/publish/self-unsubscribe/capacity/payload validation, Extensions registration/lifecycle/ownership, runtime raw-JSON context and AI service contracts, and an integration test for Python/PowerShell/PHP process execution, output capture, and timeout. Worker syntax checks cover Python, PowerShell, and PHP. | Core registry and runtime contract tests, native UI tests, extension fixtures, lifecycle/unload tests, and end-to-end Salamander API adapter tests. |

## Existing implementation evidence

### Core service and plugin integration

- `src/plugins/shared/spl_gen.h` declares `RegisterService`,
  `UnregisterService`, and `QueryService`.
- `src/zip.cpp` contains the current fixed-size process registry.
- `src/plugins/shared/spl_base.h` defines
  `FUNCTION_AUTOMATIONFRAMEWORK`.
- `src/plugins/salamatrix/salamatrix.cpp` creates persistent
  `RuntimeServices`, registers the provider, and requests load on startup.
- `src/vcxproj/salamand.sln` builds Salamatrix as part of the main solution.

### Shared services already working

- `src/plugins/salamatrix/salamatrix_ui.h` adapts the existing Salamander
  progress implementation.
- `src/plugins/salamatrix/salamatrix_commands.h` exposes the small stable
  command catalog and interactive rename/copy/move routing.
- `src/plugins/salamatrix/salamatrix_sides.h` exposes runtime-neutral sides and
  panel-tab snapshots over the new core SDK methods.
- `src/plugins/salamatrix/salamatrix_storage.h` owns validated, synchronized,
  typed per-extension storage and its versioned configuration blob.
- `src/plugins/salamatrix/salamatrix_events.h` owns host event mapping and
  unsubscribe-safe callback dispatch.
- `src/plugins/automation/salamatrixbridge.cpp` also owns the bounded
  out-of-process CPython, PowerShell, and PHP CLI adapters and their interpreter
  discovery policy.
- `src/plugins/salamatrix/salamatrix_runtime_protocol.h` defines the bounded
  incremental `SMX1` worker framing and its message vocabulary; the Automation
  bridge now supplies the first host-call dispatcher over that boundary.
- `src/plugins/automation/scriptlist.cpp` owns the lifecycle callback and the
  initial `runtime.ready`, commands, sides, and string-storage dispatch.
- `src/plugins/automation/salamatrixbridge.cpp` is a consumer-only bridge.
- `src/plugins/automation/salamatrixaut.cpp` implements the COM/Automation
  wrappers.
- `src/plugins/automation/salamander.idl` publishes the `UI`, `Commands`,
  `FileOperations`, `Sides`, `Events`, and isolated `Storage` namespaces.

### Script discovery and manifest parsing

- `src/plugins/automation/extensionmanifest.cpp` contains the strict, independently
  tested schema-1 JSON parser and validated manifest model.
- `src/plugins/automation/scriptlist.cpp` recursively discovers legacy
  ActiveScript files, registered runtime-adapter entry points, and valid
  manifest entry points even when their extension has no COM association.
- The same file resolves manifest runtime id/minimum version through the broker
  before execution. Inline `Salamatrix.Command*` metadata remains supported.
- `src/plugins/automation/sample-scripts/Salamatrix Progress Demo/` is the
  first manifest-based sample.

### Existing legacy Automation value

The new platform should preserve these working capabilities:

- active left/right/source/target panel path, focused item, all items, selected
  items, selection manipulation, and panel path changes;
- message/input/error/question/overwrite dialogs, progress and wait windows;
- modal Forms with Label, TextBox, CheckBox, and Button;
- file viewing, path normalization, mask matching, tracing, sleep/cancel;
- basic global persistent values;
- JScript and VBScript, plus third-party `IActiveScript` engines where
  available.

## Important implementation risks found by the audit

1. The core registry stores borrowed string and interface pointers, has no
   provider ownership, leases, reference counting, or unload protection.
2. Service interface pointers are still borrowed and the core cannot prevent a
   provider unload while a consumer is actively using a service. Salamatrix now
   rolls back partial registration and the Automation bridge verifies broker
   identity before cleanup, but general ownership/lease rules remain missing.
3. Posted Commands/FileOperations report `ok` when a command was accepted, not
   when the modal operation completed successfully. `cancel` is not observable.
4. The command catalog currently maps both `Move` and `MoveRename` to the same
   native command and contains only three unique operations.
5. The manifest parser retains every validated command, but the legacy
   Automation menu surface currently contributes only the first command.
6. Discovery and execution now go through runtime adapters, but the v1
   compatibility callback still keeps old ActiveScript hosting inside
   Automation. Out-of-process modern adapters and cancellation transport remain
   to be implemented.
7. Long-lived script callbacks require explicit UI-thread, reentrancy,
   cancellation, exception, and unload rules; the current one-shot
   `IActiveScript` lifecycle does not provide them.
8. A language process must never receive raw C++/COM implementation pointers.
   Out-of-process adapters need a versioned RPC/value protocol, while
   in-process adapters need ABI-stable interfaces.

## Recommended implementation order

### Phase 0: make the foundation safe

1. Route extension discovery and execution through the existing
   `Salamatrix.Runtime` broker. **Implemented.**
2. Add provider ownership/leases, diagnostics, and lifecycle rules to runtime
   and core services.
3. Replace the manifest text scanner with a real parser and validate a versioned
   schema.

### Phase 1: complete the useful common object model

1. Publish `Salamatrix.Sides` with Left, Right, Source, and Target.
   **Implemented.**
2. Add side and tab snapshots/handles: path, active tab, tabs, focused/selected
   items, view mode, tree visibility, detached state. **Implemented for tab
   identity, path, active/source/target/locked/detached state, and activation;
   item/view details remain.**
3. Add clipboard and panel refresh.
4. Expand stable Commands and implement synchronous, programmatic file
   operations.
5. Add isolated per-extension storage. **Implemented for typed key/value
   persistence; settings schemas, quotas, migrations, and uninstall policy
   remain.**

### Phase 2: make extensions persistent

1. Extension instances and lifecycle: discover, load, activate, deactivate,
   unload. **The owner-aware `Salamatrix.Extensions` registry and persistent
   `IRuntimeSession` activation seam are implemented; host dispatch and
   worker bindings remain.**
2. Command contribution service independent of menu/toolbar/hotkey placement.
3. Event service with a small first set: startup/shutdown, active side/tab,
   path, selection, tab create/close. **Implemented for host lifecycle,
   settings/configuration/colors, panel swap, and active-panel events; persistent
   instance queues and richer side/tab events remain.**
4. Package state in the existing Plugin Manager.

### Phase 3: runtimes

1. Keep current Automation as the `legacy-wsh` compatibility adapter.
2. Add a modern bundled JavaScript adapter as the default lightweight runtime.
3. Add PowerShell and CPython adapters as separate runtime components.
4. Add PHP only after the common out-of-process protocol is proven; it should
   reuse the same protocol rather than introduce a fourth binding architecture.
5. Run the same runtime contract suite against every adapter.

An out-of-process JSON-RPC-style transport is the safest common denominator for
CPython, PowerShell, and PHP. The host owns all Salamander objects and returns
serializable handles/snapshots; runtimes send typed API requests. A bundled
JavaScript engine may be in-process for size and speed but should implement the
same logical contract.

### Phase 4: shared UI

Extend the existing `Salamatrix.UI` native control implementation, then keep
the old Automation Forms classes as compatibility wrappers. Build the remaining
surface in this order: column binding, layout, pickers, notifications,
validation/events. Item/node/tab binding is now available. All controls need
dark-mode, DPI, localization, keyboard, accessibility, validation, and event
tests.

### Phase 5: AI assistant

Start only after the common API is useful without AI:

1. Generate a machine-readable API schema from the real contracts.
2. Define a provider-neutral model interface and structured result containing
   title, description, required capabilities, estimated effects, and script.
3. Implement Ask -> Preview -> Validate -> Run -> Save.
4. Add static validation and a repair loop against the exact installed API
   version.
5. Add an external local provider first, then an optional bundled local backend.
6. Treat the AI as a script author with no privileged control path.

## First implementation slice

The first code slice following this audit is implemented:

- publish `Salamatrix.Runtime` as a real service;
- allow runtime adapters to register versioned runtime ids and file/language
  metadata;
- make Automation advertise its legacy Active Scripting adapter through that
  broker;
- expose runtime availability in diagnostics;
- make registration failure transactional.

The second code slice is also implemented:

- replace raw scalar scanning with a strict schema-1 UTF-8 JSON parser;
- validate runtime/minimum version, safe entry points, capabilities, and the
  complete commands array;
- discover manifest entry points independently of ActiveScript associations;
- route manifest execution through a structured runtime-adapter request/result;
- execute existing JScript/VBScript compatibility engines through the broker;
- add a standalone manifest parser test target and invalid-manifest fixtures.

The third code slice is also implemented:

- append versioned tab snapshot and activation methods to the native plugin SDK
  and raise the SDK requirement to 105;
- assign every live panel tab an opaque 64-bit lifetime id;
- publish `Salamatrix.Sides` with Left/Right/Source/Target resolution, tab
  snapshots, paths, activation, and active-tab path changes;
- expose the service to scripts as `Salamander.Sides`, `Side`, and `Tab`
  Automation objects;
- represent ids as decimal strings and revalidate each operation by id so stale
  script objects fail safely;
- compile the main executable, Salamatrix, and Automation in Debug x64, Debug
  Win32, and Release x64.

The fourth code slice is also implemented:

- publish `Salamatrix.Storage` as a transactionally registered service;
- isolate namespaces by the validated manifest extension id instead of asking
  scripts to invent collision-resistant prefixes;
- support UTF-8 strings, signed 64-bit integers, booleans, delete, and clear
  through a synchronized runtime-neutral contract;
- persist one bounded, versioned, fully validated configuration blob through
  the plugin registry abstraction;
- expose `Salamander.Storage.has/get/set/remove/clear` only to manifest
  extensions and preserve the old global API for loose-script compatibility;
- add `/W4 /WX` tests for isolation, validation, types, mutation, save/load
  round-trip, unchanged saves, and corrupt data.

The fifth code slice is also implemented:

- publish `Salamatrix.Events` with lifecycle, settings/configuration, colors,
  panel-swap, and active-panel event kinds;
- map the existing Salamander plugin `Event` callback into copied,
  runtime-neutral payloads containing panel/tab/path context;
- dispatch matching callbacks outside the registry lock so self-unsubscribe and
  reentrant subscription are safe;
- expose `Salamander.Events.subscribe/unsubscribe` with decimal opaque ids and
  automatic cleanup when the one-shot Automation root is released;
- add `/W4 /WX` tests for event delivery, self-unsubscribe, capacity, and invalid
  payload/kind handling.

The sixth code slice is also implemented:

- publish `Salamatrix.Extensions` as a bounded, owner-aware lifecycle registry;
- track explicit discovered/activating/active/deactivating/inactive/failed
  states and invoke lifecycle callbacks outside the registry lock;
- register valid manifest-backed Automation scripts at load/refresh and remove
  them before their `CScriptInfo` owners are deleted;
- retain and stop a persistent `IRuntimeSession` from the owning script during
  activate/deactivate and destruction;
- add `/W4 /WX` tests for registration, duplicate-owner rules, lifecycle state
  transitions, validation, and owner cleanup.

This slice intentionally stops before host API dispatch. The registry and
session are the stable seam for the next runtime-host work.

The seventh code slice is now implemented:

- extend the runtime request/result contract with working-directory, timeout,
  process/exit, and bounded output fields;
- register optional out-of-process `Python.CPython`, `PowerShell`, and `PHP.CLI`
  adapters from Automation using explicit environment overrides or `PATH`;
- execute entries through non-shell `CreateProcessW`, drain stdout/stderr,
  terminate timed-out children, and surface structured failure/exit results;
- document the remaining persistent worker/RPC boundary instead of treating a
  one-shot CLI process as a full Salamander-aware runtime.

The transport foundation is covered by a standalone `/W4 /WX` test for partial
frames, round trips, malformed ids, newline rejection, and the 1 MiB limit.

The eighth code slice is now implemented:

- add `IRuntimeSession` and optional `IRuntimeAdapter::StartPersistent()`;
- implement bidirectional persistent process sessions with bounded frame reads,
  CRLF-tolerant worker output, safe stop/Release cleanup, and timeout-aware
  liveness checks;
- connect manifest lifecycle activation/deactivation to session ownership;
- start and join a bounded host pump thread so worker frames are processed in
  normal plugin operation;
- verify a Python echo worker end to end through `SMX1` frames.

This does not pretend that modern Python or PowerShell support is complete. It
creates the contract those adapters need and removes the current architectural
assumption that every script file must map directly to an `IActiveScript` COM
CLSID.

The ninth code slice is now implemented:

- add a host-dispatch callback to the persistent runtime request;
- bind `runtime.ready`, `salamander.commands.execute`,
  `salamander.sides.activeTab`, and string `salamander.storage.get/set` calls;
- bind event subscribe/unsubscribe and push matching `event` frames, plus a
  parented message-box call;
- ship language-specific bootstrap scripts that expose the same `Salamander`
  object model and route calls/events over SMX1;
- add bounded JSON string-member extraction tests and verify the Automation
  plugin plus the Python worker round trip.
