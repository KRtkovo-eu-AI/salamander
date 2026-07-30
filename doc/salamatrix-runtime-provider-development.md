# Developing a Salamatrix language runtime provider

Salamatrix language runtimes are optional Open Salamander plugins. A runtime
provider registers one language adapter with `Salamatrix.Runtime`, starts the
language interpreter out of process, and translates between the interpreter
and Salamander through the language-neutral SMX1 protocol.

This design keeps language engines out of `SALAMATRIX.SPL` and the Automation
plugin. It also means that a provider can support a system-installed
interpreter without linking its SDK or redistributing it. The built-in
PowerShell, CPython, PHP CLI, Node, and Lua providers are working reference
implementations.

## Choose the process model

An out-of-process CLI adapter is the recommended starting point for a new
language:

- crashes and global interpreter state stay outside Salamander;
- the provider only depends on Win32, the Salamander plugin SDK, and the
  Salamatrix runtime headers;
- users can update or replace the interpreter independently;
- every language receives the same capability-checked host API over SMX1.

An in-process adapter is possible through `IRuntimeAdapter`, but it owns the
additional ABI, allocator, threading, shutdown, and interpreter-lifetime risks.
Do not expose Salamander C++ objects or COM pointers directly to scripts.

## Components

A complete provider contains four parts:

1. A normal `.SPL` plugin that owns an `IRuntimeAdapter`.
2. Interpreter discovery and process/session management.
3. A language-specific worker bootstrap that exposes the `Salamander` facade.
4. Build, packaging, catalog, documentation, and test integration.

Use these files as references:

- `src/plugins/powershellruntime/powershellruntime.cpp` for plugin lifecycle,
  interpreter discovery, one-shot execution, and persistent pipes;
- `src/plugins/powershellruntime/runtime/salamatrix_worker.ps1` for a worker
  facade in a dynamic language;
- `src/plugins/salamatrix/salamatrix_runtime_api.h` for the native ABI;
- `src/plugins/salamatrix/salamatrix_runtime_protocol.h` for SMX1 framing;
- `doc/salamatrix-automation-api.md` for the script-facing API.

## 1. Describe the adapter

Implement `Salamatrix::Runtime::IRuntimeAdapter` and return a stable
`RuntimeAdapterDescriptor`:

```cpp
m_descriptor.RuntimeId = "Example.CLI";
m_descriptor.DisplayName = "Example CLI runtime provider";
m_descriptor.LanguageId = "example";
m_descriptor.FileExtensions = ".example";
m_descriptor.RuntimeVersion = 0x00010000;
m_descriptor.Flags =
    Salamatrix::Runtime::RuntimeAdapterFlagOutOfProcess |
    Salamatrix::Runtime::RuntimeAdapterFlagPersistentExtensions;
```

`RuntimeId` is part of extension manifests and AI output, so changing it breaks
existing packages. Use a language-neutral dotted identifier when several
engines could implement the same language. `FileExtensions` is a
semicolon-separated list if the adapter accepts more than one suffix.

`IsAvailable()` must be cheap after its first call. The existing providers
resolve an explicit `SALAMATRIX_<LANGUAGE>` environment variable first and then
search a short list of executable names with `SearchPathW`. An environment
variable may contain either a full path or a command name.

`SupportsEntryPoint()` should reject unavailable runtimes and compare the
entry-point suffix case-insensitively.

## 2. Register without taking ownership of the framework

The provider and `SALAMATRIX.SPL` have independent plugin lifetimes. Query
`SALAMATRIX_SERVICE_RUNTIME` with minimum version
`SALAMATRIX_RUNTIME_VERSION_1_0`, then register the adapter through
`RuntimeProviderRegistration`.

Registration may not be possible during `SalamanderPluginEntry` when the
framework has not loaded yet. Retry from `Connect` and `Event`. Before
registering, use `FindAdapter(runtimeId, 0)` to avoid a duplicate.

On `Release`, unregister only when the currently published runtime service is
the exact service stored by the registration helper. If the framework has
already unloaded, discard the borrowed registration state without calling
through the stale interface. The standalone providers demonstrate this unload
guard in `UnregisterRuntimeProvider`.

The provider must set `SetFlagLoadOnSalamanderStart(TRUE)`. Its adapter object
and descriptor strings must remain alive for the whole registration.

## 3. Implement one-shot execution

`Execute()` is the direct CLI path:

1. Validate `StructSize`, pointers, availability, and entry-point suffix.
2. Build a correctly quoted Unicode command line.
3. Set the working directory to `WorkingDirectory`, or to the entry-point
   directory when it is absent.
4. redirect stdin to `NUL` and capture stdout/stderr through a pipe;
5. start the interpreter with `CREATE_NO_WINDOW`;
6. enforce the request timeout with an absolute upper bound;
7. drain bounded output, preserve the process ID and exit code, and fill every
   field in `RuntimeExecutionResult`.

Never pass an untrusted command line through `cmd.exe`. Reject embedded quotes
unless the provider has a complete Windows argument quoting implementation.
Keep output bounded; the reference providers capture at most 1 MiB and copy a
bounded summary into the ABI result.

## 4. Implement persistent sessions

Manifest extensions normally use `StartPersistent()`. Create two anonymous
pipes:

- host writes to worker stdin;
- host reads from worker stdout;
- worker stderr goes somewhere other than the protocol stream.

When `RuntimeExecutionFlagUseWorkerBootstrap` is set, start the provider's
worker file and pass the entry point plus optional command ID and handler. When
`RuntimeExecutionFlagOneShotWorker` is also set, ask the worker to exit after
the entry point returns.

The returned `IRuntimeSession` owns the process, thread, and pipe handles. It
must:

- send complete UTF-8 SMX1 lines;
- receive one bounded line at a time while preserving unread bytes;
- pump worker calls through `RuntimeHostDispatchProc`;
- translate dispatch success to `result` and failure to `error` frames;
- stop and close handles synchronously;
- report stable running, exited, stopped, and failed diagnostics;
- never leave a worker capable of calling unloaded provider code.

`Release()` deletes the session. `Stop()` must be idempotent. A provider may
terminate an unresponsive worker, but it must first close the input channel and
use bounded waits.

## 5. Write the worker bootstrap

SMX1 is a line protocol:

```text
SMX1<TAB>kind<TAB>decimal-id<TAB>compact-json<LF>
```

Each encoded frame, including the newline, is limited to 1 MiB. Worker stdout
is reserved exclusively for these frames. Redirect or replace the language's
normal `print` facility so extension output cannot corrupt the transport.

The worker starts with:

```text
SMX1	hello	0	{"protocol":1,"runtime":"example"}
```

It waits for `result` ID `0`, builds the language-native `Salamander` object,
and executes the entry point. Host calls use monotonically increasing IDs and
a payload containing `method` plus its arguments. While waiting for a result,
the worker must dispatch interleaved `event` frames. A `shutdown` frame ends
the persistent loop.

Implement JSON inside the worker without requiring an optional language
package, or ship and license a dependency beside the provider. Preserve JSON
strings, booleans, signed integers, arrays, objects, and null. Reject malformed,
oversized, or non-SMX1 input instead of attempting to recover silently.

The facade should cover the same method vocabulary as the other modern
workers: commands, storage, file operations, sides/tabs, UI and dialogs,
clipboard, AI, events, runtime enumeration, and host appearance/language.
Keep language naming idiomatic, but do not change host method names or payload
fields.

## 6. Add the project and package

Follow the standalone provider layout:

```text
src/plugins/<language>runtime/
  <language>runtime.cpp
  <language>runtime.h
  <language>runtime.def
  <language>runtime.rc
  runtime/salamatrix_worker.<suffix>
  vcxproj/<language>runtime.props
  vcxproj/<language>runtime.vcxproj
```

The project output belongs under:

```text
plugins/extension-runtimes/<project-name>/
```

Copy the worker to the provider's `runtime` subdirectory after the build. Add
the project to `src/vcxproj/salamand.sln`, include both the `.spl` and worker in
the installer, declare a dependency on `salamatrix`, and add a catalog entry.
Do not make the runtime an Automation dependency and do not bundle an
interpreter unless its redistribution, updates, architecture, and licenses are
an explicit product decision.

An extension selects the adapter by its stable runtime ID:

```json
{
  "id": "example.demo",
  "version": "1.0.0",
  "runtime": "Example.CLI",
  "entryPoint": "main.example",
  "capabilities": ["panels.read", "ui.dialogs"]
}
```

## 7. Verify the provider

Verification should include:

- Debug and Release x64 project builds;
- project XML and package path checks;
- worker syntax checks with a real interpreter;
- interpreter discovery through both the environment override and `PATH`;
- one-shot output, nonzero exit, and timeout cases;
- SMX1 hello/result/call/error/event/shutdown framing;
- persistent session diagnostics and unload ordering;
- the isolated process-runtime integration test with
  `SALAMATRIX_WORKER_ROOT`;
- a visible demo extension that exercises representative host APIs.

Use a disposable worker root containing only the bootstrap files under test.
The process-runtime tests do not require starting Salamander.
After building or staging the package, verify its PE exports and colocated
worker without loading the plugin:

```powershell
.\tools\verify_runtime_packages.ps1 `
  -SalamanderPath .\build\salamander\Release_x64 `
  -Architecture x64 -Plugin <language>runtime
```

## Lua reference provider

`LuaRuntime.SPL` follows this model with runtime ID `Lua`, language ID `lua`,
and `.lua` entry points. It resolves `SALAMATRIX_LUA`, then `lua.exe`,
`lua55.exe`, or `lua54.exe`. The provider does not redistribute Lua.

The worker supports stock Lua without external modules. It supplies its own
bounded JSON codec, reserves stdout for SMX1, exposes the global `Salamander`
table, and executes the entry point with `dofile`. A package can therefore use:

```lua
local path = Salamander.sides.context("source").path
Salamander.ui.notify("Current path: " .. path, "Lua extension")
```

Users with another executable name or a private Lua installation can set
`SALAMATRIX_LUA` to the executable path before starting Salamander.
