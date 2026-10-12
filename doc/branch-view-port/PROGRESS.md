# Stav portu PR #769

2026-10-12, etapa **S00**. Dokumentační bootstrap dokončen jako jediný lokální
commit; **runtime baseline BLOCKED**. Žádná implementace Branch View nebyla
převzata a nebyla zahájena S01. Commit S00 identifikovat v historii podle
traileru `Port-Step: S00`; dokument nemůže obsahovat vlastní výsledné SHA.

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
| S00 baseline build/native/GUI | BLOCKED | Chybí v145, některé testové dependencies/submodules, target runtime a GUI ověření |
| S01 | PENDING, vstupní brána BLOCKED | Nejprve doplnit baseline; potom pouze izolované SafeFile opravy |
| S02–S26 | PENDING | Postupné předpoklady v PLAN a jednotlivých steps dokumentech |
| S27a–e, S27f.01–11 | PENDING | Aktivace jednotlivých ověřených příkazů/plug-inů; ne jeden monolitický krok |
| S28–S30 | PENDING | Experimentální gate zůstává; correctness neodkládat |
| S31 | PENDING | Budoucí úplný hunk/ABI/resource/test audit; COVERAGE zatím nevzniklo |
| S32 | PENDING | Veřejné zapnutí až po úplné validaci; samo neautorizuje push |
| Push / interní či upstream PR | NOT REQUESTED / NOT RUN | Tento běh je lokální bootstrap |

## Skutečné výsledky baseline

Testované runtime zdroje jsou přesně TARGET_SHA; změny S00 jsou jen docs.

- Debug a Release x64 `build.ps1`: BLOCKED chybějícím v145. Zavádějící exit 0
  neznamená úspěch; nevznikla binárka ani staging.
- Native runner Debug: 37 skupin, 20 PASS zdrojových/PowerShell kontrol,
  16 blokovaných native buildů (MSB8020), 1 FAIL kvůli chybějícímu SFTP souboru.
- CI pytest výběr: BLOCKED, pytest chybí; lizard také nedostupný.
- Existující unittest subset: 41 testů PASS; nepokrývá celou CI sadu.
- Release native, Win32/ARM64, čistá publikovaná matice, CodeQL, native
  conversion-table, donorové I/O a GUI smoke: neprovedeno.
- Přesné příkazy, rozlišení evidence a runtime staging: [VALIDATION.md](VALIDATION.md).
  Logy a výsledky: `C:/Samandarin/codex_pr769_prompts/evidence/baseline`.

## Další nejmenší krok

1. Obnovit kontext pouze z těchto dokumentů a ověřit HEAD/dirty stav. Piny
   nepřepočítávat podle živého PR; hotové S00 znovu neaplikovat.
2. Zajistit autorizované v145 prostředí, pinned submodules a chybějící testové
   závislosti, bez samovolné instalace či globálních změn. Opakovat blokované
   baseline kontroly a provést skutečný smoke runtime sestaveného z targetu.
   Pokud to prostředí neumožní, zachovat BLOCKED; žádné tvrzení „funguje“.
3. Po přijetí baseline vykonat právě [S01](steps/S01.md): Read/Write silent Skip
   assignment, Retry `OPEN_EXISTING`, failure vracející `INVALID_HANDLE_VALUE`.
   Zachovat dnešní path boundary. Adaptovat pouze ASCII část skutečného donor
   SafeFile harnessu; `SafeFileRootLength` a wide scénáře výslovně odložit S03.
   Chybějící Write Skip/Skip All assertion doplnit v S01, ne vydávat za donor PASS.
4. Před commitem této etapy provést její Debug/Release/native/GUI bránu,
   aktualizovat REUSE/PROGRESS a přidat provenance trailery. Zastavit po S01.

## Rozsah dokumentačního commitu

Pouze `doc/branch-view-port/`. Žádný nový adaptační runtime kód, žádné změny
SDK, projektů, resources nebo workflow. Úplný donor patch, pomocné auditní
skripty, logy, build outputy a fixtures nejsou v commitu. CONTRACT je bajtová
kopie uživatelem dodaného COMMON.md. Před commitem zkontrolovat celý staged
diff, `git diff --cached --check`, úplnost mapy i vlastní Python cache.

Commit message:

```text
docs: bootstrap incremental port of PR 769

Source-PR: KRtkovo-eu-AI/salamander#769
Source-Commit: 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413
Port-Step: S00
```
