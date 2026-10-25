# S31 — Úplnost portu proti donoru

S31b3a4a: NTFS helper/worker exact wide identity a failure propagation ověřené
native160/config D/R včetně skutečných EFS/komprese; GUI staged do S31b3a4b.
G04 attributes REMAINS_OPEN, viz [S31b3a4a](steps/S31b3a4a.md).

S31b3a3: základní dialog atributů aktivní s root/tab/generation/selection guards;
GUI exact long apply/Cancel D/R PASS. NTFS compression/encryption staged do
S31b3a4; G04 attributes REMAINS_OPEN, viz [S31b3a3](steps/S31b3a3.md).

S31b3a2: attribute/time worker error handling opravené a native73/config D/R
ověřené. G04 attributes REMAINS_OPEN do modal/selection a GUI S31b3a3.
Viz [S31b3a2](steps/S31b3a2.md); nepovoluje nové příkazy.

S31b3a1: atributy mají donorový exact per-item zdroj v planneru. G04 attributes
REMAINS_OPEN: ChangeAttr/host gate stále blokují, worker/dialog/GUI navazují.
Native399/config a host D/R PASS, viz [S31b3a1](steps/S31b3a1.md).

Aktualizace S31b2: shortcut/link-target část G04 IMPLEMENTED_LOCAL; exact wide
source, COM/PIDL, reparse handle a skutečné povolení CM_AFOCUSSHORTCUT. Native92,
router173/config, host D/R a GUI evidence viz [S31b2](steps/S31b2.md). Neuzavírá
ostatní G04/G05 a S32; historické donor indexy níže zůstávají snapshotem S30.

Aktualizace S31b1a: archive-entry část G04 nyní RESOLVED_LOCAL po opravě
preexistujícího 7zip wide-name úniku. Baseline/current ordinary i Branch měly
stejný 30byte leak; scoped ownership jej odstraňuje bez změny heap filtru či ABI
projekce. Native185/config a instrumentované GUI PASS, viz [S31b1a](steps/S31b1a.md).
Ostatní core G04/G05 a S32 BLOCKED trvají. Starší záznamy níže jsou historické.

Aktualizace S31b1: archive entry část G04 IMPLEMENTED_LOCAL, přesná identita a
callback guards; native267/config, host D/R a funkční GUI D/R PASS. Tři navazující
path buffery opravené; finální unload ověření viz [S31b1](steps/S31b1.md).
G04 zbývající core operace a G05 zůstávají otevřené, goal ACTIVE, S32 BLOCKED.
Historické S30 indexy níže zůstávají snapshotem před opravami.

Aktualizace S31a3b: **G03 RESOLVED_LOCAL** po produkční opravě S31a3a a aktivaci
celého rozsahu v S31a3b. Default/registrované příkazy/discovery mají 20 + 19 PASS,
assertions zachované, rename notifikace aktivní i v All. Viz [S31a3b](steps/S31a3b.md).
G01/G02 jsou již uzavřené; G04/G05 a S32 BLOCKED trvají. Uživatel požaduje core
operace G04 implementovat: následuje S31b1, nikoli pouze dokumentace odmítání.
Historické indexy pod tímto záznamem zůstávají snapshotem S30.

Aktualizace S31a3a: původní označení Shell G03 erroru jako selector-only bylo
nesprávné. Obnoven skutečný donorový post-retry guard a quiet capture; původní
test beze změny assertions nyní PASS, native20/config + D/R host a GUI PASS.
Viz [S31a3a](steps/S31a3a.md). G03 ještě vyžaduje S31a3b (clipboard selector,
All a temporary selection). Uživatel pro S31b výslovně zvolil doplnění donorových
core operací po malých ověřených krocích; dokumentace odmítání je neuzavírá.
G04/G05 a S32 zůstávají otevřené, goal ACTIVE.

Aktualizace S31a2 (2026-10-25): **G02 RESOLVED_LOCAL**, donorový ANSI-to-wide
rename hook, Debug/Release buildy, vynucený native fallback a skutečné ordinary/
Branch GUI PASS, viz [S31a2](steps/S31a2.md). G01 je již uzavřen S31a1.
Historické TSV zůstávají snapshotem S30; G03–G06 a S32 BLOCKED trvají. Goal S31
pokračuje S31a3 a uzavřením support/validation hranic S31b/c. Bez push a remote CI.

Aktualizace S31a1 (2026-10-25): **G01 RESOLVED_LOCAL** — donorové zapojení
wrapperu opraveno, host Debug/Release i native/GUI ověření PASS, viz
[S31a1](steps/S31a1.md). Následující audit a TSV indexy jsou neměnný snapshot S30;
počet DEFERRED v nich není aktuální počet otevřených nálezů. G02–G06 a S32 BLOCKED
zůstávají. Bez push a remote CI tohoto následného checkpointu.

Stav: AUDIT_COMPLETE_WITH_FINDINGS, 2026-10-25. **S32 BLOCKED.** Tento checkpoint
mění pouze dokumentaci. Neopravuje níže nalezené mezery ani nezpřístupňuje další
operace. Branch View zůstává ve standardním procesu vypnuté. Výsledkem auditu je
uzavřený seznam rozhodnutí a opravných podetap, nikoli potvrzení úplné funkční parity.

## Připnuté vstupy a metoda

| Vstup | Přesná revize |
| --- | --- |
| Donor PR | [KRtkovo-eu-AI/salamander#769](https://github.com/KRtkovo-eu-AI/salamander/pull/769) |
| DONOR_SHA | `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413` |
| DONOR_BASE_SHA / TARGET_SHA | `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449` |
| Auditovaný přijatý port S30 | `58aabd60054f98d964f2be7b93bbf408ac692dde` |
| SHA-256 uloženého donor.patch | `7bd21bd8e40b745630a605fa80f96f89bc02da3cce28f1723fa3c22f139158cc` |

SOURCE.json a donorový checkout zůstávají nezměněné. Inventář je odvozen z diffu
DONOR_BASE_SHA..DONOR_SHA a skutečných souborů auditovaného portu. Oba vycházejí
ze stejného target/base commitu. Dodatečné target opravy se automaticky nenahrazují
donorovým textem. Kontrola zahrnuje implementace, volající, hlavičky, resources,
registrace projektů/testů, binární conversion data a explicitní odklady.

Úplné rozhodovací indexy tvoří součást auditu:

- [COVERAGE-FILES.tsv](COVERAGE-FILES.tsv): všech 231 donorových cest, donor/target SHA-256, rozhodnutí a důvod.
- [COVERAGE-HUNKS.tsv](COVERAGE-HUNKS.tsv): všech 892 bloků v pořadí uloženého patch, rozsah a rozhodnutí; ID se vztahuje pouze k tomuto připnutému patch.
- [COVERAGE-EXTRAS.tsv](COVERAGE-EXTRAS.tsv): všech 190 dalších změněných target cest BASE..S30, jejich role a poslední checkpoint. Nové dokumenty S31 v tomto historickém inventáři nejsou.

147 souborů je EXACT po odstranění UTF-8 BOM a normalizaci konců řádků; pouze
6 je současně bajtově shodných. 82 existujících cest je ADAPTED a 2 chybějící
release dokumenty jsou DEFERRED. Na úrovni bloků je 470 EXACT, 416 ADAPTED a
6 DEFERRED. ADAPTED není tvrzení úplné parity: odkazované G01–G06 mají přednost.
Hunk 165 například obsahuje převzaté části i chybějící bezpečné směrování.

Textový screening nalezl 762 donorových postimages v targetu a 130 k ručnímu
prověření. Tento počet není počet ověřených funkcí: stejný text může být v jiné
funkci. Proto jsou všechny bloky adaptovaného souboru konzervativně ADAPTED,
pokud nejsou konkrétně DEFERRED; EXACT bloky pocházejí jen z celých shodných
souborů. Například obsluha IDC_BRANCH_KEEP_PATHS je přítomná v reálném
CCopyMoveMoreDialog; její nepřevzaté umístění v základním dialogu není funkční mezera.

## Nálezy a nejmenší další podetapy

### G01 — Nezapojený wrapper vytváření adresáře; S31a1, blokuje S32

`src/worker.cpp::SalCreateDirectoryEx` stále obsahuje staré tělo s lokálním
`3 * MAX_PATH` bufferem a podmíněným přechodem na wide API. Donor hunk 859
nahrazuje tělo voláním `WorkerCreateDirectory(name, err)`. Samotný wide helper
a jeho přímé worker použití již existují a mají testy; to neověřuje tento wrapper.
Wrapper má živé volající: MakeDir v `src/fileswn5.cpp`, veřejný SDK delegát v
`src/zip.cpp` a spotřebitele FTP/DemoPlug. Nejde o prokázaně mrtvý kód.

S31a1 má převzít úzké donorové zapojení při zachování současných wide oprav a
ověřit skutečný wrapper pro běžné ASCII, Unicode, UTF-8 byte-long, UTF-16 long,
relativní vstup, trailing dot/DOS collision a přesnou chybu/retry. Vyžaduje oba
host buildy, relevantní nativní konzumenty a skutečné GUI MakeDir. Audit doložil
chybějící zapojení staticky; neprovedl destruktivní demonstraci chybné operace.

### G02 — ANSI fallback inline přejmenování; S31a2, blokuje S32

`src/fileswn5.cpp::RenameFileInternal` nemá donorovou časnou Branch odbočku do
`RenameFileInternalW` (smíšený hunk 165). Běžná cesta F2 je wide a již byla
ověřena. `QuickRenameBegin` však při neúspěchu wide edit control může vytvořit
ANSI control; `HandeQuickRenameWindowKey` potom volá starou funkci. Tento vzácný
fallback nesmí odvodit cíl z rootu a basename místo skutečné identity položky.

S31a2 má převzít odpovídající směrování nebo přesně odmítnout nepodporovaný
fallback. Vynucený fallback test musí zachovat přesnou identitu, ochrany cache,
reentrance a obyčejné přejmenování; následně Debug/Release a GUI. Běžný úspěšný
wide GUI scénář nepokrývá tuto větev. Záměna uživatelského souboru nebyla v auditu
runtime testem reprodukována; chybějící hook a dosažitelná větev jsou doložené.

### G03 — Rozsah a odložené source-contract testy; S31a3, blokuje S32

`branch_view_contract_tests.py` stále odkládá
`test_temporary_dialog_selection_uses_exact_branch_identity`. Jeho čerstvé
explicitní spuštění prošlo (1 test). Je nutné jej zapojit do běžného běhu.

`branch_operations_contract_tests.py` implicitně spouští OrdinaryOperations,
14 z 20 metod; runner nepředává `--scope All`. Čerstvé explicitní All spustilo
20 metod: 18 PASS a 2 ERROR. Test názvové schránky vyhledává staré
`if (IsBranchView())`, kde port záměrně používá širší `if (Is(ptDisk))`.
Test fallbacku Shell menu hledá starou přesnou podmínku po změně bezpečného
směrování kontextové nabídky. Jde o ValueError selektoru, nikoli důkaz selhání
souborové operace, ale výsledky rozhodně nejsou PASS.

S31a3 má aktualizovat pouze vyhledání skutečných relevantních těl, zachovat
bezpečnostní assertions, zapnout All a dosud odložený test. Všechny dosavadní
regrese zůstanou registrované. Inventář zachoval všech 41 pojmenovaných donorových položek; před S31a1 jich
port má 57. Samotná existence položky neznamená běh všech jejích testů.

### G04 — Bezpečně nepodporované operace; S31b, rozhodnutí před S32

Následující odchylky mají explicitní ochrany. Jejich plošné odstranění není oprava:

| Oblast | Stav proti donoru a rozhodnutí |
| --- | --- |
| `TryEnterFileAsArchive` | Branch guard zachován, donorové úplné identity v hunks 43/44 nepřevzaty. Odlišit tento vstup od již ověřené běžné Enter/archive cesty S27a2. |
| `FocusShortcutTarget` | Branch guard; donorový `FocusBranchShortcutTarget`, deklarace a hook chybějí (hunk 45 a části branch_view.cpp/fileswnd.h). |
| Windows Properties, ChangeAttr, přímé Unpack | Branch vstupy nadále odmítnuté; nepovažovat za úplnou donorovou podporu. |
| Kopírování/přesun do nepodporovaného Branch cíle | Kořen, identita a kontext jsou ověřované; nepodporované cíle zůstávají odmítnuté. |
| F3 plugin viewers | Obecné DLL prohlížeče nejsou připuštěné. Interní a konfigurovaný externí viewer mají vlastní ověření. PictView EXIF menu 188 a konfigurace 197 neprokazují obecné Branch F3. |
| Neznámé plugin příkazy, externí packery | Explicitní admission policy v plugins1.cpp, žádné automatické povolení podle existence DLL. DemoPlug diskový snapshot se SDK>=105 neznamená obecné povolení jeho Branch příkazu. |
| Shell a dlouhé cesty | Program/document oddělení a Windows kontroly zachovány. Nepodporovaný případ odmítnut s vysvětlením; LongPathsEnabled=0, registry a zabezpečení se neměnily. |
| PAK Optimize | Záměrně skryté menu v původním kódu i donoru; uživatel výslovně zvolil zachování a dokumentaci. Test těla příkazu nepřidává viditelnou funkci. |
| ZIP Repair | Záměrně skrytý příkaz i v donoru; nepovažovat jej za dokončenou aktivní nabídku. |

S31b má pro konkrétní veřejně požadované core operace vybrat samostatnou
implementaci s testy, nebo výslovné release omezení a kontrolu všech jejich vstupů.
PAK Optimize se neobnovuje. Přijaté limity Windows se neobcházejí. Do uzavření
rozhodnutí nelze tvrdit úplnou funkční paritu ani zapnout celý port pro vydání.

### G05 — Hranice skutečné validace; S31c, uzavřít před S32

- Undelete RestoreEncrypted2 má native EFS callback stubs a skutečné GUI kopie nešifrovaných souborů. Windows EFS import/export skutečně šifrovaných dat zůstává NOT_TESTED. Je třeba cílená skutečná validace, nebo výslovná podpora omezená v dokumentaci a admission policy.
- Connect image ověřuje přesný wide prefill a odmítá nepodporovaný ACP/legacy vstup. Fyzický disk a všechny backend scénáře nebyly tímto ověřeny.
- Historické SMB/live UNC/offline cancellation a některé ADS/ACL/hardlink scénáře mají konkrétní unavailable/skip hranice; test řetězce UNC není skutečný SMB provoz. Chybějící prostředí nesmí být přepsáno na PASS.
- Všech 11 překladů má ověřené Branch resource ID/řádky. Kompletní jazykový balicí/staging runtime matrix zatím neproběhl. Čistá podporovaná CI matice je Debug Win32/x64/ARM64 se stávajícím vyloučením SFTP; CodeQL Release x64 není celé Release sestavení všech pluginů.

S31c má uzavřít konkrétní validační tvrzení: doplnit potřebnou skutečnou zkoušku,
nebo zúžit veřejný příslib a ověřit bezpečné odmítnutí. Audit neopakuje celou
historickou GUI matici ani neprohlašuje všechny tyto scénáře za novou regresi.

### G06 — Release dokumentace a malé nepřesnosti; S32 po bráně

Chybějí donorové `doc/branch-view.md` a `doc/branch-view-plugin-paths.md`.
Jejich vytvoření patří plánované S32: popsat skutečný port a jeho limity, bez
převzetí donorových historických testů jako target důkazu. Dva komentáře nových
služeb v spl_gen.h stále popisují jen ordinary disk/UNC; rozšíření na Branch
má být popsáno bez změny signatur nebo layoutu. Chybějící IDE filter položka
explorerpropertywork.h nebrání build registraci; může být uklizena samostatně.

## Převzaté kontrakty a úmyslné adaptace

| Oblast | Auditované rozlišení |
| --- | --- |
| Scanner, scheduler, publikace | Donor algoritmy zachovány; S30 připojuje recursive watchery a owned full-path notifikace bez ztráty Stop Scan. S28a mění pouze zobrazení Path; absolutní cesty pro I/O zůstávají. |
| Identita a životnost | Snapshoty vlastní cesty/metadata; Name/NameW zůstávají basename. Target přidává reentrance/cache kontroly, exact temporary selection a owned parent metadata. |
| Host operace | Wide create/copy/move/delete/recycle, clipboard/drop a packer enumerace mají jednotlivé checkpointy; jejich důkaz nelze přenést na odmítnuté operace G04 ani wrapper G01. |
| Shell | Přímé programy oddělené od dokumentů, bezpečnostní kontroly Windows, odložené host dispatch po uzavření menu, ochrany vlastnictví a rootů. Žádné vypnutí ochrany nebo změna registrů. |
| Balení | ZIP/7-Zip snapshoty a host callbacky; target wide SafeDeleteFile opravuje odstranění přesné zdrojové identity při move packing. |
| Plugin spotřebitelé | Checksum, Split/Combine, FileComp, Automation, Salamatrix, připuštěné ZIP/7-Zip/PictView/Undelete příkazy mají úzké native/GUI důkazy v S27f checkpoint dokumentech. Obecná kompatibilita DLL není kompatibilita každého Branch příkazu. |
| SafeFile | Target navíc řeší krátký relativní vstup pod dlouhým cwd; široký create/open/retry se nepřepisuje starší donor podmínkou. |
| Icon/handle tracking | Zachované dřívější target opravy účetnictví a ukončení CLR. Donorová odstranění registrací nesmějí vytvořit dvojí registraci nebo skrýt diagnostiku. |
| Konfigurace a historie | Uložená preference relativní/absolutní Path s absolutním defaultem; tab/history a vypnutý process gate podle S29. Persistence se neposuzuje šířeji než skutečné scénáře. |

## SDK, pět providerů, resources a data

Nejnovější skutečně publikované upstream vydání znovu ověřené přes GitHub API je
[5.0-samandarin-0.15.1](https://github.com/KRtkovo-eu-AI/salamander/releases/tag/5.0-samandarin-0.15.1),
commit `c0cddbe7442fe49abcb859624e271ce687aee0d8`; větev 0.16 není vydaný SDK základ.
Nezávislý parser všech 13 SDK hlaviček porovnal 38 Abstract rozhraní a 632 pure
virtual metod: vydané prefixy pořadí/podpisů zůstávají shodné. spl_com.h, tedy
CFileData/PluginData kontrakt, odpovídá vydání po textové normalizaci. Existující
ABI golden/HEAD testy doplňují tento širší audit. To je prefix/layout kontrola,
nikoli důkaz funkce libovolné staré binárky v Branch View.

Adresáře JavaScript, Python, PowerShell, PHP a Lua runtime providerů nemají změnu
BASE..S30. Sdílené Salamatrix sides rozhraní odpovídá donoru. Čerstvé skutečné
Automation/shared-consumer native testy `--scope All --configuration Both`
prošly 243 kontrolami v každé konfiguraci včetně pěti forwarding kontraktů,
nezávislého JSON escaping/schema/metadata a původního limitu 64 položek. Tento
běh není nové GUI spuštění všech pěti enginů; skutečné engine/runtime scénáře
mají vlastní historickou S27f.06a evidenci.

Zůstává limit 64 selected položek a 65536bytový provider buffer (65535 payload
plus terminátor). ZIP/Automation ponechávají SAL_MAX_PATH **byte** kapacitu,
FileComp svou vstupní byte kapacitu a Undelete image backend lossless ACP limit.
Tyto explicitní limity nejsou rozšířeny samotným wide snapshot API. Změna limitů
nebo runtime schématu není součást portu ani tohoto auditu.

Všech 13 donorových native projektů existuje. Runner zachovává 41 donorových
pojmenovaných položek; snapshot S30 jich má 57. Jde o Name assignments, nikoli
ClassName labely či dynamicky registrované projekty. Původní údaj 59 chybně
zahrnoval dva ClassName labely; S31a1 opravuje pouze tuto početní nepřesnost.
G03 vyjmenovává skutečné scope mezery.
Všech 15 donorových IDS_BRANCH_* má shodné číselné ID a právě jeden řádek v
každém z 11 jazyků. Conversion cfg zůstávají ve Windows CP1250/1251/1252 bez
UTF-8 BOM; 28/17/25 tabulek má po 256 bajtech. Česká jména Kameničtí/KOI-8 ČS2
byla kontrolována i po dekódování, nikoli plošnou UTF-8 konverzí binárních dat.

## Důkazy a jejich omezení

Přijatá S30 má pro přesný head 58aabd60 všech 13 kontrol SUCCESS v
[PR #62](https://github.com/majklvi/salamander/pull/62), včetně skutečných CodeQL
analyze/SARIF kroků a všech reálných Build full solution kroků čisté podporované
matice. [CodeQL/build run](https://github.com/majklvi/salamander/actions/runs/40186355068)
a [clean matrix](https://github.com/majklvi/salamander/actions/runs/40186366561)
jsou acceptance S30, nikoli důkaz uzavření G01–G06. Interní validační draft se
nemerguje do samandarin-0.16. Lokální S30 GUI/build/native důkazy zůstávají v
[S30](steps/S30.md); audit netvrdí nový host build nebo celou GUI zkoušku.

Audit historie prošel 104 checkpoint commitů a 83 metadatových záznamů.
655 source hash vazeb odpovídá prvnímu odvozenému checkpointu; všech 753
kontrolovaných archivovaných evidence hashů souhlasí. Ze 16 počátečních source
neshod se 6 dohledalo v odpovídajícím ancestor snapshotu. Zbývajících 10 jsou
archivované rozpracované source snapshoty, které nebyly commitnuté bajtově:
S06 owned runner; dva S12-preflight WIC testy; S26b PLAN; S27a PLAN a pět zdrojů/
harnessů. Neoznačují se jako ověřené commitnuté zdroje. Staré schéma metadata
není úplný replay každého buildu; aktuální rozhodnutí stojí také na skutečném
kódu, finálních checkpoint vazbách a fresh testech. Starší NOT_PUSHED/PENDING
odstavce v etapových dokumentech jsou historické, pokud je novější záznam nahrazuje.

Úplná lokální evidence je mimo Git v `codex_pr769_prompts/evidence/S31/`:
`inventory.json`, `inventory-summary.json`, `residuals/`, `classification.json`,
`checkpoint-evidence-audit.json`, `abi-provider-audit.json`,
`provider-consumers.json` a stdout, `tests-resources-data-audit.json`, explicitní
probe stdout/stderr a generátor `write-coverage-tables.py`. Historická S30 CI
odpověď je v sousedním `S30/ci-completion.json`. Do projektu se necommitují logy,
binárky, raw donor diff ani automaticky generované fixture soubory.

Původní donor výsledky se nepoužily jako nové target PASS. Předchozí ZIP dialog
Debug heap diagnostika byla doložena i na parent checkpointu; zůstává evidovaným
baseline omezením, nikoli tvrzením, že tiché S30 ukončení opravilo všechny úniky.

## Pořadí pokračování

1. S31a1 — wrapper SalCreateDirectoryEx a přímé regresní důkazy.
2. S31a2 — bezpečné ANSI rename fallback směrování a vynucená větev testu.
3. S31a3 — aktivace úplného source-contract rozsahu bez oslabení assertions.
4. S31b — uzavření podporovaných/odmítnutých operací G04 po konkrétních příkazech.
5. S31c — uzavření chybějících skutečných validací nebo omezení příslibů G05.
6. S32 — teprve po uzavření brány release dokumentace G06 a publikační rozhodnutí.

Audit S31 je tímto dokončen. Produkční opravy, další publikace a odstranění gate
jsou samostatné kroky. S31 nepřepisuje žádný přijatý checkpoint ani nepředstírá,
že zelené CI uzavírá nepouštěné testy či nepodporovanou funkci.
