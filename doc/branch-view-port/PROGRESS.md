# Stav portu PR #769

2026-10-12, předkrok **S00a**. Obnova prostředí a záznam baseline dokončeny;
**runtime baseline BLOCKED — Debug GUI startup**, nikoli chybějící v145.
S00 je commit `75950e887e34ec4bdbe1eb71aa6c04b0c3bfaa8d`. Žádná implementace
Branch View nebyla převzata a nebyla zahájena S01. Dokumentační follow-up
identifikovat podle traileru `Port-Step: S00a`; vlastní výsledné SHA se zde
neuvádí. Nejbližší krok je [S00b](steps/S00b.md).

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
| S00 zdroje, smlouva a plán | DONE — dokumentace | CONTRACT, SOURCE, PLAN, 32 stage dokumentů, BOUNDARIES, FIXTURES |
| S00 mapa převzetí | DONE — mapování | REUSE: všech 231 donorových souborů; 393 řádků hranic, všechny DEFERRED |
| S00a prostředí a evidence | DONE — dokumentace | v145 nainstalováno; pinned submodules; izolovaný pytest/lizard; logy a runtime manifesty |
| Baseline build/native/Python | PASS v provedeném rozsahu | Debug/Release x64 host + helpers; 37/37 native skupin každá konfigurace; pytest 49/49; PHP subcase SKIPPED |
| Baseline Release GUI | PASS s inicializovaným vlastním configem | ASCII i Unicode viewer; F5 kopie, hash a 7 nezměněných zdrojů; první prázdný config měl nevykreslené panely |
| Baseline Debug GUI | FAIL, baseline brána BLOCKED | DeleteObject handle not found, iconlist.cpp:483/485, handles.cpp:864; výchozí kód shodný s donorem |
| S00b | PLANNED | Nejmenší oprava baseline ikonových handleů a nové GUI ověření |
| S01 | PENDING, vstupní brána BLOCKED | Po S00b a přijetí baseline pouze izolované SafeFile opravy |
| S02–S26 | PENDING | Postupné předpoklady v PLAN a jednotlivých steps dokumentech |
| S27a–e, S27f.01–11 | PENDING | Aktivace jednotlivých ověřených příkazů/plug-inů; ne jeden monolitický krok |
| S28–S30 | PENDING | Experimentální gate zůstává; correctness neodkládat |
| S31 | PENDING | Budoucí úplný hunk/ABI/resource/test audit; COVERAGE zatím nevzniklo |
| S32 | PENDING | Veřejné zapnutí až po úplné validaci; samo neautorizuje push |
| Push / interní či upstream PR | NOT REQUESTED / NOT RUN | Lokální dokumentační checkpoint |

## Skutečné výsledky baseline

Testovaný checkout je S00 `75950e887e34ec4bdbe1eb71aa6c04b0c3bfaa8d`;
runtime zdroje jsou přesně TARGET_SHA, rozdíl je pouze dokumentace.

- VS 2026 Community 18.10.3, MSBuild 18.10.1.42706, MSVC 14.51.36231 (v145),
  SDK 10.0.26100.0. SFTP/HardView inicializovány na připnutých revizích.
- MSBuild Debug/Release x64 host, english.slg a salmon/salopen/salspawn PASS
  (exit 0). Oddělený runtime `work/build/pr769-baseline-v145/salamander/`.
- Native runner Debug i Release: 37 skupin, 0 selhání v každé konfiguraci.
  PHP process-runtime subcase SKIPPED kvůli chybějícímu php.exe, přestože JUnit
  skupinové skipped=0. Native conversion-table prošel oběma konfiguracemi.
- CI pytest výběr: 49 PASS, vlastní venv s pytest 9.1.1 a lizard 1.23.0.
- Release GUI: F3 ASCII i česká/CJK cesta správného obsahu, F5 copy; hash kopie
  shodný, 7 originálů včetně sentinelu beze změny. První prázdný file-backed
  config měl bílé panelové plochy i po refresh/resize; uložení a restart je
  odblokoval. Příčina pozorování není určena.
- Debug GUI: startup diagnostiky `DeleteObject: Handle ... not found` v
  `iconlist.cpp:483/485`; běžné panely/operace nebyly přijaty. Pouze vlastní
  zablokovaný testovací proces ukončen, uživatelské instance zachovány.
- Runtime SHA256 manifesty: 469 Debug / 485 Release souborů bez Intermediate;
  convert data bajtově odpovídají targetu, .tab mají 256 bajtů.
- Plná čistá publikační matice Win32/x64/ARM64, všechny plug-iny/dependency DLL,
  vzdálené CI a CodeQL: NOT RUN. Toolchain probes nejsou tato matice.
  Donorové I/O testy a ruční dlouhé cesty: DEFERRED/NOT RUN.
- Přesné příkazy, rozlišení evidence a runtime staging: [VALIDATION.md](VALIDATION.md).
  Nové logy, JUnit, GUI report, fixture a runtime manifesty:
  `C:/Samandarin/codex_pr769_prompts/evidence/baseline-v145`.
  Historická S00 evidence zůstává v `evidence/baseline`: wrapper exit 0 bez
  buildu, native 20 PASS/17 FAIL, pytest BLOCKED, unittest subset 41 PASS.

## Další nejmenší krok

1. Obnovit kontext pouze z těchto dokumentů a ověřit HEAD/dirty stav. Piny
   nepřepočítávat podle živého PR; hotové S00 znovu neaplikovat.
2. Provést právě [S00b](steps/S00b.md), opravit baseline Debug ikonové tracking
   hranice a zopakovat Debug/Release GUI. Nevypínat tracker a nepřevzít S02.
   Prověřit také první start s prázdným vlastním configem. Zastavit po S00b.
3. V pozdějším běhu po přijetí baseline vykonat právě [S01](steps/S01.md): Read/Write silent Skip
   assignment, Retry `OPEN_EXISTING`, failure vracející `INVALID_HANDLE_VALUE`.
   Zachovat dnešní path boundary. Adaptovat pouze ASCII část skutečného donor
   SafeFile harnessu; `SafeFileRootLength` a wide scénáře výslovně odložit S03.
   Chybějící Write Skip/Skip All assertion doplnit v S01, ne vydávat za donor PASS.
4. Před kódovým commitem provést jeho Debug/Release/native/GUI bránu,
   aktualizovat REUSE/PROGRESS a přidat provenance trailery. Zastavit po S01.

## Rozsah dokumentačního commitu

Pouze `doc/branch-view-port/`. Žádný nový adaptační runtime kód, žádné změny
SDK, projektů, resources nebo workflow. Úplný donor patch, pomocné auditní
skripty, logy, build outputy a fixtures nejsou v commitu. CONTRACT je bajtová
kopie uživatelem dodaného COMMON.md. Před commitem zkontrolovat celý staged
diff, `git diff --cached --check`, úplnost mapy i vlastní Python cache.

Commit message:

```text
docs: record v145 baseline recovery and GUI blocker

Source-PR: KRtkovo-eu-AI/salamander#769
Source-Commit: 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413
Port-Step: S00a
```
