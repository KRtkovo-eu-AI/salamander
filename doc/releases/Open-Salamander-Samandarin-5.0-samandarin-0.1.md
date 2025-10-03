# Open Salamander: Samandarin 5.0-samandarin-0.1

## Why this release matters

Open Salamander: Samandarin continues to deliver a fast, two-panel file manager for Windows that evolves the original project with AI-crafted enhancements while staying compatible with the upstream plugin ecosystem.【F:README.md†L1-L34】 This release keeps the playful spirit behind the Samandarin name—mixing legacy, daring experimentation, and a fresh identity—while reaffirming the fork's commitment to transparent, adventurous development.【F:README.md†L16-L34】

## Release highlights

- **AI-driven iteration without sacrificing compatibility.** Development remains guided by AI tooling yet tracks closely with Open Salamander 5.0 so features can merge upstream when production-ready.【F:README.md†L11-L34】
- **Safe side-by-side installs.** Configuration continues to live in the dedicated "Open Salamander Samandarin" registry hive, keeping official installations untouched.【F:README.md†L36-L38】
- **Documented build pipeline.** Windows 11 with Visual Studio 2022 and the C++ workload remain the baseline for building the solution, with helper scripts to populate runnable build directories.【F:README.md†L54-L71】

## New plugin experiences

### JSON Viewer (.NET)

The JSON Viewer plugin now ships as part of the default experience, targeting .NET Framework 4.8 and bundling Newtonsoft.Json 13.0.3 for modern JSON handling under MIT-friendly licensing.【F:src/plugins/jsonviewer/DEPENDENCIES.md†L1-L6】 It recognizes `.json`, `.pc`, and `.jbeam` payloads (case-insensitive), giving vehicle simulation and configuration formats first-class previews directly inside the panels.【F:src/plugins/jsonviewer/SUPPORTED_FILE_TYPES.md†L1-L7】

### PrismSharp Text Viewer (.NET)

PrismSharp Text Viewer moves beyond plain-text rendering by combining the .NET Framework 4.8 runtime with WebView2, PrismSharp, and Newtonsoft.Json, blending WebView-powered visualization with extensive syntax-highlighting palettes.【F:src/plugins/textviewer/DEPENDENCIES.md†L1-L9】 Out of the box it understands common text, configuration, web, Markdown, scripting, C-family, and MSBuild project files, ensuring day-to-day workflows open with the right formatting cues.【F:src/plugins/textviewer/SUPPORTED_FILE_TYPES.md†L3-L18】 Beyond those staples, the viewer registers hundreds of Prism lexers—from ABAP to Zig—so that nearly any source file dropped into Salamander benefits from accurate coloring and navigation.【F:src/plugins/textviewer/SUPPORTED_FILE_TYPES.md†L20-L60】

### WebView2 Render Viewer (.NET)

The WebView2 Render Viewer rounds out the reading stack by pairing the .NET Framework 4.8 runtime with Microsoft.Web.WebView2 and Markdig for GPU-accelerated HTML and Markdown rendering inside Salamander.【F:src/plugins/webview2renderviewer/DEPENDENCIES.md†L1-L10】 It acts as a universal document canvas, covering web pages, Markdown (including MDX), SVG, modern image formats like WebP and AVIF, classic raster files, and PDFs up to 32 MB.【F:src/plugins/webview2renderviewer/SUPPORTED_FILE_TYPES.md†L3-L12】

### Samandarin Update Notifier

The new Samandarin plugin runs a managed update-notification service: it bootstraps a WinForms host, exposes commands to initialize, configure, trigger checks, respond to palette changes, and shut down cleanly, and surfaces polished error handling when anything goes wrong.【F:src/plugins/samandarin/managed/EntryPoint.cs†L19-L125】 UpdateCoordinator persists user preferences, schedules timers, honours start-up checks, and reaches out to the fork's GitHub releases feed with TLS 1.2+ support to detect fresh builds without overwhelming the servers.【F:src/plugins/samandarin/managed/EntryPoint.cs†L128-L318】 When a new tag appears it offers to open the download page; if you're already current it reports the latest known release, and if connectivity fails it conveys detailed error chains—all while marshalling UI updates back onto the correct thread.【F:src/plugins/samandarin/managed/EntryPoint.cs†L202-L640】

## Getting started

Set up your development workstation with Windows 11, Visual Studio 2022, and the Desktop development with C++ workload (plus optional Git, PowerShell, and HTMLHelp Workshop), then build `src\vcxproj\salamand.sln` or run the helper scripts to populate a runnable directory.【F:README.md†L54-L71】 Once installed, the Samandarin registry hive keeps experiments isolated, letting you explore the new plugin lineup without disturbing stable Open Salamander deployments.【F:README.md†L36-L38】

