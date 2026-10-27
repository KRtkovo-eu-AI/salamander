# Konečný uzavírací seznam S31

Stav: **COMPLETE_LOCAL_WITH_ACCEPTED_VALIDATION_LIMITS**, 2026-10-28.
Funkční head `1d6ef63846f1da654645778ecb431e26ed1daedc`; konečný dokumentační
commit a důkazy váže externí `evidence/S31-final/committed-validation.json`.
Uživatel výslovně schválil předání níže uvedených chybějících prostředí a
publikačních zkoušek do následné práce. Neověřeno se nepovažuje za nepodporováno
ani za PASS. Bez pushe, vzdálených PR/CI a bez S32; vydání není schválené.

## 1. Úplnost rozbalení a bezpečné odstranění archivů/svazků

- **Požadavek:** odstranit zdroj jen po úplné extrakci a z vlastněného úplného
  seznamu svazků; maska/Skip/chyba/Cancel/neúplná metadata zdroj zachovají.
- **Implementace hotová:** host wide deletion plán, ZIP/7Zip completion a
  pre/post error/volume metadata; rozbalení bez mazání zachované, SDK/CRC beze změny.
- **Ověření hotové:** single-volume úplný/částečný/chybový průběh D/R;
  skutečný aktuální ZIP o třech svazcích bez mazání, missing-volume/Cancel a
  přesné odstranění právě tří kopií D/R (`016c2dd9`). 7Zip bez prokázaného
  úplného seznamu svazků automatické odstranění nepovolí.
- **Akce / dokončení:** nic funkčního nezbývá; aktuální nezměněná těla a scoped
  důkazy svázané ve finálním záznamu. Nejde o všechny formáty spanning archivů.

## 2. Odpovídající integrace do Branch View

- **Požadavek:** stejné kvalifikované rozbalení z Branch do ordinary/Branch root,
  včetně source/target/tab/generation/backend a per-delete kontrol.
- **Implementace i ověření hotové:** `b497f058`, no-delete/cancel/error a osm
  nových částečných/úplných ZIP/7Zip D/R GUI readbacků. Stale ZIP první pokus
  zůstává FAIL; nahradil jej aktuálně sestavený a ověřený ZIP, nikoli přepsaný log.
- **Akce / dokončení:** splněno; vlastní panely/profily uklizené, přesné výstupy,
  zálohy a sentinely ověřené; aktuální controller/dispatcher těla nezměněná.

## 3. Pack-target a archivní F5 source-lifetime

- **Požadavek:** vlastnit identity a zabránit půjčení zaniklého panelu/listingu
  přes modal/InitDLL/callback; cíl Branch vždy znamená skutečný wide root.
- **Implementace hotová:** F5 `72d83019`, Pack `1d6ef638`; epoch, live tab/owner,
  backend binding, busy/refresh scopes, wide kvalifikovaný scanner s výslovným
  zachováním legacy ACP, bezpečný metadata/preview/empty-directory cleanup ZIP.
- **Ověření hotové:** nativní stale/reentrant/OOM/negative-control a D/R buildy;
  F5 ordinary/Branch D/R payloady; Pack ZIP/7Zip Copy/Move/Cancel D/R má všech
  16 přesných readbacků PASS a automatický refresh. Profily obnovené.
- **Přijaté omezení:** dlouhý ZIP multi-volume/SFX uživatel výslovně schválil
  bezpečně odmítat před preview/defaults/backend; krátké cesty a běžný ZIP/7Zip
  zůstávají zachované. Actual handler/default-loader D/R testy a buildy PASS.
- **Akce / dokončení:** splněno v tomto rozsahu; plná dlouhá multi/SFX podpora
  je následná práce. [Pack důkazy a přesné limity](steps/S31-pack-target.md).

## 4. Konečné rozhodnutí G04

- **Požadavek:** každý dohodnutý core příkaz má implementaci, důkaz a přesný gate.
- **Implementace i ověření hotové:** archive-entry, shortcut, atributy/NTFS a
  Properties mají 17 aktuálně nezměněných kvalifikovaných těl; body 1–3 doplňují
  Unpack/Pack/F5. Aktuální operation contracts All 20, view 19 a skutečný plugin
  policy/dispatch D/R 179/config PASS. Počty nenahrazují jejich GUI důkazy.
- **Akce / dokončení:** aktuální G04 tabulka v COVERAGE rozlišuje ověřené
  bundled backendy, neověřené pluginy/externí packery a virtuální FS cíle.
  Žádné nové plošné povolení DLL; dohodnuté core funkce se nezúžily. Splněno.

## 5. Skutečné validační mezery a přesná omezení G05

- **Požadavek:** opravit skutečné mezery portu a odlišit ověřeno/neověřeno/
  nepodporováno; nepřevzít všechny historické chyby jako další úkol S31.
- **Implementace i ověření hotové:** EFS `59c39d5d`, skutečný Windows RAW a ADS
  v šesti třídách cest D/R, early/midstream cancel, native fault/late-cancel a
  plugin D/R buildy. Windows odmítnutí úklidu se hlásí, neobchází; neúplný soubor
  může zůstat. První starý FAIL zachovaný, tehdejší delete error nebyl zachycen.
- **Přesně chybějící důkazy:** úplné image/physical-disk backend scénáře,
  skutečné SMB/offline, celá per-operation ADS/ACL/hardlink matice, všechny jazyky
  v runtime/staging a čistá platformní CI/CodeQL této revize. **NOT_VERIFIED /
  NOT_RUN**, funkce ani kontroly se neruší. Connect prefill/legacy refusal má
  vlastní D/R důkaz a sedm nezměněných kvalifikovaných těl.
- **Rozhodnutí a dokončení:** uživatel 2026-10-28 výslovně schválil jejich
  předání do následné validační práce a lokální uzavření. Konkrétní budoucí
  akce a ověřitelné podmínky jsou v [konečné matici](steps/S31-final-validation.md).
  Odložený ChangeDir se neobnovuje; Debug heap výhrada se nepřepisuje na PASS
  ani bez důkazu neoznačuje za novou regresi. Položka lokálně uzavřená rozhodnutím.

## 6. Coverage, provenience a důkazy konečné revize

- **Požadavek:** žádná skrytá chybějící core funkce, změněný zdroj nebo stale
  binárka vydávaná za konečný důkaz.
- **Ověření hotové:** 231 donor souborů / 892 hunků a 352 target extras aktuálně
  klasifikované; všech 140 změněných source/tool cest S31 má REUSE mapu.
  Čtyři funkční DEFERRED hunks G01/G04 jsou doloženě vyřešené. Jen dva S32
  dokumenty zůstávají DEFERRED. Textový screen není runtime kvalifikace.
- **Provenience a integrita:** donor pin čistý, původní odložený patch bajtově
  zachovaný, veřejné SDK proti S30 beze změny, původní scoped důkazy použité jen
  pro shodná těla. Nové Pack/EFS source, test a artifact hashe svázané; historické
  FAIL/timeouty zachované. Windows registry stále LongPathsEnabled=0.
- **Akce / dokončení:** finální diff/registrace/doc vazby a čistý lokální commit
  ověřené závěrečným záznamem. Goal lze uzavřít lokálně s přijatými limity bodu 5.
  PAK Optimize zůstává skryté, S32 se nezahajuje a nic se nepublikuje.
