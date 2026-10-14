# Plán postupného portu PR #769

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
