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
the normal dialog `get`/control-text mechanism. It is a folder chooser button,
not yet an editable text field with a separate browse button.

Verification at the current pause point: all four provider Debug x64 projects
build successfully and their worker files pass available Python, PowerShell,
PHP, and Node syntax checks. The end-to-end process runtime test binary also
builds, but cannot run in this environment until `SALAMATRIX_WORKER_ROOT` is
configured.
