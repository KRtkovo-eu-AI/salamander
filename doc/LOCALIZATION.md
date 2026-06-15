# How to Translate Samandarin

For routine work, use only `tools/localization/localize.ps1`. The other scripts in that directory are implementation details.

## Are New Texts from Source Resources Added?

**Yes.** The `start` command works as follows:

1. it takes the current `english.slg` from the build as the complete project skeleton,
2. exports the current `.slt` skeleton from it,
3. automatically converts the old `.slt` translation to that skeleton using resource IDs,
4. imports the converted archive and opens the result in Translator.

Existing translated items are therefore preserved, while new strings, dialogs, or menus from the current `.rc`/`.rh2` resources remain **untranslated** in Translator. You do not need to add them to translation templates manually.

The automatic conversion is important: Translator cannot directly import an old `.slt` if the resource structure has changed in the meantime. Therefore, `localize.ps1 start` automatically rebases the old archive onto the current skeleton before importing it. An error such as `Syntax error ... salamand.slt on line ...` should not appear during normal use.

The only important requirement is that you rebuild and populate the build from the current sources before running `start`. The script reads new resources from the resulting `english.slg`, not directly from the source `.rc` files. If you use an old build, it will not contain the new texts yet.

## Quickest Workflow

You need Windows, PowerShell 7 (`pwsh`), and a completed x64 build containing `salamand.exe`, plugins, English `.slg` files, and `utils/translator.exe`.

### 1. Open a Translation

For example, the Czech translation of the main window:

```powershell
pwsh -File .\tools\localization\localize.ps1 start czech salamand `
  -BuildRoot .\build\out\salamand\Release_x64
```

Or the Czech translation of the Samandarin plugin:

```powershell
pwsh -File .\tools\localization\localize.ps1 start czech samandarin `
  -BuildRoot .\build\out\salamand\Release_x64
```

The command prepares everything required, loads the existing translation onto the current English resource skeleton, and automatically opens Translator.

### 2. Work in Translator

1. Translate untranslated items. New texts added since the last translation remain marked as untranslated.
2. For dialogs, verify that translated text fits inside the controls.
3. Save the project with **Ctrl+S**.
4. Close Translator.

If you later need to reopen Translator without preparing a new workspace:

```powershell
pwsh -File .\tools\localization\localize.ps1 open czech salamand
```

### 3. Save the Result to the Repository

```powershell
pwsh -File .\tools\localization\localize.ps1 finish czech salamand
```

The result is `translations/czech/salamand.slt`, which you should review and commit. For the `samandarin` plugin, both the command and the resulting file would use the name `samandarin`.

That is the complete routine translation workflow: **start → translate and Ctrl+S → finish**.

## Useful Commands

List plugins that are missing translation archives:

```powershell
pwsh -File .\tools\localization\localize.ps1 check
```

Build all available language `.slg` files into the completed runtime:

```powershell
pwsh -File .\tools\localization\localize.ps1 build `
  -BuildRoot .\build\out\salamand\Release_x64
```

## When a New Text Is Missing from Translator

Translator can translate only texts stored in Windows resources. Therefore, user-facing text must not be hard-coded in C/C++/C# code.

- A string must have a stable resource ID in the relevant `.rh2`, English text in the string table, and code that loads it through `LoadStr`, `LoadString`, or the corresponding localization API.
- Dialogs and menus must be in the relevant `.rc`.
- A new plugin must build `plugins/<plugin>/lang/english.slg`.

After fixing the resources, rebuild/populate the application and run `localize.ps1 start ...` again. The current English `.slg` is the source of truth, so the new text will then appear as untranslated.

## What to Commit

Commit:

- changed resources and code,
- the resulting `translations/<language>/<module>.slt`,
- any changes to documentation and localization tools.

Do not commit the `out/localization` directory, `.atp` files, generated `.slg` files, or logs.

## Troubleshooting

- **`Build root ... does not look like ...`**: the path passed through `-BuildRoot` does not contain a populated build with `salamand.exe`.
- **Missing `translator.exe`**: the build must contain `utils/translator.exe`.
- **Unknown module**: the plugin in the build does not have `plugins/<plugin>/lang/english.slg`, or its name is misspelled.
- **New resource texts are missing from Translator**: `-BuildRoot` probably points to an old build; rebuild/populate the current sources and run `start` again.
- **Importing the converted archive fails**: the `start` command exits with an error and a reference to the existing `out/localization/localize.log`; Translator does not open a broken project.
- **The `<module>.quiet.log` file does not exist**: Translator in this repository does not create this log and returns the historical exit code `1` for a successful quiet operation. The wrapper therefore writes its own `out/localization/localize.log`.
- **`Syntax error ... on line 1`**: you used an older wrapper that exported a diff archive without the required `[EXPORTINFO]` section during rebase; update the repository and run `start` again.
- **`$LASTEXITCODE cannot be retrieved because it has not been set`**: you used an older version of `localize.ps1` that launched the GUI `translator.exe` directly; update the repository and run the same command again.
- **I need an advanced rebase or headless validation**: use the helper scripts `rebase_text_archive.ps1` and `verify_translation_workspace.ps1`; they are not needed for routine translation.
