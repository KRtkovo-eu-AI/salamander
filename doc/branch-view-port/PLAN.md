# Plán postupného portu PR #769

Stav k 2026-10-12: S00/S00a/S00b/S00c a **S01 dokončeny**.
S01 lokální Debug/Release x64 host/native/GUI brána PASS; SafeFile harness
99 kontrol v každé konfiguraci, native 38 skupin a pytest 49/49. Nejbližší je
[S02](steps/S02.md), v tomto běhu nezačala. Nový S01 není publikovaný; jeho
plná platformní/plugin matice a CodeQL NOT RUN. Samostatné baseline PR #774/#775
mají úspěšnou fork CI pro vlastní head SHA a neposouvají piny SOURCE.
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
| [S02](steps/S02.md) | Wide utility a Debug sledování handleů | S01; pouze potřebné společné pomocné funkce |
| [S03](steps/S03.md) | SafeFile Unicode a dlouhé cesty | S01, S02 |
| [S04](steps/S04.md) | Cesty položek běžného diskového panelu | S02; existující QueryService |
| [S05](steps/S05.md) | Owned SDK, host service a DemoPlug společně | S04; neměnné vydané ABI |
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
