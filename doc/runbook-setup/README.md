# setup.inf how to runbook

Use this runbook after all binaries and plugins have been built and copied into
`%OPENSAL_BUILD_DIR%` (for example by running `build.cmd` followed by
`!populate_build_dir.cmd`). The goal is to produce redistributable
`setup.exe`/`remove.exe` installers that include every staged plugin, including
new managed plugins such as JsonViewer, TextViewer, WebView2RenderViewer and
Samandarin.

## Prerequisites

- Run everything from a **Developer Command Prompt for VS 2022** so
  `msbuild.exe` is available and `%OPENSAL_BUILD_DIR%` is defined (keep the
  trailing backslash, e.g. `H:\_projects\salamander\output\`).
- `setup.inf` templates for each architecture (e.g. the customised
  `tools\setup_x64.inf` and `tools\setup_x86.inf`) that already list all
  plugins in their `[CreateDirs]` / `[CopyFiles]` sections.
- A clean staging tree under
  `%OPENSAL_BUILD_DIR%\salamander\Release_x64` and
  `%OPENSAL_BUILD_DIR%\salamander\Release_x86` containing only files you wish
  to ship. If you want to remove intermediate objects, run
  `src\vcxproj\!clean_all_interm.cmd` before building the installer.

## Step-by-step

1. **Ensure build outputs are complete**
   - Verify that every plugin directory in
     `%OPENSAL_BUILD_DIR%\salamander\Release_{x64,x86}\plugins` contains the
     expected `.spl`, managed DLLs, language packs, data folders, etc.
   - If you keep PDBs for debugging, leave them in place; otherwise delete them
     now to shrink the installer payload.

2. **Stage the correct `setup.inf` files**
   - Copy your prepared INF next to the installer output before (or immediately
     after) compilation:
     ```cmd
     copy tools\setup_x64.inf %OPENSAL_BUILD_DIR%\setup\Release_x64\setup.inf
     copy tools\setup_x86.inf %OPENSAL_BUILD_DIR%\setup\Release_x86\setup.inf
     ```
     Adjust paths if you maintain alternative INF files. The installer reads the
     INF located beside `setup.exe` at runtime; rebuilding the solution does not
     recreate it automatically.

3. **Build the installer executables**
   - From the repository root, run:
     ```cmd
     cd src\vcxproj\setup
     msbuild setup.sln /t:Rebuild /p:Configuration=Release /p:Platform=x64
     msbuild setup.sln /t:Rebuild /p:Configuration=Release /p:Platform=Win32
     ```
     The outputs appear under
     `%OPENSAL_BUILD_DIR%\setup\Release_x64\setup.exe` and
     `%OPENSAL_BUILD_DIR%\setup\Release_x86\setup.exe`.

4. **Validate the results**
   - Confirm that `setup.exe`, `remove.exe`, and the matching `setup.inf` share
     the same directory.
   - Optionally run the installer in a VM or use `setup.exe /extract` to review
     the embedded file list and ensure all new plugins are packaged.

5. **Package for distribution**
   - Archive or sign the `setup.exe` (and optionally `setup.inf`) as required.
     Keep the matching `remove.exe` so the uninstaller remains branded for your
     fork.

Re-run the sequence whenever plugin outputs or `setup.inf` change. Keeping the
INF templates in `tools/` ensures they can be version-controlled alongside the
source tree.