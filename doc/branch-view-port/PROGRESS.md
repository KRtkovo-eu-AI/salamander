# Stav portu PR #769

## S13 — IMPLEMENTED / lokální brána PASS s baseline omezením menu (2026-10-22)

Tři produkční ZIP soubory main.cpp, precomp.h a vcxproj/zip.vcxproj jsou
EXACT donor 47661e92. ExecuteMenuItem používá jednou zachycený owned snapshot,
respektuje skutečné parents a explicitně odmítá SAL_MAX_PATH byte limit bez
truncation. Archive panel ponechává původní archivní kontrakt. ZIP engine ani
host-managed PackToArchive nebyly změněny; PackToArchive zůstává S22.

Archive menu test driver/template jsou ADAPTED: compiler helpers z již
existujícího disk_selection_snapshot_tests místo budoucího renamer driveru,
scope ZIP/All (default ZIP) výslovně odkládá pouze 7-Zip assertions do S14.
Všechny původní assertions zůstávají ve zdroji. ZIP scope 19 checks/config
D/R PASS: skutečný command body, úplné cesty při zániku listing, ASCII,
Unicode, UNC, UTF8 byte/UTF16 long paths, directories, capture failure,
notifications, formatting, invalid UTF16 a engine byte limit. Host snapshot
a engine jsou zde test doubles; toto není skutečný packing test.

Host+ZIP a tranzitivní dependencies D/R x64 /warnaserror PASS, pytest 49 PASS.
Skutečný ZIP engine přes GUI Test Archive: Debug dvě vybrané položky
ordinary.zip a ž-東京.zip, Release samostatný focused ž-東京.zip bez výběru,
vše v dlouhé Unicode cestě, každý výsledek is OK. Načtené zip.spl hashově
odpovídají právě sestavenému pluginu. Šest vlastních archivů nezměněno,
12 nezávislých ověření obsahu členů, 97 předchozích input files a sentinel
nezměněny, žádné temp zbytky, normální shutdown bez reportu, config obnoven.

Původní soukromý seed měl ZIP asociaci na 7-Zip. Pouze vlastní nový seed
pro S13 ji nastavuje na ZIP index -1, uživatelské nastavení nedotčeno.
Po správné asociaci se samostatně potvrdilo baseline omezení: menu při
výběru 100-CJK basename používá zkrácený CFileData.Name a zakáže příkaz.
Tento GUI případ je SKIP, nikoli PASS; ExecuteMenuItem a nové wide vstupy
se přes něj nespustí. GetMenuItemState je beze změny shodný s donorem.
Omezení musí být vyřešeno před tvrzením úplné GUI podpory takových názvů.

Source parent ac8641ac, runtime patch SHA256
a8c95db05b67ad0545a1edc5374183b341460ff97132c3622d8f12f6d6b1e1b4.
Evidence evidence/S13/source-manifest.json, final-validation.json,
focused-menu.*, host/zip-*.json, *-ok.txt, *-loaded-module.json a private seed
provenance. Runner zachovává starších 54 groups a přidává jednu; celý runner
znovu neběžel. Full-solution CI/CodeQL a push NEPROVEDENY. Pokračuje S14.

## S12 — IMPLEMENTED / lokální brána PASS (2026-10-22)

Navazuje na samostatný WIC baseline předkrok 270f2cb6. Owned snapshot
CEnumFiles a wide EXIF regenerace thumbnailu tvoří jeden paired krok.
thumbs.cpp a EXIF exif.c/.h/.def jsou EXACT donor 47661e92; nový C++ test
je také EXACT. Projekt adaptován pouze o spl_diskselection.h, Python driver
pouze o --configuration (default Both zachován). S18 navigation/title zůstává
DEFERRED. Testy S05 obnovují skutečný CEnumFiles a obě HostTests assertions,
zachovávají target DemoPlug a released SDK ABI kontroly.

Aktuální zdroj: parent 270f2cb6; runtime patch SHA256
254d9562832c4c8e17fb1bce6664a6a0a149acce910db9cea53585c9edfd1c52.
Host+PictView+paired EXIF D/R x64 /warnaserror PASS; skutečná staged EXIF
v10, wide i legacy export a bezpečné I/O: 51 checks/config PASS. Zahrnuje
Unicode/long path, temporary cleanup, původní obraz, read-only a locked
failure bez poškození originálu. Snapshot test 57 checks/config PASS,
WIC callback regresní test 39 checks/config PASS, decoder contract PASS,
pytest 49 PASS. Runner přidává thumbnail skupinu a zachovává všech 53 starších.
Celý runner znovu neběžel: jde o relevantní lokální bránu; starší initial
S12 timeouty a shard výsledky jsou zachované, nejsou aktuální full-run PASS.

Skutečné GUI D/R Update Thumbnail PASS: 3 vybrané JPEGy/config v dlouhé
Unicode cestě, ordinary.jpg, ž-東京.jpg a 100 CJK basename. Všech 6 výstupů
má platný JPEG thumbnail v EXIF IFD1; orientation=6 zachována a všechny
non-APP/COM JPEG segmenty včetně compressed scan bajtově shodné s originály.
Skutečně načtené PictView/EXIF DLL odpovídají launch hashes nové sestavy.
Předchozích 87 input files, 3 vstupy předkroku i sentinel nezměněny; žádné
temp zbytky, normální ukončení host/reporter bez reportu, konfigurace obnoveny.
WIC je skutečný dostupný engine; předkrok reuse encoderu není nový engine.

Evidence evidence/S12: source-manifest.json, resumed-final-validation.json,
resumed-*.json/log, *-launch.json, resumed-*-loaded-modules.json.
Starý runtime e22210d1 a původní neúspěšný GUI pokus zůstávají jako initial
evidence; po předkroku byla brána znovu provedena na aktuálním zdroji.
S12 nepushed; full-solution CI/CodeQL NEPROVEDENY. Pokračuje lokální S13.

## S12-preflight — IMPLEMENTED / lokální brána PASS (2026-10-22)

Samostatná oprava existujícího WIC backendu před obnovením S12. Původní
blob ef72188c53a6be5a713841475f654c96b102885d je shodný v targetu, release
base i donoru 47661e92. AddExtendedPrefixIfNeeded používal SAL_MAX_PATH
místo Win32 MAX_PATH; 415-unit JPEG tak skončil jako Shell bitmapa.
sPVSaveImage navíc odmítal NULL filename a neuměl PVSF_USERDEFINED_OUTPUT.
Oprava používá původní SaveFrame encoder přes seekable memory IStream,
pak kontrolované 64KiB callback writes. File output zachovává původní encoder.
Zrušení funguje před encodingem a při předávání výstupu; nepřerušuje vnitřní
WIC encoding. Celý encoded output je v paměti, limit legacy callbacku DWORD.

Host+PictView+EXIF Debug/Release x64 /warnaserror PASS, EXIF stále v9.
39 native checks/config skutečného prefix helperu a callback adaptéru PASS;
encoder dependency v tomto malém harnessu je stub. Doplňující skutečná
Release pictview.spl přes přesné PDB symboly: 17 checks PASS, dlouhá Unicode
cesta rozpoznána jako JPEG, platný 32x24 callback JPEG nezávisle dekódován,
short-write/cancel/invalid descriptor a běžné JPEG/PNG/TIFF file saves PASS.
Native decoder D/R build a runtime PASS, decoder source contract PASS,
pytest 49 PASS. Původních 52 runner groups zachováno, přidána jediná skupina;
celý runner v tomto předkroku znovu neběžel.

Skutečné GUI Update Thumbnail v obou konfiguracích nad vlastní krátkou
ASCII cestou PASS: 714 -> 1761 bytes, 1005-byte JPEG thumbnail v EXIF IFD1,
orientation 6 zachována, všechny non-APP/COM JPEG segmenty a compressed scan
bajtově shodné. Skutečně načtené plugin/EXIF DLL hashově evidovány, normální
ukončení bez reportu, konfigurace obnoveny, 94 předchozích input files a vlastní
sentinel nezměněny. GUI dlouhých cest patří obnovené S12, není zde tvrzeno.

Evidence evidence/S12-preflight; source-manifest.json, gui-final-validation.json,
actual-release-backend-v2.json, *-loaded-modules.json a bounded command logs.
Source parent 45d4a3e7; runtime patch SHA256
437e6119b04185901f98efcee245431e89c5715018a78dbdcf0c3d7bcbcf8d3c.
S12 donor candidate e22210d1 je odložen mimo Git; jeho starší výsledky nejsou
validační branou tohoto předkroku. S12 musí být obnovena a znovu ověřena.
Předkrok nepushed; full-solution CI/CodeQL pro něj NEPROVEDENY.

## S11 — IMPLEMENTED / lokální brána PASS (2026-10-22)

S10 přijato 60987d42 po zeleném PR #33 včetně skutečného CodeQL a celé
sestavy. Lokální S11 přebírá tři EXACT donor 47661e92 soubory Salamatrix:
salamatrix_packages.cpp, salamatrix_sides.h, vcxproj/salamatrix.vcxproj.
Shared context JSON vychází z owned wide snapshotů; native ItemInfo ABI,
JSON schema, 64-item limit a pět providerů se nemění. Native ItemInfo
překročení odmítá; JSON neprochází jeho fixed byte kapacitami.

Adaptace jsou pouze v testech/runneru: All scope aktivuje S10 odložené
TestSides/TestDynamicJsonContext a všech pět forwarding contracts. Nový
TestPayloadBoundaries kompiluje skutečné CopyResult a protocol header;
ověřuje exact/undersized/null buffer, reálný 65536-byte response limit,
1MiB whole-frame hranici, escaping a nezávislé JSON parsování celé struktury.
Žádné oslabení donor assertions; starší Automation-only scope stále 161
checks/config. All má 243 checks/config. Host+Salamatrix+Python provider
D/R x64 /warnaserror PASS, 52 native groups/config, Python 49 PASS.

Skutečný Python worker -> PackageManager::HostDispatch v D/R PASS: dvě
selected položky, focused 255 UTF16-unit / 763 UTF8-byte CJK basename,
plná 652-unit cesta. Oba hosty/reporter normálně zavřeny, žádný nový report;
3 nové a 84 předchozích input files hashově nezměněné. Soukromé konfigurace
obnoveny, vlastní testovací extensions archivovány mimo runtime. GUI není
běh všech pěti providerů; pět forwarding/capacity kontrol je source contract,
existující process-runtime testy mají skutečný Python/PowerShell a PHP SKIP.
PHP a LoadImageW subcase SKIPs zůstávají SKIP. Evidence evidence/S11,
source-manifest.json a final-validation.json; [steps/S11.md](steps/S11.md).
Runtime patch SHA256 970d582d38b9dbca897f7984a6ff15881117e1676d5c43236163d329fb766b28.

S11 nepushed; publikace/full-solution CI/CodeQL pro S11 NEPROVEDENY.
Podle rozšířeného user goalu po lokálním commitu pokračuje S12. Dřívější
baseline S10 Debug handle a file-focus setter vady nejsou touto etapou opraveny.

## S10 — publikováno a přijato (2026-10-22)

Automation 8ef4ab24 přijato po všech 12 SUCCESS checks aktuálního headu
[PR #33](https://github.com/majklvi/salamander/pull/33). Skutečný CodeQL job
119984548335 v runu 39824113333: Initialize, Release x64 MSBuild, Analysis
a SARIF SUCCESS; dokončen 17:53:32 UTC. Čistý full build 39824142309
SUCCESS Win32/x64/ARM64 včetně přípravy závislostí. Jeho CodeQL je záměrně
SKIP pro dispatch, skutečná analýza doložena v PR. Evidence evidence/S10:
accept-current-pr.stdout.log, check-full-build.stdout.log,
check-pr-build.stdout.log a publish-summary.json. Lokální baseline výhrady
(Debug handle tracking, starý dlouhý file-focus setter) i PHP/LoadImageW
subcase SKIPs zůstávají platné, zelené CI je neopravuje.

Přijímá se lokální checkpoint; interní draft se nemerguje do reference base.
Uživatel rozšířil zadání na postupné lokální etapy S11 až S20: zachovat
samostatné commity a každou lokální validační bránu; toto výslovné zadání
nahrazuje dřívější omezení na jedinou etapu v běhu. Další publikace/PR
nejsou tímto automaticky zadány. SOURCE/CONTRACT piny se nemění.

## S10 — IMPLEMENTED / lokálně ověřeno s baseline omezeními (2026-10-22)

S09 přijato na f13b3ed1 po ověření PR #32 a čisté sestavy. Lokální větev
integration/pr769-s10-candidate přebírá pouze Automation: vlastněné wide
COM položky, sdílený disk snapshot collections/enumerators, nezávislé cursory,
Reset bez nového capture a samostatné focused položky. SDK/COM IDL/host,
Salamatrix, CI a SOURCE/CONTRACT piny beze změny. Sedm produkčních souborů
EXACT donor 47661e92; itemcoll.cpp ADAPTED pouze inicializací iKey=-1 po
prokázaném MSVC C4701. Dva testy ADAPTED: explicitní Automation/All scope
odkládá pouze budoucí S11 a přidává skutečné COM collection/enumerator metody.
Všechny donorové Automation assertions zachovány, S11 assertions ponechány
v template/All režimu. Runner zachovává všech 51 starších skupin.

Host a Automation Debug/Release x64 /warnaserror PASS, 161 focused native
checks/config, 52 regresních groups/config a Python 49 PASS. Skutečný plugin
v D/R prošel 32 COM kontrolami/config nad devíti kategoriemi metadata/cest,
včetně spuštění skriptu na 344 UTF8-byte a 416 UTF16-unit cestě. Retained
items/collections/enumerators přežily deselection, refresh, změnu panel path
a další focused getter. Sort/destrukce řádků a poslední snapshot/COM release
ověřeny nativně nad skutečnými migrovanými metodami; GUI samo není důkazem
všech refcountů. Všech 13 finálních vlastních a 71 předchozích vstupů hashově
beze změny, konfigurace obnovena, oba finální hosty normálně ukončeny.

Baseline výhrady: původní scriptlist.cpp v Debug při raw CreateFileW a
tracked CloseHandle hlásí neevidovaný handle; před skriptem zaznamenáno a
potvrzeno, diagnostika nevypnuta. Původní host ChangeDir při přímém zaměření
souboru přes MAX_PATH vyvolal buffer assertion; první Debug host byl řízeně
ukončen, nejde o normální shutdown. Finální dlouhé cesty proto ověřeny přes
přechod do adresáře a skutečné COM Items. Tento starší setter limit zůstává
NEOPRAVEN; S10 nelze prezentovat jako úplnou podporu dlouhého file-focus
přechodu. Přesné místo a shoda s releasebase doloženy mimo Git.
PHP/LoadImageW subcase SKIPs zůstávají SKIP. Evidence evidence/S10 včetně
neúspěšných pokusů, source-manifest.json, runtime.patch, final-validation.json
a [steps/S10.md](steps/S10.md). Lokální runtime patch SHA256
cad448992989c7d69d9f6a90906765565eab8a6e1316f918979e447a0c13a7c7.
Publikační full build/platformní CI/CodeQL S10 NEPROVEDENY. S10 nepushed,
accepted reference zůstává f13b3ed1. S11 v tomto běhu nezahájena.

## S09 — publikováno a přijato (2026-10-22)

Split & Combine 8b97128d přijato po 12 SUCCESS checks přesného headu
v interním [draftu PR #32](https://github.com/majklvi/salamander/pull/32).
PR buildy Win32/x64/ARM64 /warnaserror, native/Python/source-contract,
VSIX, growth a squash guard prošly. Skutečný CodeQL job 119952659278
v PR runu 39814889856 dokončen 2026-10-22 16:48:06 UTC: Initialize,
Release x64 build, Perform Analysis a Upload SARIF SUCCESS. Samostatný
čistý full build 39814915560 SUCCESS na všech třech platformách včetně
přípravy závislostí. Jeho CodeQL záměrně SKIP pro workflow_dispatch;
skutečná analýza je doložena samostatně. Evidence mimo Git evidence/S09:
check-current-pr.stdout.log, check-full-build.stdout.log,
check-codeql-job.stdout.log a publish-summary.json. Lokální GUI/obsahové
důkazy a známé PHP/LoadImageW subcase SKIPs zůstávají evidované.

Přijímá se lokální checkpoint, draft se nemerguje do samandarin-0.16.
SOURCE/CONTRACT, donor/release piny, SDK a Branch gate beze změny.
Navazuje pouze S10 Automation; Salamatrix patří do S11.

## S09 — IMPLEMENTED / lokální brána PASS (2026-10-22)

S08 přijato na 68b66c33 po 12 SUCCESS checks PR #28, skutečné CodeQL analýze
včetně SARIF a čistém Win32/x64/ARM64 full buildu. Lokální větev
integration/pr769-s09-candidate přebírá jen Split & Combine: owned selection
před dialogem/I/O, skutečný parent pro default target/sibling discovery,
full paths explicitních dílů, wide API/dialogy a UTF8 BAT. Všech 10 produkčních
a 2 testové soubory jsou EXACT donor 47661e92. Jediný adaptační kód je
registrace selection_paths_tests.py s podporovaným --configuration ve
stávajícím runneru; všech 50 starších skupin zachováno. SDK/host/ostatní
pluginy/CI/Branch gate ani SOURCE/CONTRACT se nemění. Input volume/file IDs
se kontrolují před target creation; read handles bez write/delete sharing
chrání i před target-alias race. RAII zachovává handle/progress cleanup.

Host+plugin Debug/Release x64 /warnaserror PASS; native actual command/target/
plan/engine/BAT harness 210 checks/config PASS; celé native sady 51 groups/config
PASS; stávající CI pytest výběr 49 PASS. Host Debug prvně selhal až ve staging
skriptu na chybějícím Get-FileHash kvůli PSModulePath; opravena pouze invocation
environment, oba konečné buildy PASS. Neúspěšný protokol zachován. PHP runtime
a historický LoadImageW long-bitmap backend subcase zůstávají SKIP, ne PASS.

Skutečné GUI v produktovém Windows Dark Mode (experimental): Debug ASCII,
Debug Unicode UTF8-long a Release Unicode UTF16-long Split/Combine. Devět
vytvořených dílů porovnáno po bajtech a pořadí, tři výsledky přesně shodné
se zdroji; tři UTF8 BAT/metadata/CRC32 ověřeny nezávisle. Focus-only discovery
našlo správné tři sourozence a načetlo původní název (včetně % & ^ !).
Relativní nový Combine target se vztahoval ke skutečnému parentu. Skutečný
GUI pokus zapsat do input dílu i jeho hardlinku byl odmítnut bez změny dílů.
Aktivní Split i Combine nad 4GiB souborem byly zrušeny přes Cancel/Yes;
nedokončené outputy odstraněny, původní zdroj zachován. Všech 5 vlastních
+ 66 předchozích originálů má původní SHA256; přesný inventář 16 očekávaných
outputů navíc. Čtyři vlastní hosty/reportery normálně ukončeny, žádný nový
report, configstorage.ini obou konfigurací bajtově obnoveny.

Evidence mimo Git evidence/S09; testovaný parent 68b66c33 + runtime.patch
SHA256 18a3a0ac9ce186c114b295d266abeb5562dd6d0306fab394be4b92ff2ea267e2.
Source manifest ověřuje přesné donorové bloby/raw SHA i runner; launch logy
vazbu na skutečné host/plugin binárky. Finalize-S09.py znovu ověřil celý
řetězec po GUI. [steps/S09.md](steps/S09.md) uvádí příkazy a meze pokrytí.
Mixed-parent rows, alias race, resolver/read failure a odmítnutí adresáře
jsou native fixture; skutečný UNC share/BAT execution/pickery nebyly GUI
ověřeny. Publikační čistý full build/platformní CI/CodeQL S09 NEPROVEDENY,
větev zatím lokální. Další krok push/fork CI po zadání uživatele; po přijetí
S09 následuje S10 Automation. S10 v tomto běhu nezahájena.

## S08 — publikováno a přijato (2026-10-22)

Checksum c76b52b5 přijato po ověření všech 12 checks přesného headu
v interním [draftu PR #28](https://github.com/majklvi/salamander/pull/28).
Win32/x64/ARM64 Debug /warnaserror, native/Python/source-contract, VSIX,
growth a squash guard SUCCESS. Skutečný CodeQL job 116089251585 dokončen
2026-10-14 20:44:16 UTC: Release x64 build, Perform Analysis a Upload SARIF
SUCCESS. Čistý full build 38632382603 SUCCESS na všech třech platformách;
CodeQL v dispatch záměrně SKIP, skutečná analýza v PR runu 38632358631.
Evidence mimo Git evidence/S08/{resume-current-pr.stdout.log,
resume-full-build.stdout.log,resume-codeql-job.stdout.log,publish-summary.json}.
Lokální GUI/test důkazy a evidované PHP/LoadImageW subcase SKIPs platí dál.

Přijímá se lokální checkpoint; draft se nemerguje do samandarin-0.16.
SOURCE/CONTRACT, donor, release base, SDK i veřejný Branch gate se nemění.
Navazuje pouze S09 Split & Combine s exact input paths a input safety.

## S08 — IMPLEMENTED / lokální brána PASS (2026-10-14)

S07 včetně obecné CI opravy přijato na dec9c828 po čerstvém ověření všech
10 checks PR #27, skutečné CodeQL analýzy/SARIF a čistého full buildu
Win32/x64/ARM64. Nová lokální větev integration/pr769-s08-candidate přebírá
Checksum jako jeden celek: owned capture před modeless UI/workerem, přesné
FullPath oddělené od display/export jména, wide rekurze/I/O/export a Verify
podle skutečného rodiče manifestu. Všech 9 produkčních a 2 testové soubory
odpovídají donorovým Git blobům 47661e92 beze změny. Jediná adaptace je
registrace testu do stávajícího runneru bez neexistujícího --configuration
přepínače (driver sám vždy testuje D/R). SDK, algoritmy, resources/formát
manifestu, disk-only guard, Branch gate a obecná CI oprava zůstávají zachovány.

Host+Checksum Debug/Release x64 /warnaserror PASS; native actual-consumer
harness 62 checks/config PASS, celé native sady 50 groups/config PASS,
CI pytest výběr 49 PASS. Native digest facade je stub; reálné algoritmy
ověřil skutečný plugin přes GUI: 80 nezávislých CRC/MD5/SHA1/SHA256/SHA512
porovnání nad osmi zdroji v D/R, 32 úplných exportních identit a digestů
(MD5 relativně, SHA256 v jiném Unicode adresáři), 32 pozitivních Verify
řádků. Negativní manifest přesně 1 Corrupted + 7 OK, 0 missing/skipped.
Zrušení aktivního výpočtu 512 MiB přešlo do Canceled bez exportu. Debug
Calculate/export přežil změnu source panelu a refresh po capture; native
queued worker přežil úplné zrušení panel rows včetně mixed-parent fixtures.
Všech 11 vlastních + 55 předchozích originálů má přesné původní cesty/obsahy;
jen 4 očekávané exporty navíc. Všechny 4 hosty/reportery normálně ukončeny
bez nového reportu, oba configstorage.ini bajtově obnoveny.

Vazba evidence: parent dec9c828 + runtime.patch SHA256
728f8592e28333d8d5b3e89ab8f348d0bc1202954afc36a5e20f8571a63f71b1,
donorové bloby/raw hashe a hashe skutečně spuštěného hostu/Checksum pluginu.
Podrobné příkazy a limity: [steps/S08.md](steps/S08.md), evidence mimo Git
v evidence/S08. Počáteční infrastruktura test runneru neměla msbuild v PATH;
po opravě pouze prostředí celá sada PASS, původní neúspěšný protokol zachován.
PHP a historický LoadImageW long-bitmap subcase SKIPs nejsou vydávány za
provedené testy. Nová publikační platformní CI/CodeQL a čistý full build S08
zatím NEPROVEDENY: větev je lokální. Accepted reference zůstává dec9c828.
Další checkpoint je push/fork CI po zadání uživatele, potom přijetí S08;
S09 Split & Combine nebylo zahájeno.

## S07 — publikováno a přijato (2026-10-14)

Batch Rename 3f9a0134 a obecná oprava full-build CI 6c1b034e přijaty po
čerstvém ověření všech 10 checks přesného headu 6c1b034e v interním
[draftu PR #27](https://github.com/majklvi/salamander/pull/27).
Win32/x64/ARM64 /warnaserror, native/Python, VSIX, growth a squash guard
SUCCESS. Skutečný CodeQL job 116018461833 dokončen 18:19:06 UTC: Initialize,
Release MSBuild, Perform Analysis i Upload SARIF SUCCESS. Samostatný čistý
full build 38611664410 SUCCESS ve všech třech architekturách včetně skutečné
instalace Lua a TagLib. Jeho CodeQL job je záměrně SKIP pro workflow_dispatch;
skutečná analýza proběhla v PR runu 38611675021. Původní job na 3f9a0134 byl
nahrazen novým headem a není vydáván za dokončený upload. Evidence mimo Git:
evidence/S07/{acceptance-current-pr.stdout.log,ci-fix-verified.json,
ci-fix-full-final.stdout.log,ci-fix-final-codeql-job.stdout.log,publish-summary.json}.

Přijímá se lokální integrační checkpoint, nikoli merge draftu do release base.
Draft zůstává otevřený; SOURCE/CONTRACT, donor a release base a84dbed6 se
nemění. PHP/LoadImageW subcase SKIPs zůstávají evidované. Navazuje pouze
S08 Checksum; smíšené rodiče zatím native fixture, veřejné Branch UI zůstává
vypnuté. S07 GUI/obsahové důkazy i známé Filecomp UI limity zůstávají platné.

## S07-CI — příprava závislostí čistého sestavení (2026-10-14)

Oprava společného .github/workflows/pr-msbuild.yml: workflow_dispatch
obchází Select changed projects, a proto nyní sám zapne instalaci Lua a
mmviewer TagLib přes stávající instalační krok. PR výběr, triplet mapping,
oddělený statický TagLib root a /warnaserror zachovány. Jde o obecnou opravu
manuálního full buildu, nikoli fork-specific aktivaci CI. Bez změny aplikace
či donorových 18 souborů S07. Předchozí run 38609591197 selhal na chybějících
Lua/TagLib ve všech třech architekturách. Lokální pytest tools/code_metrics/tests
+ tools/codeql/tests: 8 PASS; diff --check PASS. Nový čistý full build a
CodeQL přesného nového headu PR #27 jsou do ukončení CI PENDING; původní
S07 lokální runtime testy se nad identickým kódem neopakovaly. Výsledky a
konkrétní run/job ID budou uloženy v evidence/S07 mimo Git.

## S07 — IMPLEMENTED / lokální brána PASS (2026-10-14)

S06 přijato na f43b9d86 po 12 úspěšných kontrolách PR #26 včetně skutečného
CodeQL. Nová lokální větev integration/pr769-s07-candidate obsahuje Batch
Rename jako jednu etapu: owned snapshot před workerem, přesné rodiče položek,
relative/full mode, preview, wide I/O a Undo. Všech 14 produkčních a 4 testové
soubory odpovídají blobům připnutého donoru 47661e92; vlastní adaptace je jen
registrace dvou testů do stávajícího runneru se zachováním všech starších testů.
SDK/CFileData, disk-only menu guard a Branch gate se nemění.

Host+Renamer Debug/Release x64 /warnaserror PASS. Native panel harness
67 checks/config a I/O harness 89 checks/config PASS; celé native sady
49 groups/config PASS; CI pytest výběr 49 PASS. Přijatý předchozí consumer
negativně: kompilace PASS, 31 očekávaných selhání ze 40 checks (Debug).
Pět vlastních GUI hostů, 21 skutečných přejmenování souborů a jejich Undo
ve čtyřech dávkách: celá devíti-souborová sada v D/R včetně ASCII, Unicode
jména, více Unicode rodičů, UTF8>260 i UTF16>260 cest. Samostatný Debug
duplicate test prokázal rozdílný obsah, třetí nevybranou položku a životnost
po refresh/sort panelu; relative mode zkontrolován v preview. Cancel dlouhého
jména beze změn. Všech 55 fixtures nakonec na původních cestách s původním
obsahem, oba configstorage.ini obnoveny; všech 5 hostů/reporterů normálně
ukončeno bez nového reportu. Uživatelská instalace nedotčena.

Evidenci váže parent f43b9d86 + runtime patch SHA256
c4ae16be26caf38ec5ea1f24520351ceb07a102b05de1236135998fabd06c653,
zdrojové/blokové a binární hashe; podrobně [steps/S07.md](steps/S07.md).
PHP není dostupné a dva historické Win32 LoadImageW long-bitmap subcases
jsou SKIP, nikoli provedené PASS. Nová S07 platformní CI/CodeQL ani čistý
build celé sestavy zatím neběžely: větev nebyla publikována. Accepted reference
zůstává f43b9d86. Nejbližší checkpoint je push a fork CI po zadání uživatele;
teprve po přijetí S07 navazuje S08 Checksum. S08 nebylo zahájeno.

## S06 — publikováno a přijato (2026-10-14)

Kód 9580b153434bde8942dd3b1604aa478fb5e276ea na
origin/integration/pr769-s06-candidate ověřen interním
[draftem PR #26](https://github.com/majklvi/salamander/pull/26).
Všech 12 checks přesného headu SUCCESS: Win32/x64/ARM64 /warnaserror,
native/Python/reporty, VSIX, growth a squash guard; CodeQL gate i skutečná
analýza. CodeQL job 115981207148 skončil 17:04:14 UTC; Initialize,
Release MSBuild, Perform Analysis a Upload SARIF SUCCESS. Žádný check
není pending/skipped. Lokální PHP/LoadImageW subcase SKIPs a známé nezměněné
Filecomp výstupní UI limity zůstávají pravdivě evidované níže.
Při zahájení S07 znovu ověřeno: evidence/S06-final/acceptance-current-pr.stdout.log,
ci-current-codeql-job.stdout.log a publish-summary.json.
Přijímá se integrační checkpoint S06; draft zůstává otevřený a nemerguje se
do samandarin-0.16. SOURCE/CONTRACT ani release base a84dbed6 beze změny.
Navazuje pouze S07 Batch Rename podle jejího úzkého donorového diffu.

## S06 — IMPLEMENTED / lokální brána PASS (2026-10-14)

S06-preflightb přijat na 2d836336 po čerstvém ověření všech 12 kontrol
PR #25 včetně skutečné CodeQL analýzy a SARIF uploadu, job 115946770877.
Lokální accepted reference je 2d836336; draft se nemerguje do release base.

File Comparator nyní přebírá owned disk selection snapshots místo živých
řádků panelu. Přesné wide identity a názvy určují dvojici; nejednoznačný
target basename zůstane prázdný, chyba služby nevede k odhadu root/name.
Zachováno selected/focused chování, source/target a pořadí left/right.
UTF8 vstupy jsou heap-backed a kapacitně kontrolované před spuštěním threadu.
Produkční hunks a Python test driver odpovídají připnutému donoru;
native template navíc ověřuje 3 mezní případy aktivní snapshot větve.
SDK, CFileData, engine, dialog cleanup, ikonočtečka a Branch gate beze změny.

Host+Filecomp Debug/Release x64 /warnaserror PASS; skutečný command/service
native harness 58 checks/config PASS, celé sady 47 groups/config PASS,
pytest 49 PASS. Původní přijatý consumer negativně 23 failures/51 checks.
Ruční Compare: 11 úspěšných dvojic s přesnými vstupy/skutečnými obsahy,
Unicode jménem/adresáři a oběma dlouhými cestami v D/R, včetně správného
left/right pořadí při pravém zdroji. Všech 10 vlastních hostů i reporterů
normálně ukončeno, žádný nový report, 45 původních fixtures beze změny.
Známé nezměněné ANSI výstupní hlavičky a zkrácený Unicode caption suffix
nejsou označeny jako opravené ani jako plný Unicode UI PASS.

Podrobnosti, přesné příkazy a omezení: [steps/S06.md](steps/S06.md), evidence
mimo Git v evidence/S06-final. PHP a LoadImageW subcase SKIPs zachovány.
Větev integration/pr769-s06-candidate je lokální; nové S06 CI/CodeQL a čistá
publikační platformní matice NOT RUN. Před přijetím/pokračováním S07 nejprve
publikovat S06, Ensure-ForkCI a ověřit přesný head. Tento krok push neopravňuje.

## S06-preflightb — IMPLEMENTED / lokální brána PASS (2026-10-14)

Vlastní S06 Compare migrace narazila při shutdownu dlouhých panelů na původní
přepsání stacku v IconThreadThreadFBody. Podle CONTRACT se provádí jen tento
nejmenší bezpečnostní předkrok: heap-backed narrow/wide scratch, skutečná
kapacita konverze UTF8-first/ACP a wide offset v převedených znacích.
Legacy MAX_PATH guardy pro shell/overlay zůstávají; plná migrace S26/S28
se nepřebírá. SDK, CFileData, diagnostika a tmavé scrollbary beze změny.

Host i obnovený Bug Reporter Debug/Release x64 /warnaserror PASS; Filecomp
pro runtime znovu sestaven z přijatého parentu bez vlastní S06 migrace.
Skutečný preparační blok nativně 93 checks/config PASS, celé sady 46 groups/
config a pytest 49 PASS. PHP a LoadImageW subcase SKIPs zůstávají evidované.
Skutečné běžné panely se zdroji 390/391 UTF8 bajtů (220/221 UTF16 jednotek)
a 407/408 UTF16 jednotek: D/R listing, F3 s přesnou celou cestou/obsahem,
normální shutdown hostu i jeho reporteru, žádný nový report — čtyři PASS.
Všech 45 původních fixture souborů/sentinelů beze změny, bootstrapy obnovené.

Podrobnosti a omezení: [steps/S06-preflightb.md](steps/S06-preflightb.md).
Evidence mimo Git: evidence/S06-preflightb/{local-summary,source-manifest,
gui-observations}.json, runtime.patch, build reporty a JUnit. Připravená S06
migrace/testy jsou hashově uchovány v evidence/S06-owned/pending-S06-owned.patch;
v tomto commitu nejsou a S06 se neoznačuje za dokončené.

Větev integration/pr769-s06-preflightb-candidate je pouze lokální.
Nové fork CI, CodeQL a publikační platformní matice NOT RUN; accepted
reference zůstává bf78c81723abc983805a61acaeec6d92fd4b2316. Další krok:
publikovat tento předkrok, vytvořit/reuse interní draft přes Ensure-ForkCI,
ověřit přesný head a poté obnovit S06 patch včetně obou native registrací.

## S05 — IMPLEMENTED / lokální brána PASS (2026-10-14)

S04 přijat na kódu 359a6b4f po opětovném ověření všech 12 current-head CI
checks včetně skutečného CodeQL. Dokumentační přijetí/accepted reference
`aef114a222f971efefb2029495784bb28b1488aa`; SOURCE a CONTRACT beze změny.
S05 vzniká na lokální `integration/pr769-s05-candidate` nad touto referencí.

Převzat celý spl_diskselection.h, CDiskSelectionService a jeho singleton,
nové samostatné SDK interfaces/constants a built-in QueryService route.
Žádná změna existujících virtual metod nebo CFileData. Owned snapshot kopíruje
cesty i metadata, zachovává pořadí a režimy výběru, vynechává '..', selhává
atomicky a po návratu nepoužívá živé řádky. Capture jen UI thread; immutable
čtení i na workeru; Release v alokujícím modulu. Fallback jen v podporovaném
QueryService-capable Samandarin ABI; starý Open Salamander není podporovaný.
DemoPlug disková DOP ukázka používá snapshot před otevřením progress dialogu;
původní archive example zůstává doslova zachovaný pod diskovým guardem.
Ostatní pluginové operace a Branch state/scanner se nepřebírají.

Donor ABI golden je byte-exact a nezměněný: 5.0-samandarin-0.15.1,
c0cddbe7442fe49abcb859624e271ce687aee0d8. GitHub release listing potvrzuje,
že je stále poslední vydaný tag; golden ověřen přímo proti jeho pěti interfaces
a celému spl_com.h. S04 navíc zachovává celé bodies vydané target base a84dbed6.
Testy SDK/storage/host/singleton a skutečného DemoPlug 55 checks/config PASS
/W4 /WX: 43 zachovaných donor assertions + 12 doplněných DemoPlug kontrol. Pouze PictView
CEnumFiles extrakce a přesně dvě consumer assertions jsou DEFERRED do S12;
žádná jiná donorová assertion nevypuštěna. S04 celý QueryService harness nyní
196/config (původních 193 + 3 nové discovery/version assertions); factory nové
služby je v tomto resolver testu discovery facade, skutečná service/singleton
jsou kompilované v owned harnessu a volané skutečným DemoPlug v GUI.

Host + všech 30 podporovaných SDK konzumentů Debug/Release x64 /warnaserror
PASS (60 consumer builds). Native runner 44 groups/config PASS, pytest 49 PASS.
Historical portables projekt a canonical BUILD_SFTP_PLUGIN=false excluded;
nejsou označené PASS. PHP chybí a LoadImageW long-bitmap backend Win32 2 je
explicitní subcase SKIPPED. Po finálním DemoPlug ABI guardu native runner znovu 44/config PASS
(58,02 / 61,45 s). Hosty po icon opravě přestavěny,
core handle test znovu PASS 132 Debug / 109 Release se stejným bitmap skipem.

GUI objevil baseline CPlugins::CreateIconsList chybu plugins2.cpp:1156:
raw CopyImage, pak HANDLES(DestroyIcon). Funkce v parentu i a84dbed6 shodná.
Nejmenší oprava HANDLES_Q(CopyImage) zachovává tracker i všechny větve.
První vlastní Debug proces 3768 ukončen v diagnostice, není PASS. Finální
Debug fresh 8776/restart 13300, Release fresh 2880/restart 7224: viditelné
panely, fresh F3 obsah/F5 kopie/hash, oba fresh DemoPlug 23 selected DOP files
včetně Unicode a dlouhého názvu, úspěšné deselect a všechny Alt+F4 shutdowny
bez heap/handle dialogu PASS. Kopie 33 bytes, 25 nových a 7 původních fixtures
beze změny. Vlastní bootstrapy obnovené, žádný vlastní GUI proces nezůstal.
Po finálním ABI guardu přestavěn jen DemoPlug D/R a zopakován skutečný
23-file GUI command/shutdown v procesech Debug 10760 / Release 256;
host a ostatní DLL se nezměnily. GUI fixture maximum 250 UTF16 units,
424 UTF8 bytes; skutečná delší UTF16 cesta je v compiled fixtures.
GUI Cancel nebyl doložen: Escape přišel po dokončení a výběr již byl zrušen;
nejde o Cancel PASS. Skutečné SMB I/O ani migrace long-path I/O konzumentů
nejsou S05; DemoPlug stále jen simuluje práci, snapshot není filesystem transakce.

Evidence `C:/Samandarin/codex_pr769_prompts/evidence/S05`: přesné bounded
příkazy, source/donor/ABI manifests, consumer matrix, JUnit, GUI ledger,
runtime payload manifests a originální fixtures. Každý příkaz má parent SHA
a patch hash svého okamžiku; before-menu-fix.patch zachován. Menu fix přidává jen plugins2.cpp; pozdější ABI guard mění pouze DemoPlug
menu a jeho dva testové soubory, zbytek SDK/consumer zdrojových hashů shodný. První host post-build
FAIL kvůli chybějícímu PSModulePath/Get-FileHash vyřešen jen prostředím;
žádná produktová/workflow změna kvůli tomu. Evidence collector opraven pro
extended-length cesty; jeho přípravná selhání nejsou výsledky produktu.

S05 commit je lokální checkpoint; push/PR, nové CodeQL a publikační čistá
Win32/x64/ARM64 matice NOT RUN. Accepted reference zůstává na přijatém S04.
DemoPlug stále deklaruje kompatibilitu se SDK 103: nový diskový case proto
navíc vyžaduje SalamanderVersion >= 105 (QueryService zavedeno v spl_vers.h).
SDK 103/104 použije přesný původní postup bez jediného QueryService volání;
105/106 používá owned službu. Skutečný DOP case se kompiluje v testu proti
progress facade; staré verze, archive, destroy-on-open, Cancel a capture
failure dávají 12 nových assertions. Header zachovává svoji precondition:
volající musí před vstupem zaručit QueryService-capable ABI. Nemění se
verze SDK ani již existující nesouvisející PoC příkazy DemoPlug.

S06 nezačala. Další krok je samostatně autorizovaná publikace této stejné větve,
interní fork-CI draft přes Ensure-ForkCI.ps1 a ověření přesného head výsledku.


## Přijetí S04 — DONE (2026-10-14)

Na pokyn uživatele zahájit S05 přijat S04 checkpoint
`359a6b4f768c4a8769ba6dcb923f9ea663b91aad`. Přesný head interního
[PR #22](https://github.com/majklvi/salamander/pull/22) znovu ověřen:
12 COMPLETED/SUCCESS včetně Win32/x64/ARM64, native/Python, source growth,
guardů a skutečné MSBuild CodeQL analýzy/uploadu. Lokální host + 30 SDK
konzumentů Debug/Release, native 43/config, resolver 193/config, pytest 49
a GUI fresh/restart/F3/F5/shutdown PASS; historické PHP/LoadImageW skips
zachované. Chyba závislosti ABI testu na Git historii vyřešena v 359a6b4f.

Referenční refactor/pr-769-incremental posunut fast-forward přes tento
kód a dokumentační přijetí. Draft se nemerguje do samandarin-0.16;
SOURCE/CONTRACT/release/donor piny beze změny. Další etapa S05, pouze lokální
implementace a validace; nové zveřejnění není tímto zadáním autorizované.
Historické pending záznamy níže popisují dřívější stav.


## S04 CI předkrok — ABI test bez historického checkoutu (2026-10-14)

První current-head CI bfa74f8e: Win32/x64/ARM64 buildy, source growth a Python
PASS, native FAIL pouze `panel_item_paths_tests`. Přesný JUnit traceback:
Git show a84dbed6:src/plugins/shared/spl_gen.h chybí v omezeném CI checkoutu.
Je to chyba nové testové infrastruktury, nikoli potvrzená produktová regrese.
Workflow není měněné a žádná assertion nebyla vypuštěna.

`panel_item_paths_abi.json` nyní drží SHA256 všech pěti vydaných Abstract class
bodies ze spl_gen.h a celého spl_com.h, se source_commit a84dbed6. UTF8/BOM/EOL
normalizace deklarovaná. Normal test porovnává stejné celé definice/layout proti
zmrazené referenci bez Git historie. Explicitní `--verify-abi-source` zvlášť
ověřuje fixture against připnutý release; lokálně PASS včetně seznamu tříd.
Stejné production C++ assertions dál 193/config PASS. Samostatný adresář s
přesnými kopiemi zdrojů a bez .git prošel Debug i Release 193/config; cleanup
vlastního rootu ověřen. Původní parent-query negative control zůstává dostupný.

Produktové/SDK/runner/C++ template bytes z dd344546 se nemění. Buildy všech
konzumentů a GUI se bez runtime změny neopakují; nativní runner nového testu
opakovaný Debug/Release. Nová CI musí ověřit nové SHA, ne Bfa ani dd344546.
S04 nadále PENDING CI/acceptance, accepted reference ea1dd462, S05 nezačala.
Evidence S04: failed-head-test-results/native.xml, publish-ci-03,
ci-fix-focused, ci-fix-historyless/proof, ci-fix-native-*, ci-fix.patch.
Původní testovaný patch zachován jako local-stage-runtime.patch.

## S04 — IMPLEMENTED / lokální brána PASS (2026-10-14)

Kandidát na `integration/pr769-s04-candidate` nad přijatým `ea1dd462`.
Čtyři ordinary disk-row gettery z donorového `branch_view.cpp` přeneseny do
již kompilovaného `fileswn1.cpp`, deklarace a const GetPath z fileswnd.h.
Žádný Branch state/scanner. Donor Join nahrazen existujícím SalPathAppendW;
wide root/name zůstávají přesné, byte fallback volí validní UTF-8 před ACP.
Původní donorový ACP default reprodukoval čtyři neúspěšné assertions u UTF8
root/name bez wide dat; finální adaptace projde při zachování ACP testu.

CPanelItemPathsService/FindCurrentItem/GetItemFullPath a factory převzaty ze
zip.cpp, built-in route přes stávající QueryService. Nová optional SDK třída
CSalamanderPanelItemPathsAbstract, žádná změna vydaných vtables nebo CFileData.
Current array membership se ověří před dereference; explicitní UI-thread,
unsupported archive/FS, capacity/OOM/invalid errors. Výstup nikdy částečný.
Borrowed pointer nesmí přežít refresh/message pumping; kontrola adresy není
záruka proti ABA reuse. Synthetic '..' vrací navigation path root\.. stejně
jako ordinary donor gettery, nejde o rozšíření selection/file-operation API.

Host a všech 30 aktuálně podporovaných SDK konzumentů MSVC v145 Debug/Release
x64 /warnaserror PASS (60 consumer builds). Historical portables variant a
canonical BUILD_SFTP_PLUGIN=false zůstávají excluded, nejsou označeny PASS.
Native runner 43 groups/config PASS; nový whole-production resolver/service/
QueryService harness 193 checks/config /W4 /WX. Existujících 5 SDK Abstract
class bodies spl_gen.h a celý spl_com.h shodné s release a84dbed6.
Parent QueryService control s aktuálními resolvery expected FAIL 5 assertions;
není to build celého parent hostu. Pytest 49 PASS. PHP missing a long-bitmap
LoadImageW backend Win32 2 subcases zůstávají explicitně SKIPPED.

Actual GUI D/R fresh/restart panels, fresh F3 ASCII obsah, fresh F5 kopie a
všechny shutdowny PASS. Kopie SHA256/30 bytes shodné, vlastní nové empty configy,
sedm původních fixtures beze změny a bootstrapy obnovené. V SDK ještě není
migrovaný GUI consumer této služby; konkrétní service dispatch je testovaný
nativním whole-production harness s panel/UI/registry facades. GUI smoke je
běžná panelová regresní kontrola, není vydáván za GUI invocation nové služby.
Real SMB I/O, external/viewer long-path operation migration a Branch metadata
nejsou součástí S04. S05/consumer migration ani S26 scanner nejsou importovány.

Evidence `C:/Samandarin/codex_pr769_prompts/evidence/S04`: bounded commands,
source/runtime/fixture manifests, 60-row consumer matrix, JUnit a GUI ledger.
Původní UTF8 BOM zip.cpp zachován; API source/extracted semantics se nemění.
Kód dd344546 na zadání uživatele publikován do forku na stejné
`integration/pr769-s04-candidate`, interní [draft PR #22](https://github.com/majklvi/salamander/pull/22)
proti samandarin-0.16/a84dbed6. Stav **PENDING current-head CI/acceptance**.
Publikační záznam je dokumentační commit, kód/test bytes beze změny. Jeho nové
CI se váže k aktuálnímu head SHA, nikoli pouze k lokálně ověřenému dd344546.
Poslední accepted reference zůstává ea1dd462; SOURCE/CONTRACT/release/donor
piny nezměněné. Další krok: ověření CI a přijetí S04 před S05.

## Přijetí S03 — DONE (2026-10-14)

Uživatel po ověření CI schválil pokračování a přijetí kódového checkpointu
`770f48e096260a596c4827341cf57490a0e2a37e`. Před přijetím znovu ověřeny přesné
head/base interního [PR #21](https://github.com/majklvi/salamander/pull/21):
12 COMPLETED/SUCCESS checks. Build Win32/x64/ARM64, native/Python, source growth
a squash guard prošly. CodeQL provedlo Release x64 build, vlastní analýzu,
filtr změněných řádků a upload SARIF; nebylo přeskočeno.

Lokální host/ZIP Debug/Release, SafeFile 268/config, ZIP lifetime/dispatch
151 Debug / 138 Release, native 42 groups/config, pytest 49 a skutečné cold
Unicode ZIP extraction/hash/restart/shutdown ověřují totožný kód. Historické
PHP/LoadImageW skips, negativní controls a diagnostické timeouty zůstávají
popsané níže; nejsou překlasifikované na PASS.

S03 a její baseline preflight DONE. Referenční `refactor/pr-769-incremental`
se posouvá fast-forward přes přijatý kód a tento dokumentační záznam. S02b
integration branch, SOURCE/CONTRACT/donor piny a release base se nemění.
Acceptance commit je pouze dokumentace; jeho automatické CI po pushi je
nový běh, oddělený od zelených výsledků přijatého kódu 770f48e0. Build/GUI se
bez změny kódu neopakují. Fork draft #21 zůstává otevřený pro validaci.
Další etapa [S04](steps/S04.md) je plánovaná, v tomto přijetí se neimplementuje.
Evidence: `C:/Samandarin/codex_pr769_prompts/evidence/S03-preflight/acceptance-*`
a `ci-current-codeql-job.stdout.log`, `ci-current-build-run.stdout.log`.

## Historický S03-preflight — lokální runtime brána PASS (2026-10-14)

Opraven potvrzený baseline únik `DupZipUtf8NameW`: hostův legacy SDK copy
nepřebírá `NameW`, takže wide kopii nyní vlastní interní `CZIPFileData` a jeho
stávající release callback. Reject/reset i metadata OOM mají jediný cleanup;
nízkopaměťová větev zachována přes úzké `std::nothrow`/debug-new obnovení.
Při cold archive smoke navíc prokázán AV při předání ZIP listing interface
nově nakonfigurovanému 7-Zip unpackeru. `PackUncompress` používá uloženého
vlastníka stejného panel/archive/interface; ostatní dispatch se nemění.
SDK layout/vtable, 7-Zip vendor kód a původní heap MENU predicate beze změny.

Host i ZIP plugin Debug/Release x64 v145 /warnaserror PASS. Finální native
runner 42 groups/config PASS, SafeFile 268 checks/config, nový skutečný ZIP
listing/dispatch harness 151 Debug / 138 Release checks; pytest 49 PASS.
Parent `919e804d` s totožným harness má expected FAIL (25 failed assertions,
včetně navazujících counter assertions), takže test odhaluje původní chyby.
PHP a long-bitmap LoadImageW subcases zůstávají explicitně SKIPPED.

Skutečné GUI cold ZIP rozbalení v Debug i Release ověřilo čtyři soubory
včetně Unicode názvu, Unicode adresáře a stejných leaf names v různých
adresářích; obsah/velikosti/SHA256 shodné. Restart panely a normální ukončení
PASS oběma konfiguracemi. Finální Debug po odstranění diagnostiky znovu
přestavěn a zavřen bez heap/handle dialogu. Doplňující restart capture ukazuje
pouze známých 16 normal / 512 bytes MENU, predicate known=1, exit 0.
Historický další restart warning nebyl v těchto finálních bězích zopakován;
je zachován jako pozorování, nikoli připsán nové neprokázané příčině.

Evidence `C:/Samandarin/codex_pr769_prompts/evidence/S03-preflight`: přesné
bounded commands, source/runtime/fixture hash manifesty, negativní controls,
PDB crash stack a GUI ledger. Own crash report zachován, žádný report odeslán.
Dva diagnostické 90s deadline běhy nejsou PASS; startup interference po
vlastním crash reporteru je odlišená od produktového AV. Bootstrapy obnoveny,
vlastní procesy ukončeny, sedm původních fixtures a vlastní ZIP beze změny.

S03 kandidát zůstává na `integration/pr769-s03-candidate`, interní draft
[PR #21](https://github.com/majklvi/salamander/pull/21). Předchozí `919e804d`
měl všech 12 checks SUCCESS včetně skutečné CodeQL analýzy; tyto výsledky
nedokazují nové opravné SHA. Po pushi opravy je nutná nová current-head CI.
S03 **není přijatá**, accepted reference zůstává `17678e7c`. SOURCE/CONTRACT
piny se nemění. Žádný upstream PR, merge release base ani S04.

## Historický S03 kandidát — otevřená runtime brána (2026-10-13)

Na výslovné zadání uživatele připraven kandidátní commit a push do forku na
samostatné `integration/pr769-s03-candidate` nad přijatým `17678e7c`, pro
interní fork-CI draft proti `samandarin-0.16`. Commit neznamená přijetí S03;
stav PENDING a baseline runtime blokace níže trvají. Accepted reference a
původní S02b integration branch zůstávají na `17678e7c`, bez merge release base.
Přesné published SHA/PR a zahájené CI ukládá externí `evidence/S03/publish-*`;
výsledek CI se nepředjímá. Žádný upstream PR ani S04.
Nejmenší další krok [S03-preflight](steps/S03-preflight.md).

SafeFile wide create/open/overwrite, recursive directories, Retry/cleanup a
bad_alloc hranice převzaty z připnutého donoru; všechny S01 opravy zachované.
Adaptace resolveru opravuje skutečně reprodukovaný donorový problém krátké
relativní cesty, jejíž absolutní UTF-16 délka překročí MAX_PATH. Donor control
se stejným rozšířeným harness selže právě zde; finální kód projde.

Host Debug/Release x64 v145 /warnaserror PASS. Actual SafeFile driver /W4 /WX:
268 checks/config PASS, včetně Unicode parent/name, UTF8-byte-long parent,
UTF16-long/extended/relative paths, recursive creation, Retry/Skip, overwrite,
read-only, reálného DOS aliasu, ACP fallback a OOM. Vše v owned temp root,
cleanup a sentinel ověřeny. Native runner 41 groups/config a pytest 49 PASS;
PHP a long-bitmap LoadImageW backend subcases nadále explicitně SKIPPED.

GUI panely a F3/F5 ASCII Debug/Release PASS, Debug ZIP extrakce a obsah PASS.
Debug shutdown po archive hlásí heap leak; kontrolní host s parent SafeFile
reprodukuje hlášení již po pouhém otevření stejného ZIPu. Finální Debug restart
s jeho vlastním configem hlásí totéž. Původ alokace dosud neurčen; nejde o
PASS celé runtime brány. Zdroj následně obnoven, finální host přestavěn.
Release fresh/restart shutdown bez této diagnostiky. Externí evidence S03,
code patch SHA256 `F6269765AFE375887DC14272EAD5523701BE62A152362C0805E3DE82BBF4BCBA`.
CODEQL/publikační platformní matice neprovedeny. SOURCE/CONTRACT/SDK/workflows
beze změny; původní accepted S02 checkpointy nejsou změněné.

## Přijatý checkpoint S02b — DONE (2026-10-13)

Uživatel schválil přijetí kódu `23d5406d49eeee40cda9ef19288ff66b6618fd57`.
Při přijetí znovu ověřeno přesné head/base PR #20: 12 COMPLETED/SUCCESS checks,
včetně Debug Win32/x64/ARM64, native/Python, source growth a skutečně dokončené
CodeQL analýzy. Lokální brána: host + 30 supported consumers Debug/Release x64,
41 native groups/config, pytest 49, plné fresh/restart/F3/F5/hash/shutdown
a actual delayed-init recovery v obou konfiguracích. Historické explicitní
PHP/LoadImageW skips a excluded consumers nejsou překlasifikované na PASS.

**S02a i S02b DONE; S02 DONE.** Uzavřené i baseline preflighta/b.
Referenční `refactor/pr-769-incremental` navazuje fast-forward na přijatý kód
a tento dokumentační záznam. SOURCE/CONTRACT a release/donor piny nezměněny.
Dokumentační acceptance commit nemění žádný src/test/build/workflow soubor;
jeho další CI po pushi se vede odděleně od zeleného testovaného kódu 23d5406d.
Evidence `evidence/S02b-preflightb/acceptance-current-head-ci.*` a checkpoint.
Fork draft #20 zůstává otevřený; žádný merge release base ani upstream PR.
Další nejmenší etapa [S03](steps/S03.md); v tomto běhu se neimplementuje.

## Historické S02b-preflightb — lokální oprava před dokončením CI

Parent 20d6018c. Samandarin již nedrží globální native path/version storage;
prázdný SDK tracker nealokuje. Volitelná dvoufázová native exit příprava nejprve
ověří init před odpojením ostatních CLR pluginů, pak při skutečném Unload zavře
callback bránu a uvolní nativní reference. Původní tracker leak diagnostika
proběhne včas, než hostovo rozhraní zanikne; pozdější destructor ji neopakuje.
Příčina AV doložena stackem C__Trace -> C__Handles -> LdrShutdownProcess.

Host a 30 podporovaných pluginů Debug/Release x64 PASS; native 41 groups/config,
plugin wide 426 Debug/89 Release, shutdown 26/config, pytest 49. PHP a nativní
LoadImageW long-bitmap subcase SKIPPED. Metadata Unicode/UTF8-byte-long/UTF16-long
fixture běhy PASS oběma konfiguracemi. Plný GUI cold/restart/F3/F5/hash/shutdown
oddělen od debugger controls. Skutečný init worker pozdržen jen ve vlastních
Debug/Release procesech: odmítnutý close zachoval pluginy, následný init result 1,
retry exit 0 bez AV. Timeout diagnostického helperu není počítán jako PASS.

První pozdní hook dovolil unload jiných CLR pluginů před odmítnutím close;
prokázaná chyba opravena readiness fází před UnloadAll loop. Historické logy
zůstávají v evidence/S02b-preflightb. Žádné odeslané crash reporty nebo změny
uživatelského nastavení. Nové SHA musí projít fork draft #20; jeho parent
20d6018c měl všech 12 viditelných checks SUCCESS včetně CodeQL. To není CI
nového commitu. S02b není DONE, S03 nezačala; accepted reference stále ef19d788.
Podrobnosti [S02b-preflightb](steps/S02b-preflightb.md) a VALIDATION.md.

## Historické S02b-preflighta — kandidát, plná runtime brána BLOCKED

2026-10-13. Nad zveřejněným `207a8a57` opravena tři lokální icon ownership
místa Service Exploreru: EnsureServiceImageList, CreateServiceBitmap a
SetServiceExplorerIconList nyní párují HANDLES(LoadImage) s HANDLES(DestroyIcon).
Ikony předávané hostiteli přes GetFSIcon/GetPluginIcon mají dál původní externí
vlastnictví. Nejde o další donorový import ani změnu společného SDK.

Service Explorer Debug/Release x64 /warnaserror PASS; host Debug/Release PASS.
Běžné GUI oběma: první panely, F3 správný ASCII obsah a F5 hash PASS.
Release normální shutdown PASS. Debug icon diagnostics odstraněné, ale při
shutdown stále heap dialog; celý S02b-preflight ani S02b nejsou DONE.

Oddělený diagnostický host používá existující TRACE_TO_FILE přes compiler env,
bez změny produktu či vypnutí heap/tracker kontrol. Dump: 4126 B / 21 normal
blocks, z toho 3200 B SDK tracker allocation a zbývající nativní storage
konzistentní s managed bridge Update Notifieru + známých 512 B MENU noise.
Samandarin SPL a CLR zůstaly při heap bráně načtené; log doložil přeskočení
shutdown callbacku v plugins1.cpp. Stejný běh bez jediné vlastní kopie tohoto
SPL skončil bez heap dialogu a s Closing log file. Tento izolující control
není PASS plné sestavy; plugin byl poté obnoven.

Samandarin bridge/shutdown host source jsou beze změny proti TARGET_SHA;
SDK tracker constructor/destructor jsou beze změny proti parentu S02a.
Přesný parent runtime znovu nesestaven; tvrzení o baseline vychází z této
source parity, načteného modulu a A/B izolace, nikoli z vymyšleného parent PASS.
Nejmenší další krok [S02b-preflightb](steps/S02b-preflightb.md).

Fork PR #20 pro `207a8a57`: všechny 12 viditelné check runs SUCCESS včetně
CodeQL (dva běhy squash guard po editaci popisu); není to CI nového patch/SHA.
Oprava ikon je samostatný PENDING kandidát na integrační větvi. Referenční
accepted větev zůstává `ef19d788`; SOURCE/CONTRACT piny nezměněny, S03 nezačala.
Evidence `evidence/S02b-preflight`: bounded build JSON/logs, GUI launches,
TRACE_TO_FILE logy, A/B výsledek a source/fixture hash vazba. Původních 7 fixtures
nedotčeno, bootstrapy obnoveny, vlastní diagnostické pomocníky ukončeny.
## Historický kandidát S02b 207a8a57 — PENDING/BLOCKED

2026-10-13. Poslední přijatý parent `ef19d788c1a2807e134a2ea9ba3aa9f1e1ac253d` zůstává na
`refactor/pr-769-incremental`. Uživatelem autorizovaný PENDING kandidátní commit
je na nové publikační větvi `integration/pr769-s02b-candidate`; není přijetím S02b.
Interní fork-CI draft slouží k ověření dosavadního integračního obsahu.
Přesné kandidátní SHA a URL jsou svázané s testovanými hashi v externí evidenci.
Převzaty EXACT mhandles.h/.cpp: CreateFileA/W, FindFirstFileA/W a best-effort
UTF-8 diagnostika se zachováním GetLastError. Nový full-production tracker test,
řízené diagnostic allocation/conversion faults, ANSI/UNICODE probe a fixtures
vyplňují donorovou testovací mezeru; fixture ledger adaptovaný ze S02a/donoru.

PASS 60 plugin buildů (30 podporovaných konzumentů, Debug/Release x64), host
Debug/Release, Portables language peers, native 40 skupin/config (plugin
214 Debug / 89 Release; S01 99/config), pytest 49. Negativní parent compile
úmyslně FAIL. Release GUI panely/F3/F5/hash/shutdown PASS; Debug panely/F3/F5
PASS, ale startup Service Explorer tracker diagnostics a shutdown heap leaks
**FAIL čisté runtime brány**. Bug Reporter byl viditelný; jeho čtení CU mělo
approval timeout, žádný report odeslán. Žádné varování ani heap check vypnuté.

Dvě GUI kopie shodné hashe, všech 7 originálů nedotčeno, own configstorage
bootstrapy obnoveny. První GUI pokusy skončily v chybějícím language modulu
Portables: samostatný CPP projekt nemá language ProjectReference; opraveno
explicitním buildem existujícího lang_portables, bez změny produktu. Debug
icon mismatch je doložen v nezměněném Service Explorer kódu; heap původ zatím
neurčen. Další krok [S02b-preflight](steps/S02b-preflight.md), nikoli S03.

Evidence `evidence/S02b`; runtime patch `962125d93e6f7f4f27c715da91c962d513cf8101d8ea1069261e496874b506e4`,
code/test patch `772b2b635af915e68202cf6289defaf01faf24337d082f68023c4afe8a91f85d`, raw source hashe a 67 finálních úspěšných command
výsledků + zvlášť neúspěšné přípravné pokusy. Piny SOURCE/CONTRACT nezměněny.
PHP a S02a LoadImageW-long backend subcase SKIPPED; CodeQL/plná publish matice
dosud bez výsledku pro kandidáta. S02 nadále PARTIAL.

## Historický přijatý checkpoint S02a

2026-10-13, **S02a DONE — core LoadImage/FindFirstChangeNotification A/W**.
Deklarace a implementace převzaté EXACT z připnutého donoru. Core CreateFileA/W,
FindFirstFileA/W a widepath už baseline měla; znovu se nepřidávají. Nový cílený
nativní test vyplňuje chybějící přímé ověření donoru; fixture ownership/cleanup
adaptovaný z donorového SafeFile harnessu. SDK a pluginový kód beze změny.

Lokální brána PASS: host/Salmon Debug/Release x64, Translator/Trace Server
Debug/Release Win32; native 39 skupin/config, core handles Debug 132 a Release
109 kontrol bez selhání, stávající SafeFile 99/config, pytest 49/49. GUI oba
fresh starty: okamžitě oba panely, F3 správný ASCII obsah, F5 shodný hash kopie,
normální shutdown bez handle diagnostiky; všech 7 originálů nezměněno.
Negative control proti parent core úmyslně nekompiluje chybějící A/W členy.

Parent `2c1480e064d5fd8cca0174b02ae5503411701b7b`; runtime patch `6154561f61dc71387b5582b03ab0825752409a5b2bb8903637f5e5f77e661e61`;
code/test patch `e36fe9d1859e04a81faaa5a4615ba856c307ad7e170bcbdd6ef259679760827c`. Evidence `evidence/S02a` obsahuje raw source hashe,
11 úspěšných finálních příkazů, GUI/runtime manifesty a vazbu finálního commitu.
Commit najít trailerem `Port-Step: S02a`; SOURCE a CONTRACT piny beze změny.

PHP process-runtime subcase SKIPPED. Jedna bitmapová long-path backend subcase
v každé konfiguraci SKIPPED: samotné LoadImageW vrací Win32 error 2, ačkoli
přesný soubor existuje. Wrapper parity/no-registration při selhání PASS;
notifikace stejného UTF-16-long adresáře PASS. S02a CodeQL a plná čistá
publikační platformní/plugin matice NOT RUN. Žádný push/PR tohoto portu.

S02 celkem **PARTIAL**; nejbližší [S02b](steps/S02b.md). S03–S32 PENDING.
Branch View zůstává nedostupné. Samostatné baseline PR #774/#775 a jejich
fork-CI výsledky nejsou validací S02a a nemění donor/release piny.

## Historický checkpoint S01

2026-10-12, **S01 DONE — SafeFile Retry/Skip**. Pět donorových oprav,
adaptovaný nativní ASCII harness a jeho registrace. Debug/Release x64 host
PASS; native 38 skupin v každé konfiguraci (SafeFile 99/99), pytest 49/49,
GUI fresh panels/viewer/copy/shutdown oběma PASS. Negativní kontrola parentu
7/99 FAIL potvrzuje detekci původních chyb. S02 nezačala a je další nejmenší
etapou. S01 plná publish matice/pluginy/CodeQL NOT RUN; PHP subcase SKIPPED.

Parent `87b8afae7a8c2a934527468130bb7e7a2bac7e5c`; runtime patch SHA256
`470A2BB28DC7E587D99D0CE9804F5AF978A0DA7A1233DCA5F20DE79E550BE815`. Code/test/runner patch
`092530462D9C3CA59CCC27CE3361A3BDECA5CDE51FA06FE25C5C23D41529D150`; evidence `evidence/S01`.
Výsledné SHA najít podle traileru `Port-Step: S01`; externí checkpoint váže
finální commit na hashe právě ověřených souborů. Piny SOURCE zůstávají původní.

Samostatné baseline opravy jsou upstream PR #774/#775 a fork CI #18/#19
(14/14 SUCCESS včetně CodeQL na stejných SHA). Portová větev S01 zůstává
lokální; další push/PR není v tomto zadání. Její baseline fixy už jsou přítomné
v historii, neaplikovat je znovu ani automaticky měnit release/donor piny.

## Historický checkpoint S00c

2026-10-12, **S00c DONE — viditelné panely při prvním startu**. Lokální
Debug/Release x64 baseline PASS v deklarovaném rozsahu; první empty file-backed
config, restart, běžné panely, viewer, copy a shutdown ověřeny. S01 nezačala a je
další nejmenší etapou. Plná publikační matice/CodeQL zůstávají NOT RUN.
S00 commit `75950e887e34ec4bdbe1eb71aa6c04b0c3bfaa8d`, S00a
`0772e7d228a6fc5d587c8def3418f744a9ebb9a1`, S00b
`216e79d1e4f6f56ba1da7a8297906b1da9796df0`. S00c najít podle traileru;
vlastní výsledné SHA se zde neuvádí. Žádný donorový runtime hunk nepřevzat.

## Checkpoint

- Větev: `refactor/pr-769-incremental`.
- Pracovní worktree: `C:/Samandarin/codex_pr769_prompts/work`.
- Read-only donor worktree: `C:/Samandarin/codex_pr769_prompts/donor`, detached.
- DONOR_SHA: `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413` (beze změny vůči vodítku).
- PR_BASE_SHA: `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449`.
- DONOR_BASE_SHA: `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449`.
- TARGET_SHA: `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449` (skutečná upstream base).
- Úplné zdroje/obnova: [SOURCE.json](SOURCE.json). Devět donor-only commitů,
  žádný target-only commit ani cherry ekvivalent po merge-base.
- Všechny existující worktrees byly při inventuře bez tracked/untracked změn.
  Jejich větve a soubory zůstaly zachovány; přibyly jen dvě nové worktrees
  a lokální piny. Origin je fork, upstream remote se jmenuje `samandarin`.

## Stav etap

| Etapa | Stav | Doklad / důvod |
| --- | --- | --- |
| S00 zdroje/smlouva/plán/mapa | DONE | Piny SOURCE, CONTRACT a původních 393 donorových řádků beze změny |
| S00a prostředí/evidence | DONE | v145, pinned submodules, isolated pytest/lizard; historie v VALIDATION |
| S00b tracking/cleanup | DONE | 9 úzkých source změn; nové baseline REPLACED řádky, žádný import PR #769 |
| Build/native/Python | PASS provedeného rozsahu S05 | Host/30 consumers Debug/Release x64, 44 native groups/config, pytest 49; PHP/LoadImageW subcases SKIPPED |
| Normal-panel GUI Debug/Release | PASS s initialized private config | F3 ASCII, F5 copy/hash, 7 originálů beze změny, normální shutdown bez Debug handle diagnostiky |
| První empty private config | PASS Debug/Release | Po S00c oba panely viditelné ihned bez zásahu; F3/F5 a hash kontrola |
| S00c | DONE | WM_CREATE visibility až po přiřazení obou aktuálních panelů; první start i restart ověřeny |
| S01 | DONE | Pět úzkých SafeFile oprav, compiled harness 99/99 v obou konfiguracích, native/host/GUI brána PASS |
| S02a | DONE | Core A/W EXACT; host/core consumers/native/Python/GUI brána PASS |
| S02b | DONE | Kód 23d5406d, full local runtime PASS, přesných 12 fork CI checks SUCCESS |
| S02b-preflighta/b | DONE | Icon ownership, native storage/diagnostic lifetime a init readiness ověřeny |
| S02 celkem | DONE | Obě podetapy přijaté; další předpoklad S03 splněn |
| S03 | DONE | Přijatý checkpoint ea1dd462 |
| S04 | DONE | Ověřený 359a6b4f + přijetí aef114a2 |
| S05 | DONE | 89698944, interní draft #23, všech 13 current-head checks SUCCESS včetně skutečného CodeQL |
| S06-preflight | DONE | e0b69771, interní draft #24: 13 current-head checks SUCCESS včetně skutečného CodeQL |
| S06 | READY | Vlastní File Comparator owned-selection migrace; předpoklady S05 a S06-preflight přijaty |
| S07–S32 | PENDING | Nezahájené; Branch View nadále nedostupné |
| Publikace | S05 fork draft #23 otevřený | Přesný head 89698944: 13/13 SUCCESS; validační drafty se nemergují do reference base |

## Rozsah S00b

`CIconList::ReplaceIcon`: existing HANDLES_Q(CopyImage), registrace owned
GetIconInfo bitmap před všemi cleanup větvemi. `GetIconPixelWidth` totéž.
Externí shell/extractor ikony zachovávají netrackovanou návratovou smlouvu;
NOHANDLES cleanup temporary a stale-DPI ikon odpovídá stávajícím větvím.
Dva startup vlastníci zapisují převzatý HICON do trackeru. Hlavní okno při
WM_DESTROY uvolňuje owned DPI ikony — CWindow se odpojuje před WM_NCDESTROY.
Z normal-panel smoke ještě opraven tracked brush/raw delete pár a dvojí
registrace wide copy handle. Nezměněny path/DPI/cache/fallback chování,
SDK/hlavičky, workflow, legacy data a resources. Nový kód je doložená oprava
baseline, nikoli donorový port; podrobnosti v [S00b](steps/S00b.md).

## Historické výsledky S00b a revize

Testovaný parent `0772e7d228a6fc5d587c8def3418f744a9ebb9a1` plus runtime patch
SHA256 `EB4E10DC6D1448AB23D66252E2DF9C6F23C807B111D7246EA33BAD53F5A35017`.
Runtime base stále TARGET_SHA, plus zde popsaný nezávislý fix. Final buildy:
Debug 10,45 s, Release 67,59 s; native 52,53 / 60,78 s, pytest 6,48 s.
Finální Debug PID 7072 a Release PID 5880, přesné vlastní exe cesty a nové
copy cíle. Debug zavřen normálně bez startup/cleanup/leak dialogu.
PHP subcase SKIPPED; JUnit 37 skupin s failures=0 neznamená PHP PASS.
Manifesty 469 Debug / 485 Release souborů bez Intermediate; convert payload
hashově shodný s targetem. Helpers z vlastní S00a sestavy, bez donor binárek.
Žádný nově přidaný test nenahrazuje reálné runtime ověření; použity existující
native/contract/CI sady a skutečné GUI. Plná publish matice a CodeQL NOT RUN.
Ruční long/Unicode viewer v S00b NOT RUN; tyto fixtures zůstaly beze změny.

Evidence: `C:/Samandarin/codex_pr769_prompts/evidence/S00b`, přesné příkazy a
historie neúspěšných přípravných pokusů v [VALIDATION.md](VALIDATION.md).
S00/S00a historické výsledky zachovány v původních evidence adresářích.

## Skutečné výsledky S00c

Testovaný parent `216e79d1e4f6f56ba1da7a8297906b1da9796df0` plus runtime patch
SHA256 `18AC1A9A8078543763711991D4FF637DED17365EB00A900BED04C543D0FED47F`.
Pouze WM_CREATE v mainwnd3.cpp: dva visibility calls za přiřazením panelů,
+4/-2 řádky. AddPanelTab neaktivuje nový tab; staré pořadí oba skrylo a restart
je znovu aktivoval při LoadConfig. Konkrétní původní chyba na finálním S00b
zopakována; žádný donorový import ani změna config/path/theme smluv.

Debug/Release host build PASS (15,46 / 60,84 s), native 37/37 každá konfigurace
(59,80 / 53,10 s), pytest 49/49 (7,55 s), PHP subcase SKIPPED. Čtyři skutečné
GUI procesy: Debug Fresh 15376 / Restart 9352, Release Fresh 9296 / Restart
16136. Oba první configy nové a nulové délky; panely viditelné při prvním
pozorování, F3 správný ASCII obsah, F5 shodné kopie. Restart oběma ověřil panely
a shutdown; viewer/copy na restartu neopakovány. Vše normálně zavřeno bez Debug
handle/leak dialogu, sedm originálů včetně sentinelu beze změny. Obnoveny pouze
vlastní předchozí runtime bootstrapy, skutečné uživatelské nastavení nedotčeno.

Evidence `C:/Samandarin/codex_pr769_prompts/evidence/S00c`, nové runtime manifesty
471 Debug / 486 Release souborů včetně vlastních mutable config fixtures.
Binárky odpovídají launch hashům, convert payload shodný s targetem. Piny SOURCE
beze změny, původních 393 donorových DEFERRED řádků zachováno, přibyl samostatný
baseline REPLACED řádek S00c. Podrobnosti [VALIDATION.md](VALIDATION.md).

## S05 — publikováno a přijato (2026-10-14)

Kód `896989447ad410c4970d469d9f961c62e5b7a8dc` je pushnut na
`origin/integration/pr769-s05-candidate`. Interní draft
[majklvi/salamander#23](https://github.com/majklvi/salamander/pull/23) proti
nezměněné release base `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449` zůstává otevřený;
neslouží k merge do samandarin-0.16. Všech 13 kontrol přesného headu SUCCESS:
čistá Win32/x64/ARM64 matice /warnaserror, VSIX build/test, native/Python,
reporty, growth a guardy, CodeQL gate i skutečná analýza. CodeQL job
115823621238 dokončen 2026-10-14 11:32:24 UTC, včetně Initialize, Release
MSBuild, analýzy a SARIF uploadu. Žádný z těchto checks není pending/skipped.
Detailní lokální výsledky a jejich PHP/LoadImageW subcase SKIPs níže zůstávají
v platnosti; zelený CI job je nepřejmenovává na PASS.

Evidence `evidence/S05/publish-final-ci.json`, `publish-final-codeql-job.json`
a `publish-summary.json`. Dokumentační acceptance nemění testovaný runtime.
Další nejmenší etapa S06: File Comparator; read-only baseline audit zatím
odhalil Debug heap hlášení už při otevření/zrušení původního Compare dialogu.
Před migrací izolovat příčinu, nevypínat diagnostiku a neskrývat ji změnou
heap ignore predikátu. SOURCE.json, CONTRACT.md a release piny beze změny.

## S06-preflight — lokální brána PASS (2026-10-14)

Opraven pouze lifecycle drop callbacku dvou File Comparator vstupů.
Podrobnosti, původ chyby, příkazy, omezení a další krok:
[steps/S06-preflight.md](steps/S06-preflight.md).
D/R host+plugin /warnaserror, 964 native lifecycle checks/config,
45 regresních groups/config, pytest 49 a skutečný Compare/Cancel/shutdown
PASS. Původní callback je v negativní kontrole prokazatelně vadný.
Předkrok je lokální; S06 owned-selection migrace PENDING. Před ní publikovat
předkrok a ověřit CI jeho přesného headu. Accepted reference ffa1519d (S05).
## S06-preflight — publikováno a přijato (2026-10-14)

Kód e0b69771bbaf683d7aa3debcf158d2efec65d93c na
origin/integration/pr769-s06-preflight-candidate je ověřen interním
[draftem #24](https://github.com/majklvi/salamander/pull/24). Všech 13 kontrol
přesného headu SUCCESS: Win32/x64/ARM64 /warnaserror, VSIX build/test,
native/Python/reporty, growth, guardy a skutečné CodeQL. Job 115856740584
dokončen 12:53:45 UTC; Initialize, Release MSBuild, analýza i SARIF upload
SUCCESS. Lokální PHP/LoadImageW subcase SKIPs zůstávají pravdivě evidované.
Draft zůstává otevřený a nemerguje se do reference base a84dbed6.
Evidence: evidence/S06/publish-final-ci.json a publish-final-codeql-job.json.
Předpoklad S06 splněn; pokračovat pouze vlastní owned-selection migrací.
Dokumentační acceptance nemění testovaný runtime ani SOURCE/CONTRACT piny.
