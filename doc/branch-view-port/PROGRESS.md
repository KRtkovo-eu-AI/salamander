# Stav portu PR #769

## Aktuální S02b-preflightb — lokální oprava, current-head CI PENDING

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

## Poslední přijatý checkpoint S02a

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
| Build/native/Python | PASS provedeného rozsahu | Debug/Release x64 host/resources, 37/37 native skupin každá konfigurace, pytest 49/49; PHP subcase SKIPPED |
| Normal-panel GUI Debug/Release | PASS s initialized private config | F3 ASCII, F5 copy/hash, 7 originálů beze změny, normální shutdown bez Debug handle diagnostiky |
| První empty private config | PASS Debug/Release | Po S00c oba panely viditelné ihned bez zásahu; F3/F5 a hash kontrola |
| S00c | DONE | WM_CREATE visibility až po přiřazení obou aktuálních panelů; první start i restart ověřeny |
| S01 | DONE | Pět úzkých SafeFile oprav, compiled harness 99/99 v obou konfiguracích, native/host/GUI brána PASS |
| S02a | DONE | Core A/W EXACT; host/core consumers/native/Python/GUI brána PASS |
| S02b | PREPARED/BLOCKED | Build/native PASS; čisté Debug GUI startup/shutdown FAIL |
| S02b-preflight | PENDING | Service Explorer ownership + identifikace Debug heap leak |
| S03–S32 | PENDING | S02 PARTIAL; Branch View nadále nedostupné |
| Publikace | S02a lokální, bez push/PR | Baseline opravy oddělené v upstream #774/#775; jejich fork CI zelená, bez změny portové větve |

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

Provést [S02b-preflight](steps/S02b-preflight.md) pro zjištěné Debug startup/
shutdown překážky. Zachovat necommitovaný S02b patch; žádný reset, stash nebo
přepis pinu. Po reprodukci/izolaci a opravě opakovat jen dotčené testy, sestavení
a Debug/Release GUI; S02b přijmout až po úspěšné runtime bráně. S03 nezačínat.
