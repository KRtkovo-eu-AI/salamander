# Salamatrix Studio

Salamatrix Studio is a developer-first Visual Studio Code and VSCodium
extension for creating Open Salamander extensions. It is distributed as a
standalone VSIX and does not require a running or installed Open Salamander.

## Current vertical slice

- discovers `extension.json` projects in the workspace;
- shows projects, manifests, and dialogs in the Salamatrix Studio activity bar;
- creates `.salamatrix/dialogs/*.salamatrix-dialog.json` design documents;
- opens design documents in a visual custom editor;
- supports drag-and-drop insertion, selection, movement, resizing, deletion,
  and basic text/geometry editing;
- saves project-local dialog templates and creates dialogs from them;
- generates readable PowerShell, Python, and Node dialog modules.

The existing PowerShell Extension Menu Builder remains an independent shipped
extension. Its behavior will be reproduced in Studio without changing or
removing the original extension.

## Development

Requirements for building Studio are Node.js and npm. End users do not need
either dependency because the TypeScript and React sources are bundled into the
VSIX.

```powershell
cd src\tools\salamatrix-studio
npm install
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

The versioned JSON-line protocol is defined in
`src/salamatrix-sdk/contracts/studio-host-protocol.schema.json`. The companion
host will link only the host-independent Salamatrix native UI runtime. It will
not link or start `salamand.exe`. Native preview implementation and extraction
of the current `NativeDialog` code are the next native slice.
