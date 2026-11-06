# Japanese translation (`ja-JP`)

Japanese UI archives for Open Samandarin. Locale is Japanese (Japan), Windows
`LANGID` 1041.

All module `.slt` files in this directory are translated (`state=1` for
nonempty strings). Common buttons follow Japanese UI conventions (`OK`,
`キャンセル`, `ヘルプ`, `はい`, `いいえ`) while preserving placeholders,
accelerators, and the product names Open Samandarin / Open Salamander /
Salamatrix.

Rebuild language packs from a populated build output:

```powershell
pwsh -File tools\localization\localize_all_openai.ps1 -Languages japanese -BuildLanguagePacks
```

Or import the committed `.slt` archives with the Salamander Translator and
produce `japanese.slg` beside the runtime.
