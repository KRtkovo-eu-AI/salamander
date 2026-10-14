# S06-preflight — úklid subclass řetězce File Comparatoru

Stav: IMPLEMENTED / lokální brána PASS (2026-10-14). Samostatný nejmenší
předkrok podle CONTRACT; vlastní owned-selection migrace S06 není provedena.
Větev `integration/pr769-s06-preflight-candidate`, parent
`ffa1519d419af030586c78b977a4887e600e5e31` (přijetí S05 kódu 89698944).
SOURCE.json, CONTRACT.md, připnutý donor a release reference beze změny.

## Prokázaná příčina a oprava

Původní Compare command v Debug správně zobrazil dva očekávané rozdíly,
ale při shutdownu hlásil heap leak. Pouhé otevření dialogu a Cancel stačilo:
CRT dump 26 normal blocks / 928 bytes. Dočasný alokační hook + symbolizovaný
trace přiřadil dodatečné bloky k File Comparator ComboBoxData/ThemeData.
DragDropEditProc získával rodičovský CCompareFilesDialog z WindowsManager;
ten jej odpojí již při WM_DESTROY. Pro pozdní zprávy obou child comboboxů
pak callback vracel NULL a nepředal WM_NCDESTROY dalším subclass procedurám.
Nemohly tedy uvolnit svá data pro vzhled ovládacích prvků.

Oprava pouze dialogs.cpp/.h: SetWindowSubclass pro oba vstupy,
DefSubclassProc pro nezpracované zprávy bez závislosti na rodiči a
RemoveWindowSubclass při WM_NCDESTROY. OldEditProc1/2 již nejsou potřeba.
WM_DROPFILES zachovává původní DragQueryFile/SetWindowText/DragFinish větev.
Žádná změna resolveru, výběru dvojice, porovnávacího enginu, cest, veřejného
SDK, darkmodelib ani heap ignore predikátu. Dočasný heap.cpp hook je odstraněn;
finální soubor je bajtově shodný s parentem.

Callback před opravou je přesně shodný v target a84dbed6, parentu i připnutém
donoru 47661e92; nejde o donorový port. Samostatné auditní zjištění: stacky
původních osmi host MENU skupin ukazují také na app-owned ThemeData; komentář
stávající 512-byte heap výjimky o OS-owned původu tím není doložen. Výjimka
zůstala nedotčena; zde se opravují jen dvě pole File Comparatoru. GUI výsledek
znamená shutdown bez diagnostického dialogu, nikoli důkaz nulových alokací.

## Skutečně provedená validace

Evidence `C:/Samandarin/codex_pr769_prompts/evidence/S06`, zejména
source-manifest.json, runtime.patch, local-summary.json, gui-ledger.json,
Debug/Release-runtime-manifest.json a všechny bounded command reports.
Parent + runtime patch SHA256
`08bc6369fe9dfa0aaf875f0a1ae97a575bfaadfc578d17eb2561675c755c74bb`.

- MSBuild host + filecomp Debug/Release x64 /warnaserror PASS; poslední host
  Debug 7,10 s, Release 65,30 s; plugin Debug 7,16 s, Release 14,59 s.
  Interní dialogs.h mění pouze tento plugin, společné SDK se nemění.
- `python -B src/tests/filecomp_dialog_lifetime_tests.py` a registrace v
  `tools/run_native_tests.ps1`: 964 checks/config PASS /W4 /WX. Kompiluje
  skutečný celý callback i registrační hunk; reálné Win32 ComboBox/subclass
  řetězce, 40 cyklů, oba vstupy, HDROP obsah, nezpracované zprávy a úklid po
  odpojení rodiče. Facade simuluje pouze mapování životnosti rodiče.
  Původní kód přes --source-ref ffa1519d: compile PASS, 804 checks / 161
  failures EXPECTED FAIL, nikoli produktový PASS.
- `tools/run_native_tests.ps1 -Configuration Debug|Release -Platform x64
  -ResultsPath ../evidence/S06/native-<config>.xml -CommandTimeoutSeconds 60`:
  45 groups/config, failures/errors 0; 51,47 / 59,16 s vnější bounded report.
  Chybějící php.exe a LoadImageW long bitmap Win32 2 zůstávají subcase SKIPPED.
- CI pytest výběr podle python.json: 49 PASS (6,72 s), cache vypnutá.
- Skutečné GUI Debug 8504 (nový empty private config), Debug 6264 a Release
  11092 (vlastní config se schématem Windows Dark Mode experimental): Cancel,
  znovu otevřený Compare, přesné ASCII left/right vstupy, obsah alpha/beta
  a dva očekávané rozdíly; normální ukončení bez heap/handle dialogu PASS.
  Poslední ověřovací host Debug build změnil hash exe; proto nový proces 9668
  zopakoval Cancel/shutdown na finální binárce. Filecomp DLL hash se nezměnil;
  starší launch hashe v ledgeru zachovány. Žádná GUI diagnostika se nepotlačila.
- Všech 43 vlastních původních souborů/sentinelů hashově beze změny, vlastní
  runtime bootstrapy obnovené, žádný vlastní GUI/helper proces nezůstal.
  Instalovaný uživatelský Salamander PID 692 nedotčen.

Diagnostické 90s helpery zachytily dump/stacky, ale skončily na deadline;
nejsou shutdown PASS. Přípravné chyby testového harnessu i evidence collectoru
opravené a logy ponechané; nejsou selháními finálního produktu.

NOT RUN: ruční GUI drag/drop gesto (HDROP callback otestován nativně),
vlastní migrace S06 a její Unicode/long-path Compare matice, nová fork CI,
CodeQL a publikační Win32/ARM64 matice. Tento předkrok zůstává lokální;
accepted reference ffa1519d se nepovyšuje před publikací a current-head CI.
Další nejmenší krok: publikovat a ověřit předkrok; poté původní S06.
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
