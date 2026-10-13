# S02b-preflightb — Samandarin Update Notifier shutdown lifetime

Stav: **PENDING**, samostatný nejmenší baseline předkrok po icon fixu.

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