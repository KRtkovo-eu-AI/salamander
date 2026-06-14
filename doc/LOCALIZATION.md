# Lokalizační runbook pro Samandarin

Samandarin používá původní formáty Open Salamanderu: sestavené jazykové DLL mají příponu `.slg`, zatímco verzované textové archivy pro překladatele mají příponu `.slt`. Tento postup je odvozený z workflow forku Sally, ale počítá i s našimi novými pluginy a změnami hlavního okna.

## Co patří do překladu

- Jádro aplikace je modul `salamand`; jeho anglický zdroj vzniká ze `src/salamand.rc`, `src/salamand.rc2` a navázaných `.rh2` souborů.
- Plugin je lokalizovatelný, pokud má `src/plugins/<plugin>/lang/lang.rc`. Generátor jej objeví z **populovaného buildu**, takže zahrne i nově přidané pluginy, pokud build obsahuje `plugins/<plugin>/lang/english.slg`.
- Překladové archivy se verzují jako `translations/<language>/<module>.slt`.
- Negenerované workspace, `.atp`, `.slg` a logy se necommitují.

Text viditelný uživateli nesmí být natvrdo v C/C++/C# kódu. Přidejte mu stabilní resource ID do příslušného `.rh2`, anglickou hodnotu do resource souboru/string table a načítejte jej přes stávající lokalizační API (`LoadStr`, `LoadString` apod.). Dialogy a menu patří přímo do `.rc` zdrojů. Jinak Translator text neuvidí a žádný skript jej nemůže nabídnout překladateli.

## Předpoklady

Automatizace Translatoru běží na Windows v PowerShellu 7 (`pwsh`). Nejdříve sestavte a populujte x64 build včetně `utils/translator.exe`, jádra, pluginů a jejich anglických `.slg`. Příklad cesty používané níže je `build/out/salamand/Release_x64`; upravte ji podle skutečného buildu.

## 1. Audit pokrytí modulů

Přenosný audit rychle ukáže nové pluginy bez archivů a staré archivy modulů, které už nejsou ve zdrojích:

```bash
python tools/localization/audit_translation_coverage.py
```

V CI lze požadovat úplné pokrytí všech aktuálních modulů:

```bash
python tools/localization/audit_translation_coverage.py --fail-on-missing
```

Chybějící archiv je očekávaný u právě přidaného pluginu; musí se vytvořit exportem aktuální anglické kostry a následně přeložit. Audit sám nekontroluje hardcoded UI texty, proto je při code review nutné ověřit, že každý nový uživatelský text má resource ID.

## 2. Vygenerování pracovního prostoru

```powershell
pwsh -File .\tools\localization\prepare_translation_workspace.ps1 `
  -BuildRoot .\build\out\salamand\Release_x64 `
  -OutputDir .\out\translation-workspace `
  -Languages czech,slovak `
  -Force
```

Skript zkopíruje runtime, objeví jádro i všechny pluginy s `english.slg`, vytvoří `projects/<language>/<module>/<module>.atp`, seedne cílové `.slg` a zkopíruje existující `.slt`. Pro aktualizaci existujících překladů přidejte `-ImportArchives`; lze také omezit `-Modules salamand,samandarin,jsonviewer`.

```powershell
pwsh -File .\tools\localization\prepare_translation_workspace.ps1 `
  -BuildRoot .\build\out\salamand\Release_x64 `
  -OutputDir .\out\translation-workspace `
  -Languages czech `
  -Modules salamand,samandarin,jsonviewer `
  -ImportArchives -Force
```

Workspace používá lokální absolutní cesty v `.atp`, CRLF a symbolové soubory bez BOM; generátor tyto požadavky řeší automaticky.

## 3. Zachycení nových textů a rebase starých překladů

Aktuální anglická `.slg` sestavená z aktuálních resources je zdroj pravdy. Po vygenerování workspace exportujte diff-friendly aktuální kostru:

```powershell
.\out\translation-workspace\runtime\utils\translator.exe `
  -quiet-export-slt-for-diff .\out\slt-current `
  .\out\translation-workspace\projects\czech\salamand\salamand.atp
```

Pokud starý archiv po změně dialogů, menu nebo ID nejde čistě importovat, přeneste překlady na aktuální kostru podle shodných ID:

```powershell
pwsh -File .\tools\localization\rebase_text_archive.ps1 `
  -CurrentArchive .\out\slt-current\salamand.slt `
  -LegacyArchive .\translations\czech\salamand.slt `
  -OutputArchive .\out\rebased\czech-salamand.slt
```

Nové položky zůstanou nepřeložené (`state=0`) a překladatel je uvidí. Stejný postup použijte pro každý nový nebo změněný plugin.

## 4. Překlad a export

Projekt otevřete v Translatoru, přeložte položky a proveďte kontroly layoutu, formátovacích parametrů, akcelerátorů a plurálů. Poté exportujte archiv:

```powershell
.\out\translation-workspace\runtime\utils\translator.exe `
  -quiet-export-slt .\out\exports\czech `
  .\out\translation-workspace\projects\czech\salamand\salamand.atp
```

Volitelný `translate_slt.py` umí doplnit položky se `state=0` přes Anthropic API; vyžaduje `ANTHROPIC_API_KEY` a výsledek musí zkontrolovat člověk:

```bash
python tools/localization/translate_slt.py translations/german/samandarin.slt --language german --dry-run
```

Commitujte pouze zkontrolované archivy do `translations/<language>/` spolu s případnými změnami resources a nástrojů.

## 5. Validace a sestavení jazykových balíčků

Pro ověření reprezentativních projektů a headless import/export workflow:

```powershell
pwsh -File .\tools\localization\verify_translation_workspace.ps1 -WorkspaceDir .\out\translation-workspace
```

Ruční smoke test otevření Translator projektů:

```powershell
pwsh -File .\tools\localization\smoke_test_translation_workspace.ps1 -WorkspaceDir .\out\translation-workspace
```

Všechny dostupné archivy lze importovat, validovat a výsledná `.slg` vložit do populovaného runtime takto:

```powershell
pwsh -File .\tools\localization\build_language_packs.ps1 `
  -BuildRoot .\build\out\salamand\Release_x64
```

Skript odmítne nezměněný anglický seed vydávaný za překlad. Pro lokální neúplný build lze použít `-AllowSeedRejections`, ne však pro release.

## Checklist pro každou změnu UI

1. Nový text má stabilní resource ID a není hardcoded v kódu.
2. Nový plugin vytváří `lang/english.slg` a objeví se v auditu i generovaném workspace.
3. Pro každý podporovaný jazyk je aktuální `.slt` rebased/importovaný a nové položky přeložené.
4. Translator validace prošla; byly ručně zkontrolovány dialogy, menu, klávesové zkratky, `%` parametry a plurály.
5. Release build vytvořil neanglická `.slg`; do Gitu se dostaly pouze resources, `.slt`, dokumentace a nástroje.
