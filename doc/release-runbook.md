<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-13 | Check removal of English language seeds from release payloads. | Ship embedded English resources without english.slg intermediates. | | openrouter/openai/gpt-6-luna-pro |
| 2026-11-13 | Update language-pack release checks for plug-in resources. | Verify embedded translations before signing distributable .spl files. | | openrouter/openai/gpt-6-luna-pro |
| 2026-11-12 | Add the release runbook. | Document the manual Open Samandarin release workflow. | | openai/gpt-6-luna-pro |
-->

# Open Samandarin release runbook

This runbook describes the release sequence for the Windows x64 product and its plugins/extensions. It uses the repository's existing build, localization, signing, packaging, catalog, and upload helpers. Website publication and branch merging are manual steps.

Run the commands below in **PowerShell 7** (`pwsh`) from the repository root unless a step says otherwise. Replace example release numbers, branches, paths, and credentials with the values approved for the release. Do not stage or commit unrelated working-tree changes.

## Before starting

1. Confirm that the release changes are based on the intended development commit and that you know the release branch, release-preparation branch, release number, and website checkout location.
2. Check the working tree before making changes:

   ```powershell
   git status --short --branch
   ```

   Keep unrelated changes out of the release commit. Do not use `git add .` unless you have verified every changed file is part of this release.
3. Install/use the documented Windows build prerequisites: Windows 11, Visual Studio 2026 with the Desktop development with C++ workload, PowerShell 7.4+, Git, Python, and 7-Zip. Translation additionally needs an OpenRouter API key; signing needs a logged-in Certum SimplySign certificate and Windows SDK SignTool.
4. Set the build directory for this PowerShell session. `OPENSAL_BUILD_DIR` must end with a backslash. Use a location with enough free disk space:

   ```powershell
   $env:OPENSAL_BUILD_DIR = 'D:\OpenSamandarin\build\'
   $buildRoot = Join-Path $env:OPENSAL_BUILD_DIR 'salamander\Release_x64'
   $releaseId = '5.0-samandarin-0.17'
   $packageDir = Join-Path (Get-Location).Path "out\plugin-packages-$releaseId"
   ```

   These are examples; change the paths and release ID to match the machine and release. Keep the same `$buildRoot` for every later step.
5. Keep this PowerShell window open so its `$env:` settings and variables remain available. If you open a new window, repeat the setup commands and any release variables needed in later steps.

## 1. Bump plugin and extension versions

First decide which plugins and extensions are included in this release. The plugin version helper compares plugin source changes with a baseline tag and offers a proposed version bump for each affected plugin. It is interactive; read each proposal and accept, edit, skip, or quit as appropriate.

Preview plugin version changes without writing them:

```powershell
python .\tools\bump_plugin_versions.py --dry-run
```

If the preview is correct, run the helper again without `--dry-run` and respond to its prompts:

```powershell
python .\tools\bump_plugin_versions.py
```

The helper updates plugin `versinfo.rh2` files only; it does not update extension versions. For each extension being released, edit its `extension.json` and update the string in its `version` field. Do not change versions of unrelated extensions. Review the resulting diff and ensure each version is the intended next version before building.

The separate `tools/bump_samandarin_version.py` helper changes the main application's version and compatibility metadata and has no dry-run option. Use it only when the main Open Samandarin product version is also meant to advance.

## 2. Build the Release_x64 application

Open `src\vcxproj\salamand.sln` in Visual Studio, select **Release** and **x64**, and build the solution. Alternatively, run the repository build and populate commands from the repository root:

```powershell
& .\src\vcxproj\rebuild.cmd
& .\src\vcxproj\!populate_build_dir.cmd
```

The populated runtime directory is `$buildRoot` (for example, `D:\OpenSamandarin\build\salamander\Release_x64`). Confirm the build completed successfully and the runtime contains `salamand.exe`, the expected plugin/extension directories, the English language files, and `utils\translator.exe` before continuing. The localization scripts use this populated runtime; do not translate from an old or incomplete build.

## 3. Run OpenRouter automatic translation

The batch translator requires an OpenRouter key in the current PowerShell process. Set it here; do not put it in a source file, commit it, or paste it into release notes. The model setting is optional; `openai/gpt-5.4-nano` is the script's documented default.

```powershell
$env:OPENROUTER_API_KEY = 'PASTE_YOUR_OPENROUTER_KEY_HERE'
$env:OPENROUTER_MODEL = 'openai/gpt-5.4-nano' # Optional; remove this line to use the script default.
$env:TRANSLATION_PROVIDER = 'openrouter'        # Optional; OpenRouter is the default provider.
```

Start with a dry run:

```powershell
pwsh -File .\tools\localization\localize_all_openai.ps1 -BuildRoot $buildRoot -DryRun
```

**Dry run still sends translation requests to OpenRouter.** It writes candidate translation files and a report under `out\localization-openai\candidate`, but does not apply them to `translations`, run the Translator validation/export, or build language packs. Review the candidates and report. When ready, run the same command without `-DryRun`:

```powershell
pwsh -File .\tools\localization\localize_all_openai.ps1 -BuildRoot $buildRoot
```

Review the resulting `.slt` changes for meaning, terminology, accelerator keys, placeholders, and dialog fit. You can limit a run with `-Languages` and `-Modules`. Do not use `-ForceRetranslate` unless you intentionally want to replace existing translated entries. After the translation work, clear the key from this PowerShell session:

```powershell
$env:OPENROUTER_API_KEY = $null
```

## 4. Build language packs

Build the language packs after translation and review have succeeded:

```powershell
pwsh -File .\tools\localization\localize.ps1 build -BuildRoot $buildRoot
```

The build validates the translation layout and `.slt` round trip. Stop and fix errors instead of shipping packs when validation fails. Confirm that `lang\langpack.dll` contains the main-application translations and each translated first-party plugin's `.spl` contains its own resources; all generated `.slg` intermediates, including `english.slg` seeds, should have been removed from the runtime tree. Commit source `.slt` translation files when they are part of the release; do not commit generated `.slg` files or temporary output directories. Complete this resource update before signing.

## 5. Manually test the built project

Test the populated Release_x64 build, not a previous installation or a stale staging directory. At minimum:

- Start Salamander and check that the main window opens without an error.
- Check that the released plugins load and their key functions work. Test extensions and the extension runtime when included.
- Select several generated languages and check host/plugin menus, dialogs, accelerators, placeholders, and text clipping.
- Exercise the release-specific fixes/features and any relevant existing regression tests.
- Check startup and relevant logs for missing files, load errors, or crashes.

The optional runtime package checker catches common packaging problems but does not replace manual testing:

```powershell
pwsh -File .\tools\verify_runtime_packages.ps1 -SalamanderPath $buildRoot
```

Record the tested build and results. If a test fails, fix the source, rebuild, regenerate affected translations/packs and packages, and repeat the tests before signing.

## 6. Digitally sign the tested build (when required)

Only sign after the manual tests pass. SignTool and the Certum SimplySign application must be installed; sign in to SimplySign Desktop and confirm the signing certificate is available in the current user's certificate store. Copy its certificate thumbprint without spaces, then set the required variables in this PowerShell session:

```powershell
$env:CODESIGN_ENABLED = '1'
$env:CODESIGN_CERT_SHA1 = 'PASTE_CERTIFICATE_THUMBPRINT_WITHOUT_SPACES'
```

Run the existing signing helper:

```powershell
& .\tools\codesign\codesign_certum.cmd --inno-x64 --payload-dir $buildRoot
```

This helper signs/verifies the executable and payload files selected by the Inno Setup script; it does not mean that every file in every plugin archive was signed. The final installer signing step is separate. Check the helper's result and signatures, and do not modify signed files afterward. Clear the thumbprint from the current session when finished:

```powershell
$env:CODESIGN_CERT_SHA1 = $null
$env:CODESIGN_ENABLED = $null
```

## 7. Package plugins and extensions as 7z archives

Use a new output directory for this release so that an old archive cannot be mistaken for the new one. Ensure 7-Zip is on `PATH` (`7z`, `7zz`, or `7za`) or pass its location with the script's `-SevenZipPath` option.

```powershell
pwsh -File .\tools\package_salamander_plugins.ps1 `
  -SalamanderPath $buildRoot `
  -OutputPath $packageDir
```

The packager discovers plugins, runtime plugins, extensions, and supported demo packages. It reads plugin versions from the plugin binary metadata and extension versions from `extension.json`; archive names follow `plugin_5.0_<package>_<version>_x64.7z`. It writes `package-sha256.txt` and bundled-plugin metadata. Review the console output, list the resulting archives, and check that every expected package is present with the correct version:

```powershell
Get-ChildItem -LiteralPath $packageDir -Filter '*.7z' | Select-Object Name, Length
Get-Content -LiteralPath (Join-Path $packageDir 'package-sha256.txt')
```

The script skips an archive that already exists. Prefer a fresh output directory; use `-Force` only when you deliberately intend to overwrite existing archives.

## 8. Upload each updated package to salamander-plugins

For each `.7z` that is being released, call the existing URL helper once. Example (replace the archive name with one printed in step 7):

```powershell
$archive = Get-Item (Join-Path $packageDir 'plugin_5.0_ftp_5.01_x64.7z')
pwsh -File .\tools\new_plugin_release_url.ps1 -ArchivePath $archive.FullName
```

The helper defaults to the `KRtkovo-eu-AI/salamander-plugins` GitHub repository, calculates checksums, and opens a prefilled **new release** page. It does **not** upload or publish the archive. In the browser, verify the package/tag and description, attach the exact `.7z` file selected above, and publish the plugin release. Repeat for every updated package. Use `-PluginDescription` only if the helper does not know the package description. The helper may update a matching local catalog hash; still verify the catalogs in the following steps.

## 9. Update the release catalogs

Update the catalog entries for every package being released. Select the correct stable plugin, stable extension, runtime, or unofficial catalog for each package. The JSON files under `doc\catalogs-base` are base templates and may not be the current live catalogs.

Repeat these checks for each archive. Set `$archive` to that archive (as in step 8), then get its SHA-256 from the archive itself:

```powershell
Get-FileHash -LiteralPath $archive.FullName -Algorithm SHA256
```

Set the corresponding catalog entry's `packageSha256` to that exact digest in lowercase, and ensure its download URL points to the published package release. The helper used by the packager can fill matching local hashes; if needed, it is also available directly:

```powershell
python .\tools\catalogs\fill_package_hashes.py --archive $archive.FullName
```

Review the JSON diff and verify the archive URL, version, description, and hash for each changed entry. Official catalog consumers reject a missing or mismatching package hash, so the live catalog must be updated before publishing a client release that requires the hash.

## 10. Merge the final release commit into the release branch

Finish and commit the source-side release changes first. On the release-preparation branch, stage only the intended files, inspect the staged diff, and make the final release commit. Replace the sample paths below with the actual changed files; do not stage the whole working tree:

```powershell
$workBranch = 'release-prep/5.0-samandarin-0.17'
$releaseBranch = 'release/5.0-samandarin-0.17'
git switch $workBranch
git status --short --branch
$releaseFiles = @('src/plugins/<plugin>/versinfo.rh2', 'path/to/other-release-file')
git add -- $releaseFiles
git diff --cached --check
git diff --cached
git commit -m 'Prepare Open Samandarin 5.0-samandarin-0.17 release'
```

Then merge the release-preparation branch into the approved release branch. The names below are examples; use the exact branch names approved for this release:

```powershell
git fetch origin
git switch $releaseBranch
git pull --ff-only origin $releaseBranch
git merge --no-ff $workBranch
git status --short --branch
git log -1 --oneline
```

If `git switch $releaseBranch` reports that the local branch does not exist, create it from its remote tracking branch with `git switch --track "origin/$releaseBranch"`, then continue with the pull and merge.

Resolve any merge conflicts, inspect the final result, and run the required checks. Push the release branch if direct pushes are permitted:

```powershell
git push origin $releaseBranch
```

If the release branch is protected, use the repository's required pull-request process instead of the direct merge/push commands. Confirm the final release commit is on the release branch before tagging it.

## 11. Update the `_ghpages` release information

In the website checkout, update the release's displayed version and descriptions, the version-comparison information/page, and the website changelog. Follow the existing formatting and copy for the preceding release. Check that the displayed links and release number refer to the release branch/tag being prepared. The public comparison page is `https://samandarin.krtkovo.eu/compare.html`; the main repository links its current changelist from `README.md` (currently `doc/changelog-0.15.md`).

Set the website checkout path, then inspect its branch names:

```powershell
$webRepo = 'D:\path\to\website-repository' # Replace with the website checkout path.
git -C $webRepo branch --all
```

Work on the website's `_ghpages` branch (or the exact equivalent branch name used by your checkout). This website content is a separate set of changes from the source release commit. Review the page diff in a browser preview or the site's normal preview before publishing.

## 12. Sync the updated catalogs to the website repository

Copy/sync the reviewed catalog JSON files from the release source into the website repository's published `catalogs` directory, preserving the site's directory structure. Sync only the catalogs changed for this release. Check that the website copies contain the same package versions, download URLs, and lowercase `packageSha256` values as step 9. The live catalog endpoints are:

- `https://samandarin.krtkovo.eu/catalogs/plugins-stable.json`
- `https://samandarin.krtkovo.eu/catalogs/extensions-stable.json`
- `https://samandarin.krtkovo.eu/catalogs/extension-runtimes.json`
- `https://samandarin.krtkovo.eu/catalogs/plugins-unofficial.json`

For a catalog whose source template is confirmed current, this PowerShell pattern copies it into the matching website folder; change the filename for each catalog being synchronized:

```powershell
$catalogName = 'plugins-stable.json'
$sourceCatalog = Join-Path (Get-Location).Path "doc\catalogs-base\$catalogName"
$websiteCatalog = Join-Path $webRepo "catalogs\$catalogName"
Copy-Item -LiteralPath $sourceCatalog -Destination $websiteCatalog
```

The source catalog templates can lag behind those published files. Compare the website catalog with the intended live contents before copying; if the template is older, merge the approved entry changes into the website copy instead of replacing it. Review the website diff before committing.

## 13. Create the source tag and publish the product release

Create the tag only after step 10's final release commit is on the release branch. The repository's release reports use tags such as `5.0-samandarin-0.16`; use the approved next tag, not this example if it is already taken.

```powershell
$tag = '5.0-samandarin-0.17'
git switch $releaseBranch
git pull --ff-only origin $releaseBranch
git log -1 --oneline
git tag -a $tag -m $tag
git push origin $tag
git show --no-patch --decorate $tag
```

If the tag already exists or points to the wrong commit, stop and resolve that with the release maintainer; do not force-move a published tag. In the source repository's GitHub Releases page, create a release from this tag, use the approved release title/notes and attach the final product artifacts required for this release. Verify the release page and assets after publishing. Plugin archives were published separately in step 8.

## 14. Commit and push the website branch

After the version pages, descriptions, comparison information, changelog, and synced catalogs have all been reviewed, commit them on the website branch and push that branch. Set `$webRepo` to the path of the website repository checkout and `$webBranch` to its actual branch name:

```powershell
$webRepo = 'D:\path\to\website-repository' # Replace with the website checkout path.
$webBranch = '_ghpages'                     # Use the exact branch name in that checkout.
git -C $webRepo status --short --branch
```

Read the status output and stage only the website files changed for this release. Replace the example paths below with the actual relative paths shown by `git status`:

```powershell
$webFiles = @('path/to/release-page', 'catalogs/plugins-stable.json')
git -C $webRepo add -- $webFiles
git -C $webRepo diff --cached --check
git -C $webRepo diff --cached
git -C $webRepo commit -m 'Update website for Open Samandarin 5.0-samandarin-0.17'
git -C $webRepo push origin $webBranch
```

If the web branch is protected, use its pull-request process. After publishing, verify the live version/comparison/changelog pages and the catalog URLs listed in step 12.

## Repository references

- [`DEVELOPMENT.md`](../DEVELOPMENT.md) — Windows build prerequisites and build-directory setup.
- [`doc/LOCALIZATION.md`](LOCALIZATION.md) — translation and language-pack workflow.
- [`tools/codesign/codesign.md`](../tools/codesign/codesign.md) — manual signing requirements and signing helper.
- [`doc/catalogs-base/README.md`](catalogs-base/README.md) — catalog formats and package hash requirements.
- [`tools/README.md`](../tools/README.md) — package URL helper and runtime package verification.
- [`doc/code-growth/releases/README.md`](code-growth/releases/README.md) — release-tag reporting convention.
