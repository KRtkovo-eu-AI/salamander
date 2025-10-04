# Open Salamander: Samandarin 5.0-samandarin-0.1

## Why this release matters

Open Salamander: Samandarin remains an intentionally experimental fork—its AI-implemented features are actively evolving, may surface unexpected issues, and are best explored by advanced, adventurous users who can tolerate work-in-progress behavior. Open Salamander: Samandarin continues to deliver a fast, two-panel file manager for Windows that evolves the original project with AI-crafted enhancements while staying compatible with the upstream plugin ecosystem. This release keeps the playful spirit behind the Samandarin name—mixing legacy, daring experimentation, and a fresh identity—while reaffirming the fork's commitment to transparent, adventurous development. For a deeper look at the fork's background, goals, and build instructions, see the [README](../../README.md).

## Release highlights

- **AI-driven iteration without sacrificing compatibility.** Development remains guided by AI tooling yet tracks closely with Open Salamander 5.0 so features can merge upstream when production-ready.
- **Safe side-by-side installs.** Configuration continues to live in the dedicated "Open Salamander Samandarin" registry hive, keeping official installations untouched.
- **Documented build pipeline.** Windows 11 with Visual Studio 2022 and the C++ workload remain the baseline for building the solution, with helper scripts to populate runnable build directories.
- **Tabbed Panels (PoC 07).** The proof-of-concept branch lands in this release, giving power users dockable panels inside each pane so multiple locations stay open without launching extra windows.
- **Dark Mode (PoC 01).** A first pass at theme-aware rendering debuts, pairing the new viewer plugins with a low-light palette tuned for the Samandarin UI experiments.

## New plugin experiences

### JSON Viewer (.NET)

The JSON Viewer plugin now ships as part of the default experience, targeting .NET Framework 4.8 and bundling Newtonsoft.Json 13.0.3 for modern JSON handling under MIT-friendly licensing. It recognizes `.json`, `.pc`, and `.jbeam` payloads (case-insensitive), giving vehicle simulation and configuration formats first-class previews directly inside the panels.

### PrismSharp Text Viewer (.NET)

PrismSharp Text Viewer moves beyond plain-text rendering by combining the .NET Framework 4.8 runtime with WebView2, PrismSharp, and Newtonsoft.Json, blending WebView-powered visualization with extensive syntax-highlighting palettes. Out of the box it understands common text, configuration, web, Markdown, scripting, C-family, and MSBuild project files, ensuring day-to-day workflows open with the right formatting cues. Beyond those staples, the viewer registers hundreds of Prism lexers—from ABAP to Zig—so that nearly any source file dropped into Salamander benefits from accurate coloring and navigation.

### WebView2 Render Viewer (.NET)

The WebView2 Render Viewer rounds out the reading stack by pairing the .NET Framework 4.8 runtime with Microsoft.Web.WebView2 and Markdig for GPU-accelerated HTML and Markdown rendering inside Salamander. It acts as a universal document canvas, covering web pages, Markdown (including MDX), SVG, modern image formats like WebP and AVIF, classic raster files, and PDFs up to 32 MB.

### Samandarin Update Notifier

The new Samandarin plugin runs a managed update-notification service: it bootstraps a WinForms host, exposes commands to initialize, configure, trigger checks, respond to palette changes, and shut down cleanly, and surfaces polished error handling when anything goes wrong. UpdateCoordinator persists user preferences, schedules timers, honours start-up checks, and reaches out to the fork's GitHub releases feed with TLS 1.2+ support to detect fresh builds without overwhelming the servers. When a new tag appears it offers to open the download page; if you're already current it reports the latest known release, and if connectivity fails it conveys detailed error chains—all while marshalling UI updates back onto the correct thread.

### Plugin availability notes

Several legacy viewers remain on the roadmap for a later drop: the PictView, UnRAR, and Encrypt plugins are absent from this bundle. In addition, the FTP client plugin currently lacks the OpenSSL libraries it needs for secure transfers; install them separately from [openssl.org](https://www.openssl.org/) if you need encrypted connectivity.

### Language availability

Samandarin currently ships with an English-only interface; localization files are not yet available, so the UI and documentation remain untranslated.

## Getting started

Set up your development workstation with Windows 11, Visual Studio 2022, and the Desktop development with C++ workload (plus optional Git, PowerShell, and HTMLHelp Workshop), then build `src\vcxproj\salamand.sln` or run the helper scripts to populate a runnable directory. Once installed, the Samandarin registry hive keeps experiments isolated, letting you explore the new plugin lineup without disturbing stable Open Salamander deployments.

