# Plán postupného portu PR #769

## S12-preflight — doložená mezera existujícího WIC backendu (2026-10-22)

S12 candidate runtime e22210d1 uložen mimo Git a reverzně odložen, aby se
předkrok sestavil/otestoval samostatně proti S11 45d4a3e7. GUI Update Thumbnail
nad vlastními validními deep Unicode JPEGy skončil jako non-JPEG, žádný soubor
nezměněn. Skutečný Release backend přes přesné PDB function symbols vrací
format=PVF_BMP; nezávislé WIC čtení téhož souboru s extended prefixem rozpozná
JPEG a skutečné CollectFrames/DecodeFrame uspějí. Celý WicBackend.cpp blob
je ef72188c53a6be5a713841475f654c96b102885d v HEAD, releasebase i donoru.

Konkrétní příčina prefixu: AddExtendedPrefixIfNeeded čeká na SAL_MAX_PATH,
ikoli Win32 MAX_PATH, takže validní 415-unit cesta nemá potřebný prefix a
WIC se zbytečně nahrazuje Shell bitmapou. Druhá prokázaná mezera:
sPVSaveImage vrací PVC_UNSUP_OUT_PARAMS pro NULL filename a ignoruje existující
PVSF_USERDEFINED_OUTPUT callback. Regenerace tak nemůže fungovat ani s JPEG.

Nejmenší předkrok: správný Win32 prefix threshold; stávající WIC SaveFrame
zpřístupnit přes IStream z memory při callback output, zachovat původní file
save a encoder/resize/metadata implementaci. Žádný nový image engine, žádná
viewer enumeration/S18. Ověřit skutečný staged backend, validní JPEG output,
callback error/short writes a normální save; host/PictView D/R /warnaserror,
native regrese a skutečný GUI smoke obyčejné operace. Pak samostatný commit
předkroku, obnovit přesně uložený S12 patch a znovu jeho lokální bránu.
Dřívější S12 candidate testy nejsou důkazem hotového předkroku.

## S12 — plán paired PictView thumbnail/EXIF (2026-10-22)

Navazuje na lokálně ověřenou S11 45d4a3e7. Převzít celý donor thumbs.cpp
(CEnumFiles owned snapshot, temporary/attributes RAII, safe replace a wide
EXIF volání), exif.c/.h/.def jako jeden párovaný krok. Projekt přidá pouze
spl_diskselection.h; unicodetitle.h a PictView navigation hunks zůstávají S18.
Převzít oba pictview_thumbnail_paths_tests; obnovit skutečný CEnumFiles a dva
S05 odložené HostTests assertions bez ztráty DemoPlug/ABI rozšíření targetu.
Native harness musí načíst skutečnou právě sestavenou DLL s wide i legacy
exportem, ověřit Unicode/long I/O, původní pixels/EXIF a bezpečnou náhradu.
Driver přidá pouze configuration selector pro bounded runner, zachová default
Both; runner zachová všech 52 starších skupin. Žádný nový image engine.

Gate: host+PictView+EXIF D/R /warnaserror, skutečná DLL, native/pytest regrese,
GUI Regenerate Thumbnail nad vlastními JPEG fixtures a kontrola obsahu,
originálů, temp cleanup a právě načtené EXIF DLL. Pro dostupný open WIC backend
oddělit skutečný runtime od proprietárního enginu; chybějící pokrytí nepočítat
jako PASS. Žádná S18 viewer-enumeration integrace ani publikace této etapy.

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

## S11 — plán shared sides/context (2026-10-22)

S10 přijato na 60987d42. Převzít pouze tři donorové Salamatrix soubory:
salamatrix_packages.cpp, salamatrix_sides.h a vcxproj/salamatrix.vcxproj.
JSON serializace přímo z wide owned snapshots; native ItemInfo ABI zůstává
pevné a překročení vrací explicitní chybu. Schema a 64-item context limit
beze změny. Pět providerů zůstává forwarding stejného shared host method.
Aktivovat celý odložený selection_snapshot_tests --scope All v runneru;
doplnit mezeru escaping/úplného JSON a CopyResult kapacity/protocol limitu
při kompilaci skutečných produkčních metod, bez jejich duplikování.

Lokální gate: host+Salamatrix D/R x64 /warnaserror, všechny aktuální native
regrese a Python; skutečný smoke změněné operace nad vlastní fixture.
Pět forwarding source contracts není běh pěti runtime prostředí: doložit
skutečně dostupné runtimes a jejich výsledky odděleně. SDK/IDL/CI se nemění.
Žádný push/PR; po lokální bráně pokračovat dalším zadaným krokem do S20.

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

## S10 — plán Automation (2026-10-22)

S09 přijato na f13b3ed1; lokální integration/pr769-s10-candidate. Převzít
8 produkčních souborů Automation v úzkém donor diffu a matching testy.
COM item vlastní wide Name/Path, collection sdílí owned disk snapshot s
každým enumerátorem, Reset neopakuje capture, focused getter vytváří vlastní
item. Zachovat legacy archive/FS a virtuální .., COM ABI/IDL a kapacitní
pravidla focused script launch. Žádné změny Salamatrix/SDK/host/CI/Branch.

Nově zjištěná testová vazba: selection_snapshot_tests.py extrahuje
SidesService::CopyDiskItemInfo a GetSelectedItem/GetFocusedItem ze
salamatrix_sides.h, SideItemJson wide overload a DiskSideContextJson ze
salamatrix_packages.cpp. Tyto symboly patří do S11, v targetu zatím chybí.
Samotná produkční Automation migrace tuto závislost nemá. Podle CONTRACT
odložit pouze TestSides/TestDynamicJsonContext + 5 provider forwarding
assertions do S11; zachovat je v donorovém template/All režimu. S10 test driver
získá explicitní scope Automation/All, default Automation. Převzatý
TestAutomation neoslabit. Nejmenší doplnění mezery: nativně kompilovat i
skutečné collection/enumerator metody + CUnknownImpl, ověřit Next/Reset,
nezávislé enumerátory, uvolnění collection a COM item/snapshot cleanup.

Validační brána: host+Automation Debug/Release x64 /warnaserror, focused
native a celé regrese, skutečný Automation script přes plugin v D/R nad
vlastními fixtures. Zachované collections/items/enumerator musí přežít
DeselectAll, refresh/sort a změnu panel path i nové focused getters; úplné
Unicode/UTF8-long/UTF16-long properties, žádné borrowed rows. Nepoužívat
Salamatrix transport jako náhradu Automation COM. Vlastní skripty pouze
čtou metadata a zapisují test report mimo originály; 71 předchozích originálů
ověřit hashově. Publikační CI/CodeQL/full build S10 až po dalším zadání.

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

## S09 — plán Split & Combine (2026-10-22)

Přijatý checkpoint S08 68b66c33; lokální integration/pr769-s09-candidate.
Úzký donor diff má 12 souborů výhradně src/plugins/splitcbn: owned input
selection/plan, skutečný parent pro výchozí cíle a sibling discovery,
wide I/O/dialogs, UTF8 BAT, RAII/progress cleanup a input identity guards.
Ponechat zdroje otevřené bez write/delete sharing a porovnat volume/file ID
před vytvořením cíle; zachovat i ochranu proti target-alias race. Převzít
celý nový splitcbn_paths.h a matching selection_paths_tests.py/.cpp.in.
Driver podporuje --configuration; zaregistrovat jen nový test v existujícím
runneru, ponechat všech 50 stávajících skupin. Bez host/SDK/Branch/CI změn.

Ověřit native actual consumer Debug/Release, host+plugin /warnaserror v obou
konfiguracích a celé regresní sady. Poté reálný GUI Split/Combine a nezávislé
porovnání obsahu/pořadí dílů nad vlastními ASCII/Unicode/UTF8-long/UTF16-long
fixtures; UTF8 BAT metadata, implicitní target a discovery, Cancel, stejné
input/output a hardlink bezpečně odmítnuty. Native mixed-parent/race fixture
není náhradou GUI. Zachovat všech 66 předchozích originálů a vlastní vstupy.
Nepodporovanou podmínku vykázat jako SKIP, ne PASS; důkazy parent+patch+runtime.
S09 bez publikace; čistý full build/platformní CI/CodeQL až při autorizovaném
publikačním checkpointu. S10 v tomto běhu nezačínat.

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

## S08 — plán úzké migrace Checksum (2026-10-14)

Předpoklady S05 a S07 ověřeny; accepted reference dec9c828. Úzký diff:
9 produkčních souborů checksum (včetně nového checksum_paths.h) a 2 testové
soubory. Přenést owned seed/FullPath před modeless threadem, oddělené
relative display/export jméno, wide rekurzi/I/O/export a Verify podle
DirectoryW skutečného manifestu. Zachovat algoritmy, formát manifestu,
nastavení/dialog resources a stávající disk-only guard. Bez SDK/Branch UI.
Native harness kompiluje reálné commands/service/worker/parser/export;
CHashAlgo digest je však stub. Proto vedle native běhu ověřit skutečný
plugin Calculate + Save + Verify proti nezávislým CRC/SHA hashi nad vlastními
ASCII/Unicode/UTF8-long/UTF16-long soubory i více Unicode komponentami.
Export v témže i jiném adresáři, Cancel beze změn, modeless panel refresh/
změna selection po capture; mixed-parent duplicitní basename jen native.
Driver checksum_selection_paths_tests.py nemá --configuration a sám vždy
běží Debug i Release; registrace do runneru jej nebude volat s neexistujícím
přepínačem. Před commitem host+Checksum D/R /warnaserror, všechny regrese,
skutečný GUI smoke, exact zdrojové/runtime hashe a nedotčenost starších
55 fixtures. Do další etapy ani publikace se v tomto běhu nepokračuje.

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

## S07-CI — oprava závislostí ručního full buildu (2026-10-14)

Samostatný workflow_dispatch run 38609591197 na S07 3f9a0134 selhal ve
Win32/x64/ARM64: Lua runtime a mmviewer TagLib nebyly připraveny. Select
changed projects se při dispatch nespouští, takže chyběly needs_lua a
needs_mmviewer_taglib outputs a instalační krok byl přeskočen. Nejmenší
oprava: při workflow_dispatch spustit tentýž instalační krok s oběma
příznaky true; PR výběr i oddělený statický TagLib install root zachovat.
Nejde o fork-specific aktivaci workflow; opravuje se společná ruční cesta.
Aplikační kód, SDK, kontroly /warnaserror, SFTP a reference beze změny.
Ověření: existující Python workflow testy, nový čistý build všech tří
architektur a skutečné dokončení CodeQL na novém přesném headu PR #27.
Do dokončení těchto běhů publikační brána zůstává PENDING.

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

## S07 — plán úzké migrace Batch Rename (2026-10-14)

S06 přijat na kódu 9580b153 po 12 checks včetně skutečného CodeQL; lokální
accepted reference f43b9d86 (jen acceptance dokumentace). S07 provádí pouze
14 souvisejících produkčních souborů renamer včetně dvou path/I/O headers a
čtyři donorové testové soubory podle mapy. Existující disk-only menu guard
MENU_EVENT_DISK zůstává; nová podpora archivů/FS ani Branch UI se nepřidává.

Přenést společně snapshot intake před worker threadem, owned CSourceFile,
per-item rodiče, relative/full mode, preview, wide rename/move I/O a Undo.
Kapacita 4*SAL_MAX_PATH na haldě, Win32 extended cesty a ne-throwing source
factory včetně všech donorových failure větví. Bez nového SDK/scanneru.
Native panel harness zkompiluje skutečný intake/konstruktory/generování cíle,
I/O harness skutečné wide wrappers nad disposable fixtures. Oba D/R.
Povinná GUI brána: Cancel beze změny souborů, preview/sort/refresh životnost,
skutečné přejmenování a Undo s různými obsahy stejných basename ve dvou
rodičích, Unicode jméno/rodiče a obě délkové hranice. Native wrapper samotný
není důkazem skutečného GUI Undo. Zdrojový donorový payload basename-derived
u duplicate FileCase sám nedokazuje rozlišení obsahů; tuto mezeru pokryjí
odlišné souběžné vlastní fixture markery. Běžný panel/shutdown D/R, host+
renamer /warnaserror, regresní sady, přesné hashe a obnova bootstrapů.
Do accepted reference S07 povýšit až po samostatném publikování a CI headu.

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

## S06 — obnovená migrace po přijetí S06-preflightb (2026-10-14)

PR #25 head 2d836336 má čerstvě ověřeno všech 12 kontrol SUCCESS včetně
skutečné CodeQL analýzy a SARIF uploadu (job 115946770877). Přijatá lokální
reference je 2d836336; interní draft zůstává otevřený a release base se nemění.
Uložený S06-owned patch byl ověřen hashem a obnoven nad touto opravou.
Tři produkční Filecomp soubory a native Python driver odpovídají přesně
připnutému donoru. Native template zachovává tři doplněné kontroly kapacity;
runner nyní obsahuje oba testy, ikonočtečku i Filecomp owned consumer.

V tomto běhu pouze S06: cílený skutečný command/service harness a negativní
kontrola přijatého starého consumeru; host+Filecomp Debug/Release x64,
celé regresní sady a skutečné Compare I/O se selected/focused/source/target,
Unicode názvem, více Unicode komponentami a oběma dlouhými cestami.
Ukončení musí zahrnout host i reporter a nepřítomnost nového reportu.
Nepřebírat Branch UI, SDK ani pozdější icon/overlay migraci. Známé nezměněné
výstupní ANSI hlavičky Filecomp a dark dialog starých reportů evidovat
samostatně; netvrdit jejich opravu. Evidence: evidence/S06-final.

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

## S06-preflightb — bezpečný scratch ikonočtečky běžného panelu

S06 migrace je připravena a native 58 checks/config, celé sady 46/config
prošly; ruční Compare načte Unicode i dlouhé cesty. GUI brána však při
shutdownu selhala: RTC Stack memory corruption, reporty vlastních procesů
2552/15196 a původní IconThreadThreadFBody v src/fileswn1.cpp:1824–1842.
memmove kopíruje celý GetPath() do char path[MAX_PATH+10], poté zapisuje
wPath přes wName=wPath+l, kde l je počet bajtů cesty. 390/391 UTF8 a
407/408 UTF16 cesty prokazatelně překračují scratch. Donor (pozdější S26/S28) nahrazuje
narrow scratch CPathBuffer(4*SAL_MAX_PATH+16), ale celý worker navíc závisí
na BranchView cache keys a novém overlay API, které S06 nezavádí.

Nejmenší bezpečnostní předkrok: převzít pouze donorový heap narrow scratch,
heap wide scratch SAL_MAX_PATH+16 (panel Path má 32768 bajtů), skutečnou
kapacitu konverze a wide offset podle počtu převedených znaků, UTF8-first/ACP. Zachovat stávající MAX_PATH guardy
legacy shell/overlay operací; nepředstírat hotovou long-icon/Branch migraci.
Ověřit skutečný preparační blok nativně v D/R, hranice a canary integrity,
oba host buildy, regresní sady a skutečný dlouhý panel/shutdown v D/R.
Původní stack/heap diagnostika se nevypíná. Až samostatně tento předkrok
uzavřít; S06 migrace je kompletně uchována v evidence/S06-owned/
pending-S06-owned.patch s manifestem a výsledky. Do accepted reference
se nepřijímá, její kód ani testy v tomto předkroku nejsou přítomny.

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

## S06-preflight — lokální brána PASS (2026-10-14)

Opraven pouze lifecycle drop callbacku dvou File Comparator vstupů.
Podrobnosti, původ chyby, příkazy, omezení a další krok:
[steps/S06-preflight.md](steps/S06-preflight.md).
D/R host+plugin /warnaserror, 964 native lifecycle checks/config,
45 regresních groups/config, pytest 49 a skutečný Compare/Cancel/shutdown
PASS. Původní callback je v negativní kontrole prokazatelně vadný.
Předkrok je lokální; S06 owned-selection migrace PENDING. Před ní publikovat
předkrok a ověřit CI jeho přesného headu. Accepted reference ffa1519d (S05).

## S06-preflight — izolace výchozího heap hlášení

Před převzetím ExecuteCompare v src/plugins/filecomp/filecomp.cpp baseline
89698944 skutečně zobrazí správné dva rozdíly, ale při shutdownu Debug
hlásí heap leak. Samotné otevření a Cancel původního Compare dialogu stačí:
CRT dump 26 normal blocks / 928 bytes, včetně dvou COMBOBOX skupin a MENU.
To není donorová migrace ani důkaz OS-owned úniku. Nejmenší předkrok:
zachytit alokační call stack konkrétních neuklizených bloků; případně opravit
jen prokázaného vlastníka a ověřit D/R host, dotčený plugin, native/GUI.
Dočasná diagnostika musí být před finálními buildy beze zbytku odstraněna.
IsKnownSystemMenuLeakDiff se nesmí rozšířit a heap tracker se nevypíná.
V tomto běhu nejprve tento předkrok; migrace S06 až po jeho uzavření.

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

## Minimální S05 předkrok — menu icon registration

GUI otevření Plugins v Debug S05 zastavilo na handles.cpp:864,
plugins2.cpp:1156, DestroyIcon: Handle not found. CPlugins::CreateIconsList
vytváří sizedIcon raw CopyImage, ale uvolňuje jej HANDLES(DestroyIcon).
Celá funkce je shodná v parentu a84dbed6 i přijatém S04; nový DiskSelection
se při tvorbě menu nevolá. Nejmenší oprava: registrovat návrat CopyImage
existujícím HANDLES_Q, stejně jako ostatní již přijaté icon fixes. Žádné
vypnutí trackeru, změna velikosti, vlastnictví či fallbacku. Po opravě host
Debug/Release, nativní tracker kontrola a skutečné Plugins/DemoPlug/shutdown.
První vlastní GUI proces byl ukončen v diagnostickém dialogu; není GUI PASS.

## Historie kandidátů před přijetím S02b

Aktuální stav 2026-10-13: S02b-preflighta opravuje lokální tracker páry ikon
Service Exploreru; Debug/Release build a GUI view/copy PASS. Plný Debug shutdown
stále BLOCKED kvůli Samandarin Update Notifieru, izolováno A/B kontrolou.
S02b není přijatá. Nejbližší [S02b-preflightb](steps/S02b-preflightb.md): nativní
shutdown a životnost resources CLR-přidržovaného pluginu, bez potlačení kontrol.
Publikovaný integrační kandidát může vyvolat CI; S03 ani upstream PR nezačínají.

Historický stav kandidáta 207a8a57: poslední přijatý checkpoint **S02a** (`ef19d788`).
**S02b PENDING/BLOCKED**: donorové pluginové wrappery a přímé native testy
připravené, host + 30 podporovaných konzumentů Debug/Release x64 a regresní
sady PASS. Debug GUI odhalilo nezměněný Service Explorer icon ownership
mismatch a heap leak při ukončení; runtime brána není PASS; kandidátní commit slouží pouze k autorizovanému CI.
Další nejmenší krok [S02b-preflight](steps/S02b-preflight.md); nejprve vyřešit
nebo doloženě izolovat tato zjištění, teprve pak přijmout S02b. S02 PARTIAL,
S03 nezačala. Publikační větev `integration/pr769-s02b-candidate` uchovává dosavadní
integrační historii a PENDING kandidáta; interní fork-CI draft není upstream PR.
CodeQL a publikační platformní matice čekají na výsledky pro přesné SHA.
Referenční pracovní větev `refactor/pr-769-incremental` zůstává na přijatém S02a.
Rozhodující zdroje jsou
[CONTRACT.md](CONTRACT.md), [SOURCE.json](SOURCE.json), [PROGRESS.md](PROGRESS.md)
a [VALIDATION.md](VALIDATION.md). Výsledky původního PR nejsou výsledky portu.
Před každým krokem ověřit větev, aktuální diff, skutečné závislosti a baseline.
Při blokované validační bráně nepokračovat do další implementační etapy.

## Zdroje a hranice

- Pracovní větev `refactor/pr-769-incremental` vznikla z aktuálního tipu
  `release/5.0-samandarin-0.16`, nikoli z donorové větve.
- Donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413` má proti targetu
  `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449` devět commitů a 231 změněných souborů.
  PR_BASE_SHA, DONOR_BASE_SHA a TARGET_SHA se nyní shodují; význam zůstává odlišný.
- Target je předkem donoru, bez vlastních commitů po merge-base; `git cherry`
  vrací devět `+`. Není zde dodatečný target commit, který by tento delta-port
  už obsahoval. Existující společná infrastruktura targetu se nepřenáší znovu.
- Úplný binární patch a statistiky jsou mimo Git v `../../../evidence/source/`
  vzhledem k této složce. Přesné absolutní cesty a SHA256 obsahuje SOURCE.json.
- Výchozí pořadí pochází z dodané sady `C:/Samandarin/codex_pr769_prompts`.
  Každý odkaz níže drží rozsah, donorové oblasti, testy, acceptance a PR hranici.
  [REUSE.tsv](REUSE.tsv) pokrývá všechny donorové soubory; u sdílených souborů
  je třeba před extrakcí zpřesnit symboly/hunks, ne převzít celý soubor.
- S00 nemění runtime, SDK, resources, workflow ani build/test kód.

## Pořadí, závislosti a navržené PR hranice

Řádky jsou plán, nikoli tvrzení o hotové implementaci. Číselné pořadí je
výchozí provozní pořadí i tehdy, když některé kroky nemají přímou závislost.
Jedna etapa/podetapa znamená jeden koherentní checkpoint a samostatný
reviewovatelný PR, pokud níže není uvedena pevně svázaná dvojice.

| Etapa | Rozsah a PR hranice | Minimální předpoklady |
| --- | --- | --- |
| S00a | Obnova v145, pinned submodules, izolované testové prostředí a baseline evidence; DONE — bez změny runtime | S00; uživatelem autorizovaná instalace |
| [S00b](steps/S00b.md) | Baseline tracking a cleanup ikon; DONE, včetně souvisejících evidence chyb ze smoke | S00a; vlastní reprodukce Debug startup/shutdown |
| [S00c](steps/S00c.md) | Viditelnost obou prvních panelů po přiřazení aktuálních panelů; DONE | S00b; první start a restart Debug/Release ověřeny |
| [S01](steps/S01.md) | Izolované SafeFile Retry/Skip opravy | Ověřená baseline; rozdělit donorové testy podle skutečných závislostí |
| [S02](steps/S02.md) | Wide utility a Debug sledování handleů; S02a/S02b DONE | S01; pouze potřebné společné pomocné funkce |
| [S03](steps/S03.md) | SafeFile Unicode a dlouhé cesty; DONE včetně baseline preflight | S01, S02 |
| [S04](steps/S04.md) | Cesty položek běžného diskového panelu; DONE | S02; existující QueryService |
| [S05](steps/S05.md) | Owned SDK, host service a DemoPlug společně; local PASS | S04; neměnné vydané ABI |
| [S06](steps/S06.md) | File Comparator | S05 |
| [S07](steps/S07.md) | Batch Rename | S03, S05 |
| [S08](steps/S08.md) | Checksum | S03, S05 |
| [S09](steps/S09.md) | Split & Combine | S03, S05; identity souborů |
| [S10](steps/S10.md) | Automation COM snapshot | S05 |
| [S11](steps/S11.md) | Salamatrix sides/context | S05; všech pět providerů |
| [S12](steps/S12.md) | PictView thumbnail + spárovaná EXIF DLL | S02, S05; jedna kompatibilní dvojice |
| [S13](steps/S13.md) | ZIP menu operace nad archivem | S05 |
| [S14](steps/S14.md) | 7-Zip Test Archive | S05 |
| [S15](steps/S15.md) | PAK Optimize | S05; existující input |
| [S16](steps/S16.md) | Undelete restore/image boundary | S03, S05 |
| [S17](steps/S17.md) | Host getters a disk-size | S04, S05 |
| [S18](steps/S18.md) | a: launch; b: ViewerEnumeration + PictView navigation | S02, S04, S17; služba a konzument v jednom checkpointu |
| [S19](steps/S19.md) | Worker wide cesty, copy/Retry/ADS | S02, S03; beze změny scheduling |
| [S20](steps/S20.md) | a: rename + temporary selection; b: copy/move; c: delete | S04, S17, S19; každý krok s Cancel/failure |
| [S21](steps/S21.md) | Shell, kontextové menu a clipboard | S04, S17, S20; lze rozdělit helpers a call sites |
| [S22](steps/S22.md) | Host archive callback | S04, S05, S17; zachovat ABI root + relative |
| [S23](steps/S23.md) | Array reserve | Samostatná podpora; všichni konzumenti hlavičky |
| [S24](steps/S24.md) | Typed message expander | Samostatná podpora; resources až s volajícími |
| [S25](steps/S25.md) | Scanner bez UI | S02; cancellation/ownership již součást |
| [S26](steps/S26.md) | Gated panel, read-only a default-off | S04, S05, S17, S23–S25; identity/lifetime i před S28 |
| [S27](steps/S27.md) | Aktivace integrací po operacích | S26 a odpovídající S06–S22, vždy skutečný Branch smoke |
| [S28](steps/S28.md) | Async vlastnosti, ikony, thumbnail, preparation | S26; žádné odkládání již potřebných stale/lifetime ochran |
| [S29](steps/S29.md) | Taby, historie, focus a výběr | S26, příslušné S27; restore respektuje gate |
| [S30](steps/S30.md) | Refresh/debounce a zbylé optimalizace | S26, S28, S29; vlastní měření |
| [S31](steps/S31.md) | Audit úplnosti a COVERAGE.md | Všechny předchozí přijaté kroky |
| [S32](steps/S32.md) | Veřejná aktivace a dokumentace | S31 a vyřešené mezery, plný build/native/GUI |

S27 se provádí po jedné podetapě: **a** viewer/edit/Enter/Open Containing Folder;
**b** rename/temporary selection; **c** copy/move/delete včetně flat/relative;
**d** Shell/clipboard/Properties/recycle; **e** ZIP/7-Zip host packing;
**f** každý již převzatý plug-in samostatně. Velké c či f lze dále rozdělit
před editací podle jednotlivých ověřitelných příkazů. S27d Shell Properties
neznamená asynchronní sloupce Explorer properties ze S28. S29 pouze doplní
obnovu historie/focus k příkazu Open Containing Folder zavedenému v S27a.

## Podetapy S02 (audit před editací, 2026-10-12)

S02a i S02b dokončeny 2026-10-13. Následující rozdělení zachovává původní audit.
S02 má dvě nezávislé skupiny Debug wrapperů se značně odlišnou množinou
konzumentů. V jednom běhu provést pouze první podetapu:

1. **S02a — core LoadImage/FindFirstChangeNotification A/W.** Převzít páry
   deklarací a implementací z common/handles.h/.cpp. Core CreateFileA/W a
   FindFirstFileA/W i společné widepath helpery už baseline má; nepřidávat
   druhé implementace. Ověřit host a všechny core consumers: salamand,
   salmon, translator a tserver, Debug/Release na jejich podporované lokální
   platformě (x64 host/salmon, Win32-only translator/tserver), nativní test
   skutečného trackeru, A/W+legacy macro aliasů a vlastních Unicode/long fixtures,
   existující native/Python sady a běžné GUI. Chybějící přímý donorový wrapper
   test je doložená testovací mezera, doplnit cílené nativní ověření.
2. **S02b — plugin CreateFile/FindFirstFile A/W.** Až v dalším běhu převzít
   mhandles.h/.cpp včetně best-effort UTF-8 diagnostiky a zachování GetLastError;
   ověřit všechny podporované konzumenty této sdílené pluginové hlavičky i
   chybovou/low-memory diagnostickou větev. Bez pluginových I/O migrací.

Teprve po obou bránách je S02 DONE a lze zahájit S03. Donor, release base,
SDK virtual layout, obecná path knihovna a aktivace Branch View se nemění.

## Předkrok S02b-preflight (zjištěn při GUI, 2026-10-13)

Rozšířená vlastní sestava pluginů odhalila dvě Debug chyby `DestroyIcon: Handle
not found` v Service Exploreru (fs1.cpp:48 a plugin.cpp:226). Tyto soubory se
v S02b nemění: raw LoadImage a tracked DestroyIcon jsou už v parentu. Po Alt+F4
se objeví `Heap Message: Detected memory leaks!` a vlastní Bug Reporter;
původ heap leak dosud neurčen. Nelze jej označit za baseline ani regresi bez
reprodukce/izolace. Rozsah a acceptance [S02b-preflight](steps/S02b-preflight.md).
Do něj nevtahovat I/O migrace, SDK či Branch View; S03 nezačínat.

## Pravidla pro zjištěné předkroky

Rozsah S00c po reprodukci na finálním S00b: `CMainWindow::WindowProc`, WM_CREATE
v `src/mainwnd3.cpp`, volá `UpdatePanelTabVisibility(cpsLeft/cpsRight)` ještě
před přiřazením `LeftPanel`/`RightPanel`. `AddPanelTab` nyní záměrně pouze vytváří
tab a neaktivuje jej; visibility porovnává s dosud NULL aktuálním panelem a
obě okna skryje. Běžný WM_SIZE mění jejich geometrii bez SWP_SHOWWINDOW.
Uložení/restart chybu maskuje následným restore/activation tabů v LoadConfig.
Nejmenší oprava: přesunout obě visibility aktualizace až za obě přiřazení,
zachovat AddPanelTab smlouvu a config/theme/path chování. Žádný donorový import.
Ověřit nový empty file-backed config i restart v Debug/Release, viewer/copy,
neporušené ostatní soubory, native panel/tab regresní sadu a oba host buildy.

Historický důvod dokončené S00b: S00a odblokovala buildy a automatické testy,
Debug host však před opravou při startu
hlásí `C__Handles::CheckClose` (`src/common/handles.cpp:864`), volaný z
`CIconList::ReplaceIcon` (`src/iconlist.cpp:483/485`): `DeleteObject` handle
nenalezen. `GetIconInfo` vytváří bitmapy mimo tracker, jejich uvolnění jde
přes `HANDLES`; ověřit také vlastnictví výsledku `CopyImage` a cleanup BAIL.
`iconlist.cpp` je mezi targetem a připnutým donorem totožný, donor opravu
neobsahuje. S00b proto představuje doloženou opravu výchozího produktu,
žádný port wide infrastruktury ze S02. Sledování handleů se nesmí vypnout.
Samostatně zopakovat pozorování prázdných panelů při prvním spuštění s novým
prázdným souborovým configem; po materializaci a restartu Release fungoval.
Příčina tohoto pozorování není určena. Další etapu zahájit až po ověření
Debug/Release běžných panelů a zápisu přesného config/runtime původu.

Základní transport `CSalamanderGeneralAbstract::QueryService` je již v targetu
(`src/plugins/shared/spl_gen.h`); nevkládat jej znovu do veřejného vtable.
Nové služby a jejich vlastnictví se extrahují atomicky s implementací.
Pokud úzký donorový hunk používá funkci z budoucí etapy, zapsat přesný symbol,
call site, nejmenší potřebný rozsah a test. Teprve potom provést samotný
předkrok; nepřevzít celý `branch_view.cpp` ani import skrytý přepínačem.

S01 musí zůstat oprava Retry/Skip na současných cestách. Wide část stejného
donorového testu patří S03 a musí být výslovně DEFERRED, dokud nemá implementaci.
S23/S24 lze předsunout pouze jako takto doložený minimální předkrok. Bez
takového dokladu zůstává původní pořadí. Konkrétní výsledky auditu hranic
jsou v [BOUNDARIES.md](BOUNDARIES.md).

## Publikace a pravidla autorů

S00 je lokální. S32 znamená veřejné zapnutí produktu, samo o sobě nedává souhlas
k pushi/PR. Publikaci řídí samostatné zadání `control/92-publish-next-pr.md`.
Před ním provést `control/90-audit-history.md` a `control/91-prepare-prs.md`.
Pro časný základní PR vytvořit dílčí coverage jeho hranice; netvrdit, že tím
proběhl globální audit S31. První vadný checkpoint zastavuje audit dalších.

Pracovní historie zůstane lineární na `refactor/pr-769-incremental`. Review
větve nesou jednotlivé funkční změny; přesně stejná review větev se použije
pro fork-CI i upstream PR. Stacked PR jen s vhodnou base a skutečným oprávněním;
jinak postupné PR po začlenění závislosti. Žádný automatický merge/force-push.
Po squash merge se závislost ověřuje i obsahem, nikoli jen ancestry.

Ověřený `pr-squash-guard.yml` omezuje PR autorů `Lemi257` a `bellus869` na
nejvýše jeden commit. `auto-label-author.yml` jim přiřazuje `comments translation`;
`pr-comments-guard.yml` při tomto labelu ověřuje pouze komentáře ve strict
i ignore-whitespace režimu. Přihlášený účet ověřený `gh api user --jq .login`
je `majklvi`; to není důkaz merge/admin práv. Před publikací ověřit tehdejší
pravidla a skutečného autora znovu. Žádný guard se nemění ani nevypíná.

Po prvním autorizovaném pushi nové review větve místní `C:/Samandarin/AGENTS.md`
vyžaduje okamžitý interní draft proti `majklvi/salamander:samandarin-0.16` přes
`C:/Samandarin/tools/Ensure-ForkCI.ps1`. Je to validační draft vedle případného
produktového upstream draftu z control/92. Helper znovu používá existující PR;
validační draft zůstává otevřený a nemerguje se. Upstream PR až po výsledcích
pro přesné aktuální head SHA; pending/skipped/unavailable uvést pravdivě.

Obecná doporučení RUNBOOK.md k main, globálnímu rerere, mergům nebo obnově
historie nejsou pro tento port pokynem k jejich provedení. Zde platí konkrétní
zadání nové větve z TARGET_SHA, lineární historie a zákaz globálních změn.

S06-preflight zúžen po alokačním trace: konkrétní neuklizené ComboBoxData a
ThemeData vznikají ve File Comparator darkmodelib. DragDropEditProc hledá
již odpojený rodičovský CCompareFilesDialog a pro pozdní zprávy vrátí NULL
místo předání dalším procedurám. Převést pouze tuto obsluhu dvou comboboxů
na SetWindowSubclass/DefSubclassProc s odpojením při WM_NCDESTROY; odstranit
OldEditProc1/2. Přidat native lifecycle test skutečného callbacku, D/R plugin
+ host a opakovaný skutečný Compare/Cancel/shutdown. Sdílená SDK, theme
knihovna ani heap ignore predikát se nemění. Původních osm host MENU skupin
je starší samostatná baseline záležitost; trace jejich alokace ukazuje také
na app-owned ThemeData, dosavadní komentář o OS-owned původu proto není
potvrzen. Tento předkrok opravuje pouze dvě File Comparator pole.
