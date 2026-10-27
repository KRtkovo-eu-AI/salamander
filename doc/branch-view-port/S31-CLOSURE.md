# Konečný uzavírací seznam S31

Výchozí revize: `0461f19396705c9e8dcc251cd7f51b252bde8076`.
Tento seznam podle zadání uživatele nahrazuje historické návrhy dalších podetap.
Starší OPEN/PENDING v deníku je stav tehdejšího checkpointu, nikoli nový úkol.
S31 dosud není dokončená. Pracujeme lokálně, bez pushe, PR a S32.

Nový nález patří sem jen pokud blokuje požadovanou funkci portu, je regresí portu,
nebo ohrožuje data při zpřístupněné operaci. Ostatní patří do následné práce.
Hotové důkazy se neopakují, pokud jejich vazba na aktuální implementaci platí.

## 1. Úplnost rozbalení a bezpečné odstranění archivů/svazků

- **Požadavek:** automatické odstranění smí následovat jen po úplném rozbalení
  a úplném, vlastněném seznamu zdrojových svazků. Skip, maska, chyba, Cancel či
  neúplná metadata nesmějí odstranit zdroj; rozbalení bez mazání zůstává funkční.
- **Implementace hotová:** ohraničený wide plán hostitele, kontrolované Add a
  interní ZIP kolekce, oddělená úplnost ZIP/7Zip včetně pending výsledků a nově
  metadata archivu před/po extrakci. SDK ABI a kontrola CRC zachované.
- **Ověření hotové:** S31b6c7b/c1/c2a/c2b1/c2b2/c3b; ordinary single-volume
  ZIP/7z, částečný výběr, Skip, obnova poškozeného ZIP bez odstranění a úplná
  extrakce s odstraněním testovací kopie v Debug/Release. Aktuální c3b důkazy
  a platnost nezměněných dřívějších testů váže `final-audit.json` mimo Git.
- **Nově ověřeno:** skutečný třísvazkový ZIP (WinZip-style, stored): úplná sada,
  přechody disk0 → disk1 → disk0, chybějící svazek/Cancel a přesné odstranění
  tří kopií D/R. Zdroje/sentinely/obsah ověřené hashi, profily obnovené;
  důkaz `evidence/S31-closure-volumes/final-result.json` mimo Git. Deduplikace
  navíc má již dříve ověřený nativní wrapper/collection kontrakt. 7Zip zatím nemá
  open-volume callback ani úplný seznam více svazků; jejich mazání není připuštěné.
- **Oprava stavu důkazů:** první GUI pokus položky 2 odhalil ve zděděném runtime
  starý ZIP plugin z c2a, před completion opravou b1. B1 má vlastní správný build
  a GUI důkaz; starší binárka převzatá přes b2 jej nenahrazuje. První Branch
  mask+delete pokus proto selhal a zůstává evidovaný jako FAIL, nikoli PASS
  aktuálního kódu. Aktuální ZIP je nově sestavený D/R; jeho GUI se ověřuje.
- **Zbývá:** potvrdit dotčené ZIP GUI důkazy včetně vícesvazkové kvalifikace proti
  skutečně aktuální binárce. Neopakovat nedotčené 7Zip/host native kontroly.
  Funkční integrace do Branch je samostatná položka 2; žádný příslib podpory
  libovolného formátu či všech spanning variant.

## 2. Odpovídající integrace do Branch View

- **Požadavek:** stejné kvalifikované rozbalování musí fungovat pro Branch zdroj
  i diskový Branch cíl; root/tab/generace, vlastnictví výběru a backend zůstanou
  ověřené přes dialogy a callbacky. Mazání nelze aktivovat jen odstraněním guardu.
- **Implementace i ověření hotové:** přímý Unpack ZIP/7Zip z Branch zdroje do
  ordinary panelu a do kořene Branch cíle včetně automatického podadresáře,
  Cancel, odmítnutí ručního přesměrování a ztraceného cíle: c5a/c5b/c6 D/R.
  F5 ZIP/7Zip do Branch cíle je ověřené b6b3b/b6b3c3; životnost zdroje níže zbývá.
- **Nová implementace hotová:** kvalifikované Branch mazání je propojené s
  vlastněným seznamem svazků. Nezávislý dispatcher i post-InitDLL wrapper
  vyžadují platný sink a ověřený backend; source/target/root/tab/generation
  kontroly zůstávají v controlleru i před každým odstraněním a opakováním.
- **Nové ověření hotové:** native controller a související kontrakty D/R,
  sestavení hostitele a aktuálního ZIP D/R. GUI Debug: ZIP i 7Zip částečné
  rozbalení zachová zdroj a úplné odstraní pouze testovací kopii. Release ZIP
  i 7Zip všechny čtyři případy prošly; přesné výstupy a sentinely ověřené hashi.
- **Ověření této integrace dokončené:** oba backendy D/R, částečné/chybové
  zachování, stale-context odmítnutí a přesný úplný úspěch bez zásahu do jiného
  tabu. Vlastní GUI zavřené a profily obnovené. Důkazy váže
  `evidence/S31-branch-unpack/final-audit.json` mimo Git; následuje lokální commit.

## 3. Pack-target a archivní F5 source-lifetime

- **Požadavek:** donorové core operace nesmějí používat zaniklý panel/listing
  po modal dialogu nebo plugin callbacku; Branch cíl znamená skutečný root.
- **Hotovo:** diskové Copy/Move cíle uzavřené S31b5c. Archivní F5 má wide cíl,
  captured disk target a post-InitDLL kontroly; ZIP/7Zip Branch target admission
  a output/cleanup jsou ověřené. Tyto hotové části znovu neimplementovat.
- **Zbývá implementovat/ověřit:** `CFilesWindow::Pack` dosud odmítá Branch cíl.
  Archivní `UnpackZIPArchive` má cílové kontroly, ale samostatný důkaz vlastnictví
  a životnosti source listing přes dialog/callback není dokončený.
- **Akce a dokončení:** převzít odpovídající donorové návaznosti po funkčních
  celcích; přesné source/target/backend snapshoty, bezpečný cleanup a refresh.
  Nativní reentrant/stale testy a skutečné Pack/F5 D/R ověří obyčejný i Branch
  případ a odmítnutí změněného kontextu bez operace nad jiným zdrojem/cílem.

## 4. Konečné rozhodnutí G04: podporované a odmítané operace

- **Požadavek:** pro každý původně požadovaný core příkaz existuje implementace,
  skutečné ověření a přesná admission politika, nebo výslovné rozhodnutí uživatele.
- **Hotovo:** archive-entry a následná oprava vlastnictví b1/b1a, shortcut b2,
  atributy/NTFS b3a4b a Properties b3b2 mají funkční D/R důkazy; počáteční blanket
  refusal v historické coverage již nepopisuje jejich aktuální stav.
  Vazba 17 relevantních těl na kvalifikované commity potvrzuje nezměněnou
  implementaci; `evidence/S31-closure-g04/existing-core-binding.json` mimo Git.
  Aktuální G04 tabulka už tyto příkazy neuvádí jako chybějící.
- **Zbývá:** spojit výsledky položek 1–3 s aktuálním routerem a tabulkou G04;
  jednotlivě odlišit kvalifikované bundled ZIP/7Zip od neověřených pluginů,
  externích packerů/viewerů a virtuálních filesystem cílů. Nevyvozovat podporu
  pouze z možnosti načíst DLL ani tiše nezúžit dohodnuté core operace.
- **Hotovo až:** aktuální G04 tabulka odkazuje pro každou operaci na platnou
  implementaci, test a přesný gate. Opravdová změna rozsahu bude samostatné
  konkrétní rozhodnutí uživatele; neověřeno se nepřejmenuje na nepodporováno.

## 5. Skutečné validační mezery a přesná omezení G05

- **Požadavek:** oddělit ověřeno, neověřeno a nepodporováno; doplnit chybějící
  důkazy pro požadované portované chování, ne opravovat libovolné historické chyby.
- **Hotovo:** jednotlivé lokální D/R buildy a operation-specific GUI/nativní
  důkazy. Reálné NTFS encrypt/decrypt je doložené b3a4a/b; to samo není důkaz
  raw EFS import/export v Undelete. String UNC není živý SMB test.
- **Chybí nebo potřebuje přesné přiřazení:** skutečné EFS raw restore, Connect
  backend/image hranice, SMB/offline/Cancel a konkrétní ADS/ACL/hardlink scénáře,
  kompletní jazykové staging/runtime pokrytí. Čistá podporovaná platformní matice
  a CodeQL aktuální konečné revize nejsou provedené; vzdálené spuštění je zakázané.
- **Akce a dokončení:** přiřadit existující důkaz aktuálnímu kódu a doplnit jen
  chybějící požadovaný scénář. U neproveditelného ověření zapsat konkrétní důvod,
  přesnou funkční hranici a potřebné rozhodnutí; neoznačit funkci za ověřenou.
  ChangeDir zůstává výslovně odložený podle rozhodnutí uživatele; související
  long-source GUI je NOT_VERIFIED, nikoli nově deklarovaná nepodporovaná funkce.

## 6. Coverage, provenience a důkazy konečné revize

- **Požadavek:** žádná chybějící core funkce schovaná za historickou položkou;
  původ převzatého kódu a skutečný rozsah ověření jsou dohledatelné.
- **Hotovo:** připnutý donor, průběžná REUSE/third_party mapa, lokální checkpointy
  a hashově svázané důkazy. Odložený ChangeDir WIP zůstává beze změny.
- **Zbývá:** po funkčních položkách sjednotit COVERAGE/REUSE/VALIDATION/PROGRESS,
  ověřit důkazy proti konečným blobům a doplnit pouze ověření změněného chování.
- **Hotovo až:** všech šest položek má ověřitelný výsledek bez nevyřešené
  povinnosti; finální diff, provenience, retained tests a revize důkazů souhlasí.
  Počet assertions či nový auditní dokument samy nic neuzavírají. Goal zůstává
  otevřený do splnění těchto podmínek. PAK Optimize zůstává skrytý; Windows
  bezpečnostní nastavení, testy a kontroly se neoslabují. S32 se nezahajuje.
