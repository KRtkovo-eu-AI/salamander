# Polish translation

Polish (Poland), `pl-PL`, Windows `LANGID` 1045.

The `.slt` archives in this directory were bootstrapped from English
`english.slg` resources and then manually translated to Polish.

Rebuild language packs from a populated build output:

```powershell
pwsh -File tools\localization\build_language_packs.ps1 `
  -BuildRoot .\build\out\salamand\Release_x64 -Languages polish
```
