# Pracovní smlouva: postupný port PR #769

Cílem je převzít existující řešení z PR KRtkovo-eu-AI/salamander#769 do nové
lineární větve jako malé, sestavitelné a ověřené změny. Není to nový návrh
Branch View ani opakování celého původního vývoje.

## Zdroje a rozsah

- Řiď se platnými AGENTS.md/AGENTS.override.md pro dotčené cesty, oprávněními
  prostředí a dokumenty doc/branch-view-port/{PLAN.md,SOURCE.json,REUSE.tsv,
  PROGRESS.md,VALIDATION.md}. Pravidla projektu ani CI neoslabuj.
- DONOR_SHA je připnutý výsledek původního PR; DONOR_BASE_SHA jeho merge-base
  s tehdejší base; TARGET_SHA výchozí commit nové větve. Nezaměňuj tyto hodnoty.
  Po bootstrapu je bez samostatného zadání neměň.
- Používej finální donorový kód včetně pozdějších oprav, ne automaticky pořadí
  původních commitů. Historii prohlížej jen při nejasném původu konkrétní změny.
- Před psaním nové funkce nejprve najdi odpovídající implementaci a test v donoru.
  Celé nové, izolované soubory lze převzít beze změny. U existujících souborů
  přenášej jen relevantní hunks nebo funkce; zachovej mezitím přidané změny targetu.
- Nevytvářej vše znovu, nepřekládej znovu hotové překlady, nepřidávej paralelní
  framework, duplicitní API ani novou generickou architekturu. Nový kód je
  odůvodněný jen chybějící závislostí, přechodovým adaptérem, integrační mezerou
  nebo doloženou opravou.
- Žádný plošný checkout donorového stromu, cherry-pick celého monolitického
  commitu ani import celé funkce schovaný za přepínač. Žádný globální formatter.
- Zachovej SPDX, copyright, provenance, formát zdrojů a binární/legacy encoding
  dat. Nespoléhej na přesměrování textu v shellu, které může změnit BOM či bajty.

## Pravidla jedné etapy

- Proveď právě zadanou etapu. Standardně vytvoř jeden koherentní commit.
  Změna zahrnuje implementaci, deklarace, projektové soubory, potřebné resources,
  odpovídající testy a jejich registraci — ne rozbité meziverze.
- Před editací ověř stav větve, její předpoklady a skutečný kód, ne pouze checkbox.
  Hotovou etapu znovu neaplikuj. Nevytvářej prázdný commit.
- Narazíš-li na nehotovou závislost, vypiš konkrétní symbol a místo použití.
  Zapiš nejmenší potřebný předkrok do PLAN.md; nevtahuj skrytě zbytek PR.
  Buď proveď pouze tento malý předkrok, nebo skonči s blokací. Nepřeskakuj dopředu.
- Příliš velkou etapu rozděl podle funkčních hranic na podetapy. Jejich pořadí
  a akceptační testy nejprve zapiš. V jednom běhu proveď pouze první z nich.
  Nerozděluj pevně svázané rozhraní a implementaci tak, že by nešel build.
- Do REUSE.tsv průběžně zapisuj donorový soubor/symbol/hunk, cílovou cestu,
  etapu, stav EXACT/ADAPTED/DEFERRED/REPLACED a důvod odchylky. Neuváděj
  vymyšlená procenta převzatého kódu.
- Neanalyzuj znovu celý repozitář. Čti plán, úzký diff této etapy a její závislosti.
  Celý donorový diff měj uložený mimo kontext, do odpovědí ho nevypisuj.
- Necommituj cizí změny, nemaž uživatelova data, neprováděj reset --hard,
  clean -fd, automatický stash ani force-push. V cizím dirty worktree vytvoř
  oddělený pracovní strom, jestli to prostředí dovoluje.
- Nepushuj a nevytvářej vzdálené PR bez publikačního zadání. Originální PR,
  donorová větev a target branch se nikdy nemění.

## Zachování smluv a bezpečnosti

- CFileData.Name/NameW zůstávají názvy, nikoli plné cesty. Neměň layout CFileData,
  význam PluginData ani existující pořadí/signatury veřejných virtual metod.
  Nové služby získávej přes existující QueryService, pokud ho baseline podporuje.
- Běžné diskové panely fungují v každé etapě. Archivy a plug-in filesystémy si
  zachovají své odlišné virtuální cesty. Diskové API jim nevnucuj.
- Unicode a dlouhé cesty přenes až k I/O, ne pouze do vstupního dialogu.
  Selhání resolveru či limit bufferu znamená explicitní chybu, ne ořezání,
  záměnu souboru ani odhad root + basename. Snapshot výběru není transakce
  nad filesystémem a nezaručuje neměnnost skutečného souboru.
- Uchovej donorové ochrany životnosti, rušení, stale generations, reentrantních
  callbacků a identity souborů. Neodkládej správnost jako „pozdější optimalizaci“.
- Branch View zůstane do publikační etapy uživatelsky vypnuté. Nový kód se ale
  normálně kompiluje; minimální dočasný runtime gate řídí aktivaci, ne kompilaci.
  Musí platit také pro klávesy, obnovu historie a jiné vstupy, ne jen menu.
- V experimentálním režimu nesmí být dostupná neověřená operace. Počáteční panel
  je jen pro čtení. Neznámé plug-in příkazy nezpřístupňuj jen proto, že DLL lze
  načíst. Pro dočasný gate použij nejmenší nutnou ochranu; nevymýšlej nové
  veřejné capability SDK. Odchylky od donoru eviduj a před vydáním vyřeš.

## Ověření a úspora prostředků

- Používej skutečné příkazy z baseline/CI a VALIDATION.md. Nevymýšlej přepínače
  build.ps1 nebo test runneru. Během editací stačí zaměřené testy a přírůstkové
  buildy; před přijetím etapy následuje její závazná validační brána.
- Každý kódový commit musí projít Windows buildem Debug i Release x64:
  hostitel, změněné komponenty a jejich tranzitivní závislosti. U společných SDK,
  hlaviček či build infrastruktury ověř všechny aktuálně podporované konzumenty.
  Dále proveď relevantní nativní testy a smoke běžného panelu/změněné operace.
- Pro každý commit určený k publikaci navíc ověř čistý build celé podporované
  sestavy a povinnou platformní matici projektu, lokálně nebo autorizovaným CI.
  Čisté sestavení bez runtime testu není důkaz funkčnosti; samotný Python
  source-contract test není náhrada nativního buildu nebo GUI testu.
- Testy přebírej z donoru současně s kódem. Neoslabuj assertions. Závisí-li
  budoucí test na nezavedené funkci, odlož jej s přesným důvodem a etapou;
  nikdy ho neoznačuj jako prošlý. Existující regresní testy nevyřazuj.
- Každý test nad soubory používej jen nad vlastním dočasným adresářem s kontrolou
  kořene. Ověř cestu, výsledek/obsah a nedotčenost ostatních zdrojů.
  Výkonnostní a rozsáhlé GUI scénáře spusť až v relevantní etapě, ne při každé editaci.
- Izoluj build/output adresáře podle worktree a konfigurace. Runtime obsahuje
  binárky, resources, DLL a data z právě testované revize, ne přimíchaný donor.
- Chybí-li Windows/MSVC nebo možnost runtime ověření, uveď přesnou blokaci.
  Můžeš připravit označený neověřený patch, ale neprohlašuj etapu za hotovou
  a nezačínej další. Autorizované CI může potřebovat kandidátní commit; ten
  zůstane PENDING, dokud neskončí kontroly pro jeho přesné SHA.
- Dodržuj časové limity a oprávnění prostředí. Neobcházej je skrytými procesy
  či vypnutím sandboxu. Výchozí selhání baseline odděl od nově způsobené regrese.

## Výstup a pokračování

Před závěrem zkontroluj staged diff, git diff --check, změny mimo rozsah a vlastní
cache artefakty. Aktualizuj PROGRESS.md a mapu převzetí, nepřidávej objemné logy
ani binárky do Gitu. Ke commitu přidej:
  Source-PR: KRtkovo-eu-AI/salamander#769
  Source-Commit: skutečné DONOR_SHA
  Port-Step: skutečné ID etapy

V odpovědi uveď: ID etapy a commit; převzaté části; nový adaptační kód a důvod;
skutečně spuštěné příkazy a jejich výsledky; neprovedené kontroly; další
nejmenší etapu. Uchovej výsledky s vazbou na přesnou testovanou revizi.
Nespoléhej na čísla assertions z původního PR ani na historii jiné relace.
