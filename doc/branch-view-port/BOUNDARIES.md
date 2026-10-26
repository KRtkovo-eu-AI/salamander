# S00 read-only donor boundary audit

## S31b6c4a — interní whole-archive post-init panel guard (2026-10-26)

Concrete host UnpackWholeArchive přijímá volitelné source/target snapshots a po
InitDLL ověří live identity a source routing před operation context/SDK callback.
Reuses unchanged CFilesActionDiskTarget; veřejné SDK/pluginy beze změny.
[Důkazy a rozsah](steps/S31b6c4a.md): nový native119/config, původní post-init124/
config a disk448/config, operations20/transitions28; host D/R a ordinary7z GUI
D/R Cancel+5exact files/sentinels PASS, LongPathsEnabled=0. Plugin runtime přesně
z ověřeného c3. Stávající runtime callers stále používají NULL defaults: nejde
o hotovou controller lifecycle ochranu ani guarded GUI důkaz. Další S31b6c4b
backend/config binding, potom c4c controller wiring/guarded GUI. Branch admission,
SDK, volume policy beze změny; S31/G04/G05 OPEN, ChangeDir/long-source GUI
NOT_VERIFIED a původní WIP nedotčen. Bez push/PR/S32/clean whole matrix/CodeQL.

## S31b6c3 — úplný whole-archive seznam 7-Zip (2026-10-26)

GatherItems používá skutečnou kapacitu heap bufferu, file/directory append failure
hlásí existující lokalizovanou chybu a OPER_CANCEL místo tichého přeskočení.
Recursive parent se obnoví i při selhání; explicitní OOM a neúspěšný array Add
nepředají částečný seznam Decompress. Původní listing/output assertions zachovány.
[Důkazy a limity](steps/S31b6c3.md): nový native56/config, listing185/config,
output314/config, operations20/transitions28; plugin/deps/host D/R PASS.
Actual ordinary7z AltF9 D/R: Cancel a všech5files s dlouhými Unicode internal
paths do root264/266UTF16, exact hashes/sentinels PASS, LongPathsEnabled=0.
Initial Release build timeout95s zachován, samostatný final build PASS.
Další S31b6c4: source/target/backend lifecycle před admission; celá S31/G04/G05
OPEN. Branch guards/SDK/volume policy beze změny, ChangeDir/long-source GUI
NOT_VERIFIED a původní WIP nedotčen. Bez push/PR/S32/clean whole matrix/CodeQL.

## S31b6c2 — exact source-parent context (2026-10-26)

Interní source helper zachytí owned archiv a parent ze stejné exact wide identity,
s nezměněným donor SalPathFindFileNameW a zachovaným kořenovým separator. Controller
už nepoužívá panelový GetPathW pro relativní target base. Původní assertions a
controller od CPathBuffer dále beze změny; žádná Branch/SDK/backend/delete admission.
[Implementace a důkazy](steps/S31b6c2.md): source native96/config, Unicode dialog38/
config, operations20/transitions28; host x64 Debug32.79s/Release55.54s PASS.
Actual ordinary AltF9 GUI D/R: default/alternate/subdir a Cancel, relativní Unicode
výstup root260/262UTF16 (268/270UTF8), čtyři přesné file hashes + sentinels PASS.
LongPathsEnabled=0; ZIP runtime nezměněn z ověřeného buildu. Další S31b6c3: oprava
whole-archive7Zip collection podle c1. Source/target lifecycle a další admission
zůstávají otevřené; S31/G04/G05 OPEN. ChangeDir/long-source GUI NOT_VERIFIED,
původní WIP nedotčen. Bez push/PR/S32/nové clean matrix/CodeQL.

## S31b6c1 — audit přímého Unpack (2026-10-26)

Uzavřený audit, produkce/testy beze změny. Source helper vlastní úplný text cesty,
ale controller stále používá panelový root pro relativní cíl a nemá doloženou
source/target lifecycle garanci přes modal/InitDLL. Whole-archive dispatcher
používá konfigurovaný SupportCustomUnpack, nikoli ověřeného listing ownera F5.
7-Zip GatherItems má SAL_MAX_PATH storage, ale dvě append hranice MAX_PATH;
selhání append položku/podstrom přeskočí bez chyby. Stejné v donoru. Delete-volume
loop má stále byte ClearReadOnlyAttr/DeleteFile a vyžaduje samostatné ověření.
[Symboly, důkazy a následné akceptace](steps/S31b6c1.md): actual source helper
native54/config D/R, operations20/transitions28 PASS. Žádný nový host/plugin
build ani engine/GUI/CodeQL/clean matrix běh; Branch source i target Unpack
zůstávají blokované. Další S31b6c2: exact source-parent context bez admission.
S31/G04/G05 OPEN; ChangeDir/long-source GUI NOT_VERIFIED a původní WIP nedotčen.
Bez push/PR/S32, bezpečnostní ani systémová nastavení se nemění.

## S31b6b3c3 — ověřený 7-Zip cíl Branch View (2026-10-26)

Po c2 rozšířena existující owner policy pouze o bundled7zip\7zip.spl. Přesný
private metadata/archive/owner/target binding zůstává; před InitDLL zachycen
backend kind a po něm odmítnuta i záměna ZIP↔7-Zip na témže objektu. Původní
InitDLL pořadí a všechny původní post-init assertions zachovány, bez SDK změny.
[Implementace a důkazy](steps/S31b6b3c3.md): post-init124/config, ZIP dispatcher/
listing195D/182R,7-Zip listing185/config, extraction314/config, directory240/config,
disk448/config; operations20/transitions28; host x64 D/R PASS. Actual7z F5 do
Branch rootu266/268 UTF16 v Debug Dark Mode a Release light: Cancel, odmítnutí
ruční změny cíle, missing-root1237 a úspěch; všech5 hashes a sentinels souhlasí.
LongPathsEnabled=0; plugin/deps přesně z ověřeného nezměněného c2 runtime.
Další nejmenší krok dle S31b6a: S31b6c audit source admission přímého Unpack.
Source lifetime zůstává samostatná otevřená hranice. Celá S31/G04/G05 OPEN;
ChangeDir/long-source GUI odložené NOT_VERIFIED. Bez push/PR/S32/nové clean CI/CodeQL.

## S31b6b3c2 — opravený 7-Zip output lifecycle (2026-10-26)

Jedna plná UTF8-first wide identita pro stream, atributy a cleanup; kapacity a
relative-name guard před první kopií. Full byte path pro overwrite/UI zůstává
stabilní. Interní wide delete sdílí původní retry policy s char wrapperem;
current-output ownership brání úklidu skipped/dokončené položky a pokrývá chybu
parent check před PrepareOperation. SDK, vendor code pages a Branch policy beze změny.
[Implementace a důkazy](steps/S31b6b3c2.md): native314/config, listing185/config,
ZIP listing174D/161R, post-init96/config, host/plugin/deps D/R a actual7z F5 GUI
D/R do Unicode rootů266/268 UTF16; všech5 archivních SHA256 + sentinel souhlasí.
Cancel/SkipAll/overwrite Cancel beze změny dat. LongPathsEnabled=0.
Další S31b6b3c3: teprve kvalifikovat7-Zip Branch admission; S31/G04/G05 OPEN,
ChangeDir/long-source GUI odložené, bez push/PR/S32/nové clean matrix/CodeQL.

## S31b6b3c1 — prokázaná 7-Zip output/cleanup návaznost (2026-10-26)

Nová diagnostika s reálnými vendored AString/UString/COutFileStream a actual
SafeDeleteFile (pouze error-dialog facade) D/R:99 kontrol/config na ACP1250.
Ze šesti tvarů cest ASCII funguje; u pěti Unicode legacy stream vytvoří dva
chybně pojmenované výstupy a třikrát selže otevření. Částečná oprava jen wide
identity zápisu nechá u všech pěti nefunkční ACP cleanup. Sentinel beze změny.
[Úplná hranice a akceptace](steps/S31b6b3c1.md): další S31b6b3c2 musí opravit
celý output lifecycle (bounds/names, stream, attributes, cleanup) společně,
potom S31b6b3c3 může kvalifikovat Branch admission. Produkce/testy/SDK beze změny;
nejde o PASS správného rozbalování. S31/G04/G05 OPEN, ChangeDir odložené,
bez push/PR/S32. ZIP-only dokončený checkpoint S31b6b3b zůstává zachovaný.

## S31b6b3b — ověřené ZIP F5 do kořene Branch View (2026-10-26)

Uzavřený ZIP-only checkpoint: skutečný archive listing owner + private metadata
+ archiv musí kvalifikovat bundled ZIP, nezávisle na asociaci. Controller před
modalem i po něm, dispatcher a post-InitDLL kontrola chrání přesný diskový Branch
root; ruční jiný/virtuální cíl se odmítne před ParsePath. Unknown/7-Zip/external,
Branch source, archive delete a virtuální target se tímto nezpřístupňují.
[Implementace a ověření](steps/S31b6b3b.md): native D/R, host D/R, actual ZIP GUI
D/R s vybranými podadresáři, Unicode a kořeny265/267 UTF16; exact readback,
Cancel, manual rejection i missing-root ERROR_RETRY PASS. LongPathsEnabled=0.
S31b6b3/G04/G05/S31 OPEN; další S31b6b3c samostatný7-Zip output adapter.
ChangeDir/long-source GUI odložené, žádný push/PR/S32 ani nový CodeQL/clean matrix.

## S31b6b3a — kvalifikace archivních backendů (2026-10-26)

Audit rozlišil association-first PackGetPluginForArchiveFile od skutečného
listing-owner dispatch v PackUncompress. Admission musí hlídat owner+metadata,
nikoli jen příponu. ZIP je první kandidát, jeho Branch F5 dosud neověřeno.
7-Zip GetStream používá Windows default-ACP převod UTF8 output cesty: actual
conversion probe D/R na ACP1250 doložil2 neshody ze3 cest, bez I/O; existující
UTF8 helper je zachoval. Tento stream body je shodný s donorem, ne nová regrese.
[Audit a návaznost](steps/S31b6b3a.md): další S31b6b3b ZIP-only policy/admission,
potom samostatná 7-Zip output oprava/validace. Produkce/guardy beze změny;
S31b6b3/G04/G05/S31 OPEN, bez push/PR/S32, ChangeDir odložené.

## S31b6b2b2 — zapojený diskový cíl archivního F5 (2026-10-26)

Stejný interní snapshot má adresářový režim bez masky. UnpackZIPArchive zachytí
živý diskový target před callbacky, validuje modal/parser/create/backend hranice
a předá jej do post-InitDLL gate. Zmizelý implicitní root nesmí být znovu vytvořen.
Původních448 mask assertions zachováno, directory240/config, post-init68/config,
listing151D/138R a další native/build x64 D/R PASS. Actual GUI D/R default/manual/
inactive/detached cíle uspěly; missing-root/switch/closed =>1237 s nedotčenými daty.
[Podrobnosti a meze](steps/S31b6b2b2.md). S31b6b2 diskový lifecycle předkrok uzavřen;
S31b6b3 Branch target admission, G04/G05 a celá S31 OPEN. Branch guardy, SDK,
archive delete a odložené ChangeDir zachovány. Bez push/PR/CodeQL/S32.

## S31b6b2b1 — kontrola cíle po inicializaci pluginu (2026-10-26)

Interní PackUncompress předává optional disk target do CPluginData::UnpackArchive;
nový gate po InitDLL odmítne neaktuální cíl před operation contextem a plugin I/O.
Produkční callers zatím používají NULL: zapojení controlleru je další S31b6b2b2.
Native actual wrapper68/config, listing151D/138R, identity448 a destination64/config,
host x64 D/R i ordinary ZIP F5 GUI s exact readbackem PASS. Safe early-check mutant
má očekávaných30 selhání. [Rozsah a evidence](steps/S31b6b2b1.md).
SDK, Branch guardy a odložené ChangeDir beze změny; celé S31b6b2/G04/G05/S31 OPEN.
Bez push/PR/CodeQL; external unpacker post-init ochrana touto etapou nedoložena.

## S31b6b2a — sdílený helper cílové identity (2026-10-26)

CFilesActionDiskTarget přesunut beze změny do interní fileactiondisktarget.h
pro fileswn7/8; controller bodies i všech448 assertions shodné. Native448+64
D/R, host D/R a nové ordinary Copy/Move/ZIP F5 GUI s exact readbackem PASS.
[S31b6b2a](steps/S31b6b2a.md) je pouze ověřený sdílecí předkrok; celá S31b6b2
zůstává otevřená. Další S31b6b2b: directory default a modal/post-InitDLL target
validace. Branch guardy a odložené ChangeDir beze změny; bez push/S32.

## S31b6b1 — úplný diskový cíl archivní extrakce (2026-10-26)

UnpackZIPArchive má owned destination/prefix/create storage, skutečné kapacity,
lossless default a explicitní copy, wide create/cleanup. Unicode component
kontroly rozlišují UTF8 bajty od ASCII controls; původní ASCII zákazy zachované.
Nový native64/config a šest existujících native fixtures D/R PASS, host a ZIP
x64 Debug/Release build PASS. F5 GUI D/R existující ASCII/Unicode/byte-long/
long cíle, Cancel a finální new long Unicode create/file-target refusal PASS
s přesným readbackem a sentinels. Vazby starších GUI a finálních binárek jsou
výslovně oddělené v [S31b6b1](steps/S31b6b1.md). Branch guardy zůstávají;
S31b6b2 doplní target lifecycle. ChangeDir odložené, G04/G05 OPEN, bez push/S32.

## S31b6a — audit archivního cílového resolveru (2026-10-26)

F5 z otevřeného archivu a Alt+F9 diskového archivu jsou odlišné pipeline.
Donor UnpackZIPArchive má stejné krátké destination mezivrstvy; odstranění
Branch guardu nestačí. Audit doložil pět konkrétních mezer/vazeb a registrace
existujících testů, bez nového native/build/GUI běhu. S31b4c1 ZIP long-target
oprava zůstává platná; long-source GUI/ChangeDir nadále odložené.
Další S31b6b1 opraví diskovou destination větev UnpackZIPArchive při zachování
Branch guardů; potom lifecycle a selektivní admission. Podrobnosti a akceptace
v [S31b6a](steps/S31b6a.md). G04/G05 OPEN, S31 nedokončená, bez push/PR/S32.

## S31b5c — uzavírací audit resolveru (2026-10-26)

D01 uzavřen pro ověřené diskové Copy/Move do jediného Branch cíle; D02/D03
implementačně uzavřené s native42/config a výslovně omezenou GUI validací.
Audit aktuálních funkcí versus testované commity a JSON/log bindings PASS:
parser42, split190, atomic dialog52, getters399 a identity448/config D/R;
host d2 D/R a GUI e1/e2/e3 znovu nespouštěny, jejich přesná source/hash vazba
ověřena. Nový příkaz Audit.py + controller placement PASS; žádný nový build.
Drop root-only helper a SDK hlavičky v rozsahu S31b5 beze změny. Klasický
resolver zachovává vlastní úplný root/tab/generation a explicitní ruční cíl.
ChangeDir a dlouhý source GUI/Unpack DEFERRED/NOT_VERIFIED, původní WIP nedotčený.
Zbývající archive/FS guardy nejsou touto diskovou aktivací odstraněny. Viz
[S31b5c](steps/S31b5c.md). Další nejmenší krok S31b6a: audit UnpackZIPArchive
a samostatné archivní destination pipeline, potom ověřené donorové dílčí opravy.
G04/G05 OPEN, celá S31 neuzavřena, S32 BLOCKED; lokální commit bez push/PR/CI.

## S31b5b3e3 — zavřený a odpojený Branch cíl ověřen (2026-10-26)

Debug/Release GUI PASS: odpojený zdroj, nový připojený Branch cíl zvolený
chooserem zavřen během Copy dialogu. Potvrzení skončí chybou1237 bez změny dat,
i když druhá záložka stále ukazuje stejný root. Nový odpojený Branch cíl zvolený
přes Detached funguje pro Copy i Move; přesné názvy/obsah proti zálohám/SHA256,
Copy zdroj zachován, Move zdroj absent, sentinel nedotčený. Native test doplněn
o8 lifecycle assertions:448/config D/R PASS, všech původních440 zachováno.
Safe extra-getter mutant vyvolá4 semantic failures/448 bez neplatné paměti.
Produkce beze změny, d779e0f4 D/R runtime ověřený hashem, nový host build netvrzen.
GUI blokace po dostupné ploše odstraněna; oba privátní runtime ini obnoveny.
LongPathsEnabled0, ChangeDir odložený/NOT_VERIFIED, jeho WIP nedotčený.
Viz [S31b5b3e3](steps/S31b5b3e3.md). Navazuje uzavírací audit S31b5 a přesné
zbylé G04/G05 hranice; celá S31 neuzavřena, S32 BLOCKED, bez push/PR/CI.

## S31b5b3e2 — odpojený zdroj, neaktivní Branch cíl a modal switch (2026-10-26)

GUI Debug Windows Dark Mode scheme a Release light PASS: z odpojené zdrojové
záložky F5/F6, explicitní chooser neaktivního dlouhého diskového Branch cíle.
Copy zachová zdroj, Move jej přesune; přesné výsledky/zálohy/SHA256 a sentinel
PASS. Při Move vybrán viditelný Branch cíl, v hlavním okně přepnuto na jinou
záložku během otevřeného dialogu: po potvrzení chyba1237, žádná změna dat.
Nový výběr původní neaktivní Branch záložky pak uspěje. Produkce/testy beze
změn, reuse d779e0f4 src/binárky ověřený hashem; nový build/native běh netvrzen.
LongPathsEnabled0, privátní profily, oba runtime ini obnoveny byte-exact.
Viz [S31b5b3e2](steps/S31b5b3e2.md). Zbývá zavřený/odpojený cíl a audit;
G04/G05 OPEN, goal ACTIVE, S32 BLOCKED. ChangeDir DEFERRED/NOT_VERIFIED,
odložený patch nedotčený. Lokální checkpoint bez push/PR/CI.

## S31b5b3e1 — GUI Copy/Move se zachováním podadresářů (2026-10-26)

Ověření existujícího keepBranchPaths TRUE bez změny produkce nebo testů.
Skutečné GUI Debug Windows Dark Mode scheme a Release light: dva různé same.txt
z one-東京 a two-🙂, zdroj i dlouhý diskový cíl v Branch View, F5/F6 s Keep
subfolder paths. Obě operace/config PASS; přesné relativní názvy a obsah vůči
zálohám/SHA256, Copy ponechá zdroje, Move odstraní pouze původní souborové cesty.
Všech8 root/bystander sentinelů nezměněno. Debug keep-paths Cancel bez výstupu.
Použity hashově nezměněné binárky a src tree d779e0f4 ze S31b5b3d2; nový build
ani native běh nebyl potřeba a není tvrzen. LongPathsEnabled0, privátní profily,
runtime ini obnoveny přesně, odložený ChangeDir patch nedotčený/NOT_VERIFIED.
Viz [S31b5b3e1](steps/S31b5b3e1.md). Další samostatná podetapa: detached/modal
destination identity. G04/G05 OPEN, goal ACTIVE, S32 BLOCKED; bez push/PR/CI.

## S31b5b3d2 — diskový Move do Branch cíle ověřen (2026-10-26)

Private vstupní politika AllowsBranchTransfer dovolí atCopy/atMove pro diskový
zdroj a jediný diskový Branch cíl, countSizeMode0 a bez selected-dirs. Ostatní
operace zůstávají odmítnuté. Resolver/snapshot/preflight/worker i bezpečnostní
kontroly se nemění. Native policy/snapshot440 a Branch getters399/config D/R
PASS, transitions28/operations All20 PASS. Host x64 D/R /warnaserror PASS.
GUI D dark/R light při LongPathsEnabled0: Cancel beze změn, přesun2 souborů do
Branch rootu272/274 UTF16, ruční Unicode cíl mimo root, nested Unicode Branch
source Move do rootu a další Cancel. V každé konfiguraci4 přesunuté zdroje chybí,
výstupy přesně odpovídají zálohám/hashes; cancel.txt a alias sentinels nedotčené.
Safe policy mutant rozšířený o Delete440/1 semantic failure. Viz
[S31b5b3d2](steps/S31b5b3d2.md). Zbývá integrační destination matice keep-paths,
detached/modal target a uzavírací audit; G04/G05 OPEN, goal ACTIVE, S32 BLOCKED.
ChangeDir nadále odložený/NOT_VERIFIED. Lokální checkpoint bez push/PR/CI.

## S31b5b3d1 — diskové Copy do Branch cíle ověřeno (2026-10-26)

FilesAction nyní dovolí atCopy do jediného diskového Branch cíle při diskovém
zdroji a countSizeMode0. Move/selected-dirs i ostatní typy zůstávají odmítnuté.
Owned snapshot, atomický dialog, implicit root/mask preflight a modal/build/start
rechecks se nemění. Explicitní ruční cíl zůstává podporovaný.
Native snapshot/policy440 a Branch getters399/config D/R PASS; transitions28 a
operations All20 PASS. Host x64 D/R /warnaserror PASS. GUI D dark/R light při
LongPathsEnabled0: dvě ordinary-source kopie do Branch rootu272/274 UTF16,
kopie z nested Unicode Branch řádku, explicitní Unicode cíl mimo root, Cancel
bez dalších souborů a F6 bez Move dialogu. Všechny zdroje/alias sentinels SHA256
nezměněny, přesné výsledné názvy/obsah ověřeny. Safe policy mutant440/1 failure.
Viz [S31b5b3d1](steps/S31b5b3d1.md). Navazuje vlastní ověření a aktivace Move;
GUI reentrant target switch/closed-tab injection není prohlášena za hotovou.
ChangeDir odložený/NOT_VERIFIED, G04/G05 OPEN, goal ACTIVE, S32 BLOCKED.
Lokální checkpoint bez push/PR/remote CI/CodeQL.

## S31b5b3c2 — preflight implicitního rootu/mask (2026-10-26)

FilesAction rozliší přesně zachovanou výchozí hodnotu dialogu ještě před její
normalizací. Pro tento implicitní diskový cíl ověří po ParsePath úplný captured
root, Windows directory interpretaci a secondPart ukazující jen na*.* před
SalSplitWindowsPath/create. Po splitu ověří caller-owned masku, obě NUL a kořen;
navíc znovu ověří identitu panelu. Ručně zadaná jiná hodnota používá původní
resolver. Branch destination entry guard nadále zachován, aktivace není hotová.
Native snapshot248/config D/R (původních50 zachováno), parser42 a split/create190
PASS; transitions28/operations All20 PASS. Safe semantic mutants248/24 a248/12
failures. Host x64 D/R /warnaserror PASS. GUI D dark/R light: předvyplněný cíl,
explicitní child\東京-🙂.txt a Cancel, přesné readback/hashes PASS.
[S31b5b3c2](steps/S31b5b3c2.md) vymezuje scope i zbývající runtime Branch matice.
ChangeDir odložený/NOT_VERIFIED, deferred patch nedotčený. G04/G05 OPEN, goal
ACTIVE, S32 BLOCKED. Lokální checkpoint bez push/PR/CI.

## S31b5b3c1 — atomický cíl dialogu Copy/Move (2026-10-26)

Opraveno ignorované selhání načtení cíle v CCopyMoveMoreDialog::Transfer.
Úplná wide cesta se ověří před změnou caller bufferu, historie i voleb; chyba
zastaví potvrzení stávající lokalizovanou hláškou. Unicode ovládací prvky a wide
historie zachovají UTF8-first/ACP fallback. Sdílený Unpack reader jen přesunut,
jeho tělo i Unpack Transfer beze změny. SDK ABI a Branch target guard zachovány.
Native actual Copy/Move52 a Unpack38 kontrol/config D/R PASS; snapshot50,
transitions28 a operations All20 PASS. Bezpečná in-memory mutace ignorovaného
výsledku vyvolá14 semantic failures/52 checks, bez souborového I/O či overflow.
Host x64 Debug/Release /warnaserror PASS. GUI ordinary Copy do child\東京-🙂.txt
+ Cancel D dark/R light PASS, přesný obsah a nezměněné zdroje/sentinel.
Podrobnosti: [S31b5b3c1](steps/S31b5b3c1.md).
ChangeDir nadále DEFERRED/NOT_VERIFIED a jeho oddělený patch beze změny.
Root/mask preflight a ověřená aktivace Branch cíle zbývají; G04/G05 OPEN,
goal ACTIVE, S32 BLOCKED. Pouze lokální checkpoint, bez push/PR/CI.

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

## S28 dokončena lokálně (2026-10-25)

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


## S17 size boundary complete; remaining copy regression belongs to S20

Only SalGetFileSize2 production change, exact donor file; S04 getters/services
unchanged. Ordinary donor getter assertions run, all future Branch metadata
assertions preserved behind S26 guard and explicitly not run. Disk-size native
79/config includes actual symlink and long paths, ordinary membership 196/config.
Actual D/R GUI copies short ASCII link to long Unicode target as identical
2107-byte regular files. A long Unicode SOURCE link copy fails later Error
Opening File; retained in evidence/S17, mandatory concrete retest in S20.
Do not present size helper support as complete file-operation migration.

## S16 Undelete legacy image boundary

Eight production files and C++ fixture exact donor; Python only configuration
selector. Wide restore is separate from unchanged ACP/fixed-capacity image
engine. Connect preserves full exact input and fails explicitly before backend
on unsupported paths. Actual GUI verified full edit value and capacity refusal,
no physical volume analysis. Native restore performs temporary disk I/O; EFS
is a stub, real encrypted Windows import/export not exercised. Conditional
ACP1250/1252 assertion remains unchanged; registry ACP is recorded separately.

## S15 PAK engine and hidden-command boundary

Five production files and C++ fixture exact donor. OP_EXISTING_ONLY changes
only an internal mode bit, signatures/layout/vtables unchanged. Engine is
compiled into pak.spl by current project; old paklib.dll loader is commented.
The donor/target Connect deliberately hides Optimize; ordinary menu case SKIP.
Actual D/R GUI command/engine exercised with a temporary AddMenuItem adapter,
source/binary hashes recorded separately, not equivalent to an unmodified
menu runtime. Adapter removed byte-exact before final D/R production rebuild
and commit. No public menu restoration in S15. Native production OpenPak
missing-file/noncreation evidence separate from mocked menu engine harness.

## S14 complete archive command boundary

Three exact 7-Zip donor production files and exact shared C++ menu harness.
All S13 deferred 7-Zip assertions restored; driver only adapts helper import.
Actual D/R GUI tests prove focused Unicode archive while another is selected.
Command harness engine doubles are distinct from this real archive testing;
host-managed packing remains S22. Staged engine DLL hashes are not presented
as observed loaded modules after the transient engine has already unloaded.
The S13 ZIP GetMenuItemState long-basename limitation remains unresolved.

## S27f.08a resolves the S13 ZIP long-basename menu limitation

Exact NameW association matching and real final D/R Branch GUI pass for100-CJK
basenames. The earlier S13 SKIP below is historical. ZIP command/engine unchanged;
actual message hard-wrap byte splitting was also fixed and verified with wide GDI.
Comment/CreateSFX remain private-gated for .08b; full S27 is not complete.

## S13 ZIP command boundary and menu limitation

ZIP ExecuteMenuItem migration complete, three exact donor production files.
PackToArchive/engine remain S22. 7-Zip assertions preserved but deferred in
archive_menu_selection_tests ZIP scope until S14; no claim of their execution.
Real Debug selected archives and Release focused archive passed Test Archive
on long Unicode paths; isolated engine harness is separate evidence.
Unchanged donor/target GetMenuItemState uses CFileData.Name for archive masks:
100-CJK basename disables menu even with correct ZIP association. This GUI
case is SKIP; fix before claiming full menu coverage for such basenames.

## S12 completed boundary

Owned thumbnail CEnumFiles and paired EXIF v10 are integrated together.
Legacy EXIF export retained. Five exact donor files verified by Git blobs;
project includes only disk selection SDK, Unicode title/navigation remain S18.
S05 deferred CEnumFiles and both assertions now restored. Real EXIF native
51 checks/config and six actual GUI outputs pass against current binaries.
The separate baseline WIC repair is parent 270f2cb6; initial S12 evidence is
preserved and does not count as current-source full-run/CI validation.

## S12-preflight: WIC output boundary

Evidence confirms a target/donor baseline defect, separate from owned selection
and paired wide EXIF migration. SaveFrame is reused, not replaced by another
image engine. The adapter buffers the whole encoded image; cancellation does
not interrupt WIC encoding. Native adapter tests use a stub encoder dependency;
17 real Release DLL checks and Debug/Release GUI provide separate integration
evidence. S18 navigation and Unicode title remain deferred. S12 parked patch
must be restored after this commit and all its current-source gates rerun.

Pinned donor: `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`.
Diff base and target: `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449`.
Scope: changed-file list, selected critical function hunks and S01-S32 prompts. No implementation, donor build, fixture execution, mutation of donor or target, or assertion that a test passed. REUSE.tsv maps all 231 distinct donor paths; every row remains DEFERRED. Shared files have multiple stage rows; these are planning boundaries, not an assertion that all hunks already received independent parity review. S31 must resolve every actual hunk, including BOM-only differences and project registrations.

## S02 audit correction, 2026-10-13

The opening scope/status describes the historical S00 audit. Current progress
is in PROGRESS/REUSE/VALIDATION; not all rows are still DEFERRED. A narrow actual
core diff disproved the original generic CreateFile/CreateEvent hunk labels:
core A/W CreateFile and FindFirstFile are already in the target. S02a reuses
only the exact donor LoadImage and FindFirstChangeNotification A/W additions;
all four core projects and direct live-tracker/native/GUI coverage passed.
Plugin mhandles changes remain S02b DEFERRED. This adds no scanner dependency.

## Existing baseline facilities (reuse)

- `src/common/widepath.h/.cpp` already define `CPathBuffer`, `CWidePathBuffer`, `SalIsExtendedLengthPathW`, `SalPathAddExtendedPrefixW`, `IsValidPathUtf8Text`, `SalMultiByteToWidePath` and wide-to-byte conversion helpers. They are not changed donor files. S02 adds A/W handle wrapper coverage, not another conversion library.
- `src/salamdr3.cpp` already implements `SalCreateDirectoryExW`; `src/salamdr5.cpp` already supplies `SalGetFileSize`. S03's extracted SafeFile harness reads these existing implementations.
- `CSalamanderGeneralAbstract::QueryService` is already in baseline `src/plugins/shared/spl_gen.h:3658`, alongside existing Register/Unregister and appended panel/owned-service lifecycle methods. Concrete QueryService is `src/zip.cpp`. Do not add another virtual transport or alter existing slot order.
- Donor adds optional `Salamander.PanelItemPaths`, `Salamander.DiskSelection`, and `Salamander.ViewerEnumeration` classes/service IDs in spl_gen.h. CFileData remains untouched. `spl_diskselection.h` explicitly supports QueryService-capable Samandarin ABI; fallback must never call an absent historical virtual slot.
- Baseline native projects under src/tests: clipboard_contract_tests, conversion_table_tests, filenamecase_tests, native_decoder_tests, panel_icon_size_tests, storage_scheduling_tests, viewer_unicode_tests. Keep these; inspect other plugin test projects through runner discovery. They are not replaced by donor contracts.

## Concrete stage dependencies and smallest adaptations

1. S01 has no new SDK/scanner/wide prerequisite. SafeFileRead and SafeFileWrite change silent `dlgRet |= DIALOG_SKIP` to assignment; both Retry reopen paths change only the disposition to OPEN_EXISTING while retaining the target's existing CreateFile boundary, desired access, sharing and flags. SafeFileCreate's failed owned filename duplication returns INVALID_HANDLE_VALUE and ERROR_NOT_ENOUGH_MEMORY after closing its handle. Do not import the wide-path helpers/catches merely because donor placed these changes nearby.
2. The full donor SafeFile harness cannot be copied unchanged for S01: cpp.in main unconditionally tests `SafeFileRootLength`, added only in S03, and runs Unicode/long-path scenarios whose implementation is deferred. Reuse `RoundTrip`, `InjectRead`, `InjectWrite`, `FailDuplicate`, `Fixtures` and the generator that compiles actual production SafeFile methods. Keep ASCII retry/offset/content/error tests in S01; explicitly stage the wide/root tests in S03. Add required Write Skip, Read/Write Skip All assertions without weakening existing donor coverage. Donor directly covers silent READ Skip, not all these promised cases.
3. S03 already has wide conversion/directory primitives in target; no Branch scanner dependency. Preserve S01 and the original UTF-8-first/ACP fallback byte-path contract. Core CreateFileA/W already exist in the baseline; S02a adds only LoadImageA/W and FindFirstChangeNotificationA/W. S02b supplies missing plugin CreateFileA/W and FindFirstFileA/W plus wide diagnostic text. S03 must reuse the existing core CreateFileW wrapper instead of duplicating it.
4. S04 ordinary getters are donor `CFilesWindow::GetItemDirectoryW`, `GetItemFullPathW`, `GetItemIdentityW`, `GetItemRelativePathW` in branch_view.cpp:269-309. `GetItemFullPathW` calls `Salamander::BranchView::Join`; the ordinary branch needs NameW/UseWideName plus existing widepath conversion. Extract only these ordinary parts into a compiled host translation unit and a single shared join implementation before scanner stage. The exact target path must be recorded when choosing that minimal extraction; REUSE.tsv retains donor destination as provisional while DEFERRED. Do not add CBranchViewState or stub state to satisfy these symbols. Getters' declarations belong to fileswnd.h.
5. `CPanelItemPathsService::FindCurrentItem/GetItemFullPath` is donor zip.cpp:720; address membership is proven before dereference, capacity and UI thread errors are explicit. `CDiskSelectionService::Capture` is zip.cpp:801 and uses all three getters. S05 therefore requires completed S04. CaptureFallback/OwnedSnapshot in spl_diskselection.h must preserve allocation-module Release and failure atomicity.
6. S17 partly overlaps S04: ordinary panel getters should already be done. Transfer remaining `SalGetFileSize2` wide size boundary in salamdr5.cpp, run getter/disk-size tests, defer real Branch metadata assertions to S26. There are no separate S17 getter modifications in zip.cpp in this pinned diff. Do not invent another host API.
7. S18 viewer enumeration must include `CFileNamesEnumData::LastFileName/FileName` std::wstring and W declarations in consts.h, concrete `CViewerEnumerationService` in salamdr6.cpp, QueryService route in zip.cpp, SDK service declaration, and PictView navigation consumers (`pictview.cpp/.h`, render1.cpp, unicodetitle.h). PictView thumbnail/EXIF is S12; previous/next/navigation is S18. fileactionpath.h includes viewerpath.h; production launch/argument expansion and actual child argv/current-directory tests belong together. S18 may be split launch then viewer service, one accepted commit at a time.
8. S19 workerpath.h is isolated and does not include branch_view.h. Its WorkerOperationPathW/WorkerOpenFile/etc need no scanner import. S20 should split rename+selection, copy/move, and delete; S21 should split native helper+tests and host call sites if size warrants it.
9. S23 Reserve must precede scanner/panel capacity consumers. S24 generic formatting is isolated; resource-dependent text assertions wait for matching S26 IDs. S25 donor branch_view.cpp already fences host code with BRANCH_VIEW_TEST for native scanner tests; production must compile normal code, never hide complete feature behind a compile gate.
10. S26 cannot defer exact cache keys, pointer lifetime, icon-reader shutdown order, generation checks or reentrant safety to S28/S30. Runtime gate defaults off and must cover mainwnd3 dispatch, keyboard activation, history/tab restore, and other real entry points. Experimental commands require explicit operation support. S27 splits a-f, with each plugin independently enabled only after actual end-to-end validation.
11. S29 donor salamand.h hunks concern CPathHistoryItem/CPathHistory BranchState only; viewer declarations are in consts.h. Keep every restoration behind the gate. S30 handles only refresh/publication work not already needed for correctness. S31 audits every original hunk, providers, paired runtime files, SDK and resources; S32 only activates after closure.
12. SDK ABI fixture `disk_selection_snapshot_abi.json` names release `5.0-samandarin-0.15.1` and commit `c0cddbe7442fe49abcb859624e271ce687aee0d8`. It is reusable evidence but not automatically sufficient for the newest release-prefix rule. S05 must also verify actual released target tag/API prefix; never regenerate a golden from the edited header to make tests pass.

## Existing fixture coverage and limits

- SafeFile: `src/tests/safe_file_paths_tests.cpp.in`, `Fixtures::{Owned,Directory,File}`, `RoundTrip`, `InjectRead`, `InjectWrite`, `main`. Own process/tick unique temp root; Own checks before deletion; reverse cleanup. Cases include ASCII, Czech/CJK/emoji name, multiple Unicode parents, 90 CJK-character basename, deep UTF-16 path. RoundTrip writes `first-` then forced-failure/retry `second`, checks 12-byte content and seek offset, silent read skip, overwrite cancel and unchanged complete file; main injects DupStr allocation failure and verifies exclusive reopen. UNC root construction is a string assertion, not an SMB test. Read-only/DOS-alias/Write Skip/SkipAll scenarios are not established by this fixture and remain required acceptance gaps.
- Renamer: `src/tests/renamer_io_tests.cpp.in`, `Fixture`, `FileCase`, `ConversionTests`, `RunFilesystem`. `RunFilesystem` explicitly checks `Utf8(utf8Long).size()>260 && utf8Long.size()<260` for an 85-Han-character parent; separately `deep.size()>260` for true Win32-long UTF-16. ASCII and Czech/CJK parents, duplicate same.png, Unicode names, actual Move/Undo and collision/normalization content preservation are available. The duplicate FileCase payload is basename-derived, so it alone does not establish differing same-name content hashes.
- Checksum: `src/tests/checksum_selection_paths_tests.cpp.in`, `Fixtures::{Owned,Directory,File}`, `Populate`, `RunPending`, `main`. same.bin under ASCII, Czech/CJK and long parents has sizes 1/2/3 (content x/xx/xxx), plus 90-CJK basename of size 4. Actual worker reads and different duplicate hashes are asserted after panel storage is cleared; Verify uses manifest's actual parent. CHashAlgo::GetDigest in this fixture is a one-byte sum STUB (lines 122-128), not CRC/SHA; this proves source bytes/identity but not real checksum algorithms. Add independent real hashes and actual plugin Calculate/Verify at S08/S27f. This is production command/worker extraction, not launched plugin smoke.
- Split/Combine: `src/plugins/splitcbn/tests/selection_paths_tests.cpp.in` has actual temp files for ASCII, Czech name/parents, Czech+CJK parents, UTF8-byte-long and deep UTF16 paths; verifies sibling discovery and input/output identity including hardlinks where supported. Its python wrapper drives a compiled harness, not native application UI.
- File launch: `src/tests/file_action_path_tests.cpp` uses `NativePathTestParent` (new shared test support) and real child processes to verify arguments/current directory for ASCII/CJK/emoji/Czech and long paths. Long current-directory behavior depends on DOS alias availability and must be reported, not assumed.
- All requested categories exist across donor harnesses, but no single pre-existing manifest covers every category with distinct hashes and untouched sentinels. Reuse generators per operation. For UI tests, define a temporary owned root, record full UTF-16 and UTF-8 lengths and different payload/hash per identity, plus an unrelated sentinel; create only during the test, verify expected mutations and all untouched inputs, clean only verified root children. S00 creates no fixture tree.
- Path-string tests, generated production-code harnesses, native component binaries, actual paired DLL tests, and full GUI end-to-end tests are distinct evidence types. PictView/EXIF requires actual paired DLL validation; Undelete EFS stub is not real EFS import/export; archive command harness engines may be mocked. Network, filesystem unsupported flags/ADS/hardlinks must be explicit skipped/blocking evidence.

## Reproducible stage recipe (not executed during this audit)

Run only after the corresponding code/test stage is introduced in work/, never in donor/. In an initialized x64 MSVC shell, runner source's native compile form is:

`msbuild.exe src\tests\<name>_tests.vcxproj /m /t:Build /p:Configuration=Debug /p:Platform=x64 /p:OPENSAL_BUILD_DIR=<absolute-worktree-local-build-root>\ /p:PreferredToolArchitecture=x64 /p:RestoreFallbackFolders= /nr:false /v:minimal`

Run `<build-root>\tests\Debug_x64\<name>_tests.exe`; repeat Release. Use a new configuration-specific root. Apply outer command deadline less than 120 seconds; terminate child process tree before timeout. The complete runner has per-child timeouts, but its full many-test loop can exceed 120 seconds, so split into bounded invocations/tests rather than leave it running unbounded.

Native donor projects by scope: array_reserve (S23); branch_context_menu/branch_operations (S21, relevant operation subsets S20); branch_preparation/branch_properties/explorer_property_work (S28); branch_view (S25, performance extension S30); branch_view_text (S24 then resource S26); file_action_path/viewer_enumeration (S18); panel_temporary_selection (S20); worker_path/worker_copy_path (S19). Every source has matching *_tests.vcxproj in REUSE.tsv.

Generated real-C++ harness commands: `python -B src\tests\safe_file_paths_tests.py --configuration Debug` then Release (S01 minimal adapted subset, full S03); `python -B src\tests\renamer_io_tests.py` (S07); `python -B src\tests\checksum_selection_paths_tests.py` (S08); `python -B src\plugins\splitcbn\tests\selection_paths_tests.py` (S09). Only safe_file_paths shown here has its inspected --configuration flag. Other wrappers expose different interfaces; read their parser before adding arguments. Their generated source compiles production snippets into temporary executables, with mocks for selected UI/services; do not count them as just source assertions or as complete application smoke.

Run source contracts registered for the current stage with `python -B <script.py>` and retain all baseline contracts. Keep donor-only tests DEFERRED until dependencies land. Test evidence records exact target candidate SHA, source dirty/staged state if applicable, toolchain/configuration, command, exit result, output/runtime paths and hashes, fixture capability skips, actual GUI steps and expected/actual outputs. No donor's historical assertion count or performance percentage is a port result.
## Additional harness coupling found during final review

- disk_selection_snapshot_tests.py and cpp.in extract PictView `CEnumFiles` from thumbs.cpp and instantiate it in HostTests before the core snapshot cases. This is S12, so S05 needs a narrowly documented deferred PictView subset while retaining all owned SDK/ABI tests; restore and run it in S12. REUSE.tsv has separate S12 rows.
- branch_panel_getters_tests.py extracts `Entry`, `Entry::FullPath`, Join and whole getters from branch_view.cpp. For ordinary S04, adapt only the generator/source location and ordinary subset; keep Branch-state assertions DEFERRED until S26. Do not import scanner just to satisfy test extraction.
- S11 donor diff changes only salamatrix_packages.cpp, salamatrix_sides.h and its project. No new dedicated long-CJK JSON payload test is among the 231 donor files. Existing salamatrix_regression_tests.py is unchanged and source-contract coverage; ordinary SDK long-CJK extension tests are not proof of context serialization/payload limits. S11 must inspect reusable existing tests and add the minimal missing actual serialization/runtime acceptance coverage, with all five provider forwarding contracts validated.
- Host project registration includes `spl_diskselection.h` in S05. Its .filters diff only registers explorerpropertywork, panel_temporary_selection and branch_view_text; do not invent other donor filter hunks.
## S10 current integration boundaries (2026-10-22)

Automation production has no S11 dependency. Its donor selection_snapshot_tests
has a test-only coupling: SidesService::CopyDiskItemInfo and selected/focused
methods, wide SideItemJson and DiskSideContextJson, plus five provider forwarding
contracts. Keep these assertions in the template/explicit All scope and run in
S11; default Automation scope compiles actual item/collection/enumerator methods
and CUnknownImpl. TestAutomation is unchanged. Do not silently import Salamatrix
to satisfy extraction. S11 still needs its actual context serialization/payload
acceptance beyond these tests.

GUI discovered two unchanged releasebase defects outside the S10 diff:
scriptlist.cpp raw CreateFileW versus HANDLES(CloseHandle) registration in Debug;
and fileswn3.cpp CFilesWindow::ChangeDir file-focus branch char
shortenedPath[MAX_PATH] followed by strcpy(shortenedPath, copy). The latter
asserts for long full-file navigation through unchanged COM put_Path. Do not
claim this setter is repaired by owned COM properties. Final S10 GUI navigates
the parent directory then reads actual Items; native tests cover row destruction,
reordering and final ownership release. Initial asserted Debug process was
terminated and recorded separately, not relabeled normal shutdown. These remain
explicit follow-up defects; no host/diagnostic suppression included in S10.

## S11 closure of deferred consumer tests (2026-10-22)

S10 TestSides/TestDynamicJsonContext and all five forwarding contracts now run
in All scope. Native new coverage compiles actual PackageManager::CopyResult
and salamatrix_runtime_protocol.h, then independently parses emitted JSON.
All five providers retain response(65536); protocol whole-frame limit is 1MiB,
including framing overhead. Dynamic context exceeding either capacity is an
explicit failure, not a promise of unbounded transport. Native fixed ItemInfo
ABI remains unchanged and retains insufficient-buffer error for valid CJK names.
GUI is real Python only (D/R), never reported as five worker executions.

## S31b5b1 — úplný základ relativní cesty a kapacita chyb

ParsePath nyní používá owned wide diskový základ převedený beze ztráty do UTF8,
heap SAL_MAX_PATH a skutečný výsledek GetGeneralPath pro virtuální kontexty.
Selhání kontextu ani doplnění lomítka nepředá zkrácený prefix backendu.
SalParsePath ověřuje vstupní kapacitu, odmítne zkrácený/prázdný fallback,
formátuje úplné lokalizované chyby do dynamického bufferu a explicitně odmítá
UNC root přesahující dosavadní legacy ABI. Také vstupní trace respektuje kapacitu.
Native actual wrapper/parser42/config D/R PASS; semantic capacity mutation42/4
failures a error-truncation42/1 failures. Host Debug/Release x64 /warnaserror
PASS. GUI ordinary relative Unicode Copy + Cancel v Debug dark/Release light:
exact readback, zdroje/sentinel nedotčené. GUI před poslední trace-only úpravou;
po ní opět native42/config a oba host buildy PASS. Žádná aktivace Branch targetu.
ChangeDir zůstává odložený, long-source unpack NOT_VERIFIED. Navazuje S31b5b2
owned target identity a modal recheck, poté S31b5b3 ověřená aktivace. Goal ACTIVE,
G04/G05 OPEN, S32 BLOCKED; bez push/PR/CI/CodeQL. Viz [S31b5b1](steps/S31b5b1.md).

## S31b5b2 — identita cílové karty přes modal/callback hranice

Diskový target má owned wide root a úplný UTF8 default s maskou; živost se
ověřuje přes registry běžných i detached karet před dereferencí uloženého pointeru.
Kontrola zahrnuje tab ID, typ, root, Branch stav/generaci a původní viditelnou
stranu. Neaktivní explicitně vybraná karta z detached chooseru zůstává podporovaná.
FilesAction ověřuje target před/po dialogu, po resolveru/CheckPath, před analýzou
stromu a před spuštěním. Ordinary ručně zadaný jiný cíl zůstává povolený.
Actual snapshot50/config D/R + placement contracts PASS, mutation tab50/1 a
root50/6 failures; transitions28/operations All20 PASS. Host D/R x64 /warnaserror
PASS; actual GUI Debug dark/Release light relative Unicode copy + Cancel, exact
readback a nezměněné zdroje/sentinel. Branch target guard stále blokuje vstup;
nejde o ověřené celé Branch Copy/Move ani stale-target GUI. Navazuje S31b5b3
preflight výsledného rootu/mask a ověřená aktivace; před delšími novými cíli také
prověřit SalSplitWindowsPath changesRoot legacy buffer (statický nález níže).
ChangeDir beze změny, G04/G05 OPEN, goal ACTIVE, S32 BLOCKED, bez push/PR/CI.
Viz [S31b5b2](steps/S31b5b2.md).

## S31b5b3a — plný prefix oznámení vytvořených adresářů

SalSplitWindowsPath uchová existující prefix v owned stringu před prvním
filesystem zápisem; oznámení již nepoužívá changesRoot[MAX_PATH]. Zachovány
confirmation/cancel, chyby, maska a non-recursive refresh i při selhání tvorby.
Actual shared Windows/general split a wide CreateDirectory:81 kontrol/config
D/R PASS nad vlastními NTFS fixtures, včetně dlouhých UTF8/UTF16 prefixů,
kolize se souborem, Cancel a přesné notifikace. Safe truncation mutation81/4
failures. Host D/R x64 /warnaserror PASS; transitions28/operations All20 PASS.
GUI D dark/R light ordinary Unicode Copy + readback PASS; Debug dlouhý nový
cíl370 UTF16/884 UTF8: Create confirmation + No bez adresářů/výstupu. Skutečné
vytvoření dlouhých adresářů prokázáno nativně, nikoli GUI tlačítkem Yes.
Navazuje S31b5b3b kapacita path+mask a SDK legacy minimum2*MAX_PATH; tento commit
neopravuje všechny mezní délky. Branch target entry guard zachován. ChangeDir
nedotčený, long-source unpack NOT_VERIFIED, G04/G05 OPEN, goal ACTIVE,
S32 BLOCKED; bez push/PR/CI. Viz [S31b5b3a](steps/S31b5b3a.md).

## S31b5b3b — skutečná kapacita cesty, masky a newDirs

Interní split funkce mají povinné explicitní kapacity. Core předává SAL_MAX_PATH,
nezměněné SDK virtual wrappery garantovaná minima2*MAX_PATH/path a MAX_PATH/newDirs.
Růst o NUL, backslash a universal mask je ověřen před zápisy/potvrzením;
Windows adapter používá soukromý heap mezikrok a kontroluje konečné packed bytes
před tvorbou adresářů. Přesně zaplněný výstup uspěje i při větším mezikroku;
mask se přebazuje do caller bufferu, chyba/cancel ponechá caller path beze změny.
Actual split/create190 kontrol/config D/R PASS, včetně všech předchozích81
podmínek, exact-fit/one-short, SDK minima, redzones a real-I/O odmítnutí před
creation. Safe extra mutace: packing173/32, newDirs173/12, pointer190/16 failures.
Host D/R x64 /warnaserror PASS; snapshot50/config, transitions28/operations All20
PASS. GUI D dark/R light ordinary Unicode Copy + Cancel, exact readback PASS.
SDK hlavičky/vtable a Branch entry guard beze změny; runtime plugin/Branch matice
není tímto hotová. Navazuje S31b5b3c preflight výsledného rootu/mask a ověřená
aktivace. ChangeDir zůstává odložený, G04/G05 OPEN, goal ACTIVE, S32 BLOCKED;
bez push/PR/CI. Viz [S31b5b3b](steps/S31b5b3b.md).
