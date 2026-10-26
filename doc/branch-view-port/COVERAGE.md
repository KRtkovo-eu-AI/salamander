# S31 — Úplnost portu proti donoru

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

S31b4a: přímé Unpack má owned exact source před modalem; actual native
54/config D/R s reálným readbackem a root-alias negativní kontrolou, běžný
ASCII-panel ZIP GUI Cancel/Unpack D/R PASS. Jde o zdrojový předkrok, nikoli
aktivaci Branch Unpack ani long-source GUI PASS. Startup -l nad dlouhým
Unicode adresářem zatím bez targetable window; izolovat v navazující etapě.
Unpack G04/G05 OPEN, goal ACTIVE. Viz [S31b4a](steps/S31b4a.md).

S31b3b2: Properties část G04 RESOLVED_LOCAL po guarded command admission,
actual dialog78/selection22/router179 a real Windows metadata I/O107/config
D/R, GUI single/multi/Cancel/readonly failure + ordinary panel, D/R host Build.
Cesty a hodnoty jsou wide/owned, registry/security policy beze změny.
Handler capabilities/ACL dále rozhodují; snapshot není filesystem transakce.
Viz [S31b3b2](steps/S31b3b2.md). Zbývající G04 a G05 OPEN, goal ACTIVE;
S31b4 unpack je další krok, S32 BLOCKED. Starší záznamy jsou historické.

S31b3b1: actual Windows metadata backend107/config D/R PASS bez změny
produkce. Jde o I/O preflight před guarded GUI aktivací S31b3b2, nikoli
uzavření Properties části G04. Viz [S31b3b1](steps/S31b3b1.md).

S31b3a4b: attributes/NTFS část G04 RESOLVED_LOCAL po skutečném GUI D/R
Cancel + čtyři NTFS roundtripy nad přesnými short/Unicode/long identitami.
Native52/22/178/160/73 každá konfigurace a host D/R PASS; viz
[S31b3a4b](steps/S31b3a4b.md). Zbývající G04/G05 OPEN, goal ACTIVE.
Starší záznamy níže zachovávají stav v okamžiku svého checkpointu.

S31b3a4a: NTFS helper/worker exact wide identity a failure propagation ověřené
native160/config D/R včetně skutečných EFS/komprese; GUI staged do S31b3a4b.
G04 attributes REMAINS_OPEN, viz [S31b3a4a](steps/S31b3a4a.md).

S31b3a3: základní dialog atributů aktivní s root/tab/generation/selection guards;
GUI exact long apply/Cancel D/R PASS. NTFS compression/encryption staged do
S31b3a4; G04 attributes REMAINS_OPEN, viz [S31b3a3](steps/S31b3a3.md).

S31b3a2: attribute/time worker error handling opravené a native73/config D/R
ověřené. G04 attributes REMAINS_OPEN do modal/selection a GUI S31b3a3.
Viz [S31b3a2](steps/S31b3a2.md); nepovoluje nové příkazy.

S31b3a1: atributy mají donorový exact per-item zdroj v planneru. G04 attributes
REMAINS_OPEN: ChangeAttr/host gate stále blokují, worker/dialog/GUI navazují.
Native399/config a host D/R PASS, viz [S31b3a1](steps/S31b3a1.md).

Aktualizace S31b2: shortcut/link-target část G04 IMPLEMENTED_LOCAL; exact wide
source, COM/PIDL, reparse handle a skutečné povolení CM_AFOCUSSHORTCUT. Native92,
router173/config, host D/R a GUI evidence viz [S31b2](steps/S31b2.md). Neuzavírá
ostatní G04/G05 a S32; historické donor indexy níže zůstávají snapshotem S30.

Aktualizace S31b1a: archive-entry část G04 nyní RESOLVED_LOCAL po opravě
preexistujícího 7zip wide-name úniku. Baseline/current ordinary i Branch měly
stejný 30byte leak; scoped ownership jej odstraňuje bez změny heap filtru či ABI
projekce. Native185/config a instrumentované GUI PASS, viz [S31b1a](steps/S31b1a.md).
Ostatní core G04/G05 a S32 BLOCKED trvají. Starší záznamy níže jsou historické.

Aktualizace S31b1: archive entry část G04 IMPLEMENTED_LOCAL, přesná identita a
callback guards; native267/config, host D/R a funkční GUI D/R PASS. Tři navazující
path buffery opravené; finální unload ověření viz [S31b1](steps/S31b1.md).
G04 zbývající core operace a G05 zůstávají otevřené, goal ACTIVE, S32 BLOCKED.
Historické S30 indexy níže zůstávají snapshotem před opravami.

Aktualizace S31a3b: **G03 RESOLVED_LOCAL** po produkční opravě S31a3a a aktivaci
celého rozsahu v S31a3b. Default/registrované příkazy/discovery mají 20 + 19 PASS,
assertions zachované, rename notifikace aktivní i v All. Viz [S31a3b](steps/S31a3b.md).
G01/G02 jsou již uzavřené; G04/G05 a S32 BLOCKED trvají. Uživatel požaduje core
operace G04 implementovat: následuje S31b1, nikoli pouze dokumentace odmítání.
Historické indexy pod tímto záznamem zůstávají snapshotem S30.

Aktualizace S31a3a: původní označení Shell G03 erroru jako selector-only bylo
nesprávné. Obnoven skutečný donorový post-retry guard a quiet capture; původní
test beze změny assertions nyní PASS, native20/config + D/R host a GUI PASS.
Viz [S31a3a](steps/S31a3a.md). G03 ještě vyžaduje S31a3b (clipboard selector,
All a temporary selection). Uživatel pro S31b výslovně zvolil doplnění donorových
core operací po malých ověřených krocích; dokumentace odmítání je neuzavírá.
G04/G05 a S32 zůstávají otevřené, goal ACTIVE.

Aktualizace S31a2 (2026-10-25): **G02 RESOLVED_LOCAL**, donorový ANSI-to-wide
rename hook, Debug/Release buildy, vynucený native fallback a skutečné ordinary/
Branch GUI PASS, viz [S31a2](steps/S31a2.md). G01 je již uzavřen S31a1.
Historické TSV zůstávají snapshotem S30; G03–G06 a S32 BLOCKED trvají. Goal S31
pokračuje S31a3 a uzavřením support/validation hranic S31b/c. Bez push a remote CI.

Aktualizace S31a1 (2026-10-25): **G01 RESOLVED_LOCAL** — donorové zapojení
wrapperu opraveno, host Debug/Release i native/GUI ověření PASS, viz
[S31a1](steps/S31a1.md). Následující audit a TSV indexy jsou neměnný snapshot S30;
počet DEFERRED v nich není aktuální počet otevřených nálezů. G02–G06 a S32 BLOCKED
zůstávají. Bez push a remote CI tohoto následného checkpointu.

Stav: AUDIT_COMPLETE_WITH_FINDINGS, 2026-10-25. **S32 BLOCKED.** Tento checkpoint
mění pouze dokumentaci. Neopravuje níže nalezené mezery ani nezpřístupňuje další
operace. Branch View zůstává ve standardním procesu vypnuté. Výsledkem auditu je
uzavřený seznam rozhodnutí a opravných podetap, nikoli potvrzení úplné funkční parity.

## Připnuté vstupy a metoda

| Vstup | Přesná revize |
| --- | --- |
| Donor PR | [KRtkovo-eu-AI/salamander#769](https://github.com/KRtkovo-eu-AI/salamander/pull/769) |
| DONOR_SHA | `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413` |
| DONOR_BASE_SHA / TARGET_SHA | `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449` |
| Auditovaný přijatý port S30 | `58aabd60054f98d964f2be7b93bbf408ac692dde` |
| SHA-256 uloženého donor.patch | `7bd21bd8e40b745630a605fa80f96f89bc02da3cce28f1723fa3c22f139158cc` |

SOURCE.json a donorový checkout zůstávají nezměněné. Inventář je odvozen z diffu
DONOR_BASE_SHA..DONOR_SHA a skutečných souborů auditovaného portu. Oba vycházejí
ze stejného target/base commitu. Dodatečné target opravy se automaticky nenahrazují
donorovým textem. Kontrola zahrnuje implementace, volající, hlavičky, resources,
registrace projektů/testů, binární conversion data a explicitní odklady.

Úplné rozhodovací indexy tvoří součást auditu:

- [COVERAGE-FILES.tsv](COVERAGE-FILES.tsv): všech 231 donorových cest, donor/target SHA-256, rozhodnutí a důvod.
- [COVERAGE-HUNKS.tsv](COVERAGE-HUNKS.tsv): všech 892 bloků v pořadí uloženého patch, rozsah a rozhodnutí; ID se vztahuje pouze k tomuto připnutému patch.
- [COVERAGE-EXTRAS.tsv](COVERAGE-EXTRAS.tsv): všech 190 dalších změněných target cest BASE..S30, jejich role a poslední checkpoint. Nové dokumenty S31 v tomto historickém inventáři nejsou.

147 souborů je EXACT po odstranění UTF-8 BOM a normalizaci konců řádků; pouze
6 je současně bajtově shodných. 82 existujících cest je ADAPTED a 2 chybějící
release dokumenty jsou DEFERRED. Na úrovni bloků je 470 EXACT, 416 ADAPTED a
6 DEFERRED. ADAPTED není tvrzení úplné parity: odkazované G01–G06 mají přednost.
Hunk 165 například obsahuje převzaté části i chybějící bezpečné směrování.

Textový screening nalezl 762 donorových postimages v targetu a 130 k ručnímu
prověření. Tento počet není počet ověřených funkcí: stejný text může být v jiné
funkci. Proto jsou všechny bloky adaptovaného souboru konzervativně ADAPTED,
pokud nejsou konkrétně DEFERRED; EXACT bloky pocházejí jen z celých shodných
souborů. Například obsluha IDC_BRANCH_KEEP_PATHS je přítomná v reálném
CCopyMoveMoreDialog; její nepřevzaté umístění v základním dialogu není funkční mezera.

## Nálezy a nejmenší další podetapy

### G01 — Nezapojený wrapper vytváření adresáře; S31a1, blokuje S32

`src/worker.cpp::SalCreateDirectoryEx` stále obsahuje staré tělo s lokálním
`3 * MAX_PATH` bufferem a podmíněným přechodem na wide API. Donor hunk 859
nahrazuje tělo voláním `WorkerCreateDirectory(name, err)`. Samotný wide helper
a jeho přímé worker použití již existují a mají testy; to neověřuje tento wrapper.
Wrapper má živé volající: MakeDir v `src/fileswn5.cpp`, veřejný SDK delegát v
`src/zip.cpp` a spotřebitele FTP/DemoPlug. Nejde o prokázaně mrtvý kód.

S31a1 má převzít úzké donorové zapojení při zachování současných wide oprav a
ověřit skutečný wrapper pro běžné ASCII, Unicode, UTF-8 byte-long, UTF-16 long,
relativní vstup, trailing dot/DOS collision a přesnou chybu/retry. Vyžaduje oba
host buildy, relevantní nativní konzumenty a skutečné GUI MakeDir. Audit doložil
chybějící zapojení staticky; neprovedl destruktivní demonstraci chybné operace.

### G02 — ANSI fallback inline přejmenování; S31a2, blokuje S32

`src/fileswn5.cpp::RenameFileInternal` nemá donorovou časnou Branch odbočku do
`RenameFileInternalW` (smíšený hunk 165). Běžná cesta F2 je wide a již byla
ověřena. `QuickRenameBegin` však při neúspěchu wide edit control může vytvořit
ANSI control; `HandeQuickRenameWindowKey` potom volá starou funkci. Tento vzácný
fallback nesmí odvodit cíl z rootu a basename místo skutečné identity položky.

S31a2 má převzít odpovídající směrování nebo přesně odmítnout nepodporovaný
fallback. Vynucený fallback test musí zachovat přesnou identitu, ochrany cache,
reentrance a obyčejné přejmenování; následně Debug/Release a GUI. Běžný úspěšný
wide GUI scénář nepokrývá tuto větev. Záměna uživatelského souboru nebyla v auditu
runtime testem reprodukována; chybějící hook a dosažitelná větev jsou doložené.

### G03 — Rozsah a odložené source-contract testy; S31a3, blokuje S32

`branch_view_contract_tests.py` stále odkládá
`test_temporary_dialog_selection_uses_exact_branch_identity`. Jeho čerstvé
explicitní spuštění prošlo (1 test). Je nutné jej zapojit do běžného běhu.

`branch_operations_contract_tests.py` implicitně spouští OrdinaryOperations,
14 z 20 metod; runner nepředává `--scope All`. Čerstvé explicitní All spustilo
20 metod: 18 PASS a 2 ERROR. Test názvové schránky vyhledává staré
`if (IsBranchView())`, kde port záměrně používá širší `if (Is(ptDisk))`.
Test fallbacku Shell menu hledá starou přesnou podmínku po změně bezpečného
směrování kontextové nabídky. Jde o ValueError selektoru, nikoli důkaz selhání
souborové operace, ale výsledky rozhodně nejsou PASS.

S31a3 má aktualizovat pouze vyhledání skutečných relevantních těl, zachovat
bezpečnostní assertions, zapnout All a dosud odložený test. Všechny dosavadní
regrese zůstanou registrované. Inventář zachoval všech 41 pojmenovaných donorových položek; před S31a1 jich
port má 57. Samotná existence položky neznamená běh všech jejích testů.

### G04 — Bezpečně nepodporované operace; S31b, rozhodnutí před S32

Tato tabulka zachycuje vstupní stav auditu před opravami S31b1 a dále; aktuální
checkpointy a jejich důkazy jsou uvedeny výše. Staré odmítnutí nelze přenášet na
později ověřené operace. Následující odchylky měly explicitní ochrany; jejich
plošné odstranění není oprava:

| Oblast | Stav proti donoru a rozhodnutí |
| --- | --- |
| `TryEnterFileAsArchive` | Branch guard zachován, donorové úplné identity v hunks 43/44 nepřevzaty. Odlišit tento vstup od již ověřené běžné Enter/archive cesty S27a2. |
| `FocusShortcutTarget` | Branch guard; donorový `FocusBranchShortcutTarget`, deklarace a hook chybějí (hunk 45 a části branch_view.cpp/fileswnd.h). |
| Windows Properties, ChangeAttr, přímé Unpack | Branch vstupy nadále odmítnuté; nepovažovat za úplnou donorovou podporu. |
| Kopírování/přesun do Branch cíle — aktualizace S31b5c | Diskový Copy/Move do jediného diskového cíle ověřen v d1/d2/e1/e2/e3; úplný root/mask, modal tab/generation a explicitní ruční cíl. Archive source/FS/multi-target Branch případy tato aktivace nezpřístupňuje. |
| F3 plugin viewers | Obecné DLL prohlížeče nejsou připuštěné. Interní a konfigurovaný externí viewer mají vlastní ověření. PictView EXIF menu 188 a konfigurace 197 neprokazují obecné Branch F3. |
| Neznámé plugin příkazy, externí packery | Explicitní admission policy v plugins1.cpp, žádné automatické povolení podle existence DLL. DemoPlug diskový snapshot se SDK>=105 neznamená obecné povolení jeho Branch příkazu. |
| Shell a dlouhé cesty | Program/document oddělení a Windows kontroly zachovány. Nepodporovaný případ odmítnut s vysvětlením; LongPathsEnabled=0, registry a zabezpečení se neměnily. |
| PAK Optimize | Záměrně skryté menu v původním kódu i donoru; uživatel výslovně zvolil zachování a dokumentaci. Test těla příkazu nepřidává viditelnou funkci. |
| ZIP Repair | Záměrně skrytý příkaz i v donoru; nepovažovat jej za dokončenou aktivní nabídku. |

S31b má pro konkrétní veřejně požadované core operace vybrat samostatnou
implementaci s testy, nebo výslovné release omezení a kontrolu všech jejich vstupů.
PAK Optimize se neobnovuje. Přijaté limity Windows se neobcházejí. Do uzavření
rozhodnutí nelze tvrdit úplnou funkční paritu ani zapnout celý port pro vydání.

### G05 — Hranice skutečné validace; S31c, uzavřít před S32

- Undelete RestoreEncrypted2 má native EFS callback stubs a skutečné GUI kopie nešifrovaných souborů. Windows EFS import/export skutečně šifrovaných dat zůstává NOT_TESTED. Je třeba cílená skutečná validace, nebo výslovná podpora omezená v dokumentaci a admission policy.
- Connect image ověřuje přesný wide prefill a odmítá nepodporovaný ACP/legacy vstup. Fyzický disk a všechny backend scénáře nebyly tímto ověřeny.
- Historické SMB/live UNC/offline cancellation a některé ADS/ACL/hardlink scénáře mají konkrétní unavailable/skip hranice; test řetězce UNC není skutečný SMB provoz. Chybějící prostředí nesmí být přepsáno na PASS.
- Všech 11 překladů má ověřené Branch resource ID/řádky. Kompletní jazykový balicí/staging runtime matrix zatím neproběhl. Čistá podporovaná CI matice je Debug Win32/x64/ARM64 se stávajícím vyloučením SFTP; CodeQL Release x64 není celé Release sestavení všech pluginů.

S31c má uzavřít konkrétní validační tvrzení: doplnit potřebnou skutečnou zkoušku,
nebo zúžit veřejný příslib a ověřit bezpečné odmítnutí. Audit neopakuje celou
historickou GUI matici ani neprohlašuje všechny tyto scénáře za novou regresi.

### G06 — Release dokumentace a malé nepřesnosti; S32 po bráně

Chybějí donorové `doc/branch-view.md` a `doc/branch-view-plugin-paths.md`.
Jejich vytvoření patří plánované S32: popsat skutečný port a jeho limity, bez
převzetí donorových historických testů jako target důkazu. Dva komentáře nových
služeb v spl_gen.h stále popisují jen ordinary disk/UNC; rozšíření na Branch
má být popsáno bez změny signatur nebo layoutu. Chybějící IDE filter položka
explorerpropertywork.h nebrání build registraci; může být uklizena samostatně.

## Převzaté kontrakty a úmyslné adaptace

| Oblast | Auditované rozlišení |
| --- | --- |
| Scanner, scheduler, publikace | Donor algoritmy zachovány; S30 připojuje recursive watchery a owned full-path notifikace bez ztráty Stop Scan. S28a mění pouze zobrazení Path; absolutní cesty pro I/O zůstávají. |
| Identita a životnost | Snapshoty vlastní cesty/metadata; Name/NameW zůstávají basename. Target přidává reentrance/cache kontroly, exact temporary selection a owned parent metadata. |
| Host operace | Wide create/copy/move/delete/recycle, clipboard/drop a packer enumerace mají jednotlivé checkpointy; jejich důkaz nelze přenést na odmítnuté operace G04 ani wrapper G01. |
| Shell | Přímé programy oddělené od dokumentů, bezpečnostní kontroly Windows, odložené host dispatch po uzavření menu, ochrany vlastnictví a rootů. Žádné vypnutí ochrany nebo změna registrů. |
| Balení | ZIP/7-Zip snapshoty a host callbacky; target wide SafeDeleteFile opravuje odstranění přesné zdrojové identity při move packing. |
| Plugin spotřebitelé | Checksum, Split/Combine, FileComp, Automation, Salamatrix, připuštěné ZIP/7-Zip/PictView/Undelete příkazy mají úzké native/GUI důkazy v S27f checkpoint dokumentech. Obecná kompatibilita DLL není kompatibilita každého Branch příkazu. |
| SafeFile | Target navíc řeší krátký relativní vstup pod dlouhým cwd; široký create/open/retry se nepřepisuje starší donor podmínkou. |
| Icon/handle tracking | Zachované dřívější target opravy účetnictví a ukončení CLR. Donorová odstranění registrací nesmějí vytvořit dvojí registraci nebo skrýt diagnostiku. |
| Konfigurace a historie | Uložená preference relativní/absolutní Path s absolutním defaultem; tab/history a vypnutý process gate podle S29. Persistence se neposuzuje šířeji než skutečné scénáře. |

## SDK, pět providerů, resources a data

Nejnovější skutečně publikované upstream vydání znovu ověřené přes GitHub API je
[5.0-samandarin-0.15.1](https://github.com/KRtkovo-eu-AI/salamander/releases/tag/5.0-samandarin-0.15.1),
commit `c0cddbe7442fe49abcb859624e271ce687aee0d8`; větev 0.16 není vydaný SDK základ.
Nezávislý parser všech 13 SDK hlaviček porovnal 38 Abstract rozhraní a 632 pure
virtual metod: vydané prefixy pořadí/podpisů zůstávají shodné. spl_com.h, tedy
CFileData/PluginData kontrakt, odpovídá vydání po textové normalizaci. Existující
ABI golden/HEAD testy doplňují tento širší audit. To je prefix/layout kontrola,
nikoli důkaz funkce libovolné staré binárky v Branch View.

Adresáře JavaScript, Python, PowerShell, PHP a Lua runtime providerů nemají změnu
BASE..S30. Sdílené Salamatrix sides rozhraní odpovídá donoru. Čerstvé skutečné
Automation/shared-consumer native testy `--scope All --configuration Both`
prošly 243 kontrolami v každé konfiguraci včetně pěti forwarding kontraktů,
nezávislého JSON escaping/schema/metadata a původního limitu 64 položek. Tento
běh není nové GUI spuštění všech pěti enginů; skutečné engine/runtime scénáře
mají vlastní historickou S27f.06a evidenci.

Zůstává limit 64 selected položek a 65536bytový provider buffer (65535 payload
plus terminátor). ZIP/Automation ponechávají SAL_MAX_PATH **byte** kapacitu,
FileComp svou vstupní byte kapacitu a Undelete image backend lossless ACP limit.
Tyto explicitní limity nejsou rozšířeny samotným wide snapshot API. Změna limitů
nebo runtime schématu není součást portu ani tohoto auditu.

Všech 13 donorových native projektů existuje. Runner zachovává 41 donorových
pojmenovaných položek; snapshot S30 jich má 57. Jde o Name assignments, nikoli
ClassName labely či dynamicky registrované projekty. Původní údaj 59 chybně
zahrnoval dva ClassName labely; S31a1 opravuje pouze tuto početní nepřesnost.
G03 vyjmenovává skutečné scope mezery.
Všech 15 donorových IDS_BRANCH_* má shodné číselné ID a právě jeden řádek v
každém z 11 jazyků. Conversion cfg zůstávají ve Windows CP1250/1251/1252 bez
UTF-8 BOM; 28/17/25 tabulek má po 256 bajtech. Česká jména Kameničtí/KOI-8 ČS2
byla kontrolována i po dekódování, nikoli plošnou UTF-8 konverzí binárních dat.

## Důkazy a jejich omezení

Přijatá S30 má pro přesný head 58aabd60 všech 13 kontrol SUCCESS v
[PR #62](https://github.com/majklvi/salamander/pull/62), včetně skutečných CodeQL
analyze/SARIF kroků a všech reálných Build full solution kroků čisté podporované
matice. [CodeQL/build run](https://github.com/majklvi/salamander/actions/runs/40186355068)
a [clean matrix](https://github.com/majklvi/salamander/actions/runs/40186366561)
jsou acceptance S30, nikoli důkaz uzavření G01–G06. Interní validační draft se
nemerguje do samandarin-0.16. Lokální S30 GUI/build/native důkazy zůstávají v
[S30](steps/S30.md); audit netvrdí nový host build nebo celou GUI zkoušku.

Audit historie prošel 104 checkpoint commitů a 83 metadatových záznamů.
655 source hash vazeb odpovídá prvnímu odvozenému checkpointu; všech 753
kontrolovaných archivovaných evidence hashů souhlasí. Ze 16 počátečních source
neshod se 6 dohledalo v odpovídajícím ancestor snapshotu. Zbývajících 10 jsou
archivované rozpracované source snapshoty, které nebyly commitnuté bajtově:
S06 owned runner; dva S12-preflight WIC testy; S26b PLAN; S27a PLAN a pět zdrojů/
harnessů. Neoznačují se jako ověřené commitnuté zdroje. Staré schéma metadata
není úplný replay každého buildu; aktuální rozhodnutí stojí také na skutečném
kódu, finálních checkpoint vazbách a fresh testech. Starší NOT_PUSHED/PENDING
odstavce v etapových dokumentech jsou historické, pokud je novější záznam nahrazuje.

Úplná lokální evidence je mimo Git v `codex_pr769_prompts/evidence/S31/`:
`inventory.json`, `inventory-summary.json`, `residuals/`, `classification.json`,
`checkpoint-evidence-audit.json`, `abi-provider-audit.json`,
`provider-consumers.json` a stdout, `tests-resources-data-audit.json`, explicitní
probe stdout/stderr a generátor `write-coverage-tables.py`. Historická S30 CI
odpověď je v sousedním `S30/ci-completion.json`. Do projektu se necommitují logy,
binárky, raw donor diff ani automaticky generované fixture soubory.

Původní donor výsledky se nepoužily jako nové target PASS. Předchozí ZIP dialog
Debug heap diagnostika byla doložena i na parent checkpointu; zůstává evidovaným
baseline omezením, nikoli tvrzením, že tiché S30 ukončení opravilo všechny úniky.

## Pořadí pokračování

1. S31a1 — wrapper SalCreateDirectoryEx a přímé regresní důkazy.
2. S31a2 — bezpečné ANSI rename fallback směrování a vynucená větev testu.
3. S31a3 — aktivace úplného source-contract rozsahu bez oslabení assertions.
4. S31b — uzavření podporovaných/odmítnutých operací G04 po konkrétních příkazech.
5. S31c — uzavření chybějících skutečných validací nebo omezení příslibů G05.
6. S32 — teprve po uzavření brány release dokumentace G06 a publikační rozhodnutí.

Audit S31 je tímto dokončen. Produkční opravy, další publikace a odstranění gate
jsou samostatné kroky. S31 nepřepisuje žádný přijatý checkpoint ani nepředstírá,
že zelené CI uzavírá nepouštěné testy či nepodporovanou funkci.

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
