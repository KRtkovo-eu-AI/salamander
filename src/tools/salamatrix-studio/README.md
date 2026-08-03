# Salamatrix Studio

Salamatrix Studio is a developer-first Visual Studio Code and VSCodium
extension for creating Open Salamander extensions. It is distributed as a
standalone VSIX and does not require a running or installed Open Salamander.

## Implemented scope

- discovers `extension.json` projects in the workspace;
- shows projects, manifests, and dialogs in the Salamatrix Studio activity bar;
- creates `.salamatrix/dialogs/*.salamatrix-dialog.json` design documents;
- creates standalone dialog designs for native plugin projects without an
  `extension.json`;
- opens design documents in a visual custom editor;
- supports drag-and-drop insertion, selection, movement, resizing, deletion,
  text/geometry editing, option maps, items, columns, selection, and validation;
- saves project-local dialog templates and creates dialogs from them;
- provides a visual `extension.json` and command/menu editor while preserving
  manifest fields outside its visual surface;
- generates readable PowerShell, Python, Node.js, PHP, Lua, Automation JScript,
  and native C++ dialog modules, either from the project runtime or an explicit
  per-generation target selection;
- bundles a standalone x64 native Win32 preview host in the VSIX.

The existing PowerShell Extension Menu Builder remains an independent shipped
extension. Its behavior will be reproduced in Studio without changing or
removing the original extension.

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

`npm run package` produces a `win32-x64` VSIX. The project structure and native
host protocol are architecture-neutral so a `win32-arm64` package can be added
later.

## Project files

The visual design document is the source of truth for the generated dialog
module:

```text
MyExtension/
  extension.json
  main.ps1
  generated/settings-dialog.generated.ps1
  .salamatrix/dialogs/settings.salamatrix-dialog.json
  .salamatrix/templates/optional-template.salamatrix-dialog.json
```

Files below `generated/` are overwritten by Studio. Hand-written entry points
and application logic are never rewritten.

## Native preview boundary

The preview host does not link or start `salamand.exe`. It consumes the Studio
dialog model through a versioned private transport and compiles the same SDK
`NativeDialog` and Salamatrix-specific control sources as `Salamatrix.spl`.
There is no separate Studio renderer or Win32 fallback control path.

The planned persistent request/response boundary is described by
`src/salamatrix-sdk/contracts/studio-host-protocol.schema.json`; the current
VSIX launches one isolated host process per preview.
