# Project map for Salamatrix extensions

Use this as a navigation aid, then read the relevant current files in the target checkout. Repository-relative paths below are rooted at the Open Samandarin repository.

## Authoring and platform references

- `src/tools/salamatrix-studio/README.md` — preferred source-first extension scaffold/editor, manifest preservation, generated-file boundaries, and supported runtimes.
- `src/extensions/demos/README.md` — complete five-runtime packages combining a command, Viewer mask, and FS provider; includes installation and restart notes.
- `doc/salamatrix-platform.md` — platform concepts, extension registry, runtime and worker lifecycle, manifest discovery, and Plugin Manager display.
- `doc/salamatrix-ui.md` — shared native UI service and worker facade contract, including `ui.viewer`.
- `doc/salamatrix-filesystems.html` — current FS role contract, panel navigation, item fields, columns, actions, refresh and localization.
- `src/plugins/salamatrix/salamatrix_manifest.h` and `.cpp` — actual manifest data model and strict validation.
- `src/plugins/salamatrix/tests/manifest_tests.cpp` — accepted and rejected manifest cases; update/use when validating parser changes.

## Working extension examples

- `src/extensions/demos/{javascript-node,python,powershell,php,lua}/` — runtime-specific three-role examples and worker API spellings.
- `src/extensions/event-viewer/`, `process-explorer/`, `hardware-monitor/` — larger schema-2 FS-first packages, root hierarchies, columns, action filtering, refresh, localized labels, and icons.
- `src/extensions/git-worktree-navigator/` and `file-lock-inspector/` — command-first packages with Plugin menu, context-menu, toolbar and action handlers.
- `src/extensions/extension-menu-builder/` — PowerShell-generated menu package patterns; inspect its manifest serialization and localization, but avoid copying its standalone WinForms UI for ordinary extensions.
- `src/extensions/json-viewer-nodejs/` — a more advanced Viewer using the shared document renderer and temporary content staging.

For exact fields, search these sources and the manifest parser rather than relying on this index. Schema support has evolved: older command-first examples use schema 1, while new packages and Viewer/FS contributions use canonical `"schema": 2`.

## Practical checks

- Confirm `id`, `runtime`, `entryPoint`, command IDs/handlers, Viewer masks/handlers and FS provider IDs/handlers do not drift apart.
- Keep entry points and asset paths package-relative and safe. Use UTF-8 JSON; don't generate absolute machine-specific paths into the manifest.
- Check icon paths and light/dark variants; package-owned artwork should live with the package.
- Check every supported locale file is complete and valid JSON. Use the existing localization shape rather than hard-coding UI labels in one language.
- Follow `AGENTS.md`'s file-header, localization, dark-mode, attribution, and runtime-parity requirements when touching repository files.
