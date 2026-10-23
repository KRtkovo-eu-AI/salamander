# S27a2 — bezpečné oddělení programů a dokumentů

## Výsledek a změna akceptačního rozsahu

Dokončeno lokálně podle výslovného zadání uživatele z 2026-10-23: zachovat
výchozí Windows, bezpečnostní kontroly a dokončit s popsanými nepodporovanými
případy. Starší požadavek na úspěšné Shell spuštění každé dlouhé programové
cesty je tím nahrazen. Dlouhé programové cesty nejsou prohlášeny za vyřešené.
Předchozí výzkumné protokoly zůstávají zachované jako historické důkazy.

Výchozí zdroj tohoto pokračování: 320ed0996857e5c2eec7f72156901989b20f9549.
Donor: 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, PR #769.
Evidence mimo Git: ../evidence/S27a2-final, včetně runtime.patch,
*-proof.json, stdout/stderr, GUI fixture a ověření skutečného zdroje/CWD.

## Chování

- Ověřené PE programy s příponou .exe mají samostatnou programovou větev.
  Host vytvoří pouze vlastní utils/salopen.exe s přesným pracovním adresářem.
  Soukromý verzovaný požadavek přenáší vlastněnou úplnou UTF16 cestu, HWND
  vlastníka a výsledek. Starý pomocník příkaz odmítne před interpretací souboru.
- Pomocník ověří PE typ, skutečný CWD proti rodiči pomocí file ID a Windows
  klasifikaci původu se MUTZ_ISFILE | MUTZ_REQUIRESAVEDFILECHECK. Vyžaduje
  Local Machine; Internet/Restricted/unknown a chyby odmítne bez náhradní cesty.
- IAttachmentExecute dostane skutečnou existující cestu a stabilní identitu
  klienta. CheckPolicy musí vrátit S_OK. Vlastní spuštění provádí Execute,
  takže závěrečné ověření, skenery a případné systémové UI vlastní Windows.
  Samotné CheckPolicy nikdy neopravňuje raw CreateProcess cílového programu.
- Dokumenty, skripty, odkazy a instalátory zůstávají u Shell asociací. Dlouhé
  Shell cesty jsou v této etapě explicitně nepodporované. Archivy používají
  samostatnou existující cestu s již ověřenou širokou metadata předkontrolou.
- Po odmítnutí se nic nespouští jiným mechanismem. Neexistuje kopírování či
  přejmenování cíle, aliasová náhradní cesta, PowerShell fallback, odstranění
  MOTW, SEE_MASK_NOZONECHECKS ani změna systému. Historie vzniká jen po úspěchu.
- Čekání na pomocníka je omezené na 30 s, obsluhuje synchronní zprávy a kreslení
  vlastníka; nové panelové příkazy nepouští. Timeout ukončí vlastní pomocník
  a nevede k opakování požadavku. Úspěch znamená vrácený procesní handle,
  nikoli záruku úspěšného dokončení cílového programu.

## Omezení

Testované skutečně dlouhé .exe cesty Windows klasifikoval jako Restricted;
vracíme 0x800704EC a nespouštíme je. Hodnota LongPathsEnabled sama o sobě toto
odmítnutí nevysvětluje a její zapnutí nezaručuje změnu. Zapnutý stav nebyl na
skutečném systému testován; rozlišení stavů registru ověřují nativní testy.

Programová větev přijímá pouze absolutní kanonické DOS cesty .exe a podporované
PE 32/64. UNC, relativní/device/ADS cesty, URL-citlivé znaky % a #, komponenty
končící tečkou/mezerou a nepodporované binární typy končí odmítnutím. Nejde
o obecný launcher všech spustitelných formátů. Požadavek na potvrzení vyjádřený
CheckPolicy bez S_OK je v tomto konzervativním rozsahu odmítnut. Závěrečné
Execute může stále vyvolat potřebné Windows UI; nikdy je automaticky nepotvrzujeme.

Vstupní read lock se uvolní před závěrečným Execute, aby Windows mohl soubor
ověřit, označit či přesunout do karantény. Nejde o atomickou transakci nad
filesystémem. Aplikace neupravuje původní bezpečnostní metadata; systémové trust
služby mají svou standardní pravomoc. Test s Internet Zone.Identifier dokládá
odmítnutí a bajtově nezměněný stream v tomto konkrétním scénáři.

Změna aktivuje jen již experimentálně povolené Branch Enter. Běžné panely,
SDK prefixy a ostatní operace se nemění. Branch View zůstává standardně vypnutý.
Další nejmenší krok je samostatně plánované S27b; tímto nezačalo.

## Hláška a návod

Po chybě dlouhé cesty se pouze čte nativní pohled HKLM FileSystem /
LongPathsEnabled. Rozlišujeme disabled, enabled, absent a unknown včetně
neplatného typu či hodnoty. Enabled připomíná procesní cache. Z hodnoty se
neodvozuje oprávnění spouštět ani příčina chyby.

Dialog obsahuje plnou cestu, HRESULT a stav. Výchozí Zavřít funguje i přes Enter;
jen výslovná volba návodu ukáže místní informační dialog s cestou ke skupinové
politice a ekvivalentem registru. Salamander nic nemění. Návod zmiňuje potřebná
oprávnění správce, restart procesů/případně Windows a podporu cílové aplikace.

Texty jsou v resources, s českým překladem a anglickým zdrojem. Všech 11 SLT
obsahuje nové záznamy; dalších 10 jazyků má explicitně nepřeloženou anglickou
verzi (stav 0), která čeká na lokalizační etapu. Starší SLG bez nových ID použije
anglický resource v hostiteli. Úplný překlad všech jazyků se netvrdí. Dialog
používá stávající SalMessageBoxEx a podporuje experimentální tmavé schéma.

## Skutečně provedené ověření

MSBuild src/vcxproj/salamand.vcxproj /m:1 /t:Build /p:Platform=x64
/p:Configuration=Debug nebo Release /warnaserror /nr:false /v:minimal,
s izolovaným OPENSAL_BUILD_DIR=build/pr769-baseline-v145 a v145.
Pro Debug resources bylo po interním selhání inkrementálního linkeru použito
samostatné lang.vcxproj /p:LinkIncremental=false; salopen sestaven samostatně,
host pak /p:BuildProjectReferences=false. Závislosti tedy nebyly vynechány.
Release běžel s referencemi. Kvůli existujícímu Release postbuild kopírování
pomocníka do obou konfigurací byl Debug salopen závěrem znovu Rebuildnut.
Projektová/CI pravidla ani varování se neoslabovala. Binární hashe v evidenci.

Nativní příkazy (Python -B, všechny s --configuration Both):

- src/tests/branch_program_launch_tests.py: **93 kontrol/config, 0 chyb**.
  Kompiluje skutečný salopen.cpp, jeho resource a manifest, pak ověřuje skutečné
  spuštění pro ASCII, Unicode basename, více Unicode předků, UTF8-byte-long
  cestu a mezery, skutečné module/CWD file ID. Dlouhá UTF16 cesta má úspěšně
  ověřené odmítnutí, nikoli úspěšné spuštění. Dále MOTW integrita, chybný CWD,
  neexistující a falešný PE, poškozený paket a simulované registry bez zápisu.
- src/tests/branch_panel_getters_tests.py: **186/config, 0 chyb**. Zachováno
  předchozích 181 kontrol; nové ověřují programovou větev před Shell a historii
  i vlastnictví po reentrantním zničení metadat. Dispatch rekordér není důkaz
  skutečného Shell spuštění; to je odděleno od I/O a GUI důkazů.
- src/tests/panel_item_paths_tests.py: **196/config, 0 chyb**, 5 nezměněných SDK
  prefixů. Běžné panely a služba cest regresně ověřeny.
- Všech **28 registrovaných zdrojových/menu skupin** přes shodný runner shard
  bez native-generated skupin: PASS. Nová registrace programového testu v runneru
  zůstala zachována; skutečný nativní test běžel samostatně výše.

Skutečné host GUI: Debug v Windows Dark Mode (experimental), Release Light.
Oba spustily vybraný krátký PE z Branch, ověřen skutečný modul a fyzický CWD.
Oba dlouhý PE odmítly bez přepsání původního capture výsledku; zůstalo 7 položek.
Tmavý návod i světlé výchozí Zavřít byly zkontrolovány. Dlouhé ZIP listing,
refresh a skutečná extrakce/view unikátního markeru byly ověřeny v Debug,
i Release: unikátní markery Debug/Release unicode exact source marker. Default-off gate nebyl měněn a ověřují jej
kontrakty; nové default-off GUI se v tomto pokračování neopakovalo.

Debug při ukončení opět zobrazil Heap Message / Detected memory leaks. Dialog
potvrzen, potom žádný vlastní host, reporter ani nový report. Bez alokačního
profilu netvrdíme příčinu ani že jde o prokázanou baseline chybu. Tato diagnostika
zůstává známým nevyřešeným omezením ověření; není potichu započtena jako PASS
čistého shutdownu. Release se ukončil bez varování, host/reporter nezůstaly a nevznikl report.
Všech 14 nových vstupních hashů souhlasí; oba soukromé configstorage.ini byly
obnoveny bajtově. Instalovaný uživatelský host zůstal nedotčen.

Publikace vyžaduje samostatný čistý build celé podporované
sestavy a aktuální CI/CodeQL; v tomto lokálním kroku neproběhly.

## Původ

Donorový Execute/archive tok je adaptovaný z PR #769; nové soukromé rozhraní,
Windows broker a diagnostika jsou místní integrační oprava. Koncept oddělení
programů/dokumentů vychází z dříve připnutého srovnání FAR/Files. Žádný jejich
produkční zdroj ani fallback řetězec nebyl importován. Kredit je v third_party.md
a mapa převzetí v REUSE.tsv. Zdrojové piny ani původní PR se nemění.
