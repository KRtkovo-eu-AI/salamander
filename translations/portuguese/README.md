<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-06 | Record completed Brazilian Portuguese SLT pack. | portuguese/*.slt archives are fully translated (pt-BR / LANGID 1046). | | Composer |
-->

# Brazilian Portuguese translation (`pt-BR`)

Brazilian Portuguese UI archives for Open Samandarin. The folder name is
`portuguese`; the locale is Brazilian Portuguese (`pt-BR`) with Windows
`LANGID` 1046.

All module `.slt` files in this directory are translated (`state=1` for
nonempty strings). Common buttons follow Brazilian Portuguese UI conventions
(`OK`, `Cancelar`, `Ajuda`, `Sim`, `Não`) while preserving placeholders,
accelerators, and the product names Open Samandarin / Open Salamander /
Salamatrix. Prefer BR vocabulary (e.g. `arquivo`, not European `ficheiro`).

Rebuild language packs from a populated build output:

```powershell
pwsh -File tools\localization\localize_all_openai.ps1 -Languages portuguese -BuildLanguagePacks
```

Or import the committed `.slt` archives with the Salamander Translator and
produce `portuguese.slg` beside the runtime.
