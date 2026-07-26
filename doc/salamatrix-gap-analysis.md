# Salamatrix framework gap analysis

Status date: 2026-07-26

Source vision: <https://krtkovo.eu/salamander/framework.html>

This audit compares the framework vision with the implementation in this
repository. It distinguishes working code from API placeholders, samples, and
catalog metadata. The target is not to reproduce the complete native Plugin SDK
in scripting languages. It is the middle extensibility layer described by the
vision: persistent lightweight extensions with commands, UI, events, storage,
and a shared Salamander object model.

Status labels in the matrix are deliberately requirement-level, rather than
commit-level: `Implemented` means the scoped vision item is covered by working
code and a focused test; `MVP` means the public slice works end-to-end but the
vision still calls for a broader surface; `Partial` means substantial pieces
exist but the current slice is not yet a coherent user-facing feature;
`Missing` means there is no implementation of the requested surface yet.

## Executive summary

The repository already contains a real Salamatrix foundation, not only a design:

- `SALAMATRIX.SPL` is a load-on-start Automation Framework provider.
- The core has a process-local, versioned service registry with provider
  ownership and short consumer leases.
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
- Manifest extensions carry their package SVG identity icon (plus an optional
  dark-mode variant) into the existing Plugin Manager list and toolbar image
  lists. Every manifest command may override those two SVG assets for its own
  toolbar button; no separate Extension Manager is introduced.

This is an MVP/PoC, not yet the framework described by the vision. In particular:

- Automation keeps the legacy ActiveScript compatibility path, while the
  standalone JavaScript/Node, CPython, PowerShell, and PHP CLI `.spl` providers
  use the shared SMX1 worker bootstrap and host-call/UI bindings. Dependency
  environments, richer values, and long-lived event queues remain.
- JavaScript has both the legacy Windows JScript fallback and an independent
  modern Node adapter. Python, PowerShell, and PHP have independent process
  adapters; bundled interpreters and managed dependency environments remain
  optional future work.
- The first modern side/tab model is exposed through the native SDK,
  Salamatrix, and Automation without raw core pointers. Item snapshots,
  selection/focus mutation, view settings, tab creation/closing, and change
  events remain broader follow-up work.
- Modern manifest-backed scripts run through the shared worker bootstrap;
  discovery registers the complete batch first, resolves declared extension
  dependencies in bounded passes, and retries activation after a later
  runtime/plugin configuration change. One-shot scripts still cannot remain
  alive after their execution ends. If a required separately installed
  runtime or manifest dependency is missing, discovery keeps the extension in a
  visible waiting state and reactivates it after the requirement appears.
- The strict schema-1 manifest parser validates package/runtime identity,
  minimum runtime version, safe entry points, package-owned and per-command
  `icon`/optional `iconDark` SVG paths, capabilities, an optional event allow-list, typed
  settings/defaults, dependencies, and multiple command records. Dependency
  declarations are resolved against the owner-aware extension registry and
  unresolved packages stay visible as waiting-for-dependency. Package-owned
  BCP-47 locale resources may localize the extension name, declared command
  titles, and setting presentation text; settings migrations/files and richer handler
  lifecycle metadata are still missing.
- Persistent workers may register multiple commands. The selected command ID
  now follows menu activation through the native host and external runtime
  bootstrap into `Salamander.command_id`, so one entry point can dispatch
  distinct handlers without relying on the first command as a fallback. The
  selected manifest handler is also exposed as `Salamander.command_handler`.
- Manifest packages with multiple command records now publish every declared
  command into the existing Plugin Manager/menu/toolbar path when their
  persistent worker activates; handler dispatch remains explicit in the
  worker via the command context.
- Runtime and manifest commands now carry append-only `enabled`/`visible`
  state metadata. All four modern workers can update that state through the
  shared `setState` host call, and the existing Plugin Manager/menu/toolbar
  path refreshes without adding a public vtable method.
- Persistent process sessions now publish richer lifecycle diagnostics without
  changing the session ABI: cached process id/exit code, explicit host stop,
  failed nonzero exits, and bounded diagnostic messages are available in the
  Automation bridge and all four standalone providers.
- The shared native UI service exposes progress, input-box, and a reusable
  `IDialog`/`IControl` contract for modern workers and native plugins. The
  native implementation now creates Label, TextBox, CheckBox, RadioButton,
  ComboBox, Button, ListView, TreeView, TabControl, FolderPicker, and the
  editable FilePicker controls with item/node binding, columns, explicit
  layout, notifications, validation, pickers, clipboard, and dark-mode
  support. DPI/accessibility polish and migration of legacy Forms wrappers
  remain.
- `Salamatrix.AI` has a provider-neutral contract. The standalone
  `SalamatrixAI.SPL` registers the optional bounded local command provider and
  owns an interactive chat window; Automation remains only a consumer for its
  legacy Ask-AI flow. The provider exchanges one bounded JSON request/response
  and enforces the two-minute ceiling; bundled inference remains optional.

## Requirement matrix

| Vision area | Status | Current implementation | Missing work |
| --- | --- | --- | --- |
| Three extensibility levels | MVP | Native plugins, one-shot Automation scripts, and persistent manifest-backed workers now share an owner-aware lifecycle catalog; registration activates persistent workers and the host-call dispatcher binds commands, sides, storage, event subscriptions, and UI calls. Extension host callbacks now hold owner-aware unload leases. The append-only `IRuntimeSession::GetDiagnostic` contract reports bounded running/stopped/exited/failed lifecycle state, cached process id/exit code, and diagnostic text without exposing process handles. | Provider-specific startup failure capture, unload timelines, and richer host-facing lifecycle diagnostics; no separate Extension Manager is planned. |
| Shared cross-runtime API | MVP | The Salamatrix service layer, versioned ABI, SMX1 transport, and Python/PowerShell/PHP/JavaScript worker facades use the same host method vocabulary, including commands, sides, file operations, storage, events, clipboard, UI, and AI. Storage now preserves string, boolean, and signed 64-bit integer values across the host boundary. | Broader runtime-neutral value model, complete object model, and formal error/threading rules. |
| Left/Right/Source/Target sides | MVP | `Salamatrix.Sides` and all modern workers resolve all four references and expose active tabs, bounded path/type, selected-item snapshots, focused-item metadata, item name/path/extension/size/attributes/UTC write time and hidden/link/offline/size-valid flags, and active/source/target/locked/detached flags without raw core pointers. Modern workers now also enumerate tabs, activate a tab, change the active-side path, request a side refresh, select individual/all items, and move focus by stable panel index. | View mode/tree state and item change events. |
| Tabs and detached windows | MVP | SDK snapshots and opaque process-local ids expose tab count, index, path/type, active/source/target/locked/detached flags, activation, path changes, refresh, selection/focus, and snapshot-derived lifecycle events. Sides schema 1.3 now publishes append-only create/close/reorder/move/detach methods, the host bridge validates decimal tab ids and side/index inputs, and all four worker facades expose the same contract. Isolated process-runtime coverage uses an explicit SALAMATRIX_WORKER_ROOT built from provider workers. | Direct core load/version negotiation, colors, richer detached-window operations, and persistence semantics. |
| Existing Salamander commands | MVP | The stable catalog now covers view/edit/open, rename, copy/move, email/delete/properties, case/attribute/space operations, refresh, directory creation, drive info, directory sizes, and disconnect; execution still uses Salamander's normal enablement and dialogs. | Parameterized command calls, richer synchronous/modal results, state/change notifications, and non-modal operation handles. |
| Programmatic file operations | MVP | `IFileOperationsService` and all modern worker bindings expose interactive rename/copy/move/delete/create-directory/refresh/properties workflows. | Typed source/target values, progress/cancellation handles, and final structured results without requiring the native dialog. |
| Extension command registration | MVP | Discovery-time metadata gives a script stable identity/caption/placement hints, and a persistent worker can register/unregister multiple owner-scoped commands with synthetic native menu ids, context masks, native menu hotkeys, toolbar contributions, optional handler names, and append-only enabled/visible state. Manifest command records are now published automatically with their initial state, while worker `setState` updates the existing Plugin Manager/menu/toolbar path and removal triggers a menu/toolbar rebuild. Each selected runtime command propagates its ID and handler into the worker as `Salamander.command_id`/`Salamander.command_handler`, so one entry point can dispatch distinct handlers. | Enable/visible callbacks, command palette integration, richer handler lifecycle metadata, and richer ownership/unload leases. |
| Plugin and context menu placement | MVP | Metadata booleans and context masks are applied to Automation menu items and persistent registrations. | Independent placement contributions, icons, and dynamic menu APIs beyond the current MVP surface. |
| Toolbar and shortcuts | Partial | Dynamically registered extension commands now pass Salamander hotkeys through the normal menu-extension path, contribute toolbar buttons, and honor per-command enabled/visible state. Native/runtime registrations use core-owned DPI-aware image lists; manifests may provide package-wide SVG `icon` and optional dark-mode `iconDark`, or override them on each `commands[]` item. If the selected `iconDark` is missing or invalid, the core generates a dark-friendly raster variant from the normal icon and also creates the disabled/gray image. Dynamic placement is now serialized with stable extension/plugin keys instead of transient runtime ids. | Explicit placement/conflict UX, command palette integration, richer state callbacks/change notifications, and richer toolbar metadata. |
| Events | MVP | Salamatrix.Events maps host lifecycle/settings/configuration/color/panel events, successful shared-Sides path/selection/tab/refresh operations, core path/selection/tab notifications, and core filesystem-change notifications (fileChanged, including the affected path and recursive-scope flag) to unsubscribe-safe native callbacks; successive tab snapshots add tabCreated, tabClosed, tabReordered, windowDetached, and windowAttached with changedTabId, tabIndex, and previousTabIndex, delivered before legacy tabChanged. Automation exposes subscribe/unsubscribe with copied payloads and preserves the V1 payload prefix. Manifest-backed extensions may declare an event allow-list, which the host enforces before creating a subscription. Persistent worker sessions now enqueue bounded event frames instead of writing from the core callback directly into a potentially back-pressured pipe. | Typed create/rename/delete classification, UI-thread marshalling for richer event payloads, coalescing, event replay, richer direct window lifecycle hooks, and unload-safe leases across modern runtimes. |
| Per-extension storage | Implemented | `Salamatrix.Storage` persists isolated manifest-id namespaces with UTF-8 strings, signed 64-bit integers, booleans, delete/clear, validation, and synchronized access. Manifest packages may declare typed settings with optional defaults; the host materializes missing defaults without overwriting user values and exposes `storage.schema()` to every modern worker. The SMX1 host dispatcher and Python/PowerShell/PHP/JavaScript workers preserve all three value types instead of coercing settings to strings. The Plugin Manager can now edit the declared scalar settings through the shared UI service. Legacy global persistence remains for compatibility. | Settings files/enumeration, quotas, migrations, transactional batches, and uninstall retention/deletion policy. |
| Shared UI framework | MVP | `Salamatrix.UI` provides a reusable native `IDialog`/`IControl` model and `NativeDialog` implementation for labels, text boxes, check/radio buttons, combo boxes, buttons, native ListView/TreeView/TabControl controls, a dialog-embedded `FolderPicker`, and an editable `FilePicker` with a UTF-8 edit value plus a wide Win32 browse button. It supports item binding, explicit bounds, dialog width/height, columns and selection, required validation, control-change events, common `readOnly`/`checked`/`dialogResult`/`keepOpen`/`multiline` options across native and Python/PowerShell/PHP/Node workers, file-picker `filter`/`save` options, message/input boxes, non-modal auto-dismissing `ui.notify`, UTF-8 file/folder pickers, clipboard copy, dark-mode initialization, keyboard-tab traversal with initial focus, and per-monitor `WM_DPICHANGED` child/layout scaling through the existing dynamic DPI helper. Progress covers Salamander's one-bar/file-progress and two-bar modes, totals/positions, stepping, text, cancellation, and cleanup; all workers expose the same calls. A standalone UI layout contract test covers picker split metrics and 100%/150% DPI scaling in both directions. | Richer notification actions/stacking, virtualized data, accessible names/roles and screen-reader audit, interactive per-monitor GUI testing, reentrancy policy, and migration of legacy Forms wrappers. |
| Manifest/package | MVP | Strict UTF-8 JSON parsing validates schema 1, package/runtime identity and minimum version, safe entry point, package-owned SVG `icon`/`iconDark` assets, optional command-specific SVG `icon`/`iconDark` overrides, capabilities, typed settings declarations/defaults, bounded presentation metadata (`label`, `description`, `group`, `order`, `width`, `multiline`), up to 32 extension dependencies, up to 32 BCP-47 `locales` entries, and up to 64 command records with initial `enabled`/`visible` state. A selected package locale may provide `name`, declared command-title translations, and localized setting label/description/group metadata; persistent packages publish up to 16 declared commands to the native menu/toolbar surface and pass each selected command's declared handler and state to the worker. Ask-AI can save a validated package and refresh discovery. Missing separately installed runtime adapters and unresolved extension dependencies are represented explicitly during discovery instead of failing silently. | Settings migrations/files, handler lifecycle metadata, package install/uninstall, and richer diagnostics/dependency prompts. |
| Runtime adapters | MVP | `Salamatrix.Runtime` registers/enumerates versioned adapters and executes manifest entry points through structured requests/results. Automation registers only legacy ActiveScript adapters; modern CPython, PowerShell, PHP CLI, and Node runtimes are supplied through independent `.spl` providers using the same broker. All four standalone providers now pass Debug and Release x64 project builds, with deferred registration when Salamatrix loads later. The isolated process-runtime contract test now runs with an explicit `SALAMATRIX_WORKER_ROOT`, exercises Python/PowerShell/PHP host-worker lifecycle and UI calls, and verifies `GetDiagnostic` running/stopped/exited/failed state, cached process id/exit code, and diagnostic message. Manifest publication checks the requested adapter and minimum version, marks unavailable extensions as waiting, and retries them after a broker refresh. | Direct standalone `.spl` load test through Salamander, startup failure capture before a session exists, cancellation beyond process termination, debugging, dependency environments, and richer dependency policy. |
| JavaScript runtime | Process MVP | Windows JScript remains the compatibility fallback for legacy `.js`; `JavaScriptRuntime.SPL` now discovers Node via `SALAMATRIX_NODE`, `node.exe`, or `node`, owns the dependency-free SMX1 `.mjs` worker, and registers `JavaScript.Node` independently of Automation. The worker now exposes the same host API surface as Python/PowerShell/PHP, including convenience dialog controls, native dialogs, and event subscriptions. Debug and Release x64 builds are verified. | Explicit `.js`/`.mjs` precedence with the legacy fallback, package deployment policy, and richer value bindings. |
| Python runtime | Process MVP | ActivePython COM compatibility remains; `Python.CPython` discovers `python.exe`/`python3.exe` or `SALAMATRIX_PYTHON`, runs `.py` entries out of process, and the optional worker bootstrap exposes the shared SMX1 `Salamander` API including multiple command registration. A standalone `PythonRuntime.SPL` project owns the same adapter/worker, unregisters it through the broker, and now passes a Debug x64 build. | Richer UI/value bindings, environment/dependency policy, and bundled runtime. |
| PowerShell runtime | Process MVP | `PowerShell` discovers `pwsh.exe`/`powershell.exe` or `SALAMATRIX_POWERSHELL`, runs `.ps1` entries out of process, and ships the same SMX1 worker bootstrap/API shape including multiple command registration. A standalone `PowerShellRuntime.SPL` project owns the adapter/worker and broker registration path and now passes a Debug x64 build. | Richer UI/value bindings, environment policy, cancellation/error mapping, and bundled runtime. |
| PHP runtime | Process MVP | Legacy PHPScript remains; `PHP.CLI` discovers `php.exe` or `SALAMATRIX_PHP`, runs `.php` entries out of process, and ships the same SMX1 worker bootstrap/API shape including multiple command registration. A standalone `PHPRuntime.SPL` project owns the adapter/worker and broker registration path and now passes a Debug x64 build. | Richer UI/value bindings, dependency policy, and support decision/bundled runtime. |
| Extension management | Partial | The existing Plugin Manager remains the single management surface: it shows ordinary `.SPL` plugins and now appends discovered manifest-extension rows from `Salamatrix.Extensions` with name, state, version, runtime, entry point, and manifest ID details. Extension rows use the package SVG icon, prefer `iconDark` in dark mode, and derive a dark-friendly bitmap from the normal icon when the dark asset is absent or invalid. Missing runtime adapters are shown as `runtime unavailable`; unresolved manifest dependencies are shown as `dependency unavailable`; a user-deactivated extension is explicitly shown as `disabled`. Its existing Activate/Deactivate action persists the enabled state in the extension's isolated Storage namespace, stops the worker before disabling, and reactivates it after re-enabling. Configure opens a `Salamatrix.UI` native dialog for up to 24 declared string/integer/boolean settings, uses localized labels and visible groups, applies bounded width/multiline layout metadata, and writes typed values into the extension's isolated Storage namespace. Rows are still not treated as loadable `.SPL` files. No separate Extension Manager is planned. | Remove, package install/uninstall, settings help/tooltips and richer layout semantics, more than 24 settings, and richer runtime/status details in the existing Plugin Manager. |
| Permissions/capabilities | MVP | Manifest capabilities are retained by Automation and enforced at the SMX1 host boundary for declared-capability extensions (`panels.read`, `panels.write`, `ui.dialogs`, `commands`, `file-operations`, `storage`, `events`, and `ai`); panel path/refresh/selection/focus mutations require `panels.write`, while legacy scripts without a capabilities list remain compatible. | Explicit user grant/revocation UI, capability-specific filesystem/network/process policy, effect audit trail, and richer package UX. |
| AI automation assistant | MVP | `Salamatrix.AI` provides provider registration, structured generation validation, parsed effect flags, runtime/existing-script/feedback hints, a safe `generate`/`preview` seam, and focused `Salamander.ai.api(topic)` retrieval. The separately installable `SalamatrixAI.SPL` owns the bounded local command provider, an optional native WinHTTP local model provider (`SALAMATRIX_AI_MODEL` plus Ollama `SALAMATRIX_AI_OLLAMA_URL`/`SALAMATRIX_AI_HTTP_URL` or OpenAI/llama.cpp `SALAMATRIX_AI_LLAMA_URL` with `SALAMATRIX_AI_PROTOCOL=chat-completions`), and an interactive multiline chat with Ask/Preview/Run/Save actions; follow-up prompts carry the previous script as repair context. The chat now enumerates available providers, shows ready/unavailable status, and supports explicit selection or automatic fallback. Its dynamic Menu Extension publishes a localized command for opening the chat, and its shutdown path validates the borrowed AI service identity before unregistering providers so framework-first unload cannot dereference a stale pointer. A shared static validator checks response shape, capabilities, effects, runtime identity, and undeclared external/network operations; bounded `GenerateWithRepair` is used by the standalone chat and both Automation consumers. When Automation is loaded, `Salamatrix.ScriptRunner` routes Run through the same capability-aware SMX1 host dispatcher as regular extensions. Automation's Ask-AI flow remains a compatibility consumer with context capture, preview/copy, Run/Save As, repair feedback, and validated extension-package save. The full API description now also carries live native service contract versions, the currently registered runtime adapter inventory, and schema fragments supplied by the native command, file-operation, UI, sides, events, storage, extensions, and runtime contracts. | Bundled llama.cpp inference/model distribution, complete method/field schema coverage without hand-maintained fragments, richer semantic validation, and remote providers. |
| Testing | Initial | Standalone `/W4 /WX` tests cover the strict manifest parser (including localized setting metadata, layout validation, and initial command state), Storage persistence, Events subscribe/publish/self-unsubscribe/capacity/payload validation, Extensions registration/lifecycle/ownership and legacy setting-schema prefixes, the local owned-service registry, runtime raw-JSON context and AI service contracts (including focused API slices), and integration tests for Python/PowerShell/PHP process execution, output capture, timeout, editable file-picker protocol calls with flat `filter`/`save` options, command enabled/visible state changes, persistent-session lifecycle diagnostics for running/stopped/exited/failed workers, and deterministic UI layout/DPI metrics. The explicit `SALAMATRIX_WORKER_ROOT` run uses isolated copies of provider worker assets and passes without starting Salamander. The core Salamander Debug build compiles the owner-aware `CSalamanderGeneral` registry. Worker syntax checks cover Python, PowerShell, PHP, and Node; provider project/XML audits cover all four standalone packages. A fast source-contract regression test now locks the Plugin Manager labels, AI dynamic-menu command, stale-service shutdown guard, and core dynamic-menu call chain. | Direct core-registry runtime tests, interactive native UI tests, extension fixtures, lifecycle/unload integration tests, startup-failure fixtures, and end-to-end Salamander API adapter tests. |

## Current verification snapshot

The newest implementation commits are `4a380ef3c` (editable native
`FilePicker` and standalone worker parity), `89a1d4fbf` (isolated process
runtime verification), `a6a8cedc8` (Plugin Manager settings configuration),
`9aa3cbf84` (per-command SVG toolbar artwork), and `6e1711cd8` (dialog
`FolderPicker`). They were compiled into isolated Debug x64 output
directories; the user's locally running Salamander process was never started,
stopped, or otherwise controlled by this work.

- `a6a8cedc8`: Automation, Salamatrix, the core Salamander executable, and
  Extensions native tests passed.
- `9aa3cbf84`: strict manifest parser tests and Automation Debug x64 build
  passed.
- `6e1711cd8`: Salamatrix, Automation, and all four standalone runtime
  provider Debug x64 projects passed; runtime-protocol tests passed; Python,
  PowerShell, PHP, and Node worker syntax checks passed.
- The Python/PowerShell/PHP process-runtime integration executable built, but
  its invocation was previously skipped because this environment did not set
  `SALAMATRIX_WORKER_ROOT`. That gap is now closed: on 2026-07-26 the
  `build\verification\folder-picker\process-tests\processruntime_tests.exe`
  executable passed with exit code 0 when `SALAMATRIX_WORKER_ROOT` pointed to
  the isolated `build\verification\process-runtime-worker-root` containing
  only copied Python, PowerShell, and PHP bootstrap files. The run exercised
  interpreter discovery, one-shot and persistent SMX1 sessions, host calls,
  typed storage, commands, events, clipboard, file/folder pickers, native
  dialog controls, shutdown, output capture, and timeout coverage without
  starting or controlling Salamander.

At this pause point all Salamatrix source and documentation changes are
committed. The pre-existing worktree-only `.gitignore` change keeps the two
syntax-check artifacts out of source control:
`src/plugins/automation/runtime/__pycache__/` and
`src/plugins/pythonruntime/runtime/__pycache__/`. They are not source changes
and are not staged.

On 2026-07-26, the append-only lifecycle diagnostic contract was verified by
an isolated Debug x64 rebuild of the process test, protocol test, and all four
standalone provider projects. With `SALAMATRIX_WORKER_ROOT` set to
`build\verification\runtime-diagnostics\worker-root`, the process-runtime
executable passed with exit code 0; its one-shot worker reported `running`,
then `exited` with exit code 0. The run also exposed stale typed-storage/schema
methods in the standalone Python worker, which were brought back to parity
before the passing rerun.

The UI slice was verified on 2026-07-26 with an isolated Debug x64
`Salamatrix.SPL` rebuild and the standalone `ui_layout_tests.exe`. The test
passed picker geometry checks plus 96-to-144 DPI and 144-to-96 DPI scaling
checks. No GUI Salamander process was started or controlled.

## Existing implementation evidence

### Core service and plugin integration

- `src/plugins/shared/spl_gen.h` declares `RegisterService`,
  `UnregisterService`, `QueryService`, and the appended owner-aware
  register/unregister/acquire/release methods. Its appended toolbar contract
  accepts package-owned SVG paths (normal plus optional dark-mode variant),
  while the core renders those files through NanoSVG into the shared toolbar
  image lists. When the optional dark asset is absent, the core derives a
  dark-friendly bitmap from the normal SVG while preserving alpha and color
  information.
- `src/zip.cpp` contains the fixed-size process registry, its critical-section
  guard, lease table, unload gate, and condition-variable wait for active
  consumers.
- `src/plugins/shared/spl_base.h` defines
  `FUNCTION_AUTOMATIONFRAMEWORK`.
- `src/plugins/salamatrix/salamatrix.cpp` creates persistent
  `RuntimeServices`, registers the provider, and requests load on startup.
- `src/plugins/salamatrixai/` is a separately buildable/installable provider
  plugin. It registers `local.command` plus the optional `local.ollama` WinHTTP
  provider under `Salamatrix.AI` and exposes the shared native chat dialog; it
  is not a dependency of Automation.
- Automation additionally publishes the optional `Salamatrix.ScriptRunner`
  service. This is the non-privileged bridge used by the AI plugin to execute a
  saved script through the ordinary extension host dispatcher.
- `src/vcxproj/salamand.sln` builds Salamatrix as part of the main solution.

### Runtime provider packaging boundary

The runtime broker is already a host service rather than an Automation-private
API: `IRuntimeService::RegisterAdapter` and `UnregisterAdapter` accept adapter
objects from any loaded plugin, and runtime descriptors are enumerated through
the same service by native callers and workers. The standalone
`PythonRuntime.SPL`, `PowerShellRuntime.SPL`, `PHPRuntime.SPL`, and
`JavaScriptRuntime.SPL` projects now own their provider adapters, interpreter
discovery, and worker bootstrap. Their standalone Windows projects pass
isolated Debug x64 builds, and the worker package gate verifies their
bootstrap/export boundary without launching Salamander. Final loading and GUI
behavior still require the target machine; Automation retains only its legacy
ActiveScript engines and consumes the broker like every other plugin.

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
- The standalone runtime provider projects own bounded out-of-process
  CPython, PowerShell, PHP CLI, and Node adapters; Automation retains only
  transitional source/test implementations while consuming the broker.
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

1. The core service registry still exposes borrowed interface pointers to
   callers, but provider ownership and short consumer leases now prevent new
   acquisitions during unload and wait for active calls before removal. The
   extension lifecycle registry has the same owner-aware callback behavior;
   unload diagnostics are still minimal.
2. Runtime provider registration is still an explicit plugin-lifetime contract;
   unload diagnostics and a user-facing dependency view remain.
3. Posted Commands/FileOperations report `ok` when a command was accepted, not
   when the modal operation completed successfully. `cancel` is not observable.
4. The command catalog currently maps both `Move` and `MoveRename` to the same
   native command and contains only three unique operations.
5. The manifest parser retains every validated command and the Automation menu
   surface iterates all registered runtime command contributions; remaining
   work is richer independent placement/toolbar metadata rather than dropping
   additional commands.
6. Discovery and execution now go through runtime adapters. The v1
   compatibility callback still keeps old ActiveScript hosting inside
   Automation; modern process cancellation is currently stop/termination based.
7. Long-lived script callbacks still require richer UI-thread, reentrancy,
   cancellation, and exception rules; host callbacks now reject new calls
   during extension deactivation and hold an unload lease while active.
8. A language process must never receive raw C++/COM implementation pointers.
   Out-of-process adapters need a versioned RPC/value protocol, while
   in-process adapters need ABI-stable interfaces.

## Recommended implementation order

### Phase 0: make the foundation safe

1. Route extension discovery and execution through the existing
   `Salamatrix.Runtime` broker. **Implemented.**
2. Add provider ownership/leases, diagnostics, and lifecycle rules to runtime
   and core services. **Provider ownership and short leases are implemented;
   richer diagnostics remain.**
3. Replace the manifest text scanner with a real parser and validate a versioned
   schema.

### Phase 1: complete the useful common object model

1. Publish `Salamatrix.Sides` with Left, Right, Source, and Target.
   **Implemented.**
2. Add side and tab snapshots/handles: path, active tab, tabs, focused/selected
   items, view mode, tree visibility, detached state. **Implemented for tab
   identity, enumeration, path, activation, active/source/target/locked/
   detached state, side path changes, refresh, selection mutation, and focus;
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
   settings/configuration/colors, panel swap, active-panel events, successful
   shared-Sides path/selection/tab/refresh operations, and core
   path/selection/tab notifications; persistent instance queues now use
   bounded worker-session frames. File-operation and window lifecycle hooks
   remain.**
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

The tenth code slice is now implemented:

- add stable toolbar contribution keys to the appended native toolbar ABI;
- persist extension toolbar placement in the existing toolbar configuration,
  while retaining automatic defaults for older configurations;
- resolve persisted entries by manifest/plugin identity rather than transient
  process-local command ids;
- derive a dark-mode bitmap from the normal extension SVG when an extension
  does not ship a separate `iconDark` asset.
- keep icon rendering and placement shared between native plugins and all
  runtime-backed extensions.

The eleventh code slice is now implemented:

- connect the AI service to the live runtime broker;
- publish versions of all native Salamatrix service contracts in the machine-
  readable API description;
- publish the current runtime adapter inventory (id, language, extensions,
  version, and availability) for prompt construction;
- cover the generated metadata with the runtime protocol test target.

The twelfth code slice is now implemented:

- let the native command, file-operation, UI, sides, events, storage,
  extensions, and runtime services publish compact schema fragments next to
  their actual contracts;
- aggregate those fragments into the AI service's machine-readable schema;
- validate the resulting `nativeContracts` JSON object in the runtime protocol
  tests.

The thirteenth code slice is now implemented:

- add a common AI response validator for capabilities, estimated effects,
  runtime identity, and undeclared external/network operations;
- expose the validator and a bounded `GenerateWithRepair` loop through the
  provider-neutral AI ABI, preserving compatibility defaults for older services;
- use the same repair-aware generation path in the standalone Salamatrix AI
  chat and both Automation AI consumers;
- cover valid, unsupported-capability, and unsafe-script responses in the
  runtime protocol tests.

The fourteenth code slice is now implemented:

- extend schema-1 manifests with an optional `events` declaration;
- validate event names and reject duplicates during strict manifest parsing;
- enforce the declared allow-list at the persistent worker host boundary while
  preserving unrestricted event subscriptions for legacy manifests and
  generated one-shot AI scripts;
- cover declared, absent, unknown, and duplicate event declarations in the
  manifest parser tests.

The fifteenth code slice is now implemented:

- represent an unavailable requested runtime with the ABI-safe
  `ExtensionFlagRuntimeUnavailable` flag and `ExtensionStateWaitingForRuntime`;
- have Automation check the shared runtime broker for the adapter and minimum
  version while publishing manifest extensions;
- keep waiting extensions visible in the existing Plugin Manager and annotate
  their runtime/function status without offering an install flow;
- make activation a safe no-op while the dependency is absent, then clear the
  waiting state and activate through the existing same-owner refresh when the
  standalone runtime plugin registers;
- cover registration, blocked activation, and reactivation in the extension
  lifecycle test.

The sixteenth code slice is now implemented:

- retain the selected runtime command ID in the Automation execution context;
- pass that ID through the Python, PowerShell, PHP, and Node worker bootstrap
  command lines for both the Automation compatibility broker and standalone
  runtime providers;
- expose it consistently as `Salamander.command_id` (with JavaScript's
  camel-case alias `commandId` as well);
- publish the field in the AI/API description so generated extensions can
  dispatch manifest or dynamically registered handlers explicitly;
- cover the end-to-end Python one-shot path and re-run all worker syntax and
  provider Debug builds.

The seventeenth code slice is now implemented:

- retain all validated manifest command records (up to the native runtime
  command limit) instead of applying only the first record;
- publish each declared command with its own plugin/context/toolbar placement
  and `requires` event masks when the persistent extension activates;
- keep the manifest command surface stable across worker teardown and restore
  its default command metadata on the next activation;
- allow the worker to add or remove dynamic commands alongside manifest-owned
  commands without losing the package identity;
- preserve the selected command context for all of these entries through the
  existing `Salamander.command_id` worker field.

The eighteenth code slice is now implemented:

- extend the versioned runtime request with an optional `CommandHandler` field;
- resolve the selected manifest command's declared `handler` by command ID;
- pass that handler through the Automation broker and all four standalone
  worker providers using the bounded `--command-handler`/`-CommandHandler`
  bootstrap argument;
- expose the value consistently as `Salamander.command_handler` (plus
  JavaScript's camel-case alias) and cover the Python one-shot path in the
  process-runtime integration test.

The nineteenth code slice is now implemented:

- retain an optional handler name on every native runtime command record;
- extend `Salamander.commands.register(...)` in Python, PowerShell, PHP, and
  JavaScript with the common optional handler argument;
- persist dynamic handler names through native command activation and expose
  the selected value through the same `Salamander.command_handler` field;
- publish the handler field in the live AI/API command schema and verify the
  worker-to-host registration payload in the Python bootstrap integration test.

The twentieth code slice is now implemented:

- add the ABI-safe `fileChanged` event kind without extending the events vtable;
- forward Salamander core `AcceptChangeOnPathNotification` callbacks through
  Salamatrix with the UTF-8 affected path and recursive-scope flag;
- expose the event consistently to modern workers, Automation, manifest event
  allow-lists, and the live AI/API schema;
- cover subscription acceptance and path/scope delivery in the native Events
  test suite.

The twenty-first code slice is now implemented:

- add the append-only UI 1.1 notification contract and native timed popup;
- expose `Salamander.ui.notify(message, title, timeout)` consistently to
  Python, PowerShell, PHP, JavaScript, native Automation adapters, and the
  live AI/API schema;
- keep notification calls non-modal and bounded (default five seconds,
  maximum ten minutes) while allowing click-to-dismiss;
- cover the worker-to-host notification call in the Python bootstrap test.

The twenty-second code slice is now implemented:

- validate an optional manifest `dependencies` array (bounded, identifier-only,
  case-insensitive unique ids) and retain it with the discovered script;
- register the complete manifest batch before activation, resolve dependency
  chains in bounded passes, and represent unresolved dependencies with the
  ABI-safe `waitingForDependency` state/flag;
- make activation a safe no-op while a dependency is absent and surface the
  reason in the existing Plugin Manager alongside runtime-unavailable status;
- cover dependency parsing, blocked activation, and same-owner reactivation in
  the manifest and Extensions test suites.

The twenty-third code slice is now implemented:

- append the Extensions 1.1 `disabled` state and `SetExtensionDisabled` API
  without changing the existing descriptor layout;
- persist the manifest extension's enabled choice in its isolated
  `salamatrix.enabled` Storage key, where no key remains backwards-compatible
  as enabled;
- make Plugin Manager Deactivate stop the worker first, then persist and show
  `disabled`; Activate clears that state and starts the ordinary lifecycle;
- cover disabled registration, safe no-op activation, re-enable, and
  re-disable transitions in the Extensions lifecycle test.

The twenty-fourth code slice is now implemented:

- append the ABI-safe Extensions 1.2 settings-schema contract with bounded
  string/integer/boolean setting records, without changing the descriptor
  layout;
- publish manifest setting declarations from Automation after each successful
  extension registration;
- use the shared `Salamatrix.UI` dialog/control API from Plugin Manager's
  Configure action to edit typed per-extension values (with a deliberate
  24-setting fixed-layout limit);
- cover schema registration, retrieval, duplicate rejection, and the
  Extensions 1.2 service build alongside Automation, Salamatrix, and core
  Salamander Debug x64 builds.

The twenty-fifth code slice is now implemented:

- allow every manifest `commands[]` record to declare a package-contained SVG
  `icon` and optional dark-mode `iconDark` asset;
- validate the command artwork with the same safe-relative-path and SVG-only
  rules as package artwork, resolve it during discovery, and prefer it for its
  specific toolbar button;
- preserve package artwork as the compatibility fallback and retain core dark
  icon generation when no valid dark asset is supplied;
- cover valid and invalid command artwork through the strict manifest parser
  test and build Automation Debug x64.

The twenty-sixth code slice is now implemented:

- add a native `FolderPicker` control to `Salamatrix.UI` dialogs; it opens the
  standard folder browser, returns its selected UTF-8 path through `IControl`,
  and raises the standard control-change event;
- expose it to every modern worker facade as Python `add_folder_picker`,
  PowerShell `AddFolderPicker`, PHP `addFolderPicker`, and Node
  `addFolderPicker`, with the raw runtime protocol kind `folderpicker` also
  accepted;
- include the control in the runtime and AI API schemas so the local script
  helper can generate it;
- build Salamatrix and Automation Debug x64, run the API protocol test, and
  syntax-check all four runtime worker implementations. The process-runtime
  integration run was completed later with an explicit isolated
  `SALAMATRIX_WORKER_ROOT`; its result is recorded in the twenty-seventh slice
  below.

The twenty-seventh code slice is now implemented:

- add the append-only `filepicker` control kind to the shared native dialog;
  its edit control keeps a UTF-8 path value while the adjacent browse button
  opens the wide Win32 file dialog with heap-backed path storage rather than a
  new `MAX_PATH` dependency;
- expose `add_file_picker`/`AddFilePicker`/`addFilePicker` in the Python,
  PowerShell, PHP, and Node workers and publish the control in the runtime and
  AI API schemas;
- extend the host mapping and the cross-runtime process test to cover the new
  control for Python, PowerShell, and PHP;
- fix typed-storage/schema drift in the standalone PowerShell and PHP worker
  assets, which the isolated provider-root test exposed after the first
  attempt;
- rebuild Salamatrix, Automation, all four standalone provider projects, the
  runtime protocol test, and the process-runtime test into
  `build\verification\editable-file-picker`; the protocol test and the
  process-runtime test both passed, with the latter returning exit code 0
  under an explicit isolated `SALAMATRIX_WORKER_ROOT`.

The twenty-eighth code slice is now implemented:

- extend manifest setting declarations with validated label, description,
  group, order, width, and multiline metadata while keeping the storage key
  stable;
- allow selected package locale resources to translate setting presentation
  text in addition to extension names and command titles;
- append presentation fields to `ExtensionSettingInfo`, but accept and return
  the legacy 1.2 prefix by honoring caller `StructSize`, preserving the
  existing ABI and service vtable;
- publish the metadata through `storage.schema()` and the existing Plugin
  Manager's Configure dialog, including localized labels/groups and bounded
  multiline/width layout;
- cover parser validation, localized settings, the new registry fields, and
  legacy setting-schema callers. The isolated parser/registry tests and
  Automation plus Salamatrix Debug x64 builds passed under
  `build\verification\manifest-settings`; no Salamander process was started
  or controlled.

The twenty-ninth code slice is now implemented:

- add initial `enabled`/`visible` state to manifest and runtime command
  metadata while keeping the public service/vtable prefixes unchanged;
- expose `setState`/`set_state`/`SetState`/`setState` through the common worker
  facades, apply the state in Automation's existing menu and toolbar path, and
  trigger the normal Plugin Manager/menu refresh;
- cover manifest state parsing, the runtime API schema, and state changes from
  Python, PowerShell, and PHP in the isolated process-runtime test;
- build Automation, Salamatrix, all four standalone provider projects, and the
  focused tests into `build\verification\command-state`. The explicit
  `SALAMATRIX_WORKER_ROOT` process run, all worker syntax checks, and the
  parser/protocol tests passed; no Salamander process was started or
  controlled.

The thirtieth code slice is now implemented:

- complete the append-only `IRuntimeSession::GetDiagnostic` implementation in
  the Automation process bridge and all four standalone runtime providers;
- preserve cached process id and exit code after handle teardown, distinguish
  running, explicit host stop, clean exit, and nonzero failed exit, and expose
  bounded diagnostic text without changing any public vtable prefix;
- extend the process-runtime test with running/stopped/process-id assertions
  and a deterministic nonzero-exit failure fixture;
- build Automation, the process-runtime test, and all four standalone
  providers into `build\verification\runtime-lifecycle`. The explicit
  `SALAMATRIX_WORKER_ROOT` run passed with exit code 0; no Salamander process
  was started or controlled.

The thirty-first code slice is now implemented:

- append `filter` and `save` file-picker options to the shared native control
  contract without changing existing enum values, vtable prefixes, or control
  layout prefixes; `save` selects the native `GetSaveFileNameW` path and
  `filter` uses the UTF-8 pipe-separated Win32 filter format;
- keep the SMX1 wire contract flat (`kind`, `text`, `filter`, and `save`) and
  preserve positional compatibility in the Python and Node helpers by placing
  the new optional values after the existing layout argument;
- update the Automation host mapping, all four standalone worker assets, the
  runtime/AI schema fragments, and the Python/PowerShell/PHP process bootstrap
  assertions for both option values;
- rebuild Salamatrix, Automation, all four standalone providers, and the
  focused tests into `build\verification\file-picker-options`; UI layout,
  runtime protocol, worker syntax, and explicit `SALAMATRIX_WORKER_ROOT`
  process-runtime tests passed with exit code 0. No Salamander process was
  started or controlled.

The thirty-second code slice is now implemented:

- append tabCreated, tabClosed, tabReordered, windowDetached, and
  windowAttached as event kinds 16–20 without changing the existing event enum
  prefix, service vtable, or V1 EventPayload prefix;
- infer lifecycle transitions from per-side heap-backed tab snapshots on
  PLUGINEVENT_TABCHANGED, establish an explicit baseline for empty sides, and
  publish inferred lifecycle events before the legacy tabChanged;
- bridge changedTabId, tabIndex, and previousTabIndex into Automation's SMX1
  JSON event frame while retaining safe defaults for older V1 payloads;
- extend the existing Plugin Manager manifest event allow-list and the
  Salamatrix/AI event schemas, with deterministic native, manifest, and schema
  tests;
- build the focused tests, Automation, Salamatrix, and all four standalone
  providers into build\verification\tab-lifecycle; all passed, as did the
  explicit worker-root process-runtime run and all four worker syntax checks.
  No Salamander process was started or controlled.

The thirty-third code slice is implemented and verified:

- a deterministic Python bootstrap scenario verifies the fixed host/worker tab
  mutation contract: createTab returns a decimal string tab id and closeTab,
  reorderTab, moveTab, and setDetached return their exact response shapes;
  each dispatcher path is counted once;
- the published Sides schema advertises version 1.3 and all five append-only
  mutation methods;
- core, Automation host, Salamatrix, events, runtime protocol, process-runtime,
  extension-manifest, and all four standalone provider projects pass isolated
  Debug x64 verification under `build\verification\tab-object-model`;
- all four worker syntax checks pass. The process-runtime test passed with an
  explicit provider worker root in `SALAMATRIX_WORKER_ROOT`; no Salamander
  process was started or controlled.

The thirty-fourth code slice is implemented and verified:

- classify Salamatrix Framework, JavaScript/Python/PowerShell/PHP runtime
  providers, and SalamatrixAI by stable registry-key prefixes with display-name
  fallbacks, preserving the ordinary Automation Framework label for unrelated
  plugins;
- expose SalamatrixAI's Menu Extension capability in the existing Plugin
  Manager and publish a localized command that opens the local-model chat;
- make SalamatrixAI shutdown unload-safe when Salamatrix Framework unloads
  first by querying the current `SALAMATRIX_SERVICE_AI` record and checking
  pointer identity before provider unregistration, then clearing all borrowed
  service pointers;
- add a deterministic source-contract regression script and rebuild both the
  Salamatrix core and SalamatrixAI into `build\verification\regressions`;
- the regression script, core Debug x64 build, and SalamatrixAI Debug x64 build
  passed. No Salamander process was started or controlled.
