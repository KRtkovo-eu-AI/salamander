# Standalone Salamatrix runtime providers

Runtime providers are optional Salamander plugins (`.SPL`, i.e. DLLs). They
are not interpreter installers and they do not depend on the Automation plugin.
The user installs Python, PowerShell, PHP, or Node separately; the provider
only discovers that executable, owns its worker bootstrap, and registers an
adapter with the already loaded `Salamatrix.Runtime` broker.

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
