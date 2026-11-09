**Potvrzený smìr**

Za výchozí bod vezmu commit `92243dee`, který už zavádí základní archivní provenance pro #748. Ticket #733 rozšíøíme na obecný model asociací, ale bez okamžitého pøepisu všech runtime konzumentù.

Cílové rozhodnutí:

- jedna nová stránka `File Associations`,
- jeden pattern na øádek, napøíklad `*.txt`, `*.tgz` nebo `*.*`,
- sloupce `View`, `Alternate View`, `Edit` a `Panel Archive`,
- panelové archivy budou mít v detailu dvì operace: `List/Unpack` a `Delete/Pack`,
- každá kategorie/pattern má nejvýše jednoho aktivního vlastníka,
- pøekrývající se patterny jsou konflikt a nový model je nepovolí,
- `Unassigned` bude v UI znamenat explicitnì vypnutou asociaci bez dynamického defaultu,
- `Reset to default` odstraní uživatelský override a aktivuje aktuální pluginový default,
- pluginové defaulty budou dynamická vrstva,
- uživatelské zmìny budou uložené v novém kanonickém modelu,
- staré konfigurace budou zachovány jako `Legacy`,
- staré konflikty se nebudou automaticky opravovat; zobrazí se jako `Legacy Conflict` a uživatel je bude muset explicitnì vyøešit,
- externí Viewery, Editory a panelové archivery budou mít spoleèný preset katalog.

**Backendový návrh**

Navrhnu nový interní modul, pracovnì napøíklad `fileassociations`, který bude obsahovat:

- normalizovaný file pattern,
- kategorii asociace,
- typ cíle,
- stabilní identitu cíle,
- provenance,
- stav `Default`, `User`, `Legacy`, `Disabled` nebo `Conflict`,
- pøípadnì prioritu/pùvodní poøadí kvùli migraci,
- pro panelové archivy dva oddìlené cíle,
- odkaz na externí preset, pokud je cíl externí.

Stabilní pluginové identity nebudou založené na souèasných záporných indexech. Ty zùstanou pouze runtime projekcí. Identita bude složená ze stabilního pluginového identifikátoru a identity handleru nebo labelu. Pøi návrhu se vyhneme zmìnì existujících pluginových vtable rozhraní kvùli pravidlu binární kompatibility SDK.

Nový model bude mít pravdìpodobnì dvì vrstvy:

1. **Dynamická default vrstva**
   - vzniká z aktuálnì naètených pluginù,
   - mùže se mìnit pøi instalaci, odebrání nebo aktualizaci pluginu,
   - není zdrojem uživatelských rozhodnutí.

2. **Persistentní uživatelská vrstva**
   - uložené cíle,
   - explicitní `Unassigned`,
   - preset katalog,
   - provenance a vazby na stabilní identity.

Výsledná efektivní tabulka se bude materializovat do souèasných struktur:

- `ViewerMasks`,
- `AltViewerMasks`,
- `EditorMasks`,
- `CPackerFormatConfig`.

Tím zùstanou stávající runtime èásti funkèní, zatímco nový dialog bude pracovat s jedním kanonickým modelem.

**Migrace**

Migrace bude probíhat ze ètyø souèasných zdrojù:

- `ViewerMasks`,
- `AltViewerMasks`,
- `EditorMasks`,
- `CPackerFormatConfig`.

Pøi migraci:

- semicolon-separated masky se rozloží na samostatné patterny,
- u archivních asociací se použije existující `User Extension List`,
- staré øádky bez provenance se oznaèí jako `Legacy`,
- staré ruèní zmìny, které dnes nelze spolehlivì rozpoznat, se nebudou zpìtnì hádat,
- zachová se pùvodní konfigurace jako fallback/importní zdroj,
- starý runtime zùstane použitelný, dokud uživatel nevyøeší pøípadné konflikty v nové stránce,
- nový kanonický stav se uloží až po validním pøevodu.

Dùležitý pøípad je napøíklad `*.*` souèasnì s `*.txt`. V novém modelu to bude `Legacy Conflict`, nikoliv automaticky vybraná hodnota podle poøadí. Pøi otevøení Configuration uživatel uvidí problém, ale aplikace mu pøi migraci potichu nezmìní chování.

**Preset katalog**

Preset katalog bude muset pokrýt:

- externí Viewery,
- externí Editory,
- externí panelové archivery,
- command,
- arguments,
- initial directory,
- názvy a lokalizované popisky,
- stabilní ID,
- informaci, kde je preset používán.

Pøed smazáním nebo zmìnou presetu bude UI muset zobrazit, ke kterým patternùm je pøipojen. Smazání používaného presetu buï odmítne, nebo nejprve pøevede asociace na `Unassigned`.

**Frontend**

Nová stránka bude navržena jako:

- hlavní matice patternù,
- sloupce pro ètyøi kategorie,
- krátký název cíle v buòce,
- badge provenance/stavu,
- vyhledávání,
- filtrování podle kategorie, provenance a konfliktù,
- detailní panel nebo dialog pro úpravu vybrané buòky,
- hromadné akce pro více patternù,
- akce `Assign`, `Unassigned`, `Reset to default`, `Edit preset` a `Resolve conflict`.

Detail panel bude obsahovat podle typu cíle:

- interní viewer,
- pluginový viewer s názvem handleru,
- externí preset,
- command,
- arguments,
- initial directory,
- pro archivní cíle samostatnì `List/Unpack` a `Delete/Pack`.

Staré stránky se skryjí až po splnìní parity checklistu. Jejich implementace a naèítání ale prozatím zùstanou kvùli migraci, fallbacku a pøípadnému importu starších konfigurací.

Nové zdroje bude nutné doplnit zejména v:

- `src/cfgdlg.h`,
- `src/dialogs4.cpp`,
- `src/dialogs5.cpp`,
- `src/dialogsp.cpp`,
- `src/lang/lang.rc`,
- `src/lang/lang.rh`,
- `src/lang/texts.rc2`.

Souèástí návrhu musí být i:

- dark mode,
- `MS Shell Dlg`,
- lokalizace všech textù,
- klávesové ovládání,
- resize,
- validace pøed commitem,
- pøístupnost a èitelné zobrazení konfliktù.

**Pracovní fáze**

1. Sepsat backendovou specifikaci datových typù, stavù a pravidel konfliktù.
2. Zmapovat pøesné významy všech souèasných registry hodnot a pøipravit migraèní tabulku.
3. Implementovat kanonický model vèetnì save/load a verzování.
4. Implementovat stabilní identity pluginových handlerù a dynamickou default vrstvu.
5. Implementovat migraci starých asociací a zachování `Legacy Conflict`.
6. Implementovat runtime projekci do souèasných struktur.
7. Implementovat preset katalog.
8. Pøidat backendové testy save/load, migrace, plugin reorder/update, provenance, disabled/unassigned a konflikty.
9. Navrhnout resource layout nové stránky.
10. Implementovat jednotnou matici a detailní editor.
11. Pøidat UI contract testy a parity checklist.
12. Teprve potom skrýt staré stránky a ovìøit downgrade/fallback chování.

**První konkrétní krok**

Zaèal bych èistì backendovým návrhem bez zmìny UI:

- definice kanonických typù,
- návrh nového registry/configuration schématu,
- stabilní identity targetù,
- migraèní pravidla pro každý ze ètyø existujících modelù,
- testovací pøíklady pro `*.*`, `*.txt`, `.tgz`, ruèní override, plugin update a `Legacy Conflict`.

Po schválení tohoto datového kontraktu bude možné implementovat backend bez toho, aby se pozdìji musel pøepracovávat kvùli podobì matice.