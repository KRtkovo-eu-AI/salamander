# Salamatrix Automation API reference

This document describes the API that is implemented by the current Salamatrix
Framework and exposed to scripted extensions. It is intended for both people
writing scripts and AI providers generating scripts. Examples use public worker
facades, not the private `SMX1` transport.

The rule is simple: a generated script may combine the documented Salamander
API with normal facilities of its selected language runtime. A missing Python
package, Node package, PowerShell module, or PHP extension is an environment
concern, not a missing Salamander capability. `canImplement=false` is reserved
for a host integration that the documented Salamander API genuinely does not
provide.

## Runtime model

| Runtime id | Script style | Salamander calls |
| --- | --- | --- |
| `JavaScript.Node` | ECMAScript module (`.mjs` or `.js`) | asynchronous; use `await` |
| `Python.CPython` | Python module | synchronous |
| `PowerShell` | PowerShell script | synchronous |
| `PHP.CLI` | PHP script | synchronous |
| `Lua` | Lua script | synchronous |

The worker injects one root object:

| Runtime | Root object |
| --- | --- |
| JavaScript | `Salamander` |
| Python | `Salamander` |
| PowerShell | `$Salamander` |
| PHP | `$Salamander` |
| Lua | `Salamander` |

JavaScript scripts run as modules and may use top-level `await`. They must not
use an invented `this.selectedItems` property. The current selection is obtained
through `await Salamander.sides.context("source")`.

### One-shot scripts and extension packages

The generated source contract is the same for a one-shot preview and for a
saved extension. A one-shot script is executed immediately with the selected
runtime. **Save as extension** creates a directory containing `extension.json`
and the generated `main` entry point, then asks Salamatrix to refresh extension
discovery. The assistant returns source code, not a JSON manifest embedded in
the source.

The generated manifest uses schema version 2 and binds one plugin-menu command
to handler `main`:

```json
{
  "schema": 2,
  "id": "generated.example",
  "name": "Generated example",
  "version": "1.0.0",
  "description": "What the extension does",
  "runtime": "Python.CPython",
  "entryPoint": "main.py",
  "capabilities": ["panels.read", "ui.dialogs"],
  "commands": [
    {
      "id": "generated.example",
      "title": "Generated example",
      "handler": "main",
      "menu": "plugin",
      "requires": "any"
    }
  ]
}
```

Hand-authored packages may define up to 64 commands. Command fields are `id`,
`title`, `handler` or `path`, `menu`/`placement`, `contextMenu`, `toolbar`,
`toolbarMenu`, and `requires`. Supported `requires` values are `any`, `disk`,
`focused`, `file`, and `selection`. Entry points must be safe relative paths;
absolute paths and parent traversal are rejected. Manifests are strict UTF-8
JSON and reject duplicate members, invalid types, duplicate command ids, and
unsupported schema versions.

A command with `path` and no `handler` changes the active/source panel directly
on the host UI thread and does not start a runtime worker. This is intended for
toolbar shortcuts to fixed disk, archive, or plug-in FS paths, requires the
`panels.write` capability, and is mutually exclusive with `handler`.

Schema 2 adds two optional native roles while schema 1 remains accepted:

- `viewers[]` registers one or more file masks, an optional user-visible
  `name`, and a one-shot `handler`. Viewer configuration lists every registered
  package/Viewer identity as a separate **Associated viewer** choice instead of
  showing only the shared Salamatrix Framework plugin. The selected identity is
  preserved with the association across configuration saves and restarts and
  is carried to the framework when that association is invoked. Therefore a
  concrete extension Viewer selected manually for `*.*` (including as an
  alternate Viewer opened with Alt+F3) runs that exact handler even when the
  opened file does not match the Viewer's original manifest mask.
  Declarative Viewer identities and masks are registered independently of
  runtime-provider startup order; dispatch still requires the runtime to be
  available when the Viewer is opened.
  The handler receives `Salamander.invocation` with `role="viewer"`, the local
  `path`, suggested window geometry, show state, always-on-top state, and the
  optional enumeration source/index. Viewer associations are registered during
  Salamatrix startup; installing or changing a viewer package requires a host
  restart before new masks enter the global viewer association list.
- `fileSystems[]` contributes a provider under `salamatrix:`. Each provider
  declares `id`, `name`, `listHandler`, optional `openHandler`, SVG icons,
  optional package-relative ICO `defaultFileIcon`, `refreshIntervalMs` (`0`
  disables periodic refresh), optional
  `refreshDepth` (minimum virtual-path depth for timer refreshes), optional
  `refreshPaths[]` (provider-relative paths eligible for timer refreshes), optional
  declarative directory-only `rootItems[]`, detailed-view `columns[]`, and item
  `actions[]`. A non-empty `rootItems[]` is rendered synchronously at the provider
  root without starting its runtime worker; locale resources can translate names
  through `fileSystems.<fileSystemId>.rootItems.<itemId>`. `refreshDepth` defaults
  to `0`, preserving periodic refresh at every level. When `refreshPaths[]` is
  non-empty, timer refresh is further limited to exact matching paths; manual
  refresh always reloads the current path.
  A column declares `id`, `name`, optional `description`, `width`, `numeric`,
  `size`, and `dateTime`. A `size` column carries an unsigned byte count and is formatted
  using the user's Configuration > Panels > Show sizes choice (bytes, KB, or
  short mixed units); it is implicitly numeric. A `dateTime` column carries
  Unix milliseconds in each item's `columns` value. It sorts by that numeric
  UTC instant and displays date and time using the user's Windows regional
  format and time zone.
  The list handler calls `Salamander.fileSystem.addItems` /
  `file_system.add_items` once for a snapshot (or `addItem` / `add_item` for a
  small incremental list). The batch call returns the number of accepted items
  and avoids one synchronous worker/host round-trip per row. Item records contain
  `id`, `name`, optional `compactName` used outside Detailed view, optional package-relative SVG `icon`/`iconDark`, optional
  `fileIcon` containing the fully qualified Unicode path of a file whose embedded
  icon resource should be displayed, `directory`, and `enabled`, and an optional
  `columns` object mapping declared column ids to display strings. File icons are
  extracted lazily and directly from the file, without consulting file-extension
  associations; an unavailable path or a file without an icon resource first
  falls back to the provider's `defaultFileIcon`, then to the Salamatrix icon.
  Columns are rendered and sorted by the native detailed panel;
  numeric columns use numeric ordering. An executable action declares `id`,
  `title`, `handler`, optional `default`, and optional `refresh` (true by
  default). Optional `itemIdPrefix` limits an action (or separator) to matching
  item ids; a standalone `{ "separator": true }` inserts a native menu separator.
  Only an explicitly default action executes on Enter. The native Unicode context
  menu exposes the declared actions in order, and locale resources may translate
  their captions through `fileSystems.<fileSystemId>.actions.<actionId>`.
  Provider contents include a native `..`
  item, Directory Line exposes clickable breadcrumb segments, and the main
  window honors full, shortened, and directory-only path display modes.
  Runtime list handlers execute on a background worker. The panel immediately
  returns cached rows and refreshes itself when a new snapshot is ready, so a
  slow runtime or a large provider cannot block Salamander's UI thread. While an
  initially empty snapshot is being prepared, Salamander displays the panel
  throbber and stops it automatically when the completed listing is installed.
  Rename/copy/move/delete/upload and complex hierarchical navigation are
  intentionally unsupported in the flat v1 role.

## Virtual file-system panels

Schema-2 extensions can contribute a flat virtual file system through `fileSystems[]`. Salamander exposes these providers under `salamatrix:<extension-id>!<file-system-id>` paths and opens them in the ordinary file panels; the provider supplies roots, rows, typed detailed-view columns, package icons, refresh policy, and selection actions while the host owns navigation, rendering, sorting, context menus, and dark-mode behavior.

The distributed Event Viewer, Process Explorer, and Hardware Monitor extensions use this role for native-feeling diagnostic panels. This surface is distinct from `Salamatrix.Sides`, which reads the ordinary panel selection, and from `Salamatrix.FileOperations`, which operates on regular files.



The distributed Salamatrix extension demos (`src/extensions/demos/README.md`)
include equivalent schema-2 Viewer and FS handlers for Node.js, Python,
PowerShell, PHP, and Lua, each with a distinct sample file mask and provider.
The existing Plugin Manager derives each package's **Functions** summary from
these manifest contributions: `commands[]` is shown as **Menu Extension**,
`viewers[]` as **File Viewer**, and `fileSystems[]` as **File System (FS)**.

An extension must declare every applicable gated framework surface it calls.
Public capability names are `panels.read`, `panels.write`, `ui.dialogs`, `commands`,
`file-operations`, `file-system`, `storage`, `events`, `ai`, `clipboard`, and `runtimes`.
For backward compatibility, omitting `capabilities` keeps the legacy unrestricted
behavior. An explicitly empty `capabilities: []` is deny-all, and any non-empty
list grants only the named surfaces (or `*`).
Application language/appearance and other host metadata have no separate
capability name.
Direct use of language/runtime libraries does not add a Salamatrix capability,
but its real effects must still be reported by AI preview metadata.

The entry point executes at top level. Its invocation identity is available as
`commandId`/`commandHandler` in JavaScript, `command_id`/`command_handler` in
Python, PHP, and Lua, and `CommandId`/`CommandHandler` in PowerShell. Persistent
event subscriptions are useful only for an activated extension whose runtime
supports persistent sessions; a one-shot script normally exits before a future
event can arrive.

Modern workers also expose the same parsed `Salamander.invocation` object. It is
empty for ordinary commands and carries role-specific data for Viewer and FS
list/action invocations. The runtime providers pass this append-only context as
strict JSON and preserve Unicode and embedded quotes through Windows command-line
escaping.

### How the AI consumes this contract

The live `Salamatrix.AI` service publishes versioned machine-readable slices
for invocation context, extension manifests, sides, file operations, commands,
UI, storage, clipboard, application state, events, runtime discovery, and AI.
The assistant selects slices from the user's task instead of sending an
unbounded copy of the whole manual. A request explicitly asking for the whole
framework receives every public slice.

The bundled local model receives these slices through a **Strict Interface
Contract**, split into five explicit sections:

1. `INPUT CONTRACT`: typed task, selected runtime id, current Salamander context,
   existing source, and optional repair feedback, including their actual values;
2. `INSTALLED SALAMANDER API CONTRACT`: only the relevant implemented API
   slices;
3. `SELECTED RUNTIME CONTRACT`: the real worker facade conventions for the
   selected JavaScript, Python, PowerShell, PHP, or Lua runtime;
4. `OUTPUT CONTRACT`: the exact closed JSON Schema for the response;
5. `GENERATION RULES`: precedence, grounding, capability, effect, and honest
   framework-GAP rules.

The exact same output schema instance is included in the prompt and supplied to
`llama.cpp` through `--json-schema-file`. It therefore acts both as instructions
for the model and as a constrained-generation grammar. The runtime contracts
document the injected root object, synchronous or asynchronous call model,
naming convention, selected-item access, and use of language libraries. Full
general-purpose language manuals are deliberately not inserted into every
request because they would consume context without describing the Salamatrix
binding.

The local llama.cpp companion offers two Q4_K_M profiles. The recommended
Qwen2.5-Coder 1.5B Instruct model is intended for normal multilingual tasks.
The lightweight 0.5B profile is retained for constrained machines and accepts
English prompts only. Both are run in one-turn conversation mode using the
chat template embedded in their GGUF metadata. The invariant API, runtime, and
output contracts are sent as the system message; the typed task input is sent
as the user message.

For operations with a verified implementation recipe, the provider may also
constrain semantic fields and source code through JSON Schema constants. This
is intentional: prose documentation helps the model choose an API, while the
schema prevents a small local model from replacing a verified recipe with
invented source. Static validation then checks cross-field facts such as:

- `sides.context` implies `readSelection=true`;
- a file writer implies `modifyContents=true`;
- Node module scripts use `import`, not CommonJS `require`;
- Lua scripts use the injected global `Salamander` table and standard Lua
  libraries available in the provider package;
- `selectedItems` must originate from `Salamander.sides.context`;
- a known implementable recipe must not be reported as a framework GAP.

The concrete validation message is returned to the repair loop and to the UI.
An invalid response must not be reduced to the generic statement that the
model returned no valid script.

## Side names and current context

Every API accepting a side uses one of:

- `source`: the panel from which the command was invoked;
- `target`: the opposite panel;
- `left`: the physical left panel;
- `right`: the physical right panel.

`source` and `target` are normally the correct choice for reusable automation.

### Reading selected files

The canonical operation is `sides.context(side)`.

| Runtime | Call |
| --- | --- |
| JavaScript | `await Salamander.sides.context("source")` |
| Python | `Salamander.sides.context("source")` |
| PowerShell | `$Salamander.Sides.Context("source")` |
| PHP | `$Salamander->sides->context("source")` |

It returns:

```json
{
  "ok": true,
  "path": "C:\\Work",
  "pathType": 1,
  "selectedCount": 2,
  "selectedItems": [
    {
      "name": "one.txt",
      "path": "C:\\Work\\one.txt",
      "extension": "txt",
      "size": 123,
      "sizeValid": true,
      "attributes": 32,
      "lastWriteUtc": 134000000000000000,
      "isDirectory": false,
      "hidden": false,
      "link": false,
      "offline": false
    }
  ],
  "focusedItem": null
}
```

`selectedItems` is bounded to 64 returned records. `selectedCount` reports the
complete selection count, so scripts that require every selected record must
detect `selectedCount > selectedItems.length` and explain the limitation rather
than silently processing only the prefix.

### Side and tab methods

The generic facade is `Salamander.sides`; runtimes also provide side-bound
facades such as `sourceSide`/`source_side`.

| Operation | Parameters | Result |
| --- | --- | --- |
| `activeTab` | `side="source"` | active tab object |
| `context` | `side="source"` | context object described above |
| `tabs` | `side="source"` | array of tab objects |
| `activateTab` | `tabId`, `focus=true` | boolean |
| `changePath` | `path`, `side="source"` | host result object |
| `refresh` | `side`, `force=false`, `focusFirstNewItem=false` | boolean |
| `selectItem` | `index`, `select=true`, `side`, `repaint=true` | boolean |
| `selectAll` | `select=true`, `side`, `repaint=true` | boolean |
| `focusItem` | `index`, `side`, `partVisible=true` | boolean |
| `createTab` | `side`, optional `path`, optional `index` | `{created, tabId}` |
| `closeTab` | `tabId` | boolean |
| `reorderTab` | `tabId`, `index` | boolean |
| `moveTab` | `tabId`, `side`, optional `index` | boolean |
| `setDetached` | `detached` | boolean |

Tab ids are decimal strings and must not be converted to floating-point
numbers. Tab objects contain `id`, `index`, `side`, `pathType`, `flags`, and
`path`.

## File contents versus Salamander file operations

The runtime language may read or write a path returned by `sides.context` using
its standard filesystem APIs. This is the correct approach for hashing,
parsing, creating sidecar files, or transforming contents.

`Salamander.fileOperations` is different: its methods invoke Salamander's
interactive commands for the current panel selection. They do not accept
arbitrary source and destination paths.

| Method | Parameters | Result |
| --- | --- | --- |
| `rename()` | none | textual command result |
| `copy()` | none | textual command result |
| `move()` | none | textual command result |
| `delete()` | none | textual command result |
| `createDirectory()` | none | textual command result |
| `refresh()` | none | textual command result |
| `properties()` | none | textual command result |

Python uses `file_operations.create_directory()`. PowerShell uses
`$Salamander.FileOperations.CreateDirectory()`. PHP uses
`$Salamander->file_operations->createDirectory()`.

### Complete JavaScript MD5 sidecar example

```javascript
import { createHash } from "node:crypto";
import { readFile, writeFile } from "node:fs/promises";

const context = await Salamander.sides.context("source");
if (context.selectedCount > context.selectedItems.length) {
  throw new Error("The selection exceeds the context record limit.");
}

for (const item of context.selectedItems) {
  if (item.isDirectory) continue;
  const digest = createHash("md5")
    .update(await readFile(item.path))
    .digest("hex");
  await writeFile(
    `${item.path}.md5`,
    `${digest} *${item.name}\r\n`,
    "utf8"
  );
}
```

This script uses Salamander capability `panels.read`. Its effects are
`readSelection=true` and `modifyContents=true`; the other six effect flags are
false. Node's `node:crypto` and `node:fs/promises` are runtime facilities and
must not be reported as missing Salamander capabilities.

## Commands

Commands belong to an extension and may appear in the plugin menu, panel
context menu, or toolbar.

| Operation | Parameters | Result |
| --- | --- | --- |
| `execute` | `commandId` | textual result |
| `register` | `commandId`, `title`, `pluginMenu=true`, `contextMenu=false`, `hotKey=0`, `toolbar=false`, `handler=""`, `enabled=true`, `visible=true` | boolean |
| `unregister` | `commandId` | boolean |
| `setState` | `commandId`, optional `enabled`, optional `visible` | boolean |

Python uses snake case (`set_state`, `plugin_menu`, `context_menu`, `hot_key`).
The current invocation is available as `commandId`/`commandHandler` in
JavaScript, `command_id`/`command_handler` in Python and PHP, and
`CommandId`/`CommandHandler` in PowerShell.

## Typed storage

Storage is scoped to the extension package. Supported value types are string,
integer, and boolean.

| Operation | Parameters | Result |
| --- | --- | --- |
| `get` | `key`, optional default | stored value or default |
| `set` | `key`, supported value | none/host result |
| `remove` | `key` | boolean |
| `clear` | none | boolean/host result |
| `schema` | none | declared settings records |
| `keys` | none | records containing `key` and `type` |

## Clipboard

`copyText(text, showEcho=false)` copies UTF-8 text and returns a boolean.
Python names it `copy_text`.

## Runtime discovery

`runtimes.list()` returns records with `id`, `name`, `language`, `extensions`,
`version`, and `available`. The runtime id placed in generated assistant output
must exactly equal the selected runtime id.

## Application language and appearance

`application.language()` returns the selected Salamander language record.
`application.appearance()` returns the current appearance record, including
whether Salamander is using its **Windows Dark Mode (experimental)** color
scheme. Extensions must use these host values for localized behavior and
appearance decisions; operating-system app mode is not the Salamander dark-mode
contract. Python and Lua use `application.language()` / `appearance()`,
PowerShell uses `$Salamander.Application.Language()` / `Appearance()`, PHP uses
`$Salamander->application->language()` / `appearance()`, and JavaScript awaits
the corresponding calls.

## Events

`events.subscribe(name, handler)` returns a subscription id.
`events.unsubscribe(subscriptionId)` removes it.

Implemented event names:

```text
hostStartup, hostShutdown, settingsChanged, configurationChanged,
colorsChanged, panelsSwapped, activePanelChanged, sidePathChanged,
sideSelectionChanged, sideTabChanged, sideRefreshed, pathChanged,
selectionChanged, tabChanged, tabCreated, tabClosed, tabReordered,
windowDetached, windowAttached, fileChanged
```

An AI must not invent an event name. Event callbacks are meaningful for saved
extensions; a one-shot script normally completes before future events arrive.

## Shared UI

The UI facade creates native Salamander windows and controls. Scripts should
use it instead of Win32, Tkinter, HTML, or another unrelated UI toolkit when
the required control exists.

### Immediate UI

| Operation | Main parameters | Result |
| --- | --- | --- |
| `messageBox` | message, title | integer dialog result |
| `notify` | message, title, timeout | boolean |
| `fileProperties` | fully qualified file path | `{shown, error}` |
| `inputBox` | prompt, title, initial value | `{accepted, value}` |
| `pickFile` | save, title, filter, initial | `{selected, path}` |
| `pickFolder` | title, initial | `{selected, path}` |
| `progress` | title, total, style flags, optional second total | progress object |

`fileProperties` invokes the native Windows File Properties sheet from the
long-lived Salamander host process through `SHObjectProperties` with
`SHOP_FILEPATH`. The path stays Unicode end-to-end; Python and Lua expose the
method as `file_properties`.
| `dialog` | title, width, height | dialog object |
| `uptime` | none | host uptime in milliseconds as a decimal string |

Python uses `message_box`, `input_box`, `pick_file`, and `pick_folder`.

File filters are pipe-separated description/pattern pairs, for example
`"Text files|*.txt|All files|*.*"`. Save mode enables overwrite prompting.

### Progress object

| Operation | Purpose |
| --- | --- |
| `update` | set position, optional total, text, paint mode, and second bar |
| `step` | increment the primary bar |
| `setTotals` | set both totals |
| `setPositions` | set both positions |
| `setTitle` | change the title |
| `setCancelEnabled` | enable or disable cancellation |
| `isCancelled` | query cancellation |
| `close` | destroy the progress UI |

Python uses snake-case names. Every loop doing substantial work should check
cancellation and close the progress object even when an operation fails.

### Dialog object

Supported controls are `label`, `statictext`, `textbox`, `checkbox`, `radio`,
`combobox`, `button`, `listview`, `treeview`, `tabcontrol`, `folderpicker`,
`filepicker`, `groupbox`, `hyperlink`, `progressbar`, `arrowbutton`,
`textarrowbutton`, `colorarrowbutton`, and `toolbarheader`.

The generic add operation accepts a control id, text, optional
`x/y/width/height`, and applicable options: `readOnly`, `checked`,
`dialogResult`, `keepOpen`, `multiline`, `filter`, and `save`. The complete
native option set additionally includes `styleFlags`, `pathSeparator`,
`toolTip`, `actionOpen`, `actionCommand`, `actionHint`, `progress`,
`progressCurrent`, `progressTotal`, `progressText`,
`indeterminateDuration`, `indeterminateInterval`, `textColor`,
`backgroundColor`, `alignControlId`, and `buttonMask`.

Every modern runtime forwards that same extended option set to the framework
package host. The package host owns the dialog independently of Automation,
routes change events back through SMX1, and destroys outstanding dialogs when
the extension deactivates. Automation JScript exposes a thin COM `UI.dialog`
adapter over the same `IDialog`/`IControl` service, while native plugins query
`SALAMATRIX_SERVICE_UI` directly.

Convenience methods include:

```text
addLabel, addTextBox, addFolderPicker, addFilePicker, addCheckBox,
addRadioButton, addComboBox, addListView, addTreeView, addTabControl,
addButton, addItem/addNode/addTab, addColumn, setSelectedIndex,
clearItems, setValidation, onChange, offChange, show/showModal,
get/getValue, set/setValue, close/destroy
```

Names follow each runtime's normal convention. Consult the corresponding
worker facade when positional argument order matters:

- `src/plugins/javascriptruntime/runtime/salamatrix_worker.mjs`
- `src/plugins/pythonruntime/runtime/salamatrix_worker.py`
- `src/plugins/powershellruntime/runtime/salamatrix_worker.ps1`
- `src/plugins/phpruntime/runtime/salamatrix_worker.php`
- `src/plugins/luaruntime/runtime/salamatrix_worker.lua`

The complete control catalog, layout/property reference, dark-mode/DPI model,
and side-by-side JavaScript, Python, PowerShell, PHP, Lua, Automation JScript,
and native C++ examples are documented in
[Salamatrix.UI framework and custom dialog guide](salamatrix-ui.md). The
bundled demo sources deliberately build the complete capabilities gallery
themselves instead of calling a prebuilt showcase window.

## AI service

Scripts may call the provider-neutral AI service through `ai.generate`,
`ai.preview`, and `ai.api`/`apiDescription`. Generation accepts:

```text
prompt, context, provider, runtime, existingScript, feedback
```

The assistant itself must return exactly one JSON object:

```json
{
  "title": "Short action title",
  "description": "What the generated script actually does",
  "capabilities": ["panels.read"],
  "estimatedEffects": {
    "readSelection": true,
    "readMetadata": false,
    "renameFiles": false,
    "moveFiles": false,
    "deleteFiles": false,
    "modifyContents": true,
    "executeExternal": false,
    "network": false
  },
  "runtime": "JavaScript.Node",
  "canImplement": true,
  "missingCapabilities": [],
  "script": "..."
}
```

Allowed capability names are exactly:

```text
panels.read, panels.write, ui.dialogs, commands, file-operations,
storage, events, ai, clipboard, runtimes
```

All eight effect keys are mandatory booleans. `capabilities` says which
Salamander framework surfaces the script uses; `estimatedEffects` says what the
script actually does, including operations performed directly by runtime
libraries.

If and only if required Salamander host integration is absent:

```json
{
  "canImplement": false,
  "missingCapabilities": [
    "Provide a thumbnail for a panel item"
  ]
}
```

The missing entries are human-readable framework gaps, not misspelled API
identifiers and not missing language packages. A script that can obtain a
selected file path and process it with its runtime is implementable.

## Capability and effect guide

| Script behavior | Capability | Effects |
| --- | --- | --- |
| Read current selection | `panels.read` | `readSelection` |
| Read item metadata returned by Salamander | `panels.read` | `readMetadata` |
| Change panel selection/path/tab state | `panels.write` | according to resulting file impact |
| Invoke interactive rename | `file-operations` | `renameFiles` |
| Invoke interactive move | `file-operations` | `moveFiles` |
| Invoke interactive delete | `file-operations` | `deleteFiles` |
| Read a selected file with a runtime library | usually `panels.read` | `readSelection`, optionally `readMetadata` |
| Write a sidecar or transformed file | capability for how paths were obtained | `modifyContents` |
| Start another process | none unless Salamander API is also used | `executeExternal` |
| Access HTTP or another network service | none unless Salamander API is also used | `network` |

Conservative declaration is required, but setting every effect and every
capability to true is incorrect. The preview is useful only when it describes
the generated source accurately.

## Known limits and honest GAP reporting

The current context returns at most 64 selected item records. The framework
exposes custom columns only for manifest-backed flat `fileSystems[]` providers;
it does not expose arbitrary columns for disk/archive panels, panel thumbnail
providers, overlay icons, archive internals, or dockable panes to scripts.
Those are legitimate `missingCapabilities`.

By contrast, hashing files, parsing formats, creating sidecars, calling an
installed command-line tool, or using a language package is not automatically
a framework GAP once the script can obtain the required paths.
