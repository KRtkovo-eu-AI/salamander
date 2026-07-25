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
    if (broker == NULL)
        return NULL; // Salamatrix.SPL must be loaded first.

    if (!providerRegistration.Register(broker, &pythonAdapter))
        return NULL;
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
| `JavaScriptRuntime.SPL` | `JavaScript.Node` or future QuickJS id | `SALAMATRIX_NODE`, `node.exe`, `node` |

Automation keeps its legacy JScript/VBScript ActiveScript adapters. It becomes
just another broker consumer for the providers above; installing or loading a
provider must not require Automation. A manifest's runtime id and minimum
version select the provider through the broker, while a missing provider is a
clear unavailable-runtime result.

The current branch has the broker contract, worker protocol, and provider
lifecycle helper. The CPython/PowerShell/PHP process implementations still
live in Automation and are the next extraction targets; no separate runtime
`.SPL` is claimed until its project, adapter, worker assets, load/unload path,
and integration test exist.
