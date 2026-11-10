<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-10 | Document the FlaUI/UIA3 test scaffold and its limits. | Make native UI automation reproducible while keeping test-profile changes contained. |  | GPT-6 (Codex) |
-->

# FlaUI / UIA3 UI testy

Tato kostra spouští samostatný C# NUnit projekt nad nativním `salamand.exe`. FlaUI.UIA3 přistupuje k oknům přes Windows UI Automation; první test ověřuje pouze nalezení hlavního okna Salamanderu. Projekt je v `tests/ui-automation/` a jeho spuštění zajišťuje `tests/run-ui-automation.ps1`.

## Proč samostatný projekt

Nativní aplikace je Win32 C++, zatímco FlaUI je .NET knihovna. UI testy proto nejsou součástí `.sln` native testů. Samostatný NUnit projekt drží závislosti a runtime odděleně a lze jej spouštět pomocí `dotnet test`.

Inspirací je fork [taskscape/FileManager](https://github.com/taskscape/FileManager), zejména jeho [dokumentace testů](https://github.com/taskscape/FileManager/blob/main/testing.md), [UI test runner](https://github.com/taskscape/FileManager/blob/main/run-ui-tests.ps1) a [UI test projekt](https://github.com/taskscape/FileManager/tree/main/tests/FileManager.UiTests). Jejich postup je: běžný .NET testovací projekt, NUnit kategorie, FlaUI `UIA3Automation`, spuštění skutečného EXE a opakované hledání top-level oken. Testy běží interaktivně pod aktuálním přihlášeným uživatelem, ne jako služba/headless proces. Fork navíc upravil nativní aplikaci tak, aby testy mohly používat oddělený kořen registru a testovací datový adresář.

## Požadavky a bezpečnost profilu

- Windows s přihlášenou interaktivní plochou. Test potřebuje přístup k desktopu; nespouštějte jej jako Windows službu ani v session bez UI.
- .NET SDK 10 a dostupný NuGet feed pro první restore.
- Salamander sestavený z aktuálního checkoutu. Ukazujte přímo na jeho `salamand.exe`, ne na instalovanou kopii.
- Použijte jednorázový Windows uživatelský profil nebo zahoditelnou Windows VM. Testovací běh spouští aplikaci pod aktuálním uživatelem a Salamander ukládá konfiguraci do HKCU. Současná aplikace zde ještě nemá oddělený kořen konfigurace pro UI testy.
- Zavřete všechny instance Salamanderu. Aplikace používá single-instance předání, které by jinak mohlo test přesměrovat do existujícího procesu.

`-ConfirmDisposableProfile` je záměrná pojistka proti náhodnému spuštění v běžném profilu; nenahrazuje skutečně jednorázový účet/VM a nemaže ani neizoluje registry.

## První spuštění

Nejprve sestavte a nasaďte běhové soubory Salamanderu podle [runbooku buildu](runbook-setup/README.md). V jednorázovém profilu spusťte jednou jeho `salamand.exe` ručně a dokončete případné první spuštění, zejména volbu jazyka. Potom aplikaci zavřete.

V PowerShellu z kořene checkoutu spusťte:

```powershell
.\tests\run-ui-automation.ps1 `
  -ExecutablePath 'C:\path\to\salamand.exe' `
  -ConfirmDisposableProfile
```

Runner obnoví NuGet závislosti a spustí UI kategorii. Pro přímé ladění lze použít:

```powershell
$env:SAMANDARIN_UI_EXE = 'C:\path\to\salamand.exe'
$env:SAMANDARIN_UI_DISPOSABLE_PROFILE = 'YES'
dotnet test .\tests\ui-automation\Samandarin.UiTests.csproj --filter 'TestCategory=UI'
```

Přímý příkaz vyžaduje stejné jednorázové prostředí. Po skončení odstraňte proměnné z daného PowerShellu.

## Jak psát testy

- Testy označte `[Category("UI")]`; zatím běží sériově (`[NonParallelizable]`), protože sdílí interaktivní desktop a Salamander je single-instance.
- Test musí sám zjišťovat konkrétní `Window` a jeho controls přes FlaUI/UIA3. Upřednostňujte `AutomationId`/stabilní vlastnosti před lokalizovaným viditelným textem.
- Nativní menu, owner-drawn seznamy a vlastní Salamander ovládací prvky nemusí zveřejňovat potřebné UIA patterns. Nejprve ověřte strom přes Inspect.exe / FlaUInspect; teprve pak zvolte úzký Win32 helper pro konkrétní prvek. Fork taskscape používá FlaUI pro běžné okna a controls a Win32 zprávy jen pro owner-drawn nebo UIA-nepřístupné prvky.
- Každý test si připraví vlastní soubory pod jedinečným adresářem v zahoditelném profilu a uklidí pouze to, co sám vytvořil. Testy nesmí předpokládat nebo měnit obsah běžného profilu.
- Testy, které mění nastavení a ověřují jeho zachování, musí po testu obnovit výchozí hodnotu nebo běžet pouze v jednorázovém profilu.
- Testy interakce s dialogem mají čekat na jeho výskyt s časovým limitem, ověřit výsledek a dialog vždy zavřít. Vyhněte se pevným dlouhým `Sleep`; pollujte stav aplikace/okna.

## Omezení kostry a další krok

Tato první verze pouze prokazuje, že se testovací proces umí připojit k hlavnímu top-level oknu `SalamanderMainWindowVer25`. Ještě neizoluje registr, neovládá menu a dialogy, nepořizuje screenshoty ani nezpracovává crash reporter/modal startup chybová okna. Nový uživatelský profil nejdřív inicializujte ručně.

Pro bezpečné opakované UI testy a CI je potřeba doplnit do aplikace testovací konfiguraci s odděleným HKCU kořenem a datovým/TEMP adresářem, ověřovanou pouze v explicitním isolated režimu. Pak lze přidat společný NUnit fixture: spuštění aplikace, čekání na konkrétní hlavní okno, připojení k dialogům, logování procesu/oken, ukončení pouze vlastních procesů a úklid jen testovacích dat. Bez nativní podpory nelze spolehlivě garantovat, že změny nastavení z testu nezapíšou do normálního profilu.

## Externí reference

- [taskscape/FileManager – automated testing](https://github.com/taskscape/FileManager/blob/main/testing.md)
- [taskscape/FileManager – focused UI runner](https://github.com/taskscape/FileManager/blob/main/run-ui-tests.ps1)
- [FlaUI](https://github.com/FlaUI/FlaUI) a [FlaUI.UIA3 NuGet balíček](https://www.nuget.org/packages/FlaUI.UIA3)
