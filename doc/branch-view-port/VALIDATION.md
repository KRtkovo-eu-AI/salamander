# Validace portu a baseline S00

## Aktuální S00b — evidence/cleanup, 2026-10-12

**S00b oprava PASS v provedeném rozsahu; baseline BLOCKED kvůli prvnímu startu
s prázdným private configem.** S01 nezačala. Testovaný parent
`0772e7d228a6fc5d587c8def3418f744a9ebb9a1` plus finální runtime patch SHA256
`EB4E10DC6D1448AB23D66252E2DF9C6F23C807B111D7246EA33BAD53F5A35017`;
patch, přesné příkazy, logy, JUnit a manifesty jsou v
`C:/Samandarin/codex_pr769_prompts/evidence/S00b`. Dokumentační dokončení nemění
testovaný runtime diff. Tento fix je nový doložený baseline adaptační kód,
nikoli převzetí donoru. Devět REPLACED řádků S00b v REUSE je odlišných od
původních 393 DEFERRED donorových řádků.

Finální příkazy jsou stejné MSBuild host a native/pytest příkazy jako S00a,
se stejným OPENSAL_BUILD_DIR a s výsledky v `evidence/S00b` (`*-final.xml`).
Použitý `Invoke-Bounded.ps1` navíc zaznamenává runtime patch hash. V PATH je
venv/MSBuild; pro postbuild Windows PowerShell je child `PSModulePath` omezen
na WindowsPowerShell Modules, aby se neimportovaly nekompatibilní PS7 moduly.
První Debug kompilace/link prošly, staging selhal na chybějícím Get-FileHash
kvůli module environment; záznam není PASS. Scoped child environment a nový
úspěšný staging problém odstranily, bez změny produkčních skriptů či globálního
prostředí. Žádný timeout; procesní limit 95/100 s, native subprocess limit 60 s.

| Finální kontrola | Výsledek |
| --- | --- |
| MSBuild Debug x64 host/resources | PASS exit 0, 10,45 s; `host-Debug-x64-accepted.*` |
| MSBuild Release x64 host/resources | PASS exit 0, 67,59 s; `host-Release-x64-accepted.*` |
| Native Debug x64 | PASS 37 skupin, 0 failures, 52,53 s |
| Native Release x64 | PASS 37 skupin, 0 failures, 60,78 s |
| CI pytest výběr | PASS 49 testů, 6,48 s |
| PHP process-runtime subcase | SKIPPED, php.exe nedostupné |
| Native conversion table a panel icon size | PASS v obou native sadách |
| Debug/Release GUI, initialized config | PASS panely, F3 ASCII, F5 copy, kontrola hashů a shutdown |
| Debug exit tracking | PASS finální proces ukončen normálně bez „not found“ či leak dialogu |
| První empty private config | Omezení trvá v opakování před finálními shutdown opravami; S00c znovu ověří finální revizi |

Runtime zůstává výhradně v tomto worktree a konfiguraci. Nezměněné
salmon/salopen/salspawn helpers jsou vlastní S00a buildy stejných target zdrojů,
včetně obnovených Debug utils po Release restagingu; žádná donor/installed DLL.
Host znovu ověřil 357 Inno payload souborů (0 změn), 0 vcpkg DLL payloadů.
Finální manifesty 469 Debug / 485 Release souborů obsahují hash a source+patch
vazbu; conversion data bajtově shodná s targetem. Code-growth hook zaznamenal
proti tagu 0.15.1 +92/-36 net +56, z toho samotný runtime fix +55/-26 net +29.
Signing přeskočen původním hookem; plná publikační matice/CodeQL NOT RUN.

Computer Use ověřilo úplné exe cesty a obsah vieweru. Finální Debug PID 7072,
Release PID 5880; nové prázdné copy cíle v původním vlastním GUID fixture rootu,
oba výsledky shodné s ASCII zdrojem. Všech 7 originálů včetně Unicode/long
fixtures a sentinelu beze změny. Ruční long/Unicode viewer scénáře v tomto
fixu NOT RUN (Unicode viewer na targetu byl ověřen S00a); path behavior se nemění.
`validation-summary.json`, `gui-*-accepted-launch.json`, runtime manifesty.

Meziverze Debug po první opravě odhalila další startup DestroyIcon diagnostiku
`icncache.cpp:1183`. Po initialized panel/viewer/copy meziverze hlásila 9
zbylých tracker položek. Read-only diagnostika vlastního procesu podle vlastního
PDB identifikovala dvě owned window ikony, šest štětců z gui.cpp a duplicitní
copy file handle (`debug-shutdown-handles.json`). Tyto konkrétní páry jsou
opraveny; tracker nebyl vypnut. Trace Server byl sestaven Win32 (x64 kombinace
nepodporována), neexponoval UI, vlastní pomocný proces ukončen; nebyl náhradou
diagnostiky. Finální shutdown bez dialogu ověřuje odstranění všech devíti.

Piny SOURCE a smlouva nezměněny. Donorové hunks stále DEFERRED, samostatné
baseline opravy nevydávat za S01 či S02. Bez push/PR. Další nejmenší krok je
[S00c](steps/S00c.md), diagnostika prvního private config startu.

## Historická S00a — obnova prostředí, 2026-10-12

**Baseline: BLOCKED kvůli Debug GUI startup diagnostikám.** v145 a testové
dependencies již nejsou blokací. S00a je pouze dokumentační checkpoint;
žádný runtime kód ani donorový test nebyl přenesen. Testovaný checkout
`75950e887e34ec4bdbe1eb71aa6c04b0c3bfaa8d` obsahuje S00 docs; runtime zdroje
jsou beze změny proti TARGET_SHA `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449`.
Piny SOURCE a všech 393 DEFERRED řádků REUSE (231 donorových souborů) nezměněny.

### Prostředí a reprodukce

- Uživatelem autorizovaná instalace: VS 2026 Community 18.10.3,
  installationVersion 18.10.12224.181, MSBuild 18.10.1.42706,
  MSVC 14.51.36231 / v145, SDK 10.0.26100.0; bez požadavku na restart.
  Instalační/probe evidence je v `evidence/toolchain-install`.
- `git submodule update --init --recursive -- src/plugins/sftp 3rd-party/HardView`
  PASS; SFTP `a293b5beaecef97b56974d4d119a7adc26adb2d4`, HardView
  `b7e68cf032ad2a7dd556775da67ac8424896372f`. Žádný update jejich refs.
- Izolovaný `evidence/baseline-v145/venv`: pytest 9.1.1, lizard 1.23.0,
  přesné dependencies v `python-freeze.txt`. Globální Python/Git beze změny.
- CWD příkazů níže: `C:/Samandarin/codex_pr769_prompts/work`.
  PATH child procesů obsahovala venv Scripts a VS18 MSBuild před inherited PATH.
  `PYTHONDONTWRITEBYTECODE=1`, Python `-B`, MSBuild `/nr:false` a
  `MSBUILDDISABLENODEREUSE=1`. Žádná cache v pracovním stromu nalezena.
- Externí `evidence/baseline-v145/Invoke-Bounded.ps1` zaznamenává executable,
  argumenty, CWD, environment, SHA, exit, čas a logy. Celý procesní strom má
  limit 95 s (native runner 100 s), při timeoutu Kill(true); timeout nenastal.
  Vnitřní native subprocess limit 60 s. Všechny příkazy skončily pod 120 s.

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' src\vcxproj\salamand.vcxproj /m /t:Build /p:Configuration=Debug /p:Platform=x64 /p:OPENSAL_BUILD_DIR=C:\Samandarin\codex_pr769_prompts\work\build\pr769-baseline-v145\ /p:PreferredToolArchitecture=x64 /nr:false /v:minimal
# Stejný příkaz s Configuration=Release.
& ..\evidence\baseline-v145\venv\Scripts\python.exe -B -m pytest src/plugins/automation/tests/salamatrix_ai_local_tests.py tools/code_metrics/tests tools/codeql/tests tools/localization/tests --junitxml=../evidence/baseline-v145/python.xml -p no:cacheprovider
& .\tools\run_native_tests.ps1 -Configuration Debug -Platform x64 -ResultsPath ..\evidence\baseline-v145\native-debug.xml -CommandTimeoutSeconds 60
& .\tools\run_native_tests.ps1 -Configuration Release -Platform x64 -ResultsPath ..\evidence\baseline-v145\native-release.xml -CommandTimeoutSeconds 60
```

| Kontrola | Výsledek a skutečný čas celého procesu |
| --- | --- |
| Host + ProjectReference `src/vcxproj/lang.vcxproj`, Debug x64 | PASS exit 0, 26,60 s |
| Host + language resources, Release x64 | PASS exit 0, 55,63 s |
| CI pytest výběr | PASS 49 testů, exit 0, 6,63 s |
| Native Debug x64 | PASS 37 skupin, 0 failures, exit 0, 94,12 s |
| Native Release x64 | PASS 37 skupin, 0 failures, exit 0, 75,50 s |
| Native conversion-table | PASS obě konfigurace, skutečná shipped data |
| PHP process-runtime subcase | SKIPPED: php.exe nenalezen; skupinový JUnit skipped=0 toto nesděluje |
| Release GUI, inicializovaný private config | PASS běžné panely, ASCII/Unicode F3, ASCII F5 copy |
| Release GUI, první prázdný private config | FAIL pozorování vykreslení panelů; příčina neurčena |
| Debug GUI | FAIL při startupu, baseline nepřijata |

Native sada obsahuje 16 native a 21 source-contract/PowerShell skupin v každé
konfiguraci. Čas v JUnit (Debug 20,783 / Release 13,283 s) zahrnuje pouze testy,
nikoli úspěšné kompilace; nepoužívat jej jako dobu celého runneru.

### Runtime a GUI evidence

Host staging skutečně zkopíroval 357 Inno payload souborů (toolbars/doc/convert)
a našel 0 vcpkg DLL payloadů. Není to sestavení všech plug-inů. Release signing
zůstal přeskočen původním hookem. Source-growth hook úspěšně zaznamenal výchozí
+37/-10 (net +27) proti tagu 0.15.1; jde o původní target, nikoli portový růst.

Samostatně byly stejnými MSBuild flags sestaveny projekty
`src/vcxproj/salmon/salmon.vcxproj`, `salopen/salopen.vcxproj` a
`salspawn/salspawn.vcxproj`, Debug i Release x64, vše PASS. `MyEntryPoint`
potřebuje vlastní `utils/salmon.exe`. Release salopen/salspawn postbuild kopíruje
helpers i do Debug runtime; po Release buildech byly Debug salopen/salspawn
obnoveny `/t:Rebuild` (PASS). Nepoužívat souběžné helper buildy jako důkaz
konfigurační izolace. Žádná binárka z donoru ani instalované aplikace nepřimíchána.

Runtime kořen `work/build/pr769-baseline-v145/salamander/{Debug,Release}_x64`.
Manifesty `runtime-Debug-manifest.json` (469 souborů) a
`runtime-Release-manifest.json` (485) obsahují relativní cesty, SHA256 a délky,
vynechávají Intermediate a označují vlastní config jako mutable smoke stav.
Veškeré staged convert soubory se hashově rovnají target datům, všechny .tab
mají 256 bajtů. GUI běželo v novém procesu po stagingu.

Externí `smoke-fixture.json` popisuje vlastní GUID adresář pod izolovaným build
rootem: ASCII, české/CJK názvy a parent, duplicitní basename, UTF-8 376 bajtů
při UTF-16 206 jednotkách, UTF-16 393 jednotek a sentinel. Dlouhé cesty byly
vytvořeny, ručně však neověřeny; donorové scénáře zůstávají DEFERRED.

Každý runtime má vlastní `configstorage.ini` s `StorageType=RegFile` a vlastní
absolutní `RegFilePath`; skutečná uživatelská konfigurace se neimportovala.
Release byl nejprve spuštěn s platným prázdným config.reg: panely zůstaly bílé
i po refresh, změně view a resize. Po normálním zavření, uložení vlastní
konfigurace a novém startu byly panely viditelné. Příčina není prokázána.

Spuštění přes CLI přesné exe (nikoli instalovaného Salamanderu) použilo
zdrojově ověřené argumenty `-l "<own-fixture-root>" -r "<own-fixture-root>\copy"
-t "PR769 BASELINE Release" -language english`. `english.slg` byl chybný
přípravný pokus a není započten do PASS. `sky.launch_app` nezajistil požadovaný
staged proces; jeho případné instalované okno se také nezapočítává.
Skutečný Release PID 7208 a úplná exe cesta byly ověřeny v Computer Use.
F3 `ascii.txt` zobrazil `PR769 baseline ASCII content`; F5 kopie do prázdného
copy adresáře byla potvrzena se správným cílem. F3
`žluťoučký-東京/český-東京.txt` zobrazil `PR769 baseline CJK file content`.
Kontrola SHA256 potvrdila kopii a všech 7 nezměněných původních souborů včetně
sentinelu. Report `gui-smoke.json`, reprodukce `Prepare-Smoke.ps1` a
`Record-Smoke.ps1`; testovací okna Release byla normálně zavřena.

Debug PID 4496, vlastní Debug exe, dostal kopii pouze tohoto vlastního
materializovaného configu. Startup hlásil `DeleteObject: Handle ... not found`
z `src/iconlist.cpp:483`, poté po potvrzení z `:485`, oba
`src/common/handles.cpp:864`. Druhá diagnostika ukončila smoke; jen tento vlastní
proces byl terminován. Debug normální panel, viewer ani copy nejsou PASS.
Úzký diff potvrdil, že `iconlist.cpp` je shodný v targetu a donoru. Oprava není
součástí S00a; další předkrok [S00b](steps/S00b.md). SDK ani tracker se nevypínaly.

Plná publikační build matice, všechny plug-iny/dependencies a CodeQL NOT RUN;
toolchain probes Win32/ARM64 nejsou plná matice. Žádné vzdálené CI, push či PR.
Nová úplná evidence je mimo Git v
`C:/Samandarin/codex_pr769_prompts/evidence/baseline-v145`.

## Historický S00 před instalací — následující původní záznam

**Tehdejší baseline: BLOCKED.** Testovaná revize runtime zdrojů je
`a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449` (TARGET_SHA), 2026-10-12.
S00 přidává pouze dokumentaci. Žádný `salamand.exe` ani úplný staging z targetu
nevznikl; funkčnost aplikace ani GUI smoke se neprohlašují za ověřené.

## Prostředí a limity

- Windows `10.0.26200`, PowerShell Core `7.6.5`, Python `3.12.10`.
- `vswhere -all` našel pouze VS 2022 BuildTools `17.14.37614.0`, MSVC
  `14.44.35207` (v143). Target projekty i build.cmd vyžadují **v145 / VS 2026**.
- SDK include `10.0.26100.0` je dostupné. `msbuild`/`cl` nejsou v počáteční PATH.
  Jen pro child procesu native runneru byla PATH rozšířena o existující MSBuild.
- Chybí Python balíčky `pytest` a `lizard`. Nic nebylo instalováno ani retargetováno.
- Pinned SFTP submodule `a293b5beaecef97b56974d4d119a7adc26adb2d4` a HardView
  `b7e68cf032ad2a7dd556775da67ac8424896372f` nejsou inicializovány.
- Platí živé sandbox permissions; `.codex/config.toml` s danger-full-access
  je nepřepisuje. `.codex/environments` nemají setup script.
- AGENTS vyžaduje eskalaci každého příkazu a ukončení včetně child procesů
  před 120 s. Použitý externí `evidence/baseline/Invoke-Bounded.ps1` měl
  90s limit celého procesního stromu a `Kill(true)` při timeoutu. Žádný timeout
  nenastal. Native runner měl navíc 8s limit jednotlivého subprocessu.
- `PYTHONDONTWRITEBYTECODE=1`, Python `-B`, MSBuild node reuse vypnuto pro běhy.
  Žádná globální konfigurace se nezměnila. Při delším buildu rozdělit práci na
  povolené konkrétní projekty či použít zvlášť autorizované CI; limit neobcházet.

## Skutečně spuštěné příkazy

CWD všech níže uvedených build/test příkazů:
`C:/Samandarin/codex_pr769_prompts/work`. Jsou to obsahové příkazy uložené
v bounded child scriptech; každý skončil `exit $LASTEXITCODE`. Logy, JSON
s příkazem/SHA/časem a JUnit jsou mimo Git v
`C:/Samandarin/codex_pr769_prompts/evidence/baseline`.

```powershell
& .\build.ps1 -Configuration Debug -Platform x64 -BuildDir 'C:\Samandarin\codex_pr769_prompts\evidence\baseline\target-build'
& .\build.ps1 -Configuration Release -Platform x64 -BuildDir 'C:\Samandarin\codex_pr769_prompts\evidence\baseline\target-build'
```

Oba běhy: **BLOCKED**, hláška o chybějícím v145; nic se nekompilovalo.
Exit kód je v obou případech 0, což **není PASS**. Jde o zjištění ve výchozím
build wrapperu; S00 jej neopravuje. Evidence `build-debug-x64.*` a
`build-release-x64.*` (1,04 s / 1,02 s).

```powershell
$env:PATH = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin;' + $env:PATH
& .\tools\run_native_tests.ps1 -Configuration Debug -Platform x64 -ResultsPath '..\evidence\baseline\native-debug-x64.xml' -CommandTimeoutSeconds 8
```

Exit 1, 15,02 s. **37 skupin: 20 PASS, 17 FAIL**. Šestnáct nativních buildů
selhalo MSB8020 kvůli v145, jejich nativní programy nebyly spuštěny.
`ui_font_contract_tests` selhal FileNotFoundError na
`src/plugins/sftp/src/fs1.cpp` z neinicializovaného submodulu. Zbylých 20
source-contract/PowerShell smoke skupin prošlo; ty nejsou GUI smoke ani náhrada
nativních testů. Přesné názvy: `native-summary.json`, výsledky `native-debug-x64.xml`.

```powershell
python -B -m pytest src/plugins/automation/tests/salamatrix_ai_local_tests.py tools/code_metrics/tests tools/codeql/tests tools/localization/tests --junitxml=../evidence/baseline/python.xml -p no:cacheprovider
```

Exit 1, **BLOCKED**, `No module named pytest`; `python.xml` nevznikl.
Evidence `ci-python.*`. Chybí i `lizard`, potřebný pro část této sady/metriky.

```powershell
python -B -m unittest src/plugins/automation/tests/salamatrix_ai_local_tests.py tools/localization/tests/test_rebase_text_archive.py tools/localization/tests/test_serviceexplorer_resources.py tools/localization/tests/test_translate_slt_with_openai.py
```

Exit 0, **41 testů PASS**, 1,29 s včetně startu procesu. Evidence
`stdlib-python-subset.*`. Jde pouze o existující unittest kompatibilní subset;
pytest metrics/CodeQL testy tím nebyly provedeny.

## Matice pro další etapy

| Kontrola | Skutečný vstup/požadavek | Stav S00 |
| --- | --- | --- |
| Host + změněné části + transitivní dependencies | Debug i Release x64, v145, relevantní resources | BLOCKED v145 |
| Sdílené SDK/hlavičky | Všichni aktuálně podporovaní konzumenti + vydané ABI prefixy | Neprovedeno; žádná taková změna v S00 |
| Nativní sada | `tools/run_native_tests.ps1 -Configuration Debug -Platform x64 -ResultsPath ...` a Release podle smlouvy | Debug buildy blokované; Release neprovedeno |
| CI platformy | PR Build Debug Win32/x64/ARM64; `/t:Rebuild`, `/warnaserror`, `/m`, `/nr:false`, PreferredToolArchitecture=x64 | Neprovedeno |
| Čistá sestava pro publikaci | Celá podporovaná konfigurace; přesné SHA review větve | Neprovedeno, není publikační zadání |
| CodeQL | Po úspěšném PR Build, Release x64 host, `security-extended`, přesné head SHA | Neprovedeno |
| Source growth | Aktuální base a canonical release tagy, `lizard==1.23.0` | Neprovedeno; lizard chybí |
| Native conversion table | Skutečné shipped binární convert.cfg/.tab | Build blokovaný |
| Normal-panel a změněná operace | Skutečný staged runtime, GUI evidence | BLOCKED; žádný target runtime ani native UI ovládání |
| Donorové I/O fixtures | [FIXTURES.md](FIXTURES.md), současně s portovaným kódem | DEFERRED do příslušných etap |

`build.ps1` má pouze `Configuration=Debug/Release`, `Platform=Win32/x86/x64`,
`BuildDir`; nemá ARM64, target-list ani clean přepínač. ARM64 a výběr projektů
řeší skutečné MSBuild příkazy/filtr z `pr-msbuild.yml`, ne vymyšlené argumenty.
Native runner podporuje x64, Debug/Release, ResultsPath a CommandTimeoutSeconds
1–119 (default 110); jeho vnitřní timeout nenahrazuje limit celého příkazu.

PR Build nyní vybírá změněné projekty a filtruje nepodporované konfigurace.
SFTP je ve workflow výslovně vypnutý; to nevysvětluje chybějící submodule pro
source-contract testy. CodeQL gate sleduje `src/` C++ mimo `plugins/`; čistě
dokumentační/plug-in-only změna může analýzu přeskočit. Úspěšný workflow se
skipped buildem není provedený build. PR Tests používá Debug x64 a uvedený
pytest výběr; Studio workflow má vlastní paths filter a VSIX/native UI testy.
Při SDK změnách nespoléhat pouze na changed-project selection, pokud nepokrývá
všechny konzumenty požadované smlouvou.

## Staging a důkaz původu runtime

OPENSAL_BUILD_DIR musí mít vlastní absolutní kořen tohoto worktree, s trailing
backslash. Runtime je v `salamander/<Configuration>_<ShortPlatform>`, native
runner používá `work/build/ci-tests` a vlastní `ci-worker-root`. Donor se
nebuildí; žádný output z `C:/Samandarin/branch-view` ani jiného checkoutu se
nesmí přimíchat. Debug/Release mají oddělené podadresáře.

Host `salamand.vcxproj` po úspěšném buildu volá targetové staging skripty:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/stage_inno_payload.ps1 -RepositoryRoot <target-worktree> -DestinationRoot <configuration-runtime>
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/stage_vcpkg_payload_dlls.ps1 -RepositoryRoot <target-worktree> -DestinationRoot <configuration-runtime>
```

To jsou dokumentované vstupy s placeholdery, **v S00 nespouštěné**. Druhý
skript je x64 dependency staging a potřebuje odpovídající dependency payload.
`lang.vcxproj` je host ProjectReference pro `lang/english.slg`; přidat změněné
plug-iny, jejich DLL/resources a správné CRT. Host kopíruje config a Release
metadata/sample skripty. Inno staging kopíruje toolbars/doc/convert bajtově.
Starší `!populate_build_dir.cmd` je interaktivní a obsahuje pevné VS2022/SDK
cesty; nebyl spuštěn a sám není důkazem úplného současného stagingu.

Po úspěchu uložit mimo Git manifest relativních cest, SHA256 všech binárek,
DLL, resources a dat, testované source SHA, build logy i konfiguraci. Ověřit
spuštěnou exe cestu a návaznost na tento build, spustit nový proces po změně
convert (data se cachují). Conversion cfg zachovat v deklarovaných legacy
code pages, .tab přesně 256 bajtů. Teprve pak běžný panel a změněná operace
na vlastním dočasném korpusu. V tomto běhu tato vazba na runtime **neexistuje**.

## Evidence a pokračování

PASS znamená skutečně dokončenou kontrolu konkrétní revize; FAIL rozlišit na
chybu produktu a prostředí; BLOCKED znamená chybějící předpoklad. SKIPPED,
PENDING a NOT RUN nesmějí být přejmenovány na PASS. Evidence JSON uchovává
SHA, CWD, přesný příkaz, exit, timeout, čas a log. Při dirty kódové změně
navíc uložit hash diffu; samotné HEAD SHA nestačí. Plné logy nepatří do Gitu.

S00 staticky kontroluje manifest, 231/231 donorových souborů v REUSE,
32 stage dokumentů, lokální odkazy, staged scope a `git diff --check`.
Tyto dokumentační kontroly nenahrazují výše blokované runtime ověření.
Git identita pro jediný commit byla uživatelem potvrzena jako
`majklvi <7870647+majklvi@users.noreply.github.com>`; použije se jednorázově
bez změny globálního nastavení.

Před S01 nejprve zajistit autorizované v145 prostředí, úplné pinned submodules
a testové závislosti, zopakovat blokované kontroly a získat skutečný runtime
smoke. Dostupné API computer-use nyní umožňuje browser, native UI je vypnuté.
Bez potřebného ověření zůstává přechod na S01 blokován. Jakýkoli kandidátní
commit/CI kvůli ověření vyžaduje příslušné publikační zadání a přesné SHA.
