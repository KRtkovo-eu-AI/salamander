# Stav portu PR #769

## Aktuální checkpoint S01

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
| S02–S32 | PENDING | Pořadí a podetapy v PLAN; Branch View nadále nedostupné |
| Publikace | S01 lokální, bez push/PR | Baseline opravy oddělené v upstream #774/#775; jejich fork CI zelená, bez změny portové větve |

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

V samostatném běhu provést právě [S02](steps/S02.md): minimální wide Win32
hranice a Debug A/W handle wrappery podle připnutého donoru. Znovu použít
stávající path helpery; zachovat SDK ABI, tracking a new makra. Žádný scanner
nebo plošná pluginová migrace. Před převzetím ověřit konkrétní konzumenty,
provést příslušnou Debug/Release/native/GUI bránu a zastavit po S02.
S01 neautorizuje publikaci portové větve ani automatický posun do S02.
