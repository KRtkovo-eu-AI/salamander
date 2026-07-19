# Salamatrix Platform Foundation

## Purpose

Salamatrix is the working technical name for the unified extensibility platform in
Open Salamander. It is intended to connect the Salamander core, native plugins,
scripted extensions, automation runtimes, shared UI services, and future SDK
surface area without creating separate APIs for each runtime.

This document captures the initial platform foundation for the first MVP. It is a
design and integration guide, not a commitment that all subsystems must be built
at once.

## Component name and location

The first runtime component should use the technical name **Salamatrix**. The
preferred source-tree location for the runtime plugin is:

```text
src/plugins/salamatrix/
```

The component should be packaged and registered as a runtime/service plugin, not
as a user-facing command plugin. It may expose a small About/configuration page
and demo commands while the MVP is developed, but its primary purpose is to
provide shared services to other plugins and script runtimes.

Recommended user-visible names:

- `Salamatrix Runtime`
- `Salamatrix UI Runtime`
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
├── Salamatrix.Storage
└── Salamatrix.SDK
```

For the MVP, only these names should be treated as active design targets:

- `Salamatrix.UI`
- `Salamatrix.Commands`
- `Salamatrix.FileOperations`
- `Salamatrix.Runtime`

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
```

In this model, **Salamatrix** is the platform and SDK name, while
**Salamander** is the user-friendly script root object. Runtime adapters may also
expose advanced metadata under `Salamander.Runtime` when needed.

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
```

Recommended C-style constants for future public headers:

```cpp
#define SALAMATRIX_SERVICE_UI              "Salamatrix.UI"
#define SALAMATRIX_SERVICE_COMMANDS        "Salamatrix.Commands"
#define SALAMATRIX_SERVICE_FILEOPERATIONS  "Salamatrix.FileOperations"
#define SALAMATRIX_SERVICE_RUNTIME         "Salamatrix.Runtime"

#define SALAMATRIX_VERSION_1_0 0x00010000
```

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
    const char* ProviderPluginName;
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
registers the PoC UI, Commands, FileOperations, and Automation services with both
its local registry and the host registry while the runtime object is alive.

### Provider registration

Runtime plugins should register services during their plugin entry/init phase,
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
if (!QueryService("Salamatrix.UI", SALAMATRIX_VERSION_1_0, &ui))
{
    SalamanderGeneral->SalMessageBox(
        parent,
        "This plugin requires Salamatrix UI Runtime.",
        pluginName,
        MB_OK | MB_ICONERROR);
    return FALSE;
}
```

### Script callers

Script runtimes should translate missing required services into clear runtime
exceptions. Recommended wording:

```text
This script requires Salamatrix UI Runtime 1.0 or newer. Install or enable the
Salamatrix Runtime plugin and try again.
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
This extension requires Salamatrix UI Runtime.
Install or enable Salamatrix Runtime and try again.
```

If the Plugin Manager supports runtime classification, Salamatrix should be shown
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

The first script mapping should remain user-facing through the `Salamander` root
object while still being backed by `Salamatrix.UI` internally:

```python
with Salamander.UI.progress("Processing files") as progress:
    progress.total = len(files)
    for file in files:
        progress.add_text(file.name)
        if not progress.step(1):
            progress.cancel_enabled = False
            break
```

Existing Automation scripts may keep using `Salamander.ProgressDialog`; a later
adapter can expose `Salamander.UI.progress(...)` as a more structured wrapper
without breaking the older object.

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
```

For the command-posting MVP, `OperationResultOk` means the existing command was
accepted and posted, while `OperationResultError` means the command was unknown,
disabled, or no command service was available. `OperationResultCancel` is reserved
for the next synchronous/modal integration step where a direct workflow wrapper
can observe the dialog result.

Recommended script mapping:

```python
Salamander.Commands.execute("QuickRename")
Salamander.FileOperations.rename_interactive()
Salamander.FileOperations.copy_interactive()
Salamander.FileOperations.move_interactive()
```

The script layer should convert `OperationResultError` to a readable exception
and may expose `OperationResultCancel` as `False`, `None`, or a typed result
object depending on the final scripting style.

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
Salamander.Commands.execute(...)         -> Salamatrix::Automation::ScriptCommandsAdapter
Salamander.FileOperations.*_interactive  -> Salamatrix::Automation::ScriptFileOperationsAdapter
```

`ScriptProgressDialog` creates a native `Salamatrix::UI::IProgressDialog` through
`IUIService`, opens it with the supplied title, delegates `total`, `add_text`,
`step`, `is_cancelled`, and `cancel_enabled`, then closes and destroys the native
progress object when the script wrapper is destroyed. Existing Automation
`Salamander.ProgressDialog` can remain as a compatibility facade and later be
implemented on top of the same `IUIService`.

`ScriptCommandsAdapter::Execute(...)` delegates to `ICommandService::Execute(...)`
so script calls such as `Salamander.Commands.execute("QuickRename")` still use the
existing Salamander command handlers. `ScriptFileOperationsAdapter` delegates
`rename_interactive`, `copy_interactive`, and `move_interactive` to
`IFileOperationsService`, which in the MVP routes to the existing Quick Rename,
Copy, and Move workflows.

The generic form builder is intentionally not part of this MVP. The adapter file
only reserves the minimal future object model needed to connect the existing
Automation GUI component approach to the native `Salamatrix.UI` core:

- `Dialog` -> `IDialogAdapter`
- `Container` -> `IContainerAdapter`
- `Label` -> `ControlKindLabel`
- `TextBox` -> `ControlKindTextBox`
- `CheckBox` -> `ControlKindCheckBox`
- `ComboBox` -> `ControlKindComboBox`
- `Button` -> `ControlKindButton`
- `ListView` -> `ControlKindListView`

The current Automation GUI layer already separates component state, containers,
window creation, and individual control implementations. The Salamatrix direction
is to move the native control model into `Salamatrix.UI` and leave Automation
classes as COM/IDispatch wrappers over those native objects.

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
Automation adapter in Salamander's core-facing `CSalamanderGeneralAbstract`
service registry. DemoPlug remains only a consumer/sample and no longer needs to
act as the long-lived provider.

- `Runtime::ServiceRegistry` provides a minimal fixed-size local
  `RegisterService`/`QueryService` implementation, while `CSalamanderGeneral`
  provides the core-facing registry used by plugin consumers.
- `Runtime::LocalUIService` implements `Salamatrix::UI::IUIService` by creating
  and destroying `Salamatrix::UI::ProgressDialog` objects over the existing
  `CSalamanderForOperationsAbstract` progress API.
- `Runtime::RuntimeServices` wires one in-process UI service, command service,
  file-operation service, script root adapter, and service registry together.
  The Salamatrix runtime plugin owns one persistent instance and unregisters the
  host services when the plugin is released.
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
runtime-provided `Salamatrix.Automation`, `Salamatrix.UI`,
`Salamatrix.Commands`, and `Salamatrix.FileOperations` services when the plugin
connects and immediately before script execution. It does not create a local
fallback runtime, so the Automation layer remains an adapter/consumer of
`SALAMATRIX.SPL` rather than another provider of duplicated UI or command logic.


Automation 2.0 starts exposing that bridge to scripts through `Salamander.UI`,
`Salamander.Commands`, and `Salamander.FileOperations`. The first script-facing
UI object is `Salamander.UI.progress(...)`, returning the existing progress
Automation interface backed by `Salamatrix.UI`. Commands and interactive file
operations return textual MVP results: `ok`, `cancel`, or `error`.


The initial command catalog is deliberately small and stable: `QuickRename`,
`Copy`, `Move`, and `MoveRename`, with script aliases `quick_rename`, `copy`,
`move`, and `move_rename`. All MVP entries route to existing Salamander command
handlers and require the current panel context to make the command meaningful.


For the first real scripted-extension sample, Automation recognizes optional
script header metadata:

```javascript
// Salamatrix.CommandId: Salamatrix.ProgressDemo
// Salamatrix.CommandTitle: Salamatrix Progress Demo
```

This is the MVP command-registration path for script files discovered by the
existing Automation script repository. The command id is stored with the script
metadata and the title overrides the file-name-derived menu caption; execution
continues to use the existing Automation script command workflow.

## MVP acceptance criteria

The platform skeleton is ready when:

1. The runtime component name and source location are documented as Salamatrix in
   `src/plugins/salamatrix/`.
2. The active MVP subsystem names are defined as `Salamatrix.UI`,
   `Salamatrix.Commands`, `Salamatrix.FileOperations`, and
   `Salamatrix.Runtime`.
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
   wrappers over the native UI, Commands, and FileOperations MVP services.
9. The generic form-builder model is reserved as adapter contracts only; no
   duplicate Automation UI implementation is introduced outside `Salamatrix.UI`.
10. The in-process Salamatrix PoC wires UI, Commands, FileOperations,
   Automation adapters, a local service registry, and the core-facing
   `CSalamanderGeneral` service registry together and is exposed as a DemoPlug
   `Salamatrix PoC` menu sample.
11. `SALAMATRIX.SPL` exists as the first runtime provider plugin, creates the
   persistent `Runtime::RuntimeServices` aggregate, registers the MVP services
   with `CSalamanderGeneral`, and unregisters them during plugin release.
12. The Automation plugin contains a consumer-only bridge that refreshes and
   caches host-registered Salamatrix services instead of instantiating a local
   duplicate runtime.
