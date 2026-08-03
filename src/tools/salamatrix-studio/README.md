# Salamatrix Studio

Salamatrix Studio is a developer-first Visual Studio Code and VSCodium
extension for creating Open Salamander extensions. It is distributed as a
standalone VSIX and does not require a running or installed Open Salamander.

## Implemented scope

- creates functional extension projects for PowerShell, Python, Node.js, PHP,
  Lua, and Automation JScript without overwriting existing files;
- discovers `extension.json` projects in the workspace and offers a welcome
  page for creating or adding one when the workspace is empty;
- shows each project as **Overview**, **Menu Builder**, **Dialogs**, and
  **Source Files** in the Salamatrix Studio activity bar;
- creates `.salamatrix/dialogs/*.salamatrix-dialog.json` design documents;
- creates standalone dialog designs for native plugin projects without an
  `extension.json`;
- opens design documents in a visual custom editor;
- supports drag-and-drop insertion, selection, movement, resizing, deletion,
  text/geometry editing, option maps, items, columns, selection, and validation;
- saves project-local dialog templates and creates dialogs from them;
- provides separate visual Overview and Menu Builder tabs while preserving
  manifest fields outside their visual surface;
- stores Studio-owned menu action metadata in `.salamatrix/menu.json`, previews
  plugin/context/toolbar placements with light or dark SVG icons, and can
  generate simple Program, Open, Command Line, and PowerShell actions;
- generates readable PowerShell, Python, Node.js, PHP, Lua, Automation JScript,
  and native C++ dialog modules, either from the project runtime or an explicit
  per-generation target selection;
- bundles a standalone x64 native Win32 preview host in the VSIX.

The existing PowerShell Extension Menu Builder remains an independent shipped
extension and is not changed or removed by Studio.

## Development

Requirements for building Studio are Node.js, npm, and Visual Studio 2022 C++
Build Tools. End users do not need these dependencies because the bundled
JavaScript and native x64 host are packaged into the VSIX.

```powershell
cd src\tools\salamatrix-studio
npm install
npm run build:host
npm run check
npm run build
npm run package
```

`npm run package` produces a `win32-x64` VSIX in
`build/salamatrix-studio/` at the repository root. The project structure and
native host protocol are architecture-neutral so a `win32-arm64` package can
be added later.

## Project files

The visual design document is the source of truth for the generated dialog
module:

```text
MyExtension/
  extension.json
  main.ps1
  generated/menu-actions.generated.ps1
  generated/settings-dialog.generated.ps1
  .salamatrix/menu.json
  .salamatrix/dialogs/settings.salamatrix-dialog.json
  .salamatrix/templates/optional-template.salamatrix-dialog.json
```

Files below `generated/` are overwritten by Studio. A newly scaffolded entry
point contains one stable menu-dispatch integration block. Existing projects
remain in non-invasive Custom Handler mode until the developer explicitly
enables generated actions and accepts a preview of that one-time entry-point
integration; code outside the marked block is left untouched.

## Native preview boundary

The preview host does not link or start `salamand.exe`. It consumes the Studio
dialog model through a versioned private transport and compiles the same SDK
`NativeDialog` and Salamatrix-specific control sources as `Salamatrix.spl`.
There is no separate Studio renderer or Win32 fallback control path. Its C/C++
runtime is linked statically, so the installed VSIX does not require the Visual
C++ Redistributable or app-local MSVC/UCRT DLLs.

The planned persistent request/response boundary is described by
`src/salamatrix-sdk/contracts/studio-host-protocol.schema.json`; the current
VSIX launches one isolated host process per preview.
