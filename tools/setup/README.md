# setup.inf helper scripts

The installer produced by `setup.sln` gets all of its content from
`setup.inf`. Every new set of plugin files therefore has to be added by
hand to the `[CopyFiles]` and `[CreateDirs]` sections. Maintaining large
managed plugins (TextViewer, WebView2RenderViewer) manually is error
prone, because they contain dozens or even hundreds of supporting files.

The `update_setup_inf.py` script automates this step: it scans the
staging tree at `%OPENSAL_BUILD_DIR%\salamander\Release_{x86,x64}\plugins`
and generates entries for the `jsonviewer`, `textviewer`,
`webview2renderviewer`, and `samandarin` plugins.

## Usage

1. Build Salamander (ideally for both architectures) so that the
   packageable plugin files are available at
   `%OPENSAL_BUILD_DIR%\salamander\Release_x64\plugins` and
   `%OPENSAL_BUILD_DIR%\salamander\Release_x86\plugins`.
2. Prepare `setup.inf`—you can start from the official INF and adjust
   the `[Private]`, `[CopyFiles]`, and other sections as needed.
3. Run the script:

   ```powershell
   python tools\setup\update_setup_inf.py \` \
       --build-root "$env:OPENSAL_BUILD_DIR" \` \
       --setup-inf "$env:OPENSAL_BUILD_DIR\setup\Release_x64\setup.inf"
   ```

   If you maintain a separate `setup.inf` for the x86 installer, run the
   script again with the appropriate path (and a different `--build-root`
   if required).

The script:

- removes the existing lines related to the plugins listed above,
- adds new source → destination pairs to `[CopyFiles]` including an
  architecture comment,
- ensures all required subdirectories are created in `[CreateDirs]`,
- creates a `setup.inf.bak` backup on the first write (unless you use
  `--no-backup`).

Copy the resulting `setup.inf` next to `setup.exe`/`remove.exe` and the
installer will include the new plugins together with their managed
dependencies.
