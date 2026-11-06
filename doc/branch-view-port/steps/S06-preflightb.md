# S06-preflightb — bezpečná příprava cesty v ikonočtečce

## S06-preflightb — publikováno a přijato (2026-10-14)

[Interní draft PR #25](https://github.com/majklvi/salamander/pull/25), přesný
head 2d83633687299c2545a117dfaf9e3e6f1399846a: všech 12 checks SUCCESS,
Win32/x64/ARM64 /warnaserror, VSIX, native/Python/reporty, growth, squash guard,
CodeQL gate i skutečná analýza. CodeQL job 115946770877 dokončen 15:53:03 UTC;
Initialize, Release MSBuild, analýza a SARIF upload úspěšné. Žádný check není
pending/skipped. Lokální PHP/LoadImageW subcase SKIPs tím nejsou přeznačeny.
Při zahájení S06 znovu ověřeno a uloženo jako evidence/S06-final/
accepted-preflight-ci.json; plné job kroky v S06-preflightb/ci-current-codeql-job.stdout.log.
Accepted reference posunuta bez force z bf78c817 na 2d836336. Draft zůstává
otevřený; samandarin-0.16 a SOURCE/CONTRACT piny se nemění.

Stav: IMPLEMENTED / lokální brána PASS, publikace a current-head CI PENDING.
Parent bf78c81723abc983805a61acaeec6d92fd4b2316; lokální větev
integration/pr769-s06-preflightb-candidate. Donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413 a SOURCE/CONTRACT beze změny.

## Problém a rozsah

Při pokusu o S06 se Compare skutečně dostal k oběma obsahům, ale Debug
shutdown scénářů utf8-byte-long PID2552 a utf16-long PID15196 skončil
RTC Error 2, Stack memory corruption; main thread čekal na zánik panelu.
Původní IconThreadThreadFBody (src/fileswn1.cpp:1824–1842 v parentu) kopíruje
celý GetPath() do path[MAX_PATH+10] bez kapacity a ukončuje wPath přes
wName=wPath+l, kde l označuje UTF8 bajty. Oba scratch buffery jsou malé
i pro běžný dlouhý panel. Tato část byla nezměněná proti výchozí a84dbed6.

Z připnutého donoru se přebírá pouze narrow CPathBuffer(4*SAL_MAX_PATH+16).
Celý donorový worker závisí na budoucích Branch cache keys a overlay API;
zůstává DEFERRED S26/S28. Lokální wide scratch CWidePathBuffer(SAL_MAX_PATH+16)
je na haldě. Panelový Path má 32768 bajtů včetně terminátoru; maximální root,
doplněný oddělovač i nulový konec proto do obou scratch bufferů vejdou.
Konverze používá jejich skutečnou kapacitu, UTF8-first/ACP a vrácený počet
wide znaků pro wName. Nezavádí se velký stack buffer ani změna SDK či CFileData.
Existující MAX_PATH guardy dalších shell/overlay operací zůstávají; tento krok
neznamená úplnou podporu dlouhých ikon/overlay a nepřebírá Branch UI.

## Testy a runtime

Nový icon_reader_path_scratch_tests kompiluje celý skutečný preparační blok
a skutečné konstruktory/Data/Capacity CPathBuffer a CWidePathBuffer.
Facades jsou pouze panelový root/type, kontrola komponent a Google Drive dotaz.
Debug /RTC1 i Release /O2, obojí /W4 /WX: 93 checks/config PASS, včetně ASCII,
trailing separator, více Unicode komponent, UNC, UTF8 nad MAX_PATH při krátkém
UTF16, UTF16 nad MAX_PATH, maximálního 32767bajtového rootu, skutečného ACP
fallbacku, non-disk guardu, přesných suffix offsetů a integrity heapu/tailu.
Harness neprovádí shell ani file I/O; to nepředstírá.

Host Debug (13,83 s), Release (65,78 s) /warnaserror PASS; původní Filecomp D/R
znovu sestaven bez odloženého S06 kódu. Debug/Release Bug Reporter obnoven ze
stejných zdrojů, aby runtime neobsahoval starou diagnostickou binárku.
Celé native sady 46 groups/config PASS, pytest výběr 49 PASS. Skutečné lokální
subcase SKIPs: php.exe chybí; LoadImageW pro dlouhou bitmapu vrací Win32 2.
Přesné příkazy, časy a výstupy: evidence/S06-preflightb/*.json, *.stdout.log,
*.stderr.log a native-debug.xml/native-release.xml/python.xml. Všechny buildy
a testy váže source-manifest.json a runtime.patch SHA256
ebecf5b68919547c3192470f8800455f172908fdef170fb3aac5edb95daf7174.

Ruční Computer Use, vlastní izolovaný config se schématem Windows Dark Mode
experimental, běžné diskové panely a F3:

| Config | Scénář | Host / reporter PID | Výsledek |
| --- | --- | --- | --- |
| Debug | UTF8 390/391 bajtů, UTF16 220/221 | 14676 / 8784 | listing, přesná celá F3 cesta, alpha obsah, normální shutdown PASS |
| Debug | UTF16 407/408 jednotek | 10332 / 14592 | listing, přesná celá F3 cesta, alpha obsah, normální shutdown PASS |
| Release | UTF8 390/391 bajtů, UTF16 220/221 | 12620 / 2572 | listing, přesná celá F3 cesta, alpha obsah, normální shutdown PASS |
| Release | UTF16 407/408 jednotek | 14180 / 6880 | listing, přesná celá F3 cesta, alpha obsah, normální shutdown PASS |

U všech čtyř ověřen konec hostu i jeho reporteru a nepřítomnost nového reportu.
Nejen zánik hlavního okna. Původních 45 souborů/sentinelů hashově nezměněno,
vlastní bootstrap configstorage.ini obnoven byte-exact, runtime D/R manifesty
uloženy mimo Git. Uživatelský instalovaný Salamander PID692 a salmon11020
nedotčeny. Do produktu nebyla přidána ani ponechána dočasná instrumentace.

## Neúspěšné pokusy a zbývající nálezy

První generování native fixture selhalo kvůli escapování backslashů v testu;
opravený harness výše prošel. První GUI starty 10684/13288/11516 oznámily,
že Bug Reporter neběží; nejsou PASS. Event Log ukázal vlastní salmon C00000FD.
Omezený debugger pouze vlastních procesů zachytil opakovaný MyGetThemeColor
v src/third_party/darkmodelib/src/DmlibHook.cpp:458 při starých reportech.
Diagnostický host byl ukončen helperem a není shutdown PASS. Tento nezměněný
dialogový problém se neopravuje uvnitř icon scratch předkroku.

Pouze dva reporty vytvořené předchozími vlastními S06 pády byly po ověření
shodných hashů archivovány z LOCALAPPDATA/Open Salamander do
evidence/S06-preflightb/archived-owned-reports (kopie navíc v S06-owned).
owned-report-archive.json eviduje přesné cesty/hash. Žádné cizí reporty nebyly
smazány, neklikalo se na globální Do Not Send Report a diagnostika se nevypnula.
Nové finální starty měly živý vlastní salmon po celou dobu a žádný nový report.

S06-owned GUI rovněž ukázalo nezměněné ANSI vykreslování UTF8 hlaviček
v filecomp/controls.cpp CFileHeaderWindow::WM_PAINT a zkrácený difference-count
suffix u Unicode názvu. Přesné vstupy a skutečné I/O fungovaly, ale nejde o
plný Unicode UI PASS. Tyto samostatné nálezy jsou zapsané pro navazující audit.

## Odložená S06 a další krok

Celá vlastní Filecomp migrace a testy jsou uchovány mimo Git jako
evidence/S06-owned/pending-S06-owned.patch, SHA256
463bf8fa7e5037cb6f1ccfe7ee732d25fddfbd7dbe960f779f273898011d529f.
58 native checks/config a tehdejší 46 regresních groups/config prošlo;
starý skutečný command negativně 23 failures z 51 checks. Debug Compare I/O
šesti scénářů prošlo, ale dvě dlouhá ukončení selhala; Release GUI NOT RUN.
Proto není migrace přijata, její produkční kód ani testy v tomto commitu nejsou.

Nejprve publikovat tento předkrok a použít Ensure-ForkCI bez duplicitního draftu.
Nové CI/CodeQL/Win32/ARM64 NOT RUN; accepted reference zůstává bf78c817.
Po úspěchu přesného headu obnovit uložený S06 patch, při integraci runneru
zachovat test icon_reader_path_scratch i filecomp_panel_paths a opakovat
S06 native/Debug/Release skutečnou Compare bránu. Žádný push/PR/merge není
součástí tohoto lokálního kroku. Následující etapy se nezahajují.
