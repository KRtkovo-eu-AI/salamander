# Open Salamander: Samandarin
Open Salamander: Samandarin is a fast and reliable dual-pane file manager for Windows.

It evolves the [original project](https://github.com/OpenSalamander/salamander) with enhancements and new features while staying compatible with the upstream code and plugin ecosystem.

<img width="1247" height="845" alt="Samandarin application preview" src="https://github.com/user-attachments/assets/8cfedd13-c875-4400-b51c-6b9395a16c8e" />

## Included Features Overview

### File management
- Unicode file names and Windows extended-length local and UNC paths across core file operations and supported plug-ins.
- Copy and Move operations can run globally in sequence, concurrently when storage paths do not conflict, or ask for each operation; separate SSD/NVMe limits control parallel file transfers within one operation.
- Copy and Move directly between plug-in file systems and archives; copy to multiple selected target folders with `Shift+F5`.
- Probe and open archive content with `Ctrl+PgDown` even when a file has a missing or incorrect extension.
- Mounted-folder volumes, optional Windows Sandbox volumes, recursive file lists, DiskDir-compatible catalogs, path autocomplete, and extended delete confirmation.

### Interface and panels
- Tabbed panels, detachable individual tabs, independently detached panel windows, locked tabs, persistent navigation history, and a docked or floating Tree View.
- Windows dark-mode scheme, Per-Monitor V2 DPI awareness, and configurable fonts across menus, dialogs, viewers, plug-ins, and panels.
- Explorer properties can be selected by category, reordered, persisted, searched, edited where supported, and used for panel sorting; tags are searchable and editable.
- Explorer-compatible file icons and ICO thumbnails, rich item tooltips, additional user folders, and configurable Windows Terminal or command-shell profiles.
- Sortable Find, Registry Search, and Batch Renamer result lists.

### Viewers and extensions
- Internal Viewer with Unicode, status and selection information, zoom, line numbers, and live Log View Mode.
- Prism Text Viewer with syntax highlighting, plus WebView2 rendering for HTML, Markdown, SVG, modern images, and PDF.
- PictView combines WIC with native decoders, embedded previews, interactive STL preview, and expanded STL/CDR/folder/image thumbnails.
- Salamatrix Framework v1 provides commands, viewers, panel file systems, native UI, localization, and shared capabilities across JavaScript, Python, PowerShell, PHP, and Lua.
- Process Explorer, Hardware Monitor, and Event Viewer extensions provide integrated Windows diagnostics panels.
- [Salamatrix Studio](https://samandarin.krtkovo.eu/salamatrix/studio.html) adds VS Code project tooling, a native dialog/menu designer, preview, and code generation.

### Configuration and security
- Portable file-based configuration, import/export, configuration management, and a guided restore workflow.
- AI-assisted localization tooling for application and plug-in translations.
- Official catalog installations verify the package SHA-256 and first-party Authenticode publisher, then disclose curated network, external-process, and web capabilities.
- Distributed binaries and the installer are digitally signed.

Explore the [complete feature guide](https://samandarin.krtkovo.eu/#features), [version comparison](https://samandarin.krtkovo.eu/compare.html), and [0.15 changelist](doc/changelog-0.15.md).

## Plugin Catalog
- Stable: https://samandarin.krtkovo.eu/catalogs/plugins-stable.json
- Unofficial third party: https://samandarin.krtkovo.eu/catalogs/plugins-unofficial.json
- Extension runtimes: https://samandarin.krtkovo.eu/catalogs/extension-runtimes.json

Catalog entries can carry icons, dependencies, integrity information, and curated capability declarations. Plugin Updates can install or update plug-ins, runtimes, and extensions together with required dependencies.

## Samandarin Fork Overview

### Fork Name Inspiration

The “Samandarin” name is a three-way pun that pays homage to the Salamander legacy:

- **Fire salamander** – the black-and-yellow amphibian that inspired the original project.
- **Samandarin** – the natural toxin secreted by that salamander, symbolising a spicy, daring twist — just like our AI-crafted changes.
- **Mandarin orange** – the vibrant citrus fruit whose fresh color palette inspired the fork logo.

Together they promise the same Salamander DNA with a spicy hint of danger.

### Installation Notes

Samandarin writes its configuration to a dedicated registry hive named "Open Salamander Samandarin" to avoid interfering with an official installation. If you previously tried the "tabbed panels PoC" pre-release, manually clean the legacy registry keys because that build still stored settings in the original location.


## Origin Overview

The original version of Servant Salamander was developed by Petr Šolín while he was studying at the Czech Technical University. He released it as freeware in 1997. After graduating, Petr Šolín founded [Altap](https://www.altap.cz/) together with Jan Ryšavý. In 2001, they released the first shareware version of the program. In 2007, the project was renamed Altap Salamander with the release of version 2.5. Many other programmers and translators have [contributed](AUTHORS) to the project over the years. In 2019, Altap was acquired by [Fine](https://www.finesoftware.eu/). After the acquisition, Altap Salamander 4.0 was released as freeware. In 2023, the source code was released under the GPLv2 license as Open Salamander 5.0.

The name Servant Salamander came from a brainstorming session between Petr Šolín and his friend Pavel Schreib. At the time, the best-known file managers were the aging Norton Commander and the increasingly popular Windows Commander. They wondered why a file manager should be called a commander at all: a good file manager serves its users rather than commands them. That idea led to the name Servant Salamander.

Salamander was our first major C++ project, and the code reflects both that learning process and the era in which it was built. It does not follow the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines), use smart pointers, rely on [RAII](https://en.cppreference.com/w/cpp/language/raii), or use libraries such as the [STL](https://github.com/microsoft/STL) or [WIL](https://github.com/microsoft/wil). Most of these practices and libraries were still emerging when Salamander was created. Many comments are still written in Czech, but recent advances in AI-assisted translation make them much easier to improve incrementally. Salamander is a pure WinAPI application and does not use application frameworks such as MFC.

We would like to thank [Fine](https://www.finesoftware.eu/) for making the open-source release of Salamander possible.

The Samandarin fork continues this story by blending the original Salamander DNA with AI-driven enhancements, keeping development moving while inviting adventurous contributors to explore and extend the project.


## Resources

- [Security policy](SECURITY.md)
- [Network use and privacy](doc/network-and-privacy.md)
- [Plugin catalogs](doc/catalogs-base/README.md)
- [Open Salamander 5.0](https://github.com/OpenSalamander/salamander) original project
- [Open Salamander SDK 5.0 Unofficial](https://github.com/lejcik/as-sdk4-unofficial)
- [Altap Salamander Website](https://www.altap.cz/)
- Altap Salamander 4.0 [features](https://www.altap.cz/salamander/features/)
- Altap Salamander 4.0 [documentation](https://www.altap.cz/salamander/help/)
- Servant Salamander and Altap Salamander [changelogs](https://www.altap.cz/salamander/changelogs/)
- [User community forum](https://forum.altap.cz/)
- Altap Salamander on [Wikipedia](https://en.wikipedia.org/wiki/Altap_Salamander)

## License

Open Salamander is open-source software licensed under [GPLv2](LICENSE) or later.
Some individual [files and libraries](doc/third_party.md) use different but compatible licenses.
