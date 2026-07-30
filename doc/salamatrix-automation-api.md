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

The worker injects one root object:

| Runtime | Root object |
| --- | --- |
| JavaScript | `Salamander` |
| Python | `Salamander` |
| PowerShell | `$Salamander` |
| PHP | `$Salamander` |

JavaScript scripts run as modules and may use top-level `await`. They must not
use an invented `this.selectedItems` property. The current selection is obtained
through `await Salamander.sides.context("source")`.

### How the AI consumes this contract

The live `Salamatrix.AI` service publishes versioned machine-readable slices
for `sides`, `fileOperations`, `commands`, `ui`, `storage`, `events`,
`runtimes`, and `ai`. The assistant selects slices from the user's task instead
of sending an unbounded copy of the whole manual.

The bundled local model receives these slices through a **Strict Interface
Contract**, split into five explicit sections:

1. `INPUT CONTRACT`: typed task, selected runtime id, current Salamander context,
   existing source, and optional repair feedback, including their actual values;
2. `INSTALLED SALAMANDER API CONTRACT`: only the relevant implemented API
   slices;
3. `SELECTED RUNTIME CONTRACT`: the real worker facade conventions for the
   selected JavaScript, Python, PowerShell, or PHP runtime;
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
| `inputBox` | prompt, title, initial value | `{accepted, value}` |
| `pickFile` | save, title, filter, initial | `{selected, path}` |
| `pickFolder` | title, initial | `{selected, path}` |
| `progress` | title, total, style flags, optional second total | progress object |
| `dialog` | title, width, height | dialog object |

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

Supported controls are `label`, `textbox`, `checkbox`, `radio`, `combobox`,
`button`, `listview`, `treeview`, `tabcontrol`, `folderpicker`, and
`filepicker`.

The generic add operation accepts a control id, text, optional
`x/y/width/height`, and applicable options: `readOnly`, `checked`,
`dialogResult`, `keepOpen`, `multiline`, `filter`, and `save`.

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
    "Register a custom panel column",
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
does not currently expose arbitrary custom panel columns, panel thumbnail
providers, overlay icons, archive internals, or dockable panes to scripts.
Those are legitimate `missingCapabilities`.

By contrast, hashing files, parsing formats, creating sidecars, calling an
installed command-line tool, or using a language package is not automatically
a framework GAP once the script can obtain the required paths.
