# Standalone Salamatrix runtime providers

Runtime providers are optional Salamander plugins (`.SPL`, i.e. DLLs). They
are not interpreter installers and they do not depend on the Automation plugin.
The user installs Python, PowerShell, PHP, or Node separately; the provider
only discovers that executable, owns its worker bootstrap, and registers an
adapter with the already loaded `Salamatrix.Runtime` broker.

All four providers and the standalone `SalamatrixAI.SPL` use the conventional
plugin metadata structure: their resource script includes `versinfo.rh2` and
`versinfo.rc2`, their entry point uses the `VERSINFO_*` metadata constants, and
their Plugin Manager homepage is `https://samandarin.krtkovo.eu/`. SalamatrixAI
uses the shared Framework icon at `src/res/sal_r.ico`; it remains a separate
Menu Extension plugin and does not move AI ownership back into Automation.

Native UI dark mode is owned by the Salamatrix Framework provider. It reads the
host's explicit `Windows Dark Mode (experimental)` scheme and current scheme
colors, configures the existing `USE_DARKMODELIB=1` integration, refreshes
dialog controls and title bars on theme/configuration broadcasts, and routes
runtime/UI-service Unicode message boxes through the same dark-aware helper.

## Provider lifecycle

```cpp
void WINAPI SalamanderPluginEntry(...)
{
    IRuntimeService* broker = QueryService(
        SalamanderGeneral,
        SALAMATRIX_SERVICE_RUNTIME,
        SALAMATRIX_RUNTIME_VERSION_1_0);
    // If Salamatrix.SPL is not loaded yet, keep the plugin valid and retry
    // from Connect/Event after the framework provider appears.
    TryRegisterProvider();
    return &PluginInterface;
}

BOOL WINAPI CPluginInterface::Release(HWND, BOOL)
{
    providerRegistration.Unregister();
    return TRUE;
}
```

`RuntimeProviderRegistration` in `salamatrix_runtime_api.h` retains the exact
broker/adapter pair and unregisters it during provider release. This keeps
unload independent from Automation and lets native plugins, Automation, and
other runtime providers resolve the same descriptor through `IRuntimeService`.

## Package split

The intended packages are:

| Package | Runtime id | Interpreter discovery |
| --- | --- | --- |
| `PythonRuntime.SPL` | `Python.CPython` | `SALAMATRIX_PYTHON`, `python.exe`, `python3.exe` |
| `PowerShellRuntime.SPL` | `PowerShell` | `SALAMATRIX_POWERSHELL`, `pwsh.exe`, `powershell.exe` |
| `PHPRuntime.SPL` | `PHP.CLI` | `SALAMATRIX_PHP`, `php.exe` |
| `JavaScriptRuntime.SPL` | `JavaScript.Node` | `SALAMATRIX_NODE`, `node.exe`, `node` |

Automation keeps its legacy JScript/VBScript ActiveScript adapters. It becomes
just another broker consumer for the providers above; installing or loading a
provider must not require Automation. A manifest's runtime id and minimum
version select the provider through the broker, while a missing provider is a
clear unavailable-runtime result.

The standalone `SalamatrixAI.SPL` is likewise separate from Automation. It owns
the local assistant provider, chat window, and localized `Ask Salamatrix AI...`
menu command; its menu extension uses the same text as a bounded fallback when
the plugin resource lookup is unavailable. Automation does not publish a
duplicate AI menu item or own the chat flow; it retains only the shared
`Salamatrix.AI` host/API bridge needed by runtime scripts and the
`Salamatrix.ScriptRunner` compatibility service.

The current branch has the broker contract, worker protocol, and provider
lifecycle helper. `PythonRuntime.SPL`, `PowerShellRuntime.SPL`,
`PHPRuntime.SPL`, and `JavaScriptRuntime.SPL` now have their own projects,
adapters, worker assets, and load/unload registration paths. Debug and Release
x64 builds now produce all four standalone `.SPL` binaries; provider
registration is deferred safely when Salamatrix is loaded later.
No provider should be made a dependency of Automation.

## Current worker UI surface

The four modern workers expose the same Salamatrix dialog surface. Along with
labels, text boxes, check/radio buttons, combo boxes, buttons, list/tree/tab
controls, validation, events, and file/folder pickers, each worker now exposes
a folder picker embedded in a dialog:

| Runtime | Dialog method |
| --- | --- |
| Python | `dialog.add_folder_picker(id, path="")` |
| PowerShell | `$dialog.AddFolderPicker(id, path)` |
| PHP | `$dialog->addFolderPicker(id, path)` |
| Node | `await dialog.addFolderPicker(id, path)` |

It maps to the runtime protocol control kind `folderpicker`, opens the standard
native folder browser when clicked, and returns the chosen UTF-8 path through
the normal dialog `get`/control-text mechanism. For editable file paths, the
same four workers additionally expose `add_file_picker`/
`AddFilePicker`/`addFilePicker`; this maps to `filepicker`, keeps the path in an
editable native edit control, and places a separate wide Win32 browse button
next to it.

The editable file picker accepts optional filter and save-mode values. The SMX1
payload stays flat and appends `filter` (UTF-8 pipe-separated description/pattern
pairs) and `save` (boolean) to the existing dialog-add payload:

| Runtime | Dialog method |
| --- | --- |
| Python | `dialog.add_file_picker(id, path="", layout=None, filter="", save=False)` |
| PowerShell | `$dialog.AddFilePicker(id, path, filter, save)` |
| PHP | `$dialog->addFilePicker(id, path, filter, save)` |
| Node | `await dialog.addFilePicker(id, path, layout=null, filter="", save=false)` |

An omitted or empty filter uses the all-files fallback. `save=true` selects the
native save dialog and enables overwrite prompting; the selected UTF-8 path
continues to use the normal dialog control-text/get contract.

## Command state

All four workers accept optional `enabled` and `visible` fields when registering
commands. They also expose the same append-only state update operation:

| Runtime | Registration | State update |
| --- | --- | --- |
| Python | `commands.register(..., enabled=True, visible=True)` | `commands.set_state(id, enabled=None, visible=None)` |
| PowerShell | `$Salamander.Commands.Register(..., $Enabled, $Visible)` | `$Salamander.Commands.SetState(id, $Enabled, $Visible)` |
| PHP | `$Salamander->commands->register(..., $enabled, $visible)` | `$Salamander->commands->setState($id, $enabled, $visible)` |
| Node | `commands.register(..., enabled, visible)` | `commands.setState(id, enabled, visible)` |

The host applies these values to the existing Automation command record and
posts the normal Plugin Manager/menu refresh. Hidden commands are omitted from
the native menu and disabled commands remain visible but non-invokable. This
does not add a public vtable method or require a separate Extension Manager.

Verification at the current pause point: all four provider Debug x64 projects
build successfully and their worker files pass available Python, PowerShell,
PHP, and Node syntax checks. The isolated process-runtime integration run now
also passes with the standalone provider worker assets: with
`SALAMATRIX_WORKER_ROOT` explicitly set to
`build\verification\command-state\worker-root`, the Python/PowerShell/PHP
process test executable returned exit code 0 and completed the SMX1 host-call,
persistent-session, UI, storage, event, picker, command-state, shutdown,
output-capture, and timeout scenarios. The lifecycle assertions verify the
append-only `IRuntimeSession::GetDiagnostic` contract for running, explicit
host stop, clean exit, and nonzero failed exit, including cached process id,
exit code, error code, and bounded message. The provider projects contain the
same diagnostic behavior even though the process-runtime executable exercises
the Automation-side adapter. No Salamander process was started or controlled.

The file-picker option slice was additionally rebuilt into
`build\verification\file-picker-options`. The explicit worker-root run
verified `filter` and `save=true` for Python, PowerShell, and PHP without
starting or controlling Salamander.

The Plugin Manager/AI integration slice was verified separately in
`build\verification\regressions`: the core Debug x64 build and standalone
`SalamatrixAI.SPL` Debug x64 build both passed, and the source-contract test
verified the four runtime labels, the localized AI chat command, and the
framework-first unload guard. The AI helper does not add a runtime dependency
or installer; it continues to consume the shared Salamatrix services. No
Salamander process was started or controlled.

Manifest settings migrations are intentionally host-side: the Automation
discovery layer applies the bounded typed rename/remove chain before publishing
the shared `storage.schema()` view. All four provider workers therefore keep
the same storage wire contract and receive already-migrated values without a
runtime-specific migration implementation. The same workers expose
`storage.keys()` (Python), `Storage.Keys()` (PowerShell), `storage->keys()` (PHP),
and `Storage.keys()` (Node); the host returns typed UTF-8 key records in
deterministic case-insensitive order.

`RuntimeSessionDiagnostic` is a bounded value snapshot. It reports lifecycle
state, process id, exit code, and a host/provider error code without exposing a
process handle or provider-owned string. Current process providers additionally
retain the process id and exit code after `Stop()`, report explicit host stops
as `Stopped`, clean exits as `Exited`, nonzero exits as `Failed`, and include a
bounded message. The default ABI-compatible implementation still derives the
running/exited state from the existing session methods, so older providers
remain usable while newer providers can append richer diagnostics.

## Tab lifecycle event bridge

The native Salamatrix.Events service keeps its original vtable and event
payload prefix. Event kinds 16–20 are append-only:

| Event name | Meaning |
| --- | --- |
| tabCreated | A tab id exists in the current side snapshot but not the previous one. |
| tabClosed | A previous tab id is absent from the current side snapshot. |
| tabReordered | The tab order changed while the side retained the same tab ids. |
| windowDetached | A tab's detached flag changed from clear to set. |
| windowAttached | A tab's detached flag changed from set to clear. |

The host compares heap-backed left/right tab snapshots when the core emits
PLUGINEVENT_TABCHANGED. The first snapshot is only a baseline, including an
empty side; the next transition from zero tabs therefore reports tabCreated.
Inferred lifecycle events are published before the existing tabChanged
notification. Their appended payload fields are changedTabId, tabIndex, and
previousTabIndex; tabId in the worker JSON frame remains the active tab id for
compatibility. Older V1 payloads are still accepted for legacy events and
receive 0/-1 defaults for the appended JSON fields.

Manifest extensions may subscribe only to event names in the existing Plugin
Manager manifest allow-list. The native Events schema, runtime event bridge,
and SalamatrixAI focused events API slice publish the same names. This is
snapshot inference, not a new core Salamander event ABI; direct
create/close/reorder/window hooks remain a later GAP item.

The slice was rebuilt into build\verification\tab-lifecycle and verified by
the native event tests, manifest parser tests, runtime protocol/schema tests,
Automation/Salamatrix Debug x64 builds, all four standalone provider builds,
all four worker syntax checks, and the explicit isolated
SALAMATRIX_WORKER_ROOT Python/PowerShell/PHP process-runtime test. No
Salamander process was started or controlled.

## Tab mutation worker contract

The implemented Sides version 1.3 contract exposes these host calls:

| Host method | Arguments | Return |
| --- | --- | --- |
| `salamander.sides.createTab` | `{side:string, path?:string, index?:int}` | `{created:true, tabId:string}`; `tabId` is decimal text. |
| `salamander.sides.closeTab` | `{tabId:string}` | `{ok:true}` |
| `salamander.sides.reorderTab` | `{tabId:string, index:int}` | `{ok:true}` |
| `salamander.sides.moveTab` | `{tabId:string, side:string, index?:int}` | `{ok:true}` |
| `salamander.sides.setDetached` | `{detached:bool}` | `{ok:true, detached:bool}` |

Python exposes these as `source_side.create_tab(path=None, index=-1)`,
`close_tab`, `reorder_tab`, `move_tab(side, index=-1)`, and
`set_detached`. Node, PHP, and PowerShell retain their existing
camelCase/PascalCase naming conventions. The deterministic bootstrap and
dispatcher-count test is isolated under
`build\verification\tab-object-model`; it does not start or control
Salamander.

## SalamatrixAI ownership

`SalamatrixAI.SPL` is the sole native owner of the local assistant workflow.
Its menu extension opens the chat and owns the complete context, localization,
repair/refinement, preview, clipboard, save, run, and extension-package export
flow. The package writer emits the manifest and runtime entry script for the
existing Plugin Manager discovery path.

The Automation plugin remains an independent legacy JavaScript/VBScript host
and shared Salamatrix runtime client. It no longer contains the assistant
command or its interactive workflow, but continues to provide the shared
`Salamatrix.AI` and `Salamatrix.ScriptRunner` services used by compatible
clients. `IScriptRunner::RefreshExtensions` is append-only and lets the AI
helper request manifest discovery without coupling to Automation internals.

The ownership restoration was verified by the source-contract test and
isolated Debug x64 builds in `build\verification\ai-restored-logic-final` and
`build\verification\automation-restored-logic-final`; no Salamander process
was started or controlled.

## Shutdown and unload safety

The runtime providers borrow the `Salamatrix.Runtime` broker; they do not own
it. During application shutdown the Framework provider can therefore be
unloaded before a standalone runtime provider. Each provider now validates the
currently published `SALAMATRIX_SERVICE_RUNTIME` pointer before unregistering
its adapter. A missing or replaced broker causes only the provider's local
registration state to be reset; `UnregisterAdapter` is called only while the
original broker is still live.

This prevents the stale-broker virtual call reported by the JavaScript runtime
shutdown crash. The four providers were rebuilt in the isolated
`build\verification\shutdown-guard-*` directories and the source regression
test passed. No Salamander process was started or controlled.

The shared runtime header includes `src/darkmode.h` directly instead of
depending on a provider-specific precompiled header to expose dark-mode
declarations. This keeps `Salamatrix.Poc` consumers such as `demoplug`
self-contained while retaining each project's existing `USE_DARKMODELIB`
configuration. The isolated `demoplug` Debug x64 verification build passed.

The shared `src/res/sal_r.ico` artwork is intentionally used only by the
Salamatrix Framework and SalamatrixAI plugins. The four standalone runtime
providers do not define a custom plugin icon resource and therefore keep the
standard Plugin Manager fallback icon. SalamatrixAI registers resource ID
1030 through its normal GUI icon-list callbacks during `Connect`.

The standalone SalamatrixAI provider initializes its borrowed
`CSalamanderGUIAbstract` pointer during `SalamanderPluginEntry`, before
`Connect()` is reached. Its dynamic `Ask Salamatrix AI...` command is added
with `MENU_SKILLLEVEL_ALL` and remains enabled while the shared Framework
services are resolved lazily. Selecting the menu item or its Plugin Manager
keyboard shortcut therefore reaches the same `ExecuteMenuItem()` path;
`ShowChat()` still reports the existing Framework-not-loaded warning when the
optional services are unavailable.
