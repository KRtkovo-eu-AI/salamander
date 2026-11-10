# Salamatrix extension roles

Start from the closest current manifest and handler implementation. The manifest parser in `src/plugins/salamatrix/salamatrix_manifest.cpp` is the authority for accepted fields, ranges, path rules, and schema compatibility.

## Menu and Extension Bar commands

A manifest command maps a stable command `id` and localized `title` to a `handler` in the package entry point. Existing examples use fields such as `menu`, `contextMenu`, `toolbar`, `toolbarMenu`, icon variants, `requires`, and (where appropriate) `requiresExecutable`. These flags control contribution and availability; they do not implement the action. The entry-point code must dispatch the supplied handler and use Salamatrix host APIs for panel context, dialogs, file operations, storage or notifications.

Use the smallest placement that serves the request: Plugin menu, panel context menu, Extension Bar button, or a toolbar button that opens the package's contributed menu. Check the current parser and `git-worktree-navigator`/`file-lock-inspector` samples for valid placement values. A toolbar button that opens a command menu must follow the parser's `toolbarMenu` constraints. Never claim a menu or toolbar contribution is implemented if it exists only in UI mockups.

## File Viewer associations

A `viewers[]` entry binds one or more short filename masks to a handler; it may also provide an optional display `name`. Viewer patterns are masks, not paths. Read the file from the invocation provided by the host, handle empty/unreadable/large input deliberately, and use the shared `Salamander.ui.viewer(path, options)` when opening a rendered document. Do not launch a second viewer process or hand-build host window chrome when the shared Viewer contract covers the need.

The minimal demos use the host's ordinary file preview/message-box APIs to demonstrate the handler. For an actual viewer experience, compare `src/extensions/json-viewer-nodejs/` and the current `doc/salamatrix-ui.md`; renderer selection, source path, display title and temporary-content lifecycle matter. Restart Salamander after installing/changing Viewer associations because native masks are registered during framework startup.

## Virtual file-system panel

A `fileSystems[]` entry defines a provider id, name, list handler, optional root items/icons, typed columns, refresh behavior and declared actions. A command can open it at `salamatrix:<extension-id>!<provider-id>`; use the exact extension id and FS id. The extension implements listing and action handlers and returns items through the runtime's `file_system`/`fileSystem` facade. Inspect `src/extensions/demos/<runtime>/main.*` for the common item fields (`id`, `name`, `directory`, `enabled`, icon, custom `columns`) and action invocation shape.

Model navigation as stable item IDs and relative provider paths. Return bounded, correctly typed rows; use `directory` consistently; give declared custom-column values in each item's columns map. Match each manifest action handler and ID, decide which action is default, filter actions by item prefix only when needed, and request refresh only after state-changing operations that affect the current listing. Use `rootItems`, `refreshDepth`, and `refreshPaths` only when hierarchical roots and targeted refresh are useful. See `doc/salamatrix-filesystems.html` and the Event Viewer, Process Explorer and Hardware Monitor implementations for details.

## Combining roles

One package can declare commands, `viewers[]`, and `fileSystems[]` together. Keep role IDs and handlers unique and make each route independently testable. A command may navigate to its own FS provider, while a Viewer handler may use the same entry point as other commands. Declare the capabilities needed for each route and no broader set; the Plugin Manager reports these roles from manifest metadata.

## Shared runtime, localization, and security

Use the facade spelling and async behavior of the selected runtime, while preserving equivalent host methods and payload semantics across JavaScript, Python, PowerShell, PHP, and Lua. The checked-in five-runtime demos are the quickest parity reference. Do not use runtime-native UI/process APIs when the shared Salamatrix service is the intended surface.

Keep every visible name, action, column, root, notification, and dialog text localizable. Use package `locales` and the manifest localization structures supported by the current parser. Follow the repository's established light/dark icon conventions where icons are supplied.

Treat `capabilities` and `security` as both disclosures and host policy inputs. Read `doc/salamatrix-platform.md` and existing package declarations to choose valid capability names; do not guess permissions. Ask before adding an external side effect that is not part of the user's request. For destructive handlers, validate the target and selection, provide a clear result, and preserve Salamander's existing interactive file-operation path where applicable.
