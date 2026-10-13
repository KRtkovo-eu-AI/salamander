# S02b-preflightb — Samandarin Update Notifier shutdown lifetime

Stav: **DONE**, 2026-10-13. Kód `23d5406d49eeee40cda9ef19288ff66b6618fd57`
prošel lokální runtime i všech 12 exact-head fork CI checks včetně CodeQL;
uživatel následně schválil přijetí S02b. Samostatná baseline oprava po icon fixu.

## Implementace a důvod

Nejde o nový donor import. Prázdný SDK tracker alokuje až při první registraci;
neúspěšné zvětšení nepokračuje zápisem do NULL. Assembly path/version jsou
operation-owned; dynamická wide cesta zachovává dlouhé a Unicode adresáře.

Volitelný soukromý export `Samandarin_PrepareForProcessExit(timeoutMs, finalize)`
má dvě fáze. `CPlugins::UnloadAll` nejprve ověří dokončení inicializace před
odpojením kteréhokoliv pluginu. Čekání je omezené na 2000 ms a dispatchuje pouze
sent messages, nikoli posted UI práci. Timeout ponechá všechny pluginy načtené.
Starší SPL bez exportu zachovává dosavadní fallback. Nejde o virtual SDK změnu.

Finalizace až při skutečném Unload Samandarinu uzavře přístup nativních callbacků,
po jejich doběhnutí uvolní init thread HANDLE a vlastní CLR hosting reference.
Managed Shutdown/CLR Stop zůstávají na process-close cestě vynechané. Nativní
P/Invoke vstupy po uzavření brány vracejí neutrální výsledek bez host pointers.
Normální plugin Release mimo ukončení procesu zachovává původní chování.

Tracker provede původní leak diagnostiku ještě za života hostitele, poté uvolní
storage. Finalizace je idempotentní; pozdější destruktor CLR-přidržovaného DLL
už neopakuje diagnostiku přes zaniklé hostovo rozhraní. Žádná heap/handle kontrola
ani low-memory hlášení nebyly odstraněny. Přibyly metody/privátní stav nativního
trackeru kompilovaného do každého SPL; žádné veřejné Abstract vtable se nemění.

## Důkazy a omezení

Vlastní native debugger zachytil AV v C__Trace::SendMessageToServer (dbg.cpp:285)
z C__Handles destructor během LdrShutdownProcess. Po časné finalizaci exit 0 bez
AV. Zpoždění skutečného InitializeRuntimeThread v plné Debug/Release sestavě
ověřilo timeout se zachovanými pluginy, následný initialization result 1 a
opakované zavření s exit 0. Pomocník pozdržuje pouze vlastní testovací proces;
GUI vstupy přes Computer Use. Žádný crash report nebyl odeslán.

Pozdní první verze hooku nestačila: jiné pluginy již zastavily CLR, takže
obnovená inicializace selhala. Opraveno readiness fází před UnloadAll loop;
neúspěšné pokusy zůstávají v externí evidenci, nepočítají se jako PASS.

Host + 30 podporovaných consumers Debug/Release x64 PASS (60 consumer builds).
Native runner: 41 skupin/config PASS; reálný SDK test 426 Debug/89 Release,
shutdown test 26/config. Pytest 49 PASS. PHP subcase SKIPPED a long-bitmap
LoadImageW backend subcase SKIPPED; celé JUnit groups je nezapočítávají jako skip.
SFTP a legacy Portables 2015 EXCLUDED z podporované sestavy, nikoli PASS.

Metadata fixture executable běhy pokrývají Unicode parents/name, UTF8-byte-long
a skutečnou UTF16-long cestu v obou konfiguracích. Pro poslední případ .NET
launcher selhal error 206; bounded native CreateProcessW launcher uspěl.
Parent 20d6018c se starou eager alokací očekávaně neprošel unused-registry CRT
assertion (422 checks/1 failure v tehdejší verzi testu); nejde o finální 426 test.

Plný runtime zachovává notifier i ostatní plugins/resources. GUI cold start,
restart, F3/F5/hash a čistý shutdown jsou vedené zvlášť od diagnostických běhů.
Piny SOURCE/CONTRACT zůstávají beze změny. Referenční větev navazuje fast-forward
na přijatý kód a dokumentační acceptance záznam. Win32/x64/ARM64 matice a CodeQL
pro kód 23d5406d SUCCESS; S02b přijatá. Další etapa S03 zde nezačíná.

Evidence: `C:/Samandarin/codex_pr769_prompts/evidence/S02b-preflightb`.
Finální command JSON, zdrojové/fixture/runtime manifesty a historie selhání
oddělují postupné patche nad parent 20d6018c. Podrobnosti v VALIDATION.md.

## Původní zadání předkroku

Konkrétní oblast: CPluginData::Unload/Remove přeskočí Samandarin Release během
UnloadingPluginsForMainWindowClose. ManagedBridge_Shutdown čeká na background
initialization, volá managed Shutdown/CLR Stop/Release a používá globální
std::wstring. CLR může držet SPL přes P/Invoke NativeStrings déle než hostův
heap sentinel. Zůstává SDK tracker pole + path/version native storage.
Tyto zdroje jsou beze změny proti pinned target; není to další donor import.

Nejprve ověřit, které nativní alokace/lifetime lze ukončit bezpečně před hostovým
heap checkpointem. Zachovat existující důvod pro vynechání managed callbacks;
neodstraňovat skip naslepo a nezavádět neomezené čekání na UI threadu. Nepotlačit
heap/tracker diagnostics, nerozšířit byte-count výjimku, nezměnit vtable SDK.
Případný úzký plugin/host teardown protokol musí být samostatně zdůvodněný a
kompatibilní se starším pluginem; jiné runtime providers se do něj nevtahují.

Acceptance: skutečná plná staged sestava s notifierem, Debug/Release, cold
první start a restart, rychlé zavření během background init i normální zavření
po inicializaci; žádný deadlock/heap/handle modal. F3/F5/hash/sentinel a S02b
Unicode/fault regresní sady zachovat. Důkazy vázat na přesný patch/SHA.
Bez notifieru byl pouze izolující control, nikoli přijatá acceptance sestava.
