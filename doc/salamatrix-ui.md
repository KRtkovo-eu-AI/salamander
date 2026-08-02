# Salamatrix.UI framework and custom dialog guide

Salamatrix.UI is the framework-owned native user-interface service for Open
Salamander extensions. It lets a native plugin, an Automation script, or an
out-of-process JavaScript, Python, PowerShell, PHP, or Lua extension construct
the same dialog from the same control model.

This is both the user-facing UI authoring guide and the normative source for AI
providers generating Salamatrix dialogs. The installed HTML edition is linked
from the Automation API reference. Local AI providers receive a bounded,
machine-readable projection of the same lifecycle, control kinds, option names,
result fields, and runtime conventions; they must not infer methods from the
examples that are absent from that projection.

The window is not rendered by the language runtime and it is not owned by the
Automation plugin. Callers describe a dialog and its controls; Salamatrix
creates the native window, attaches Open Salamander controls, applies the
selected Salamander color scheme, scales the layout for the current monitor,
and returns values and events through the caller's native facade.

The bundled **Salamatrix UI capabilities** demos are executable examples of
this contract. Every demo builds the complete window itself. The only visible
difference is the `Created by` group, which identifies the runtime and the
extension or plugin that constructed the dialog.

## Ownership and execution model

```text
Node / Python / PowerShell / PHP / Lua extension
                   |
                   | salamander.ui.dialog.* (SMX1 JSON calls)
                   v
        Salamatrix package host dispatcher
                   |
                   v
       Salamatrix.UI IDialog / IControl
                   |
                   v
 Open Salamander native controls, DPI and dark mode
```

Native plugins query `SALAMATRIX_SERVICE_UI` and call the C++ interfaces
directly. Automation JScript uses a COM facade over the same service. Neither
path introduces a second dialog implementation.

Runtime providers remain independent `.SPL` plugins. In particular, Python,
PowerShell, PHP, Node, and Lua dialogs do not require the Automation plugin to
be installed or loaded.

## Dialog lifecycle

All facades follow the same lifecycle:

1. Create a dialog with a title and native layout size.
2. Add controls in the desired order.
3. Populate list, tree, combo, or tab items where required.
4. Configure validation or change-event delivery where required.
5. Show the dialog modally.
6. Read values and checked/selection state.
7. Destroy or close the dialog in a `finally`/cleanup block.

Worker calls use these host methods:

| Method | Purpose |
| --- | --- |
| `salamander.ui.dialog.create` | Create a framework-owned dialog. |
| `salamander.ui.dialog.add` | Add a control and optional explicit bounds. |
| `salamander.ui.dialog.item` | Add a combo/list/tree/tab item. |
| `salamander.ui.dialog.column` | Add a ListView column. |
| `salamander.ui.dialog.selection` | Change the selected item. |
| `salamander.ui.dialog.clearItems` | Remove all bound items. |
| `salamander.ui.dialog.validation` | Configure required validation. |
| `salamander.ui.dialog.events` | Enable or disable change events. |
| `salamander.ui.dialog.show` | Run the native modal dialog. |
| `salamander.ui.dialog.get` | Read text, checked state, item count, and selection. |
| `salamander.ui.dialog.set` | Change control text. |
| `salamander.ui.dialog.destroy` | Release the native dialog and callbacks. |

The package host stores dialog ownership on the manifest extension. Deactivation
or package removal destroys any remaining dialogs and unregisters their event
callbacks before the worker session is released.

## Supported controls

The `kind` passed to `addControl`/`add_control`/`AddControl` maps to a native
Salamatrix control kind:

| Runtime kind | Native control | Typical use |
| --- | --- | --- |
| `label` | Label | Ordinary caption text. |
| `statictext` | `CGUIStaticTextAbstract` | Styled, ellipsized, notify-enabled host text. |
| `textbox` | Edit | Single-line or multiline editable/read-only text. |
| `checkbox` | Check box | Boolean value. |
| `radio` | Radio button | Mutually exclusive choice. |
| `combobox` | Combo box | Drop-down item selection. |
| `button` | Button | Command, default button, or dialog result. |
| `listview` | Native ListView | Columns, rows, selection, and host list styles. |
| `treeview` | Native TreeView | Hierarchical items using parent indexes. |
| `tabcontrol` | Native TabControl | Tab headers and selection. |
| `folderpicker` | Edit plus browse button | Editable UTF-8 folder path. |
| `filepicker` | Edit plus browse button | Open/save file selection with a filter. |
| `groupbox` | Group box | Visual grouping and caption. |
| `hyperlink` | `CGUIHyperLinkAbstract` | Open URL, post command, or show a hint. |
| `progressbar` | `CGUIProgressBarAbstract` | Known or indeterminate progress. |
| `arrowbutton` | Host arrow button | Compact arrow action. |
| `textarrowbutton` | `CGUITextArrowButtonAbstract` | Text plus right/drop-down/more arrow. |
| `colorarrowbutton` | `CGUIColorArrowButtonAbstract` | Color sample, optional text, and arrow. |
| `toolbarheader` | `CGUIToolbarHeaderAbstract` | List header with modify/up/down/etc. buttons. |

## Common layout and options

Explicit bounds use `x`, `y`, `width`, and `height`. They are expressed in the
same native dialog-layout units used by the Salamatrix dialog definition. The
framework scales the dialog and every child consistently for the current DPI.

Common options are:

| Option | Meaning |
| --- | --- |
| `readOnly` | Prevent editing of an edit-based control. |
| `checked` | Initial checkbox or radio state. |
| `dialogResult` | Result returned when a button closes the dialog. |
| `keepOpen` | Keep the dialog open after the button is clicked. |
| `multiline` | Create a multiline text box. |
| `filter`, `save` | File-picker filter and save mode. |
| `styleFlags` | Native Salamatrix/Open Salamander control flags. |
| `pathSeparator` | Separator used by path ellipsis. |
| `toolTip` | Tooltip text owned by the native control. |
| `actionOpen` | URL or shell target opened by a hyperlink. |
| `actionCommand` | Command posted by a hyperlink. |
| `actionHint` | Hint displayed by a hyperlink. |
| `progress`, `progressText` | Direct progress value and optional text. |
| `progressCurrent`, `progressTotal` | 64-bit progress values. |
| `indeterminateDuration`, `indeterminateInterval` | Self-moving progress timing. |
| `textColor`, `backgroundColor` | `COLORREF` values for a color-arrow button. |
| `alignControlId`, `buttonMask` | List control and buttons for a toolbar header. |

The five worker facades deliberately forward the same option set. Language
syntax is idiomatic, but payload fields, defaults, and native behavior are the
same.

## The capabilities gallery layout

The common demo dialog is `463 x 236`. It mirrors the Open Salamander DemoPlug
control gallery:

```text
+--------------------------------+-----------------------+
| CGUIStaticTextAbstract         | CGUIHyperLinkAbstract |
|                                +-----------------------+
|                                | SetCurrentToolTip     |
+--------------------------------+-----------------------+
| CGUIProgressBarAbstract        | CGUIToolbarHeader...  |
| known + indeterminate progress |                       |
+--------------------------------+-----------------------+
| Buttons and color arrows       | Created by            |
|                                | Runtime / Extension   |
+--------------------------------+-----------------+-----+
                                                    Close
```

The exact bounds live in every demo source so the source itself demonstrates
dialog construction rather than calling a prebuilt gallery shortcut. The
identity group uses these values:

| Demo | Runtime | Extension/plugin |
| --- | --- | --- |
| Node | `JavaScript.Node` | `Salamatrix Node Demo` |
| Python | `Python.CPython` | `Salamatrix Python Demo` |
| PowerShell | `PowerShell` | `Salamatrix PowerShell Demo` |
| PHP | `PHP.CLI` | `Salamatrix PHP Demo` |
| Lua | `Lua` | `Salamatrix Lua Demo` |
| Automation | `Automation.JScript` | `Salamatrix Progress Demo` |
| Native | `Native` | `DemoPlug` |

## JavaScript / Node

Node uses an asynchronous dialog facade. Call `create()` before adding
controls and await every host operation:

```javascript
const dialog = await Salamander.ui.dialog(
  "Salamatrix UI capabilities",
  { width: 463, height: 236 },
).create();

const add = (kind, id, text, x, y, width, height, options = {}) =>
  dialog.addControl(kind, id, text, { x, y, width, height }, options);

await add("groupbox", "origin-group", "Created by", 269, 169, 185, 38);
await add("label", "runtime-label", "Runtime:", 277, 181, 42, 8);
await add("statictext", "runtime-value", "JavaScript.Node",
          323, 181, 122, 8, { styleFlags: 2 });
await add("button", "close", "Close", 403, 213, 50, 14,
          { dialogResult: 1, styleFlags: 0x100000 });

try {
  await dialog.show();
} finally {
  await dialog.close();
}
```

Complete demo: `src/extensions/demos/javascript-node/main.mjs`.

## Python

Python creates the native dialog synchronously and accepts the extended option
map through `options`:

```python
dialog = Salamander.ui.dialog("Salamatrix UI capabilities", 463, 236)

def add(kind, control_id, text, x, y, width, height, **options):
    dialog.add_control(
        kind, control_id, text,
        layout={"x": x, "y": y, "width": width, "height": height},
        options=options,
    )

add("progressbar", "progress", "", 15, 138, 235, 12,
    progress=120)
add("groupbox", "origin-group", "Created by", 269, 169, 185, 38)
add("statictext", "runtime-value", "Python.CPython",
    323, 181, 122, 8, styleFlags=2)
add("button", "close", "Close", 403, 213, 50, 14,
    dialogResult=1, styleFlags=0x100000)

try:
    dialog.show()
finally:
    dialog.close()
```

Complete demo: `src/extensions/demos/python/main.py`.

## PowerShell

PowerShell passes a layout hashtable and an extended options hashtable:

```powershell
$dialog = $Salamander.ui.Dialog('Salamatrix UI capabilities', 463, 236)

function Add-CapabilityControl(
    $kind, $id, $text, $x, $y, $width, $height, $options = @{}
) {
    $dialog.AddControl(
        $kind, $id, $text, $false, $false, 0,
        @{x=$x; y=$y; width=$width; height=$height},
        $false, $false, $options)
}

Add-CapabilityControl groupbox origin-group 'Created by' 269 169 185 38
Add-CapabilityControl statictext runtime-value PowerShell 323 181 122 8 `
    @{styleFlags=2}
Add-CapabilityControl button close Close 403 213 50 14 `
    @{dialogResult=1; styleFlags=0x100000}

try { [void]$dialog.Show() } finally { $dialog.Close() }
```

Complete demo: `src/extensions/demos/powershell/main.ps1`.

## PHP

PHP exposes the same extra option map as the last `addControl` argument:

```php
$dialog = $Salamander->ui->dialog(
    'Salamatrix UI capabilities', 463, 236);

$layout = array('x'=>269, 'y'=>169, 'width'=>185, 'height'=>38);
$dialog->addControl('groupbox', 'origin-group', 'Created by',
    false, false, 0, $layout);

$layout = array('x'=>323, 'y'=>181, 'width'=>122, 'height'=>8);
$dialog->addControl('statictext', 'runtime-value', 'PHP.CLI',
    false, false, 0, $layout, false, false,
    array('styleFlags'=>2));

$layout = array('x'=>403, 'y'=>213, 'width'=>50, 'height'=>14);
$dialog->addControl('button', 'close', 'Close',
    false, false, 0, $layout, false, false,
    array('dialogResult'=>1, 'styleFlags'=>0x100000));

try { $dialog->show(); } finally { $dialog->close(); }
```

Complete demo: `src/extensions/demos/php/main.php`.

## Lua

Lua uses snake-case option names in its language facade and translates them to
the same SMX1 payload fields:

```lua
local dialog = Salamander.ui.dialog(
    "Salamatrix UI capabilities", 463, 236)

dialog.add_control("groupbox", "origin-group", "Created by", {
    x=269, y=169, width=185, height=38
})
dialog.add_control("statictext", "runtime-value", "Lua", {
    x=323, y=181, width=122, height=8, style_flags=2
})
dialog.add_control("button", "close", "Close", {
    x=403, y=213, width=50, height=14,
    dialog_result=1, style_flags=0x100000
})

local shown, failure = pcall(dialog.show)
dialog.close()
if not shown then error(failure) end
```

Complete demo: `src/extensions/demos/lua/main.lua`.

## Automation JScript

Automation exposes a thin COM dialog object. `add` creates a control and `set`
configures properties that need an additional value:

```javascript
var dialog = Salamander.UI.dialog(
    "Salamatrix UI capabilities", 463, 236);

dialog.add("groupbox", "origin-group", "Created by",
           269, 169, 185, 38);
dialog.add("statictext", "runtime-value", "Automation.JScript",
           323, 181, 122, 8, 2);
dialog.add("progressbar", "progress", "", 15, 138, 235, 12);
dialog.set("progress", "progress", 120);
dialog.add("button", "close", "Close",
           403, 213, 50, 14, 0x100000, 1);

try { dialog.show(); } finally { dialog.close(); }
```

Complete demo:
`src/plugins/automation/sample-scripts/Salamatrix Progress Demo/main.js`.

## Native C++ plugins

A native plugin obtains the versioned UI service and builds the same controls
directly. The plugin owns only the `IDialog` handle; the concrete window and
host control adapters remain in Salamatrix:

```cpp
CSalamanderServiceQuery query;
CSalamanderServiceResult result;
query.ServiceId = SALAMATRIX_SERVICE_UI;
query.MinimumVersion = SALAMATRIX_UI_VERSION_1_4;
query.Flags = 0;

if (SalamanderGeneral->QueryService(&query, &result))
{
    Salamatrix::UI::IUIService* ui =
        static_cast<Salamatrix::UI::IUIService*>(result.Interface);

    Salamatrix::UI::DialogOptions dialogOptions;
    dialogOptions.Title = "Salamatrix UI capabilities";
    dialogOptions.Parent = parent;
    dialogOptions.Width = 463;
    dialogOptions.Height = 236;
    Salamatrix::UI::IDialog* dialog =
        ui->CreateSalamatrixDialog(dialogOptions);

    Salamatrix::UI::ControlOptions controlOptions;
    controlOptions.Id = "origin-group";
    controlOptions.Text = "Created by";
    Salamatrix::UI::ControlLayout layout;
    layout.HasBounds = TRUE;
    layout.X = 269; layout.Y = 169;
    layout.Width = 185; layout.Height = 38;
    dialog->AddControlEx(
        Salamatrix::UI::ControlKindGroupBox,
        controlOptions, layout);

    dialog->ShowModal();
    ui->DestroyDialog(dialog);
}
```

Complete demo: `src/plugins/demoplug/menu.cpp`.

## Lists, trees, tabs, validation, and events

Collection controls use stable control IDs:

```python
dialog.add_control("listview", "files", "",
                   layout={"x": 10, "y": 10, "width": 240, "height": 100})
dialog.add_column("files", "Name", 150)
dialog.add_column("files", "Size", 70)
dialog.add_item("files", "report.txt")
dialog.set_selected_index("files", 0)
```

Required-field validation is configured after adding the control. Change
events are delivered as SMX1 event frames with dialog ID, control ID, control
kind, text, checked state, and selected index. The worker facades expose
`onChange`/`on_change` and matching unsubscribe helpers.

## Dark mode, DPI, and accessibility

Dark mode means the **Windows Dark Mode (experimental)** scheme selected in
Open Salamander under **Configuration > Colors > Scheme**. It does not mean the
operating-system app-mode setting.

`NativeDialog` applies the shared Salamander dark-mode policy to the title bar,
dialog background, standard controls, Open Salamander GUI controls, pickers,
lists, and progress components. Callers should not choose dark colors or query
Windows app mode themselves.

The framework also:

- scales dialog and child bounds on `WM_DPICHANGED`;
- uses the host dialog font and keyboard tab order;
- assigns an initial focus to the first interactive control;
- retains bounded accessibility name and description metadata;
- provides native tooltip fallback for accessibility descriptions;
- keeps file-picker edit and browse HWNDs under the same metadata/lifecycle.

## Manifest capability

Scripted extensions that create dialogs declare `ui.dialogs`:

```json
{
  "schema": 1,
  "id": "example.ui-demo",
  "name": "UI demo",
  "version": "1.0.0",
  "runtime": "Python.CPython",
  "entryPoint": "main.py",
  "capabilities": ["ui.dialogs"]
}
```

The host enforces declared capabilities at the SMX1 boundary. Native plugins
use service-version negotiation instead of manifest capability checks.

## Verification and current limits

The dialog dispatcher and facades are covered by worker syntax checks,
framework/Automation/DemoPlug Release x64 builds, deterministic native layout
tests, and isolated Python/PowerShell/PHP process-runtime tests using a worker
root that contains only the provider bootstraps under test. The capabilities
gallery sources also make the complete geometry independently reviewable in
all supported languages.

The remaining UI work includes richer notification actions and stacking,
virtualized data sources, complete UI Automation roles and a screen-reader
audit, interactive per-monitor/DPI test automation, a documented reentrancy
policy, and migration of the remaining legacy Forms wrappers. The framework is
for native modal extension dialogs; it does not yet expose arbitrary dockable
panes or custom panel columns to scripts.

The native C++ control enum also contains `ControlKindSplitter`; the generic
SMX1 `kind` mapping does not publish a `splitter` worker control yet. The new
Automation JScript COM adapter intentionally covers the controls and properties
needed by the capabilities gallery; its older Forms objects remain the
compatibility surface for other legacy JScript/VBScript forms until the COM
adapter is broadened.

## Related documentation

- [Salamatrix Platform Foundation](salamatrix-platform.md)
- [Salamatrix Automation API reference](salamatrix-automation-api.md)
- [Standalone Salamatrix runtime providers](salamatrix-runtime-providers.md)
- [Developing a Salamatrix language runtime provider](salamatrix-runtime-provider-development.md)
- [Salamatrix framework gap analysis](salamatrix-gap-analysis.md)
