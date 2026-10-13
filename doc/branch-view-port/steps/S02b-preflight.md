# S02b-preflight — Debug startup/shutdown acceptance

Stav: **PENDING**, zjištěno při širší S02b sestavě 2026-10-13.
Je to nejmenší předkrok přijetí S02b; ne další donorová implementační etapa.

1. Service Explorer `EnsureServiceImageList` v fs1.cpp:41–48 a
   `SetServiceExplorerIconList` v plugin.cpp:213–226: raw LoadImage získá HICON,
   HANDLES(DestroyIcon) jej hledá v trackeru, kam nebyl vložen. Debug hlášení
   oběma doložena při prvním auto-install startupu. `CreateServiceExplorerIconBitmap`
   má stejný vzor v plugin.cpp:185–190; ověřit jej v témže úzkém ownership auditu.
   Tyto soubory v S02b nezměněné. Nejmenší oprava musí párovat evidence a
   lifecycle, nikoli vypnout tracker/diagnostiku nebo plošně měnit ikony.
2. Debug Alt+F4 po úspěšném F3/F5 ukázalo `Detected memory leaks!`; následoval
   vlastní Bug Reporter. Leak původ není známý. Nejprve reprodukovat na přesném
   parentu s odpovídající úplnou vlastní pluginovou sestavou nebo izolovat
   příčinný plugin/allocation; pak stanovit úzkou opravu. Neoznačovat za
   baseline jen podle domněnky a nemaskovat vypnutím heap checků.

Zachovat existující S02b kandidátní commit, patch a jeho raw source hashe/evidence.
V dirty worktree neprovádět reset/stash ani mazání. Případný nezávislý baseline
fix řešit odděleně od portového kandidáta; piny SOURCE neměnit a úplné donorové
SDK/branch změny nepřebírat. Neposouvat S03 ani upstream publikaci; interní PENDING fork draft je autorizovaný.

Acceptance: dotčené komponenty a host Debug/Release, skutečný Debug první start
s úplnými matching resources bez icon diagnostiky; F3/F5/hash/sentinel a čistý
shutdown bez heap hlášení. Native S02b faults/Unicode/long a původní regresní
sady zachovat. Po změně zopakovat relevantní bránu pro přesný patch/SHA.
Portables language-peer build musí být součástí sestavy; první přerušené
spuštění bez language resources není PASS. Debug report nebyl odeslán;
čtení Bug Reporteru v CU mělo app approval timeout. Logy uchovány mimo Git.
