# Korean translation (`ko-KR`)

Hangul UI archives for Open Samandarin. Locale is Korean (Korea), Windows
`LANGID` 1042.

All module `.slt` files in this directory are translated (`state=1` for
nonempty strings). Common buttons follow Korean UI conventions (`OK`,
`취소`, `도움말`, `예`, `아니요`) while preserving placeholders,
accelerators, and the product names Open Samandarin / Open Salamander /
Salamatrix.

Rebuild language packs from a populated build output:

```powershell
pwsh -File tools\localization\localize_all_openai.ps1 -Languages korean -BuildLanguagePacks
```

Or import the committed `.slt` archives with the Salamander Translator and
produce `korean.slg` beside the runtime.
