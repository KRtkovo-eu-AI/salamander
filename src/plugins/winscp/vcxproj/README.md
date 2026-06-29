# WinSCP Visual Studio migration

This directory contains the Visual Studio entry point for the WinSCP Open Salamander plugin.  The old Borland C++Builder 6 projects are kept next to the original source files for history, but the solution in this directory is the build graph intended for Visual Studio and 64-bit plugin work.

## Projects

- `winscp.sln` is the Visual Studio solution and exposes both `Win32` and `x64` platforms.
- `winscp.vcxproj` builds the Salamander plugin DLL with the `.spl` target extension and references the migrated static-library projects.
- `winscp_scp_core.vcxproj` migrates the old `ScpCore.bpr` static library.
- `winscp_putty.vcxproj` migrates the old `Putty.bpr` static library.
- `winscp_forms.vcxproj` migrates the old `SalamandForms.bpr` and package/form sources into a static library project.
- `winscp.props` centralizes WinSCP-specific linker settings shared by the migrated projects.

## Notes

The solution follows the same property-sheet layout used by the other Visual Studio plugin projects in this tree: platform sheets from `src/plugins/shared/vcxproj`, the common plugin base sheet, and per-configuration debug/release sheets.  Use `Release|x64` to build the 64-bit plugin configuration.
