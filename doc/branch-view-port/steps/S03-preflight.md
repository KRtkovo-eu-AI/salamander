# S03-preflight — Debug heap lifetime po archive smoke

Stav: PLANNED/BLOCKING. S03 je implementovaná a lokální build/native brána
prošla, ale její runtime brána není uzavřená. Nepokračovat do S04.

## Pozorování a hranice

S03 Debug fresh: první panely, F3 ASCII, F5 copy a skutečné rozbalení vlastního
ZIPu přes `SalamanderSafeFile->SafeFileCreate` (`src/plugins/zip/extract.cpp`)
fungují; obsah výsledku ověřen zvlášť. Po Alt+F4 je `Heap Message: Detected
memory leaks!`. Hlášení pochází z `C__GCHeapInit::~C__GCHeapInit`,
`src/common/heap.cpp:134`, po `_CrtMemDifference` a dumpu objektů.

Kontrolní host Debug byl sestaven s přesným `src/safefile.cpp` z parentu
`17678e7cac710f176683403b8cae382aee087dbf`; ostatní host runtime zdroje jsou
beze změny proti parentu. Samostatný nový prázdný config, vlastní stejné ZIP,
jeho otevření v levém panelu a ukončení reprodukují stejné hlášení, bez
extrakce či volání změněného SafeFile. S03 wide implementace tedy není nutná
k reprodukci. To neurčuje vlastníka alokace ani nedokazuje, že příčinou je ZIP.
Po obnovení finálního SafeFile a přestavbě hostu hlásí heap i běžný Debug
restart s configem vytvořeným během archive smoke. Release fresh/restart,
F3/F5 a ukončení bez této diagnostiky.

## Nejmenší další krok

Zachytit skutečný CRT dump, počty/velikosti bloků a jejich původ v tomto
vlastním runtime. Porovnat prázdný diskový start bez archive, pouhé otevření
ZIPu, extrakci a restart; použít stejné binárky/resources a oddělené configy.
Teprve podle důkazu určit minimální lifetime/cleanup opravu a test.
Nerozšiřovat ignorované heap rozdíly, nevypínat tracker ani heap diagnostiku.
Neimportovat další donorové etapy, pluginové path migrace nebo SDK změny.

Po opravě ověřit Debug/Release host a skutečně dotčené konzumenty, příslušné
nativní testy, kontrolní původní scénář, běžné panely, extrakci/obsah a čisté
ukončení. Pak lze uzavřít S03 a přijmout její koherentní checkpoint.
Uživatel mezitím autorizoval kandidátní commit/push S03 do forku pro interní
CI na `integration/pr769-s03-candidate`; tento kandidát zůstává PENDING.
Původní SOURCE/CONTRACT/donor piny se nemění. Tento plán není oprávnění k
pushi/PR ani označení S03 za DONE.

Evidence: `C:/Samandarin/codex_pr769_prompts/evidence/S03`;
`Debug-Fresh-launch.json`, `parent-Debug-Fresh-launch.json`,
`Debug-Restart-launch.json`, `parent-host-debug.*`, `host-debug-restored.*`
a `final-summary.json`. Kontrolní parent build/launch JSON sdílejí obecný
code-patch digest kandidáta; skutečné použití parent zdroje uvádí samostatná
control evidence. Neinterpretovat je jako běh finální S03 binárky.
