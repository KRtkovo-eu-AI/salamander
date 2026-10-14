# Stav portu PR #769

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
| Build/native/Python | PASS provedeného rozsahu S02b | Host/30 consumers Debug/Release x64, 41 native groups/config, pytest 49; PHP/LoadImageW subcases SKIPPED |
| Normal-panel GUI Debug/Release | PASS s initialized private config | F3 ASCII, F5 copy/hash, 7 originálů beze změny, normální shutdown bez Debug handle diagnostiky |
| První empty private config | PASS Debug/Release | Po S00c oba panely viditelné ihned bez zásahu; F3/F5 a hash kontrola |
| S00c | DONE | WM_CREATE visibility až po přiřazení obou aktuálních panelů; první start i restart ověřeny |
| S01 | DONE | Pět úzkých SafeFile oprav, compiled harness 99/99 v obou konfiguracích, native/host/GUI brána PASS |
| S02a | DONE | Core A/W EXACT; host/core consumers/native/Python/GUI brána PASS |
| S02b | DONE | Kód 23d5406d, full local runtime PASS, přesných 12 fork CI checks SUCCESS |
| S02b-preflighta/b | DONE | Icon ownership, native storage/diagnostic lifetime a init readiness ověřeny |
| S02 celkem | DONE | Obě podetapy přijaté; další předpoklad S03 splněn |
| S03–S32 | PENDING | Nezahájené; Branch View nadále nedostupné |
| Publikace | Fork integration draft #20 | Kód S02b ověřen; upstream funkční PR dosud nepodané, draft se nemerguje do reference base |

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

## Další nejmenší krok

Ověřit aktuální head interního fork draftu #22: platformní buildy, native/Python
a skutečnou CodeQL analýzu; vyřešit případné change-caused failures. Teprve
poté přijmout S04 a začít S05. Lokální brána PASS, accepted reference ea1dd462.
Draft nemergovat do referenční release base.
