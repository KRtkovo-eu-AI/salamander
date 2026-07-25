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
