# Polish translation seed

This directory deliberately starts without committed `.slt` archives. On the
first run, the batch localization workflow exports current English resources,
marks all text as untranslated, and writes the generated Polish archives here.
That avoids committing a stale English snapshot as a translation seed.

Run the translation and build the language pack from a populated build output:

```powershell
$env:CURSOR_API_KEY = "..."
pwsh -File tools\localization\localize_all_openai.ps1 -Languages polish -BuildLanguagePacks
```

The workflow uses Polish (Poland), `pl-PL`, with Windows `LANGID` 1045. Once
the run succeeds, the resulting `.slt` files are committed in this directory
and `polish.slg` files are placed in the build output by `-BuildLanguagePacks`.
