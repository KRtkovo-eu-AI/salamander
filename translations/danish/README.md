# Danish translation seed

This directory deliberately starts without committed `.slt` archives. On the
first run, the OpenAI localization workflow exports current English resources,
marks all text as untranslated, and writes the generated Danish archives here.
That avoids committing a stale English snapshot as a translation seed.

Run the translation and build the language pack from a populated build output:

```powershell
$env:OPENAI_API_KEY = "..."
pwsh -File tools\localization\localize_all_openai.ps1 -Languages danish -BuildLanguagePacks
```

The workflow uses Danish, `da-DK`, with Windows `LANGID` 1030. Once
the run succeeds, the resulting `.slt` files are committed in this directory
and `danish.slg` files are placed in the build output by `-BuildLanguagePacks`.
