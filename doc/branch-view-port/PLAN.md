# Plán postupného portu PR #769

## S31b5a — audit klasického Copy/Move cíle proti drop rootu

Klasický FilesAction odmítá Branch target před dialogem; DropCopyMove naproti
tomu připustí přesně shodný wide root. Zákaz nelze odstranit bez ověřeného
target snapshotu a kontroly výsledku resolveru. Navíc ParsePath zděděný z donoru
bere source-relative základ do520 byte curPath a ignoruje FALSE z GetGeneralPath
při zkrácení. SalParsePath má také krátký error buffer s neohraničeným formátem
plné cesty; jde o statický nález, nikoli nový doložený pád/exploit.
Další nejmenší S31b5b1 opraví kapacitu a chybový kontrakt resolveru, potom
S31b5b2 target identity/modal recheck a S31b5b3 ověřená aktivace Copy/Move do
Branch rootu. Guardy beze změny. Native getters399/config D/R a source contracts
transitions28/operations All20 PASS; nejsou nové GUI ani full-operation testy.
ChangeDir je odložený na žádost uživatele a zůstal ve starém dirty worktree.
Práce pokračuje odděleně v work-s31b5 od ověřeného0eafffd3. Goal ACTIVE,
G04/G05 OPEN, S32 BLOCKED; bez push/PR. Viz [S31b5a](steps/S31b5a.md).

## S31b4c1 — ZIP skutečně rozbaluje do dlouhého Unicode cíle (2026-10-25)

Opraven konkrétní ZIP ExtractFiles MAX_PATH limit zjištěný v S31b4b. Nový
preflight ověřuje úplný cíl před in-place assembly, skutečnou UTF8/UTF16
kapacitu, RootLen hranici a bezpečné relativní komponenty. Neodstraňuje limit
bez náhrady: odmítá dot/traversal, streams, device a nejednoznačné názvy.
Atributy a mazání nedokončeného výstupu používají wide extended APIs;
SafeFileCreate overwrite/skip i engine CRC/encryption rozhodnutí zachovaná.
Native actual guard/metadata/delete85 kontrol/config D/R PASS; semantic
mutace návratu MAX_PATH85/2 a odstranění component guardu85/10 selhání.
ZIP+host D/R x64 /warnaserror Build PASS; ZIP comment17/SFX26/listing151Debug,
138Release a encoding contract PASS. Actual GUI Debug dark/Release light:
Cancel bez výstupu a ZIP payload.txt/sub/žluť.txt do nového cíle366/368 UTF16,
876/878 UTF8, s přesným readbackem a nedotčenými zdroji. LongPathsEnabled0.
Branch Unpack stále BLOCKED; G04/G05 OPEN, goal ACTIVE. Navazuje S31b4c2
izolace long-source startupu, potom modal/backend guards a plný Branch runtime.
Bez push/PR/remote CI/CodeQL/clean full suite. Viz [S31b4c1](steps/S31b4c1.md).

## S31b4b — Unicode cílový dialog a bezpečný úklid Unpack (2026-10-25)

Dialog přenáší úplnou Unicode cestu s explicitní byte kapacitou a odmítne
overflow/lossy vstup před změnou caller path, volby backendu či delete flagu.
Řadič používá heap scratch, owned UTF8 source/target default, wide subject,
explicitní kapacitu resolveru a CheckAndCreateDirectoryW; newDir se nezkracuje.
Scoped wide cleanup odstraňuje jen prázdné běžné složky vytvořeného stromu,
ne soubory ani reparse adresáře, bez změny process current directory.
Native actual dialog38/cleanup22/source54 kontrol/config D/R PASS; mutace
truncation38/4 a junction22/3 selhání. Host D/R x64 /warnaserror Build PASS,
operations All20/transitions28 PASS. Skutečné GUI Debug dark/Release light:
dlouhá cesta v dialogu + Cancel, Unicode kratší target a přesné ZIP payloady.
Debug dlouhý target365 UTF16/875 UTF8: host vytvořil celý strom, ZIP plugin
jej explicitně odmítl MAX_PATH limitem; potvrzený Cancel uklidil nově vytvořené
prázdné složky a zachoval zdroje. To NENÍ long-target unpack PASS ani přijaté
konečné omezení. Backend oprava/validace a původní long-source startup
diagnostika navazují v S31b4c před aktivací Branch Unpack. G04/G05 OPEN,
goal ACTIVE, S32 BLOCKED; bez push/PR/CI/CodeQL/clean full suite.
Viz [S31b4b](steps/S31b4b.md) pro přesný rozsah a evidenci.

## S31b4a — owned exact archive source pro Unpack (2026-10-25)

Unpack uloží skutečnou GetItemFullPathW identitu před modalem a předá ji
ExecuteUnpacker jako vlastněný UTF8 řetězec; odstraněno root+basename skládání
po dialogu. Neplatný/lossy převod či překročení legacy byte kapacity se odmítne
lokalizovanou chybou, ne zkrácením. Branch admission se nemění.
Native actual source/conversions + skutečné file I/O54 kontrol/config D/R PASS;
negativní root-alias mutation54/24 semantic failures. Host D/R x64 Build
/warnaserror PASS; operations All20 a transitions28 PASS. Běžný ASCII panel
GUI ZIP Cancel/Unpack D/R PASS včetně Unicode jména uvnitř ZIP a přesného
obsahu, zdrojové archivy/kontrolní soubory beze změny. Long-source GUI zatím
NOT_VERIFIED: spuštění s dlouhým Unicode -l nevytvořilo targetable window,
zatímco ASCII start stejné binárky prošel. Příčina není zjištěná, nelze z toho
odvodit nepodporu samotného Unpack helperu. Další S31b4b řeší cílový dialog,
scratch kapacity a před long GUI izoluje tento startup problém. S31b4c pak
modal/backend guards a aktivace. Viz [S31b4a](steps/S31b4a.md).
Unpack část G04 OPEN, goal ACTIVE, S32 BLOCKED; bez push/PR a bez CI/CodeQL.

## S31b4 — přímé rozbalení, pořadí podetap (2026-10-25)

1. S31b4a: owned exact source archive identity před modalem, přenesená beze
   skládání root+basename až do ExecuteUnpacker. Native actual helper + skutečné
   souborové fixture/alias readback; D/R host a ordinary unpack smoke. Branch
   vstup zatím odmítnutý, žádná aktivace neověřené operace.
2. S31b4b: cílový CUnpackDialog::Transfer odstraní MAX_PATH/ANSI zkrácení;
   heap-backed path scratch, správné kapacity SalGetFullName, owned cleanup
   a wide filename display. Vyřešit také fileName[MAX_PATH] v Unpack pro
   UTF8-byte-long basename. Native dialog/overflow/Unicode a ordinary D/R GUI.
3. S31b4c: modal source/target identity guards, ověřený ZIP/7-Zip backend
   admission, reentrantní InitDLL/plugin callbacky, skutečné D/R Branch unpack
   Cancel/Apply/masks a případná delete-source větev jen po zvláštní validaci.
   Nepodporované externí/unknown backends se nepovolují. Teprve tato brána
   uzavře Unpack část G04; předchozí podkroky ji nezavírají.

Konkrétní blokace: donor i target Unpack skládají archive subject z GetPath
+file->Name; CUnpackDialog::Transfer čte IDE_PATH pomocí MAX_PATH a ANSI
zpráv, přestože controller alokuje SAL_MAX_PATH. SOURCE/CONTRACT piny beze změny.
Goal ACTIVE, bez push/PR, S32 BLOCKED.

## S31b3b2 — guarded Edit Tags and Windows Properties (2026-10-25)

CM_EDITPROPERTIES je v experimentálním Branch View aktivní. Dialog vlastní
wide cesty, před otevřením a každým zápisem ověřuje root/tab/generation/výběr;
změněný výběr ukončí zbývající zápisy jako Cancel. Chybové hlášení používá
lokalizovaný dynamický wide text s úplnou cestou a skutečným HRESULTem.
Native dialog78, selection22, router179 a skutečné metadata I/O107 kontrol/config
D/R PASS; properties14/All20/transitions28 a explorer-columns contract PASS.
GUI Debug dark/Release light: Cancel, long Unicode single Apply, dvě same.jpg
z různých rodičů, běžný panel a read-only failure0x80030005 ověřené skutečným
readback/pixely/kontrolními soubory. Doplněno správné párování sledované SVG
bitmapy; po této jediné pozdější runtime změně Debug Cancel/multi/shutdown
znovu PASS, kompletní Release matice na finální binárce PASS. Host D/R x64
Build /warnaserror PASS; nikoli čistá sestava celé sestavy. Viz
[S31b3b2](steps/S31b3b2.md) pro přesné vazby testů a omezení.
Properties část G04 RESOLVED_LOCAL. Další S31b4 přímé rozbalení, S31b5 kontexty
potom S31c/G05; goal ACTIVE, S32 BLOCKED. Bez push/PR; CI/CodeQL NOT_RUN.
Starší záznamy níže zachovávají stav tehdejších checkpointů.

## S31b3b1 — skutečný Windows metadata I/O preflight (2026-10-25)

Nezměněné donor/target Read/WriteTags, Read/WritePropertyText a capability
funkce ověřené proti skutečným Windows property handlers:107 kontrol/config
D/R PASS. Unicode/emoji, dvě same.jpg z různých rodičů, UTF8-byte-long jméno,
307–308 UTF16 long cesta, write/read/clear, pixels intact, sentinel byte-for-byte
beze změny. Read-only odmítnut 0x80030005, unsupported.txt0x80004005;
žádný fake COM handler ani registry změny. Negativní reroute do vlastního
sentinelu má107 kontrol/22 behaviorálních selhání.
Host D/R Build /warnaserror, properties14, explorer-columns contract a
transitions28 PASS. Přidána jedna runner registrace, žádná původní odstraněná.
Produkční kód beze změny; není to GUI aktivace. Viz [S31b3b1](steps/S31b3b1.md).
Navazuje S31b3b2: modal/write guards, přesný chybový display a GUI aktivace.
G04 Properties/G05 OPEN, goal ACTIVE, S32 BLOCKED, bez push/PR.

## S31b3b — Windows Properties, pořadí podkroků

1. S31b3b1: actual metadata I/O preflight. Registrovaný native harness skutečných
   Read/WriteTags a Read/WritePropertyText + capability funkce proti Windows
   property store. ASCII, Unicode/emoji, stejné basename v různých rodičích,
   byte-long i full UTF16-long path; write/read/clear, nedotčený sentinel,
   odmítnutí unsupported/readonly/missing, skutečný handler bez stubs. D/R buildy.
2. S31b3b2: owned selection guard před dialogem i zápisem, bezpečné hlášení
   dlouhé Unicode chybové identity, command admission a D/R GUI Cancel/Apply
   single/multi + běžný panel. Zachovat write capabilities a Windows chyby.

CM_EDITPROPERTIES zůstává do druhého ověřeného podkroku v Branch odmítnutý.
Výsledek I/O preflight není důkaz GUI aktivace; goal ACTIVE, G04/G05 OPEN.

## S31b3a4b — NTFS dialogy a příkazy Branch View (2026-10-25)

Aktivní Compress/Uncompress/Encrypt/Decrypt i NTFS volby Change Attributes;
stejné root/tab/generation/owned-selection guards platí i pro rychlé příkazy.
Dynamický wide confirmation používá přesnou relativní identitu a existující
lokalizované otázky/varování. Po Cancel se neprovede operace ani nezmění výběr
jiného tabu. Native confirmation52, guard22, router178, NTFSworker160 a
attrsworker73 kontrol/config D/R PASS; All20/Transitions28/Branch19 PASS.
Skutečné GUI D/R: single long Cancel, dvě same.txt z různých rodičů při všech
čtyřech NTFS operacích, kontrolní soubory/obsah beze změn, atributy vráceny32.
Host D/R Build /warnaserror PASS; nikoli čistá sestava celé sestavy.
Viz [S31b3a4b](steps/S31b3a4b.md). Attributes část G04 RESOLVED_LOCAL;
Properties, unpack, zbývající kontexty a G05 dále OPEN. Goal ACTIVE, S32 BLOCKED.
Bez push/PR, remote CI/CodeQL NOT_RUN. Registry/security policy beze změny.

## S31b3a4a — přesné NTFS worker operace (2026-10-25)

Compress/Uncompress/MyEncrypt/MyDecrypt nově přebírají optional owned wide cestu;
DoChangeAttrs ji předává bez návratu k narrow display/mirror cestě. Opravené
čtení/obnova časů, chybové otevření a dočasné Read-only/restore propagují chyby
Windows. SYSTEM potvrzení i Retry/Skip/Cancel zachované. Native skutečného
workeru160/config D/R PASS; parent ve stejné sadě31 assertion failures.
Skutečná NTFS komprese/dekomprese a EFS encrypt/decrypt Unicode/long souborů
ověřené. Trailing-dot EFS přímé Windows API odmítá error5 a worker stejnou chybu
předá bez alias fallbacku. Host D/R Build /warnaserror, původní attrs73/config,
All20/Transitions28 PASS. Viz [S31b3a4a](steps/S31b3a4a.md).
GUI volby/příkazy stále staged; navazuje S31b3a4b confirmation/display a GUI
aktivace, potom další core části. G04/G05 OPEN, goal ACTIVE, S32 BLOCKED.
Bez push/PR, remote CI/CodeQL/clean full suite NOT_RUN.

## S31b3a3 — chráněný dialog atributů v Branch View (2026-10-25)

CM_CHANGEATTR je zapojen; vlastní snapshot root/tab/generation/exact paths
brání použití změněného výběru po dialogu a přípravě skriptu. Cleanup neoznačí
jiný tab/nový výběr. Native guard22, worker73, getters/planner399 a router174
kontrol/config D/R PASS;4 semantic mutation failures ověřené. Host D/R Build
/warnaserror a GUI exact long apply/Cancel PASS; detail verzí [S31b3a3](steps/S31b3a3.md).
Volby komprese/šifrování i jejich rychlé příkazy jsou zatím zakázané pro Branch,
se zachováním jejich současných hodnot. Zbývá S31b3a4 NTFS ověření/aktivace;
pak S31b3b Properties, S31b4 unpack, S31b5 kontexty a S31c. G04/G05 OPEN.
All20, Branch19 a transitions28 PASS; transition očekávání nově obsahuje již
převzaté atChangeAttrs a původní cache guard. Goal ACTIVE, S32 BLOCKED.
Lokálně bez push/PR, remote CI/CodeQL/clean full suite NOT_RUN.

## S31b3a2 — chyby změny času v attribute workeru (2026-10-25)

Při ověření S31b3a se prokázala preexistující chyba: DoChangeAttrs ignoroval
GetFileTime/SetFileTime i selhání obnovy Read-only a mohl hlásit úspěch.
Oprava zachová Windows chybu přes CloseHandle/obnovu atributů, zastaví zápis
po neúspěšném čtení a použije původní Retry/Skip/SkipAll/Cancel tok.
Native skutečného workeru73/config D/R PASS; parent má12 assertion failures
ve stejné sadě. Skutečné wide I/O pro Unicode/long soubory i adresář, timestamps,
Read-only a nedotčený soused; UI/error injection facades jsou výslovně označené.
Host D/R /warnaserror, registrace native D/R a All20 contracts PASS.
Viz [S31b3a2](steps/S31b3a2.md). ChangeAttr/host guard stále blokují; následuje
S31b3a3 modal/selection a GUI aktivace. G04/G05 OPEN, goal ACTIVE, S32 BLOCKED.
Bez push/PR; remote CI/CodeQL/clean full suite NOT_RUN.

## S31b3a1 — příprava přesných zdrojů atributů (2026-10-25)

BuildScriptMain nově připouští atChangeAttrs v existujícím donorovém per-item
wide parent/name bloku. Zůstává kontrola aktuálního cache řádku i CWD cleanup;
ostatní dosud nepřevzaté akce jsou odmítané. Příkaz ChangeAttr a host router
zůstávají beze změny a blokované do ověření celé navazující operace.
Native skutečného bloku 399/config D/R PASS (34 nových assertions); stejný test
proti parent kódu skončí očekávanými32 assertion failures. Host D/R warnaserror,
All20 operation contracts a router173/config PASS. Jde pouze o ověřený přípravný
checkpoint, nikoli dokončení změny atributů či GUI/worker certifikaci. Další krok
S31b3a2: dialog/snapshot, worker včetně adresářů a callbacků, poté GUI aktivace.
Viz [S31b3a1](steps/S31b3a1.md). Goal ACTIVE; G04/G05 otevřené, S32 BLOCKED;
NOT_PUSHED, remote CI/CodeQL a čistá celá sestava NOT_RUN.

## S31b2 — cíl zástupce a reparse bodu (2026-10-25)

Donorový FocusBranchShortcutTarget je zapojen do Ctrl+T a host routeru pouze
pro tento příkaz. Vlastněné wide identity a kontroly zdroje/cílového tabu po COM
callbacku, původní Windows Resolve flags, filesystem PIDL a úplný název cíle.
Native92/config, host Debug/Release buildy a router173/config PASS. GUI D/R
potvrdilo přesný dlouhý adresář, dva same.lnk a Unicode focus i odmítnutí neplatné
LNK; pět případů/config je doložených v [S31b2](steps/S31b2.md). Bez změny registrů,
zabezpečení, SDK ABI či veřejné gate. G04 archive/shortcut části vyřešené lokálně;
zbývá S31b3a/b atributy/Properties, S31b4 unpack a S31b5 destination kontexty,
potom S31c/G05. Goal ACTIVE, S32 BLOCKED. NOT_PUSHED, remote CI/CodeQL a čistá
celá sestava NOT_RUN.

## S31b1a — oprava vlastnictví wide názvů 7-Zip (2026-10-25)

Srovnání se základem před S31b1 potvrdilo starší donorový únik: 7zip listing
alokoval NameW, host však kvůli legacy ABI kopíruje pouze historická pole.
Při úspěšném AddFile/AddDir tak chybělo uvolnění. Lokální scoped guard drží
vlastnictví tohoto nepřebíraného vstupu; chybové cleanupy zůstávají zachované.
Nativní test skutečné AddFileDir, cleanupu a host projekcí: 185 kontrol/config
PASS. Negativní odstranění guardu únik detekuje. Instrumentované ordinary i
Branch long GUI po opravě končí bez heap zprávy a bez dodatečného CRT reportu.
Filtr/assertions se nemění. Podrobnosti a finální build/GUI hranice:
[S31b1a](steps/S31b1a.md). S31b1 archive-entry nález je tímto opraven lokálně;
G04 ostatní core operace a G05 zůstávají otevřené. Goal ACTIVE, S32 BLOCKED.
Následuje S31b2 cíl zástupce. NOT_PUSHED; remote CI/CodeQL/čistá celá matice NOT_RUN.

## S31b1 — otevření souboru jako archivu podle obsahu (2026-10-25)

Donorové předání přesné wide identity do archive probe je doplněné. Vlastněné
cesty a kontrola generace/cache po callbacku brání použití změněného řádku.
GUI našlo tři malé buffery v záhlaví, titulku a při odpojování pluginu; tyto
lokální opravy používají dynamickou paměť. Native267/config a host Debug/Release
PASS; šest funkčních GUI případů/config PASS před poslední opravou unload,
finální unload GUI je veden zvlášť v [S31b1](steps/S31b1.md).
Funkční archive entry je doplněné; S31b1 má otevřený heap nález při shutdown.
Nejbližší S31b1a musí určit příčinu proti ordinary 7zip/baseline a případně ji opravit.
G04 celkově zůstává otevřený.
Dále S31b2 cíl zástupce, S31b3a/b atributy/Properties, S31b4 unpack a S31b5
kontexty cíle; poté S31c. Goal ACTIVE, S32 BLOCKED. NOT_PUSHED; remote CI,
CodeQL a čistá celá sestava NOT_RUN. Bez změny Windows zabezpečení či registrů.

## S31a3b dokončena lokálně — G03 uzavřen (2026-10-25)

Obě dotčené source suites nyní běžně spouštějí celý rozsah: operations All 20 a
Branch View 19 testů včetně temporary selection. Stejně prošly skutečné runner
registrace a unittest discovery. Žádná původní assertion se nezměnila; rename
parent notifikace se navíc v All již nevynechává. Native20/365 kontrol/config a
host Debug/Release buildy PASS. [S31a3b](steps/S31a3b.md) obsahuje důkazy a hranice.
NOT_PUSHED, remote CI/CodeQL/čistá celá matice neběžely; goal S31 ACTIVE.

Následuje S31b1 archive entry, S31b2 shortcut target, S31b3a/b atributy/Windows
Properties, S31b4 unpack a S31b5 donorové destination kontexty; potom S31c.
Uživatel požaduje core operace doplnit, ne uzavřít jejich odmítání dokumentací.
G04/G05 a S32 BLOCKED trvají; nejde o povolení neověřených příkazů či pluginů.

## S31a3a dokončena lokálně — Shell retry guard (2026-10-25)

Druhý G03 error byl skutečný chybějící donorový post-retry guard, nikoli pouhý
stale selektor. Obnoven guard ContextMenu/h a quiet full-selection retry.
Původní assertion prošla beze změny, nové native testy 20 kontrol/config a oba
host buildy Debug/Release PASS; GUI open/cancel v obou konfiguracích PASS.
Podrobnosti: [S31a3a](steps/S31a3a.md). All scope má nyní jediný zbývající
clipboard selector error; G03 zůstává otevřen. Následuje S31a3b aktivace All a
temporary selection. NOT_PUSHED, remote CI/CodeQL/čistá celá matice neběžely.

Uživatel zvolil pro S31b doplnění donorových core operací po malých ověřených
krocích. Jejich odmítání nelze vydávat za dokončený port; proces gate, bezpečnost
Windows a whitelist ostatních příkazů zůstávají. Goal S31 ACTIVE, S32 BLOCKED.

## S31a3 rozdělena po kontrole skutečného Shell retry (2026-10-25)

Nové zjištění: druhý G03 error je skutečně chybějící donorová post-retry validace
ContextMenu/h, nikoli pouze stale selektor. Nejprve S31a3a obnoví kontrolu a tichý
retry a ověří skutečný control flow při COM/menu failure, Debug/Release buildy.
S31a3b potom opraví jen clipboard selector, zapne All a temporary selection test,
zachová dosavadní assertions včetně post-retry guardu. Starší auditní označení
obou chyb jako selector-only je tímto opravené; G03 a S32 zatím zůstávají otevřené.

## S31a2 dokončena lokálně — G02 uzavřen (2026-10-25)

ANSI rename fallback v Branch View nyní přesně donorově přechází do wide metody
před legacy mask/root kódem. Debug/Release x64 host + dependencies s warnaserror
PASS, vynucená ANSI editace/Enter a skutečné wide I/O 44 kontrol/config PASS,
365 getter/remap kontrol/config a 28 transition testů PASS. Skutečné GUI ordinary
i Branch rename prošlo v obou konfiguracích; stejnojmenný soubor zůstal nedotčený.
Podrobnosti: [S31a2](steps/S31a2.md). Opravena také čitelnost českého S31a1 textu.
NOT_PUSHED; remote CI/CodeQL/čistá celá matice neběžely. Goal dokončení S31 je
aktivní; následuje S31a3, poté S31b/c. G03–G06 a brána S32 zůstávají otevřené.

## S31a1 dokončena lokálně — G01 uzavřen (2026-10-25)

SalCreateDirectoryEx nyní přesně donorově deleguje do WorkerCreateDirectory.
Debug/Release x64 host se závislostmi a warnaserror PASS; skutečné host+SDK
wrapper testy 68 kontrol/config, worker/copy a SafeFile regrese PASS. GUI MakeDir
v tmavém Debug a světlém Release vytvořilo přesný Unicode podadresář, kontrolní
soubor beze změny. Podrobné příkazy, výsledky a hranice: [S31a1](steps/S31a1.md).
Změna je lokální, NOT_PUSHED; remote CI/CodeQL ani čistá celá matice neběžely.
Následuje S31a2 (ANSI rename fallback); G02–G06 nadále blokují S32. Starší auditní
indexy a odstavce popisují snapshot před touto opravou, ne aktuální otevřený G01.

## S31 audit dokončen s nálezy; S30 přijata (2026-10-25)

S30 `58aabd60054f98d964f2be7b93bbf408ac692dde` přijata: všech 13 kontrol
[PR #62](https://github.com/majklvi/salamander/pull/62) SUCCESS včetně skutečného
CodeQL a čisté podporované Debug matice. Interní draft se nemerguje do reference.
S31 je dokumentační AUDIT_COMPLETE_WITH_FINDINGS; **S32 BLOCKED**.
[COVERAGE.md](COVERAGE.md) a tři úplné TSV indexy klasifikují 231 donorových cest,
892 hunks a 190 dalších target cest. Nejde o potvrzení úplné funkční parity.

Další nejmenší krok je S31a1: zapojit živý SalCreateDirectoryEx do wide helperu.
Potom S31a2: bezpečný ANSI inline-rename fallback; S31a3: úplný test scope a dva
stale selektory; S31b: rozhodnutí o bezpečně odmítnutých operacích; S31c: uzavření
skutečných validačních mezer/limitů před S32. Podrobné akceptace jsou v COVERAGE.
Tento commit pouze dokumentuje audit: bez produkčních změn a bez push/nového CI.
Starší níže uvedené stavy publikace jsou historické a tímto záznamem nahrazené.

## S30 dokončena lokálně (2026-10-25)

Přijatý základ S29 d6cff6b8 má všechny kontroly PR #61 včetně skutečné CodeQL
analýzy a čisté podporované matice SUCCESS. S30 propojuje donorové rekurzivní
watchery s již převzatým schedulerem/publikací. Lokální owned notification queue
odstraňuje truncation MAX_PATH; automatické zprávy nesmějí obnovit Stop Scan.
Debug/Release host se závislostmi, relevantní nativní testy, 28 source/menu skupin
a skutečné tmavé/světlé GUI PASS. Přesné scénáře, měření a omezení: [S30](steps/S30.md).
NOT_PUSHED; remote CI/CodeQL a čistá celá matice pro S30 NEBĚŽELY.
Následuje S31 audit úplnosti; není součástí tohoto checkpointu.

## S29 dokončena lokálně (2026-10-25)

Přijatá S28a 635f42c5 má úspěšné CI, CodeQL i čistou matici v PR60.
S29a/b/c jsou LOCAL_VALIDATED: donorová tab/history integrace, oba host buildy,
nativní regrese a všech 28 source/menu skupin PASS. Skutečné GUI prošlo 16 scénářů
v tmavém Debug a 16 ve světlém Release, včetně obnovené historie s vypnutým gate.
Úplné identity, oddělený fokus/výběr, layouty a původní sort zachovány; SDK beze změny.
[S29](steps/S29.md) uvádí výsledky a omezení persistence. Přístup k ploše byl
obnoven a GUI blokace vyřešena. NOT_PUSHED; remote CI/CodeQL/čistá celá matice S29
NEBĚŽELY. Další etapou je S30, kterou tento checkpoint neimplementuje.

## S28a — preference zobrazení Path, lokálně ověřena (2026-10-25)

Uživatelské rozšíření je v Options > Configuration > Panels (Panely): relativní
cesta je volitelná, absolutní výchozí. Běžné uložení konfigurace platí pro oba
panely; operace dál používají úplné cesty. Debug/Release x64 host a závislosti,
288 nativních kontrol na konfiguraci, 28 source/menu skupin a skutečné GUI
včetně restartu a Cancel prošly. SDK, donor, cache identita a systémové nastavení
nezměněny. [S28a](steps/S28a.md) obsahuje přesné příkazy, důkazy a omezení.
S28a NOT_PUSHED: remote CI/CodeQL/čistá celá matice zatím neběžely. Předchozí S28
byla mezitím publikována v draft PR #59; níže uvedený stav před publikací je
historický, výsledky jeho CI se zde netvrdí. Další portovací etapa je S29.

## S28 dokončena lokálně (2026-10-25)

S27f.01–.14 včetně podetap bylo mezitím publikováno a ověřeno v interních draft
PR #42–58: všech 17 aktuálních checkpointů prošlo CI/CodeQL a čistým podporovaným
sestavením (externí evidence S27f-publication-goal/completion-audit.json). Uživatel
přijal kód S27f.14 `7ca7844e`; uzavírací dokumentace `15ecc41c` zůstává zachována.
Starší tvrzení o neprovedené publikaci níže jsou historická.

S28 zachovává zvolený režim při vstupu/výstupu a povoluje existující příkazy
šablon a vlastnostního řazení nad workerem a přípravou již převzatými v S26a/b.
Debug/Release x64 host se závislostmi, 51 nativních kontrol na konfiguraci
a všech 28 source/menu skupin prošly; skutečné GUI ověřilo odlišné stejnojmenné
obrázky, dlouhou Unicode cestu, řazení, fokus, refresh a zobrazení v tmavém Debug
i světlém Release. Debug shutdown upozornění zůstává výslovnou výhradou.
Podrobné výsledky, příkazy a evidence: [S28](steps/S28.md). Experiment je výchozí
vypnutý, LongPathsEnabled=0, SDK a bezpečnostní kontroly nezměněné.

S28 zatím není pushnutá; remote CI, CodeQL a čistá celá matice pro ni NEBĚŽELY.
Další implementační etapa je S29, mimo tento goal.

## S27 uzavřena lokálně s popsanými omezeními (2026-10-24)

Aktuální stav: S27a–d jsou předchozí přijaté checkpointy; S27e1/e2 a jednotlivé
S27f.01–.14 jsou lokálně dokončené v samostatných navazujících commitech.
PAK Optimize zůstává skrytý podle výslovné volby uživatele. ZIP Repair je skrytý
i v připnutém donoru, nemá aktivní implementovanou nabídku a není povolen.
Nejde o nehotové obnovení těchto nabídek. Aktuální přehled, přesná SHA, podporovaná
hranice a výhrady jsou v [uzavření S27](steps/S27-completion.md).

Závěrečný audit ověřil všech 19 nových checkpointů od přijatého S27d df01f687,
238 vazeb na soubory v commitech a 225 hashů evidence. Donor a SOURCE.json
zůstaly připnuté, donorový strom je čistý, veřejné hlavičky SDK se nezměnily.
Experimentální gate zůstává výchozí vypnutý; LongPathsEnabled=0. Poslední
Debug/Release ověření S27f.14: host/PictView/Salamatrix a závislosti sestavené,
172 nativních kontrol/konfiguraci, všech 28 source/menu skupin a skutečné GUI
nápovědy/konfigurace prošly. Dřívější operace mají vlastní přesnou evidenci;
závěrečný audit není nové spuštění všech historických GUI/buildů.

Publikace není provedena. Remote CI, CodeQL a čistá celá podporovaná sestava
pro S27e/f zatím NEBĚŽELY. Známé baseline diagnostiky a dílčí engine limity
zůstávají výslovně zaznamenané, žádná kontrola nebyla vypnuta. Další nejmenší
krok je samostatně autorizovaný publikační checkpoint S27e s čistým CI/CodeQL,
pak postupně jednotlivé pluginové skupiny. S28 je další implementační etapa;
S28–S32 nejsou součástí tohoto dokončeného lokálního goalu.

Následující starší záznamy jsou historické checkpointy. Jejich tehdejší „next“,
„pending“ a „unfinished“ se čtou v kontextu data a novějšího přehledu výše.

## S27f.14 auxiliary refresh/help locally validated (2026-10-24)

Only bundled PictView197 and current registered named Salamatrix0x61ffffff are
newly admitted. Unknown DLL/IDs, unnamed or stale framework items stay refused.
Actual PictView refresh body and OnConfiguration are exact donor: force-refresh
only Windows disk panels; no virtual-panel mutation or image writes. Private
Salamatrix API-help resolver now uses heap-wide module/reference identity, wide
extended-length attributes and strict bounded UTF8 conversion, rejecting missing,
truncated or oversized paths and allocation failure. Existing configured viewer,
menu state, SDK/vtables and all five provider capability surfaces unchanged.

Actual policy/posted routes plus real refresh/helper bodies:172 checks/config
Debug/Release PASS. Module path and panel refresh use explicit recording facades;
reference-file attributes use real own filesystem, including long Unicode paths.
D/R host, PictView and Salamatrix/dependencies /warnaserror builds PASS; existing
Salamatrix regression contracts PASS; all28 registered source/menu groups PASS.
Real D/R GUI opens shipped local API reference in configured WebView2 viewer.
PictView opens ordinary JPEG in isolated profile, then configuration is changed
while active source is Branch with duplicate leaves/long100-CJK name; Branch
remains functional with4 files. Private IgnoreThumbnails persisted1. GUI does not
measure cache counters/pixel changes; native actual-case checks independently
verify forced disk-only refresh. Core/plugin viewer gating unchanged. Initial
baseline profile F3 used internal text viewer; test-only JPEG association was
then installed only in fresh private profiles. Initial native fixture prefix
and legacy-profile decoding errors were corrected without weakening assertions.

Exact complete fixture inventory/hashes unchanged. Final launch/loaded module
and reference hashes match snapshots; own hosts closed, INIs byte-restored,
LongPathsEnabled0. No security/registry changes, push, PR, remoteCI, CodeQL or
full clean supported matrix. Evidence ../evidence/S27f14. Next whole-S27 closing
audit, including intentional hidden PAK/ZIP Repair and publication boundaries.

## S27f.14 remaining auxiliary commands plan

Audit exact PictView197 (configuration-triggered thumbnail refresh only) and
Salamatrix0x61ffffff (registered local API help only). Admit exact DLL/IDs, help
must remain a current named menu item with actual file/enabled checks. Preserve
unknown commands. Native compile actual refresh body and documentation helpers;
real GUI configuration-triggered refresh with Branch active and ordinary panel,
local help opens configured viewer, D/R host/plugins builds and source invariants.
Help prerequisite: shipped API resolver uses GetModuleFileNameA/GetFileAttributesA;
replace private implementation with bounded heap-wide module/reference path and
checked UTF8 output, explicit failure on capacity/truncation/missing file. No SDK,
runtime wire capability change or network API docs. No selected-file mutation.

## S27f.13 Checksum Focus locally validated (2026-10-24)

Only bundled Checksum auxiliary99 admitted; actual immediate/idle posted contexts
retain event/operation semantics. Verify already owns full UTF8 fileName; Focus
now splits via accepted ChecksumPaths wide Parent/Base and consumes its owned
request, avoiding mutable CutDirectory scratch/root alias. Calculate/Verify and
hash engines unchanged. No public SDK or virtual-layout change.

Host FocusNameInPanel formerly copied MAX_PATH+200 path/name, then legacy message
matched shortened Name into MAX_PATH NextFocusName. Disk panels now reuse S27a1
wide navigation/exact NameW lookup, validating parent existence before leaving
Branch, owning strings across row replacement, preserving case-insensitive name
fallback after exact match. Applies to ordinary disk too: otherwise second Focus
after leaving Branch would hit the same old buffers. Archive/plugin FS routing
remains distinct. Allocation failure refuses navigation; no registry changes.

D/R host and Checksum/dependency warning-as-error builds PASS. Actual checksum
command+host Focus entry compiled in owned-file native harness:82/config PASS;
actual policy/posted routes142/config PASS; all28 source/menu groups PASS.
Native navigation uses explicit panel-storage/switch recording facades with real
wide filesystem attribute checks; existing62 path/worker/parser/I/O tests retained.
Real D/R final GUI: Verify4OK/config, Focus exact long Unicode duplicate parent,
then Focus100-CJK source after panel becomes ordinary. Full directory read back
through Change Directory; full focused name read from Quick Rename and Cancel.
Four final Focus operations; exact complete fixture inventory and all hashes
unchanged, including sources/manifests/unselected sentinels. Final host/plugin
hashes match launch and actual loaded module. Own hosts closed, INIs restored,
LongPathsEnabled0. Initial Branch-only GUI/build was followed by the complete
ordinary-disk refinement and final rebuild/revalidation; not substituted evidence.

No push/PR/remoteCI/CodeQL/full clean supported matrix. Evidence ../evidence/S27f13.
Next PictView config refresh197/framework help audit; whole S27 unfinished.

S27f.13 refinement: after Branch focus switches to an ordinary disk panel, a
second Focus would still use legacy truncated buffers. Apply the same wide route
to all disk panels; preserve case-insensitive fallback after exact match. Archive
and plugin filesystems retain their distinct legacy routing. Rebuild and repeat
final GUI tests; initial Branch-only evidence is not final binary acceptance.

## S27f.13 Checksum Focus prerequisites and plan

Audit: accepted Verify model stores exact owned UTF8 fileName, posted99 is gated.
Legacy FocusNameInPanel truncates path/name toMAX_PATH+200; WM_USER_FOCUSFILE
later matches shortened Name, so100-CJK focus is unreliable. Reuse S27a1 exact
wide directory switch/final NameW matching only when source is Branch; preserve
ordinary/archive/plugin routing. Keep missing directory from closing Branch.
Checksum99 splits its owned path without mutable CutDirectory scratch alias.
Compile actual focus entry and consumer with recording navigation but real wide
filesystem attributes, actual policy/posted routes, D/R host+Checksum builds.
Real Verify Focus must navigate to exact long duplicate parent and100-CJK name;
ordinary Verify/Focus regression and source hashes. No new SDK/vtable/engine.

## S27f.12 Last Command locally validated (2026-10-24)

Experimental WM_COMMAND gate swallowed Ctrl+I before accepted plugin dispatch.
CM_LAST_PLUGIN_CMD now reaches existing router; OnLastCommand validates current
bundled DLL/ID policy before cache/state callbacks in Branch. FindLastCommand
accepts only current named normal items, clears missing/unloaded/removed commands
as donor, excludes separators/submenus. Capture resolved commandID before state
callback so it cannot substitute another LastPlgCmdID. Existing enabled-state and
final dispatch gates remain; ordinary unknown-plugin compatibility preserved.

Actual FindLastCommand/OnLastCommand plus existing policy/dispatch compiled:
140 assertions/config D/R PASS, including stale dynamic removal, unloaded/missing,
separator/submenu/disabled/unknown refusal and ID mutation during state callback.
Host/dependency D/R warning-as-error builds PASS;28 source/menu groups PASS.
Real D/R GUI: 7-Zip Test Archive menu then Ctrl+I over true-long100-CJK Unicode
archive, switch ordinary panel then Ctrl+I uses newly focused ordinary.zip.
Six exact success results; all10 ZIP hashes/20 member hashes/control/sentinel and
inventory unchanged. No plugin engine or public SDK change. Final binary/loaded
plugin hashes recorded, own hosts closed, INIs restored, LongPathsEnabled0.
Native dynamic-menu cases use explicit storage/state/callback facades; no actual
dynamic-plugin GUI removal claim. Build logs predate final test-only additions;
production host source is the final GUI-tested revision throughout those builds.

No push/PR/remoteCI/CodeQL/full clean supported matrix. Evidence ../evidence/S27f12.
Next auxiliary Checksum99/PictView197/framework help audit. S27 unfinished.

## S27f.12 Last Command routing plan

Reproduced Ctrl+I is swallowed by experimental WM_COMMAND gate although accepted
plugin direct dispatch already validates its actual command. Admit only this
router, then validate current loaded DLL/ID and named normal menu membership before
state callbacks. Retain donor missing-command clearing and disabled-state checks.
No ID range admission or new SDK. Compile actual FindLastCommand/OnLastCommand
in native harness including stale/missing/unloaded/unknown/disabled branches;
D/R host builds and real Branch+ordinary repeat of nonmutating archive test.
Auxiliary Checksum99/PictView197/framework help remains next independent unit.

## S27f.11 Undelete locally validated (2026-10-24)

Admit only bundled undelete\undelete.spl Connect1/Restore2; unknown IDs/custom
DLLs remain refused. Accepted S16 owned selection and restore/Connect engines
retained. GUI exposed truncated live-panel filename in Restore label. Private
synchronous dialog now borrows the already-owned snapshot, counts its items,
requests only localized noun/count templates, substitutes exact NameW once and
sets wide label text. Host display-case %s/%S templates handled; literal percent
in names stays literal. No SDK/vtable or operation engine change/new strings.

D/R x64 host and final Undelete/dependencies warning-as-error builds PASS;
71 restore/native and128 actual dispatch assertions/config PASS;28 registered
source/menu groups PASS. Native EFS uses recording stubs, no real EFS import.
Real D/R GUI: long100-CJK Unicode and ordinary unencrypted files copied with
matching hashes; Release Cancel creates no file. Connect retains exact focused
long image path and refuses it with explicit existing legacy-limit error before
image I/O. No physical disk analyzed. Final %S case-template refinement rebuilt
D/R and repeated both long GUI copies into fresh targets with actual loaded
module verification. Earlier ordinary/cancel/Connect evidence keeps prior binary
hashes explicitly. Six resulting outputs,10 sources/control/sentinel unchanged,
exact inventory. Complete label retained in accessibility; existing dialog width
still visually clips very long labels. No claim that entire label fits on screen.

Own hosts closed, original INIs byte-restored, LongPathsEnabled0. No remote
CI/CodeQL or full clean supported matrix yet; no push/PR. Evidence ../evidence/S27f11.
Next auxiliary Checksum99/framework help/PictView config/LastCommand audit.
Whole S27 remains unfinished.

S27f.11 GUI prerequisite: Restore dialog reads legacy live-panel description;
100-CJK Name display alias splits UTF8 and contains replacement character.
Pass existing owned snapshot by private const reference to synchronous dialog;
retain localized descriptor/pattern, substitute exact NameW using wide strings.
Remove narrow fixed filename formatting and verify full label/target in real GUI.
No SDK or operation engine changes, new localized strings or live-panel guessing.


## S27f.11 Undelete validation plan

Reuse exact S16 restore/snapshot/Connect paths and engines. Admit only bundled
undelete\undelete.spl IDs1/2. Verify actual D/R host+plugin builds,68 native
restore/snapshot/I/O assertions/config plus dispatch gates. Real Branch/ordinary
unencrypted source copies into own explicit targets, Cancel and exact source/
output hashes, Connect exact focused long image and explicit legacy-limit error.
Do not analyze physical disks, mount images or claim real encrypted EFS import.
Duplicate names/recursive/collision/capture errors also covered by native harness.


## S27f.10 PAK audit closed with explicit hidden-command boundary (2026-10-24)

User explicitly chose to retain the PAK Optimize menu hidden, as in donor and
baseline. No menu restoration, host allowlist change or temporary GUI adapter.
Connect and Optimize/engine/header/project remain exact pinned donor files.
Current pak_optimize_snapshot_tests.py --configuration Both PASS15/config:
owned snapshot/duplicates/parent notifications/cancel and actual wide OpenPak
missing-input noncreation; normal pack creation behavior retained. Recording
command-engine facades are not actual PAK optimization. S15 actual engine GUI
with temporary registration remains historical evidence only, not current menu
acceptance. Production Optimize is NOT AVAILABLE in ordinary or Branch menus.

This is a documented intentional exclusion, not a newly enabled integration or
a fresh GUI/build/CI success. No production code changed, no new build necessary
for this documentation-only checkpoint. Evidence ../evidence/S27f10. Next .11
Undelete and auxiliary command audit; whole S27 remains unfinished. No push/PR.

## S27f.09 7-Zip Test Archive locally validated (2026-10-24)

Only bundled7zip\7zip.spl ID1 admitted; unknown IDs/custom DLL identities refused.
Accepted S14 TestArchive/ArchiveNameMessage remain exact pinned donor functions;
FOCUSED_ONLY preserves one exact FullPathW even with other selected rows. Earlier
S27e2 wide packing/delete adaptations retained unchanged. No engine/SDK changes.

Real100-CJK GUI found host CalculateStateCache matched shortened file->Name,
losing .zip and disabling the menu. Private FocusedArchiveFormat uses exact NameW
through existing UTF8 conversion, rejects invalid Unicode/allocation failure and
keeps configured archive association; legacy authoritative Name fallback retained.
No arbitrary plugin enablement or guessed parent/name. Native harness compiles
actual helper/conversion and asserts production call site.53 archive checks and
115 actual policy/dispatch checks/config D/R PASS;28 source/menu groups PASS.
D/R x64 host+7-Zip/dependencies /warnaserror builds PASS. Final host rebuilt after
association fix; unchanged7-Zip builds earlier in stage remain valid. Real GUI:
Debug focused100-CJK true-long Unicode path, both distinct-parent same.zip, and
ordinary; Release focused100-CJK long path and ordinary. Six exact success paths,
all10 original ZIPs/20 member hashes/control/sentinel unchanged; exact inventory.
Debug multi-selection still tests focused row only. Cancel/corrupt/capture failure
covered by explicit native engine/snapshot doubles, not real GUI interruption.

Final loaded plugin/host hashes match launch; transient engine staged hashes
recorded without claiming loaded-module snapshots. Own hosts closed, INIs exactly
restored, LongPathsEnabled0. No push/PR/CI/CodeQL/full clean supported matrix.
Evidence ../evidence/S27f09. Next .10 PAK Optimize, .11 Undelete and auxiliary
audit. Ctrl+I produced no result after closed plugin submenu; retain for planned
LastCommand audit, not accepted as functioning here. Whole S27 unfinished.

S27f.09 GUI prerequisite: CalculateStateCache still matches focused file->Name,
whose UTF8 display alias loses .zip for100-CJK names. Use exact NameW through
existing UTF8 conversion, reject invalid wide names/allocation failure; preserve
association configuration and legacy name fallback. No guessed plugin identity.
Compile actual new helper and existing conversion in archive native harness;
rerun host D/R and real GUI before accepting command.


## S27f.09 7-Zip focused archive validation plan

Reuse exact accepted S14 TestArchive and existing engine. Admit only bundled
7zip\7zip.spl ID1, retaining FOCUSED_ONLY semantics even with another row
selected. D/R host and 7-Zip builds, archive native and actual dispatch checks;
real duplicate same.zip distinct parents, long Unicode/100-CJK focused input and
ordinary panel; successful exact result paths, source/member/control invariants.
No packing, arbitrary DLL admission, SDK, engine or localization redesign.


## S27f.08b2 ZIP CreateSFX locally validated (2026-10-24)

Only bundled ZIP CreateSFX1 newly admitted, alongside accepted Test3/Comment4.
Existing S13 owned archive selection and ZIP/SFX engine retained; Repair2 remains
blocked. Exact donor/baseline prerequisites repaired: heap output capacity,
Unicode-created dialog and bounded UTF8 readback, heap progress title, wide final
PE resource/icon I/O and failure cleanup, missing malloc-failure else. Existing
overwrite/compatibility/engine guards retained; resource open rejects the actual
INVALID_HANDLE_VALUE sentinel. No SDK/vtable change or new localized strings.

D/R x64 host and ZIP /warnaserror builds PASS. Actual command/real Unicode edit
26, policy/dispatch107, archive43, comment17 native checks/config PASS;28 source/
menu groups PASS. Existing filename contract selector follows internal helper
linkage; encoding assertions unchanged. Real D/R GUI creates three selected
distinct-parent Branch outputs (duplicate same.zip, 100-CJK basename, true long
Unicode path) plus ordinary output; Release target edited to ordinary-東京.exe.
Cancel creates nothing. Independent verification:8 valid PE outputs, icon3/
group-icon14/manifest24 resources, exact16 member hashes,10 original archives/
20 source members and unselected controls/sentinel unchanged; exact inventory.
Generated EXEs not executed. Final binary/module hashes match GUI launch; INIs
restored, own hosts closed, LongPathsEnabled0. No shutdown warning observed in
these SFX runs; this does not resolve b1's separately reproduced MENU diagnostic.

Standalone shared-resource consumers: Zip2Sfx Debug full build and Release actual
compile/link targets PASS; SfxMake D/R full build PASS. SelfExtr Release/ReleaseEx
English built, fresh SfxMake generated actual English test package. Initial
Zip2Sfx Release normal Build post-packaging failed missing OPENSAL_BUILD_DIR
environment variable; full all-language packaging matrix NOT claimed. No project
or CI checks disabled. Existing package-installation MAX_PATH limits retained;
not a promise of arbitrary long installation/custom icon/package paths, ZIP64,
multi-volume SFX or generated-program execution behavior. No remote CI/CodeQL or
clean whole supported matrix run. Evidence ../evidence/S27f08b2.

Next S27f.09 7-Zip Test Archive, then .10/.11 and auxiliary command audit.
Whole S27 remains unfinished. Local commit only; no push/PR.

## S27f.08b2 SFX dependencies before enablement (2026-10-24)

Inspection traced output through ChangeSfxIconAndAddManifest (chicon.cpp) and
shared/lukas/resedit.cpp BeginUpdateResource. Both still open ANSI paths after
the existing wide CreateCFile writer. Reuse existing ZipPathToWide conversion
for ZIP_DLL resource opens/icon load/cleanup; preserve standalone tool behavior.
Build ZIP plus both standalone resource-editor consumers (zip2sfx/sfxmake).
The only second CCreateSFXDialog caller in repair.cpp is commented-out legacy.
Keep the SFX dispatch gate closed until heap output, full Unicode dialog/readback,
allocation failure and exact D/R real generated PE/ZIP data/cancel tests pass.
No SFX execution, security/registry change or arbitrary plugin admission needed.


## S27f.08b1 ZIP Comment locally validated (2026-10-24)

Private policy now admits bundled zip\zip.spl Comment4 alongside Test3; SFX1 and
Repair2 remain refused. Existing S13 owned command/engine path retained. Fixed
two exact donor/baseline prerequisites: unbounded MAX_PATH caption formatting
and continuing after failed SaveComment reopen. Existing dialog is now created
Unicode, title/path independently decoded with existing UTF8-first/ACP helper;
localized pattern retained, bad_alloc cancels. Comment byte encoding unchanged.

D/R host+ZIP /warnaserror builds PASS. Actual SaveComment/title17, archive menu43,
policy/dispatch102 checks per configuration PASS; all28 source/menu groups PASS.
Real D/R GUI: three selected distinct-parent Branch ZIPs (duplicate same.zip and
100-CJK basename in long Unicode path), ordinary ZIP and Cancel without edit.
Independent byte comparison: only eight EOCD comments changed,20 members and
unselected controls/sentinel unchanged; exact inventory, no residue, registry0,
INIs byte restored and own hosts closed. Initial Unicode caption truncation
reproduced and final W-dialog corrected. Debug shutdown warning remains: actual
candidate and parent ZIP ordinary-dialog comparison both report nine MENU pairs,
18 normal blocks/576 bytes, no other normal allocations. Heap filter unchanged;
temporary instrumentation/source comparison restored exactly before final D/R
rebuilds. Earlier GUI hashes and subsequent final rebuild hashes recorded honestly.

Evidence ../evidence/S27f08b1; see [S27f.08b1](steps/S27f08b1.md). No push/PR,
remote CI/CodeQL/full clean supported matrix. Next .08b2 SFX prerequisite/command,
then .09-.11 and auxiliary audit. Whole S27 remains unfinished.

.08b2 must fix CreateSFX output MAX_PATH, editable target capacity/Unicode
conversion, wide cleanup and missing malloc-failure else before admitting ID1.
Initial audit in baseline-defects.json: donor and parent share these defects.
No long SFX GUI case was run before correcting this dependency.

## S27f.08a ZIP Test Archive locally validated (2026-10-24)

Only bundled zip\zip.spl Test Archive3 is newly admitted. Existing S13 owned
command/engine logic retained; ZIP menu association now uses exact NameW instead
of a shortened display alias. Directories/invalid names/allocation failures are
refused. Comment4, CreateSFX1 and Repair2 remain gated for .08b/audit.

Actual GUI found host message-box hard wrapping split UTF8 bytes. The existing
function now measures wide GDI text and preserves complete surrogate pairs;
body layout uses matching DrawTextW. No new SDK/dialog/localization strings.
D/R host+ZIP /warnaserror builds PASS. Native archive43, policy97 and real-GDI
message wrap22 checks/config PASS;28 source/menu groups PASS. Final actual Debug
GUI tests three Branch archives in distinct parents; Release tests focused long
100-CJK basename with no selection. Ordinary panels PASS in both configurations.
Six exact result paths read from UI, no replacement symbols;10 original archives,
20 independently read member hashes/control/sentinel unchanged, no temp residue.
Own hosts closed normally, INIs exactly restored, registry0. Evidence ../evidence/
S27f08a; see [S27f.08a](steps/S27f08a.md). S27 remains unfinished: .08b next,
then .09-.11 and auxiliary audit. No push/PR/remote CI/CodeQL/full clean matrix.

.08b separately validates Comment4 and CreateSFX1 before enabling them. Accepted
.08a prerequisites: exact ZIP mask input and existing host wide hard wrapping.
Original reproduction retained in Debug-initial-result-3-wrap-defect.txt.

## S27f.07 PictView Update Thumbnail locally validated (2026-10-24)

Only bundled pictview\pictview.spl command188 is newly admitted by the private
Branch policy. Existing S12 donor thumbnail/EXIF10 consumers remain unchanged;
capture/scan/refresh internals, arbitrary DLLs and neighboring IDs remain refused
at dispatch. Some legacy plugin menu items can still appear enabled; this is not
a claim that every refused command is visually disabled. No SDK/layout change.

Host and PictView/EXIF Debug+Release /warnaserror builds PASS. Native policy89,
paired real EXIF51, disk selection57 and WIC39 checks per configuration PASS;
28 source/menu groups PASS. Actual D/R GUI: Cancel preserves inputs, selected
same-name JPEGs in different parents and long Unicode paths receive valid EXIF
thumbnails, original compressed image data/orientation6 remain unchanged. A fourth
ordinary-panel JPEG also succeeds while Branch selection remains on the other
side; unselected zz-keep.jpg and sentinel remain unchanged, no temp residue.
Own hosts closed normally, private INIs restored exactly, LongPathsEnabled0.
See [S27f.07](steps/S27f07.md); evidence ../evidence/S27f07.
S27 remains unfinished: next .08 ZIP, then .09-.11 and auxiliary command audit.
No push/PR, remote CI, CodeQL or full clean-build matrix claimed for this unit.

## S27f.06b Automation menu integration locally validated (2026-10-24)

Private policy now admits fixed Automation1/2/3 and actual currently registered
named loaded/runtime commands of the bundled DLL. Immediate and idle posted
callbacks share the private Branch gate; original NULL/operation contexts and
mask0 remain. Unverified background-origin posted commands are refused while the
active panel is Branch; this is not a deferred retry queue. No SDK/vtable change.

Real GUI exposed repository macro/Unicode manifest intake, missing explicit
manifest command publication and inactive one-shot host dispatch. Adaptations
resolve supported variables at I/O boundaries, preserve wide paths, publish only
on explicit execution, and restore the prior stopping guard on scope exit. Shared
tracked opens now match all text-reader closes, including empty/mapped legacy
scripts; final Debug exits without the intermediate handle-tracker diagnostic.
Capabilities, unload checks and worker protocol remain intact.

Host D/R /warnaserror builds PASS; final Automation D/R3.27s/11.90s PASS. Native
81 policy/dispatch checks/config,26 repository/manifest/lifetime/handle checks/config,
243 immutable consumer checks/config,5 forwarding contracts and28 source/menu
checks PASS. Actual D/R GUI verifies loaded legacy/deferred scripts, Cancel,
registered Python command IDs and exact mixed-parent Unicode/>260 context; ordinary
right-panel smoke PASS. Last handle-close-only patch repeated with final DLLs;
legacy and runtime reports match prior functional results, Debug shutdown clean.
Source/sentinel hashes unchanged, own processes closed, INIs restored, scripts
archived, registry0. See [S27f.06b](steps/S27f06b.md); evidence ../evidence/S27f06b.
S27 goal remains unfinished: next S27f.07, then .08-.11 and auxiliary commands.
No push/PR, remote CI, CodeQL or full clean-build result claimed for this unit.

Completed prerequisites were identified before final acceptance: stored $(SalDir)
paths required expansion for metadata and modern entrypoints; Unicode path equality
required wide canonicalization; one-shot callbacks required scoped live dispatch.
Native regression preserves the original macro failure reproduction. No absolute-
only fixture workaround, autoactivation or runtime public API change was used.

## S27f.06a Salamatrix package integration locally validated (2026-10-24)

Private policy admits only actual registered package commands of bundled
salamatrix\salamatrix.spl. Unknown IDs, submenus/separators and fixed help ID
remain blocked. Consumer runtime/capability/enabled checks unchanged; no SDK edit.
Host/framework/all5 provider D/R builds PASS;64 actual policy/dispatch checks/config,
243 immutable consumer checks/config and28 source/menu groups PASS. Real D/R GUI
Python package captures exact mixed-parent Branch context,255-unit/763-byte CJK
basename and long path; ordinary right-panel source/target resolution PASS.
Actual5 production workers replay each captured context:179 exact typed scalar
fields/provider/config and5 context calls each PASS. Explicit recording transport
facade; this does not claim five GUI/provider-DLL executions. Existing native
process runtime tests D/R including PHP PASS. Files unchanged, private INIs restored,
own processes closed and extensions archived; registry0. [S27f.06a](steps/S27f06a.md).
Goal ACTIVE; next S27f.06b Automation menu integration; remaining plugin units,
Checksum Focus and auxiliary audit pending. No publication or remote CI claimed.

## S27f.06 implementation split (2026-10-24)

S27f.06a: enable only registered package commands of bundled Salamatrix; retain
its manifest capabilities, runtime availability, handler and enabled checks.
Validate actual Branch full-path context across JavaScript/Python/PowerShell/PHP/
Lua with matching expected identities, flags and limits, D/R host/framework and
all5 provider builds, native consumers and ordinary-panel smoke. Unregistered IDs
and unrelated DLL identities remain refused; no SDK change. PHP CLI is isolated
in evidence with official archive SHA256 verification; no system installation.
S27f.06b: audit and enable Automation preloaded/runtime script commands and its
deferred popup; verify registered dispatch and actual script contexts, preserving
lookup/visibility/enabled checks. Do not claim S27f.06 complete after only part a.

## S27f.05 Automation locally validated (2026-10-24)

Only bundled automation\automation.spl RunFocusedScript1 is enabled by the private
command policy. Actual D/R Branch scripts verify16 owned COM lifetime/identity
checks each, including mixed parents, duplicate names, Unicode/>260 paths,
deselect/navigation destroying rows, detached items and retained enumerators.
Ordinary Release focused-script smoke PASS. Host/Automation D/R /warnaserror
builds PASS;55 dispatch and243 Automation checks/config PASS;5 provider contracts
and28 source/menu groups PASS. Private INIs restored, source hashes unchanged,
own processes closed, LongPathsEnabled0. Existing Debug handle-tracker diagnostic
is recorded, not suppressed. See [S27f.05](steps/S27f05.md).
Goal ACTIVE; next S27f.06 Salamatrix actual runtime/provider parity and deferred
Automation menu commands. Checksum auxiliary Focus remains pending. No publication
or new remote CI/CodeQL/full clean supported matrix is claimed.

## S27f.04 Split & Combine locally validated (2026-10-24)

Bundled splitcbn\splitcbn.spl Split1/Combine2 pass the private audited command
policy; other IDs stay blocked. All45 tracked plugin files equal donor47661e92.
Host/plugin D/R /warnaserror builds PASS;53 dispatch and210 real consumer/I/O
checks/config PASS; all28 source/menu groups PASS. Actual D/R Branch GUI Split
reads the >260Unicode source and creates6parts in its actual long parent; ordered
bytes/hash equal source, generated BAT name/CRC independently correct (never run).
Actual D/R Combine of3explicit parts from3parents including long Unicode produces
exact ordered bytes/hash under first input parent. All8sources/2sentinels unchanged.
Release ordinary focused sibling discovery stays in actual parent; Cancel creates
no output. Runtime matches launch, INIs restored, own processes closed, registry0.
No publication/remote CI/CodeQL/full clean matrix. See [S27f.04](steps/S27f04.md).
Goal ACTIVE; next S27f.05 Automation; Checksum auxiliary Focus remains pending.

## S27f.03 Checksum Calculate/Verify locally validated (2026-10-24)

Only bundled checksum\checksum.spl Calculate ID1 and Verify ID2 pass the private
Branch dispatch policy. Auxiliary Focus ID99 stays blocked and is a remaining
S27 integration item; no blanket DLL permission. All58 tracked Checksum files
remain equal to pinned donor47661e92. Host/plugin D/R /warnaserror PASS;
command dispatch50/config, owned-path62/config and all28 contracts PASS.
Actual mixed-parent Branch GUI D/R:40 real digests independently match CRC32/MD5/
SHA1/SHA256/SHA512; two MD5 exports retain all8 full relative paths/digests;
16 positive Verify entries including >260 Unicode manifest parent. Debug negative
exactly1corrupted/3OK/0missing/0skipped; Release ordinary Verify4OK. All sources and
sentinels unchanged, private INIs restored, runtime matches launch, registry0.
No publication, remote CI/CodeQL/full clean matrix. See [S27f.03](steps/S27f03.md).
Goal ACTIVE; next S27f.04 Split & Combine; auxiliary Focus requires separate audit.

## S27f.02 Batch Rename locally validated (2026-10-24)

Bundled renamer\renamer.spl internal Rename ID1 now passes the existing private
Branch command policy. Other IDs remain blocked, including unpublished menu Undo
ID2; Undo runs inside the existing Rename dialog. No SDK or engine change.
All56 tracked Renamer files remain exactly equal to pinned donor47661e92.
D/R host and Renamer /warnaserror builds PASS; command dispatch47, panel67 and
filesystem89 native checks/config PASS; all28 source/menu contracts PASS.
Actual D/R Branch GUI: four mixed-parent files, duplicate Unicode basenames,
>260 UTF16 parent and >260 UTF8 basename; exact preview, Cancel, Rename and Undo
PASS with independent path/SHA256 checks. Outside-selection sentinels unchanged.
Ordinary two-file preview/Cancel PASS D/R. Private INIs restored, runtime hashes
match launch, own processes exited, LongPathsEnabled remains0.
No remote CI/CodeQL/full clean matrix or publication for this unit.
See [S27f.02](steps/S27f02.md). Goal ACTIVE; next S27f.03 Checksum separately.

## S27f.01 File Comparator locally validated (2026-10-24)

Only bundled filecomp\filecomp.spl internal Compare Files ID1 is accepted for
Branch plugin dispatch. Actual registered SUID resolves to that exact command;
hotkey dispatch checks policy before DLL init; direct/menu/toolbar dispatch checks
again before callback. Unknown DLLs/IDs/custom absolute identities stay blocked.
Private CPluginData nonvirtual method only; no SDK/vtable/data-layout change.
The accepted S06 adapter/engine blobs remain exactly equal to the pinned donor.

D/R host/dependency and Filecomp builds /warnaserror PASS. New actual private-policy
and dispatch native45/config; accepted owned-path/service native58/config; all28
source/menu groups PASS. Actual D/R Branch mixed-parent same.txt pair includes
>260 Unicode parent: exact dialog paths, one expected real content difference,
Recompare and Cancel PASS. Debug also exercises submenu/SUID dispatch. Ordinary
source-to-unique-target panel comparison PASS D/R. All6 input/control hashes
unchanged, private INIs restored, clean host/reporter exit/no crash reports.
Known S06 Unicode header rendering limitation remains visibly documented; do not
claim full Unicode output UI support. Last Command remains staged off in Branch.
Two Release attempts hit95s limits; subsequent full build PASS8.40s, failed/time-
limited logs retained. No remote CI/CodeQL/full matrix or publication for this unit.
See [S27f.01](steps/S27f01.md). Goal ACTIVE; next S27f.02 Batch Rename separately.

## S27e2 locally validated; S27e complete (2026-10-24)

Bundled ZIP/7-Zip Move packing is enabled through the same exact-relative callback.
Audit found legacy narrow source deletion: ZIP cleanup now decodes UTF8 first with
ACP fallback and uses extended wide DeleteFileW;7-Zip keeps owned full-path UTF8
and uses its existing long-wide conversion before deletion. Existing success,
Cancel/Skip/Retry and archive-self-lock semantics remain intact; no engine rewrite.

D/R host/dependencies and both archive engines /warnaserror PASS. Actual callback,
packer gate and source-deletion native tests421/config PASS, including real short
and >260 Unicode file removal, unselected sentinel and failure/Cancel. All28
registered source/menu groups PASS. User-confirmed GUI ZIP and7z Move each PASS
in D/R:5 exact relative names and independently decompressed SHA256,5 selected
sources removed, outside sentinel unchanged. Release checked-Move dialog Cancel
preserved all5 source hashes with no output. Private configs restored; policy0.
Debug repeated known heap notification (S22/S27e1; allocation origin not instrumented
this run); Release clean exit/no new crash reports. See [S27e2](steps/S27e2.md).
Unknown/external packers and Branch targets remain unavailable; no SDK/security
change. No push/remote CI/CodeQL/full clean matrix run. S27 goal remains ACTIVE:
next S27f.01 File Comparator, then separately verified plugin integrations.

## S27e1 locally validated — ZIP/7-Zip copy packing (2026-10-24)

CM_PACK now uses the accepted S22 exact-relative callback for disk Branch sources.
Only bundled ZIP/7-Zip copy packers and ordinary disk targets are accepted; Move,
external/unknown plugins and Branch targets stay blocked before archive effects.
Existing allocation-failure paths retain nothrow behavior. No SDK/engine changes.

Host/dependency and both engine builds PASS Debug/Release x64 /warnaserror.
Actual callback/gate native tests377 per configuration and28 source/menu groups PASS.
Real D/R GUI ZIP and7z archives each contain five exact relative file names with
independently decompressed SHA256 matches: duplicate basenames, Unicode parents,
UTF8-byte-long and UTF16-long source paths. Cancel preserves all sources; ordinary
panel7z control PASS both configurations. Private configs restored; policy stays0.
Debug shutdown showed the previously documented heap notification (S22); this run
only observed the notification, without proving identical allocations. Release
clean exit and no new crash report. See [S27e1](steps/S27e1.md).
No push/remote CI/CodeQL/clean full matrix run for this unit. Goal remains ACTIVE:
next S27e2 move-packer cleanup, then S27f.01-.11 separately validated integrations.

## S27d accepted; goal covers remaining S27 (2026-10-24)

User accepted S27d at df01f687f062672cb74a2cc21a9116508593bc36. PR40 current-head
checks, actual CodeQL analysis/upload and clean full supported Win32/x64/ARM64
Rebuild40079394417 all PASS. Accepted local ref refactor/pr-769-incremental advanced
by checked fast-forward from328cffd5; samandarin-0.16, donor and draft PR unchanged.

User explicitly requested a goal through completion of S27, overriding the usual
one-unit-per-run convention: retain one validated coherent commit per unit and
continue locally. No new push/upstream PR authorized. Candidate starts from S27d.

S27e split before activation:
- S27e1: copy-only host packing via bundled ZIP/7-Zip. Real Branch relative identity
  through accepted S22 callback. CM_PACK only, ordinary disk target only; block
  unknown/external packer and Move selection before any file side effect. Native
  callback/gates, D/R host and both engines, GUI real ZIP/7z entries and decompressed
  hashes, duplicate basenames, Unicode/long source, Cancel/untouched-source proof.
- S27e2: separately validate move-packer cleanup, file-link size cache/reset and
  cancellation/failure before enabling deletion. Keep unsupported routes gated.
- Then S27f.01-.11 separately: Comparator, Batch Rename, Checksum, Split/Combine,
  Automation, Salamatrix, PictView, ZIP commands, 7-Zip test, PAK optimize, Undelete.
  Audit pinned donor per unit, actual owned selection results and D/R builds/GUI.
  Unknown plugin ranges remain unavailable; no SDK-layout/security/settings change.

## S27d5b completed locally; S27d goal complete (2026-10-24)

Clipboard COPY/MOVE into Branch uses the actual collection root, independent of
focused file parent. Internal disk drag onto blank background uses that root;
all rows/updir, same-panel drag, external/virtual/archive sources and incoming
links remain rejected. Fake and non-HDROP clipboard objects remain gated. Callback
and queued worker check exact wide root; overcapacity/post/allocation failures
preserve data cleanup. Existing donor native ingestion/wide worker stays in use.

D/R host/dependencies /warnaserror final PASS12.21s/6.87s (initial Debug40.77s,
Release82.14s; final executables match actual GUI binary hashes;
first Release link hit95s deadline, preserved under evidence release-attempt1).
Actual root/queue/receive and all previous native harness361/config PASS; existing
real Shell fixture33/config and all28 source/menu groups PASS. Actual D/R GUI:
3 exact copies, cut/paste1, mouse move1 into >260 Unicode root, row rejection and
overwrite Cancel;7 final files, four original same-name sentinels unchanged.
Known S30 limitation: long target notification requires manual Ctrl+R; verified.
Private configs restored, clean exits, policy0, no security/system setting change.
See [S27d5b](steps/S27d5b.md). S27d1 through S27d5 locally complete within their
explicit staged boundaries. No push/PR/remote CI/CodeQL/full clean matrix run.
Next proposed integration unit S27e; publication remains a separate instruction.

## S27d5b activation boundary (2026-10-24)

Use the real collection root for clipboard paste and internal disk drag onto blank
background only. Reject all rows including updir, same-panel drags, plugin/archive
sources, external drags, links, fake clipboard objects and non-HDROP data. Require
wide root identity again in callback and queued worker, no root+basename inference.
Keep ordinary panels and Windows security unchanged. Native actual-boundary tests
and D/R host/GUI copy, cut/move, overwrite Cancel and rejected-row checks required.

## S27d5a completed locally (2026-10-24)

Selected left/right drag source now uses the existing complete wide IDataObject,
Windows OLE negotiation and scoped DropSourcePanel/COM cleanup. Incoming Branch
operations remain gated until S27d5b. Allocation preserves the existing low-memory
path with scoped nothrow/debug-new handling; no security policy changed.

Host/dependencies Debug/Release /warnaserror PASS13.42s/77.34s; actual-block native
harness340/config, real Shell fixture33/config, all28 source/menu groups PASS.
Actual mouse OLE MOVE D/R transfers4 mixed-parent ASCII/Unicode/long files into
ordinary disk panel, exact hashes, source removal, duplicate sources/sentinel
preserved and Branch auto-refresh to2. Default500ms rejects the instantaneous test
input; owned private Debug delay1ms, Release delay disabled for automation only.
Production defaults unchanged. Clean exits/private INIs restored, policy0.
COPY/right-button/Escape/error/allocation negotiation covered by explicit native
facades, not claimed as real GUI. See [S27d5a](steps/S27d5a.md).
No publication/remote CI. Goal continues with S27d5b destination semantics.

## S27d5 split before activation (2026-10-24)

S27d5a: drag source only, complete selected IDataObject and existing Windows OLE
transfer; native copy/move/cancel/error/lifetime and QueryContinueDrag cases, actual
D/R mouse drag into explicit ordinary disk target with hashes and untouched inputs.
Use nothrow allocation preserving source's existing NULL/cleanup path. Branch
receiving remains blocked in this unit. S27d5b: destination/paste semantics audited
separately; only an explicit real root/folder destination may be accepted, never
root+basename substitution or accidental drop-on-file execution. Default-off gate
and Windows security unchanged, no publication. Goal remains all remaining S27d.

## S27d4 completed locally (2026-10-24)

Selected context menu enabled: one-parent selections use existing exact-wide Shell
menu; mixed parents retain the entire selection and use pinned donor host fallback.
Containing-folder command never reaches a Shell handler. Deferred fallback command
runs synchronously on source panel after Shell release and EndStopRefresh. Background/
collection-root and arbitrary plugin command ranges remain gated, activation off.

D/R host/dependencies /warnaserror PASS15.38s/74.01s; native actual-method/menu harness
327/config PASS; real Shell fixture33/config and all28 source/menu groups PASS.
Actual D/R mixed menu Properties reports6 files/Various Folders216/228 bytes. Single
long Unicode Shell menu, Escape and Open Containing Folder into deep ordinary panel
with correct long filename focus PASS.12 source hashes/sentinel unchanged, private
INIs restored, clean exits, LongPathsEnabled=0. No system/security settings changed.
See [S27d4](steps/S27d4.md), evidence ../evidence/S27d4. No publication/remote CI.
Goal continues with S27d5, split drag source/destination by actual dependencies.

## S27d4 scope before import (2026-10-24)

Import donor TrackBranchHostContextMenu, scoped same-parent containing-folder item,
quiet selected menu capture, mixed-parent fallback and synchronous deferred host
command dispatch after Shell release/EndStopRefresh. Enable only selected context
menu, not collection-root/background menu or arbitrary plugin command ranges.
Native actual menu/gate tests, donor source contracts, D/R host build and actual
GUI same-parent Shell/mixed-parent host menu, Cancel and whole-selection command.
No publication authorized. S27d5 drag/drop remains the next separate unit.

## S27d3b completed locally (2026-10-24)

Paste from Branch clipboard into an explicit ordinary disk destination is verified.
Accepted S21 wide HDROP ingestion and S20 drop I/O are reused. Local dependency fix
replaces three legacy first-source buffers in DropCopyMove with one owned wide-first
UTF8 parent for source display, volume/scheduling metadata and notification input.
No implicit paste/drop into the collection root is enabled. Default-off gate intact.

D/R host/dependencies /warnaserror PASS (9.11s/77.11s), actual-method native harness
320/configuration PASS (8 new metadata cases), real Shell fixture33/configuration
PASS, all28 source/menu groups PASS. Actual D/R Ctrl+C/V copies4 mixed-parent files;
Ctrl+X/V moves the same4 with exact hashes and disappearance of just those sources.
Both duplicate same.txt inputs remain unchanged. Branch listings auto-refresh to2.
Actual D/R overwrite conflict Cancel preserves source copy and existing target.
Private INIs restored and hosts/reporters exit cleanly. LongPathsEnabled=0 unchanged.

See [S27d3b](steps/S27d3b.md); evidence ../evidence/S27d3b. No publication/remote
CI/CodeQL or clean full matrix. S30 notification transport still has its legacy
single-first-parent/short-path limit; other source panels may need manual Refresh.
Next goal units S27d4 context menu, then S27d5 drag/drop/destination semantics.

## S27d3b dependency: drop planner source metadata (2026-10-24)

Clipboard Paste to an ordinary disk destination already uses accepted wide HDROP
parsing and BuildScriptDropCopyMove. Audit found legacy MAX_PATH first-source
buffers in DropCopyMove volume/scheduling/notification preparation. Replace them
with one owned exact UTF-8 parent derived from FileNameW (fallback UTF8/ACP), reused
for metadata. Do not enable paste into the collection root implicitly. Validate
actual Ctrl+C/V and Ctrl+X/V from Branch to explicit ordinary disk destination,
wide long source/destination hashes, source survival/disappearance. Existing S30
notification transport limits remain explicit; manual refresh is allowed there.

## S27d3a completed locally (2026-10-24)

Cut export now enabled through CM_CLIPCUT and selected saCutToClipboard, reusing
accepted S21 donor publication/ghost-state/cleanup and S27d1 exact capture guards.
Native actual-method harness312/configuration PASS; includes complete MOVE export,
failed effect blocking publication, exact cross-panel duplicate identities and
ClearCutToClipFlag. Existing real Shell tests33/configuration and all28 source/menu
groups PASS. D/R x64 host/dependencies /warnaserror PASS (6.92s/75.51s). Real D/R
Ctrl+X ghosted6 selected files and exported exact complete CF_HDROP/CIDA+MOVE;
Ctrl+C restored icons and COPY|LINK, independently inspected8 checks per payload.
Ordinary-panel smoke,12 unchanged source hashes/sentinel, clean exits and restored
private INIs PASS. LongPathsEnabled=0; no system/security change, no paste yet.

See [S27d3a](steps/S27d3a.md); external evidence ../evidence/S27d3a. No publication,
remote CI/CodeQL or clean full matrix for this unit. Goal remains active: next
S27d3b paste/shared-drop dependency, then S27d4 context menu and S27d5 drag/drop.

## Remaining S27d goal: ordered units (2026-10-24)

User explicitly authorized all remaining S27d units in one goal, overriding the
single-unit-per-run convention while retaining a separately validated commit per
coherent unit. S27d3a: cut export and exact cut-state cleanup; native MOVE payload,
exact cross-panel identities, publication failure and real D/R Ctrl+X then Ctrl+C.
S27d3b: paste ingestion to an explicit ordinary disk destination, copy/move hashes,
source disappearance and failure/Cancel; audit shared drop planner before enabling.
S27d4: selection context menu, same-parent Shell and mixed-parent host fallback;
no arbitrary plugin activation, no synthetic-root action. S27d5: drag source and
explicit disk destination, split by direction/dependency. Branch-root destination
must be treated explicitly, never guessed from a row. Default-off process gate,
Windows security and original reference branches stay intact. No push/PR authorized.

## S27d2 completed locally (2026-10-24)

Selected clipboard copy export is enabled after S27d1 d8c28521; candidate
integration/pr769-s27d2-candidate. Reuses accepted S21 complete wide Shell data
object/publication and S27d1 whole-selection identity guards. Only CM_CLIPCOPY /
selected saCopyToClipboard added; cut, paste, context menu, drag/drop remain gated.
Default-off experiment unchanged, no SDK/resources/system/security setting changes.

D/R x64 host/dependency builds /warnaserror PASS; actual-method native harness305
checks/configuration PASS; existing real Shell fixture33/configuration PASS;
all28 source/menu groups PASS. Actual D/R GUI Ctrl+C for six mixed-parent files,
focused long Unicode filename and ordinary ASCII folder passed independent native
OLE inspection (8 checks/case): exact CF_HDROP, complete CIDA identities and COPY|LINK,
never MOVE. Source hashes12/12, sentinel and empty destinations unchanged; private
INIs restored, hosts/reporters exited cleanly, LongPathsEnabled=0. No paste tested.

See [S27d2](steps/S27d2.md) for commands, provenance and limits. External evidence
../evidence/S27d2 binds tested parent/patch, binary hashes and committed inputs.
No push/new PR/remote CI/CodeQL or clean entire supported matrix for this unit.
Next smallest unit S27d3 cut/paste; split export/source-state and ingestion if needed.

## S27d2 planned before activation (2026-10-24)

Only file-selection clipboard copy export, building on S27d1 d8c28521. Enable
CM_CLIPCOPY and selected saCopyToClipboard. Reuse S21 disk IDataObject publication
with COPY|LINK effect and complete validated paths. Cut, paste, drag and menus
remain blocked for Branch; default-off process gate unchanged. Extend actual
publication-block native facade tests for success/effect/publication failures and
stale selection. Existing real Shell payload tests plus real D/R GUI Ctrl+C and
independent clipboard payload inspection; ordinary panel copy and source hashes.
D/R host/dependency builds are required. No push/PR in this unit.

## S27d1 completed locally (2026-10-24)

Properties is enabled for current Branch rows, including mixed-parent selections.
Scoped donor quiet-menu and SHMultiFileProperties fallback hunks reuse accepted
S21 full-path/PIDL/CIDA helpers; local null/index/current-cache/empty-path guards
reject the entire invalid selection. No basename/root reconstruction is used.
Only CM_PROPERTIES / selected saProperties is enabled. Clipboard, cut/paste,
context menus, drag/drop, dedicated security-tab and arbitrary plugin commands
remain gated; experiment activation remains off by default. No SDK/resources change.

Debug and Release x64 host/dependencies /warnaserror PASS. Existing actual-method
harness294/configuration PASS (269 retained +25 Properties/capture/gate checks,
explicit Shell facades); actual branch_operations_tests33/configuration PASS;
all28 source/menu groups PASS. Real D/R GUI single long Unicode file, mixed-parent
6-file selection and ordinary-folder Properties PASS, all closed with Escape/Cancel.
Twelve source hashes and sentinel unchanged, private INIs restored, hosts/reporters
exited cleanly. LongPathsEnabled=0. Native Windows displayed DOS-alias Location for
the long item; independent samefile checks confirmed the exact original parent.
No system/security/global setting was changed. No file attributes/permissions edited.

See [S27d1](steps/S27d1.md) for actual commands, donor mapping and limits.
Evidence ../evidence/S27d1 binds parent328cffd5, tested patch, final inputs and commit.
No push/new PR/remote CodeQL or clean whole-solution matrix for this new unit.
Next: S27d2 clipboard copy export, then S27d3 cut/paste, S27d4 context menu,
S27d5 drag/drop; split further by dependency/direction when needed.

## S27c1a–S27c4 accepted; S27d split before implementation (2026-10-24)

User accepted exact head328cffd557fe588b8eefe05873ac5acf8983bb0f after all PR39
checks and actual CodeQL succeeded, including clean supported Debug Win32/x64/ARM64
full-solution rebuild40029651148. Local accepted ref advances from e63167f0 to this
head. The internal draft remains open and is not merged into the reference base.

S27d is split by independently testable operation:
- S27d1: Properties only. Permit CM_PROPERTIES and selected saProperties; import
  donor quiet menu / complete IDataObject / SHMultiFileProperties fallback hunks.
  Reuse accepted S21 adapters and Shell helpers. Add current-identity/null/index/
  empty-path rejection at the panel capture boundary so no partial selection can
  reach Shell. Archive/plugin routes retain their accepted behavior.
- S27d2: clipboard copy export; exact whole selection and transfer effect.
- S27d3: cut export/paste ingestion and source cut-state cleanup, split further
  if the donor dependencies require it.
- S27d4: selection context menu/host fallback; arbitrary plugin actions stay gated.
- S27d5: drag/drop source and destination, split by direction if needed.
Recycle is already accepted in S27c4 and is not ported again.

This run implements only S27d1. Keep all other Shell actions, clipboard, drop,
security-tab command and plugin ranges blocked for Branch; activation stays off
by default. Native actual capture/fallback blocks plus existing Shell PIDL/CIDA
fixtures; real D/R GUI single long/Unicode file, mixed-parent duplicates, Cancel,
ordinary-panel Properties; owned source hashes unchanged; D/R host/dependency builds.
No publication authorized for the new candidate. Source pins and SDK unchanged.


## S27c1b–S27c4 completed locally (2026-10-23)

All four requested units are implemented sequentially on integration/pr769-s27c-candidate,
with no publication. S27c1b634d3403 adds optional relative-directory copy;
S27c2 0c657b31 adds move and explicit completion refresh; S27c3 af8bc2b1 adds
permanent deletion and unambiguous wide confirmation identities. This S27c4 unit
activates recycle modes and retains the stronger accepted S20c target policy:
FOF_ALLOWUNDO, FOFX_RECYCLEONDELETE, FOFX_ADDUNDORECORD and FOFX_EARLYFAILURE.
No application fallback from failed recycle to permanent deletion is introduced.

S27c4 checks owner/flags/abort HRESULTs, rejects null, invalid-index or stale Branch
sources, and performs the queue only after every exact source has been accepted.
Branch volume support is determined by Shell for each exact source rather than
turning a requested recycle into permanent deletion based on the synthetic root.
Ordinary disk volume policy stays unchanged. The temporary S27c3 recycle guard is
removed. Clipboard/drag/drop, archive/plugin destinations and later operations
remain outside these units; the process activation gate stays off by default.

Debug/Release x64 host/dependencies /warnaserror PASS. Actual getter/planning/queue
harness269/configuration PASS:248 prior checks plus21 actual queue-block checks with
explicit COM facades (mixed-volume path capture, duplicate/long identities, creation,
owner/flags/queue/parse/perform/abort failures, stale/null/index/normalization guards).
The queue facade does not prove actual Shell behavior. Separately, the actual
DoDeleteFile extraction now passes48/configuration with real filesystem deletion
and real force-recycle helper calls over five owned path variants; failed locked
recycle preserves the file, with no direct-delete fallback. UI/progress and selected
extension mask matching are facades. All31 prior permanent-worker checks remain.
The existing real operation_recycle_tests project built and ran D/R,15 checks each,
including real Shell recycling, long Unicode paths, ambiguous components and errors.
All28 registered source/menu groups PASS. The donor parser assertion was adapted
to the target's checked shellPath, adding an explicit OperationRecyclePath assertion.

D/R GUI Branch/ordinary panel smoke, Debug Files menu Delete availability and Release
Shift-F8 confirmation Cancel PASS. No GUI recycle/destructive confirmation was invoked;
actual destructive I/O is native-test-only. All12 GUI source hashes and sentinel remain
unchanged; hosts/reporters exited cleanly, private INIs restored. LongPathsEnabled=0
was read back; no system, security or global settings changed. Cross-volume/network
Shell behavior is not an actual runtime test on this single-volume fixture. The
legacy notification transport limit for very long panel roots remains S30 work:
manual Refresh may be needed there, while operation I/O retains exact wide paths.

Evidence ../evidence/S27c4 and earlier stage directories. No push/PR, remote CodeQL
or clean whole-solution matrix has run for these local commits. Accepted reference
remains S27b e63167f0. Publication/CI is the next review gate when authorized.


## S27c3 locally validated (2026-10-23)

Permanent deletion is enabled for Branch rows using their current exact parent/name.
The ordinary DOS-alias fallback is excluded for Branch sources. Donor extended
confirmation lists show relative paths; a local single-item adaptation uses the
owned wide relative identity instead of a shortened display mirror. Normal disk,
archive/plugin and later clipboard/drag policies remain unchanged. Recycle modes
are conservatively blocked only during this intermediate commit, never converted
to permanent deletion; S27c4 removes this transition guard after recycle validation.

Debug/Release x64 host/dependencies /warnaserror PASS. Actual getter/source/planning
harness248/configuration PASS. New registered native-generated test compiles the
actual DoDeleteFile and FileNameIsInvalid functions, includes the actual workerpath
helper and executes owned filesystem I/O:31 checks/configuration PASS. Cases include
ASCII, Unicode leaf/parents, UTF8-over-260 and Win32-long paths, duplicate basenames,
readonly removal, hidden Cancel/Skip, missing-source Cancel, invalid trailing-dot
alias and preservation of a sibling payload. UI/progress and recycle are explicit
facades; no GUI deletion is claimed. /W4 /WX exposed an existing potentially
uninitialized useRecycleBin in the worker. Safe initialization to TRUE preserves
all known modes and prevents unknown configuration from implying permanent deletion;
the new native test verifies that case preserves its source.

All28 source/menu groups PASS. Actual Debug GUI basic multi/single relative-identity
confirmation Cancel and Release extended relative list Cancel PASS. All12 GUI source
hashes and sentinel unchanged; own hosts/reporters cleanly exited, private INIs
restored. No actual destructive GUI action was performed; skill action-time approval
was therefore unnecessary. No registry/security changes. LongPathsEnabled remains0.
Evidence ../evidence/S27c3. No push/PR, CodeQL or clean whole-solution CI. Next S27c4.


## S27c2 locally validated (2026-10-23)

Branch F6 move now reuses the exact parent/name and optional relative-directory
planning. Donor per-source read/write storage access and cross-volume status flags
are restored. Current-row/cache gates remain; archive/plugin and Branch destinations,
clipboard, drag/drop and delete stay blocked. Explicit completion notifications now
use the scoped donor Branch AcceptChangeOnPathNotification block; this required
dependency is documented in PLAN and leaves S26 automatic-watcher policy unchanged.

Debug/Release x64 host/dependencies /warnaserror PASS; actual-block native harness
243 checks/configuration PASS (230 retained, six move and seven notification checks).
All28 source/menu groups PASS. Actual D/R F6 moved six files each with long Unicode
names, mixed parents and duplicate basenames into distinct relative destinations.
All12 destination hashes match; original source files are gone, sentinel unchanged;
Branch refreshed to zero rows without a manual key. Debug Cancel left all sources
and empty destination unchanged. Both own hosts/reporters exited cleanly and private
INIs were restored. GUI exercised same-volume moves; cross-volume status selection
is facade-tested, not a real second-volume transfer. Existing legacy MAX_PATH
notification transport remains a limitation for refreshing very long panel roots;
operation paths themselves are owned/wide. S30 notification work remains deferred.

Evidence: ../evidence/S27c2. No push/PR, remote CodeQL or clean whole-solution CI.
Next requested unit: S27c3 permanent deletion.


# S27c continuation (explicit user multi-stage instruction, no push)

Run S27c1b, S27c2, S27c3 and S27c4 sequentially as separate coherent local
commits; do not publish. This explicit multi-unit request supersedes the
CONTRACT default of one unit per user request, without skipping any unit gate.
Accepted reference remains S27b; S27c1a local parent is4fd4463e. Donor stays47661e92.

S27c1b (locally validated; see steps/S27c1b.md): donor keepBranchPaths builder signature, per-file relative target/create
operations and enclosing Skip labels; copy dialog option/member/transfer/toggle
and both dialog templates/control ID. Existing14510 strings/translations reused.
Preserve target S20b owned full basename and S27c1a current-row/destination guards.
Do not activate move/delete/recycle or unrelated criteria-dialog code.
Test actual extracted planning/Skip blocks, nested duplicate identities, invalid
relative components, allocation/scheduling failures and default-flat behavior;
D/R host/dependencies, existing native/source tests and real D/R nested F5 copies,
Cancel and ordinary dialog. Verify sources/output hashes and private configs.

S27c2: enable only move after own source/volume/storage flags and exact-parent
refresh validation, retain copy/default/relative semantics. S27c3: enable direct
delete with exact-parent build/confirmation and no stale/DOS identity fallback.
S27c4: preserve target force-recycle policy and queue-all-before-perform adaptation,
validate actual mixed-parent identities and no permanent fallback. Native fixtures
perform destructive tests only on owned roots. GUI Delete tests stop at Cancel
unless the skill's action-time confirmation is satisfied. Never change registry,
security policy, SDK layout or global configuration.

## S27c1a completed locally (2026-10-23)

The flat-copy-only unit below has passed Debug/Release build, native and actual
GUI validation. See [S27c1a](steps/S27c1a.md). The accepted local reference remains
at S27b e63167f0; the next separate unit is S27c1b optional relative-path copy.
Move/delete/recycle dispatch remains deferred. No publication in this step.

## S27c operation split and S27c1a plan (2026-10-23)

S27b accepted at e63167f01b4b7a84f9710275a86bcce0cf191e88 after all PR38
checks, actual CodeQL and clean supported Win32/x64/ARM64 Rebuild passed.
Accepted local ref advanced by compare-and-swap; validation draft remains open.

S27c1a: default flat F5 copy from current Branch rows into an ordinary disk panel.
Reuse S19/S20b worker and BuildName; import only donor BuildScriptMain per-item
source directory/name, ADS/network flags and storage scheduling hunks. Keep
ordinary full NameW adaptation. Reject stale metadata, non-copy source actions,
Branch targets, archive/plugin destination routes and copy-to-selected-folders.
No drag/drop or clipboard activation. Stop-refresh guards already protect modal
listing lifetime; revalidate current metadata at actual builder boundary.
No public SDK/interface or path-sized buffer changes. Windows policy unchanged.

Later units: S27c1b optional relative-directory copy (donor keepBranchPaths,
create/skip labels, dialog/resources/localization), S27c2 move (flat/relative,
source disappearance and refresh), S27c3 permanent delete, S27c4 recycle (exact
mixed-parent shell queue). Re-check dependencies and narrow boundaries before
each unit. In this run perform S27c1a only; no push/PR authorized.

Gate: compile extracted actual per-item builder preparation, preserve all200
Branch checks, verify distinct same-name parents, long and Unicode identities,
source network/ADS/storage flags and stale/missing metadata rejection. Existing
BuildName/ordinary/worker native suites + source/menu contracts; Debug/Release
x64 host/dependencies. Actual D/R GUI F5 Cancel temporary/explicit selection,
copy from different parents to ordinary destination, duplicate collision Cancel/
Skip, true long parent and 100-CJK filename; verify every source/output hash and
protected inputs; own private configs restored and own host cleanly closed.

## S27b local implementation complete (2026-10-23)

The planned Rename-only unit has passed its local Debug/Release build, native
and GUI gates. Details and publication limits: [S27b](steps/S27b.md). The accepted
S27a2 reference remains 1d29dcf9; S27b is a separate local candidate commit.
No S27c command is enabled. S27c must be split into operation-sized units,
starting with a separately planned copy unit if its dependencies are ready.

## S27a2 přijato; zahájení S27b (2026-10-23)

Přijata revize1d29dcf9: PR37 všechny kontroly SUCCESS, skutečné CodeQL SUCCESS,
oba původní nálezy fixed, čistý Rebuild podporované sestavy Win32/x64/ARM64 PASS.
Interní draft se nemerguje do zrcadlové base. Lokální přijatý ref je posunut
na tuto revizi. S27b aktivuje pouze Rename a bezpečný úklid dočasného výběru.

Přebírá se donorový BranchItemRenamed, deklarace a tři úspěšné větve přejmenování
se zastavením icon readeru před změnou jména. Před otevřením operace musí souhlasit
aktuální cache identita. Metadata lock se uvolní před callbacky/refresh. Již hotové
S20a široké I/O a dočasný výběr se znovu neimplementují. Brána: extrahované skutečné
metody + původní testy, Debug/Release host a skutečné GUI Rename/Cancel s duplikáty,
Unicode/dlouhými rodiči, explicitním výběrem a kontrolou nedotčených hashů.
Výchozí gate zůstává vypnutý, S27c–f odložené. Bez nového push/PR v této etapě.

## S27a2 dokončeno v dohodnutém rozsahu (2026-10-23)

Aktuální zadání výslovně přijímá dokončení s bezpečným odmítnutím
nepodporovaných případů. Tím nahrazuje starší podmínku, že dlouhé programové
cesty musejí být úspěšně spuštěny přes Shell. Historické záznamy PARTIAL níže
popisují tehdejší stav; nejsou aktuálním výsledkem etapy.

Oddělené větve nyní obsluhují ověřené PE programy .exe a dokumenty, skripty
či odkazy přes Shell. Program spouští Windows Attachment Services v existujícím
salopen pomocníkovi; neprovádíme raw CreateProcess cílového programu po pouhém
CheckPolicy. Windows zachovává závěrečné ověření a vlastní spuštění. Archivy
ponechávají ověřenou opravu širokého metadata I/O. Odmítnutí nemá alternativní
pokus, změnu bezpečnostního označení ani změnu registru.

Chyba dlouhé cesty obsahuje pouze čtený stav LongPathsEnabled a místní návod.
Výchozí tlačítko zavírá dialog; návod upozorňuje, že zapnutí není zárukou funkce
Shellu nebo aplikace a není řešením bezpečnostního odmítnutí. Skutečný systém
zůstal LongPathsEnabled=0. Podporované případy i omezení a přesné výsledky jsou
v [závěrečném protokolu S27a2](steps/S27a2-completion.md).

Lokální Debug/Release x64 build hostitele, salopen a jazykových resources prošel.
Programový broker: 93 kontrol v každé konfiguraci; Branch dispatch: 186/config;
běžné panely: 196/config a 5 SDK prefixů; 28 registrovaných zdrojových/menu skupin.
Skutečné GUI ověřilo krátký program se správným zdrojem/CWD a odmítnutí dlouhého
programu; tmavý i světlý dialog byly zkontrolovány. Debug shutdown opět ukázal
Detected memory leaks; příčina není určena a čisté ukončení Debug netvrdíme.

Bez publikace, čistého buildu celé sestavy a vzdáleného CodeQL v tomto kroku.
S27b a další etapy zůstávají odložené. Výchozí aktivace Branch View zůstává vypnutá.
Evidence: ../evidence/S27a2-final.

## S27a2 acceptance: default Windows configuration (2026-10-23)

User decision: long Shell launch must work with default Windows settings.
Do not require LongPathsEnabled=1 or any security/zone-policy changes for end
users. Modified settings may be used only as explicitly identified diagnostic
experiments; success there cannot satisfy the acceptance gate. The current
machine retains LongPathsEnabled=0; no registry change was made.

Follow-up normal-policy probes at candidate 9a870a02 failed with short aliases,
explicit Unicode/InvokeIDList flags, absent CWD, ACP and legacy path manifests.
Read-only zone diagnostics identified the original335-UTF16-character file and
its111-character DOS alias as the same volume/file ID. Both map to zone4
(Restricted), while the233-character ancestor and ordinary ASCII control map
to zone0 (Local Machine). Neither fixture executable has a Zone.Identifier
stream. This narrows the observed failure but does not prove all Windows
versions or Shell handlers behave identically. A minimal AttachmentExecute
CheckPolicy probe also rejects the working ASCII control, so that probe is not
independent proof that short Shell execution is denied.

Evidence ../evidence/S27a2-shell/SUMMARY.json and zone-matrix logs. No production
change, new GUI smoke, full build or remote CI in this diagnostic continuation.
S27a2 remains PARTIAL. Next gate: a demonstrated supported Shell dispatch under
default settings, preserving exact file identity, normal security checks and
physical working directory; then actual Debug/Release host verification.
An explicit long-path error is graceful failure, not completion of this gate.


## S27a2 - PARTIAL / archive correction validated locally (2026-10-23)

Parent 94c391a77282df03648aa47e20cb4410642764b0. Candidate enables gated
Enter using the exact current Branch item, owned wide source/directory and
UTF8 archive identity. Two archive metadata opens now use tracked CreateFileW
with the extended prefix instead of passing UTF8 to CreateFileA. This target
adaptation fixes the reproduced long Unicode ZIP failure, including refresh.
No archive engine, plugin SDK, workflow or default activation change.

Real Debug/Release host GUI passed long Unicode Branch ZIP Enter, listing,
refresh, F3 extraction/content and return to its exact parent. Debug additionally
passed a Unicode archive basename. Release ordinary ASCII duplicate ZIP showed
its distinct marker; a missing Branch archive preserved all eight rows and
opened correctly after the owned fixture was restored. Short ASCII Shell Enter
started the exact owned executable with its physical working directory in D/R.

Long executable Shell launch remains UNRESOLVED. Normal-policy plain/PIDL/alias
probes failed on this host; an ordinary ASCII control succeeded. Context-menu
probe timed out at 20s and its owned process tree was stopped. No security
settings/zone bypass/elevation or direct-process fallback. Candidate rejects
Shell source lengths >= MAX_PATH with the existing localized error, preserving
the Branch snapshot; this is a safe failure boundary, not long-launch support.
Accepted ref remains at S27a1; S27a2/S27a are not DONE and S27b onward is deferred.

Host x64 Debug/Release /warnaserror PASS (6.62/35.60s). Actual-method native
fixture 181 assertions/config PASS (original 99 retained; real archive metadata
I/O plus recorded dispatch/lifetime). Ordinary path/service 196/config and five
released SDK prefixes PASS; all 28 source/menu groups PASS. Existing unchanged
file-action D/R executables reran successfully (12 reported checks/config).
These are local incremental builds/tests, not full clean solution CI/CodeQL.

Debug shutdown displayed a heap-leak warning; acknowledged, then no owned host,
reporter or new report remained. Allocation profile was not captured, so prior
S22/S26b diagnostics do not prove the cause of this occurrence. Release exited
normally without a warning and left no host/reporter/new report. All 16 owned
input hashes verified unchanged; both private configstorage INIs restored byte
for byte. Installed user host and global settings untouched.

Evidence: ../evidence/S27a2; details [S27a2](steps/S27a2.md).
No publication or remote CI in this continuation.

## S27a1 — IMPLEMENTED / local validation complete (2026-10-23)

S21–S26 accepted at3dc52cb6 after current-head PR35 checks; acceptance commit
6f32c1d3. Completed the first S27 unit: core/configured external View/AltView,
Edit and exact Open Containing Folder. Process gate remains default off;
Enter, plugin viewers, View/Edit With and all remaining operations stay blocked.
Missing parent preflight preserves the Branch snapshot. Successful navigation
uses an owned wide basename to focus the real ordinary row, including a name
of100 CJK characters/304 UTF8 bytes. No generic navigator/SDK changes.

Final host x64 Debug/Release /warnaserror PASS12.58/76.84s. Actual-method native
fixture99 checks/config PASS, original71 retained; ordinary panel/service196
checks/config and5 released SDK class prefixes PASS; all28 source/menu groups
PASS. Existing file-action, viewer enumeration and Unicode native suites PASS.
Actual D/R GUI confirmed duplicate identities, viewer previous/next/refresh,
external argv+physical CWD, exact parent focus, missing-parent preservation,
blocked Enter/copy, normal-panel viewer and shutdown/no new reports. All16 owned
input hashes unchanged and both private configstorage INIs restored.

S27a is PARTIAL: an initial Enter trial failed for a long executable (Windows
Security) and a long ZIP (error3). No security barrier bypass/settings change.
That import was removed; fileswn2.cpp matches the accepted source. S27a2 must
resolve and validate Shell/archive boundaries before enabling Enter. Later
S27b–f/S28–S32 remain deferred. No new push, PR, CI or multi-phase goal.
Details [S27a1](steps/S27a1.md); evidence ../evidence/S27a. Existing S26b Debug
menu-heap limitation remains documented; this step does not claim to fix it.

## S27a1 focus-name prerequisite (2026-10-23)

Final caller audit found ChangePathToDisk copies UTF8 suggestedFocusName through
backup2[MAX_PATH], and CommonRefresh compares only legacy ACP Name, missing
NameW even with a complete UTF8 input. A real 100-CJK basename confirmed lost
focus. Keep generic navigation unchanged: S27a1 navigates without narrow focus,
then selects exact NameW in the resulting ordinary file list. Own the wide
basename before navigation destroys old rows. Extract the actual method and
test callback/owned identity; verify a real >260-byte basename in D/R GUI.
Enter/archive launch remains a2 DEFERRED; full S27a is not DONE.

## Initial S27a pre-edit boundary (superseded by S27a1/a2 split)

Accepted S21–S26 at3dc52cb6; acceptance commit6f32c1d3. Import only pinned donor
OpenBranchItemDirectory, Execute Branch full-path launch/archive branch and its
menu/declaration/dispatch. Reuse existing S18 ViewFile/EditFile exact-path callers.
Process opt-in remains default off. Enable only Open/standard+alternate View/Edit
and Open Parent in the command gate. Keep all copy/move/delete/rename, file Shell/
clipboard, drop, plugin command ranges and View/Edit With dialogs blocked.

Viewer activation requires a small temporary first-matching-mask guard in the
panel caller: permit core internal/configured external viewers only, before any
plugin CanViewFile/ViewFile callback. Explicit handler-selection dialogs remain
blocked; no version-based plugin capability inference. Donor launch/archive logic
keeps exact full path and item directory; reject empty metadata before dispatch.
Existing non-Branch behavior and S18 UTF8-first path/CWD launch helpers unchanged.
Open Parent failure must retain Branch snapshot; success must focus exact wide
basename in its real parent, including duplicate/long/Unicode paths.

Required gates before accepting S27a: native original file-action/getter/viewer
identity tests plus native actual parent/launch dispatch extraction with failure
and owned argument lifetime; all current source/menu contracts; host D/R x64
/warnaserror; actual GUI D/R internal viewer next/previous/refresh, external
viewer/editor argv+cwd recording, Enter child, parent focus and missing parent,
normal-panel regression, blocked unverified commands and default-off gate. Owned
fixture hashes and private config restoration; document actual tests/limits.
No history/tab integration, no new SDK/layout, no additional phase or push/PR.


## S21–S26 — ACCEPTED after PR35 CI (2026-10-23)

User accepted head `3dc52cb6986f0acf0aa3de906243684e4d1b5d22` of internal draft
[PR35](https://github.com/majklvi/salamander/pull/35). All13 current-head checks
completed SUCCESS, including actual CodeQL initialize/Release-x64-build/analysis/
SARIF and the corrected icon-reader regression. Separate clean full supported-
solution Rebuild39949246785 passed Debug Win32/x64/ARM64 with dependencies; SFTP
remains excluded by existing policy. PR35 stays open; mirror base is not merged.
Evidence ../evidence/S27a/accepted-s21-s26-ci.json. Known Debug menu-heap diagnostic
limitation documented in S26b remains; CI success is not an allocator-origin fix.

Next originally selected unit: S27a; now split into completed a1 and deferred a2.
S27b–f, S28–S32 remain deferred. Experimental activation stays process-gated/default
off. No new push/PR is authorized by this continuation.


## S26 — IMPLEMENTED / gated read-only panel, local acceptance complete (2026-10-23)

S21–S26 are complete locally. S26a shared preparation prerequisite and S26b exact
metadata/publication/icon/property lifetime are integrated behind an exact process
opt-in, default off in Debug/Release. Read-only command and direct-operation guards,
Detailed layout, Path sorting/focus, manual refresh/Stop/Escape and close are verified.
Full provenance, actual test counts and limitations: [S26b](steps/S26b.md).

Host D/R /warnaserror, native scanner/getters/property/resource checks and all28
source/menu groups PASS; actual dark Debug/light Release GUI and8,209 fixture hashes
verified. Debug can still show a menu-only heap warning: paired gate-disabled and
Branch runs each448bytes/14 MENU blocks; no suppression change. Release close clean.
S27 operations, S28 layouts, S29 persistence and S30 watchers remain deferred.
No push/PR/new CI in this goal; publication still needs the supported clean matrix.


## S26b concrete integration boundary (2026-10-23)

Pre-edit plan (now implemented; see steps/S26b.md). S26a accepted8e3d0c92. Narrow donor diffs saved in
../evidence/S26b/*.diff for review. Required closure:
- Import donor panel state/publication metadata and path/cache helpers, using
  existing S04 UTF8-first/ACP ordinary fallbacks (do not revert them). Preserve
  S20 owned temporary selection capture on ALL disk panels, S20c wide recycle
  drive gate, S19 NOHANDLES target copy tracking and S21b ordinary Shell/text routes.
- Import mandatory icon-reader full identity snapshots, cache-key lookup,
  row reindex after sort and metadata teardown AFTER reader shutdown; wide overlay
  helper and explorerpropertywork.h ownership/target-disconnect/generation guards.
  These are required before publishing any experimental row, not deferred S28.
- Keep S27 launch/rename/copy/move/delete/Shell/archive/plugin integrations blocked.
  Use a small WM_COMMAND read-only whitelist AND guards at direct panel execution,
  quick rename, operation/Shell/clipboard/drop entry points. Block operations with
  a Branch source OR destination. Selection, scrolling, focus, built-in sort,
  Path/name as text, manual refresh/cancel, mode exit and closing may be verified.
  Guard mouse/double-click/drag/drop/direct paths, not only grayed menus.
- Process-only SAMANDARIN_EXPERIMENTAL_BRANCH_VIEW=1 is the proposed explicit
  local-test gate; absent/other values are false. Toggle/restore and all actual
  activation entry points must check it; normal Debug/Release compile all code.
- Initially force core Detailed layout during experiment, restoring the original
  layout on exit, to avoid exposing unverified arbitrary thumbnail/plugin layouts.
  Document this S26 adaptation; optional verified thumbnail layouts can be S28.
- No history/tab persistence integration yet (S29). Donor restore metadata may be
  retained for publication selection/focus lifetime, but activation must be gated.
- Port donor Path column/rendering, basic stable ordering, manual refresh/cancel,
  timer/status; no recursive watcher needed until S30. Keep asynchronous provider
  lifetime guards even with automatic refresh deferred.
- Port paired resources/all16 donor Branch strings and all relevant translations
  without retranslating; future option/open-parent texts may be retained unused.
  Enable all96 formatter resource patterns once present; record actual execution.
- Native scanner/preparation/property lifetime and real panel-getter tests must
  exercise target code. Retain earlier S18b test-only ViewerEnumeration constants
  when adapting generated fixtures. Existing donor contract assertions for future
  S27/S29/S30 behavior remain explicit deferred, never mislabeled PASS.
- Actual GUI D/R: absent gate cannot activate, explicit gate duplicate basenames
  distinguish parents/full paths, Path sort keeps focus, refresh/cancel/Escape,
  close while pending, normal panel regression, blocked unverified commands,
  dark/light. All fixture content hashes/other inputs intact; private config only.
- Host D/R /warnaserror and relevant native/source gates before commit/ref advance.
  No push/PR. Do not mark the S21-S26 goal complete until this closure is real.

## S26a — IMPLEMENTED / association preparation lifetime prerequisite PASS (2026-10-23)

Scoped donor AssociationPreparation/shared PreparationLifetime/PreparationSession
and Shell association epoch/cache/reentrancy guards imported ahead of first
panel publication. Preparation caches answers without mutating live icon records;
association release invalidates epoch and cached answers; reentrant insertion
rechecks the index before inserting. Header/helper/native fixture content exact
donor. Target GetFileIconForPanel HANDLES_ADD accounting kept; unrelated donor
removal excluded pending independent accounting audit. No Branch panel metadata,
activation, polling or user command added yet; S26 overall remains IN PROGRESS.

Native branch_preparation_tests/project EXACT:17 checks/config Debug/Release PASS
including deduplication, time/query limits, cancellation, argument ownership,
clear/replace/restart during callback and stale revision. These use deterministic
clock/resolver, not real third-party Shell reentrant callbacks. Actual Shell cache
integration is compiled and audited; no simulated callback claimed GUI proof.
Host x64 Debug/Release /warnaserror PASS46.92/81.10s final; initial Release95s link
timeout retained. All24 source/menu groups PASS. Initial panel-rendering contract
expected direct QueryShellAssociation call; imported exact donor update retaining
original query/normal cache assertions and additionally tracing prepared lookup.
Normal panel GUI D/R shows txt and unknown s26icons extension in long Unicode
parent, correct known/fallback icons; normal exit/no new reports. All4 own hashes
unchanged, private configstorage.ini restored; installed host untouched.

Evidence ../evidence/S26a manifests/bounded logs. No SDK interface/layout, public
gate or user preferences changed. No push/PR/CI. Next S26b must integrate full row
identity + icon/property lifetime before any experiment activation, default-off
runtime gate and read-only dispatch, paired resources and actual GUI acceptance.


## S26 safety-first integration substages (2026-10-23)

S26a imports donor AssociationPreparation + shared preparation lifetime/session
and actual Shell association epoch/cache/reentrancy guards, with exact native
preparation fixtures and ordinary icon smoke. Keep target icon handle tracking
unless independently proven wrong; donor's unrelated accounting removal is not
part of this prerequisite. No Branch activation/state rows in S26a.
S26b connects scanner to read-only gated panel with full identity and mandatory
icon/property-reader lifetime/generation protections, Path/sort/status/resources,
manual refresh/cancel and all dispatch/activation gates. Import prerequisite
helpers that are required by donor publication lifetime, retaining attribution.
All relevant language strings and actual GUI acceptance required before S26 done.
Do not enable unverified S27 operations or arbitrary plug-ins. No public release.

## S25 — IMPLEMENTED / isolated cancellable scanner PASS (2026-10-23)

Pinned donor branch_view.h/.cpp scanner subset imported: Entry/Progress, owned
exact paths/UTF8 cache identity, explicit directory stack, bounded batches,
shared State independent of panel/window, Start/Cancel/Drain and snapshot
comparison. Actual State/Run/Start/Cancel/Drain block verified exact donor.
Directory name-surrogate junctions/symlinks are skipped, ordinary reparse/cloud
directories retained per donor policy; hidden/system directory filtering retained.
Cancellation does not join pending SMB I/O; worker's shared state survives owner
close and replacement gets independent storage. No panel/UI APIs or activation.

Native project EXACT; donor fixture ADAPTED with test-only retained future
scheduler/publication-sort sections explicitly uncompiled until dependencies in
S26/S30; no future assertions counted passed. Scanner/concurrent-write/snapshot/
replacement/cancel/owner-destruction assertions retained. Added missing-root
error assertion, actual check count and bounded temporary-root cleanup guards.
Host project includes donor NotUsing PCH setting (initial omission failed C1020,
then corrected); IDE filters registration scoped only. Existing runner discovers
native project automatically, no registration framework introduced.

Native Debug/Release2476 checks each PASS0.92/1.60s;605 files, duplicate parents,
Unicode long filename/parent paths, exact UTF8 identity per row, multiple batches,
missing-root error, changing tree, hidden filter, independent replacement,
cancellation and16 immediately closing owners. Actual directory symlink cycle
fixture RAN in both configurations, no policy-only substitute. Not a network
outage GUI test; pending-I/O independence verified by production ownership audit.
Host Debug/Release x64 /warnaserror PASS11.86/84.38s. All24 source/menu groups
PASS. Actual ordinary panel smoke D/R: long Unicode panel and2 files visible,
normal exit/no new reports. All4 source hashes unchanged; private configs restored.
Evidence ../evidence/S25 source hashes and bounded logs. Public feature still
unavailable. No SDK/layout changes, push/PR/CI. Next S26 gated panel integration.


## S25 scanner extraction (2026-10-23)

Import donor Entry/Progress/Scan/path helpers and SameSnapshot comparison only,
without panel state or UI. Exact scanner shared-state implementation is retained.
Header/project registration is scoped; native donor fixture keeps all scanner,
concurrent-write, snapshot, replacement/cancel and owner-destruction assertions.
Retain future scheduler/publication-sort fixture sections behind a test-only
BRANCH_VIEW_EXTENDED_TESTS guard until S26/S30 dependencies exist; never report
those sections as executed. Add missing-root error regression and fixture root
cleanup guards. Association preparation and panel metadata are not scanner deps.

## S24 — ALREADY COVERED / formatter audit PASS (2026-10-23)

No duplicate implementation: S20c already imported exact pinned donor typed
branch_view_text.h and native project, actual compressed-size caller in fileswn6.
Verified helper content parity ignoring checkout EOL only. Native test source
diff contains only retained future Branch-resource filter and truthful summary;
all status/general/compression formatter assertions match donor. Existing source
comment names S27c for deferred resources; current plan schedules resource IDs
with S26 and operation-specific callers with S27c. No absent fixture called PASS.

Rebuilt native targets Debug/Release3.25/4.10s; actual formatter61 checks/config
PASS, including12 existing localized compression patterns, nulls, uint64, unknown
specifiers literal, percent payloads and long Unicode text without truncation.
Remaining84 Branch resource patterns remain explicit S26 acceptance work. No
runtime source changed, so no redundant host rebuild: preceding S23 Debug/Release
and normal panel smoke bind this exact production source revision a858096b.
Evidence ../evidence/S24/audit.json and bounded test logs. Documentation-only
closure; no new SDK, localization framework, public activation, push/PR/CI.


## S23 — IMPLEMENTED / exact array reservation PASS (2026-10-23)

src/common/array.h and array_reserve_tests.cpp/vcxproj copied EXACT from pinned
donor47661e92. Optional Reserve retains Count/items/state/Base/Delta on failure,
rounds to the existing growth lattice, checks overflow; empty reserved storage
is released by DestroyMembers/DetachMembers without reviving failed storage.
No new container, scanner, ordering or panel publication changes. Existing test
runner discovers the new project automatically; no runner edits required.

Actual shipped template native20 checks/config Debug/Release PASS, with only
realloc failure injection: failure does not poison later insertion, rounding,
negative/overflow inputs, insert/delete, empty cleanup, object lifetime counts.
Common-header consumer audit: host+Salmon x64 and Translator+Trace Server Win32
(their supported platform), all8 Debug/Release /warnaserror builds PASS. Host
52.09/82.82s final; initial Release95s linker timeout retained separately. Native
builds2.69/2.99s. The plugin SDK does not include this common header; no unrelated
30-plugin SDK rebuild claimed. reglib source consumer is compiled in host;
historical standalone REGLIBA is outside current supported main solution.

All24 source/menu groups PASS. Actual normal panel smoke D/R: two files in >260
UTF16 Unicode directory, explicit selection50/54 bytes; normal exit/no reports.
All4 fixture hashes unchanged; private configs restored. Hashes/evidence under
../evidence/S23. No public gate, SDK layout, user configuration, push/PR/CI change.


## S22 — IMPLEMENTED / archive callback and ZIP smoke PASS (2026-10-23)

Pinned donor CPanelTmpEnumData owned relative UTF8 name/link-size cache and actual
PanelEnumDiskSelection file callback ported; existing root+relative SDK ABI and
ZIP/7-Zip engines unchanged. DiskDirBuildEnumeration uses full item identity.
ADAPTED donor IsBranchView guard to explicit ExactFileSelection state, false by
default and enabled by Pack only for file-only disk selections. Directories/mixed
selections retain original recursive tree. SourcePanel is initialized; no fake
Branch methods. S27e will connect real Branch selection. No Branch activation.
Fixture driver EXACT; cpp.in ADAPTED solely for route state binding instead of
future Branch method; all assertions retained. Registered as native-generated.

Actual production callback/state/converter fixtures362/362 per Debug/Release
PASS: duplicate basenames in separate parents, all enum modes, ordered reset,
64-bit metadata, pointer lifetime, invalid index/identity/conversion/allocation
failure, cached link sizes/ignored errors across move-packer source removal.
Dependencies are deterministic fixture panel/link replies, not actual packing.
Host x64 Debug/Release /warnaserror PASS36.42/93.93s; final restored Debug8.65s.
All24 source/menu groups PASS. GUI actual ZIP plugin Debug/Release: ordinary.txt
and Czech/CJK/emoji name inside Unicode parent, correct exact archive member
names and decompressed SHA256. All4 input hashes unchanged. Release normal exit.

Debug ZIP creation uncovered shutdown heap warning, not hidden or called clean:
641 bytes/21 blocks, ten system MENU48+16 pairs and one1-byte01 block. Temporary
heap report logging and explicitly disabled new callback route reproduced the
same exact allocation profile using legacy callback/ASCII inputs. This isolates
the warning from the S22 callback; underlying allocator origin remains unresolved.
Diagnostic patches/logs retained outside Git; heap.cpp byte restored, fileswn7
production manifest rechecked, final production Debug rebuilt. Warning closed
normally; no process/report remains. No suppression or unrelated engine fix.

Private configstorage.ini restored; installed user host untouched. Evidence in
../evidence/S22 includes hashes, legacy comparison and actual ZIP validations.
No real Branch packing/move-packer GUI claim (S27e). No push/PR/CI.


## S22 integration prerequisite (2026-10-23)

The donor archive callback uses SourcePanel->IsBranchView(), which does not exist
before S26. Port its exact owned-relative/cached-link algorithm with the smallest
explicit adapter: CPanelTmpEnumData::ExactFileSelection, false by default and
set only for an ordinary disk selection containing files only. Directory/mixed
selections keep existing recursive enumeration. This exercises the new callback
now without a fake Branch method. S27e will select this route for Branch rows.
Retain all donor fixture assertions; adapt only fixture route selection. Test
actual callback D/R plus host D/R and actual normal-panel ZIP creation/hashes.

## S21b / S21 — IMPLEMENTED / ordinary Shell integration PASS (2026-10-23)

Pinned donor Shell adapters/call sites ported into ordinary disk panels. Exact
CreateIDataObjectForPaths/CreateIContextMenu2ForPaths wrappers and declarations;
scoped Properties/attribute wide identity hunks; disk name/full/UNC clipboard
route. ADAPTED donor Branch-only guard to Is(ptDisk), as with S20 ordinary
operations: disk identities are already available; no Branch placeholder API.
Archive/plugin virtual paths keep their original route. Selection menus bind the
real parent; Unicode verb directory supplied. Disk copy/cut publishes the full
exact data object and drop effect atomically. Cut matching uses full identity.
No Branch dispatch/public activation; mixed-parent host fallback stays S27d.
S20c exact-wide recycle-volume eligibility repair preserved.

Host x64 Debug/Release /warnaserror PASS11.79/44.08s. Existing S21a native helpers
unchanged (28+33 checks/config); not claimed rerun here. All24 registered source/
menu groups PASS; operation scope extended8 to10 with exact disk Shell/text
boundary regressions. Initial runner invocation lacked MSBuild PATH and failed;
final bounded invocation supplies baseline environment and passes.

Actual GUI Debug: Unicode file in >260 UTF16 path, correct real-parent context
menu; single Shell Properties29 bytes, exact full Unicode text; Copy/Cut exact
CF_HDROP and preferred effects5/2. Release: explicit2 files, Copy/Cut exact whole
selection/effects, multi Properties2 files56 bytes. Initial separate clipboard
reads observed Chromium text or a transient lock; final bounded reader runs
before UI action to observe publication without inter-tool overwrite. No claims
for failed reads or dedicated UNC/network GUI smoke. Native UNC cases in S21a.
All4 own file hashes intact. Both hosts normal exit, no new reports; private
configstorage.ini restored. User installed host untouched. Evidence ../evidence/
S21b ties runtime patch and final source/test hashes. No push/PR/CI.


## S21a — IMPLEMENTED / exact Shell helpers PASS (2026-10-23)

EXACT pinned-donor branchshell.h/branchclipboard.h and full native
branch_context_menu_tests + branch_operations_tests/projects. Host IDE header
registration scoped only; existing native runner discovers both targets without
new runner code. S20c operationrecycle.h wide-volume correction preserved exactly.
No host dispatch/call site changed; Branch View remains unavailable.

Host x64 Debug/Release /warnaserror PASS5.04/5.79s. Native context menus28 checks
per configuration PASS using actual Windows Shell canonical verbs versus independently
bound real-parent reference, UTF8-byte-long and Win32-long identities; mixed-parent
menu returns explicit unsupported rather than Desktop/first-parent menu.
Native operations33 checks/config D/R PASS: whole-selection CF_HDROP/CIDA, copy/move
drop effect, mixed-parent Properties data, Unicode/long paths, missing/unsafe
identity failure and actual recycle including locked-item failure. Fixtures own
unique temporary children and guarded cleanup. No real clipboard publication or
Properties dialog claimed: native IDataObject is the object under test.
All24 source/menu contract groups PASS. Source/patch hashes and bounded logs in
../evidence/S21a. GUI host runtime is unchanged by header-only registration;
actual ordinary-panel host integration smoke belongs to immediately following
S21b. No SDK/layout, user config, public gate, new push/PR or CI changes.


## Active goal S21–S26 (authorized 2026-10-23)

Complete the agreed multi-stage goal through the default-off experimental
read-only Branch panel. The user's explicit goal authorizes sequential stages
and substages in one sustained run, each with its own coherent validated commit;
the earlier one-stage-per-run prompts do not limit this authorized scope.
No additional push/PR is authorized for this goal.

1. S21a: exact donor Shell/clipboard helpers and native fixtures, preserving
   S20c recycle helper adaptation. S21b: scoped ordinary-panel call sites and
   real context-menu/clipboard/Properties smoke; Branch dispatch remains S27d.
2. S22: host archive selection boundary and actual normal-panel ZIP smoke;
   no ZIP/7-Zip engine rewrite or premature Branch activation.
3. S23: array reserve implementation/test parity and all supported consumers.
4. S24: audit already imported typed formatter/general tests; close only real
   gaps, preserve deferred Branch resources for S26, no empty implementation commit.
5. S25: isolated cancellable scanner, full ownership/reparse/Unicode native tests.
6. S26: minimal gated panel integration with full identity/lifetime/cache guards
   from first published row, read-only dispatch, paired resources/translations,
   dark/light UI and actual default-off/explicit-activation/Stop/close tests.

If dependencies make a stage too large, document the smallest necessary substage
before editing. Reuse pinned donor47661e92 and retain target fixes; preserve
SOURCE/CONTRACT pins. Each code checkpoint builds Debug/Release x64 plus affected
consumers and meaningful native/GUI regressions before acceptance.


## S20c / S20 — IMPLEMENTED / local operation gates PASS (2026-10-23)

S20a rename/owned selection and S20b copy/move accepted separately; S20c completes
ordinary delete/recycle, attribute and compressed-size owned-wide path hunks.
Worker DoDeleteFile receives SourceNameW and uses wide delete/readonly/attribute
APIs; target NOHANDLES copy accounting repair is preserved. operationrecycle.h
ADAPTED from donor; branch_view_text.h EXACT generic typed formatter, no Branch
state/UI. Host ordinary-disk recycle queues the whole exact wide selection before
performing it, rejects ambiguous shell components and forces recycle semantics.

Real GUI exposed a pre-existing additional boundary defect: the configured all-
files recycle policy was bypassed by ANSI MyGetDriveType on long Unicode parent.
Initial Debug own3 files were deliberately deleted; no own matching recycle
records. Fresh Release confirmed policy1 in UI but normal Delete still requested
direct deletion; canceled. Local OperationRecycleDriveTypeW uses wide volume-root
resolution, including mounted volumes, before GetDriveTypeW. This adaptation is
required so an already-wide shell operation is reachable. Initial evidence kept;
new unique fixture used for final GUI. No unrelated MyGetDriveType callers changed.

Final host Debug/Release x64 /warnaserror PASS9.86/87.93s. Earlier Release95s
post-link timeout and initial builds retained, not counted as final success.
Native recycle-only adaptation of donor branch_operations_tests:15 checks/config
D/R PASS; five actual per-item recycle paths include duplicate basenames in
different parents, long Unicode, UTF8>260/UTF16<260, missing and locked failure;
new exact-wide volume gate check. These exercise production recycle helper,
not the full panel queue. Full shell/clipboard native fixture remains S21.
Native typed formatter61 checks/config D/R PASS, including all12 real compressed-
size localized patterns; future Branch resource fixtures retained but skipped.
Project EXACT; source ADAPTED only to defer unconsumed Branch resource patterns.
All24 registered source/menu groups PASS, scoped ordinary operation contracts8;
future Branch assertions retained in All and not run/reported PASS.

Final GUI: Shift+Delete Cancel removes temporary selection and preserves explicit
selection (Debug2 files/57 bytes; Release3/126). Initial Debug3/114 also passed.
Debug normal Delete recycles100-CJK file from long parent; Shift+Delete deletes
ASCII+Czech/CJK/emoji remaining2. Release Shift+Delete deletes100-CJK file; normal
Delete recycles remaining2 as one queue. All3 recycle metadata paths and content
hashes match exact source identities (transport prefix accounted for). All6 own
sources absent as intended. 182 protected records intact, including177 previous
paths/streams, both sentinels and initial Release3 files. Final D/R normal exit
PASS; private runtime configs restored, installed user host untouched.

S20 ordinary operation scope complete. Branch-only options/resources/translations,
recursive mixed-parent planning and metadata remap remain S26/S27c, no public
Branch activation or dependency stubs. Attributes/count-size changes compiled;
no claim of dedicated attribute/compressed-size-error GUI test. Legacy dialog
subject shortening remains; I/O identity is full. No new SDK virtual changes.

The user-authorized goal through S20 is implemented in coherent local commits;
S11-S20 publication/full clean CI/CodeQL are still pending a later authorized
push. S10 was accepted into local incremental integration after its successful
current-head fork checks; validation draft PR33 is not merged into reference base.
SOURCE/CONTRACT and release-base pins unchanged. Evidence ../evidence/S20c/
source-manifest.json, final-validation.json, recycle-evidence.json and bounded logs.


## S20b — IMPLEMENTED / ordinary copy and move PASS (2026-10-23)

Scoped donor fileswn6 BuildScriptMain/Main2/File heap buffers and Unicode Copy-of
name generation; fileswn8 split-path dialog boundary uses exact item parent/base.
ADAPTED ordinary copy/move captures complete NameW as owned UTF8 before BuildName;
legacy display mirror may be truncated and must not identify the operation.
No Branch relative-path option/scheduling, archive enumerator or new resources.
These remain S26/S27c. Delete/attributes/count-size wide hunks remain S20c.

Host x64 Debug/Release /warnaserror PASS10.10/9.23s final. Initial missing include
compile failure fixed with donor widepath include; Release95s post-link timeout
preserved, subsequent complete build PASS. Production patch/source hashes saved.
Real F5/F6 Cancel D/R: temporary long-CJK selection removed; explicit3-file
selection126/138 bytes preserved. Actual copy then move of ASCII, Czech+CJK+emoji
and100-CJK names from long Unicode parent succeeds D/R. All12 final copy/move
outputs hash-identical; original own sources intentionally moved and absent.
Retested original S17 failing long-source Unicode symlink in BOTH configurations:
new outputs regular2107-byte files with exact payload hash; old links unchanged.
162 prior path/stream records and sentinel intact. Four normal shutdowns PASS,
private runtime configs restored, installed user host untouched.

All24 source/menu groups PASS, including4 scoped operation assertions. Actual
BuildName18 checks/config D/R PASS. Initial unsupported Both CLI invocation saved;
separate supported config runs pass. No full CI/CodeQL or push in this local step.
Known legacy dialog subject truncates100-CJK display label; operation paths and
output names are complete and verified. Future Branch assertions retained in All,
not reported PASS. Evidence ../evidence/S20b/{source-manifest,final-validation}.json.


## S20a — IMPLEMENTED / rename + temporary selection PASS (2026-10-23)

EXACT donor panel_temporary_selection.h and native fixture/project; scoped
rename/caller/project registration hunks ADAPTED from donor47661e92.
Disk snapshots use already-available GetItemIdentityW even before Branch View;
virtual panels retain Name fallback. Explicit user selection creates no temporary
snapshot. Wide rename takes actual item parent/path and invalidates all three
visible arrays. Branch refresh/remap hooks remain S26, no dormant Branch state.

Host Debug/Release x64 /warnaserror PASS (9.88/67.19s final builds). First Release
95s LTCG timeout preserved. Native21 checks/config D/R PASS, simulated rows
exercise production header identity matcher; not a whole-host cleanup harness.
Real F2 Cancel D/R: no explicit selection leaves zero selected; three explicitly
selected files remain selected (174/186 bytes). Actual rename of 100-CJK basename
at UTF16 path488/490 to Czech+CJK+emoji succeeds with identical content hashes.
Renamed focused row is unselected after success, the other two stay selected;
selection preservation claim applies to Cancel. Normal shutdown D/R PASS.
155 previous path/stream records and own six files/sentinel intact, two deliberate
renames accounted for; private configs restored, installed user host untouched.

Scoped operations source contract2 PASS; all24 registered source/menu groups
and49 pytest PASS. Ordinary path/service196 and getters12 checks/config D/R PASS.
Regression harness now declares ViewerEnumeration constants/discovery facade
introduced S18b; initial compile FAIL and concurrent compiler-env timeout saved.
Actual ViewerEnumeration implementation is covered by S18b, this is discovery
facade only. Future Branch operation assertions retained in All scope, not run
or reported PASS here. Copy/move and delete remain S20b/S20c; no new UI resources,
SDK vtable changes, push, full-solution CI or CodeQL in this local substage.
Evidence ../evidence/S20a/source-manifest.json, final-validation.json and logs.


## S20 — plán tří operation podetap (2026-10-22)

S20a: owned temporary selection header + ordinary capture/cleanup, všechny
nezbytné callers a hlavičky v jednom buildable kroku; rename přes existující
GetItemFullPathW/GetItemDirectoryW a notification skutečnému parentu. Donor
IsBranchView capture adaptovat na Is(ptDisk): ordinary disk využije již hotovou
S04 plnou identitu, virtuální panely uchovají legacy basename fallback.
Neimportovat Branch state/cache lifecycle; metadata rename remap bude S26.
Gate: native temporary-selection identity cases (simulované rows), host D/R,
ordinary getter/membership regrese, skutečné F2 Cancel bez/with explicit selection
a rename Unicode/long filename na vlastních fixtures, source contracts/pytest.

S20b: zbývající ordinary copy/move planning buffers, wide names a source/target
tok do hotových S19 workerů. Branch-only relative-path option/resources a
recursive source scheduling počkají na aktivní S26/S27c panel, bez stubů.
Gate F5/F6 Cancel zachová explicitní selection; skutečný copy/move Unicode/long,
hashes a untouched sentinel, plus konkrétní S17 long-source symlink copy retest.

S20c: ordinary delete wide path capture, confirmation a worker Delete/Recycling
hranice, scoped operation contracts/native helpers. Gate ShiftDelete Cancel
bez/with explicit selection a skutečné delete pouze vlastních jedinečných inputs,
včetně Unicode/long, nezměněných siblings/sentinelů. Každá podetapa má vlastní
commit, D/R build i konkrétní evidence; uživatelský goal dovoluje postupně všechny.


## S19 — IMPLEMENTED / lokální worker brána PASS (2026-10-22)

Převzat izolovaný EXACT workerpath.h a copy/retry/ADS/directory-create/attribute
I/O hunks worker.cpp, se souvisejícími call sites. Access/sharing/create flags,
identity checks, sync/async scheduling beze změny. Fileswn6 jen strict UTF8/ACP
dekódování a owned-wide copy planning error probe; capacity/Branch planning/UI
zatím S20. BuildName počítá UTF16 délku místo UTF8 bajtů, chrání empty parent;
jediná lokální úprava donorového body je explicitní l2=0 pro /W4 definite
assignment warning bez oslabení /WX. Nesouvisející icon tracking hunk vynechán.
DoDeleteFile/Recycling a jeho ThreadWorkerBody call jsou DEFERRED S20;
operationrecycle.h zde není importováno. Branch View stále vypnuté.

Kritická zachovaná target odchylka: první donor import přepsal NOHANDLES wide
target opens na HANDLES_Q před společným HANDLES_ADD_EX. Reálný Debug copy
proto při exit hlásil jeden monitored handle. Read-only SymFromName/PDB a
ReadProcessMemory pouze vlastního PID10576 identifikovaly přesný worker.cpp
target open; byla obnovena již existující target oprava, nikoli vypnuto měření.
Nový source contract hlídá dvojici NOHANDLES opens. Původní FAIL, source/binary
hashes, výstup i diagnostika zachovány; nový Debug skutečný retry/copy/ADS a
normal shutdown PASS. Také nový Release PASS. Původní Debug výstup zachován
nezměněný, finální testy zapisovaly do jiného vlastního cíle.

Host D/R x64 /warnaserror PASS (finální Debug9.02s, Release79.27s). Předchozí
Release95s LTCG/post-link timeouty zachovány, neoznačeny PASS. Native EXACT
worker_path_tests9/config a worker_copy_path_tests11/config D/R PASS, včetně
skutečného Unicode NTFS streamu a readonly helper recovery. CopyReopen je malá
reimplementace copy loop nad skutečnými production helpers, není celý worker.
Nový acceptance harness kompiluje skutečný BuildName + konverze:18 checks/config
D/R PASS (UTF8>32767 bytes při kratším UTF16, hranice32766/32767, DOS fallback,
empty parent, SkipAll/Focus). UI callbacks stub, nikoli skutečné dialogy.
Driver opravuje extrakci komentáře Translator a používá host CRT compatibility
define + C++17, /W4 /WX ponechány. Počáteční harness selhání zachována v evidence.
Šest worker source tests + celý23 source/PowerShell groups PASS, pytest49 PASS.

Skutečný GUI D/R long Unicode source copy: externí read-only shareMode0 handle
vlastního souboru vynutil error32. Po explicitním release a kliknutí Retry
worker dokončil kopii; hlavní data i pojmenovaný Unicode ADS byte/hash exact.
Zámky měly85s deadline, oba uvolněny vlastním flagem do27s, nikdy zdroj neměnily.
Normal shutdown obou finálních instancí bez handle diagnostiky/reportů. 144
starších inputs, dva source files/ADS, sentinel i počáteční output/ADS nezměněny;
konfigurace obnoveny, žádné neočekávané soubory. UNC pouze prefix assertion,
reálný EFS/net share neověřen; READONLY recovery je nativní helper test.

Parent756565a8; runtime patch caf0774032a9098ca53af19f9670ecd626cb152b4a0bffba4bd6b3969df3f005. Evidence evidence/S19 obsahuje manifest,
runtime.patch, host/worker/copy/build-name/contracts/python logy, GUI error32,
lock readiness/release, původní handle diagnostiku a final-validation.json.
Celý native runner/full solution CI/CodeQL neběžely. Lokální/nepushed; následuje
S20 rename+selection, copy/move a delete jako samostatné koherentní podetapy.


## S19 — plán worker I/O bez Branch panelu (2026-10-22)

Převzít izolovaný workerpath a nízkoúrovňové worker copy/move/reopen/ADS,
directory-create a attribute I/O hunks včetně spojených call sites. Explicitně
vynechat DoDeleteFile/Recycling a jeho ThreadWorkerBody call, které patří S20;
nepřidávat operationrecycle.h. BuildName přenést jen UTF16 length boundary,
nikoli nesouvisející icon tracking změnu. Fileswn6 zatím jen strict UTF8/ACP
dekódování a wide planning error-probe; kapacity, operation capture a Branch
planning hunks jsou S20. Neměnit scheduling, access/sharing/dispositions nebo
identity validation. Native donor worker_path/copy tests + scheduling contracts,
host D/R a skutečný GUI copy vlastního Unicode/long souboru s ADS, kde NTFS
podporuje streams. Native CopyReopen je helper-based reimplementation, nikoli
celý production worker; skutečný GUI musí dodat oddělenou integrační evidenci.


## S18 — IMPLEMENTED / obě podetapy lokálně ověřeny (2026-10-22)

S18a commit 1a8f5b77 zachovává úplné launch argumenty/cwd (samostatná evidence).
S18b atomicky přenáší host-owned Salamander.ViewerEnumeration přes QueryService,
owned std::wstring request/result, UI dispatcher v ordinary panelu i Find,
wide internal viewer open/reload/title/navigation a PictView navigation/title.
Legacy char API explicitně odmítá >MAX_PATH-byte result bez posunu cursoru;
wide služba zachytí aliasovaný vstup před vyprázdněním výstupu a krátkou kapacitu
odmítne bez posunu indexu. Žádné změny existujících virtual metod nebo CFileData.
consts/fileswnb mají jen viewer hunks, bez Branch timer/state. SDK zachovává
ordinary-panel komentáře S04/S05; zip ponechává provenance a přidává jen route.
finddlg1, salamdr6, viewer.cpp/2/3, pět PictView souborů a native fixture EXACT.

Host D/R x64 /warnaserror PASS; první Release95s timeout v LTCG zachován,
opakování86.60s PASS. Všech 30 podporovaných SDK consumerů x2 konfigurace PASS,
konkrétní project paths a 60 logů v consumer-matrix.json. Historický Portables
vcproj2015_manison a nepublikovaný SFTP zůstávají excluded. Podporovaný Portables
vcxproj byl samostatně ověřen. První souběžný Salamatrix Release selhal na
sdíleném MarkdigRenderer native mezivýstupu; Debug PASS a samostatný Release
repeat8.61s PASS, WebView2 oba buildy následně sériově PASS. Nejde o opravu SDK
ani o zamaskování původního FAIL. ABI golden pět released interfaces + osm
HEAD interfaces zachovány; release 5.0-samandarin-0.15.1 prefix PASS.

Native viewer_enumeration_tests EXACT: 11 checks/config D/R PASS (identity,
reorder/removal, case/canonical Unicode distinction, long decode/fullpath,
capacity/canary a skutečné ANSI-window wide title). Toto nejsou testy celého
UI dispatcher timeout stroje. Snapshot regression57 Debug PASS; všech22
registrovaných source-contract/Windows PowerShell menu groups PASS; pytest49
PASS; skutečné shipped conversion_table_tests D/R PASS po host staging/restartu.
Celý native runner ani full solution CI/CodeQL znovu neběžely.

Skutečný GUI internal viewer D/R: long Unicode open, obsah, úplný title včetně
CJK/emoji, previous/next; CtrlR načetl nový marker z vlastního změněného souboru,
poté byl source obnoven byte-exact. PictView D/R render64x48, next/prev přes
službu mezi ASCII a Unicode basenames na stejné >MAX_PATH cestě, úplné titles.
První Debug F3 otevřel podle původního private seed interní viewer; následně
byla pouze vlastní testovací konfigurace nastavena na PictView JPEG slot16.
Loaded pictview.spl hash match. Normal shutdown všech vlastních instancí bez
reportů; 135 starších inputs + osm nových files + sentinel nezměněny,
configstorage.ini obnoveny. Vše v evidence/S18b; žádný test uživatelských dat.

Enter donor hunk ve fileswn2 je pouze pro IsBranchView a vyžaduje branch_view.h;
zůstává DEFERRED S26, nikoli nový ordinary launch. Běžný Enter nemá S18 donor
změnu. Branch View stále vypnuté. Parent1a8f5b77; runtime patch ce3854a5b4bd39fcf1ef1b8301503d4e7a91b0f098379885818b2d2e74dc2703.
S18 lokální/nepushed; pokračovat S19 worker paths podle uživatelského goalu.


## S18a — IMPLEMENTED / lokální launch brána PASS (2026-10-22)

Převzaty EXACT donor fileactionpath.h, viewerpath.h, execute.cpp/.h,
dialogs2.cpp, mainwnd4.cpp a čtyři native launch fixture soubory plus source
contract. Fileswn5 obsahuje pouze launch/ViewFile/EditFile hunks; user-menu
mainwnd3 hunks doplňují přímý common/widepath.h include místo budoucího
branch_view.h. Native projekt je automaticky objeven runnerem; všech 59
starších groups zachováno a source contract přidán. SDK/ABI nezměněny.

Host D/R x64 /warnaserror PASS. První Debug build selhal na chybějícím include,
po opravě PASS. První Release build překročil 95s deadline po link/staging;
zachován jako TIMEOUT, opakovaný incremental doběhl 8.17s PASS. Native launch
D/R 12 checks/config PASS: skutečné CreateProcess/ShellExecute/UTF8 batch,
ASCII, Unicode name/parent, byte-long a UTF16-long paths, cleanup. Helper
test připouští explicitní neprovedení dlouhého cwd při chybějícím DOS aliasu;
zde wide DOS alias test prošel a samostatné GUI D/R prokazuje skutečný launch.

Skutečný GUI F4 a AltF3 D/R spustily fixture child z panelu s dlouhou cestou,
češtinou/CJK/emoji. Čtyři UTF16 záznamy přesně odpovídají celému full-name
argumentu a skutečnému cwd po rozvinutí DOS aliasu. 128 předchozích inputs,
dva nové sources a sentinel nezměněny; normální shutdown bez reportů,
konfigurace obnoveny. Shipped conversion_table_tests D/R PASS, sedm source
contracts PASS, pytest 49 PASS. Full runner/full solution CI/CodeQL NEBĚŽELY.

S18 není celá dokončená: následuje S18b enumeration service + PictView
navigation a internal viewer open/refresh/title/previous-next. Enter donor
hunk je pouze Branch-specific a patří S26. Branch View zůstává vypnuté.
Parent 9c6f0643; runtime patch SHA256 186939ddb76fb52a8bf44288a1932bae920f8dcd501b965ed7d777d8602cfce2.
Evidence evidence/S18a source-manifest.json, launch-*, host-*, conversion-*,
source-contract.*, python.*, GUI text/launch/hash manifests a final-validation.
S18a je lokální bez push/PR.


## S18 — plán funkčních podetap (2026-10-22)

S18a převezme fileactionpath/viewerpath helpery, skutečné external editor/viewer
launch call sites a argument/current-directory expansion v execute, mainwnd4,
user menu části mainwnd3 a compare dialogu. Native file_action_path_tests musí
spouštět skutečné child procesy a ověřit přesné argv/cwd; zapsat skutečný výsledek
dlouhého cwd/DOS aliasu. Host D/R a dosavadní source/pytest regrese. Bez změny SDK.
S18b následně atomicky přenese ViewerEnumeration deklarace, implementaci,
QueryService a PictView navigation, internal viewer paths/caption/refresh.
Brána služby zahrne ABI prefix a všechny SDK konzumenty D/R, native enumeration,
skutečné ordinary-panel previous/next a Unicode/long open/refresh/caption.
Enter Branch-only donor hunk ve fileswn2 zůstává S26; žádný Branch stub.
Conversion staging/menu změny vyžadují skutečné shipped-table regression.
S19 a S20 pokračují až po dokončení obou S18 podetap podle uživatelského goalu.


## S17 — IMPLEMENTED / lokální brána PASS s evidovanou S20 copy mezerou (2026-10-22)

Jediný production diff je SalGetFileSize2 v salamdr5.cpp; celý soubor je nyní
EXACT donor 47661e92. Funkce validuje UTF8 před ACP fallbackem, používá wide
open a heap absolutizaci dlouhých relativních cest, zachovává extended paths,
vrací přesný error i nulovou velikost při selhání. S04 ordinary getters,
membership služba, SDK/ABI a snapshot zůstávají beze změny. Žádný nový index
nebo Branch metadata stub. Branch View je stále vypnuté.

Oba disk_file_size_paths_tests soubory EXACT donor: skutečné source functions
a file I/O D/R 79 checks/config PASS, včetně Unicode/UTF8-byte-long/UTF16-long,
relativních a extended cest, symlink target size, ACP1250 fallback, null/missing
paths, error output a cleanup. Symlink subcase zde proběhl, nebyl SKIP.
Branch panel getters driver/template ADAPTED na tři skutečné ordinary gettery
ve fileswn1.cpp a existující wide helpers: 12 checks/config PASS. Všechny
donor Branch assertions zachovány pod S26 guardem, neprovedeny a nejsou PASS.
Existující panel_item_paths_tests znovu 196 checks/config D/R PASS: source,
target/focus, current membership/stale pointers, virtual rejection a frozen ABI.

Host D/R x64 /warnaserror PASS, pytest 49 PASS. Běžný GUI file-link copy D/R
PASS: krátká ASCII cesta link.bin ukazuje na vlastní >400-unit Unicode cíl.
Listing symlinku 0 bytes, skutečný výstup regular file 2107 bytes s přesným
SHA256 cíle. Host binary/source/runtime hash bound, normální shutdown bez
reportů, konfigurace obnoveny. 119 starších input files, šest vlastních
sources/links a sentinel nezměněny; dva nové výstupy ověřeny, žádné temp zbytky.

Samostatný Debug GUI pokus kopírovat symlink přímo z dlouhé Unicode source
cesty selhal Error Opening File / path not found a byl Cancel bez výstupu.
Toto je FAIL zachovaný v evidence/S17/Debug-long-copy-open-error.txt, nikoli
PASS nebo oprava celé file-operation vrstvy. Její migrace je S20; znovu
ověřit tento konkrétní vlastní fixture v S20. Native nová size funkce dlouhou
source cestu i link target podporuje; GUI ordinary smoke není tvrzení, že
všechny host copy I/O větve už jsou migrované.

Parent 60724bd9; runtime patch SHA256
0c91e4246eadec647768d22502face1000cd5e6336f6444ecba5f92d787f9623.
Evidence evidence/S17: source-manifest.json, focused-size.*, ordinary-getters.*,
membership-regression.*, host-*, final-validation.json a GUI logs. Runner
přidává dvě groups, všech 57 starších zachováno. Celý runner/full-solution CI/
CodeQL znovu neběžely; S17 nepushed. Pokračuje lokální S18 podle user goalu.

## S17 — plán zbývající wide disk-size hranice (2026-10-22)

Audit potvrzuje jediný production donor diff: SalGetFileSize2 v salamdr5.cpp.
S04 ordinary panel getters a membership resolver zůstávají beze změny;
neimportovat Branch state nebo stuby metadat. Převzít wide size funkci a
EXACT disk_file_size_paths_tests (skutečný file/link I/O). Nový branch panel
getters harness adaptovat na tři skutečné ordinary gettery ve fileswn1.cpp,
se zachováním všech budoucích Branch assertions pod explicitním S26 guardem;
žádné Branch metadata assertions nyní jako PASS. S04 panel_item_paths_tests
znovu prověří membership, stale pointers, source/target/focus a virtual panels.
Gate host D/R /warnaserror, file-size/link native test, ordinary getter subset,
membership regrese a skutečný běžný panel/file-link smoke na vlastních datech.
SDK/ABI, snapshot, index i zapnutí Branch View zůstávají beze změny.


## S16 — IMPLEMENTED / lokální brána PASS (2026-10-22)

Osm Undelete production souborů a C++ fixture jsou EXACT donor 47661e92.
Owned disk snapshot se zachytí před destination dialogem, Restore používá
skutečné wide identity, heap buffers a RAII source/destination/raw cleanup.
Connect image předvyplní skutečný focused full path, zobrazí jej wide a
před legacy image backendem explicitně kontroluje ACP roundtrip a kapacitu.
Legacy image engine nebyl rozšířen; nepodporovaný vstup je chyba, ne truncation.
Library precomp má potřebné STL/SDK/path includes, celý Undelete consumer
se normálně kompiluje. Veřejný plugin SDK/ABI se nemění.

Python driver má pouze configuration selector (default Both). Native fixture
68 checks/config D/R PASS: skutečné entry/restore/SDK/host snapshot/path
functions a dočasné disk I/O; listing zánik, duplicate basenames, kolize,
focused source, UTF16-long target/source, UTF8-byte-long source, recursive
restore, own-descendant refusal, cancel, missing/create failure a cleanup.
EFS flow/callback/context error checks jsou STUB, real EFS calls=0; nejde
o Windows encrypted EFS import/export. Legacy path tests kontrolují ASCII,
oversize a terminator capacity; lossy ACP assertion je podmíněna 1250/1252.
Registry ACP zde 1250 je evidováno samostatně, není to tvrzení o ACP každého
konkrétního procesu. Donor conditional assertion nebyla oslabena ani odstraněna.

Host+Undelete včetně vlastních library sources D/R x64 /warnaserror PASS,
pytest 49 PASS. Skutečný GUI Connect image D/R PASS: vlastní focused
ž-東京.img v >400-unit cestě je v edit control přesně celý (UIA porovnáno
s manifestem). Po výslovné volbě Disk Image je vstup odmítnut hláškou
The filename or extension is too long před backendem; následně Cancel.
Fyzické disky nebyly analyzovány ani mountovány. Načtené undelete.spl hashově
odpovídají nové sestavě. Oba vlastní vstupy, 116 starších input files a sentinel
nezměněny, žádné temp zbytky, normální shutdown bez reportu, config obnoven.

Parent 8419831e; runtime patch SHA256
60407d0e492458a51c7b4683dfd5b124bf52766d111b759f45dc6cc4af5b4f4c.
Evidence evidence/S16/source-manifest.json, focused-undelete.*, host/undelete-*,
final-validation.json, *-connect-default/image-selected/image-limit-error.txt,
*-loaded-module.json, system-acp.json a shutdown logs. Runner přidává jednu
skupinu, všech 56 starších zachováno. Celý runner, full-solution CI/CodeQL
znovu neběžely; S16 nepushed. Pokračuje lokální S17 podle rozšířeného goalu.

## S16 — plán Undelete owned restore identities (2026-10-22)

Úzký donor diff obsahuje dialogs.cpp/.h, library/precomp.h, restore.cpp/.h,
undelete.cpp, nový undelete_paths.h a projekt: převzít jako paired consumer
bez změny image engine. Restore používá owned snapshot před modal dialogem,
wide I/O a RAII cleanup; Connect image předvyplní přesnou focused cestu a
explicitně kontroluje existující ACP/MAX_PATH hranici bez ztrátové konverze.
Donor native fixture zahrnuje skutečné restore functions, temporary disk I/O
a EFS stub. C++ EXACT, Python pouze configuration selector default Both;
žádné skutečné Windows EFS import/export tvrzení. Gate host+Undelete a všechny
vlastní library consumers D/R /warnaserror, native fixture a chyby/cleanup,
GUI Connect image exact input a refusal před legacy backendem nad vlastním
Unicode image vstupem, běžný panel a zachování souborů. Bez mountování
uživatelských disků a bez přepisu legacy image backendu.


## S15 — IMPLEMENTED / lokální brána PASS s testovacím menu adaptérem (2026-10-22)

Pět PAK production souborů dll/pak_dll.cpp, dll/pakiface.h, spl/pak2.cpp,
spl/precomp.h a vcxproj/pak.vcxproj a C++ fixture jsou EXACT donor 47661e92.
Owned paths a parents zachyceny před modal UI, per-parent notifikace a scope
cleanup. Optimize otevírá OP_WRITE_MODE | OP_EXISTING_ONLY; OpenPak tento
interní flag odstraní z desiredAccess a použije OPEN_EXISTING. Běžný pack
caller ponechává OPEN_ALWAYS. pakiface.h mění jen define, žádnou signaturu
nebo vtable/layout. Aktuální projekt kompiluje engine přímo do pak.spl;
historický dynamický paklib.dll loader je komentář, žádný starý paired DLL
nezůstává v runtime.

Python driver ADAPTED na configuration selector, aktuální shared snapshot
prefix (GetDiskSelectionService singleton, již existující worked-path stub,
vynechání nepoužívaného DemoPlug command shimu); donor C++ assertions EXACT.
15 checks/config D/R PASS: skutečný command body a SDK/host snapshot, listing
zánik, duplicates, per-parent notifications, cancel, UNC, archive-panel contract;
menu engine zde mock. Samostatně skutečné production OpenPak/ClosePak a wide
I/O helper: missing long Unicode input nevytvoří soubor, běžný caller jej
vytvoří, existing-only jej následně otevře; invalid path odmítnut.

Host+PAK D/R /warnaserror PASS, pytest 49 PASS. Optimize příkaz je v targetu
i donoru úmyslně odstraněn z Connect menu; samotný původní menu GUI je SKIP.
Pro skutečný command/engine GUI test použit explicitní dočasný AddMenuItem
adapter pouze v pak.cpp. Jeho combined runtime patch e2dc8bc50ff77a11bc694db
0028dc99843cbae0d0a63a071ebe51c84074de7d9 a testované binárky zachovány v
evidence/S15/gui-adapter; nemění ExecuteMenuItem ani engine. D/R GUI Optimize
na dlouhé Unicode cestě zmenšil vlastní PAK 4475 -> 379 bytes. Nezávislý
parser potvrzuje shodné jméno i bajty členu a odstranění 4096-byte mezery.
111 starších input files, dva nesouvisející PAKy a sentinel nezměněny, missing
inputs nevznikly, žádné temp zbytky, normální shutdown bez reportů, config
obnoven. Soukromý seed přidal PAK slot31; neúspěšné seed pokusy a jejich
příčina (povinný Copyright field) zachovány odděleně.

GUI adapter poté byte-exact odstraněn; pak.cpp nemá žádný diff a finální
production PAK D/R znovu /warnaserror PASS. Netvrdíme, že skrytý příkaz je
dostupný v běžném finálním menu. Původní světle vykreslený Optimize dialog
v Dark Mode je baseline, nebyl přidán nový dialog nebo změna resources.

Parent 9c738940; production runtime patch SHA256
639606ad29e584288b4518ff2360aac59ff84c060f4d13d527516480dff551a6.
Evidence evidence/S15: source-manifest.json, focused-pak-v3.*, host/pak-*,
final-validation.json, gui-adapter/* a private seed provenance. Initial harness
compile chyby jsou zachovány jako FAIL, v3 na finálním zdroji PASS.
Runner přidává PAK group, všech 55 starších zachováno. Celý runner, čistá celá
sestava a CodeQL znovu neběžely. S15 nepushed; navazuje lokální S16.

## S15 — plán PAK Optimize existing-only engine input (2026-10-22)

Převzít pět donorových PAK souborů: dll/pak_dll.cpp, dll/pakiface.h,
spl/pak2.cpp, spl/precomp.h a vcxproj/pak.vcxproj. Owned paths a parents
zachytit před modal UI; OP_EXISTING_ONLY přidává bit do existujícího mode,
OpenPak jej odstraní z desiredAccess a použije OPEN_EXISTING; signatures
a vtable pořadí beze změny. Ostatní pack callers zachovají OPEN_ALWAYS.
Převzít donor pak_optimize_snapshot_tests, C++ EXACT, driver adaptovat jen
configuration selector a již přítomný DemoPlug placeholder sdíleného prefixu.
Gate host+PAK D/R /warnaserror, skutečné OpenPak source methods nad existujícím
i chybějícím long Unicode vstupem a native command/parent/lifetime assertions;
skutečný GUI Optimize vlastního validního PAKu, nezávislá kontrola členů,
sentinel a starší input hashes. Command mock nepředstavuje celý engine.


## S14 — IMPLEMENTED / lokální brána PASS (2026-10-22)

7zip.cpp, precomp.h a vcxproj/7zip.vcxproj jsou EXACT donor 47661e92.
TestArchive zachycuje FOCUSED_ONLY snapshot; skutečná úplná cesta a texty
jsou připravené před progress dialogem. Formátování používá heap string,
capture/invalid UTF16 selhání neodhadují root + basename. Engine, virtual
archive kontrakt, veřejné ABI a host-managed packing/S22 nejsou změněny.

Celý archive_menu_selection_tests.cpp.in je nyní EXACT donor; všechny
7-Zip assertions odložené v S13 obnoveny. Python driver má jedinou adaptaci:
import existujících disk-selection compiler/extraction helpers místo budoucího
renamer driveru. Runner už neomezuje scope na ZIP; počet groups zůstává 55.
Archive harness 34 checks/config D/R PASS: ZIP + 7-Zip Unicode/UNC/long paths,
capture failure, directories, invalid UTF16, corrupt/cancel a progress cleanup.
Engine a snapshot v harnessu jsou test doubles, žádný packing test.

Host+7-Zip a tranzitivní dependencies D/R x64 /warnaserror PASS, pytest 49 PASS.
Skutečný GUI 7-Zip Test Archive D/R PASS na dlouhé Unicode cestě: vybraný
ordinary.zip, focused ž-東京.zip; oba výsledky potvrzují právě ž-東京.zip is OK.
Načtený 7zip.spl odpovídá launch hashům. Engine DLL jsou transient a při
výsledkovém dialogu už unloaded; jejich staged hashes jsou zvlášť evidované,
netvrdíme snapshot jejich načtení. Šest vlastních archivů nezměněno, 12
nezávislých kontrol jejich členů, 104 předchozích input files a sentinel
nezměněny, žádné temp zbytky; normální shutdown bez reportů, config obnoven.
Runtime používal původní soukromý seed s 7-Zip asociací. GUI nekryl 100-CJK
basename; zděděné omezení ZIP menu ze S13 zůstává evidované a neopravené.

Parent baac372d; runtime patch SHA256
504afa34fd152d0b753a293cbba170034241e128f762593a53073142f7d9ded2.
Evidence evidence/S14: source-manifest.json, focused-menu.*, host/sevenzip-*,
final-validation.json, *-unicode-ok.txt, *-loaded-modules.json a *-shutdown.json.
Relevantní lokální brána PASS; celý runner ani full-solution CI/CodeQL znovu
neběžely. S14 nepushed. Další lokální etapa S15 podle rozšířeného user goalu.

## S14 — plán 7-Zip focused archive test input (2026-10-22)

Převzít přesně 7zip.cpp, precomp.h a vcxproj/7zip.vcxproj z připnutého donoru;
diff obsahuje pouze TestArchive, message formatter a SDK include. Obnovit
celý EXACT archive_menu_selection_tests.cpp.in; Python driver jen existing
disk-selection compiler helpers místo budoucího renamer driveru. Runner
zruší ZIP-only scope, zapne beze změny všechny odložené 7-Zip assertions.
Host-managed packing/S22, engine a virtuální archive kontrakt neměnit.
Gate host+7-Zip D/R /warnaserror, kompletní archive command harness,
skutečný GUI Test Archive nad vlastním archivem v dlouhé Unicode cestě,
focused položka nezávisle na ostatním selection. Mock engine není runtime.


## S13 — IMPLEMENTED / lokální brána PASS s baseline omezením menu (2026-10-22)

Tři produkční ZIP soubory main.cpp, precomp.h a vcxproj/zip.vcxproj jsou
EXACT donor 47661e92. ExecuteMenuItem používá jednou zachycený owned snapshot,
respektuje skutečné parents a explicitně odmítá SAL_MAX_PATH byte limit bez
truncation. Archive panel ponechává původní archivní kontrakt. ZIP engine ani
host-managed PackToArchive nebyly změněny; PackToArchive zůstává S22.

Archive menu test driver/template jsou ADAPTED: compiler helpers z již
existujícího disk_selection_snapshot_tests místo budoucího renamer driveru,
scope ZIP/All (default ZIP) výslovně odkládá pouze 7-Zip assertions do S14.
Všechny původní assertions zůstávají ve zdroji. ZIP scope 19 checks/config
D/R PASS: skutečný command body, úplné cesty při zániku listing, ASCII,
Unicode, UNC, UTF8 byte/UTF16 long paths, directories, capture failure,
notifications, formatting, invalid UTF16 a engine byte limit. Host snapshot
a engine jsou zde test doubles; toto není skutečný packing test.

Host+ZIP a tranzitivní dependencies D/R x64 /warnaserror PASS, pytest 49 PASS.
Skutečný ZIP engine přes GUI Test Archive: Debug dvě vybrané položky
ordinary.zip a ž-東京.zip, Release samostatný focused ž-東京.zip bez výběru,
vše v dlouhé Unicode cestě, každý výsledek is OK. Načtené zip.spl hashově
odpovídají právě sestavenému pluginu. Šest vlastních archivů nezměněno,
12 nezávislých ověření obsahu členů, 97 předchozích input files a sentinel
nezměněny, žádné temp zbytky, normální shutdown bez reportu, config obnoven.

Původní soukromý seed měl ZIP asociaci na 7-Zip. Pouze vlastní nový seed
pro S13 ji nastavuje na ZIP index -1, uživatelské nastavení nedotčeno.
Po správné asociaci se samostatně potvrdilo baseline omezení: menu při
výběru 100-CJK basename používá zkrácený CFileData.Name a zakáže příkaz.
Tento GUI případ je SKIP, nikoli PASS; ExecuteMenuItem a nové wide vstupy
se přes něj nespustí. GetMenuItemState je beze změny shodný s donorem.
Omezení musí být vyřešeno před tvrzením úplné GUI podpory takových názvů.

Source parent ac8641ac, runtime patch SHA256
a8c95db05b67ad0545a1edc5374183b341460ff97132c3622d8f12f6d6b1e1b4.
Evidence evidence/S13/source-manifest.json, final-validation.json,
focused-menu.*, host/zip-*.json, *-ok.txt, *-loaded-module.json a private seed
provenance. Runner zachovává starších 54 groups a přidává jednu; celý runner
znovu neběžel. Full-solution CI/CodeQL a push NEPROVEDENY. Pokračuje S14.

## S13 — plán ZIP menu owned input (2026-10-22)

Převzít přesně tři donorové ZIP soubory main.cpp, precomp.h a vcxproj.
Diff obsahuje pouze menu owned snapshot, explicitní SAL_MAX_PATH byte limit,
heap message formatting a notifikaci skutečných parents. PackToArchive/S22
se netýká. Archive menu harness přebírá všechny ZIP assertions; 7-Zip bloky
zůstávají výslovně odložené do S14 pomocí scope ZIP/All (default ZIP), nikoli
označené PASS. Sdílené compiler/extraction helpers použít existující disk
selection driver místo budoucího renamer driveru. Gate host+ZIP D/R x64
/warnaserror, native input/limits/failure a skutečný GUI Test Archive nad
vlastním platným ZIPem; mock engine není skutečný test obsahu archivu.


## S12 — IMPLEMENTED / lokální brána PASS (2026-10-22)

Navazuje na samostatný WIC baseline předkrok 270f2cb6. Owned snapshot
CEnumFiles a wide EXIF regenerace thumbnailu tvoří jeden paired krok.
thumbs.cpp a EXIF exif.c/.h/.def jsou EXACT donor 47661e92; nový C++ test
je také EXACT. Projekt adaptován pouze o spl_diskselection.h, Python driver
pouze o --configuration (default Both zachován). S18 navigation/title zůstává
DEFERRED. Testy S05 obnovují skutečný CEnumFiles a obě HostTests assertions,
zachovávají target DemoPlug a released SDK ABI kontroly.

Aktuální zdroj: parent 270f2cb6; runtime patch SHA256
254d9562832c4c8e17fb1bce6664a6a0a149acce910db9cea53585c9edfd1c52.
Host+PictView+paired EXIF D/R x64 /warnaserror PASS; skutečná staged EXIF
v10, wide i legacy export a bezpečné I/O: 51 checks/config PASS. Zahrnuje
Unicode/long path, temporary cleanup, původní obraz, read-only a locked
failure bez poškození originálu. Snapshot test 57 checks/config PASS,
WIC callback regresní test 39 checks/config PASS, decoder contract PASS,
pytest 49 PASS. Runner přidává thumbnail skupinu a zachovává všech 53 starších.
Celý runner znovu neběžel: jde o relevantní lokální bránu; starší initial
S12 timeouty a shard výsledky jsou zachované, nejsou aktuální full-run PASS.

Skutečné GUI D/R Update Thumbnail PASS: 3 vybrané JPEGy/config v dlouhé
Unicode cestě, ordinary.jpg, ž-東京.jpg a 100 CJK basename. Všech 6 výstupů
má platný JPEG thumbnail v EXIF IFD1; orientation=6 zachována a všechny
non-APP/COM JPEG segmenty včetně compressed scan bajtově shodné s originály.
Skutečně načtené PictView/EXIF DLL odpovídají launch hashes nové sestavy.
Předchozích 87 input files, 3 vstupy předkroku i sentinel nezměněny; žádné
temp zbytky, normální ukončení host/reporter bez reportu, konfigurace obnoveny.
WIC je skutečný dostupný engine; předkrok reuse encoderu není nový engine.

Evidence evidence/S12: source-manifest.json, resumed-final-validation.json,
resumed-*.json/log, *-launch.json, resumed-*-loaded-modules.json.
Starý runtime e22210d1 a původní neúspěšný GUI pokus zůstávají jako initial
evidence; po předkroku byla brána znovu provedena na aktuálním zdroji.
S12 nepushed; full-solution CI/CodeQL NEPROVEDENY. Pokračuje lokální S13.

## S12-preflight — doložená mezera existujícího WIC backendu (2026-10-22)

S12 candidate runtime e22210d1 uložen mimo Git a reverzně odložen, aby se
předkrok sestavil/otestoval samostatně proti S11 45d4a3e7. GUI Update Thumbnail
nad vlastními validními deep Unicode JPEGy skončil jako non-JPEG, žádný soubor
nezměněn. Skutečný Release backend přes přesné PDB function symbols vrací
format=PVF_BMP; nezávislé WIC čtení téhož souboru s extended prefixem rozpozná
JPEG a skutečné CollectFrames/DecodeFrame uspějí. Celý WicBackend.cpp blob
je ef72188c53a6be5a713841475f654c96b102885d v HEAD, releasebase i donoru.

Konkrétní příčina prefixu: AddExtendedPrefixIfNeeded čeká na SAL_MAX_PATH,
ikoli Win32 MAX_PATH, takže validní 415-unit cesta nemá potřebný prefix a
WIC se zbytečně nahrazuje Shell bitmapou. Druhá prokázaná mezera:
sPVSaveImage vrací PVC_UNSUP_OUT_PARAMS pro NULL filename a ignoruje existující
PVSF_USERDEFINED_OUTPUT callback. Regenerace tak nemůže fungovat ani s JPEG.

Nejmenší předkrok: správný Win32 prefix threshold; stávající WIC SaveFrame
zpřístupnit přes IStream z memory při callback output, zachovat původní file
save a encoder/resize/metadata implementaci. Žádný nový image engine, žádná
viewer enumeration/S18. Ověřit skutečný staged backend, validní JPEG output,
callback error/short writes a normální save; host/PictView D/R /warnaserror,
native regrese a skutečný GUI smoke obyčejné operace. Pak samostatný commit
předkroku, obnovit přesně uložený S12 patch a znovu jeho lokální bránu.
Dřívější S12 candidate testy nejsou důkazem hotového předkroku.

## S12 — plán paired PictView thumbnail/EXIF (2026-10-22)

Navazuje na lokálně ověřenou S11 45d4a3e7. Převzít celý donor thumbs.cpp
(CEnumFiles owned snapshot, temporary/attributes RAII, safe replace a wide
EXIF volání), exif.c/.h/.def jako jeden párovaný krok. Projekt přidá pouze
spl_diskselection.h; unicodetitle.h a PictView navigation hunks zůstávají S18.
Převzít oba pictview_thumbnail_paths_tests; obnovit skutečný CEnumFiles a dva
S05 odložené HostTests assertions bez ztráty DemoPlug/ABI rozšíření targetu.
Native harness musí načíst skutečnou právě sestavenou DLL s wide i legacy
exportem, ověřit Unicode/long I/O, původní pixels/EXIF a bezpečnou náhradu.
Driver přidá pouze configuration selector pro bounded runner, zachová default
Both; runner zachová všech 52 starších skupin. Žádný nový image engine.

Gate: host+PictView+EXIF D/R /warnaserror, skutečná DLL, native/pytest regrese,
GUI Regenerate Thumbnail nad vlastními JPEG fixtures a kontrola obsahu,
originálů, temp cleanup a právě načtené EXIF DLL. Pro dostupný open WIC backend
oddělit skutečný runtime od proprietárního enginu; chybějící pokrytí nepočítat
jako PASS. Žádná S18 viewer-enumeration integrace ani publikace této etapy.

## S11 — IMPLEMENTED / lokální brána PASS (2026-10-22)

S10 přijato 60987d42 po zeleném PR #33 včetně skutečného CodeQL a celé
sestavy. Lokální S11 přebírá tři EXACT donor 47661e92 soubory Salamatrix:
salamatrix_packages.cpp, salamatrix_sides.h, vcxproj/salamatrix.vcxproj.
Shared context JSON vychází z owned wide snapshotů; native ItemInfo ABI,
JSON schema, 64-item limit a pět providerů se nemění. Native ItemInfo
překročení odmítá; JSON neprochází jeho fixed byte kapacitami.

Adaptace jsou pouze v testech/runneru: All scope aktivuje S10 odložené
TestSides/TestDynamicJsonContext a všech pět forwarding contracts. Nový
TestPayloadBoundaries kompiluje skutečné CopyResult a protocol header;
ověřuje exact/undersized/null buffer, reálný 65536-byte response limit,
1MiB whole-frame hranici, escaping a nezávislé JSON parsování celé struktury.
Žádné oslabení donor assertions; starší Automation-only scope stále 161
checks/config. All má 243 checks/config. Host+Salamatrix+Python provider
D/R x64 /warnaserror PASS, 52 native groups/config, Python 49 PASS.

Skutečný Python worker -> PackageManager::HostDispatch v D/R PASS: dvě
selected položky, focused 255 UTF16-unit / 763 UTF8-byte CJK basename,
plná 652-unit cesta. Oba hosty/reporter normálně zavřeny, žádný nový report;
3 nové a 84 předchozích input files hashově nezměněné. Soukromé konfigurace
obnoveny, vlastní testovací extensions archivovány mimo runtime. GUI není
běh všech pěti providerů; pět forwarding/capacity kontrol je source contract,
existující process-runtime testy mají skutečný Python/PowerShell a PHP SKIP.
PHP a LoadImageW subcase SKIPs zůstávají SKIP. Evidence evidence/S11,
source-manifest.json a final-validation.json; [steps/S11.md](steps/S11.md).
Runtime patch SHA256 970d582d38b9dbca897f7984a6ff15881117e1676d5c43236163d329fb766b28.

S11 nepushed; publikace/full-solution CI/CodeQL pro S11 NEPROVEDENY.
Podle rozšířeného user goalu po lokálním commitu pokračuje S12. Dřívější
baseline S10 Debug handle a file-focus setter vady nejsou touto etapou opraveny.

## S11 — plán shared sides/context (2026-10-22)

S10 přijato na 60987d42. Převzít pouze tři donorové Salamatrix soubory:
salamatrix_packages.cpp, salamatrix_sides.h a vcxproj/salamatrix.vcxproj.
JSON serializace přímo z wide owned snapshots; native ItemInfo ABI zůstává
pevné a překročení vrací explicitní chybu. Schema a 64-item context limit
beze změny. Pět providerů zůstává forwarding stejného shared host method.
Aktivovat celý odložený selection_snapshot_tests --scope All v runneru;
doplnit mezeru escaping/úplného JSON a CopyResult kapacity/protocol limitu
při kompilaci skutečných produkčních metod, bez jejich duplikování.

Lokální gate: host+Salamatrix D/R x64 /warnaserror, všechny aktuální native
regrese a Python; skutečný smoke změněné operace nad vlastní fixture.
Pět forwarding source contracts není běh pěti runtime prostředí: doložit
skutečně dostupné runtimes a jejich výsledky odděleně. SDK/IDL/CI se nemění.
Žádný push/PR; po lokální bráně pokračovat dalším zadaným krokem do S20.

## S10 — publikováno a přijato (2026-10-22)

Automation 8ef4ab24 přijato po všech 12 SUCCESS checks aktuálního headu
[PR #33](https://github.com/majklvi/salamander/pull/33). Skutečný CodeQL job
119984548335 v runu 39824113333: Initialize, Release x64 MSBuild, Analysis
a SARIF SUCCESS; dokončen 17:53:32 UTC. Čistý full build 39824142309
SUCCESS Win32/x64/ARM64 včetně přípravy závislostí. Jeho CodeQL je záměrně
SKIP pro dispatch, skutečná analýza doložena v PR. Evidence evidence/S10:
accept-current-pr.stdout.log, check-full-build.stdout.log,
check-pr-build.stdout.log a publish-summary.json. Lokální baseline výhrady
(Debug handle tracking, starý dlouhý file-focus setter) i PHP/LoadImageW
subcase SKIPs zůstávají platné, zelené CI je neopravuje.

Přijímá se lokální checkpoint; interní draft se nemerguje do reference base.
Uživatel rozšířil zadání na postupné lokální etapy S11 až S20: zachovat
samostatné commity a každou lokální validační bránu; toto výslovné zadání
nahrazuje dřívější omezení na jedinou etapu v běhu. Další publikace/PR
nejsou tímto automaticky zadány. SOURCE/CONTRACT piny se nemění.

## S10 — IMPLEMENTED / lokálně ověřeno s baseline omezeními (2026-10-22)

S09 přijato na f13b3ed1 po ověření PR #32 a čisté sestavy. Lokální větev
integration/pr769-s10-candidate přebírá pouze Automation: vlastněné wide
COM položky, sdílený disk snapshot collections/enumerators, nezávislé cursory,
Reset bez nového capture a samostatné focused položky. SDK/COM IDL/host,
Salamatrix, CI a SOURCE/CONTRACT piny beze změny. Sedm produkčních souborů
EXACT donor 47661e92; itemcoll.cpp ADAPTED pouze inicializací iKey=-1 po
prokázaném MSVC C4701. Dva testy ADAPTED: explicitní Automation/All scope
odkládá pouze budoucí S11 a přidává skutečné COM collection/enumerator metody.
Všechny donorové Automation assertions zachovány, S11 assertions ponechány
v template/All režimu. Runner zachovává všech 51 starších skupin.

Host a Automation Debug/Release x64 /warnaserror PASS, 161 focused native
checks/config, 52 regresních groups/config a Python 49 PASS. Skutečný plugin
v D/R prošel 32 COM kontrolami/config nad devíti kategoriemi metadata/cest,
včetně spuštění skriptu na 344 UTF8-byte a 416 UTF16-unit cestě. Retained
items/collections/enumerators přežily deselection, refresh, změnu panel path
a další focused getter. Sort/destrukce řádků a poslední snapshot/COM release
ověřeny nativně nad skutečnými migrovanými metodami; GUI samo není důkazem
všech refcountů. Všech 13 finálních vlastních a 71 předchozích vstupů hashově
beze změny, konfigurace obnovena, oba finální hosty normálně ukončeny.

Baseline výhrady: původní scriptlist.cpp v Debug při raw CreateFileW a
tracked CloseHandle hlásí neevidovaný handle; před skriptem zaznamenáno a
potvrzeno, diagnostika nevypnuta. Původní host ChangeDir při přímém zaměření
souboru přes MAX_PATH vyvolal buffer assertion; první Debug host byl řízeně
ukončen, nejde o normální shutdown. Finální dlouhé cesty proto ověřeny přes
přechod do adresáře a skutečné COM Items. Tento starší setter limit zůstává
NEOPRAVEN; S10 nelze prezentovat jako úplnou podporu dlouhého file-focus
přechodu. Přesné místo a shoda s releasebase doloženy mimo Git.
PHP/LoadImageW subcase SKIPs zůstávají SKIP. Evidence evidence/S10 včetně
neúspěšných pokusů, source-manifest.json, runtime.patch, final-validation.json
a [steps/S10.md](steps/S10.md). Lokální runtime patch SHA256
cad448992989c7d69d9f6a90906765565eab8a6e1316f918979e447a0c13a7c7.
Publikační full build/platformní CI/CodeQL S10 NEPROVEDENY. S10 nepushed,
accepted reference zůstává f13b3ed1. S11 v tomto běhu nezahájena.

## S10 — plán Automation (2026-10-22)

S09 přijato na f13b3ed1; lokální integration/pr769-s10-candidate. Převzít
8 produkčních souborů Automation v úzkém donor diffu a matching testy.
COM item vlastní wide Name/Path, collection sdílí owned disk snapshot s
každým enumerátorem, Reset neopakuje capture, focused getter vytváří vlastní
item. Zachovat legacy archive/FS a virtuální .., COM ABI/IDL a kapacitní
pravidla focused script launch. Žádné změny Salamatrix/SDK/host/CI/Branch.

Nově zjištěná testová vazba: selection_snapshot_tests.py extrahuje
SidesService::CopyDiskItemInfo a GetSelectedItem/GetFocusedItem ze
salamatrix_sides.h, SideItemJson wide overload a DiskSideContextJson ze
salamatrix_packages.cpp. Tyto symboly patří do S11, v targetu zatím chybí.
Samotná produkční Automation migrace tuto závislost nemá. Podle CONTRACT
odložit pouze TestSides/TestDynamicJsonContext + 5 provider forwarding
assertions do S11; zachovat je v donorovém template/All režimu. S10 test driver
získá explicitní scope Automation/All, default Automation. Převzatý
TestAutomation neoslabit. Nejmenší doplnění mezery: nativně kompilovat i
skutečné collection/enumerator metody + CUnknownImpl, ověřit Next/Reset,
nezávislé enumerátory, uvolnění collection a COM item/snapshot cleanup.

Validační brána: host+Automation Debug/Release x64 /warnaserror, focused
native a celé regrese, skutečný Automation script přes plugin v D/R nad
vlastními fixtures. Zachované collections/items/enumerator musí přežít
DeselectAll, refresh/sort a změnu panel path i nové focused getters; úplné
Unicode/UTF8-long/UTF16-long properties, žádné borrowed rows. Nepoužívat
Salamatrix transport jako náhradu Automation COM. Vlastní skripty pouze
čtou metadata a zapisují test report mimo originály; 71 předchozích originálů
ověřit hashově. Publikační CI/CodeQL/full build S10 až po dalším zadání.

## S09 — publikováno a přijato (2026-10-22)

Split & Combine 8b97128d přijato po 12 SUCCESS checks přesného headu
v interním [draftu PR #32](https://github.com/majklvi/salamander/pull/32).
PR buildy Win32/x64/ARM64 /warnaserror, native/Python/source-contract,
VSIX, growth a squash guard prošly. Skutečný CodeQL job 119952659278
v PR runu 39814889856 dokončen 2026-10-22 16:48:06 UTC: Initialize,
Release x64 build, Perform Analysis a Upload SARIF SUCCESS. Samostatný
čistý full build 39814915560 SUCCESS na všech třech platformách včetně
přípravy závislostí. Jeho CodeQL záměrně SKIP pro workflow_dispatch;
skutečná analýza je doložena samostatně. Evidence mimo Git evidence/S09:
check-current-pr.stdout.log, check-full-build.stdout.log,
check-codeql-job.stdout.log a publish-summary.json. Lokální GUI/obsahové
důkazy a známé PHP/LoadImageW subcase SKIPs zůstávají evidované.

Přijímá se lokální checkpoint, draft se nemerguje do samandarin-0.16.
SOURCE/CONTRACT, donor/release piny, SDK a Branch gate beze změny.
Navazuje pouze S10 Automation; Salamatrix patří do S11.

## S09 — IMPLEMENTED / lokální brána PASS (2026-10-22)

S08 přijato na 68b66c33 po 12 SUCCESS checks PR #28, skutečné CodeQL analýze
včetně SARIF a čistém Win32/x64/ARM64 full buildu. Lokální větev
integration/pr769-s09-candidate přebírá jen Split & Combine: owned selection
před dialogem/I/O, skutečný parent pro default target/sibling discovery,
full paths explicitních dílů, wide API/dialogy a UTF8 BAT. Všech 10 produkčních
a 2 testové soubory jsou EXACT donor 47661e92. Jediný adaptační kód je
registrace selection_paths_tests.py s podporovaným --configuration ve
stávajícím runneru; všech 50 starších skupin zachováno. SDK/host/ostatní
pluginy/CI/Branch gate ani SOURCE/CONTRACT se nemění. Input volume/file IDs
se kontrolují před target creation; read handles bez write/delete sharing
chrání i před target-alias race. RAII zachovává handle/progress cleanup.

Host+plugin Debug/Release x64 /warnaserror PASS; native actual command/target/
plan/engine/BAT harness 210 checks/config PASS; celé native sady 51 groups/config
PASS; stávající CI pytest výběr 49 PASS. Host Debug prvně selhal až ve staging
skriptu na chybějícím Get-FileHash kvůli PSModulePath; opravena pouze invocation
environment, oba konečné buildy PASS. Neúspěšný protokol zachován. PHP runtime
a historický LoadImageW long-bitmap backend subcase zůstávají SKIP, ne PASS.

Skutečné GUI v produktovém Windows Dark Mode (experimental): Debug ASCII,
Debug Unicode UTF8-long a Release Unicode UTF16-long Split/Combine. Devět
vytvořených dílů porovnáno po bajtech a pořadí, tři výsledky přesně shodné
se zdroji; tři UTF8 BAT/metadata/CRC32 ověřeny nezávisle. Focus-only discovery
našlo správné tři sourozence a načetlo původní název (včetně % & ^ !).
Relativní nový Combine target se vztahoval ke skutečnému parentu. Skutečný
GUI pokus zapsat do input dílu i jeho hardlinku byl odmítnut bez změny dílů.
Aktivní Split i Combine nad 4GiB souborem byly zrušeny přes Cancel/Yes;
nedokončené outputy odstraněny, původní zdroj zachován. Všech 5 vlastních
+ 66 předchozích originálů má původní SHA256; přesný inventář 16 očekávaných
outputů navíc. Čtyři vlastní hosty/reportery normálně ukončeny, žádný nový
report, configstorage.ini obou konfigurací bajtově obnoveny.

Evidence mimo Git evidence/S09; testovaný parent 68b66c33 + runtime.patch
SHA256 18a3a0ac9ce186c114b295d266abeb5562dd6d0306fab394be4b92ff2ea267e2.
Source manifest ověřuje přesné donorové bloby/raw SHA i runner; launch logy
vazbu na skutečné host/plugin binárky. Finalize-S09.py znovu ověřil celý
řetězec po GUI. [steps/S09.md](steps/S09.md) uvádí příkazy a meze pokrytí.
Mixed-parent rows, alias race, resolver/read failure a odmítnutí adresáře
jsou native fixture; skutečný UNC share/BAT execution/pickery nebyly GUI
ověřeny. Publikační čistý full build/platformní CI/CodeQL S09 NEPROVEDENY,
větev zatím lokální. Další krok push/fork CI po zadání uživatele; po přijetí
S09 následuje S10 Automation. S10 v tomto běhu nezahájena.

## S09 — plán Split & Combine (2026-10-22)

Přijatý checkpoint S08 68b66c33; lokální integration/pr769-s09-candidate.
Úzký donor diff má 12 souborů výhradně src/plugins/splitcbn: owned input
selection/plan, skutečný parent pro výchozí cíle a sibling discovery,
wide I/O/dialogs, UTF8 BAT, RAII/progress cleanup a input identity guards.
Ponechat zdroje otevřené bez write/delete sharing a porovnat volume/file ID
před vytvořením cíle; zachovat i ochranu proti target-alias race. Převzít
celý nový splitcbn_paths.h a matching selection_paths_tests.py/.cpp.in.
Driver podporuje --configuration; zaregistrovat jen nový test v existujícím
runneru, ponechat všech 50 stávajících skupin. Bez host/SDK/Branch/CI změn.

Ověřit native actual consumer Debug/Release, host+plugin /warnaserror v obou
konfiguracích a celé regresní sady. Poté reálný GUI Split/Combine a nezávislé
porovnání obsahu/pořadí dílů nad vlastními ASCII/Unicode/UTF8-long/UTF16-long
fixtures; UTF8 BAT metadata, implicitní target a discovery, Cancel, stejné
input/output a hardlink bezpečně odmítnuty. Native mixed-parent/race fixture
není náhradou GUI. Zachovat všech 66 předchozích originálů a vlastní vstupy.
Nepodporovanou podmínku vykázat jako SKIP, ne PASS; důkazy parent+patch+runtime.
S09 bez publikace; čistý full build/platformní CI/CodeQL až při autorizovaném
publikačním checkpointu. S10 v tomto běhu nezačínat.

## S08 — publikováno a přijato (2026-10-22)

Checksum c76b52b5 přijato po ověření všech 12 checks přesného headu
v interním [draftu PR #28](https://github.com/majklvi/salamander/pull/28).
Win32/x64/ARM64 Debug /warnaserror, native/Python/source-contract, VSIX,
growth a squash guard SUCCESS. Skutečný CodeQL job 116089251585 dokončen
2026-10-14 20:44:16 UTC: Release x64 build, Perform Analysis a Upload SARIF
SUCCESS. Čistý full build 38632382603 SUCCESS na všech třech platformách;
CodeQL v dispatch záměrně SKIP, skutečná analýza v PR runu 38632358631.
Evidence mimo Git evidence/S08/{resume-current-pr.stdout.log,
resume-full-build.stdout.log,resume-codeql-job.stdout.log,publish-summary.json}.
Lokální GUI/test důkazy a evidované PHP/LoadImageW subcase SKIPs platí dál.

Přijímá se lokální checkpoint; draft se nemerguje do samandarin-0.16.
SOURCE/CONTRACT, donor, release base, SDK i veřejný Branch gate se nemění.
Navazuje pouze S09 Split & Combine s exact input paths a input safety.

## S08 — IMPLEMENTED / lokální brána PASS (2026-10-14)

S07 včetně obecné CI opravy přijato na dec9c828 po čerstvém ověření všech
10 checks PR #27, skutečné CodeQL analýzy/SARIF a čistého full buildu
Win32/x64/ARM64. Nová lokální větev integration/pr769-s08-candidate přebírá
Checksum jako jeden celek: owned capture před modeless UI/workerem, přesné
FullPath oddělené od display/export jména, wide rekurze/I/O/export a Verify
podle skutečného rodiče manifestu. Všech 9 produkčních a 2 testové soubory
odpovídají donorovým Git blobům 47661e92 beze změny. Jediná adaptace je
registrace testu do stávajícího runneru bez neexistujícího --configuration
přepínače (driver sám vždy testuje D/R). SDK, algoritmy, resources/formát
manifestu, disk-only guard, Branch gate a obecná CI oprava zůstávají zachovány.

Host+Checksum Debug/Release x64 /warnaserror PASS; native actual-consumer
harness 62 checks/config PASS, celé native sady 50 groups/config PASS,
CI pytest výběr 49 PASS. Native digest facade je stub; reálné algoritmy
ověřil skutečný plugin přes GUI: 80 nezávislých CRC/MD5/SHA1/SHA256/SHA512
porovnání nad osmi zdroji v D/R, 32 úplných exportních identit a digestů
(MD5 relativně, SHA256 v jiném Unicode adresáři), 32 pozitivních Verify
řádků. Negativní manifest přesně 1 Corrupted + 7 OK, 0 missing/skipped.
Zrušení aktivního výpočtu 512 MiB přešlo do Canceled bez exportu. Debug
Calculate/export přežil změnu source panelu a refresh po capture; native
queued worker přežil úplné zrušení panel rows včetně mixed-parent fixtures.
Všech 11 vlastních + 55 předchozích originálů má přesné původní cesty/obsahy;
jen 4 očekávané exporty navíc. Všechny 4 hosty/reportery normálně ukončeny
bez nového reportu, oba configstorage.ini bajtově obnoveny.

Vazba evidence: parent dec9c828 + runtime.patch SHA256
728f8592e28333d8d5b3e89ab8f348d0bc1202954afc36a5e20f8571a63f71b1,
donorové bloby/raw hashe a hashe skutečně spuštěného hostu/Checksum pluginu.
Podrobné příkazy a limity: [steps/S08.md](steps/S08.md), evidence mimo Git
v evidence/S08. Počáteční infrastruktura test runneru neměla msbuild v PATH;
po opravě pouze prostředí celá sada PASS, původní neúspěšný protokol zachován.
PHP a historický LoadImageW long-bitmap subcase SKIPs nejsou vydávány za
provedené testy. Nová publikační platformní CI/CodeQL a čistý full build S08
zatím NEPROVEDENY: větev je lokální. Accepted reference zůstává dec9c828.
Další checkpoint je push/fork CI po zadání uživatele, potom přijetí S08;
S09 Split & Combine nebylo zahájeno.

## S08 — plán úzké migrace Checksum (2026-10-14)

Předpoklady S05 a S07 ověřeny; accepted reference dec9c828. Úzký diff:
9 produkčních souborů checksum (včetně nového checksum_paths.h) a 2 testové
soubory. Přenést owned seed/FullPath před modeless threadem, oddělené
relative display/export jméno, wide rekurzi/I/O/export a Verify podle
DirectoryW skutečného manifestu. Zachovat algoritmy, formát manifestu,
nastavení/dialog resources a stávající disk-only guard. Bez SDK/Branch UI.
Native harness kompiluje reálné commands/service/worker/parser/export;
CHashAlgo digest je však stub. Proto vedle native běhu ověřit skutečný
plugin Calculate + Save + Verify proti nezávislým CRC/SHA hashi nad vlastními
ASCII/Unicode/UTF8-long/UTF16-long soubory i více Unicode komponentami.
Export v témže i jiném adresáři, Cancel beze změn, modeless panel refresh/
změna selection po capture; mixed-parent duplicitní basename jen native.
Driver checksum_selection_paths_tests.py nemá --configuration a sám vždy
běží Debug i Release; registrace do runneru jej nebude volat s neexistujícím
přepínačem. Před commitem host+Checksum D/R /warnaserror, všechny regrese,
skutečný GUI smoke, exact zdrojové/runtime hashe a nedotčenost starších
55 fixtures. Do další etapy ani publikace se v tomto běhu nepokračuje.

## S07 — publikováno a přijato (2026-10-14)

Batch Rename 3f9a0134 a obecná oprava full-build CI 6c1b034e přijaty po
čerstvém ověření všech 10 checks přesného headu 6c1b034e v interním
[draftu PR #27](https://github.com/majklvi/salamander/pull/27).
Win32/x64/ARM64 /warnaserror, native/Python, VSIX, growth a squash guard
SUCCESS. Skutečný CodeQL job 116018461833 dokončen 18:19:06 UTC: Initialize,
Release MSBuild, Perform Analysis i Upload SARIF SUCCESS. Samostatný čistý
full build 38611664410 SUCCESS ve všech třech architekturách včetně skutečné
instalace Lua a TagLib. Jeho CodeQL job je záměrně SKIP pro workflow_dispatch;
skutečná analýza proběhla v PR runu 38611675021. Původní job na 3f9a0134 byl
nahrazen novým headem a není vydáván za dokončený upload. Evidence mimo Git:
evidence/S07/{acceptance-current-pr.stdout.log,ci-fix-verified.json,
ci-fix-full-final.stdout.log,ci-fix-final-codeql-job.stdout.log,publish-summary.json}.

Přijímá se lokální integrační checkpoint, nikoli merge draftu do release base.
Draft zůstává otevřený; SOURCE/CONTRACT, donor a release base a84dbed6 se
nemění. PHP/LoadImageW subcase SKIPs zůstávají evidované. Navazuje pouze
S08 Checksum; smíšené rodiče zatím native fixture, veřejné Branch UI zůstává
vypnuté. S07 GUI/obsahové důkazy i známé Filecomp UI limity zůstávají platné.

## S07-CI — oprava závislostí ručního full buildu (2026-10-14)

Samostatný workflow_dispatch run 38609591197 na S07 3f9a0134 selhal ve
Win32/x64/ARM64: Lua runtime a mmviewer TagLib nebyly připraveny. Select
changed projects se při dispatch nespouští, takže chyběly needs_lua a
needs_mmviewer_taglib outputs a instalační krok byl přeskočen. Nejmenší
oprava: při workflow_dispatch spustit tentýž instalační krok s oběma
příznaky true; PR výběr i oddělený statický TagLib install root zachovat.
Nejde o fork-specific aktivaci workflow; opravuje se společná ruční cesta.
Aplikační kód, SDK, kontroly /warnaserror, SFTP a reference beze změny.
Ověření: existující Python workflow testy, nový čistý build všech tří
architektur a skutečné dokončení CodeQL na novém přesném headu PR #27.
Do dokončení těchto běhů publikační brána zůstává PENDING.

## S07 — IMPLEMENTED / lokální brána PASS (2026-10-14)

S06 přijato na f43b9d86 po 12 úspěšných kontrolách PR #26 včetně skutečného
CodeQL. Nová lokální větev integration/pr769-s07-candidate obsahuje Batch
Rename jako jednu etapu: owned snapshot před workerem, přesné rodiče položek,
relative/full mode, preview, wide I/O a Undo. Všech 14 produkčních a 4 testové
soubory odpovídají blobům připnutého donoru 47661e92; vlastní adaptace je jen
registrace dvou testů do stávajícího runneru se zachováním všech starších testů.
SDK/CFileData, disk-only menu guard a Branch gate se nemění.

Host+Renamer Debug/Release x64 /warnaserror PASS. Native panel harness
67 checks/config a I/O harness 89 checks/config PASS; celé native sady
49 groups/config PASS; CI pytest výběr 49 PASS. Přijatý předchozí consumer
negativně: kompilace PASS, 31 očekávaných selhání ze 40 checks (Debug).
Pět vlastních GUI hostů, 21 skutečných přejmenování souborů a jejich Undo
ve čtyřech dávkách: celá devíti-souborová sada v D/R včetně ASCII, Unicode
jména, více Unicode rodičů, UTF8>260 i UTF16>260 cest. Samostatný Debug
duplicate test prokázal rozdílný obsah, třetí nevybranou položku a životnost
po refresh/sort panelu; relative mode zkontrolován v preview. Cancel dlouhého
jména beze změn. Všech 55 fixtures nakonec na původních cestách s původním
obsahem, oba configstorage.ini obnoveny; všech 5 hostů/reporterů normálně
ukončeno bez nového reportu. Uživatelská instalace nedotčena.

Evidenci váže parent f43b9d86 + runtime patch SHA256
c4ae16be26caf38ec5ea1f24520351ceb07a102b05de1236135998fabd06c653,
zdrojové/blokové a binární hashe; podrobně [steps/S07.md](steps/S07.md).
PHP není dostupné a dva historické Win32 LoadImageW long-bitmap subcases
jsou SKIP, nikoli provedené PASS. Nová S07 platformní CI/CodeQL ani čistý
build celé sestavy zatím neběžely: větev nebyla publikována. Accepted reference
zůstává f43b9d86. Nejbližší checkpoint je push a fork CI po zadání uživatele;
teprve po přijetí S07 navazuje S08 Checksum. S08 nebylo zahájeno.

## S07 — plán úzké migrace Batch Rename (2026-10-14)

S06 přijat na kódu 9580b153 po 12 checks včetně skutečného CodeQL; lokální
accepted reference f43b9d86 (jen acceptance dokumentace). S07 provádí pouze
14 souvisejících produkčních souborů renamer včetně dvou path/I/O headers a
čtyři donorové testové soubory podle mapy. Existující disk-only menu guard
MENU_EVENT_DISK zůstává; nová podpora archivů/FS ani Branch UI se nepřidává.

Přenést společně snapshot intake před worker threadem, owned CSourceFile,
per-item rodiče, relative/full mode, preview, wide rename/move I/O a Undo.
Kapacita 4*SAL_MAX_PATH na haldě, Win32 extended cesty a ne-throwing source
factory včetně všech donorových failure větví. Bez nového SDK/scanneru.
Native panel harness zkompiluje skutečný intake/konstruktory/generování cíle,
I/O harness skutečné wide wrappers nad disposable fixtures. Oba D/R.
Povinná GUI brána: Cancel beze změny souborů, preview/sort/refresh životnost,
skutečné přejmenování a Undo s různými obsahy stejných basename ve dvou
rodičích, Unicode jméno/rodiče a obě délkové hranice. Native wrapper samotný
není důkazem skutečného GUI Undo. Zdrojový donorový payload basename-derived
u duplicate FileCase sám nedokazuje rozlišení obsahů; tuto mezeru pokryjí
odlišné souběžné vlastní fixture markery. Běžný panel/shutdown D/R, host+
renamer /warnaserror, regresní sady, přesné hashe a obnova bootstrapů.
Do accepted reference S07 povýšit až po samostatném publikování a CI headu.

## S06 — publikováno a přijato (2026-10-14)

Kód 9580b153434bde8942dd3b1604aa478fb5e276ea na
origin/integration/pr769-s06-candidate ověřen interním
[draftem PR #26](https://github.com/majklvi/salamander/pull/26).
Všech 12 checks přesného headu SUCCESS: Win32/x64/ARM64 /warnaserror,
native/Python/reporty, VSIX, growth a squash guard; CodeQL gate i skutečná
analýza. CodeQL job 115981207148 skončil 17:04:14 UTC; Initialize,
Release MSBuild, Perform Analysis a Upload SARIF SUCCESS. Žádný check
není pending/skipped. Lokální PHP/LoadImageW subcase SKIPs a známé nezměněné
Filecomp výstupní UI limity zůstávají pravdivě evidované níže.
Při zahájení S07 znovu ověřeno: evidence/S06-final/acceptance-current-pr.stdout.log,
ci-current-codeql-job.stdout.log a publish-summary.json.
Přijímá se integrační checkpoint S06; draft zůstává otevřený a nemerguje se
do samandarin-0.16. SOURCE/CONTRACT ani release base a84dbed6 beze změny.
Navazuje pouze S07 Batch Rename podle jejího úzkého donorového diffu.

## S06 — IMPLEMENTED / lokální brána PASS (2026-10-14)

S06-preflightb přijat na 2d836336 po čerstvém ověření všech 12 kontrol
PR #25 včetně skutečné CodeQL analýzy a SARIF uploadu, job 115946770877.
Lokální accepted reference je 2d836336; draft se nemerguje do release base.

File Comparator nyní přebírá owned disk selection snapshots místo živých
řádků panelu. Přesné wide identity a názvy určují dvojici; nejednoznačný
target basename zůstane prázdný, chyba služby nevede k odhadu root/name.
Zachováno selected/focused chování, source/target a pořadí left/right.
UTF8 vstupy jsou heap-backed a kapacitně kontrolované před spuštěním threadu.
Produkční hunks a Python test driver odpovídají připnutému donoru;
native template navíc ověřuje 3 mezní případy aktivní snapshot větve.
SDK, CFileData, engine, dialog cleanup, ikonočtečka a Branch gate beze změny.

Host+Filecomp Debug/Release x64 /warnaserror PASS; skutečný command/service
native harness 58 checks/config PASS, celé sady 47 groups/config PASS,
pytest 49 PASS. Původní přijatý consumer negativně 23 failures/51 checks.
Ruční Compare: 11 úspěšných dvojic s přesnými vstupy/skutečnými obsahy,
Unicode jménem/adresáři a oběma dlouhými cestami v D/R, včetně správného
left/right pořadí při pravém zdroji. Všech 10 vlastních hostů i reporterů
normálně ukončeno, žádný nový report, 45 původních fixtures beze změny.
Známé nezměněné ANSI výstupní hlavičky a zkrácený Unicode caption suffix
nejsou označeny jako opravené ani jako plný Unicode UI PASS.

Podrobnosti, přesné příkazy a omezení: [steps/S06.md](steps/S06.md), evidence
mimo Git v evidence/S06-final. PHP a LoadImageW subcase SKIPs zachovány.
Větev integration/pr769-s06-candidate je lokální; nové S06 CI/CodeQL a čistá
publikační platformní matice NOT RUN. Před přijetím/pokračováním S07 nejprve
publikovat S06, Ensure-ForkCI a ověřit přesný head. Tento krok push neopravňuje.

## S06 — obnovená migrace po přijetí S06-preflightb (2026-10-14)

PR #25 head 2d836336 má čerstvě ověřeno všech 12 kontrol SUCCESS včetně
skutečné CodeQL analýzy a SARIF uploadu (job 115946770877). Přijatá lokální
reference je 2d836336; interní draft zůstává otevřený a release base se nemění.
Uložený S06-owned patch byl ověřen hashem a obnoven nad touto opravou.
Tři produkční Filecomp soubory a native Python driver odpovídají přesně
připnutému donoru. Native template zachovává tři doplněné kontroly kapacity;
runner nyní obsahuje oba testy, ikonočtečku i Filecomp owned consumer.

V tomto běhu pouze S06: cílený skutečný command/service harness a negativní
kontrola přijatého starého consumeru; host+Filecomp Debug/Release x64,
celé regresní sady a skutečné Compare I/O se selected/focused/source/target,
Unicode názvem, více Unicode komponentami a oběma dlouhými cestami.
Ukončení musí zahrnout host i reporter a nepřítomnost nového reportu.
Nepřebírat Branch UI, SDK ani pozdější icon/overlay migraci. Známé nezměněné
výstupní ANSI hlavičky Filecomp a dark dialog starých reportů evidovat
samostatně; netvrdit jejich opravu. Evidence: evidence/S06-final.

## S06-preflightb — IMPLEMENTED / lokální brána PASS (2026-10-14)

Vlastní S06 Compare migrace narazila při shutdownu dlouhých panelů na původní
přepsání stacku v IconThreadThreadFBody. Podle CONTRACT se provádí jen tento
nejmenší bezpečnostní předkrok: heap-backed narrow/wide scratch, skutečná
kapacita konverze UTF8-first/ACP a wide offset v převedených znacích.
Legacy MAX_PATH guardy pro shell/overlay zůstávají; plná migrace S26/S28
se nepřebírá. SDK, CFileData, diagnostika a tmavé scrollbary beze změny.

Host i obnovený Bug Reporter Debug/Release x64 /warnaserror PASS; Filecomp
pro runtime znovu sestaven z přijatého parentu bez vlastní S06 migrace.
Skutečný preparační blok nativně 93 checks/config PASS, celé sady 46 groups/
config a pytest 49 PASS. PHP a LoadImageW subcase SKIPs zůstávají evidované.
Skutečné běžné panely se zdroji 390/391 UTF8 bajtů (220/221 UTF16 jednotek)
a 407/408 UTF16 jednotek: D/R listing, F3 s přesnou celou cestou/obsahem,
normální shutdown hostu i jeho reporteru, žádný nový report — čtyři PASS.
Všech 45 původních fixture souborů/sentinelů beze změny, bootstrapy obnovené.

Podrobnosti a omezení: [steps/S06-preflightb.md](steps/S06-preflightb.md).
Evidence mimo Git: evidence/S06-preflightb/{local-summary,source-manifest,
gui-observations}.json, runtime.patch, build reporty a JUnit. Připravená S06
migrace/testy jsou hashově uchovány v evidence/S06-owned/pending-S06-owned.patch;
v tomto commitu nejsou a S06 se neoznačuje za dokončené.

Větev integration/pr769-s06-preflightb-candidate je pouze lokální.
Nové fork CI, CodeQL a publikační platformní matice NOT RUN; accepted
reference zůstává bf78c81723abc983805a61acaeec6d92fd4b2316. Další krok:
publikovat tento předkrok, vytvořit/reuse interní draft přes Ensure-ForkCI,
ověřit přesný head a poté obnovit S06 patch včetně obou native registrací.

## S06-preflightb — bezpečný scratch ikonočtečky běžného panelu

S06 migrace je připravena a native 58 checks/config, celé sady 46/config
prošly; ruční Compare načte Unicode i dlouhé cesty. GUI brána však při
shutdownu selhala: RTC Stack memory corruption, reporty vlastních procesů
2552/15196 a původní IconThreadThreadFBody v src/fileswn1.cpp:1824–1842.
memmove kopíruje celý GetPath() do char path[MAX_PATH+10], poté zapisuje
wPath přes wName=wPath+l, kde l je počet bajtů cesty. 390/391 UTF8 a
407/408 UTF16 cesty prokazatelně překračují scratch. Donor (pozdější S26/S28) nahrazuje
narrow scratch CPathBuffer(4*SAL_MAX_PATH+16), ale celý worker navíc závisí
na BranchView cache keys a novém overlay API, které S06 nezavádí.

Nejmenší bezpečnostní předkrok: převzít pouze donorový heap narrow scratch,
heap wide scratch SAL_MAX_PATH+16 (panel Path má 32768 bajtů), skutečnou
kapacitu konverze a wide offset podle počtu převedených znaků, UTF8-first/ACP. Zachovat stávající MAX_PATH guardy
legacy shell/overlay operací; nepředstírat hotovou long-icon/Branch migraci.
Ověřit skutečný preparační blok nativně v D/R, hranice a canary integrity,
oba host buildy, regresní sady a skutečný dlouhý panel/shutdown v D/R.
Původní stack/heap diagnostika se nevypíná. Až samostatně tento předkrok
uzavřít; S06 migrace je kompletně uchována v evidence/S06-owned/
pending-S06-owned.patch s manifestem a výsledky. Do accepted reference
se nepřijímá, její kód ani testy v tomto předkroku nejsou přítomny.

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

## S06-preflight — lokální brána PASS (2026-10-14)

Opraven pouze lifecycle drop callbacku dvou File Comparator vstupů.
Podrobnosti, původ chyby, příkazy, omezení a další krok:
[steps/S06-preflight.md](steps/S06-preflight.md).
D/R host+plugin /warnaserror, 964 native lifecycle checks/config,
45 regresních groups/config, pytest 49 a skutečný Compare/Cancel/shutdown
PASS. Původní callback je v negativní kontrole prokazatelně vadný.
Předkrok je lokální; S06 owned-selection migrace PENDING. Před ní publikovat
předkrok a ověřit CI jeho přesného headu. Accepted reference ffa1519d (S05).

## S06-preflight — izolace výchozího heap hlášení

Před převzetím ExecuteCompare v src/plugins/filecomp/filecomp.cpp baseline
89698944 skutečně zobrazí správné dva rozdíly, ale při shutdownu Debug
hlásí heap leak. Samotné otevření a Cancel původního Compare dialogu stačí:
CRT dump 26 normal blocks / 928 bytes, včetně dvou COMBOBOX skupin a MENU.
To není donorová migrace ani důkaz OS-owned úniku. Nejmenší předkrok:
zachytit alokační call stack konkrétních neuklizených bloků; případně opravit
jen prokázaného vlastníka a ověřit D/R host, dotčený plugin, native/GUI.
Dočasná diagnostika musí být před finálními buildy beze zbytku odstraněna.
IsKnownSystemMenuLeakDiff se nesmí rozšířit a heap tracker se nevypíná.
V tomto běhu nejprve tento předkrok; migrace S06 až po jeho uzavření.

## S05 — publikováno a přijato (2026-10-14)

Kód `896989447ad410c4970d469d9f961c62e5b7a8dc` je pushnut na
`origin/integration/pr769-s05-candidate`. Interní draft
[majklvi/salamander#23](https://github.com/majklvi/salamander/pull/23) proti
nezměněné release base `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449` zůstává otevřený;
neslouží k merge do samandarin-0.16. Všech 13 kontrol přesného headu SUCCESS:
čistá Win32/x64/ARM64 matice /warnaserror, VSIX build/test, native/Python,
reporty, growth a guardy, CodeQL gate i skutečná analýza. CodeQL job
115823621238 dokončen 2026-10-14 11:32:24 UTC, včetně Initialize, Release
MSBuild, analýzy a SARIF uploadu. Žádný z těchto checks není pending/skipped.
Detailní lokální výsledky a jejich PHP/LoadImageW subcase SKIPs níže zůstávají
v platnosti; zelený CI job je nepřejmenovává na PASS.

Evidence `evidence/S05/publish-final-ci.json`, `publish-final-codeql-job.json`
a `publish-summary.json`. Dokumentační acceptance nemění testovaný runtime.
Další nejmenší etapa S06: File Comparator; read-only baseline audit zatím
odhalil Debug heap hlášení už při otevření/zrušení původního Compare dialogu.
Před migrací izolovat příčinu, nevypínat diagnostiku a neskrývat ji změnou
heap ignore predikátu. SOURCE.json, CONTRACT.md a release piny beze změny.


## S05 — IMPLEMENTED / lokální brána PASS (2026-10-14)

S04 přijat na kódu 359a6b4f po opětovném ověření všech 12 current-head CI
checks včetně skutečného CodeQL. Dokumentační přijetí/accepted reference
`aef114a222f971efefb2029495784bb28b1488aa`; SOURCE a CONTRACT beze změny.
S05 vzniká na lokální `integration/pr769-s05-candidate` nad touto referencí.

Převzat celý spl_diskselection.h, CDiskSelectionService a jeho singleton,
nové samostatné SDK interfaces/constants a built-in QueryService route.
Žádná změna existujících virtual metod nebo CFileData. Owned snapshot kopíruje
cesty i metadata, zachovává pořadí a režimy výběru, vynechává '..', selhává
atomicky a po návratu nepoužívá živé řádky. Capture jen UI thread; immutable
čtení i na workeru; Release v alokujícím modulu. Fallback jen v podporovaném
QueryService-capable Samandarin ABI; starý Open Salamander není podporovaný.
DemoPlug disková DOP ukázka používá snapshot před otevřením progress dialogu;
původní archive example zůstává doslova zachovaný pod diskovým guardem.
Ostatní pluginové operace a Branch state/scanner se nepřebírají.

Donor ABI golden je byte-exact a nezměněný: 5.0-samandarin-0.15.1,
c0cddbe7442fe49abcb859624e271ce687aee0d8. GitHub release listing potvrzuje,
že je stále poslední vydaný tag; golden ověřen přímo proti jeho pěti interfaces
a celému spl_com.h. S04 navíc zachovává celé bodies vydané target base a84dbed6.
Testy SDK/storage/host/singleton a skutečného DemoPlug 55 checks/config PASS
/W4 /WX: 43 zachovaných donor assertions + 12 doplněných DemoPlug kontrol. Pouze PictView
CEnumFiles extrakce a přesně dvě consumer assertions jsou DEFERRED do S12;
žádná jiná donorová assertion nevypuštěna. S04 celý QueryService harness nyní
196/config (původních 193 + 3 nové discovery/version assertions); factory nové
služby je v tomto resolver testu discovery facade, skutečná service/singleton
jsou kompilované v owned harnessu a volané skutečným DemoPlug v GUI.

Host + všech 30 podporovaných SDK konzumentů Debug/Release x64 /warnaserror
PASS (60 consumer builds). Native runner 44 groups/config PASS, pytest 49 PASS.
Historical portables projekt a canonical BUILD_SFTP_PLUGIN=false excluded;
nejsou označené PASS. PHP chybí a LoadImageW long-bitmap backend Win32 2 je
explicitní subcase SKIPPED. Po finálním DemoPlug ABI guardu native runner znovu 44/config PASS
(58,02 / 61,45 s). Hosty po icon opravě přestavěny,
core handle test znovu PASS 132 Debug / 109 Release se stejným bitmap skipem.

GUI objevil baseline CPlugins::CreateIconsList chybu plugins2.cpp:1156:
raw CopyImage, pak HANDLES(DestroyIcon). Funkce v parentu i a84dbed6 shodná.
Nejmenší oprava HANDLES_Q(CopyImage) zachovává tracker i všechny větve.
První vlastní Debug proces 3768 ukončen v diagnostice, není PASS. Finální
Debug fresh 8776/restart 13300, Release fresh 2880/restart 7224: viditelné
panely, fresh F3 obsah/F5 kopie/hash, oba fresh DemoPlug 23 selected DOP files
včetně Unicode a dlouhého názvu, úspěšné deselect a všechny Alt+F4 shutdowny
bez heap/handle dialogu PASS. Kopie 33 bytes, 25 nových a 7 původních fixtures
beze změny. Vlastní bootstrapy obnovené, žádný vlastní GUI proces nezůstal.
Po finálním ABI guardu přestavěn jen DemoPlug D/R a zopakován skutečný
23-file GUI command/shutdown v procesech Debug 10760 / Release 256;
host a ostatní DLL se nezměnily. GUI fixture maximum 250 UTF16 units,
424 UTF8 bytes; skutečná delší UTF16 cesta je v compiled fixtures.
GUI Cancel nebyl doložen: Escape přišel po dokončení a výběr již byl zrušen;
nejde o Cancel PASS. Skutečné SMB I/O ani migrace long-path I/O konzumentů
nejsou S05; DemoPlug stále jen simuluje práci, snapshot není filesystem transakce.

Evidence `C:/Samandarin/codex_pr769_prompts/evidence/S05`: přesné bounded
příkazy, source/donor/ABI manifests, consumer matrix, JUnit, GUI ledger,
runtime payload manifests a originální fixtures. Každý příkaz má parent SHA
a patch hash svého okamžiku; before-menu-fix.patch zachován. Menu fix přidává jen plugins2.cpp; pozdější ABI guard mění pouze DemoPlug
menu a jeho dva testové soubory, zbytek SDK/consumer zdrojových hashů shodný. První host post-build
FAIL kvůli chybějícímu PSModulePath/Get-FileHash vyřešen jen prostředím;
žádná produktová/workflow změna kvůli tomu. Evidence collector opraven pro
extended-length cesty; jeho přípravná selhání nejsou výsledky produktu.

S05 commit je lokální checkpoint; push/PR, nové CodeQL a publikační čistá
Win32/x64/ARM64 matice NOT RUN. Accepted reference zůstává na přijatém S04.
DemoPlug stále deklaruje kompatibilitu se SDK 103: nový diskový case proto
navíc vyžaduje SalamanderVersion >= 105 (QueryService zavedeno v spl_vers.h).
SDK 103/104 použije přesný původní postup bez jediného QueryService volání;
105/106 používá owned službu. Skutečný DOP case se kompiluje v testu proti
progress facade; staré verze, archive, destroy-on-open, Cancel a capture
failure dávají 12 nových assertions. Header zachovává svoji precondition:
volající musí před vstupem zaručit QueryService-capable ABI. Nemění se
verze SDK ani již existující nesouvisející PoC příkazy DemoPlug.

S06 nezačala. Další krok je samostatně autorizovaná publikace této stejné větve,
interní fork-CI draft přes Ensure-ForkCI.ps1 a ověření přesného head výsledku.


## Přijetí S04 — DONE (2026-10-14)

Na pokyn uživatele zahájit S05 přijat S04 checkpoint
`359a6b4f768c4a8769ba6dcb923f9ea663b91aad`. Přesný head interního
[PR #22](https://github.com/majklvi/salamander/pull/22) znovu ověřen:
12 COMPLETED/SUCCESS včetně Win32/x64/ARM64, native/Python, source growth,
guardů a skutečné MSBuild CodeQL analýzy/uploadu. Lokální host + 30 SDK
konzumentů Debug/Release, native 43/config, resolver 193/config, pytest 49
a GUI fresh/restart/F3/F5/shutdown PASS; historické PHP/LoadImageW skips
zachované. Chyba závislosti ABI testu na Git historii vyřešena v 359a6b4f.

Referenční refactor/pr-769-incremental posunut fast-forward přes tento
kód a dokumentační přijetí. Draft se nemerguje do samandarin-0.16;
SOURCE/CONTRACT/release/donor piny beze změny. Další etapa S05, pouze lokální
implementace a validace; nové zveřejnění není tímto zadáním autorizované.
Historické pending záznamy níže popisují dřívější stav.

## Minimální S05 předkrok — menu icon registration

GUI otevření Plugins v Debug S05 zastavilo na handles.cpp:864,
plugins2.cpp:1156, DestroyIcon: Handle not found. CPlugins::CreateIconsList
vytváří sizedIcon raw CopyImage, ale uvolňuje jej HANDLES(DestroyIcon).
Celá funkce je shodná v parentu a84dbed6 i přijatém S04; nový DiskSelection
se při tvorbě menu nevolá. Nejmenší oprava: registrovat návrat CopyImage
existujícím HANDLES_Q, stejně jako ostatní již přijaté icon fixes. Žádné
vypnutí trackeru, změna velikosti, vlastnictví či fallbacku. Po opravě host
Debug/Release, nativní tracker kontrola a skutečné Plugins/DemoPlug/shutdown.
První vlastní GUI proces byl ukončen v diagnostickém dialogu; není GUI PASS.

## Historie kandidátů před přijetím S02b

Aktuální stav 2026-10-13: S02b-preflighta opravuje lokální tracker páry ikon
Service Exploreru; Debug/Release build a GUI view/copy PASS. Plný Debug shutdown
stále BLOCKED kvůli Samandarin Update Notifieru, izolováno A/B kontrolou.
S02b není přijatá. Nejbližší [S02b-preflightb](steps/S02b-preflightb.md): nativní
shutdown a životnost resources CLR-přidržovaného pluginu, bez potlačení kontrol.
Publikovaný integrační kandidát může vyvolat CI; S03 ani upstream PR nezačínají.

Historický stav kandidáta 207a8a57: poslední přijatý checkpoint **S02a** (`ef19d788`).
**S02b PENDING/BLOCKED**: donorové pluginové wrappery a přímé native testy
připravené, host + 30 podporovaných konzumentů Debug/Release x64 a regresní
sady PASS. Debug GUI odhalilo nezměněný Service Explorer icon ownership
mismatch a heap leak při ukončení; runtime brána není PASS; kandidátní commit slouží pouze k autorizovanému CI.
Další nejmenší krok [S02b-preflight](steps/S02b-preflight.md); nejprve vyřešit
nebo doloženě izolovat tato zjištění, teprve pak přijmout S02b. S02 PARTIAL,
S03 nezačala. Publikační větev `integration/pr769-s02b-candidate` uchovává dosavadní
integrační historii a PENDING kandidáta; interní fork-CI draft není upstream PR.
CodeQL a publikační platformní matice čekají na výsledky pro přesné SHA.
Referenční pracovní větev `refactor/pr-769-incremental` zůstává na přijatém S02a.
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
| [S02](steps/S02.md) | Wide utility a Debug sledování handleů; S02a/S02b DONE | S01; pouze potřebné společné pomocné funkce |
| [S03](steps/S03.md) | SafeFile Unicode a dlouhé cesty; DONE včetně baseline preflight | S01, S02 |
| [S04](steps/S04.md) | Cesty položek běžného diskového panelu; DONE | S02; existující QueryService |
| [S05](steps/S05.md) | Owned SDK, host service a DemoPlug společně; local PASS | S04; neměnné vydané ABI |
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

## Podetapy S02 (audit před editací, 2026-10-12)

S02a i S02b dokončeny 2026-10-13. Následující rozdělení zachovává původní audit.
S02 má dvě nezávislé skupiny Debug wrapperů se značně odlišnou množinou
konzumentů. V jednom běhu provést pouze první podetapu:

1. **S02a — core LoadImage/FindFirstChangeNotification A/W.** Převzít páry
   deklarací a implementací z common/handles.h/.cpp. Core CreateFileA/W a
   FindFirstFileA/W i společné widepath helpery už baseline má; nepřidávat
   druhé implementace. Ověřit host a všechny core consumers: salamand,
   salmon, translator a tserver, Debug/Release na jejich podporované lokální
   platformě (x64 host/salmon, Win32-only translator/tserver), nativní test
   skutečného trackeru, A/W+legacy macro aliasů a vlastních Unicode/long fixtures,
   existující native/Python sady a běžné GUI. Chybějící přímý donorový wrapper
   test je doložená testovací mezera, doplnit cílené nativní ověření.
2. **S02b — plugin CreateFile/FindFirstFile A/W.** Až v dalším běhu převzít
   mhandles.h/.cpp včetně best-effort UTF-8 diagnostiky a zachování GetLastError;
   ověřit všechny podporované konzumenty této sdílené pluginové hlavičky i
   chybovou/low-memory diagnostickou větev. Bez pluginových I/O migrací.

Teprve po obou bránách je S02 DONE a lze zahájit S03. Donor, release base,
SDK virtual layout, obecná path knihovna a aktivace Branch View se nemění.

## Předkrok S02b-preflight (zjištěn při GUI, 2026-10-13)

Rozšířená vlastní sestava pluginů odhalila dvě Debug chyby `DestroyIcon: Handle
not found` v Service Exploreru (fs1.cpp:48 a plugin.cpp:226). Tyto soubory se
v S02b nemění: raw LoadImage a tracked DestroyIcon jsou už v parentu. Po Alt+F4
se objeví `Heap Message: Detected memory leaks!` a vlastní Bug Reporter;
původ heap leak dosud neurčen. Nelze jej označit za baseline ani regresi bez
reprodukce/izolace. Rozsah a acceptance [S02b-preflight](steps/S02b-preflight.md).
Do něj nevtahovat I/O migrace, SDK či Branch View; S03 nezačínat.

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

S06-preflight zúžen po alokačním trace: konkrétní neuklizené ComboBoxData a
ThemeData vznikají ve File Comparator darkmodelib. DragDropEditProc hledá
již odpojený rodičovský CCompareFilesDialog a pro pozdní zprávy vrátí NULL
místo předání dalším procedurám. Převést pouze tuto obsluhu dvou comboboxů
na SetWindowSubclass/DefSubclassProc s odpojením při WM_NCDESTROY; odstranit
OldEditProc1/2. Přidat native lifecycle test skutečného callbacku, D/R plugin
+ host a opakovaný skutečný Compare/Cancel/shutdown. Sdílená SDK, theme
knihovna ani heap ignore predikát se nemění. Původních osm host MENU skupin
je starší samostatná baseline záležitost; trace jejich alokace ukazuje také
na app-owned ThemeData, dosavadní komentář o OS-owned původu proto není
potvrzen. Tento předkrok opravuje pouze dvě File Comparator pole.

S27c2 dependency: explicit worker-completion notifications need the donor Branch
AcceptChangeOnPathNotification block in fileswn7.cpp. Import only that block;
it posts WM_USER_REFRESH_DIR with wParam0, so S26 watcher/manual policy stays
unchanged. Existing WorkPath1 recursive root notification and WorkPath2 target
are reused; legacy MAX_PATH notification producer limits remain documented.

S27c4 preflight: existing target S20c IFileOperation queue is already stronger
than donor flags. Retain force-recycle/undo/early failure. Check owner/flags/abort
HRESULTs before queue/perform, reject stale/null/out-of-range source identities,
and use Shell per-item volume validation instead of the synthetic Branch root
drive fallback. Test actual queue block with failure COM facades and actual
recycle helper on owned temporary paths, GUI menu smoke without destructive input.
