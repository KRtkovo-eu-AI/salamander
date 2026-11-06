# S27 — závěrečný lokální checkpoint

Stav: **LOCAL COMPLETE WITH DOCUMENTED LIMITS**, kódový konec 7ca7844e.
Tento dokument uzavírá lokální implementační goal; nepředstavuje publikaci,
merge nových S27e/f změn, úspěch vzdáleného CI ani zapnutí funkce pro uživatele.
Přijatý výchozí S27d: df01f687f062672cb74a2cc21a9116508593bc36.
Zdroj PR769: 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, beze změny.

## Dokončené lokální jednotky

Každý kódový checkpoint obsahuje vlastní Debug/Release host/plugin/dependency sestavení,
relevantní nativní kontroly a skutečné GUI ověření podle svého rozsahu. Čísla
z jednoho checkpointu se nepřenášejí na jiný; přesné příkazy, facades, výsledky,
binární hashe a odlišnosti průběžných pokusů jsou v odkazovaných dokumentech
a externí evidence/S27*/committed-validation.json. PAK je dokumentační audit.

| Jednotka | Rozsah | Lokální commit |
|---|---|---|
| [S27e1](S27e1.md) | ZIP/7-Zip kopírovací archivace | 97c857a8 |
| [S27e2](S27e2.md) | Archivace s odstraněním přesných zdrojů | b1f0ec35 |
| [S27f01](S27f01.md) | File Comparator | fd098ac0 |
| [S27f02](S27f02.md) | Batch Rename | b7aeada9 |
| [S27f03](S27f03.md) | Checksum Calculate/Verify | fc1631c6 |
| [S27f04](S27f04.md) | Split & Combine | a04e7f5c |
| [S27f05](S27f05.md) | Automation focused script | e79dd1a3 |
| [S27f06](S27f06a.md) | Salamatrix registrované balíčky a provider kontext | 005da082 |
| [S27f06b](S27f06b.md) | Automation načtené/odložené příkazy | 9654a3d0 |
| [S27f07](S27f07.md) | PictView EXIF náhledy | f929cf5b |
| [S27f08a](S27f08a.md) | ZIP Test Archive | 5d44aae9 |
| [S27f08b1](S27f08b1.md) | ZIP Comment | a87dbb3c |
| [S27f08b2](S27f08b2.md) | ZIP CreateSFX | 71ff03e7 |
| [S27f09](S27f09.md) | 7-Zip Test Archive | 586b6ce1 |
| [S27f10](S27f10.md) | PAK Optimize: záměrně skrytý | 681e1578 |
| [S27f11](S27f11.md) | Undelete Restore/Connect | f721060b |
| [S27f12](S27f12.md) | Last Command / Ctrl+I | e37e46e3 |
| [S27f13](S27f13.md) | Checksum Focus a přesná navigace | 043f7db4 |
| [S27f14](S27f14.md) | PictView refresh a místní API nápověda | 7ca7844e |

S27a1/a2, S27b, S27c1a–c4 a S27d1–d5 navazují na dříve přijaté checkpointy.
S27e1+e2 uzavírají hostitelskou archivaci. S27f.12–.14 uzavírají pomocné
příkazy původně odložené při individuální integraci pluginů.

## Podporovaná hranice a zachované limity

- Celá funkce Branch View zůstává experimentální a standardně vypnutá.
  Aktivace stále vyžaduje přesné SAMANDARIN_EXPERIMENTAL_BRANCH_VIEW=1;
  aktuální kontroly chrání klávesy, menu a obnovování stavu. S32 je samostatná etapa.
- PAK Optimize podle výslovné volby uživatele není obnoven v nabídce. ZIP Repair 2
  má v donoru i targetu zakomentovanou registraci a nemá aktivní tělo Repair.
  Oba zůstávají skryté/odmítnuté, nikoli označené za nově funkční operace.
- Povolení pluginů je soukromá explicitní politika ověřených bundled DLL/příkazů.
  Neznámé/external pluginy a packery se nepovolují podle verze SDK. Registrace,
  enabled stav, schopnosti runtime a bezpečnostní kontroly nejsou obcházeny.
- S27a podporuje core interní a nakonfigurované externí viewery/editory.
  Obecné pluginové viewer callbacks zůstávají gated. PictView 188 je ověřený
  menu příkaz pro EXIF náhledy, 197 obnova diskových panelů po konfiguraci;
  capture/scan, SaveAs/Focus a další interní viewer pokračování tím nejsou povoleny.
  Test f14 otevřel PictView z obyčejného panelu a změnil jeho izolované nastavení
  při aktivním Branch View; není to důkaz nové podpory PictView F3 v Branch.
- Shell/Enter stále zachovává dohodnuté odmítnutí nepodporovaných dlouhých
  spustitelných cest při výchozím Windows nastavení. LongPathsEnabled zůstal 0;
  žádné obcházení bezpečnostních dialogů. Vysvětlení/hláška viz S27a2-completion.
- Undelete ověřilo skutečné kopie nešifrovaných souborů; dlouhá image cesta
  dostává existující explicitní limit. Fyzické disky a skutečný EFS import nebyly
  testované. Přesný Restore label je v UIA úplný, šířka dialogu jej může vizuálně oříznout.
- ZIP/7-Zip archivace zachovává existující best-effort cleanup bez transakčního
  rollbacku. SFX výstupy byly nezávisle ověřené jako PE/ZIP, nikoli spouštěné.
  Stávající limity instalace SFX balíčků/custom nastavení, ZIP64 a multi-volume
  nejsou touto integrací rozšířeny. Úplná jazyková packaging matice ještě neběžela;
  běžný Zip2Sfx Release packaging potřebuje správné OPENSAL_BUILD_DIR prostředí.
- S27e Debug opakoval známou heap diagnostiku; v těchto bězích se původ znovu
  neinstrumentoval. S27f.08b1 zvlášť reprodukoval i na parentu obyčejného ZIP
  dialogu 9 MENU párů/576 bytů. Není to opravený leak ani bezvýhradně čistý shutdown.
  S27f.05 handle diagnostiku následně řeší a finálně ověřuje S27f.06b.
- Automatické nové publikace/refresh a navazující async properties/history/performance
  patří do S28–S30; S27 nepřepisuje jejich plán ani neoznačuje odložené assertions PASS.
  Podrobné omezení přijímacího dropu/root paste viz S27d5b.

## Závěrečné ověření

Audit.py nad čistým kódovým koncem 7ca7844e ověřil 19 navazujících commitů,
238 souborových vazeb proti skutečným Git blobům (včetně doložené EOL konverze)
a 225 hashů validační evidence. Ověřil čistý připnutý donor, nezměněný SOURCE.json,
nezměněné veřejné hlavičky SDK od S27d a nezměněný default-off gate. Jediná změna
pod shared je interní lukas/resedit.cpp z doložené SFX opravy, nikoli SDK layout.
Ve vlastních src/tools/doc nebyly nalezené Python cache artefakty.

Poslední S27f.14: Debug/Release host/PictView/Salamatrix a jejich závislosti /warnaserror PASS,
172/172 skutečných nativních policy/dispatch/helper kontrol v každé konfiguraci,
28/28 registrovaných source/menu skupin, Debug/Release skutečná místní nápověda a PictView
konfigurace. GUI ověřuje pracovní tok a zachování panelu; nativní recorder zvlášť
ověřuje parametry force-refresh. Žádné tvrzení o změřeném cache counteru/pixelech.
Kompletní fixture inventory a hashe zůstaly stejné, vlastní procesy byly zavřené,
configstorage.ini obnoveno bajtově. Jde o cílené testy, nikoli novou plnou sadu.

Externí audit: C:/Samandarin/codex_pr769_prompts/evidence/S27-completion/audit.json.
Historické „unfinished/next/pending“ v průběžných záznamech jsou časové snapshoty;
tento přehled je aktuální uzavření jejich S27 rozsahu.

## Další krok a publikační hranice

Nic nového nebylo pushnuto, žádné nové vzdálené PR ani CI/CodeQL nebylo spuštěné.
Čistý build celé podporované sestavy/platformní matice se pro nové S27e/f commity
zatím neprovedl. Lokální goal je dokončený, publikační akceptace je samostatná.

Po samostatném zadání začít koherentním checkpointem S27e1+e2, pak odděleně
jednotlivé pluginové skupiny S27f.01/.02/.03/.04/.05–.06/.07/.08/.09/.11.
Pomocné .12–.14 mají závislosti na předchozích jednotkách; zařadit je podle
skutečné ancestry/diffu. PAK dokumentace může být součástí přehledu omezení.
Nekopírovat všude slepě finální tip: před každou publikací zkontrolovat skutečný
rozsah větve a navazující opravy, vytvořit/reuse interní fork draft přes
Ensure-ForkCI.ps1 a vyčkat current-head build/test/CodeQL i čisté celé sestavy.
Není tím uděleno oprávnění k push/PR. Další kódová etapa je S28 podle svého plánu.
