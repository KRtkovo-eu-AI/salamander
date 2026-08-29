# Open Salamander: Samandarin 0.15 changelist

This document summarizes user-visible changes since 5.0-samandarin-0.14. It intentionally omits internal refactoring unless it changed compatibility, reliability, security, or observable behavior.

## File management and operations
- Added storage-aware Copy and Move operation scheduling: run operations globally in sequence, concurrently when storage paths do not conflict, or ask for each operation.
- Added separate configurable SSD/NVMe limits for parallel file transfers within a single Copy or Move operation.
- Updated the File Operation Progress window to present multiple active transfers while retaining queue, pause, resume, and error controls.
- Added `Ctrl+PgDown` content probing to open supported archives with missing or incorrect extensions, self-extracting archives, and package-based Office/OpenDocument files without changing normal Enter behavior.
- Added `Shift+F5` multi-target copy to all selected folders in the opposite panel.
- Added Copy and Move between plug-in file systems and archives, including FTP-to-archive, remote-to-remote, and archive-to-archive workflows.
- Added Total Commander DiskDir-compatible disk catalogs and recursive Make File List output for disk and archive panels.
- Added path autocomplete to Copy, Move, Quick Rename, Create Folder, and Change Directory dialogs.
- Added extended Delete confirmation with a reviewable table of selected names.
- Added mounted NTFS-folder volumes to Change Drive and optionally to the Drive Bar, including configurable labels/path display and optional Windows Sandbox volumes.
- Added an occupied-space option that remains on the current file system instead of following directory links or mounted folders.
- Updated bundled 7-Zip to 26.02 with additional archive, package, file-system, and disk-image formats.
- Improved Unicode and long-path handling in ZIP, 7-Zip, RAR, Checksum, File Comparator, viewers, editors, and external file-action workflows.
- Fixed ZIP names containing diacritics when Windows uses UTF-8 as the active system code page.

## Interface, panels, and search
- Added detachable individual tabs via context menu or drag-out, with independent windows, persistence, and reattachment to either panel side.
- Added independently detached full left/right panel windows with file-operation support.
- Added tab locking, protected path changes, tab-width options, bold active-tab styling, overflow wheel scrolling, and right-button wheel switching.
- Added shared or separate history for panels and tabs and persistence of forward/back history across restarts, including plug-in and extension locations.
- Added configurable UI and menu fonts across dialogs, viewers, plug-in windows, and panels.
- Expanded Windows Explorer properties: choose by category, reorder and persist detailed-view columns, search/edit supported values and tags, and sort by plug-in or extension columns.
- Updated panel icons to match modern Windows Explorer associations more closely and added ICO image thumbnails.
- Added WebP, SVG, STL, CDR, folder, and other supported thumbnails.
- Added sortable Find, Registry Search, and Batch Renamer results with ascending/descending order and optional mixed files/directories or keys/values.
- Added compact rich panel tooltips with file-type metadata.
- Added optional Windows Dark Mode color-scheme support across the application and participating plug-ins.
- Improved Per-Monitor V2 DPI behavior for windows, dialogs, controls, icons, panels, and detached views.
- Added a docked or floating Tree View synchronized with the active disk panel.
- Added configurable command-shell templates, discovered Windows Terminal profiles, and an elevated-shell shortcut (`Shift+Num /`).
- Added additional Windows user-folder shortcuts.
- Added visible shutdown progress for slow shutdown operations.
- Improved main-window size preservation after sleep and display changes.

## Viewers, thumbnails, and formats
- Added the native Prism Text Viewer with syntax highlighting for source, configuration, markup, logs, and many other text formats.
- Added the native WebView2 Render Viewer for HTML, Markdown, SVG, WebP, AVIF, APNG, raster images, and PDF.
- Added shared native viewer behavior including zoom, encoding selection, line numbers, wrapping, whitespace display, persistent settings, DPI support, and Salamander dark mode.
- Expanded Internal Viewer with Unicode text, line/column and selection status, zoom, optional line numbers, and auto-refreshing Log View Mode.
- Restored PictView support beyond installed WIC codecs through native decoders for TGA, PCX, Netpbm, SVG, DDS/DX10, XCF, PDN, 3DM, DWG, WMF/EMF, and embedded previews in EPS, AI, PDF, and MOV.
- Added interactive STL viewing through an installed Explorer 3D preview handler.
- Added dynamic discovery of installed WIC codecs rather than treating optional formats as universally available.
- Added JSONL handling in Database Viewer and expanded modern viewer file associations.

## Salamatrix and system extensions
- Stabilized Salamatrix Framework v1 with versioned host services and manifest-declared capabilities.
- Added extension commands, context menus, toolbars, viewers, panel file systems, typed columns, actions, icons, settings, localization, and a dedicated Extension Bar.
- Added native dialog, picker, progress, notification, storage, clipboard, and event APIs for extensions.
- Kept public runtime capabilities equivalent across JavaScript, Python, PowerShell, PHP, and Lua providers.
- Added Salamatrix Studio for Visual Studio Code with project tooling, manifest editing, visual native dialog/menu design, preview, and generated runtime code.
- Added Process Explorer with process details, end task/tree, executable location, and properties actions.
- Added Hardware Monitor with hardware hierarchy, sensors, utilization, SMART/NVMe data, and Device Manager actions.
- Added Windows Event Viewer with Custom Views, Windows Logs, Applications and Services Logs, event properties, and XML details.
- Added File Lock Inspector, Git Worktree Navigator, Interactive Extension Menu Builder, local AI provider support, Hyper-V tools, Windows Service Explorer, and Windows Portable Devices integrations.

## Plug-ins, configuration, and security
- Added automatic catalog installation and updating for native plug-ins, runtimes, and extensions with dependency resolution.
- Added catalog icons and capability declarations to Plugin Manager.
- Added fail-closed verification of official packages using catalog SHA-256 values and Authenticode publisher validation for extracted first-party binaries.
- Added disclosure of curated network, external-process, and web-access capabilities.
- Added portable file-based configuration storage and a portable installation mode without Registry entries.
- Added configuration management, import/export, target-location selection, and a guided restore workflow.
- Improved configuration autosave and recovery.
- Added AI-assisted localization workflows and an Italian localization.
- Added a modern SFTP/SCP plug-in with password, private-key/PPK, keyboard-interactive authentication, known-host verification, resume, remote edit, chmod, and automatic SCP fallback.
- Improved SFTP reconnect, saved-password prefill, and responsive drag/layout behavior.
- Added a compatibility layer for existing Altap Salamander 4.0 plug-ins.
- Digitally signed distributed executables, libraries, plug-ins, and the installer.