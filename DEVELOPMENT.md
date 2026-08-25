## Development

### Prerequisites
- Windows 11 or newer
- [Visual Studio 2026](https://visualstudio.microsoft.com/downloads/)
- [Desktop development with C++](https://learn.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-170) workload installed in VS2026
- [Windows 11 (10.0.26100.4654) SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/) optional component installed in VS2026

### Optional requirements
- [Git](https://git-scm.com/downloads)
- [PowerShell 7.4](https://learn.microsoft.com/en-us/powershell/scripting/install/installing-powershell-on-windows) or newer
- [HTMLHelp Workshop 1.3](https://learn.microsoft.com/en-us/answers/questions/265752/htmlhelp-workshop-download-for-chm-compiler-instal)
- Set the `OPENSAL_BUILD_DIR` environment variable to specify the build directory. The path must include a trailing backslash, for example `D:\Build\OpenSal\`.

### Building

You can build the `\src\vcxproj\salamand.sln` solution in Visual Studio or from the command line with `\src\vcxproj\rebuild.cmd`.

Use `\src\vcxproj\!populate_build_dir.cmd` to populate the build directory with the files required to run Open Salamander.

### Contributing

Contributions that help build, maintain, and improve Open Salamander are welcome.

## Repository Contents

```
\convert         Conversion tables for the Convert command
\doc             Documentation
\help            User manual source files
\src             Open Salamander core source code
\src\common      Shared libraries
\src\common\dep  Shared third-party libraries
\src\lang        English resources
\src\plugins     Plugin source code
\src\reglib      Access to Windows Registry files
\src\res         Image resources
\src\salmon      Crash detection and reporting
\src\salopen     Open files helper
\src\salspawn    Process spawning helper
\src\setup       Installer and uninstaller
\src\sfx7zip     Self-extractor based on 7-Zip
\src\shellext    Shell extension DLL
\src\translator  Tools for translating the Salamander UI into other languages
\src\tserver     Trace server for displaying information and error messages
\src\vcxproj     Visual Studio project files
\tools           Minor utilities
\translations    Translations into other languages
```

Some Altap Salamander 4.0 plugins are either not included or cannot currently be compiled. For example, the PictView engine, `pvw32cnv.dll`, is not open source, so the project should eventually move to [WIC](https://learn.microsoft.com/en-us/windows/win32/wic/-wic-about-windows-imaging-codec) or another image library. The Encrypt plugin is incompatible with modern SSDs and has been deprecated. The UnRAR plugin is missing [unrar.dll](https://www.rarlab.com/rar_add.htm), and the FTP plugin is missing the [OpenSSL](https://www.openssl.org/) libraries. Both issues are solvable because both projects are open source. Building the WinSCP plugin requires Embarcadero C++ Builder.

All source files use UTF-8 with BOM and are formatted with `clang-format`. See `\normalize.ps1` for details.
