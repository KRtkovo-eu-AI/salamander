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

The `build` command is the release step for language packs. Internally it calls
`tools/localization/build_language_packs.ps1`, creates a disposable Translator
workspace, imports committed `translations/<language>/<module>.slt` archives into
copies of the current English `.slg` files, validates layout, verifies an SLT
round-trip, and then copies the produced `.slg` files back into the populated
runtime tree.

If the committed files under `translations/` already contain the correct text, do
not re-encode them and do not send them through the translation API again. Build Salamander,
build the current `utils\translator.exe`, and run only the `localize.ps1 build`
command above. When a fatal validation error occurs, the script does not copy new
`.slg` files into the runtime tree; fix the reported problem and run the build
again.

This step does not change Salamander's runtime architecture: `.slg` files must be
correct before the application starts. If round-trip validation fails, it is not
a warning to ignore — that language pack must not be smoke-tested or shipped.

Every `.slt` archive must also carry the correct `LANGID` in its `[TRANSLATION]`
section for the language folder. The batch translation script normalizes this when writing
candidates and `build_language_packs.ps1` checks it during round-trip validation;
for example, `LANGID,1033` in a Chinese or Czech archive is a broken source SLT.

You can also call the implementation script directly when debugging packaging:

```powershell
pwsh -File .\tools\localization\build_language_packs.ps1 `
  -BuildRoot .\build\out\salamand\Release_x64
```

Use the `-Languages` or `-Modules` filters on the preparation/batch-translation scripts to
narrow down translation work. `build_language_packs.ps1` intentionally rebuilds
from committed `.slt` files and a fresh workspace; do not edit files in the temp
workspace and do not commit generated `.slg` files.

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

Do not commit the `out/localization` or `out/localization-openai` directories, `.atp` files, generated `.slg` files, or logs.

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
- **`32-bit IDs are not supported`**: the module contains a dialog control ID that the current Translator cannot represent. Rebuild `utils/translator.exe` from the current sources so quiet operations fail without opening the GUI; the affected module must be fixed or excluded with `-Modules`.
- **A quiet Translator operation opens the GUI**: rebuild `utils/translator.exe` from the current sources. As an additional safeguard, localization scripts terminate quiet operations that do not exit within two minutes.
- **`No candidate file found` in ImportOnly mode**: the `out/localization-openai/candidate/<language>/<module>/` directory does not contain a `.slt` file. Run a full DryRun first to generate candidates, or check the path and language/module names.
- **`Workspace does not exist` in ImportOnly mode**: the `out/localization-openai` directory does not exist or was deleted. Run a DryRun first to create the workspace and generate candidates.
- **`Error updating resource file ... (1359) An internal error occurred` during language-pack build**: rebuild `utils/translator.exe` from current sources. The language-pack round-trip verifier uses `-quiet-export-slt-for-diff`, which exports text without marking the project dirty or saving the `.slg`; older Translator builds used the normal export path and could try to rewrite every generated `.slg` during verification.

## Batch Translation with OpenRouter

`localize_all_openai.ps1` can prepare and translate every available language and module without opening Translator. It discovers languages under `translations/` and discovers `salamand` plus plugins with `lang/english.slg` in the populated build. The default provider is OpenRouter with model `openai/gpt-5.4-nano`. Cursor remains available with `-Provider cursor`, and OpenAI with `-Provider openai`.

Create a user API key at [OpenRouter → Keys](https://openrouter.ai/keys). Set the key only in the process environment; never save it in the repository or a script:

```powershell
$env:OPENROUTER_API_KEY = "..."
$env:OPENROUTER_MODEL = "openai/gpt-5.4-nano" # optional
pwsh -File .\tools\localization\localize_all_openai.ps1 `
  -BuildRoot .\build\out\salamand\Release_x64 -DryRun
```

To use the optional Cursor provider, install `cursor-sdk` (`pip install cursor-sdk`) or the [Cursor CLI](https://cursor.com/docs/cli/overview), set `CURSOR_API_KEY`, and pass `-Provider cursor`.

Remove `-DryRun` after reviewing the per-language/module report and the generated candidates. In the batch script, `-DryRun` still calls the translation API and writes translated files under `out/localization-openai/candidate/`; it only skips copying to `translations/`, Translator import/export validation, and language-pack building. Limit a run with `-Languages czech,slovak` or `-Modules salamand,automation`; use `-BuildLanguagePacks` to build packs only after every translation and validation succeeds. `-ForceRetranslate` also replaces entries already marked as translated and should be used with particular care.

The translator sends examples of existing translations from the same module with each batch as translation memory so the model keeps terminology consistent. `-AutoTrimTranslations` does not retranslate everything; it selects already translated entries that are longer than the current English source and asks the model for a shorter variant with the same technical tokens. The safest first pass is with `-DryRun`, for example:

```powershell
pwsh -File .\tools\localization\localize_all_openai.ps1 `
  -BuildRoot .\build\out\salamand\Release_x64 -Languages czech -Modules salamand `
  -AutoTrimTranslations -DryRun
```

#### ImportOnly Mode

`-ImportOnly` skips skeleton export, rebase, API translation, and workspace preparation (which would delete existing candidates). It imports existing candidate files from `out/localization-openai/candidate/` into the `.slg` projects and optionally exports the final `.slt` files to `translations/`. No API key is required.

The workspace must already exist from a previous DryRun or full run. Use this when you want to manually edit candidates before finalizing them, or when you need to re-import previously translated candidates without re-running the entire pipeline:

```powershell
# DryRun to generate candidates, then review/edit them manually
pwsh -File .\tools\localization\localize_all_openai.ps1 `
  -BuildRoot .\build\salamander\Release_x64 -Languages french -Modules salamand -DryRun

# After reviewing candidates in out/localization-openai/candidate/french/salamand/
pwsh -File .\tools\localization\localize_all_openai.ps1 `
  -BuildRoot .\build\salamander\Release_x64 -Languages french -Modules salamand -ImportOnly
```

The `-ImportOnly` mode can be combined with `-BuildLanguagePacks` to build language packs after importing.

### What the Batch Translation Workflow Produces

The workflow writes its temporary and diagnostic files under `out/localization-openai/`:

- `skeleton/<language>/<module>/<module>.slt` is the current English skeleton exported from the populated build.
- `candidate/<language>/<module>/<module>.slt` is the rebased archive after legacy translations have been merged onto that skeleton and the API has translated `state=0` entries.
- `localize.log` contains Translator quiet-mode command diagnostics.
- `openai-requests.jsonl` contains one JSON object per API request/response with language, item count, and item IDs. It never contains the API key.

When not running with `-DryRun`, each successfully translated candidate is also copied to `translations/<language>/<module>.slt` before the final Translator import/export validation. This is intentional: if a later module fails, already produced translations are not lost. Review these repository files before committing.

### Rebase Rules Before API Translation

Before any API call, `rebase_text_archive.ps1` merges the existing translation archive onto the current skeleton. The result determines which strings are sent to the model:

- Existing translated strings are preserved by resource ID when possible.
- `STRINGTABLE` entries are matched globally by the numeric string ID in the first column, regardless of the `[STRINGTABLE n]` block that currently contains them. Section number and row order are never used as the identity for stringtable text.
- When reusing an existing stringtable translation, the rebase verifies technical tokens such as placeholders, escapes, tags, and accelerator count. If they do not match, the current English text is left as `state=0` for translation/review instead of blindly reusing a risky translation.
- Dialog and menu section IDs can still use a guarded fallback: if a dialog/menu section ID changed but the number of sections of that type did not change, the rebase can fall back to matching sections by type and order.
- If dialog/menu item IDs changed inside a matched section and the item count is unchanged, the rebase can fall back to matching items by order. This fallback is not used for `STRINGTABLE`.
- New sections or items that cannot be matched safely keep the English text but are explicitly marked `state=0`; the translation step must translate them.
- If an entire module has no legacy archive yet, the script forces translation of the current skeleton instead of treating the English skeleton as already translated.

This means candidate files should not silently keep newly added English strings as `state=1`. If you see English text in a candidate, check its state: `state=0` means it is queued for translation or was rejected by validation; `state=1` means it was accepted as translated and needs investigation if it is still English.

### Validation and Retries

The script sends only untranslated resource texts and their IDs to the model, so API usage has a cost. Returned translations are accepted only if the response contains the expected IDs and preserves technical tokens such as placeholders, escapes, tags, paths, and accelerator count. If a batch fails validation, it is split into smaller batches; a single failing item is retried once with stricter preservation instructions. If the retry still changes technical tokens, only that item remains untranslated and the run continues.

Automated translation does **not** replace human review: check terminology, accelerators, placeholders, and whether text fits in dialogs before committing the generated `.slt` files.

### Troubleshooting Batch Translation Runs

- **Candidate files still contain English text marked `state=1`**: this indicates a bad rebase match or a model response that returned English as if it were translated. Check `out/localization-openai/openai-requests.jsonl`, rerun the affected module with `-ForceRetranslate`, and review the diff.
- **Candidate files contain English text marked `state=0`**: the text is still untranslated. Check the script report for `Failed`, and search stderr/logs for `translation skipped` or `technical tokens changed`.
- **A run ends with failed jobs**: successful candidates are still copied to `translations/` unless `-DryRun` was used. Fix the failed module, then rerun with `-Languages`/`-Modules` limited to the affected subset.
- **Translator opens a window during a quiet operation**: rebuild `utils/translator.exe` from current sources and inspect `out/localization-openai/localize.log`. The wrapper terminates unexpected interactive windows instead of waiting forever.
