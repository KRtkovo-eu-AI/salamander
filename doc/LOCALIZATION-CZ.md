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

Po kontrole reportu pro jednotlivé jazyky/moduly a vygenerovaných kandidátů odstraňte `-DryRun`. V dávkovém skriptu `-DryRun` stále volá OpenAI a zapisuje přeložené soubory do `out/localization-openai/candidate/`; pouze přeskočí kopírování do `translations/`, import/export validaci v Translatoru a sestavení language packů. Běh lze omezit pomocí `-Languages czech,slovak` nebo `-Modules salamand,automation`; `-BuildLanguagePacks` sestaví balíčky pouze tehdy, když všechny překlady a validace uspějí. `-ForceRetranslate` nahradí také položky již označené jako přeložené, proto jej používejte obzvlášť opatrně.

### Co OpenAI workflow vytváří

OpenAI workflow zapisuje dočasné a diagnostické soubory do `out/localization-openai/`:

- `skeleton/<language>/<module>/<module>.slt` je aktuální anglická kostra exportovaná z populovaného buildu.
- `candidate/<language>/<module>/<module>.slt` je rebased archiv po sloučení starého překladu na aktuální kostru a po překladu položek se `state=0` přes OpenAI.
- `localize.log` obsahuje diagnostiku quiet příkazů Translatoru.
- `openai-requests.jsonl` obsahuje jeden JSON objekt pro každý OpenAI request/response s jazykem, počtem položek a ID položek. API klíč se do něj nikdy nezapisuje.

Pokud neběžíte s `-DryRun`, každý úspěšně přeložený kandidát se před finální import/export validací v Translatoru zkopíruje také do `translations/<language>/<module>.slt`. Je to záměr: když později selže jiný modul, už hotové překlady se neztratí. Tyto soubory v repozitáři před commitem vždy zkontrolujte.

### Pravidla rebase před OpenAI překladem

Před jakýmkoliv API voláním `rebase_text_archive.ps1` sloučí existující překladový archiv na aktuální skeleton. Výsledek určuje, které stringy se pošlou do OpenAI:

- Existující překlady se zachovávají podle resource ID, pokud je to možné.
- Položky `STRINGTABLE` se párují globálně podle numerického string ID v prvním sloupci bez ohledu na to, ve kterém bloku `[STRINGTABLE n]` se právě nacházejí. Číslo sekce ani pořadí řádku se u stringtable nikdy nepoužívá jako identita textu.
- Při převzetí existujícího stringtable překladu rebase kontroluje technické tokeny jako placeholdery, escape sekvence, tagy a počet akcelerátorů. Pokud nesedí, ponechá aktuální anglický text jako `state=0` pro OpenAI/review místo slepého převzetí rizikového překladu.
- Dialogy a menu mohou dál použít hlídaný fallback: pokud se změnilo ID sekce dialogu/menu, ale počet sekcí daného typu se nezměnil, rebase umí fallback podle typu a pořadí sekce.
- Pokud se v nalezené dialog/menu sekci změnila ID položek a počet položek je stejný, rebase umí fallback podle pořadí položek. Tento fallback se nepoužívá pro `STRINGTABLE`.
- Nové sekce nebo položky, které nejde bezpečně spárovat, ponechají anglický text, ale explicitně dostanou `state=0`; OpenAI krok je tedy musí přeložit.
- Pokud pro celý modul zatím neexistuje legacy archiv, skript vynutí překlad aktuální kostry místo toho, aby anglickou kostru považoval za přeloženou.

Candidate soubory by tedy neměly tiše obsahovat nově přidané anglické stringy se `state=1`. Pokud v kandidátovi vidíte angličtinu, zkontrolujte její stav: `state=0` znamená, že text je připravený k překladu nebo byl odmítnut validací; `state=1` znamená, že byl přijat jako překlad a pokud je pořád anglicky, je potřeba to vyšetřit.

### Validace a retry OpenAI odpovědí

Skript posílá OpenAI pouze nepřeložené resource texty a jejich ID, takže použití API něco stojí. Vrácené překlady se přijmou jen tehdy, když odpověď obsahuje očekávaná ID a zachová technické tokeny jako placeholdery, escape sekvence, tagy, cesty a počet akcelerátorů. Když dávka neprojde validací, rozdělí se na menší dávky; samostatná problematická položka se jednou zkusí přeložit znovu s přísnější instrukcí pro zachování technických tokenů. Pokud retry pořád mění technické tokeny, zůstane nepřeložená jen tato položka a běh pokračuje dál.

Automatický překlad **nenahrazuje lidskou kontrolu**: před commitem vygenerovaných `.slt` zkontrolujte terminologii, akcelerátory, placeholdery a zda se text vejde do dialogů.

### Řešení problémů při OpenAI běhu

- **Candidate soubory pořád obsahují anglický text se `state=1`**: jde o špatný rebase match nebo model vrátil angličtinu jako překlad. Zkontrolujte `out/localization-openai/openai-requests.jsonl`, spusťte dotčený modul znovu s `-ForceRetranslate` a projděte diff.
- **Candidate soubory obsahují anglický text se `state=0`**: text je stále nepřeložený. Zkontrolujte ve výstupním reportu sloupec `Failed` a ve stderr/logu hledejte `translation skipped` nebo `technical tokens changed`.
- **Běh skončí se selhanými joby**: úspěšné kandidáty se i tak zkopírují do `translations/`, pokud nebyl použit `-DryRun`. Opravte selhaný modul a pak spusťte znovu jen dotčenou část pomocí `-Languages`/`-Modules`.
- **Translator během quiet operace otevře okno**: znovu sestavte `utils/translator.exe` z aktuálních zdrojů a zkontrolujte `out/localization-openai/localize.log`. Wrapper neočekávaná interaktivní okna ukončuje, aby skript nečekal donekonečna.
