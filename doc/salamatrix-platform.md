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

Recommended shape:

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

For a smaller MVP, this can be reduced to:

```cpp
virtual void* WINAPI QueryService(
    const char* serviceId,
    DWORD minimumVersion,
    DWORD* providedVersion) = 0;
```

A richer result structure is preferred because it allows diagnostics, provider
identification, and future flags without changing the ABI again.

### Provider registration

Runtime plugins should register services during their plugin entry/init phase,
after their own version check and language/resource initialization succeed.

Recommended shape:

```cpp
virtual BOOL WINAPI RegisterService(
    const char* serviceId,
    DWORD version,
    void* serviceInterface,
    CPluginInterfaceAbstract* providerPlugin) = 0;
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
6. The next MVP can implement `Salamatrix.UI` progress dialog without revisiting
   the naming and service-discovery foundation.
