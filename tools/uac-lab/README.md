<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-07 | Document the UAC bootstrap experiment. | Make the P0 findings reproducible. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Record interactive UAC test results. | Verify the broker bootstrap in an isolated Windows VM. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Record disconnect and Shell experiments. | Document reproduction and measured outcomes for P0. | | GPT-6.1 Sol (Codex) |
-->

# P0 — ověření UAC bootstrapu

Host vytvoří lokální pojmenovanou rouru a spustí druhou instanci laboratorního EXE. Klient ověří PID serveru proti předanému PID, drží procesový handle a kontroluje stejnou Windows session. Host ověří PID klienta proti živému procesu, jehož handle vrátilo spuštění, a potom načte značku protokolu. Z tokenu na úrovni SecurityIdentification zjistí elevaci a shodu účtů. Výměna končí potvrzením nebo chybou. Protokol obsahuje jen pevně velké značky, žádné souborové příkazy.

DACL roury je D:P(A;;0x00100083;;;BA)(A;;GA;;;<host-user-sid>). ACE pro Administrators dává práva číst a zapisovat data, číst atributy a synchronizovat se. ACE hostu umožňuje vytvořit instanci. Roura odmítá vzdálené klienty a dovoluje jedinou instanci. Host váže spojení na handle z vlastního spuštění. Klient ověřuje spojení s procesem označeným v argumentu, což samo neprokazuje důvěryhodný původ záměru. Autorizace produkčních příkazů vyžaduje vlastní návrh. [Práva pojmenovaných rour](https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-security-and-access-rights) · [Úrovně impersonace](https://learn.microsoft.com/en-us/windows/win32/secauthz/impersonation-levels)

## Sestavení a ověření

Z kořene checkoutu v x64 Developer PowerShell pro Visual Studio:

~~~
cl.exe /nologo /W4 /WX /std:c++17 /EHsc /MT /utf-8 /Fo:"$env:TEMP\uac-bootstrap-lab.obj" /Fe:"$env:TEMP\uac-bootstrap-lab.exe" tools\uac-lab\uac-bootstrap-lab.cpp /link /MANIFEST:EMBED advapi32.lib ole32.lib shell32.lib
& "$env:TEMP\uac-bootstrap-lab.exe" --local
& "$env:TEMP\uac-bootstrap-lab.exe" --reject-peer
& "$env:TEMP\uac-bootstrap-lab.exe" --reject-server
~~~

| Scénář | Očekávaný výsledek | Ověření |
| --- | --- | --- |
| Běžný proces a klient pod stejným účtem | Úspěšná výměna, elevated „no“, same user „yes“. | Prošlo na x64 i Win32 s MSVC 14.51, varování jako chyby. |
| Nesprávný klient | Odmítnutí před čtením zprávy. Test záměrně použije nesprávné očekávané PID. | Prošlo. |
| Nesprávný server | Odmítnutí před odesláním zprávy. Klient v testu očekává vlastní PID. | Prošlo. |
| Split token správce, schválení UAC | Elevated „yes“, same user „yes“. | Prošlo. Prototyp potvrdil zvýšený token stejného uživatele. |
| Standardní účet, přihlášení jiného správce | Elevated „yes“, same user „no“, návratový kód 0. | Prošlo po doplnění práva FILE_READ_ATTRIBUTES. |
| Zrušení UAC | Chyba 1223 při spuštění a uzavření roury. | Prošlo. Po volbě Ne prototyp vypsal chybu Windows 1223 (ERROR_CANCELLED). |

Scénáře UAC spusťte z běžné konzole pomocí volby `--runas` v oddělené testovací VM se snapshotem. Před pokusem správce uloží ověřené laboratorní EXE do adresáře, do něhož běžný účet nemá zápis. Spusťte jej z této absolutní cesty. Kopie v dočasném adresáři je určena pro tři lokální scénáře. Nepodepsané laboratorní artefakty jsou určené pro tuto VM. [ShellExecuteExW](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecuteexw)

V obou sestavených EXE byl pomocí mt.exe ověřen vložený manifest asInvoker a uiAccess=false.

Interaktivní běhy proběhly na vyhrazené VM se snapshotem, Windows 11 Pro x64 build 26200 a zapnutou zabezpečenou plochou UAC. První x64 binárka pro schválení a zrušení pochází z commitu f48cf5e1. Hash každé ověřované kopie se shodoval s odpovídajícím lokálním sestavením. Kopie v Program Files byla ve vlastnictví Administrators a standardní uživatelé měli oprávnění pouze ke čtení a spuštění.

Při prvním běhu pod standardním účtem vypršelo čekání hostu na připojení (1460). Diagnostická varianta se stejnou DACL zaznamenala chybu 5 (ERROR_ACCESS_DENIED) při otevírání roury pomocným procesem. Po přidání FILE_READ_ATTRIBUTES do požadavku klienta i do ACE skupiny Administrators (0x00100083) prošel celý handshake se zvýšeným tokenem jiného uživatele. Návratový kód byl 0. Maska neobsahuje FILE_CREATE_PIPE_INSTANCE. Lokální výměna a oba negativní testy po změně prošly na x64 i Win32. [Práva pojmenovaných rour](https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-security-and-access-rights)

## Ztráta spojení a srovnání se Shellem

Lokální scénáře brokeru spouští volby --drop-host a --drop-helper. Po ověřeném hello uzavře spojení host před potvrzením, nebo klient po přijetí potvrzení před odesláním testovacího výsledku. Obě varianty prošly na x64 i Win32. Volby --runas-drop-host a --runas-drop-helper provedou stejné testy se zvýšeným klientem z chráněného umístění. Oba interaktivní testy prošly. Při odpojení klienta host zaznamenal chybu 109. Při odpojení hostu čekal na ukončení klienta a ověřil jeho diagnostický kód. Návratový kód 0 označuje úspěch testovacího očekávání, nikoli úspěch přerušené operace.

Pro opakování Shell pokusu sestavte z kořene checkoutu v x64 Developer PowerShell:

~~~
cl.exe /nologo /W4 /WX /std:c++17 /EHsc /MT /utf-8 /Fo:"$env:TEMP\uac-shell-lab.obj" /Fe:"$env:TEMP\uac-shell-lab.exe" tools\uac-lab\uac-shell-lab.cpp /link /MANIFEST:EMBED ole32.lib shell32.lib uuid.lib
~~~

V testovací VM musí předem existovat chráněný adresář Samandarin-UacLab v Program Files. V běžném PowerShellu spusťte Shell sondu s volbami --control, --cancel a --approve. V režimu --control sonda nenastaví povolení výzvy. Zbývající dvě volby používají stejné flags, mají odlišný prefix názvu a požadují ruční volbu Ne, respektive Ano. Každý běh generuje nový název a vypíše výsledek zařazení, provedení, stav přerušení, výsledek položky a pozorovanou existenci cíle. Úspěšný pokus zanechá jednu prázdnou složku. Vyhodnocení je v [plánu, část 6.1](../../doc/uac-implementation-plan.md#61-výsledky-p0-a-volba-backendu). [Výsledek položky PostNewItem](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ifileoperationprogresssink-postnewitem)

## Rozhodnutí pro další řez

- **Backend.** Pro nativní engine je zvolen samostatný EXE. IFileOperation zůstává pro Koš a samostatně ověřené shellové úkony. První pilot je omezený na chráněnou instalaci Windows 11 x64 na lokálním NTFS. Podpora dalších verzí OS, architektur a portable vyžaduje samostatné ověření.
- **Identita cíle.** V lokálním x64 pokusu byl otevřen handle cílového rodiče s právem sdílet smazání. Rodič byl přejmenován a pod původním jménem vznikl jiný adresář. Volání SetFileInformationByHandle s FileRenameInfo, relativním názvem a RootDirectory vrátilo ERROR_INVALID_PARAMETER (87), i s větším bufferem. Kontrolní běh s absolutní cestou k přejmenovanému rodiči a nulovým RootDirectory uspěl. Relativní varianta je popsaná v dokumentaci. Před privilegovaným přejmenováním musí samostatný test prokázat vazbu mutace na ověřeného rodiče i při jeho výměně. [FILE_RENAME_INFO](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_rename_info)
- **První příkaz.** V P1 oddělit lokální příkaz pro vytvoření složky pod existujícím rodičem a zachovat stávající chování F7. Budoucí elevaci omezit na jednu koncovou složku, odmítnout kolizi a nepodporované reparse případy. Přejmenování bez přepisu přidat samostatně po ověření identity obou stran. Výsledek rozliší provedení, přeskočení, selhání beze změny, částečnou změnu a neznámý výsledek.
- **Distribuce.** P3 pilot bez distribučního ověření patří do izolované VM. Před použitím mimo laboratoř ověřit manifesty, architekturu, podpis, instalaci a pár verzí host/helper. Samostatné změny XML manifestu nebo Inno Setup skriptu mohou projít současným filtrem CI bez sestavení. Kontrolu artefaktů je potřeba spouštět i pro tyto změny.
