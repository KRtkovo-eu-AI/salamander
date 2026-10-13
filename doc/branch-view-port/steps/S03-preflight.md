# S03-preflight — ZIP listing ownership a rozbalování

Stav: lokální brána PASS (2026-10-14); čeká nová CI opravného commitu a přijetí S03.
Nepokračovat do S04 před uzavřením této brány.

## Potvrzená příčina a oprava

Na parentu `919e804da7fa951daa3ed2231a847c94dd1b9357` skutečný Debug CRT dump
obsahuje 34-byte blok z `src/plugins/zip/list.cpp:37`, `DupZipUtf8NameW`,
s UTF-16 obsahem `safe-archive.txt`. Host `CSalamanderDirectory::AddFile/AddDir`
kopíruje historická SDK pole a nastavuje `NameW=NULL` kvůli starším binárním
pluginům. ZIP po úspěšném přidání kopii neuvolňoval. Stejný listing je v
baseline i připnutém donoru; nejde o SafeFile wide regresi.

Wide kopii nyní vlastní interní `CZIPFileData`. Jeho destructor se volá přes
stávající `ReleasePluginData`, při reject i listing reset. `CFileData::NameW`
zůstává optional view; SDK ABI se nemění. Metadata se vytvoří před wide kopií,
`new (std::nothrow)` zachovává existující LOWMEM větev a úzké obnovení debug-new.

Cold multi-entry ZIP smoke dále odhalil skutečný AV C0000005:
`C7zClient::GetArchiveFormat` -> `CLibrary::Load/Free`. Během startupu se
configured unpacker změnil na 7-Zip, ale listing interface i per-entry data
v panelu stále patřily ZIPu. `PackUncompress` nyní pro tentýž panel/archive/
interface používá uloženého vlastníka. Chybějící či unsupported owner se
odmítne existující lokalizovanou chybou. Odlišný listing, externí konfigurace
bez bound panelu a původní fallback zachovávají dosavadní dispatch.
7-Zip zdroje, shared SDK, heap tracker i MENU ignore predicate beze změny.
Neimportují se donorové S13/S14 ani budoucí path migrace.

## Důkazy a hranice testování

- Host a ZIP plugin Debug/Release x64 v145 /warnaserror PASS. Finální Debug
  po odstranění diagnostiky znovu přeložil `heap.cpp`; žádná temp změna není v commitu.
- Celá skutečná `CZipList::List`, `CZIPFileData`, DupName, host legacy field copy,
  `PackUncompress`, owner lookup a Contains kompilované /W4 /WX: 151 Debug a
  138 Release checks PASS. Reader/SDK/UI a jednorázové OOM jsou testové adaptéry,
  skutečný CRT a allocation/free cleanup; nejde o test ZIP parseru či GUI.
  ASCII/Unicode file/directory, reject, Unix reset, read failure, long/oversized
  listing a OOM; změněná konfigurace, jiné interfaces/archive, owner rejection,
  configured/fallback/external dispatch. Stejný harness proti 919e804d selže:
  25 assertions včetně cascaded counters. Není to 25 samostatných chyb.
- Finální native 42 groups/config PASS (39,91 / 41,51 s), SafeFile actual I/O
  268 checks/config; pytest 49 PASS. PHP missing a long-bitmap LoadImageW Win32 2
  explicitně SKIPPED. Group JUnit skipped=0 z nich nedělá PASS.
- Skutečné cold GUI Debug/Release extrahovalo čtyři vlastní soubory: ASCII,
  Unicode název, Unicode rodič a duplicate leaf names v různých adresářích.
  Všechny velikosti a SHA256 ověřeny zvlášť. Restart panely a shutdown PASS.
  Původní F3/normal-disk F5 dokazují historické S03 smoke logy; v preflight
  nebyly bez důvodu znovu opakovány. Žádný test skutečného SMB I/O.
- Archive-only/disk captures po opravě exit 0 bez 34-byte ZIP bloku. Dodatečný
  restart diagnostic: `normal=16/512`, free/ignore/client=0, known=1, exit 0;
  default CRT může tyto systémové MENU bloky stále vypsat. Původní predicate
  zůstává přesně stejný. Historický další native restart heap warning se po
  finální přestavbě neopakoval, není připsán neprokázané další příčině.

Evidence `C:/Samandarin/codex_pr769_prompts/evidence/S03-preflight`: runtime.patch,
source/runtime/fixture manifesty, bounded JSON/logs, JUnit, launch/hash ledger,
PDB fault capture a zachovaný vlastní report. Diagnostické deadline captures
`Final-startup-check` a `Restart-heap-state` nejsou PASS. První startup wait
způsobil neukončený vlastní reporter; jeho report zachován hashově shodně v
evidence, bez smazání nebo odeslání. Osobní instalace nedotčena, own bootstrapy
obnoveny, owned procesy/cache odstraněné, sedm původních fixtures beze změny.

## Publikační stav

Uživatel autorizoval opravu a předtím push kandidáta do forku. Opravný commit
navazuje na stejnou `integration/pr769-s03-candidate` a interní draft
[PR #21](https://github.com/majklvi/salamander/pull/21), bez duplicitního PR.
Předchozí SHA 919e804d mělo všech 12 checks SUCCESS včetně skutečné CodeQL
analýzy; po novém pushi je nutné ověřit novou CI. S03 zůstává nepřijatá,
reference `refactor/pr-769-incremental` na 17678e7c, release base beze změny.
SOURCE/CONTRACT/donor piny nezměněné. Žádný upstream PR ani S04.
