# Salamatrix Platform Foundation

## Purpose

Salamatrix is the working technical name for the unified extensibility platform in
Open Salamander. It is intended to connect the Salamander core, native plugins,
scripted extensions, future language runtime adapters, shared UI services, and future SDK
surface area without creating separate APIs for each runtime.

This document captures the initial platform foundation for the first MVP. It is a
design and integration guide, not a commitment that all subsystems must be built
at once.

## Component name and location

The first Automation Framework provider component uses the technical name **Salamatrix**. The
preferred source-tree location for the framework plugin is:

```text
src/plugins/salamatrix/
```

The component should be packaged and registered as an Automation Framework/service plugin, not
as a language runtime or user-facing command plugin. It may expose a small About/configuration page
and demo commands while the MVP is developed, but its primary purpose is to
provide shared services to other plugins and future script runtime adapters.

Recommended user-visible names:

- `Salamatrix Framework`
- `Salamatrix UI Framework`
- `Salamatrix SDK` for developer-facing documentation and headers

## Naming model

Use one umbrella platform name and technical subsystem names beneath it:

```text
Salamatrix
├── Salamatrix.Core
├── Salamatrix.UI
├── Salamatrix.Commands
├── Salamatrix.FileOperations
├── Salamatrix.Runtime
├── Salamatrix.Events
├── Salamatrix.Extensions
├── Salamatrix.Storage
└── Salamatrix.SDK
```

For the MVP, only these names should be treated as active design targets:

- `Salamatrix.UI`
- `Salamatrix.Commands`
- `Salamatrix.FileOperations`
- `Salamatrix.Runtime`
- `Salamatrix.Sides`
- `Salamatrix.Events`
- `Salamatrix.Extensions`
- `Salamatrix.Storage`

### Native naming

Native C++ headers can use `Salamatrix` as the namespace or API prefix:

```cpp
Salamatrix::UI::CreateProgressDialog(...);
Salamatrix::Commands::Execute(...);
Salamatrix::FileOperations::CopyInteractive(...);
```

The exact ABI-safe interface exposed to plugins should still use abstract
interfaces and versioned service identifiers instead of requiring C++ callers to
link directly to a C++ implementation class.

### Script naming

Scripts should normally see the existing product concept as their root object:

```python
Salamander.UI.progress(...)
Salamander.Commands.execute(...)
Salamander.FileOperations.copy_interactive(...)
Salamander.Sides.Source.ActiveTab.Path
Salamander.Storage.get("lastPath")
```

In this model, **Salamatrix** is the platform and SDK name, while
**Salamander** is the user-friendly script root object. Future runtime adapters such as Python, JavaScript, PowerShell, or WSH may also
expose advanced adapter metadata under `Salamander.Runtime` when needed.

## Service identifiers and versioning

Each shared surface should be published as a versioned service. The service name
is a stable ASCII identifier; the version is a monotonically increasing integer
or a packed semantic version.

Initial service identifiers:

```text
Salamatrix.UI
Salamatrix.Commands
Salamatrix.FileOperations
Salamatrix.Runtime
Salamatrix.Automation
Salamatrix.Sides
Salamatrix.Events
Salamatrix.Extensions
Salamatrix.Storage
```

Implemented C-style constants in the Salamatrix headers:

```cpp
#define SALAMATRIX_SERVICE_UI              "Salamatrix.UI"
#define SALAMATRIX_SERVICE_COMMANDS        "Salamatrix.Commands"
#define SALAMATRIX_SERVICE_FILEOPERATIONS  "Salamatrix.FileOperations"
#define SALAMATRIX_SERVICE_RUNTIME         "Salamatrix.Runtime"
#define SALAMATRIX_SERVICE_AUTOMATION_ADAPTER "Salamatrix.Automation"
#define SALAMATRIX_SERVICE_SIDES           "Salamatrix.Sides"
#define SALAMATRIX_SERVICE_EVENTS         "Salamatrix.Events"
#define SALAMATRIX_SERVICE_EXTENSIONS     "Salamatrix.Extensions"
#define SALAMATRIX_SERVICE_STORAGE         "Salamatrix.Storage"

#define SALAMATRIX_UI_VERSION_1_0             0x00010000
#define SALAMATRIX_COMMANDS_VERSION_1_0       0x00010000
#define SALAMATRIX_FILEOPERATIONS_VERSION_1_0 0x00010000
#define SALAMATRIX_AUTOMATION_VERSION_1_0     0x00010000
#define SALAMATRIX_RUNTIME_VERSION_1_0        0x00010000
#define SALAMATRIX_SIDES_VERSION_1_0          0x00010000
#define SALAMATRIX_EVENTS_VERSION_1_0         0x00010000
#define SALAMATRIX_EXTENSIONS_VERSION_1_0     0x00010000
#define SALAMATRIX_STORAGE_VERSION_1_0        0x00010000
```

`SALAMATRIX.SPL` itself is an Automation Framework provider. The current host
registration publishes the UI, Commands, FileOperations, Runtime broker, and
Automation adapter, Sides, Events, Extensions, AI, and Storage services. The event service is
fed by the host's plugin event callback and is also available to native runtime
adapters.

## Salamatrix.Runtime broker and execution contract

The first runtime broker contract is declared in:

```text
src/plugins/salamatrix/salamatrix_runtime_api.h
```

`IRuntimeService` owns the process-local catalog of language runtime adapters.
Adapters publish a stable runtime id, display/language metadata, supported entry
point extensions, a runtime version, and flags describing in-process,
out-of-process, bundled, compatibility, and persistent-extension capabilities.
The broker supports registration, unregistration, enumeration, lookup by
runtime id/version, and fallback lookup by entry point. `IRuntimeAdapter` also
receives a versioned `RuntimeExecutionRequest` and returns a structured
`RuntimeExecutionResult` with succeeded, failed, or cancelled status, HRESULT,
process/exit information, a bounded UTF-16 output capture, and an adapter-owned
error message. Requests carry an explicit working directory and a default
two-minute timeout (clamped to one hour).

The Automation consumer bridge is the first adapter provider. It advertises the
legacy Active Scripting engines that are actually available:

- `Automation.JScript` for `.js`;
- `Automation.VBScript` for `.vbs`;
- `Automation.ActivePython` for `.pys`, when the legacy ActivePython COM engine
  is installed;
- `Automation.PHPScript` for `.phps`, when the legacy PHPScript COM engine is
  installed.

These adapters are explicitly marked as in-process compatibility adapters. In
addition, Automation registers optional out-of-process CLI adapters when the
interpreter is discoverable through `PATH` or an explicit environment variable:

- `Python.CPython` for `.py` (`SALAMATRIX_PYTHON`, then `python.exe`/`python3.exe`);
- `PowerShell` for `.ps1` (`SALAMATRIX_POWERSHELL`, then `pwsh.exe`/
  `powershell.exe`);
- `PHP.CLI` for `.php` (`SALAMATRIX_PHP`, then `php.exe`).

The process adapter uses a non-shell `CreateProcessW` invocation, passes the
entry point as a quoted file argument, drains a combined stdout/stderr pipe,
limits captured output to 1 MiB, terminates timed-out children, and returns the
exit code. This makes Python/PowerShell/PHP real selectable runtimes today;
they intentionally do not yet expose Salamander objects to the child process.
The persistent session seam is now present, and manifest activation starts a
bounded host pump thread so worker stdout is processed during normal plugin
operation. The Automation host dispatcher also
handles the first language-neutral calls (`runtime.ready`, command execution,
active-tab snapshots, string storage, event subscribe/unsubscribe, and a
message-box dialog) without sharing native pointers. Richer UI/value bindings
and a worker bootstrap/library remain to be added on top of this boundary.

The first worker-transport slice is declared in
`src/plugins/salamatrix/salamatrix_runtime_protocol.h`. It provides a bounded,
incremental `SMX1` line codec with typed message kinds (`hello`, `ready`, `call`,
`result`, `event`, `shutdown`, `error`), decimal request ids, and compact JSON
payloads. The codec rejects embedded newlines, malformed ids, and frames over
1 MiB, and is intentionally independent of any language's JSON library. The
Automation bridge now answers the first host calls through this transport;
the remaining work is a worker bootstrap/library plus UI, event, and richer
value bindings.

Process adapters can now opt into the common worker bootstrap with
`RuntimeExecutionFlagUseWorkerBootstrap`. Automation ships standard-library
bootstraps for Python, PowerShell, and PHP under `runtime\`; they perform the
SMX1 handshake, expose the same logical `Salamander` object model, route host
calls, and keep an event loop alive after the extension entry point returns.
`SALAMATRIX_WORKER_ROOT` is an explicit deployment/test override; the normal
plugin build copies the scripts beside the Automation binary.

Runtime registration is tied to the Automation bridge refresh/release lifecycle.
Before unregistering, the bridge verifies that the same broker is still
published by the host so a provider unload cannot turn cleanup into a call
through a stale service pointer.

The framework's host-service registration is transactional. If any of the ten
services cannot be registered, services registered earlier in the attempt are
rolled back in reverse order instead of leaving dangling partial registrations.

## Salamatrix.Sides and panel-tab snapshots

The runtime-neutral side and tab contract is declared in:

```text
src/plugins/salamatrix/salamatrix_sides.h
```

`ISidesService` resolves the logical Left, Right, Source, and Target references
to physical sides. It exposes tab counts, active-tab snapshots, lookup by a
stable process-local tab id, path reads, tab activation, and active-tab path
changes. Snapshots include physical side, index, path type, and flags for
active-on-side, source, target, locked, and detached state.

The core SDK additions in `CSalamanderGeneralAbstract` expose only value
snapshots and opaque ids. They do not leak `CFilesWindow` or tab-array pointers
across the plugin boundary. Each `CFilesWindow` receives a nonzero monotonically
increasing id for its lifetime. Callers must therefore re-read a tab by id
before each operation and treat a failed lookup as a stale handle.

Automation publishes the same model under the `Salamander.Sides` root:

```javascript
var source = Salamander.Sides.Source;
var tab = source.ActiveTab;
Salamander.TraceI(
    source.Name + ": " + tab.Path + " (" + tab.Id + ")");
tab.Activate(true);
Salamander.Sides.Target.Path = "C:\\Temp";
```

The tab id is exposed to scripts as a decimal string, not a JavaScript number,
so a 64-bit native id cannot lose precision in legacy JScript. The current
service deliberately stops at tab identity, state, path, and activation.
Focused/selected item snapshots, view settings, refresh, and tab lifecycle
events remain follow-up work.

## Salamatrix.Storage per-extension persistence

The runtime-neutral persistent storage contract is declared in:

```text
src/plugins/salamatrix/salamatrix_storage.h
```

`IStorageService` accepts a validated manifest extension id and a key, keeping
each extension in a separate namespace. Version 1 supports UTF-8 strings up to
16 KiB, signed 64-bit integers, booleans, deletion, and clearing one namespace.
The service is synchronized so future worker-process/runtime bridges do not need
to share Automation's old global `VARIANT` container.

`SALAMATRIX.SPL` owns the storage and persists all namespaces as one bounded,
versioned binary configuration blob through `CSalamanderRegistryAbstract`.
Loading validates the complete header, entry count, lengths, identifiers, types,
payload sizes, embedded NULs, and duplicate records before accepting values.
Unchanged data is not rewritten.

Manifest-based Automation extensions use the same service through:

```javascript
var previous = Salamander.Storage.get("lastPath", "(first run)");
Salamander.Storage.set("lastPath", Salamander.Sides.Source.Path);
Salamander.TraceI(Salamander.Storage.Namespace + ": " + previous);
```

`Salamander.Storage` is deliberately unavailable to loose legacy scripts with
no manifest id, because silently returning to one shared global namespace would
break the isolation guarantee. The older `SetPersistentVal` and
`GetPersistentVal` methods remain as compatibility APIs. Future package
management still needs quotas, migration hooks, settings schemas, and an
uninstall retention/deletion policy.

## Salamatrix.Events host subscriptions

The event contract is declared in:

```text
src/plugins/salamatrix/salamatrix_events.h
```

`IEventsService` provides versioned subscribe/unsubscribe with opaque 64-bit
subscription ids. Dispatch snapshots matching subscribers under a short lock,
releases the lock before invoking callbacks, and therefore permits a callback
to unsubscribe itself safely. The current payload contains the event name,
host parameter, active physical panel, active tab id, path type, and a copied
path. The initial host mapping covers startup/shutdown, settings and
configuration changes, color changes, panel swaps, and active-panel changes.

The Automation facade is:

```javascript
var subscription = Salamander.Events.subscribe(
    "activePanelChanged",
    function (name, parameter, path, activeTabId) {
        Salamander.TraceI(name + ": " + path);
    });
Salamander.Events.unsubscribe(subscription);
```

Callbacks are owned by the subscribing extension object and are automatically
unsubscribed when that object is released. Persistent extension instances and
worker-runtime dispatch queues still need to be added; one-shot legacy scripts
cannot receive an event after their execution has ended.

## Salamatrix.Extensions lifecycle registry

The lifecycle registry is declared in:

```text
src/plugins/salamatrix/salamatrix_extensions.h
```

`IExtensionsService` keeps a bounded, owner-aware catalog of valid extension
descriptors. Entries have explicit `discovered`, `activating`, `active`,
`deactivating`, `inactive`, and `failed` states and can be activated or
deactivated through a callback that runs outside the registry lock. Automation
registers valid manifest-backed scripts during initial load and refresh, using
the `CScriptInfo` address as the owner; removed scripts are unregistered before
their objects are deleted. Duplicate ids from another owner are rejected.

Automation registrations carry a lifecycle callback. Activating a manifest
looks up its selected runtime adapter and starts an `IRuntimeSession`; the
session is retained by the owning `CScriptInfo` and is stopped before that
script is removed. Legacy ActiveScript adapters deliberately reject persistent
activation, while the new CLI adapters require a worker-compatible entry point
and fail activation if the child exits immediately. Command ownership, host
call dispatch, and unload leases still build on this seam.

## Service lookup API

The natural integration point is the existing plugin host interfaces. Plugins
already receive a `CSalamanderPluginEntryAbstract` in `SalamanderPluginEntry()`
and use it to obtain core services such as the general, debug, and GUI
interfaces. The service lookup should therefore be exposed either from the plugin
entry interface or from `CSalamanderGeneralAbstract`.

Implemented MVP shape:

```cpp
struct CSalamanderServiceQuery
{
    const char* ServiceId;
    DWORD MinimumVersion;
    DWORD Flags;
};

struct CSalamanderServiceResult
{
    void* Interface;
    DWORD Version;
    const char* ProviderName;
};

virtual BOOL WINAPI QueryService(
    const CSalamanderServiceQuery* query,
    CSalamanderServiceResult* result) = 0;
```

The MVP also adds `UnregisterService(serviceId, serviceInterface)` so temporary
PoC providers can safely remove stack-owned service instances before returning
from DemoPlug command handlers.

The core-facing MVP is now implemented on `CSalamanderGeneralAbstract` and backed
by a process-local registry in `CSalamanderGeneral`. `Runtime::RuntimeServices`
registers the PoC UI, Commands, FileOperations, Runtime broker, Automation,
Sides, Events, Extensions, AI, and Storage services with both its local registry and the host registry
while the runtime object is alive.

### Provider registration

Automation Framework providers should register services during their plugin entry/init phase,
after their own version check and language/resource initialization succeed.

Implemented MVP shape:

```cpp
virtual BOOL WINAPI RegisterService(
    const char* serviceId,
    DWORD version,
    void* serviceInterface,
    const char* providerName) = 0;

virtual BOOL WINAPI UnregisterService(
    const char* serviceId,
    void* serviceInterface) = 0;
```

The registry must reject duplicate providers for the same service/version unless
a future policy explicitly supports replacement. The provider plugin must not be
unloaded while a registered service may still be queried or retained by other
plugins.

## Missing runtime behavior

### Native plugin callers

If a native plugin requests a service that is not installed, not loaded, or too
old, `QueryService` should return failure and set the output interface pointer to
`NULL`. The caller remains responsible for deciding whether this is optional or
fatal.

Recommended helper behavior for required services:

```cpp
CSalamanderServiceQuery query;
memset(&query, 0, sizeof(query));
query.ServiceId = SALAMATRIX_SERVICE_UI;
query.MinimumVersion = SALAMATRIX_UI_VERSION_1_0;

CSalamanderServiceResult result;
memset(&result, 0, sizeof(result));
if (!SalamanderGeneral->QueryService(&query, &result) || result.Interface == NULL)
{
    SalamanderGeneral->SalMessageBox(
        parent,
        "This plugin requires Salamatrix UI Framework.",
        pluginName,
        MB_OK | MB_ICONERROR);
    return FALSE;
}
```

### Script callers

Script runtimes should translate missing required services into clear runtime
exceptions. Recommended wording:

```text
This script requires Salamatrix UI Framework 1.0 or newer. Install or enable the
Salamatrix Framework plugin and try again.
```

The exception should include:

- service identifier,
- requested version,
- currently available version, if any,
- plugin/runtime that requested it.

### UI behavior

When a user invokes an extension whose required service is missing, the UI should
show a localized message in this form:

```text
This extension requires Salamatrix UI Framework.
Install or enable Salamatrix Framework and try again.
```

If the Plugin Manager supports Automation Framework classification, Salamatrix should be shown
as a runtime/service component rather than as an ordinary menu extension.

## Integration with existing plugin registration

The current plugin model already has a natural lifecycle for service providers:

1. Salamander loads a plugin and calls `SalamanderPluginEntry()`.
2. The plugin obtains `CSalamanderGeneralAbstract`, debug, and GUI interfaces.
3. The plugin verifies `SalamanderPluginGetReqVer()` compatibility.
4. The plugin loads language resources and initializes WinLib if needed.
5. The plugin calls `SetBasicPluginData()` to declare its basic metadata and
   functions.
6. The plugin returns its `CPluginInterfaceAbstract` implementation.

Salamatrix should register its services after step 4 and before returning the
plugin interface. Consumers should query services after their own basic startup
checks have completed.

The most natural place for the service registry itself is the core plugin
manager layer, near the code that tracks loaded plugins and their metadata. That
keeps service ownership tied to the same objects that already control plugin
lifetime and unload checks.

Recommended implementation direction:

- add service registry storage to the core plugin manager implementation,
- expose `RegisterService` to plugin providers through a host interface,
- expose `QueryService` to consumers through `CSalamanderGeneralAbstract` or a
  dedicated service-manager interface,
- make unload checks fail while exported services are still registered or held,
- persist only plugin installation metadata, not live service pointers.

## Salamatrix.UI progress dialog MVP

The first concrete object API in `Salamatrix.UI` is the progress dialog. The C++
MVP surface is declared in:

```text
src/plugins/salamatrix/salamatrix_ui.h
```

The declaration intentionally wraps the existing `CSalamanderForOperationsAbstract`
progress methods instead of duplicating progress-window behavior. This keeps the
first Salamatrix UI object compatible with current native plugin operations and
with the existing Automation progress object.

The initial API shape contains:

- `Salamatrix::UI::ProgressDialogOptions` for title, parent window, one/two-bar
  mode, file/total labeling, and initial Cancel state.
- `Salamatrix::UI::IProgressDialog` as the ABI-oriented object contract.
- `Salamatrix::UI::ProgressDialog` as the first C++ adapter over
  `CSalamanderForOperationsAbstract`.
- `Salamatrix::UI::IUIService` as the future service returned by
  `QueryService("Salamatrix.UI", SALAMATRIX_UI_VERSION_1_0, ...)`.

The object covers the MVP lifecycle and control flow:

1. `SetTitle(...)` sets the title before the dialog is opened.
2. `Open()` or `Open(options)` creates the existing Salamander progress dialog.
3. `SetTotal(...)` and `SetTotals(...)` configure one-bar or two-bar totals.
4. `AddText(...)` appends progress log/status text.
5. `Step(...)`, `SetPosition(...)`, and `SetPositions(...)` update progress and
   return whether the operation should continue.
6. `IsCancelled()` polls cancellation by refreshing the existing progress dialog
   with a zero-sized progress update.
7. `SetCancelEnabled(FALSE)` supports cleanup phases where Cancel must be
   disabled.
8. `Close()` closes the dialog explicitly, and the C++ adapter destructor closes
   it as a final safety net.

The first script mapping is now exposed through the `Salamander` root object and
is backed by `Salamatrix.UI` internally:

```javascript
var progress = Salamander.UI.progress("Processing files");
progress.Maximum = files.length;
progress.Show();
try {
    for (var i = 0; i < files.length; ++i) {
        progress.AddText(files[i].Name);
        progress.Position = i + 1;
        if (progress.IsCancelled) {
            progress.CanCancel = false;
            break;
        }
    }
}
finally {
    progress.Hide();
}
```

Existing Automation scripts may keep using `Salamander.ProgressDialog`; the new
`Salamander.UI.progress(...)` path returns the same progress Automation interface
but is backed by `Salamatrix.UI` through the Automation bridge.

## Salamatrix.Commands and FileOperations MVP

The command and interactive file-operation MVP surface is declared in:

```text
src/plugins/salamatrix/salamatrix_commands.h
```

`Salamatrix.Commands` is intentionally a thin layer over existing Salamander
commands. It maps public command names such as `QuickRename`, `Copy`, and `Move`
to the existing `SALCMD_QUICKRENAME`, `SALCMD_COPY`, and `SALCMD_MOVE` command
identifiers and posts them through `CSalamanderGeneralAbstract::PostSalamanderCommand`.
Before posting, the MVP implementation can check `GetSalamanderCommand` so that
disabled commands fail instead of opening inconsistent UI.

`Salamatrix.FileOperations` uses the same command path for its interactive MVP:

- `RenameInteractive(...)` opens the existing Quick Rename workflow.
- `CopyInteractive(...)` opens the existing Copy dialog/workflow.
- `MoveInteractive(...)` opens the existing Move/Rename dialog/workflow.

The file-operation MVP must not clone `.rc` dialog resources or reimplement the
copy/move/rename validation logic. It should route to the command handlers that
already use the localized strings, histories, target-directory helpers, operation
mask handling, and panel/file-system integration. This keeps the behavior aligned
with the documented Quick Rename, Copy, and Move/Rename dialog boxes.

The shared return enum is `Salamatrix::Runtime::OperationResult`:

```cpp
OperationResultOk
OperationResultCancel
OperationResultError
OperationResultNotAvailable
```

For the command-posting MVP, `OperationResultOk` means the existing command was
accepted and posted, `OperationResultNotAvailable` means the command exists but
is disabled in the current panel context (for example Quick Rename without a
focused/selected item), and `OperationResultError` means the command was unknown
or no command service was available. `OperationResultCancel` is reserved for the
next synchronous/modal integration step where a direct workflow wrapper can
observe the dialog result.

Implemented script mapping:

```javascript
Salamander.Commands.execute("QuickRename")              // returns "ok", "cancel", "not_available", or "error"
Salamander.FileOperations.rename_interactive()          // returns "ok", "cancel", "not_available", or "error"
Salamander.FileOperations.copy_interactive()            // returns "ok", "cancel", "not_available", or "error"
Salamander.FileOperations.move_interactive()            // returns "ok", "cancel", "not_available", or "error"
```

Missing Salamatrix runtime services are converted by the Automation wrapper to a
readable script exception: `This script requires Salamatrix Framework to be
installed and loaded.`

## Salamatrix Automation adapter MVP

The first script/Automation adapter contract is declared in:

```text
src/plugins/salamatrix/salamatrix_automation.h
```

The adapter is deliberately thin: it owns no dialog resources and implements no
parallel UI behavior. It receives native `Salamatrix.UI`, `Salamatrix.Commands`,
and `Salamatrix.FileOperations` services and exposes script-shaped wrapper
objects over them. This keeps the Automation/COM layer as an adapter instead of
a second UI framework.

MVP script facade mapping:

```text
Salamander.UI.progress(...)              -> Salamatrix::Automation::ScriptUIAdapter
Salamander.UI.dialog(...)                -> Salamatrix::Automation::ScriptUIAdapter::Dialog
Salamander.Commands.execute(...)         -> Salamatrix::Automation::ScriptCommandsAdapter
Salamander.FileOperations.*_interactive  -> Salamatrix::Automation::ScriptFileOperationsAdapter
Salamander.FileOperations.delete/create_directory/refresh/properties
                                           -> same native service
```

`ScriptProgressDialog` creates a native `Salamatrix::UI::IProgressDialog` through
`IUIService`, opens it with the supplied title, delegates `total`, `add_text`,
`step`, `is_cancelled`, and `cancel_enabled`, then closes and destroys the native
progress object when the script wrapper is destroyed. Existing Automation
`Salamander.ProgressDialog` can remain as a compatibility facade and later be
implemented on top of the same `IUIService`.

`ScriptUIAdapter::Dialog` creates the shared `Salamatrix::UI::IDialog`, adds
native controls, shows it modally, and exposes the same control state to the
caller. This is the in-process counterpart of the SMX1 worker dialog calls.

`ScriptCommandsAdapter::Execute(...)` delegates to `ICommandService::Execute(...)`
so script calls such as `Salamander.Commands.execute("QuickRename")` still use the
existing Salamander command handlers. `ScriptFileOperationsAdapter` delegates
`rename_interactive`, `copy_interactive`, `move_interactive`, `delete`,
`create_directory`, `refresh`, and `properties` to `IFileOperationsService`,
which posts the corresponding existing Salamander command and preserves its
normal native dialogs and enablement rules.

The shared UI contract now includes a native dialog/control model in
`salamatrix_ui.h/.cpp`. It is intentionally small but real: native plugins and
Automation workers use the same dialog object and control state:

- `Dialog` -> shared `Salamatrix::UI::IDialog` (legacy `IDialogAdapter` remains a compatibility shape)
- `Container` -> `IContainerAdapter`/shared dialog control collection
- `Label` -> `ControlKindLabel`
- `TextBox` -> `ControlKindTextBox`
- `CheckBox` -> `ControlKindCheckBox`
- `ComboBox` -> `ControlKindComboBox`
- `Button` -> `ControlKindButton`
- `RadioButton` -> `ControlKindRadioButton`
- `ListView` -> `ControlKindListView` (native common-control surface; item/column binding is next)
- `TreeView` -> `ControlKindTreeView` (native common-control surface; node binding is next)

The current Automation GUI layer can now be migrated incrementally to these
interfaces instead of creating a second runtime-specific UI. The native
implementation currently renders labels, text boxes, check/radio buttons,
combo boxes, and buttons; list/tree controls retain a declarative placeholder
until their model and virtualized data binding are added.

## Salamatrix.AI provider seam

`src/plugins/salamatrix/salamatrix_ai.h` defines a provider-neutral assistant
contract. Automation registers `local.command` when `SALAMATRIX_AI_COMMAND` is
configured. The command receives one UTF-8 JSON request on standard input and
returns one JSON object/array on standard output; the bridge bounds output to
1 MiB and clamps generation to a five-minute timeout. This makes a local
llama.cpp/Ollama wrapper usable without coupling Salamander to a model vendor.
The shared service validates the structured assistant contract (`title`,
`description`, `capabilities`, `estimatedEffects`, and `script`) and exposes
the parsed effect flags to callers. The preview and save-as-extension UI remain
follow-up work; no llama.cpp binary or model is bundled yet.

## Salamatrix PoC runtime wiring

The first in-process proof-of-concept wiring is declared in:

```text
src/plugins/salamatrix/salamatrix_poc.h
```

This PoC is intentionally small and can run either against the real Salamatrix
runtime plugin or against a local fallback service aggregate. The runtime plugin
lives in `src/plugins/salamatrix/`, has a standalone Visual Studio project in
`src/plugins/salamatrix/vcxproj/`, and exports `SALAMATRIX.SPL`; on plugin entry
it creates a persistent `Runtime::RuntimeServices` instance and registers
`Salamatrix.UI`, `Salamatrix.Commands`, `Salamatrix.FileOperations`, and the
Runtime broker, Automation adapter, Sides, Events, Extensions, AI, and Storage services in Salamander's
core-facing `CSalamanderGeneralAbstract` service registry. DemoPlug remains
only a consumer/sample and no longer needs to act as the long-lived provider.

- `Runtime::ServiceRegistry` provides a minimal fixed-size local
  `RegisterService`/`QueryService` implementation, while `CSalamanderGeneral`
  provides the core-facing registry used by plugin consumers.
- `Runtime::LocalUIService` implements `Salamatrix::UI::IUIService` by creating
  and destroying `Salamatrix::UI::ProgressDialog` objects over the existing
  `CSalamanderForOperationsAbstract` progress API.
- `Runtime::RuntimeServices` wires one in-process UI service, command service,
  file-operation service, runtime broker, script root adapter, Sides, Events,
  Extensions, Storage, and service registry together. The Salamatrix runtime plugin
  owns one persistent instance and unregisters the host services when the plugin
  is released.
- `RunProgressDialogPoc(...)` opens a native Salamatrix progress dialog, sets a
  total, adds text, steps progress, detects Cancel, disables Cancel for cleanup,
  and closes the dialog.
- `RunAutomationProgressPoc(...)` exercises the script-facing
  `Salamander.UI.progress(...)` shape through `ScriptUIAdapter`.
- `ExecuteQuickRenamePoc(...)` and `CopyInteractivePoc(...)` prove that the
  Commands/FileOperations MVP can route to existing Salamander command workflows.

The first consumer/sample integration point is a DemoPlug menu submenu named `Salamatrix PoC`.
It exposes a `Run All PoC` summary command, a progress PoC command, a Quick
Rename command PoC, and a Copy dialog PoC. The individual menu entries are always
enabled; for these PoC menu commands the adapter bypasses panel enabler checks so
the summary reports whether the existing command was accepted/posted rather than
whether the current panel context has a focused or selected item. The progress
command calls the native progress PoC and the script-facing progress adapter PoC from
`CPluginInterfaceForMenuExt::ExecuteMenuItem`, while the Quick Rename and Copy
entries route through the Commands/FileOperations adapters.
When `SALAMATRIX.SPL` is installed and loaded, the sample queries the host
registry first and uses the registered runtime services; when the runtime plugin
is missing, the PoC keeps a local fallback so the demo remains runnable.


The Automation plugin is the first non-demo consumer bridge. Its
`CAutomationSalamatrixBridge` queries `CSalamanderGeneral::QueryService` for the
framework-provided `Salamatrix.Automation`, `Salamatrix.UI`,
`Salamatrix.Commands`, `Salamatrix.FileOperations`, `Salamatrix.Runtime`, and
`Salamatrix.Sides`, `Salamatrix.Events`, `Salamatrix.Extensions`, and
`Salamatrix.Storage` services when
the plugin connects and immediately before script execution. It does not create a local fallback
framework provider, so the Automation layer remains an adapter/consumer of
`SALAMATRIX.SPL` rather than another provider of duplicated UI or command logic.


Automation 2.0 exposes that bridge to scripts through `Salamander.UI`,
`Salamander.Commands`, `Salamander.FileOperations`, `Salamander.Sides`,
`Salamander.Events`, and `Salamander.Storage`. The first script-facing UI object is
`Salamander.UI.progress(...)`, returning the existing progress Automation
interface backed by `Salamatrix.UI`. Commands and interactive file operations
return textual MVP results: `ok`, `cancel`, `not_available`, or `error`. Sides
and tabs are COM/IDispatch wrappers over the same runtime-neutral snapshots and
opaque ids used by native callers; Storage is bound to the executing manifest's
extension id.


The initial command catalog is deliberately small and stable: `QuickRename`,
`Copy`, `Move`, and `MoveRename`, with script aliases `quick_rename`, `copy`,
`move`, and `move_rename`. All MVP entries route to existing Salamander command
handlers and require the current panel context to make the command meaningful.


For the first real scripted-extension sample, Automation recognizes two MVP
registration styles while discovering scripts in the existing script repository.

Inline script metadata remains supported for tiny single-file samples:

```javascript
// Salamatrix.CommandId: Salamatrix.ProgressDemo
// Salamatrix.CommandTitle: Salamatrix Progress Demo
// Salamatrix.CommandMenu: both
// Salamatrix.CommandContextMenu: true
// Salamatrix.CommandRequires: file
```

Manifest-based extensions use an `extension.json` file in the extension root.
The entry point may be next to it or in a safe relative subdirectory:

```json
{
  "id": "Salamatrix.ProgressDemo",
  "name": "Salamatrix Progress Demo",
  "runtime": "Automation.JScript",
  "entryPoint": "main.js",
  "commands": [
    {
      "id": "Salamatrix.ProgressDemo",
      "title": "Salamatrix Progress Demo",
      "menu": "both",
      "contextMenu": true,
      "requires": "file"
    }
  ]
}
```

The reader is a strict UTF-8 JSON parser rather than a scalar text scanner. It
rejects malformed JSON, duplicate members, invalid UTF-8 and Unicode surrogate
pairs, unsafe absolute or traversing entry points, unsupported schema versions,
invalid field types, duplicate command ids, and unsupported placement/context
values. Manifests are limited to 1 MiB and nesting depth 64.

Schema version 1 supports:

- package `id`, `name`/`title`, `version`, `description`, and `entryPoint`;
- a runtime string or `{ "id", "minimumVersion" }` object;
- a validated string array of declared `capabilities`;
- up to 64 command records with `id`, `title`, `handler`, `menu`/`placement`,
  `contextMenu`, and `requires`.

Automation currently contributes the first parsed command to its legacy native
menu surface; preserving all parsed commands for dynamic command contribution
is the next command-service step. Unlike the old scanner, discovery is not
limited to extensions with an `IActiveScript` file association. A valid manifest
entry point is discovered first, then execution resolves the exact runtime id
and minimum version through `Salamatrix.Runtime`. Missing or incompatible
runtimes produce an explicit error instead of silently failing engine lookup.

MVP `requires` values map to Salamander menu event masks as follows:

- `any`: no special context; enabled by `MENU_EVENT_TRUE`.
- `disk`: requires a disk panel context.
- `focused`: requires a focused file or directory on disk.
- `file`: requires a focused file or selected files on disk.
- `selection`: requires selected files or directories on disk.

## MVP acceptance criteria

The platform skeleton is ready when:

1. The runtime component name and source location are documented as Salamatrix in
   `src/plugins/salamatrix/`.
2. The active MVP subsystem names are defined as `Salamatrix.UI`,
   `Salamatrix.Commands`, `Salamatrix.FileOperations`, and
   `Salamatrix.Runtime`, `Salamatrix.Sides`, `Salamatrix.Events`,
   `Salamatrix.Extensions`, and
   `Salamatrix.Storage`.
3. A versioned `QueryService`/`RegisterService` ABI shape is documented.
4. Missing-runtime behavior is defined for native plugins, scripts, and UI.
5. The intended integration point with existing plugin registration and plugin
   lifetime management is documented.
6. The first `Salamatrix.UI` progress dialog contract exists and wraps the
   current `CSalamanderForOperationsAbstract` progress implementation.
7. The first `Salamatrix.Commands` and `Salamatrix.FileOperations` contracts
   exist and route their MVP interactive behavior through existing Salamander
   command/workflow entry points.
8. The first `Salamatrix.Automation` adapter contract exposes script-shaped
   wrappers over the native UI, Commands, FileOperations, Sides, Events, and Storage
   MVP services.
9. The generic form-builder model is reserved as adapter contracts only; no
   duplicate Automation UI implementation is introduced outside `Salamatrix.UI`.
10. The in-process Salamatrix PoC wires UI, Commands, FileOperations, Runtime,
   Sides, Events, Extensions, Storage, Automation adapters, a local service registry, and the
   core-facing `CSalamanderGeneral` service registry together and is exposed as
   a DemoPlug `Salamatrix PoC` menu sample.
11. `SALAMATRIX.SPL` exists as the first Automation Framework provider plugin, creates the
   persistent `Runtime::RuntimeServices` aggregate, registers the MVP services
   with `CSalamanderGeneral`, and unregisters them during plugin release.
12. The Automation plugin contains a consumer-only bridge that refreshes and
   caches host-registered Salamatrix services instead of instantiating a local
   duplicate runtime.
13. Automation 2.0 exposes `Salamander.UI`, `Salamander.Commands`, and
   `Salamander.FileOperations` backed by the Salamatrix runtime bridge.
14. Script discovery supports the first command-registration metadata and a
   minimal `extension.json` manifest for the sample scripted extension.
15. Scripted-extension command placement controls plugin-menu vs context-menu
   visibility and applies MVP `requires` context masks to Automation menu items.
16. `Salamatrix.Runtime` is a registered broker service with versioned adapter
    descriptors, registration, enumeration, and lookup.
17. Automation advertises available legacy Active Scripting engines as
    compatibility runtime adapters and unregisters them during release.
18. Partial host-service registration is rolled back transactionally.
19. Runtime adapters expose structured execution requests/results; manifest
    execution is selected by runtime id and minimum version through the broker.
20. `extension.json` is parsed as strict UTF-8 JSON with schema, path, runtime,
    capability, and command validation.
21. Manifest entry points are discovered independently of legacy ActiveScript
    file associations.
22. `Salamatrix.Sides` exposes Left, Right, Source, and Target resolution plus
    safe panel-tab snapshots, opaque ids, paths, activation, and active-tab path
    changes without leaking core implementation pointers.
23. Automation exposes the same model through `Salamander.Sides`, including
    precision-safe string tab ids and stale-handle checks.
24. The plugin SDK version is raised to 105 for the appended service-registry
    and panel-tab virtual methods.
25. `Salamatrix.Storage` provides synchronized per-extension namespaces for
    UTF-8 strings, signed 64-bit integers, and booleans, with versioned
    configuration persistence.
26. Automation exposes the current manifest namespace through
    `Salamander.Storage`; loose scripts without an extension id cannot enter a
    shared fallback namespace.
27. Standalone `/W4 /WX` tests cover namespace isolation, types, limits,
    mutation, configuration round-trip, unchanged-save behavior, and corrupt
    blob rejection.
28. `Salamatrix.Events` provides unsubscribe-safe host subscriptions for the
    initial lifecycle, settings, configuration, color, panel-swap, and active
    panel events; Automation exposes the same subscription contract.
29. `Salamatrix.Extensions` provides an owner-aware manifest registry with
    explicit lifecycle states and safe activate/deactivate transitions;
    Automation publishes and removes manifest descriptors during script-list
    load and refresh.
30. Automation registers optional `Python.CPython`, `PowerShell`, and `PHP.CLI`
    out-of-process adapters when their interpreters are discoverable, with
    bounded output capture, timeout termination, and structured exit results.
31. `salamatrix_runtime_protocol.h` provides bounded incremental `SMX1` worker
    framing with typed lifecycle/call/event messages and standalone limit and
    malformed-frame tests.
32. `IRuntimeSession` and `StartPersistent()` provide bidirectional persistent
    process sessions; manifest lifecycle activation owns and releases sessions,
    and a Python echo-worker integration test verifies round-trip framing.
33. The Automation host dispatcher answers `runtime.ready`, command execution,
    active-tab snapshots, and per-extension string storage calls over `SMX1`.
34. Manifest activation starts and joins a host pump thread for each persistent
    worker, so the dispatcher is active outside of tests as well.
35. The dispatcher supports event subscriptions with pushed `event` frames and
    a parented `salamander.ui.messageBox` call; subscriptions are removed before
    the worker session is stopped.
36. Python, PowerShell, and PHP process adapters can start the shared worker
    bootstraps; the Python integration test exercises handshake, commands,
    storage, event subscription, and shutdown end to end.
37. `Salamatrix.AI` exposes provider registration and Automation supplies an
    optional bounded local command provider selected by `SALAMATRIX_AI_COMMAND`.
38. The shared worker object model exposes `Salamander.ui.input_box(...)` and
    the host renders it as a parented native Windows dialog with an editable
    value; `message_box(...)` and `input_box(...)` are routed through the
    shared `Salamatrix.UI` service and the same SMX1 host calls are available
    to Python, PowerShell, and PHP.
39. `Salamatrix.UI` now publishes a reusable native `IDialog`/`IControl`
    contract and `NativeDialog` implementation for labels, text boxes,
    check/radio buttons, combo boxes, buttons, ListView, and TreeView; the
    worker input-box path goes through this service rather than owning a second
    dialog backend.
40. The command catalog covers the available core `SALCMD_*` operations and
    `IFileOperationsService` exposes interactive rename/copy/move/delete,
    create-directory, refresh, and properties wrappers to native callers and
    all three modern worker runtimes.
41. Workers can create native dialogs, add all currently supported control
    kinds, show them modally, read control state, and destroy them through the
    same `Salamatrix.UI` service; the bounded process-runtime tests exercise
    this path in Python, PowerShell, and PHP.
