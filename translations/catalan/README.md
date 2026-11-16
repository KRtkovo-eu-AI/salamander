# Catalan translation seed

This directory deliberately starts without committed `.slt` archives. On the
first run, the OpenAI localization workflow exports current English resources,
marks all text as untranslated, and writes the generated Catalan archives here.
That avoids committing a stale English snapshot as a translation seed.

Run the translation and build the language pack from a populated build output:

```powershell
$env:OPENAI_API_KEY = "..."
pwsh -File tools\localization\localize_all_openai.ps1 -Languages catalan -BuildLanguagePacks
```

The workflow uses Catalan, `ca-ES`, with Windows `LANGID` 1027. Once
the run succeeds, the resulting `.slt` files are committed in this directory
and `catalan.slg` files are placed in the build output by `-BuildLanguagePacks`.
