# Reprodukovatelná sada cest a identity

S00 definuje sadu a místa převzetí. **Žádné testovací soubory ani nový testovací
framework se v S00 nevytvářejí.** Donorové testy nejsou součástí targetu a jejich
dřívější výsledky se nevykazují jako PASS portu. Při příslušné etapě převzít
existující fixture společně s produkčním kódem a registrací runneru.

## Existující zdroje v připnutém donoru

| Etapa | Soubor a existující fixture | Použití |
| --- | --- | --- |
| S01/S03 | `src/tests/safe_file_paths_tests.cpp.in`: `Fixtures::Owned`, `Directory`, `File`, `RoundTrip`, `InjectRead`, `InjectWrite` | Skutečné create/open/overwrite/Retry a kontrola obsahu/offsetu; široké cesty až S03 |
| S07 | `src/tests/renamer_io_tests.cpp.in`: `Fixture`, `FileCase` | Rozlišení UTF-8 >260 B při UTF-16 <260 od skutečného UTF-16 >260; move/Undo/collision |
| S08 | `src/tests/checksum_selection_paths_tests.cpp.in`: `Fixtures::File`, `Populate`, `RunPending` | `same.bin` v různých rodičích s odlišným obsahem; Calculate/Verify a lifetime po zániku panelu |
| S25 | `src/tests/branch_view_tests.cpp` | Rekurzivní strom, duplicity, batching, stop, náhradní scan, cyklus a reparse policy |

Příslušné generující `.py` soubory vyjmou skutečnou implementaci a sestaví
nativní harness. Nejsou to samotné source-contract testy. Před jejich během
zkontrolovat závislosti; donorový celek nelze spustit nad targetem bez portu.
Podrobné názvy nativních projektů a registrací jsou ve stage dokumentech a REUSE.

## Povinné případy a invarianty

Každý test vytvoří jedinečný vlastní kořen pomocí existujícího fixture pod
systémovým dočasným adresářem. Nepracovat nad uživatelovými dokumenty ani
předem existujícím stromem. Normalizovat absolutní kořen, uložit jeho vlastnictví,
před zápisem/úklidem potvrdit, že každý cíl zůstává pod ním. Před rekurzivním
úklidem ověřit přesný absolutní kořen a odmítnout prázdnou cestu, diskový kořen
i nadřazené adresáře. Reparse body nesmějí přesměrovat úklid mimo fixture.

| ID | Vstup vytvořený až testem | Povinná kontrola |
| --- | --- | --- |
| F01 | ASCII jméno a ASCII rodič pod 260 jednotek | Stejná úplná cesta, očekávané bajty, základní I/O |
| F02 | České/CJK jméno, ASCII rodič | Přesné UTF-16 jméno a obsah, bez ACP náhrady |
| F03 | ASCII jméno, český/CJK rodič | Úplný skutečný rodič až do I/O |
| F04 | Několik českých/CJK komponent, případně emoji | Žádné rozdělení UTF-8 sekvence/surrogate ani zkrácení |
| F05 | `one/same.bin`, `two/same.bin`, třetí stejné basename | Rozdílné bajty a hashe; focus/selection/operace pracuje s celou identitou |
| F06 | CJK komponenta, např. donorových 85 znaků `漢` | Výslovně assert UTF-8 počet bajtů >260 a UTF-16 počet jednotek <260 |
| F07 | Více platných komponent; donor SafeFile používá hluboký 12komponentový strom | Výslovně assert skutečný UTF-16 počet jednotek >260; wide extended prefix až k I/O |
| F08 | Stejný basename a jiné obsahy v F01/F03/F07 | Hash vybraného zdroje odpovídá výsledku; ostatní zdroje beze změny |
| F09 | Vlastní oddělený sentinel pod stejným dočasným kořenem | Před/po selhání, Cancel či Skip stejná cesta, velikost, hash a obsah |

Počty znaků jsou měřené na skutečné výsledné absolutní cestě, nikoli pouze na
jedné komponentě. `std::wstring::size()` na Windows počítá UTF-16 jednotky;
UTF-8 velikost měřit po explicitním převodu, nikoli počtem zobrazovaných znaků.
Pokud okolní temp cesta znemožní F06 <260, fixture zvolí kratší vlastní kořen
nebo případ explicitně označí jako neprovedený; nesmí jej zaměnit za F07.

## Obsah, hashe a opakování

Existující Checksum fixture používá u stejných basename odlišné payloady
(1/2/3 bajty: `x`, `xx`, `xxx`). Při portu převzít tyto bajty i očekávání z donoru;
ověřit výsledek Calculate/Verify, původní obsah i nedotčenost sourozenců.
Harness má mockované hash/GUI hranice; sám nedokazuje skutečný checksum
algoritmus ani plug-in GUI. Tyto výsledky ověřit samostatně v nativním runtime.
Kde stávající fixture kontroluje pouze obsah, doplnit v příslušné testovací
etapě manifest `relative path -> size -> SHA-256` pro vstupy, výsledky a sentinel.
Žádný nový sdílený framework není nutný. SHA-256 lze nezávisle ověřit existujícím
Python runtime přes `hashlib.sha256(path.read_bytes()).hexdigest()`.

Každý další běh vytvoří nový temp root; pořadí výčtu nesmí určovat identitu.
Po copy/move/rename/split/combine porovnat správné výsledné bajty/hash;
u přesunu přidat očekávanou neexistenci původní cesty. Při Cancel/failure
kontrolovat i ostatní zdroje a temporary outputs. Snapshot není transakce:
race, hardlink a záměna souboru mají vlastní očekávané explicitní selhání.

## Rozsah po etapách

- S01 pouze ASCII SafeFile Retry/Skip a chybové větve. Donorový `main()`
  bez úpravy vyžaduje `SafeFileRootLength` z wide etapy S03. Je nutná minimální
  adaptace harnessu; neoslabovat assertions již převzatého chování.
- S03 rozšíří stejný harness o F02–F07. Donor sám není důkaz pokrytí všech
  read-only/DOS alias/Read+Write Skip All scénářů požadovaných promptem;
  chybějící případy dodat a vykázat až tehdy.
- S04/S05 testují ownership/resolver/ABI i se syntetickými řádky; to není
  důkaz skutečné operace nad F05/F08.
- S06–S22 ověřují skutečné příkazy běžného panelu a operace nad příslušnou
  podmnožinou. GUI evidence zvlášť od nativního harnessu.
- S25 vytvoří rozsáhlejší scanner fixture až vlastním testem. S26/S27 používají
  tentýž korpus ve skutečném experimentálním panelu včetně duplicate basename.
- Výkon/refresh storms patří S30. Žádná náhrada těchto testů počty assertions
  nebo měřením z původního PR.

Závazná brána každé etapy: Debug i Release x64, odpovídající nativní testy,
normal-panel smoke a konkrétní operace. Příkazy a pravidla evidence jsou ve
[VALIDATION.md](VALIDATION.md); nynější baseline je BLOCKED.
