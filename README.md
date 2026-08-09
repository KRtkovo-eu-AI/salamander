# Open Salamander: Samandarin
Open Salamander: Samandarin is a fast and reliable dual-pane file manager for Windows.

It evolves the [original project](https://github.com/OpenSalamander/salamander) with enhancements and new features while staying compatible with the upstream code and plugin ecosystem.

<img width="1247" height="845" alt="image" src="https://github.com/user-attachments/assets/8cfedd13-c875-4400-b51c-6b9395a16c8e" />


## Included Features Overview
### Unicode and Long paths support
- Work with files and folders whose names use characters from different languages, emojis, and other Unicode symbols.
- When Windows long paths are enabled, local and UNC paths can go beyond the traditional `MAX_PATH` limit and use the Windows extended-length limit of about `32,767` characters.
- Browse, Copy, Move, Rename, Delete, and Create deeply nested folders and very long file names directly from the panels.
- The core file handling paths use Unicode-aware Windows APIs, so names no longer have to fit the active system code page.
- Code based on our prototype and partially derived from [Sally](https://github.com/0xeb/sally) fork source code (author [0xeb](https://github.com/0xeb)) which is being refined for Samandarin.
<img width="687" height="492" alt="image" src="https://github.com/user-attachments/assets/5b6e9d73-7cac-4b1b-a40f-7c3c03c1adb4" />
<img width="687" height="119" alt="image" src="https://github.com/user-attachments/assets/ab04f8a6-fe73-4f0f-8726-12755afdbde7" />

### Internal Viewer
- Support for Unicode encoding in text files (based on [Sally](https://github.com/0xeb/sally) fork source code (author [0xeb](https://github.com/0xeb)) and refined for Samandarin).
- Optional status bar with Line/Column number, counting selected characters and lines, zoom settings.
- Zoom text with Ctrl + mouse wheel or Ctrl + Num +/Num -. Zoom reset with Ctrl + Num 0.
- Optional line numbers for easier navigation in text files.
<img width="800" height="637" alt="image" src="https://github.com/user-attachments/assets/66a19715-e1c4-47bc-9613-b050575fa5e6" />


### Portability
- Save the Configuration into file storage instead of Registry
<img width="687" height="172" alt="image" src="https://github.com/user-attachments/assets/5aab6d4d-cc86-42a5-a769-95af091ecab5" />

### Manage configurations
- Reworked welcome dialog with configuration management options.
- Configuration import and export support.
- Simple wizard for restoring configurations.
- Choose between locations for target configuration.
<img width="782" height="552" alt="image" src="https://github.com/user-attachments/assets/d8c0d5a0-232a-49cf-ae20-6ce5670b8a32" />

### Split panels into detached window
- You can have each side in separate window.
<img width="782" height="316" alt="image" src="https://github.com/user-attachments/assets/bf3a6b9d-4897-4a85-989e-b2cf43840372" />

### Translations
- Automatic translate with OpenAI API (model `gpt-5.4-nano`) with our own custom localization logic.
- Few scripts partially derived from [Sally](https://github.com/0xeb/sally) fork source code (author [0xeb](https://github.com/0xeb)).
<img width="416" height="232" alt="image" src="https://github.com/user-attachments/assets/2d8012d8-d07a-43ed-8140-5b1a0e48fdbb" />

### Tabbed Panels
- One of the most wanted features over last 15 years.
- When tabs overflow the tab bar, you can scroll the tab bar with the mouse wheel.
- You can switch between tabs on one side when holding right mouse button and scrolling with the mouse wheel.
<img width="687" height="147" alt="image" src="https://github.com/user-attachments/assets/cd8919c4-2640-4c99-b83d-94f814100d9f" />
<img width="687" height="244" alt="image" src="https://github.com/user-attachments/assets/d446320b-ee21-49bb-b3cd-f249186f2d41" />
<img width="687" height="479" alt="image" src="https://github.com/user-attachments/assets/30765cbf-a26b-4a2e-8e76-720908342467" />

### Shared/separate History
<img width="687" height="155" alt="image" src="https://github.com/user-attachments/assets/569b023f-9f69-4c19-a830-57e2542b0de5" />

### Dark Mode
- Complete dark mode support for all components, windows, plugins etc.
<img width="687" height="513" alt="image" src="https://github.com/user-attachments/assets/3d124bed-a670-414d-9f21-ea453561bce3" />

### DPI Awareness
- Sharper interface rendering on high-DPI and mixed-DPI Windows displays.
- The application scales dialogs, controls, icons, and panel content more consistently when moving between monitors with different scaling settings.
<img width="1024" height="708" alt="image" src="https://github.com/user-attachments/assets/d4117eba-bebd-46ba-8b4e-0621e7668977" />

### Columns from Windows Explorer
- For detailed view, you can choose to show any Windows Explorer column.
<img width="685" height="717" alt="image" src="https://github.com/user-attachments/assets/dacf844c-8b4e-48e0-a8de-fd407ea68449" />
<img width="675" height="316" alt="image" src="https://github.com/user-attachments/assets/c7a4adc9-5105-4696-81a6-6f779775b837" />

### Configurable Command Shell Application
<img width="687" height="513" alt="image" src="https://github.com/user-attachments/assets/d5109ceb-ec72-43ce-9731-749f3cb13fec" />
<img width="1055" height="397" alt="image" src="https://github.com/user-attachments/assets/26b86292-327e-4d39-bbff-ff96a649bb0f" />

### Tree View panel
- Docked or floating panel with tree view structure of the active disk panel.
- based on [fgodoy](https://github.com/OpenSalamander/salamander/issues?q=is%3Apr+is%3Aopen+author%3Afgodoy) changes.
<img width="689" height="377" alt="image" src="https://github.com/user-attachments/assets/0717d01f-5f59-454c-a821-288b9f74b1fe" />

### User Folders
- Added missing user folders for easier access.
<img width="687" height="513" alt="image" src="https://github.com/user-attachments/assets/515790ee-46e4-40c8-ae5e-407b294afdd1" />

### Copy/Move between plugin-FS and archives
- Support for file operations between plugin-FS and archives (for example between FTP and archives, between different FTP servers, different archives, etc).
<img width="687" height="427" alt="salam_plug_arch" src="https://github.com/user-attachments/assets/4e4fc13c-e6f7-485c-b0d6-623378e1719b" />

### Autocomplete path in Copy/Move/Quick Rename/Create Folder/Change Directory dialog
- Paths are suggested when typing in the path field.
<img width="687" height="89" alt="image" src="https://github.com/user-attachments/assets/59a7cff2-0db2-4866-8d6a-74ccd2f4376f" />
<img width="397" height="185" alt="image" src="https://github.com/user-attachments/assets/3af3f1df-8628-43e9-ad94-7d8bcc52340a" />

### Extended Confirm Delete
- Show the table with filenames of selected files to delete.
<img width="606" height="293" alt="image" src="https://github.com/user-attachments/assets/cf2ee640-c3a8-4a8f-8b6f-db0bd0f6edf2" />
<img width="721" height="513" alt="image" src="https://github.com/user-attachments/assets/30be4ad5-5876-4d5b-919a-d50e0a1275e0" />

### Panel item info tooltip
- Shows the tooltip with summary info about directory or file.
- For known file types, information that might interest users is displayed (for images, this is the size or color depth, for music files, this is the length or author, etc).
<img width="456" height="552" alt="image" src="https://github.com/user-attachments/assets/0aefd5f8-fd8c-4c6f-b770-bbb0dfc14803" />
<img width="456" height="552" alt="image" src="https://github.com/user-attachments/assets/5a94121c-af71-4d9f-a8c8-564eb6e6da92" />

### Plugin Updates in Samandarin plugin
> [!NOTE]
> Plugin catalog sources:
> - Stable: https://samandarin.net/catalogs/plugins-stable.json
> - Unofficial 3rd party: https://samandarin.net/catalogs/plugins-unofficial.json
> - Extension Runtimes: https://samandarin.net/catalogs/extension-runtimes.json
<img width="966" height="643" alt="image" src="https://github.com/user-attachments/assets/54f2a2fe-f5e0-4de3-b5d4-e3a96c71c4d4" />

### Digitally signed
- All binary files including installer are digitally signed
<img width="405" height="485" alt="image" src="https://github.com/user-attachments/assets/a210db84-eee7-4b49-987a-09dd35ca56c1" />

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
