# Arabic translation seed

This directory deliberately starts without committed `.slt` archives. On the
first run, the batch localization workflow exports current English resources,
marks all text as untranslated, and writes the generated Arabic archives here.
That avoids committing a stale English snapshot as a translation seed.

Run the translation and build the language pack from a populated build output:

```powershell
$env:CURSOR_API_KEY = "..."
pwsh -File tools\localization\localize_all_openai.ps1 -Languages arabic -BuildLanguagePacks
```

The workflow uses Arabic (Saudi Arabia), `ar-SA`, with Windows `LANGID` 1025. Once
the run succeeds, the resulting `.slt` files are committed in this directory
and `arabic.slg` files are placed in the build output by `-BuildLanguagePacks`.
