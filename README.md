# Open Salamander: Samandarin
Open Salamander: Samandarin is a fast and reliable two-panel file manager for Windows, refreshed in 2025 with features implemented entirely with [OpenAI Codex](https://chatgpt.com/codex).

It evolves the [original project](https://github.com/OpenSalamander/salamander) with **experimental AI-crafted enhancements** while staying compatible with the upstream code and plugin ecosystem.

<img width="1386" height="862" alt="image" src="https://github.com/user-attachments/assets/7dab1d1e-ce95-40e6-8870-fa440e54c097" />


#

> [!IMPORTANT]
> Open Salamander: Samandarin remains an **intentionally experimental fork**—its **AI-implemented features** are actively evolving, **may surface unexpected issues**, and are best explored by **advanced, adventurous users** who can tolerate work-in-progress behavior.


## Included Features Overview
### Translations
<img width="416" height="232" alt="image" src="https://github.com/user-attachments/assets/2d8012d8-d07a-43ed-8140-5b1a0e48fdbb" />

- automatic translate with OpenAI API (model gpt-5.4-nano)

### Tabbed Panels
<img width="687" height="147" alt="image" src="https://github.com/user-attachments/assets/cd8919c4-2640-4c99-b83d-94f814100d9f" />
<img width="687" height="247" alt="image" src="https://github.com/user-attachments/assets/1443370b-e663-48dd-807a-26d17b115c41" />
<img width="687" height="479" alt="image" src="https://github.com/user-attachments/assets/30765cbf-a26b-4a2e-8e76-720908342467" />

- When tabs overflow the tab bar, you can scroll the tab bar with the mouse wheel.
- You can switch between tabs on one side when holding right mouse button and scrolling with the mouse wheel.

### Shared/separate History
<img width="687" height="154" alt="image" src="https://github.com/user-attachments/assets/a8e16767-4df5-4d11-8767-7701464611a6" />

### Dark Mode
<img width="687" height="510" alt="image" src="https://github.com/user-attachments/assets/2d326f09-305b-43f3-92e7-4ccfaf20e037" />

### Configurable Command Shell Application
<img width="687" height="510" alt="image" src="https://github.com/user-attachments/assets/ac5d1e60-3e8d-45b2-8e21-334880780dc4" />

### Tree View panel 
- based on [fgodoy](https://github.com/OpenSalamander/salamander/issues?q=is%3Apr+is%3Aopen+author%3Afgodoy) changes
<img width="690" height="480" alt="image" src="https://github.com/user-attachments/assets/09aaf1a8-378e-4b6b-9668-12be74b900a9" />

### Internal Viewer support for Unicode encoding in text files

### User Folders
<img width="687" height="510" alt="image" src="https://github.com/user-attachments/assets/f2295756-b5e6-44db-ae40-d22075e47d88" />

### Copy/Move between plugin-FS and archives
<img width="687" height="427" alt="salam_plug_arch" src="https://github.com/user-attachments/assets/4e4fc13c-e6f7-485c-b0d6-623378e1719b" />


## Included Plugins Overview
### 7-Zip 1.31
### Automation 1.7
### Checksum 2.2
### Database Viewer 1.24
### DiskMap 1.12
### FTP Client 1.35
### File Comparator 1.19
### Folders 0.1
### Hyper-V Machines 1.04
Show local Hyper-V virtual machines. You can Start/Turn Off/Shut Down/Connect/Create New Machine through the plugin.

### JSON Viewer .NET 1.0
Supported File Types
| Description | Extensions |
| --- | --- |
| JSON data interchange files | `.json`, `.pc`, `.jbeam` |

### Multimedia Viewer 1.16
### Network 1.08
### PAK 1.71
### PictView 2.21
### Portable Devices 0.1
### Portable Executable Viewer 3.0
### PrismSharp Text Viewer .NET 1.0
The viewer focuses on human-readable text content and syntax-highlighted source files. Extensions are matched case-insensitively.
Common text and configuration formats
| Category | Extensions |
| --- | --- |
| Plain text and logs | `.txt`, `.log` |
| Configuration files | `.ini`, `.cfg`, `.conf`, `.config` |
| JSON family | `.json`, `.jsonc`, `.json5` |
| YAML | `.yaml`, `.yml` |
| XML and markup | `.xml`, `.html`, `.htm`, `.php`, `.axaml`, `.xaml`, `.xlf`, `.nuspec`, `.plist`, `.storyboard` |
| Markdown | `.md`, `.markdown` |
| Windows scripting | `.bat`, `.cmd`, `.ps1`, `.psd1`, `.psm1` |
| C-family source | `.cs`, `.cpp`, `.c`, `.cxx`, `.h`, `.hh`, `.hpp`, `.hxx` |
| Project and build files | `.csproj`, `.fsproj`, `.vbproj`, `.vcxproj`, `.vcproj`, `.props`, `.targets` |

#### Prism syntax highlighting identifiers
The viewer also registers [Prism lexers](https://github.com/KRtkovo-eu-AI/salamander/blob/release/5.0-samandarin-0.1/src/plugins/textviewer/SUPPORTED_FILE_TYPES.md) so matching file extensions open in this viewer. 

### Registry Editor 1.14
### Renamer 1.13
### Samandarin Update Notifier 0.2
### Service Explorer 0.012
### Split & Combine 1.11
### TAR 3.34
### UnARJ 1.21
### UnCAB 1.27
### UnCHM 1.03
### UnFAT 1.1
### UnISO 1.37
### UnLHA 1.13
### UnMIME 1.14
### UnOLE2 1.01
### UnRAR 3.01
### UnDelete 1.11
### WebView2 Render Viewer .NET 1.0
It acts as a universal document canvas, covering web pages, Markdown (including MDX), SVG, modern image formats like WebP and AVIF, classic raster files, and PDFs up to 32 MB.
Supported File Types
| Category | Extensions |
| --- | --- |
| HTML and web archives | `.html`, `.htm`, `.xhtml`, `.mhtml`, `.mht` |
| Markdown (rendered to HTML) | `.md`, `.markdown`, `.mdown`, `.mkd`, `.mdx` |
| SVG vector graphics | `.svg`, `.svgz` |
| Modern image formats | `.webp`, `.avif`, `.apng` |
| Raster image formats | `.png`, `.jpg`, `.jpeg`, `.jfif`, `.gif`, `.bmp`, `.ico`, `.tif`, `.tiff` |
| Portable Document Format | `.pdf` |

### Windows Mobile 1.08
### ZIP 1.4




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
Some individual [files and libraries](doc/third_party.txt) use different but compatible licenses.
