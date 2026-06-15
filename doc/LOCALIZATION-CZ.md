# Jak přeložit Samandarin

Pro běžnou práci používejte pouze `tools/localization/localize.ps1`. Ostatní skripty v adresáři jsou jeho implementační detaily.

## Doplní se nové texty ze zdrojových resources?

**Ano.** Příkaz `start` postupuje takto:

1. vezme aktuální `english.slg` z buildu jako úplnou kostru projektu,
2. exportuje z ní aktuální `.slt` kostru,
3. automaticky na ni převede starý `.slt` překlad podle resource ID,
4. převedený archiv importuje a otevře výsledek v Translatoru.

Staré přeložené položky se tedy zachovají a nové stringy, dialogy nebo menu z aktuálních `.rc`/`.rh2` resources zůstanou v Translatoru jako **nepřeložené**. Není potřeba je ručně dopisovat do překladových šablon.

Automatický převod je důležitý: Translator neumí přímo importovat starý `.slt`, pokud se mezitím změnila struktura resources. `localize.ps1 start` proto starý archiv před importem automaticky rebased na současnou kostru. Chyba typu `Syntax error ... salamand.slt on line ...` se při běžném použití nemá zobrazit.

Jediná důležitá podmínka je, že před `start` musíte z aktuálních zdrojů znovu sestavit a populovat build. Skript čte nové resources z výsledné `english.slg`, nikoliv přímo ze zdrojových `.rc` souborů. Pokud použijete starý build, nové texty v něm ještě nebudou.

## Nejkratší postup

Potřebujete Windows, PowerShell 7 (`pwsh`) a hotový x64 build obsahující `salamand.exe`, pluginy, anglické `.slg` a `utils/translator.exe`.

### 1. Otevřete jeden překlad

Například český překlad hlavního okna:

```powershell
pwsh -File .\tools\localization\localize.ps1 start czech salamand `
  -BuildRoot .\build\out\salamand\Release_x64
```

Nebo český překlad pluginu Samandarin:

```powershell
pwsh -File .\tools\localization\localize.ps1 start czech samandarin `
  -BuildRoot .\build\out\salamand\Release_x64
```

Příkaz připraví vše potřebné, načte existující překlad na aktuální anglickou resource kostru a automaticky otevře Translator.

### 2. Co dělat v Translatoru

1. Přeložte nepřeložené položky. Nové texty přidané od posledního překladu zůstávají označené jako nepřeložené.
2. U dialogů zkontrolujte, že se přeložený text vejde do ovládacích prvků.
3. Projekt uložte pomocí **Ctrl+S**.
4. Translator zavřete.

Pokud Translator později potřebujete znovu otevřít bez nové přípravy workspace:

```powershell
pwsh -File .\tools\localization\localize.ps1 open czech salamand
```

### 3. Uložte výsledek do repozitáře

```powershell
pwsh -File .\tools\localization\localize.ps1 finish czech salamand
```

Výsledkem je soubor `translations/czech/salamand.slt`, který zkontrolujete a commitnete. Pro plugin `samandarin` by příkaz i výsledek používaly jméno `samandarin`.

To je celý běžný překladový postup: **start → překlad a Ctrl+S → finish**.

## Užitečné příkazy

Zobrazit pluginy, kterým chybí překladové archivy:

```powershell
pwsh -File .\tools\localization\localize.ps1 check
```

Sestavit všechny dostupné jazykové `.slg` do hotového runtime:

```powershell
pwsh -File .\tools\localization\localize.ps1 build `
  -BuildRoot .\build\out\salamand\Release_x64
```

## Když nový text v Translatoru není

Translator umí překládat pouze texty uložené ve Windows resources. Uživatelský text proto nesmí být natvrdo v C/C++/C# kódu.

- String musí mít stabilní resource ID v příslušném `.rh2`, anglický text ve string table a kód jej musí načítat přes `LoadStr`, `LoadString` nebo odpovídající lokalizační API.
- Dialogy a menu musí být v příslušném `.rc`.
- Nový plugin musí sestavit `plugins/<plugin>/lang/english.slg`.

Po opravě resources znovu sestavte/populujte aplikaci a spusťte `localize.ps1 start ...`. Aktuální anglická `.slg` je zdroj pravdy, takže nový text se pak objeví jako nepřeložený.

## Co commitovat

Commitujte:

- změněné resources a kód,
- výsledné `translations/<language>/<module>.slt`,
- případné změny dokumentace a lokalizačních nástrojů.

Necommitujte adresář `out/localization`, `.atp`, vygenerované `.slg` ani logy.

## Řešení problémů

- **`Build root ... does not look like ...`**: cesta předaná přes `-BuildRoot` neobsahuje populovaný build se `salamand.exe`.
- **Chybí `translator.exe`**: build musí obsahovat `utils/translator.exe`.
- **Unknown module**: plugin v buildu nemá `plugins/<plugin>/lang/english.slg`, případně je špatně napsané jeho jméno.
- **Nové resource texty v Translatoru nejsou**: `-BuildRoot` pravděpodobně ukazuje na starý build; znovu sestavte/populujte aktuální zdroje a spusťte `start` znovu.
- **Import převedeného archivu selže**: příkaz `start` skončí s chybou a odkazem na existující `out/localization/localize.log`; Translator se neotevře s rozbitým projektem.
- **Soubor `<module>.quiet.log` neexistuje**: Translator v tomto repozitáři tento log nevytváří a při úspěšné quiet operaci vrací historický exit kód `1`. Wrapper proto zapisuje vlastní `out/localization/localize.log`.
- **`Syntax error ... on line 1`**: používali jste starší wrapper, který pro rebase exportoval diff archiv bez povinné sekce `[EXPORTINFO]`; aktualizujte repozitář a spusťte `start` znovu.
- **`$LASTEXITCODE cannot be retrieved because it has not been set`**: používali jste starší verzi `localize.ps1`, která spouštěla GUI `translator.exe` přímo; aktualizujte repozitář a spusťte stejný příkaz znovu.
- **Potřebuji pokročilý rebase nebo headless validaci**: použijte pomocné skripty `rebase_text_archive.ps1` a `verify_translation_workspace.ps1`; pro běžný překlad nejsou potřeba.
- **`32-bit IDs are not supported`**: modul obsahuje ID ovládacího prvku dialogu, které současný Translator neumí reprezentovat. Znovu sestavte `utils/translator.exe` z aktuálních zdrojů, aby quiet operace skončila chybou bez otevření GUI; dotčený modul je nutné opravit nebo vynechat pomocí `-Modules`.
- **Quiet operace Translatoru otevře GUI**: znovu sestavte `utils/translator.exe` z aktuálních zdrojů. Lokalizační skripty navíc jako pojistku ukončí quiet operaci, která neskončí do dvou minut.

## Dávkový překlad pomocí OpenAI

`localize_all_openai.ps1` umí bez otevření Translatoru připravit a přeložit všechny dostupné jazyky a moduly. Jazyky zjistí z adresáře `translations/`; z populovaného buildu zjistí hlavní modul `salamand` a pluginy obsahující `lang/english.slg`.

API klíč nastavujte pouze v prostředí procesu; nikdy jej neukládejte do repozitáře ani skriptu:

```powershell
$env:OPENAI_API_KEY = "..."
$env:OPENAI_MODEL = "gpt-5-mini" # volitelné
pwsh -File .\tools\localization\localize_all_openai.ps1 `
  -BuildRoot .\build\out\salamand\Release_x64 -DryRun
```

Po kontrole reportu pro jednotlivé jazyky a moduly odstraňte `-DryRun`. Běh lze omezit pomocí `-Languages czech,slovak` nebo `-Modules salamand,automation`; `-BuildLanguagePacks` sestaví balíčky pouze tehdy, když všechny překlady a validace uspějí. `-ForceRetranslate` nahradí také položky již označené jako přeložené, proto jej používejte obzvlášť opatrně.

Skript odesílá OpenAI pouze nepřeložené resource texty a jejich kontext, takže použití API něco stojí. Ověřuje vrácená ID a technické tokeny, každého kandidáta před nahrazením archivu v repozitáři importuje do Translatoru a API klíč nikdy neloguje. Automatický překlad **nenahrazuje lidskou kontrolu**: před commitem vygenerovaných `.slt` zkontrolujte terminologii, akcelerátory, placeholdery a zda se text vejde do dialogů.
