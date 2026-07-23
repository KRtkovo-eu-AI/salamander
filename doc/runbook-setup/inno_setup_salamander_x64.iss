; Inno Setup script for Open Salamander Samandarin x64.
;
; This file is intentionally kept in the runbook folder next to setup_x64.inf.
; It mirrors the file list, shortcut locations, registry keys and private setup
; metadata from doc/runbook-setup/setup_x64.inf as closely as Inno Setup allows.
;
; Build example:
;   iscc.exe "doc\runbook-setup\inno_setup_salamander_x64.iss" /DPayloadDir="H:\_projects\salamander\output\salamander\Release_x64"
;
; Silent install example:
;   5.0-samandarin-0.13_win_x64.exe /VERYSILENT /SUPPRESSMSGBOXES /DIR="C:\Path\To\Install" /NORESTART /LANG=czech /INSTALLMODE=install /OVERWRITEDIR=yes /PLUGINS=default /SHORTCUTS=startmenu,desktop
;
; Silent automation parameters:
;   /LANG=<language>                 Inno Setup built-in language selector (for example czech or english).
;   /INSTALLMODE=<install|portable>  Mirrors the UI installation mode radio buttons.
;   /OVERWRITEDIR=<yes|no>           Silent answer for an already existing destination directory; no aborts before copying.
;   /PLUGINS=<default|all|none|id,..> Mirrors the plugin selection page.
;   /SHORTCUTS=<none|startmenu|desktop|startmenu,desktop> Mirrors the shortcuts tasks; ignored for portable mode.
;   /RUNAPP=<yes|no>                 Controls launching the app after install; defaults to no during silent setup.
;
; Custom [Code] message boxes are guarded with WizardSilent/UninstallSilent so
; unattended installs and uninstalls do not block on application prompts.
;
; If /DPayloadDir is not supplied, OPENSAL_BUILD_DIR is used when present; otherwise
; the fallback path below is relative to this .iss file.

#define AppToInstallName "Open Salamander Samandarin"
#define SamandarinVersion "0.13"
#define AppToInstallDisplayName "Open Salamander 5.0 Samandarin " + SamandarinVersion + " (x64)"
#define AppToInstallVersion "5.0-samandarin-" + SamandarinVersion
#define AppToInstallPublisher "Ondřej Kotas (KRtekTM)"
#define AppToInstallCopyright "© 2026 " + AppToInstallPublisher + " KRtkovo.eu"
#define AppToInstallURL "https://samandarin.krtkovo.eu/"
#define AppToInstallExeName "salamand.exe"
#define AppToInstallRegPath = "'Software\Open Salamander Samandarin\5.0-samandarin-" + SamandarinVersion + "'"
#define AppToInstallRegPathRem = "'HKCU\Software\Open Salamander Samandarin\5.0-samandarin-" + SamandarinVersion + "'"
#ifndef PayloadDir
  #if GetEnv("OPENSAL_BUILD_DIR") != ""
    #define PayloadDir AddBackslash(GetEnv("OPENSAL_BUILD_DIR")) + "salamander\Release_x64"
  #else
    #define PayloadDir "..\..\output\salamander\Release_x64"
  #endif
#endif

[Setup]
AppId=OpenSalamanderSamandarin-x64
AppCopyright=AppToInstallCopyright
AppName={#AppToInstallName}
AppVersion={#AppToInstallVersion}
AppVerName={#AppToInstallDisplayName}
AppPublisher={#AppToInstallPublisher}
AppPublisherURL={#AppToInstallURL}
AppSupportURL={#AppToInstallURL}
AppUpdatesURL={#AppToInstallURL}
AppendDefaultDirName=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma2/ultra64
ChangesAssociations=yes
CreateUninstallRegKey=yes
DefaultDirName={code:GetDefaultDirName}
DefaultGroupName={#AppToInstallName}
DisableDirPage=no
DisableProgramGroupPage=yes
DisableFinishedPage=yes
OutputBaseFilename={#AppToInstallVersion}_win_x64
PrivilegesRequired=admin
SetupArchitecture=x64
SetupIconFile=..\..\src\res\samandarin.ico
SolidCompression=yes
Uninstallable=not IsPortableInstall
UninstallDisplayName={#AppToInstallDisplayName}
UninstallDisplayIcon={app}\salamand.exe
UsePreviousAppDir=no
VersionInfoVersion=5.0.{#SamandarinVersion}
WizardSmallImageFile={#SourcePath}\setup_img_small.png
WizardSmallImageFileDynamicDark={#SourcePath}\setup_img_small.png
WizardStyle=modern dynamic


[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"; LicenseFile: "{#SourcePath}\license.chinesesimplified.txt"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"; LicenseFile: "{#SourcePath}\license.czech.txt"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"; LicenseFile: "{#SourcePath}\license.dutch.txt"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"; LicenseFile: "{#SourcePath}\license.french.txt"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"; LicenseFile: "{#SourcePath}\license.german.txt"
Name: "hungarian"; MessagesFile: "compiler:Languages\Hungarian.isl"; LicenseFile: "{#SourcePath}\license.hungarian.txt"
Name: "romanian"; MessagesFile: "compiler:Languages\Romanian.isl"; LicenseFile: "{#SourcePath}\license.romanian.txt"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"; LicenseFile: "{#SourcePath}\license.russian.txt"
Name: "slovak"; MessagesFile: "compiler:Languages\Slovak.isl"; LicenseFile: "{#SourcePath}\license.slovak.txt"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"; LicenseFile: "{#SourcePath}\license.spanish.txt"

[CustomMessages]
english.RemoveUserConfigQuestion=Do you want to remove the Open Salamander Samandarin user configuration?
english.FileStorage=File storage:
english.RegistryKey=Registry key:
english.RemoveUserConfigFiles=Choose Yes to delete the configuration files, or No to keep your settings.
english.RemoveUserConfigRegistry=Choose Yes to delete the key including all contents, or No to keep your settings.
chinesesimplified.RemoveUserConfigQuestion=是否要删除 Open Salamander Samandarin 用户配置？
chinesesimplified.FileStorage=文件存储:
chinesesimplified.RegistryKey=注册表项:
chinesesimplified.RemoveUserConfigFiles=选择“是”删除配置文件，选择“否”保留您的设置。
chinesesimplified.RemoveUserConfigRegistry=选择“是”删除该项及其所有内容，选择“否”保留您的设置。
czech.RemoveUserConfigQuestion=Chcete odstranit uživatelskou konfiguraci Open Salamander Samandarin?
czech.FileStorage=Úložiště souborů:
czech.RegistryKey=Klíč registru:
czech.RemoveUserConfigFiles=Zvolte Ano pro odstranění konfiguračních souborů, nebo Ne pro zachování nastavení.
czech.RemoveUserConfigRegistry=Zvolte Ano pro odstranění klíče včetně celého obsahu, nebo Ne pro zachování nastavení.
dutch.RemoveUserConfigQuestion=Wilt u de gebruikersconfiguratie van Open Salamander Samandarin verwijderen?
dutch.FileStorage=Bestandsopslag:
dutch.RegistryKey=Registersleutel:
dutch.RemoveUserConfigFiles=Kies Ja om de configuratiebestanden te verwijderen, of Nee om uw instellingen te behouden.
dutch.RemoveUserConfigRegistry=Kies Ja om de sleutel inclusief alle inhoud te verwijderen, of Nee om uw instellingen te behouden.
french.RemoveUserConfigQuestion=Voulez-vous supprimer la configuration utilisateur d’Open Salamander Samandarin ?
french.FileStorage=Stockage des fichiers :
french.RegistryKey=Clé de registre :
french.RemoveUserConfigFiles=Choisissez Oui pour supprimer les fichiers de configuration, ou Non pour conserver vos paramètres.
french.RemoveUserConfigRegistry=Choisissez Oui pour supprimer la clé avec tout son contenu, ou Non pour conserver vos paramètres.
german.RemoveUserConfigQuestion=Möchten Sie die Benutzerkonfiguration von Open Salamander Samandarin entfernen?
german.FileStorage=Dateispeicher:
german.RegistryKey=Registrierungsschlüssel:
german.RemoveUserConfigFiles=Wählen Sie Ja, um die Konfigurationsdateien zu löschen, oder Nein, um Ihre Einstellungen beizubehalten.
german.RemoveUserConfigRegistry=Wählen Sie Ja, um den Schlüssel einschließlich aller Inhalte zu löschen, oder Nein, um Ihre Einstellungen beizubehalten.
hungarian.RemoveUserConfigQuestion=Szeretné eltávolítani az Open Salamander Samandarin felhasználói konfigurációját?
hungarian.FileStorage=Fájltároló:
hungarian.RegistryKey=Beállításkulcs:
hungarian.RemoveUserConfigFiles=Válassza az Igen lehetőséget a konfigurációs fájlok törléséhez, vagy a Nem lehetőséget a beállítások megtartásához.
hungarian.RemoveUserConfigRegistry=Válassza az Igen lehetőséget a kulcs és teljes tartalmának törléséhez, vagy a Nem lehetőséget a beállítások megtartásához.
romanian.RemoveUserConfigQuestion=Doriți să eliminați configurația de utilizator Open Salamander Samandarin?
romanian.FileStorage=Stocare în fișiere:
romanian.RegistryKey=Cheie de registru:
romanian.RemoveUserConfigFiles=Alegeți Da pentru a șterge fișierele de configurare sau Nu pentru a păstra setările.
romanian.RemoveUserConfigRegistry=Alegeți Da pentru a șterge cheia, inclusiv tot conținutul, sau Nu pentru a păstra setările.
russian.RemoveUserConfigQuestion=Удалить пользовательскую конфигурацию Open Salamander Samandarin?
russian.FileStorage=Файловое хранилище:
russian.RegistryKey=Раздел реестра:
russian.RemoveUserConfigFiles=Выберите «Да», чтобы удалить файлы конфигурации, или «Нет», чтобы сохранить настройки.
russian.RemoveUserConfigRegistry=Выберите «Да», чтобы удалить раздел со всем содержимым, или «Нет», чтобы сохранить настройки.
slovak.RemoveUserConfigQuestion=Chcete odstrániť používateľskú konfiguráciu Open Salamander Samandarin?
slovak.FileStorage=Úložisko súborov:
slovak.RegistryKey=Kľúč registra:
slovak.RemoveUserConfigFiles=Zvoľte Áno na odstránenie konfiguračných súborov alebo Nie na zachovanie nastavení.
slovak.RemoveUserConfigRegistry=Zvoľte Áno na odstránenie kľúča vrátane celého obsahu alebo Nie na zachovanie nastavení.
spanish.RemoveUserConfigQuestion=¿Desea eliminar la configuración de usuario de Open Salamander Samandarin?
spanish.FileStorage=Almacenamiento de archivos:
spanish.RegistryKey=Clave del Registro:
spanish.RemoveUserConfigFiles=Elija Sí para eliminar los archivos de configuración, o No para conservar la configuración.
spanish.RemoveUserConfigRegistry=Elija Sí para eliminar la clave con todo su contenido, o No para conservar la configuración.
english.InstallMode=Installation mode:
english.StandardInstall=Install normally (Start Menu, Programs and uninstaller)
english.PortableInstall=Extract as a portable version (no Programs entry or uninstaller)
english.PortableDirName=samandarin
english.StartMenuShortcut=Create a &Start Menu shortcut
english.DesktopShortcut=Create a &desktop shortcut
english.Shortcuts=Shortcuts:
english.LaunchProgram=Launch {#AppToInstallName}
chinesesimplified.InstallMode=安装模式:
chinesesimplified.StandardInstall=正常安装 (开始菜单, 程序和卸载程序)
chinesesimplified.PortableInstall=提取为便携版本 (无程序条目或卸载程序)
chinesesimplified.PortableDirName=samandarin
chinesesimplified.StartMenuShortcut=创建“开始”菜单快捷方式(&S)
chinesesimplified.DesktopShortcut=创建桌面快捷方式(&D)
chinesesimplified.Shortcuts=快捷方式:
chinesesimplified.LaunchProgram=启动 {#AppToInstallName}
czech.InstallMode=Režim instalace:
czech.StandardInstall=Opravdová instalace (nabídka Start, Programy a odinstalátor)
czech.PortableInstall=Jen rozbalit jako portable verzi (bez záznamu v Programech a odinstalátoru)
czech.PortableDirName=samandarin
czech.StartMenuShortcut=Vytvořit zástupce v nabídce &Start
czech.DesktopShortcut=Vytvořit zástupce na &ploše
czech.Shortcuts=Zástupci:
czech.LaunchProgram=Spustit {#AppToInstallName}
dutch.InstallMode=Installatiemodus:
dutch.StandardInstall=Installeer normaal (Startmenu, Programmas en deïnstalleerder)
dutch.PortableInstall=Pak uit als een draagbare versie (geen Programmas-vermelding of deïnstalleerder)
dutch.PortableDirName=samandarin
dutch.StartMenuShortcut=Een snelkoppeling in het menu &Start maken
dutch.DesktopShortcut=Een snelkoppeling op het &bureaublad maken
dutch.Shortcuts=Snelkoppelingen:
dutch.LaunchProgram={#AppToInstallName} starten
french.InstallMode=Mode d’installation :
french.StandardInstall=Installer normalement (menu Démarrer, Programmes et désinstallateur)
french.PortableInstall=Extraire en tant que version portable (aucune entrée dans Programmes ni désinstallateur)
french.PortableDirName=samandarin
french.StartMenuShortcut=Créer un raccourci dans le menu &Démarrer
french.DesktopShortcut=Créer un raccourci sur le &Bureau
french.Shortcuts=Raccourcis :
french.LaunchProgram=Lancer {#AppToInstallName}
german.InstallMode=Installationsmodus:
german.StandardInstall=Normal installieren (Startmenü, Programme und Deinstallationsprogramm)
german.PortableInstall=Als portable Version extrahieren (kein Eintrag in Programme oder Deinstallationsprogramm)
german.PortableDirName=samandarin
german.StartMenuShortcut=Eine Verknüpfung im &Startmenü erstellen
german.DesktopShortcut=Eine Verknüpfung auf dem &Desktop erstellen
german.Shortcuts=Verknüpfungen:
german.LaunchProgram={#AppToInstallName} starten
hungarian.InstallMode=Telepítési mód:
hungarian.StandardInstall=Normál telepítés (Start menü, Programok és eltávolító)
hungarian.PortableInstall=Kicsomagolás hordozható verzióként (nincs bejegyzés a Programokban vagy az eltávolítóban)
hungarian.PortableDirName=samandarin
hungarian.StartMenuShortcut=&Start menü parancsikon létrehozása
hungarian.DesktopShortcut=&Asztali parancsikon létrehozása
hungarian.Shortcuts=Parancsikonok:
hungarian.LaunchProgram={#AppToInstallName} indítása
romanian.InstallMode=Mod de instalare:
romanian.StandardInstall=Instalare normală (Meniul Start, Programe și dezinstalator)
romanian.PortableInstall=Extrageți ca versiune portabilă (fără intrare în Programe sau dezinstalator)
romanian.PortableDirName=samandarin
romanian.StartMenuShortcut=Creează o scurtătură în meniul &Start
romanian.DesktopShortcut=Creează o scurtătură pe &desktop
romanian.Shortcuts=Scurtături:
romanian.LaunchProgram=Pornește {#AppToInstallName}
russian.InstallMode=Режим установки:
russian.StandardInstall=Установить нормально (меню «Пуск», Программы и деинсталлятор)
russian.PortableInstall=Извлечь как портативную версию (нет записи в Программах или деинсталляторе)
russian.PortableDirName=samandarin
russian.StartMenuShortcut=Создать ярлык в меню «&Пуск»
russian.DesktopShortcut=Создать ярлык на &рабочем столе
russian.Shortcuts=Ярлыки:
russian.LaunchProgram=Запустить {#AppToInstallName}
slovak.InstallMode=Režim inštalácie:
slovak.StandardInstall=Inštalovať normálne (ponuka Štart, Programy a odstránenie)
slovak.PortableInstall=Extrahovať ako prenosnú verziu (žiadny záznam v Programech alebo odstránení)
slovak.PortableDirName=samandarin
slovak.StartMenuShortcut=Vytvoriť odkaz v ponuke &Štart
slovak.DesktopShortcut=Vytvoriť odkaz na &pracovnej ploche
slovak.Shortcuts=Odkazy:
slovak.LaunchProgram=Spustiť {#AppToInstallName}
spanish.InstallMode=Modo de instalación:
spanish.StandardInstall=Instalar normalmente (Menú Inicio, Programas y desinstalador)
spanish.PortableInstall=Extraer como versión portable (sin entrada en Programas ni desinstalador)
spanish.PortableDirName=samandarin
spanish.StartMenuShortcut=Crear un acceso directo en el menú &Inicio
spanish.DesktopShortcut=Crear un acceso directo en el &escritorio
spanish.Shortcuts=Accesos directos:
spanish.LaunchProgram=Iniciar {#AppToInstallName}
english.CodePageWarningTitle=Code Page Compatibility Warning
english.CodePageWarning=The selected language may not display all characters correctly because your Windows system locale code page (%u) does not match the code page expected by this language (%u).\n\nTo fix this, change your system locale in Windows: Settings > Time & Language > Region > Administrative > Change system locale.
chinesesimplified.CodePageWarningTitle=代码页兼容性警告
chinesesimplified.CodePageWarning=所选语言可能无法正确显示所有字符，因为您的 Windows 系统区域代码页 (%u) 与该语言预期的代码页 (%u) 不匹配。\n\n要解决此问题，请在 Windows 中更改系统区域设置：设置 > 时间和语言 > 区域 > 管理 > 更改系统区域设置。
czech.CodePageWarningTitle=Upozornění na kompatibilitu kódové stránky
czech.CodePageWarning=Vybraný jazyk nemusí správně zobrazovat všechny znaky, protože kódová stránka vašeho systémového nastavení Windows (%u) neodpovídá kódové stránce požadované tímto jazykem (%u).\n\nPro opravu změňte své systémové nastavení v Windows: Nastavení > Čas a jazyk > Region > Správa > Změnit systémové nastavení.
dutch.CodePageWarningTitle=Codepaginacompatibiliteit waarschuwing
dutch.CodePageWarning=De geselecteerde taal kan mogelijk niet alle tekens correct weergeven omdat uw Windows-systeemlocale codepagina (%u) niet overeenkomt met de codepagina die door deze taal wordt verwacht (%u).\n\nOm dit op te lossen, wijzig uw systeemlocale in Windows: Instellingen > Tijd en taal > Regio > Beheer > Systeemlocale wijzigen.
french.CodePageWarningTitle=Avertissement de compatibilité de page de codes
french.CodePageWarning=La langue sélectionnée peut ne pas afficher correctement tous les caractères car votre page de codes de locale système Windows (%u) ne correspond pas à la page de codes attendue par cette langue (%u).\n\nPour corriger cela, modifiez votre locale système dans Windows : Paramètres > Heure et langue > Région > Administration > Modifier le système local.
german.CodePageWarningTitle=Codepage-Kompatibilitätswarnung
german.CodePageWarning=Die ausgewählte Sprache zeigt möglicherweise nicht alle Zeichen korrekt an, da Ihre Windows-Systemgebietsschema-Codepage (%u) nicht mit der von dieser Sprache erwarteten Codepage (%u) übereinstimmt.\n\nUm dies zu beheben, ändern Sie Ihr Systemgebietsschema in Windows: Einstellungen > Zeit und Sprache > Region > Verwaltung > Systemsprache ändern.
hungarian.CodePageWarningTitle=Kódlap kompatibilitási figyelmeztetés
hungarian.CodePageWarning=A kiválasztott nyelv nem jeleníti meg helyesen az összes karaktert, mert a Windows rendszerterületi kódlapja (%u) nem egyezik ezzel a nyelvvel elvárt kódlappal (%u).\n\nA javításhoz módosítsa a rendszerterületet a Windowsban: Beállítások > Idő és nyelv > Régió > Kezelés > Rendszerterület módosítása.
romanian.CodePageWarningTitle=Avertisament compatibilitate pagină de coduri
romanian.CodePageWarning=Limba selectată poate să nu afișeze corect toate caracterele, deoarece pagina de coduri a setărilor de sistem Windows (%u) nu corespunde paginii de coduri așteptate de această limbă (%u).\n\nPentru a remedia, modificați setările de sistem în Windows: Setări > Ora și limba > Regiune > Administrare > Modificarea setărilor de sistem.
russian.CodePageWarningTitle=Предупреждение о совместимости кодовых страниц
russian.CodePageWarning=Выбранный язык может отображать символы неправильно, потому что кодовая страница системных параметров Windows (%u) не соответствует кодовой странице, ожидаемой для этого языка (%u).\n\nДля исправления измените системные параметры в Windows: Параметры > Время и язык > Регион > Администрирование > Изменить параметры системы.
slovak.CodePageWarningTitle=Upozornenie na kompatibilitu kódovej stránky
slovak.CodePageWarning=Vybraný jazyk nemusí správne zobrazovať všetky znaky, pretože kódová stránka vášho systémového nastavenia Windows (%u) nezodpovedá kódovej stránke požadovanej týmto jazykom (%u).\n\nNa opravu zmeňte svoje systémové nastavenie v Windows: Nastavenie > Čas a jazyk > Región > Správa > Zmeniť systémové nastavenie.
spanish.CodePageWarningTitle=Advertencia de compatibilidad de páginas de códigos
spanish.CodePageWarning=El idioma seleccionado puede no mostrar todos los caracteres correctamente porque su página de códigos de configuración regional del sistema Windows (%u) no coincide con la página de códigos esperada para este idioma (%u).\n\nPara solucionarlo, cambie su configuración regional del sistema en Windows: Configuración > Hora e idioma > Región > Administración > Cambiar configuración regional del sistema.
english.KeepConfigQuestion=An existing configuration file (configstorage.ini) was found.\n\nIt will be renamed to configstorage.ini.BAK and the new default configuration will be installed.
chinesesimplified.KeepConfigQuestion=检测到已存在配置文件 (configstorage.ini)。\n\n该文件将被重命名为 configstorage.ini.BAK，并安装新的默认配置。
czech.KeepConfigQuestion=Byl nalezen existující konfigurační soubor (configstorage.ini).\n\nBude přejmenován na configstorage.ini.BAK a nainstaluje se nová výchozí konfigurace.
dutch.KeepConfigQuestion=Er is een bestaand configuratiebestand (configstorage.ini) gevonden.\n\nDit wordt hernoemd naar configstorage.ini.BAK en er wordt een nieuw standaard configuratiebestand geïnstalleerd.
french.KeepConfigQuestion=Un fichier de configuration existant (configstorage.ini) a été trouvé.\n\nIl sera renommé en configstorage.ini.BAK et la nouvelle configuration par défaut sera installée.
german.KeepConfigQuestion=Eine bestehende Konfigurationsdatei (configstorage.ini) wurde gefunden.\n\nSie wird in configstorage.ini.BAK umbenannt und die neue Standardkonfiguration wird installiert.
hungarian.KeepConfigQuestion=Meglévő konfigurációs fájl található (configstorage.ini).\n\nÁtnevezésre kerül configstorage.ini.BAK névre, és az új alapértelmezett konfiguráció telepítésre kerül.
romanian.KeepConfigQuestion=A fost găsit un fișier de configurare existent (configstorage.ini).\n\nAcesta va fi redenumit în configstorage.ini.BAK și va fi instalată noua configurație implicită.
russian.KeepConfigQuestion=Обнаружен существующий файл конфигурации (configstorage.ini).\n\nОн будет переименован в configstorage.ini.BAK и установлена новая конфигурация по умолчанию.
slovak.KeepConfigQuestion=Bol nájdený existujúci konfiguračný súbor (configstorage.ini).\n\nBude premenovaný na configstorage.ini.BAK a nainštaluje sa nová predvolená konfigurácia.
spanish.KeepConfigQuestion=Se encontró un archivo de configuración existente (configstorage.ini).\n\nSerá renombrado a configstorage.ini.BAK y se instalará la nueva configuración predeterminada.
english.PluginSelectionTitle=Select plugins
english.PluginSelectionDescription=Choose which bundled plugins will be installed or extracted.
chinesesimplified.PluginSelectionTitle=选择插件
chinesesimplified.PluginSelectionDescription=选择要安装或解压的随附插件。
dutch.PluginSelectionTitle=Plug-ins selecteren
dutch.PluginSelectionDescription=Kies welke meegeleverde plug-ins worden geïnstalleerd of uitgepakt.
french.PluginSelectionTitle=Sélection des modules
french.PluginSelectionDescription=Choisissez les modules inclus à installer ou extraire.
german.PluginSelectionTitle=Plugins auswählen
german.PluginSelectionDescription=Wählen Sie aus, welche mitgelieferten Plugins installiert oder entpackt werden.
hungarian.PluginSelectionTitle=Bővítmények kiválasztása
hungarian.PluginSelectionDescription=Válassza ki a telepítendő vagy kicsomagolandó mellékelt bővítményeket.
romanian.PluginSelectionTitle=Selectare pluginuri
romanian.PluginSelectionDescription=Alegeți pluginurile incluse care vor fi instalate sau extrase.
russian.PluginSelectionTitle=Выбор плагинов
russian.PluginSelectionDescription=Выберите, какие включенные плагины будут установлены или извлечены.
slovak.PluginSelectionTitle=Výber pluginov
slovak.PluginSelectionDescription=Zvoľte, ktoré pribalené pluginy sa nainštalujú alebo rozbalia.
spanish.PluginSelectionTitle=Seleccionar complementos
spanish.PluginSelectionDescription=Elija qué complementos incluidos se instalarán o extraerán.
czech.PluginSelectionTitle=Výběr pluginů
czech.PluginSelectionDescription=Zvolte, které přibalené pluginy se nainstalují nebo rozbalí.
english.InstalledVersionLabel=installed
chinesesimplified.InstalledVersionLabel=已安装
dutch.InstalledVersionLabel=geïnstalleerd
french.InstalledVersionLabel=installé
german.InstalledVersionLabel=installiert
hungarian.InstalledVersionLabel=telepítve
romanian.InstalledVersionLabel=instalat
russian.InstalledVersionLabel=установлено
slovak.InstalledVersionLabel=nainštalované
spanish.InstalledVersionLabel=instalado
czech.InstalledVersionLabel=nainstalováno

[Tasks]
Name: "startmenuicon"; Description: "{cm:StartMenuShortcut}"; GroupDescription: "{cm:Shortcuts}"; Check: not IsPortableInstall
Name: "desktopicon"; Description: "{cm:DesktopShortcut}"; GroupDescription: "{cm:Shortcuts}"; Flags: unchecked; Check: not IsPortableInstall

[Dirs]
Name: "{app}\convert"
Name: "{app}\convert\centeuro"
Name: "{app}\convert\cyrillic"
Name: "{app}\convert\westeuro"
Name: "{app}\doc"
Name: "{app}\help"
Name: "{app}\help\english"
Name: "{app}\lang"
Name: "{app}\utils"
Name: "{app}\plugins"
Name: "{app}\plugins\7zip"
Name: "{app}\plugins\7zip\lang"
Name: "{app}\plugins\automation"
Name: "{app}\plugins\automation\lang"
Name: "{app}\plugins\automation\scripts"
Name: "{app}\plugins\dbviewer"
Name: "{app}\plugins\dbviewer\lang"
Name: "{app}\plugins\diskmap"
Name: "{app}\plugins\diskmap\lang"
Name: "{app}\plugins\filecomp"
Name: "{app}\plugins\filecomp\lang"
Name: "{app}\plugins\folders"
Name: "{app}\plugins\folders\lang"
Name: "{app}\plugins\ftp"
Name: "{app}\plugins\ftp\lang"
Name: "{app}\plugins\hypervm"
Name: "{app}\plugins\hypervm\lang"
Name: "{app}\plugins\checksum"
Name: "{app}\plugins\checksum\lang"
Name: "{app}\plugins\ieviewer"
Name: "{app}\plugins\ieviewer\css"
Name: "{app}\plugins\ieviewer\lang"
Name: "{app}\plugins\jsonviewer"
Name: "{app}\plugins\jsonviewer\lang"
Name: "{app}\plugins\mmviewer"
Name: "{app}\plugins\mmviewer\lang"
Name: "{app}\plugins\nethood"
Name: "{app}\plugins\nethood\lang"
Name: "{app}\plugins\pak"
Name: "{app}\plugins\pak\lang"
Name: "{app}\plugins\peviewer"
Name: "{app}\plugins\peviewer\lang"
Name: "{app}\plugins\pictview"
Name: "{app}\plugins\pictview\lang"
Name: "{app}\plugins\portables"
Name: "{app}\plugins\portables\lang"
Name: "{app}\plugins\regedt"
Name: "{app}\plugins\regedt\lang"
Name: "{app}\plugins\renamer"
Name: "{app}\plugins\renamer\lang"
Name: "{app}\plugins\samandarin"
Name: "{app}\plugins\samandarin\lang"
Name: "{app}\plugins\salamatrix"
Name: "{app}\plugins\demoplug"
Name: "{app}\plugins\demoplug\lang"
Name: "{app}\plugins\serviceexplorer"
Name: "{app}\plugins\serviceexplorer\lang"
Name: "{app}\plugins\splitcbn"
Name: "{app}\plugins\splitcbn\lang"
Name: "{app}\plugins\tar"
Name: "{app}\plugins\tar\lang"
Name: "{app}\plugins\textviewer"
Name: "{app}\plugins\textviewer\lang"
Name: "{app}\plugins\textviewer\data"
Name: "{app}\plugins\textviewer\data\languages"
Name: "{app}\plugins\textviewer\data\themes"
Name: "{app}\plugins\textviewer\runtimes\win-arm64\native"
Name: "{app}\plugins\textviewer\runtimes\win-x64\native"
Name: "{app}\plugins\textviewer\runtimes\win-x86\native"
Name: "{app}\plugins\unarj"
Name: "{app}\plugins\unarj\lang"
Name: "{app}\plugins\uncab"
Name: "{app}\plugins\uncab\lang"
Name: "{app}\plugins\undelete"
Name: "{app}\plugins\undelete\lang"
Name: "{app}\plugins\unfat"
Name: "{app}\plugins\unfat\lang"
Name: "{app}\plugins\unchm"
Name: "{app}\plugins\unchm\lang"
Name: "{app}\plugins\uniso"
Name: "{app}\plugins\uniso\lang"
Name: "{app}\plugins\unlha"
Name: "{app}\plugins\unlha\lang"
Name: "{app}\plugins\unmime"
Name: "{app}\plugins\unmime\lang"
Name: "{app}\plugins\unole"
Name: "{app}\plugins\unole\lang"
Name: "{app}\plugins\unrar"
Name: "{app}\plugins\unrar\lang"
Name: "{app}\plugins\webview2renderviewer"
Name: "{app}\plugins\webview2renderviewer\lang"
Name: "{app}\plugins\webview2renderviewer\runtimes\win-arm64\native"
Name: "{app}\plugins\webview2renderviewer\runtimes\win-x64\native"
Name: "{app}\plugins\webview2renderviewer\runtimes\win-x86\native"
Name: "{app}\plugins\wmobile"
Name: "{app}\plugins\wmobile\lang"
Name: "{app}\plugins\zip"
Name: "{app}\plugins\zip\lang"
Name: "{app}\plugins\zip\sfx"
Name: "{app}\plugins\zip\zip2sfx"
Name: "{app}\remove"
Name: "{app}\toolbars"
Name: "{app}\toolbars\darkmode"
Name: "{app}\toolbars\pictview"
Name: "{app}\toolbars\darkmode\pictview"

[Files]
Source: "{#PayloadDir}\concrt140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1-1.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250asci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250c852.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250cork.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250ebcd.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250iso1.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250iso2.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250kame.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250koi8.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250mac.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\c8521250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\c852asci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\c852kame.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\convert.cfg"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\cork1250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\corkasci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\ebcd1250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\ebcdasci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\iso11250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\iso1asci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\iso21250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\iso2asci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\kame1250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\kameasci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\kamec852.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\koi81250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\koi8asci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\mac1250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\macasci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1-1.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251bmik.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251c855.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251c866.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251ebcd.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251iso5.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251koir.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251koiu.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251macc.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\bmik1251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\c8551251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\c8661251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\convert.cfg"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\ebcd1251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\iso51251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\koir1251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\koiu1251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\macc1251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1-1.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\10511252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1051asci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\12521051.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252c437.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252c850.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252c860.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252ebcd.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252is15.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252iso1.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252macr.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\c4371252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\c437asci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\c8501252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\c850asci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\c8601252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\c860asci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\convert.cfg"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\ebcd1252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\ebcdasci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\is151252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\is15asci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\iso11252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\iso1asci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\macr1252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\macrasci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\license.txt"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\license_gpl.txt"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_czech.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_dutch.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_french.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_german.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_hungarian.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_chinesesimplified.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_romanian.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_russian.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_slovak.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_spanish.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\translations.txt"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\7zip.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\automation.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\dbviewer.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\demomenu.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\demoplug.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\demoview.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\diskmap.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\filecomp.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\ftp.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\hypervm.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\checksum.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\checkver.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\ieviewer.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\mmviewer.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\nethood.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\pak.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\peviewer.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\pictview.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\regedt.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\renamer.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\salamand.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\salamand.chw"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\splitcbn.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\tar.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\unarj.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\uncab.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\undelete.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\unfat.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\unchm.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\uniso.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\unlha.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\unmime.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\unrar.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\winscp.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\wmobile.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\zip.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\czech.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\dutch.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\english.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\french.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\german.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\hungarian.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\chinesesimplified.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\romanian.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\russian.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\slovak.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\spanish.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\msvcp140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\7zip\7za.dll"; DestDir: "{app}\plugins\7zip"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\7zip.spl"; DestDir: "{app}\plugins\7zip"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\7zwrapper.dll"; DestDir: "{app}\plugins\7zip"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\lang\czech.slg"; DestDir: "{app}\plugins\7zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\lang\dutch.slg"; DestDir: "{app}\plugins\7zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\lang\english.slg"; DestDir: "{app}\plugins\7zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\lang\french.slg"; DestDir: "{app}\plugins\7zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\lang\german.slg"; DestDir: "{app}\plugins\7zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\lang\hungarian.slg"; DestDir: "{app}\plugins\7zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\7zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\lang\romanian.slg"; DestDir: "{app}\plugins\7zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\lang\russian.slg"; DestDir: "{app}\plugins\7zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\lang\slovak.slg"; DestDir: "{app}\plugins\7zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\lang\spanish.slg"; DestDir: "{app}\plugins\7zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\automation\automation.spl"; DestDir: "{app}\plugins\automation"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\lang\czech.slg"; DestDir: "{app}\plugins\automation\lang"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\lang\dutch.slg"; DestDir: "{app}\plugins\automation\lang"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\lang\english.slg"; DestDir: "{app}\plugins\automation\lang"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\lang\french.slg"; DestDir: "{app}\plugins\automation\lang"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\lang\german.slg"; DestDir: "{app}\plugins\automation\lang"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\lang\hungarian.slg"; DestDir: "{app}\plugins\automation\lang"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\automation\lang"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\lang\romanian.slg"; DestDir: "{app}\plugins\automation\lang"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\lang\russian.slg"; DestDir: "{app}\plugins\automation\lang"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\lang\slovak.slg"; DestDir: "{app}\plugins\automation\lang"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\lang\spanish.slg"; DestDir: "{app}\plugins\automation\lang"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Convert Images.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Count Lines.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Launch Elevated Command Prompt.vbs"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Make Link.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Make List (JScript).js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Make List (VBScript).vbs"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Unpack Multiple Archives.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\dbviewer\dbviewer.spl"; DestDir: "{app}\plugins\dbviewer"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\dbviewer\lang\czech.slg"; DestDir: "{app}\plugins\dbviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\dbviewer\lang\dutch.slg"; DestDir: "{app}\plugins\dbviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\dbviewer\lang\english.slg"; DestDir: "{app}\plugins\dbviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\dbviewer\lang\french.slg"; DestDir: "{app}\plugins\dbviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\dbviewer\lang\german.slg"; DestDir: "{app}\plugins\dbviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\dbviewer\lang\hungarian.slg"; DestDir: "{app}\plugins\dbviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\dbviewer\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\dbviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\dbviewer\lang\romanian.slg"; DestDir: "{app}\plugins\dbviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\dbviewer\lang\russian.slg"; DestDir: "{app}\plugins\dbviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\dbviewer\lang\slovak.slg"; DestDir: "{app}\plugins\dbviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\dbviewer\lang\spanish.slg"; DestDir: "{app}\plugins\dbviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\diskmap\diskmap.spl"; DestDir: "{app}\plugins\diskmap"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\diskmap\lang\czech.slg"; DestDir: "{app}\plugins\diskmap\lang"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\diskmap\lang\dutch.slg"; DestDir: "{app}\plugins\diskmap\lang"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\diskmap\lang\english.slg"; DestDir: "{app}\plugins\diskmap\lang"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\diskmap\lang\french.slg"; DestDir: "{app}\plugins\diskmap\lang"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\diskmap\lang\german.slg"; DestDir: "{app}\plugins\diskmap\lang"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\diskmap\lang\hungarian.slg"; DestDir: "{app}\plugins\diskmap\lang"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\diskmap\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\diskmap\lang"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\diskmap\lang\romanian.slg"; DestDir: "{app}\plugins\diskmap\lang"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\diskmap\lang\russian.slg"; DestDir: "{app}\plugins\diskmap\lang"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\diskmap\lang\slovak.slg"; DestDir: "{app}\plugins\diskmap\lang"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\diskmap\lang\spanish.slg"; DestDir: "{app}\plugins\diskmap\lang"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\filecomp\fcremote.exe"; DestDir: "{app}\plugins\filecomp"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\filecomp.spl"; DestDir: "{app}\plugins\filecomp"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\lang\czech.slg"; DestDir: "{app}\plugins\filecomp\lang"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\lang\dutch.slg"; DestDir: "{app}\plugins\filecomp\lang"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\lang\english.slg"; DestDir: "{app}\plugins\filecomp\lang"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\lang\french.slg"; DestDir: "{app}\plugins\filecomp\lang"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\lang\german.slg"; DestDir: "{app}\plugins\filecomp\lang"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\lang\hungarian.slg"; DestDir: "{app}\plugins\filecomp\lang"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\filecomp\lang"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\lang\romanian.slg"; DestDir: "{app}\plugins\filecomp\lang"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\lang\russian.slg"; DestDir: "{app}\plugins\filecomp\lang"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\lang\slovak.slg"; DestDir: "{app}\plugins\filecomp\lang"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\lang\spanish.slg"; DestDir: "{app}\plugins\filecomp\lang"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\folders\folders.spl"; DestDir: "{app}\plugins\folders"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\folders\lang\czech.slg"; DestDir: "{app}\plugins\folders\lang"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\folders\lang\dutch.slg"; DestDir: "{app}\plugins\folders\lang"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\folders\lang\english.slg"; DestDir: "{app}\plugins\folders\lang"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\folders\lang\french.slg"; DestDir: "{app}\plugins\folders\lang"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\folders\lang\german.slg"; DestDir: "{app}\plugins\folders\lang"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\folders\lang\hungarian.slg"; DestDir: "{app}\plugins\folders\lang"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\folders\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\folders\lang"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\folders\lang\romanian.slg"; DestDir: "{app}\plugins\folders\lang"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\folders\lang\russian.slg"; DestDir: "{app}\plugins\folders\lang"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\folders\lang\slovak.slg"; DestDir: "{app}\plugins\folders\lang"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\folders\lang\spanish.slg"; DestDir: "{app}\plugins\folders\lang"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\ftp\ftp.spl"; DestDir: "{app}\plugins\ftp"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\ftp\lang\czech.slg"; DestDir: "{app}\plugins\ftp\lang"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\ftp\lang\dutch.slg"; DestDir: "{app}\plugins\ftp\lang"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\ftp\lang\english.slg"; DestDir: "{app}\plugins\ftp\lang"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\ftp\lang\french.slg"; DestDir: "{app}\plugins\ftp\lang"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\ftp\lang\german.slg"; DestDir: "{app}\plugins\ftp\lang"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\ftp\lang\hungarian.slg"; DestDir: "{app}\plugins\ftp\lang"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\ftp\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\ftp\lang"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\ftp\lang\romanian.slg"; DestDir: "{app}\plugins\ftp\lang"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\ftp\lang\russian.slg"; DestDir: "{app}\plugins\ftp\lang"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\ftp\lang\slovak.slg"; DestDir: "{app}\plugins\ftp\lang"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\ftp\lang\spanish.slg"; DestDir: "{app}\plugins\ftp\lang"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\hypervm\HyperVM.Managed.dll"; DestDir: "{app}\plugins\hypervm"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\hypervm.spl"; DestDir: "{app}\plugins\hypervm"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\lang\czech.slg"; DestDir: "{app}\plugins\hypervm\lang"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\lang\dutch.slg"; DestDir: "{app}\plugins\hypervm\lang"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\lang\english.slg"; DestDir: "{app}\plugins\hypervm\lang"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\lang\french.slg"; DestDir: "{app}\plugins\hypervm\lang"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\lang\german.slg"; DestDir: "{app}\plugins\hypervm\lang"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\lang\hungarian.slg"; DestDir: "{app}\plugins\hypervm\lang"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\hypervm\lang"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\lang\romanian.slg"; DestDir: "{app}\plugins\hypervm\lang"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\lang\russian.slg"; DestDir: "{app}\plugins\hypervm\lang"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\lang\slovak.slg"; DestDir: "{app}\plugins\hypervm\lang"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\lang\spanish.slg"; DestDir: "{app}\plugins\hypervm\lang"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\checksum\checksum.spl"; DestDir: "{app}\plugins\checksum"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\checksum\lang\czech.slg"; DestDir: "{app}\plugins\checksum\lang"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\checksum\lang\dutch.slg"; DestDir: "{app}\plugins\checksum\lang"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\checksum\lang\english.slg"; DestDir: "{app}\plugins\checksum\lang"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\checksum\lang\french.slg"; DestDir: "{app}\plugins\checksum\lang"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\checksum\lang\german.slg"; DestDir: "{app}\plugins\checksum\lang"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\checksum\lang\hungarian.slg"; DestDir: "{app}\plugins\checksum\lang"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\checksum\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\checksum\lang"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\checksum\lang\romanian.slg"; DestDir: "{app}\plugins\checksum\lang"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\checksum\lang\russian.slg"; DestDir: "{app}\plugins\checksum\lang"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\checksum\lang\slovak.slg"; DestDir: "{app}\plugins\checksum\lang"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\checksum\lang\spanish.slg"; DestDir: "{app}\plugins\checksum\lang"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\ieviewer\css\githubmd.css"; DestDir: "{app}\plugins\ieviewer\css"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\ieviewer.spl"; DestDir: "{app}\plugins\ieviewer"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\lang\czech.slg"; DestDir: "{app}\plugins\ieviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\lang\dutch.slg"; DestDir: "{app}\plugins\ieviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\lang\english.slg"; DestDir: "{app}\plugins\ieviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\lang\french.slg"; DestDir: "{app}\plugins\ieviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\lang\german.slg"; DestDir: "{app}\plugins\ieviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\lang\hungarian.slg"; DestDir: "{app}\plugins\ieviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\ieviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\lang\romanian.slg"; DestDir: "{app}\plugins\ieviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\lang\russian.slg"; DestDir: "{app}\plugins\ieviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\lang\slovak.slg"; DestDir: "{app}\plugins\ieviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\lang\spanish.slg"; DestDir: "{app}\plugins\ieviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\DEPENDENCIES.md"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\JsonViewer.Managed.dll"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\jsonviewer.spl"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\lang\czech.slg"; DestDir: "{app}\plugins\jsonviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\lang\dutch.slg"; DestDir: "{app}\plugins\jsonviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\lang\english.slg"; DestDir: "{app}\plugins\jsonviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\lang\french.slg"; DestDir: "{app}\plugins\jsonviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\lang\german.slg"; DestDir: "{app}\plugins\jsonviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\lang\hungarian.slg"; DestDir: "{app}\plugins\jsonviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\jsonviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\lang\romanian.slg"; DestDir: "{app}\plugins\jsonviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\lang\russian.slg"; DestDir: "{app}\plugins\jsonviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\lang\slovak.slg"; DestDir: "{app}\plugins\jsonviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\lang\spanish.slg"; DestDir: "{app}\plugins\jsonviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\Newtonsoft.Json.dll"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\SUPPORTED_FILE_TYPES.md"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\mmviewer\lang\czech.slg"; DestDir: "{app}\plugins\mmviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\mmviewer\lang\dutch.slg"; DestDir: "{app}\plugins\mmviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\mmviewer\lang\english.slg"; DestDir: "{app}\plugins\mmviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\mmviewer\lang\french.slg"; DestDir: "{app}\plugins\mmviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\mmviewer\lang\german.slg"; DestDir: "{app}\plugins\mmviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\mmviewer\lang\hungarian.slg"; DestDir: "{app}\plugins\mmviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\mmviewer\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\mmviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\mmviewer\lang\romanian.slg"; DestDir: "{app}\plugins\mmviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\mmviewer\lang\russian.slg"; DestDir: "{app}\plugins\mmviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\mmviewer\lang\slovak.slg"; DestDir: "{app}\plugins\mmviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\mmviewer\lang\spanish.slg"; DestDir: "{app}\plugins\mmviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\mmviewer\mmviewer.spl"; DestDir: "{app}\plugins\mmviewer"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\nethood\lang\czech.slg"; DestDir: "{app}\plugins\nethood\lang"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\nethood\lang\dutch.slg"; DestDir: "{app}\plugins\nethood\lang"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\nethood\lang\english.slg"; DestDir: "{app}\plugins\nethood\lang"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\nethood\lang\french.slg"; DestDir: "{app}\plugins\nethood\lang"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\nethood\lang\german.slg"; DestDir: "{app}\plugins\nethood\lang"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\nethood\lang\hungarian.slg"; DestDir: "{app}\plugins\nethood\lang"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\nethood\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\nethood\lang"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\nethood\lang\romanian.slg"; DestDir: "{app}\plugins\nethood\lang"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\nethood\lang\russian.slg"; DestDir: "{app}\plugins\nethood\lang"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\nethood\lang\slovak.slg"; DestDir: "{app}\plugins\nethood\lang"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\nethood\lang\spanish.slg"; DestDir: "{app}\plugins\nethood\lang"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\nethood\nethood.spl"; DestDir: "{app}\plugins\nethood"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\pak\lang\czech.slg"; DestDir: "{app}\plugins\pak\lang"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\pak\lang\dutch.slg"; DestDir: "{app}\plugins\pak\lang"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\pak\lang\english.slg"; DestDir: "{app}\plugins\pak\lang"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\pak\lang\french.slg"; DestDir: "{app}\plugins\pak\lang"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\pak\lang\german.slg"; DestDir: "{app}\plugins\pak\lang"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\pak\lang\hungarian.slg"; DestDir: "{app}\plugins\pak\lang"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\pak\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\pak\lang"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\pak\lang\romanian.slg"; DestDir: "{app}\plugins\pak\lang"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\pak\lang\russian.slg"; DestDir: "{app}\plugins\pak\lang"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\pak\lang\slovak.slg"; DestDir: "{app}\plugins\pak\lang"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\pak\lang\spanish.slg"; DestDir: "{app}\plugins\pak\lang"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\pak\pak.spl"; DestDir: "{app}\plugins\pak"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\peviewer\lang\czech.slg"; DestDir: "{app}\plugins\peviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\peviewer\lang\dutch.slg"; DestDir: "{app}\plugins\peviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\peviewer\lang\english.slg"; DestDir: "{app}\plugins\peviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\peviewer\lang\french.slg"; DestDir: "{app}\plugins\peviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\peviewer\lang\german.slg"; DestDir: "{app}\plugins\peviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\peviewer\lang\hungarian.slg"; DestDir: "{app}\plugins\peviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\peviewer\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\peviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\peviewer\lang\romanian.slg"; DestDir: "{app}\plugins\peviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\peviewer\lang\russian.slg"; DestDir: "{app}\plugins\peviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\peviewer\lang\slovak.slg"; DestDir: "{app}\plugins\peviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\peviewer\lang\spanish.slg"; DestDir: "{app}\plugins\peviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\peviewer\peviewer.spl"; DestDir: "{app}\plugins\peviewer"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\pictview\exif.dll"; DestDir: "{app}\plugins\pictview"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\lang\czech.slg"; DestDir: "{app}\plugins\pictview\lang"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\lang\dutch.slg"; DestDir: "{app}\plugins\pictview\lang"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\lang\english.slg"; DestDir: "{app}\plugins\pictview\lang"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\lang\french.slg"; DestDir: "{app}\plugins\pictview\lang"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\lang\german.slg"; DestDir: "{app}\plugins\pictview\lang"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\lang\hungarian.slg"; DestDir: "{app}\plugins\pictview\lang"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\pictview\lang"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\lang\romanian.slg"; DestDir: "{app}\plugins\pictview\lang"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\lang\russian.slg"; DestDir: "{app}\plugins\pictview\lang"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\lang\slovak.slg"; DestDir: "{app}\plugins\pictview\lang"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\lang\spanish.slg"; DestDir: "{app}\plugins\pictview\lang"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\pictview.spl"; DestDir: "{app}\plugins\pictview"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\plugins.ver"; DestDir: "{app}\plugins"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\portables\lang\czech.slg"; DestDir: "{app}\plugins\portables\lang"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\portables\lang\dutch.slg"; DestDir: "{app}\plugins\portables\lang"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\portables\lang\english.slg"; DestDir: "{app}\plugins\portables\lang"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\portables\lang\french.slg"; DestDir: "{app}\plugins\portables\lang"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\portables\lang\german.slg"; DestDir: "{app}\plugins\portables\lang"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\portables\lang\hungarian.slg"; DestDir: "{app}\plugins\portables\lang"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\portables\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\portables\lang"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\portables\lang\romanian.slg"; DestDir: "{app}\plugins\portables\lang"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\portables\lang\russian.slg"; DestDir: "{app}\plugins\portables\lang"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\portables\lang\slovak.slg"; DestDir: "{app}\plugins\portables\lang"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\portables\lang\spanish.slg"; DestDir: "{app}\plugins\portables\lang"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\portables\portables.spl"; DestDir: "{app}\plugins\portables"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\regedt\lang\czech.slg"; DestDir: "{app}\plugins\regedt\lang"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\regedt\lang\dutch.slg"; DestDir: "{app}\plugins\regedt\lang"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\regedt\lang\english.slg"; DestDir: "{app}\plugins\regedt\lang"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\regedt\lang\french.slg"; DestDir: "{app}\plugins\regedt\lang"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\regedt\lang\german.slg"; DestDir: "{app}\plugins\regedt\lang"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\regedt\lang\hungarian.slg"; DestDir: "{app}\plugins\regedt\lang"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\regedt\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\regedt\lang"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\regedt\lang\romanian.slg"; DestDir: "{app}\plugins\regedt\lang"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\regedt\lang\russian.slg"; DestDir: "{app}\plugins\regedt\lang"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\regedt\lang\slovak.slg"; DestDir: "{app}\plugins\regedt\lang"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\regedt\lang\spanish.slg"; DestDir: "{app}\plugins\regedt\lang"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\regedt\regedt.spl"; DestDir: "{app}\plugins\regedt"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\renamer\lang\czech.slg"; DestDir: "{app}\plugins\renamer\lang"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\renamer\lang\dutch.slg"; DestDir: "{app}\plugins\renamer\lang"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\renamer\lang\english.slg"; DestDir: "{app}\plugins\renamer\lang"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\renamer\lang\french.slg"; DestDir: "{app}\plugins\renamer\lang"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\renamer\lang\german.slg"; DestDir: "{app}\plugins\renamer\lang"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\renamer\lang\hungarian.slg"; DestDir: "{app}\plugins\renamer\lang"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\renamer\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\renamer\lang"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\renamer\lang\romanian.slg"; DestDir: "{app}\plugins\renamer\lang"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\renamer\lang\russian.slg"; DestDir: "{app}\plugins\renamer\lang"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\renamer\lang\slovak.slg"; DestDir: "{app}\plugins\renamer\lang"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\renamer\lang\spanish.slg"; DestDir: "{app}\plugins\renamer\lang"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\renamer\renamer.spl"; DestDir: "{app}\plugins\renamer"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\samandarin\lang\czech.slg"; DestDir: "{app}\plugins\samandarin\lang"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\lang\dutch.slg"; DestDir: "{app}\plugins\samandarin\lang"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\lang\english.slg"; DestDir: "{app}\plugins\samandarin\lang"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\lang\french.slg"; DestDir: "{app}\plugins\samandarin\lang"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\lang\german.slg"; DestDir: "{app}\plugins\samandarin\lang"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\lang\hungarian.slg"; DestDir: "{app}\plugins\samandarin\lang"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\samandarin\lang"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\lang\romanian.slg"; DestDir: "{app}\plugins\samandarin\lang"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\lang\russian.slg"; DestDir: "{app}\plugins\samandarin\lang"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\lang\slovak.slg"; DestDir: "{app}\plugins\samandarin\lang"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\lang\spanish.slg"; DestDir: "{app}\plugins\samandarin\lang"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\Samandarin.Managed.dll"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\samandarin.spl"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\serviceexplorer\lang\czech.slg"; DestDir: "{app}\plugins\serviceexplorer\lang"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\serviceexplorer\lang\dutch.slg"; DestDir: "{app}\plugins\serviceexplorer\lang"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\serviceexplorer\lang\english.slg"; DestDir: "{app}\plugins\serviceexplorer\lang"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\serviceexplorer\lang\french.slg"; DestDir: "{app}\plugins\serviceexplorer\lang"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\serviceexplorer\lang\german.slg"; DestDir: "{app}\plugins\serviceexplorer\lang"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\serviceexplorer\lang\hungarian.slg"; DestDir: "{app}\plugins\serviceexplorer\lang"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\serviceexplorer\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\serviceexplorer\lang"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\serviceexplorer\lang\romanian.slg"; DestDir: "{app}\plugins\serviceexplorer\lang"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\serviceexplorer\lang\russian.slg"; DestDir: "{app}\plugins\serviceexplorer\lang"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\serviceexplorer\lang\slovak.slg"; DestDir: "{app}\plugins\serviceexplorer\lang"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\serviceexplorer\lang\spanish.slg"; DestDir: "{app}\plugins\serviceexplorer\lang"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\serviceexplorer\serviceexplorer.spl"; DestDir: "{app}\plugins\serviceexplorer"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\splitcbn\lang\czech.slg"; DestDir: "{app}\plugins\splitcbn\lang"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\splitcbn\lang\dutch.slg"; DestDir: "{app}\plugins\splitcbn\lang"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\splitcbn\lang\english.slg"; DestDir: "{app}\plugins\splitcbn\lang"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\splitcbn\lang\french.slg"; DestDir: "{app}\plugins\splitcbn\lang"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\splitcbn\lang\german.slg"; DestDir: "{app}\plugins\splitcbn\lang"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\splitcbn\lang\hungarian.slg"; DestDir: "{app}\plugins\splitcbn\lang"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\splitcbn\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\splitcbn\lang"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\splitcbn\lang\romanian.slg"; DestDir: "{app}\plugins\splitcbn\lang"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\splitcbn\lang\russian.slg"; DestDir: "{app}\plugins\splitcbn\lang"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\splitcbn\lang\slovak.slg"; DestDir: "{app}\plugins\splitcbn\lang"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\splitcbn\lang\spanish.slg"; DestDir: "{app}\plugins\splitcbn\lang"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\splitcbn\splitcbn.spl"; DestDir: "{app}\plugins\splitcbn"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\tar\lang\czech.slg"; DestDir: "{app}\plugins\tar\lang"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\tar\lang\dutch.slg"; DestDir: "{app}\plugins\tar\lang"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\tar\lang\english.slg"; DestDir: "{app}\plugins\tar\lang"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\tar\lang\french.slg"; DestDir: "{app}\plugins\tar\lang"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\tar\lang\german.slg"; DestDir: "{app}\plugins\tar\lang"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\tar\lang\hungarian.slg"; DestDir: "{app}\plugins\tar\lang"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\tar\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\tar\lang"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\tar\lang\romanian.slg"; DestDir: "{app}\plugins\tar\lang"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\tar\lang\russian.slg"; DestDir: "{app}\plugins\tar\lang"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\tar\lang\slovak.slg"; DestDir: "{app}\plugins\tar\lang"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\tar\lang\spanish.slg"; DestDir: "{app}\plugins\tar\lang"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\tar\tar.spl"; DestDir: "{app}\plugins\tar"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\abap.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\abnf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\actionscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ada.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\agda.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\al.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\antlr4.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\apacheconf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\apex.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\apl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\applescript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\aql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\arduino.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\arff.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\asciidoc.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\asm6502.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\asmatmel.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\aspnet.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\autohotkey.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\autoit.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\avisynth.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\avro-idl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bash.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\basic.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\batch.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bbcode.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bicep.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\birb.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bison.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bnf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\brainfuck.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\brightscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bro.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bsl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\c.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cfscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cil.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\clike.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\clojure.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cmake.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cobol.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\coffeescript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\concurnas.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\coq.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cpp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\crystal.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\csharp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cshtml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\csp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\css.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\csv.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cypher.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\d.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\dart.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\dataweave.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\dax.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\dhall.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\diff.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\django.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\dns-zone-file.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\docker.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\dot.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ebnf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\editorconfig.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\eiffel.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ejs.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\elixir.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\elm.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\erb.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\erlang.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\etlua.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\excel-formula.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\factor.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\false.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\firestore-security-rules.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\flow.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\fortran.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\fsharp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ftl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gap.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gcode.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gdscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gedcom.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gherkin.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\git.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\glsl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gn.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\go-module.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\go.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\graphql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\groovy.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\haml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\handlebars.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\haskell.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\haxe.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\hcl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\hlsl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\hoon.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\hpkp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\hsts.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\http.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\chaiscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\icon.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\icu-message-format.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\idris.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\iecst.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ignore.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ichigojam.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\inform7.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ini.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\io.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\j.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\java.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\javadoc.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\javadoclike.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\javascript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\javastacktrace.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jexl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jolie.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jq.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jsdoc.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\json.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\json5.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jsonp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jsstacktrace.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jsx.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\julia.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\keepalived.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\keyman.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\kotlin.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\kumir.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\kusto.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\languages.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\latex.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\latte.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\less.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\lilypond.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\liquid.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\lisp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\livescript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\llvm.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\log.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\lolcode.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\lua.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\magma.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\makefile.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\markdown.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\markup-templating.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\markup.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\matlab.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\maxscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\mel.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\mermaid.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\mizar.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\mongodb.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\monkey.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\moonscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\n1ql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\n4js.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nand2tetris-hdl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\naniscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nasm.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\neon.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nevod.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nginx.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nim.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nix.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nsis.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\objectivec.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ocaml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\opencl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\openqasm.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\oz.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\parigp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\parser.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\pascal.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\pascaligo.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\pcaxis.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\peoplecode.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\perl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\php.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\phpdoc.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\plsql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\powerquery.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\powershell.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\processing.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\prolog.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\promql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\properties.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\protobuf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\psl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\pug.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\puppet.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\pure.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\purebasic.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\purescript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\python.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\q.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\qml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\qore.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\qsharp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\r.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\racket.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\reason.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\regex.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\rego.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\renpy.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\rest.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\rip.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\roboconf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\robotframework.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ruby.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\rust.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\sas.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\sass.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\scala.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\scss.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\shell-session.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\scheme.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\smali.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\smalltalk.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\smarty.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\sml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\solidity.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\solution-file.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\soy.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\sparql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\splunk-spl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\sqf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\sql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\squirrel.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\stan.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\stylus.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\swift.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\systemd.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\t4-cs.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\t4-templating.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\t4-vb.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\tap.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\tcl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\textile.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\toml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\tremor.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\tsx.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\tt2.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\turtle.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\twig.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\typescript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\typoscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\unrealscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\uri.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\v.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\vala.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\vbnet.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\velocity.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\verilog.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\vhdl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\vim.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\visual-basic.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\warpscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\wasm.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\web-idl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\wiki.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\wolfram.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\wren.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\xeora.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\xojo.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\xquery.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\yaml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\yang.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\languages\zig.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\a11y-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\atom-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\base16-ateliersulphurpool.light.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\cb.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\coldark-cold.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\coldark-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\coy.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\darcula.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\dracula.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\duotone-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\duotone-earth.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\duotone-forest.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\duotone-light.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\duotone-sea.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\duotone-space.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\funky.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\ghcolors.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\gruvbox-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\gruvbox-light.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\holi-theme.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\hopscotch.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\l.txt"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\lucario.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\material-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\material-light.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\material-oceanic.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\night-owl.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\nord.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\okaidia.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\one-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\one-light.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\pojoaque.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\prism.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\shades-of-purple.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\solarized-dark-atom.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\solarizedlight.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\synthwave84.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\tomorrow.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\twilight.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\vs.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\vsc-dark-plus.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\xonokai.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\data\themes\z-touch.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\DEPENDENCIES.md"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\lang\czech.slg"; DestDir: "{app}\plugins\textviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\lang\dutch.slg"; DestDir: "{app}\plugins\textviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\lang\english.slg"; DestDir: "{app}\plugins\textviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\lang\french.slg"; DestDir: "{app}\plugins\textviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\lang\german.slg"; DestDir: "{app}\plugins\textviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\lang\hungarian.slg"; DestDir: "{app}\plugins\textviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\textviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\lang\romanian.slg"; DestDir: "{app}\plugins\textviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\lang\russian.slg"; DestDir: "{app}\plugins\textviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\lang\slovak.slg"; DestDir: "{app}\plugins\textviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\lang\spanish.slg"; DestDir: "{app}\plugins\textviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\Microsoft.Web.WebView2.Core.dll"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\Microsoft.Web.WebView2.WinForms.dll"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\Newtonsoft.Json.dll"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\PrismSharp.dll"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\runtimes\win-arm64\native\WebView2Loader.dll"; DestDir: "{app}\plugins\textviewer\runtimes\win-arm64\native"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\runtimes\win-x64\native\WebView2Loader.dll"; DestDir: "{app}\plugins\textviewer\runtimes\win-x64\native"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\runtimes\win-x86\native\WebView2Loader.dll"; DestDir: "{app}\plugins\textviewer\runtimes\win-x86\native"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\SUPPORTED_FILE_TYPES.md"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\TextViewer.Managed.dll"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\textviewer.spl"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\runtimes\win-x64\native\WebView2Loader.dll"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\unarj\lang\czech.slg"; DestDir: "{app}\plugins\unarj\lang"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\unarj\lang\dutch.slg"; DestDir: "{app}\plugins\unarj\lang"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\unarj\lang\english.slg"; DestDir: "{app}\plugins\unarj\lang"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\unarj\lang\french.slg"; DestDir: "{app}\plugins\unarj\lang"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\unarj\lang\german.slg"; DestDir: "{app}\plugins\unarj\lang"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\unarj\lang\hungarian.slg"; DestDir: "{app}\plugins\unarj\lang"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\unarj\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\unarj\lang"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\unarj\lang\romanian.slg"; DestDir: "{app}\plugins\unarj\lang"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\unarj\lang\russian.slg"; DestDir: "{app}\plugins\unarj\lang"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\unarj\lang\slovak.slg"; DestDir: "{app}\plugins\unarj\lang"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\unarj\lang\spanish.slg"; DestDir: "{app}\plugins\unarj\lang"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\unarj\unarj.spl"; DestDir: "{app}\plugins\unarj"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\uncab\lang\czech.slg"; DestDir: "{app}\plugins\uncab\lang"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\uncab\lang\dutch.slg"; DestDir: "{app}\plugins\uncab\lang"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\uncab\lang\english.slg"; DestDir: "{app}\plugins\uncab\lang"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\uncab\lang\french.slg"; DestDir: "{app}\plugins\uncab\lang"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\uncab\lang\german.slg"; DestDir: "{app}\plugins\uncab\lang"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\uncab\lang\hungarian.slg"; DestDir: "{app}\plugins\uncab\lang"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\uncab\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\uncab\lang"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\uncab\lang\romanian.slg"; DestDir: "{app}\plugins\uncab\lang"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\uncab\lang\russian.slg"; DestDir: "{app}\plugins\uncab\lang"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\uncab\lang\slovak.slg"; DestDir: "{app}\plugins\uncab\lang"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\uncab\lang\spanish.slg"; DestDir: "{app}\plugins\uncab\lang"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\uncab\uncab.spl"; DestDir: "{app}\plugins\uncab"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\undelete\lang\czech.slg"; DestDir: "{app}\plugins\undelete\lang"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\undelete\lang\dutch.slg"; DestDir: "{app}\plugins\undelete\lang"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\undelete\lang\english.slg"; DestDir: "{app}\plugins\undelete\lang"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\undelete\lang\french.slg"; DestDir: "{app}\plugins\undelete\lang"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\undelete\lang\german.slg"; DestDir: "{app}\plugins\undelete\lang"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\undelete\lang\hungarian.slg"; DestDir: "{app}\plugins\undelete\lang"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\undelete\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\undelete\lang"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\undelete\lang\romanian.slg"; DestDir: "{app}\plugins\undelete\lang"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\undelete\lang\russian.slg"; DestDir: "{app}\plugins\undelete\lang"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\undelete\lang\slovak.slg"; DestDir: "{app}\plugins\undelete\lang"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\undelete\lang\spanish.slg"; DestDir: "{app}\plugins\undelete\lang"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\undelete\undelete.spl"; DestDir: "{app}\plugins\undelete"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\unfat\lang\czech.slg"; DestDir: "{app}\plugins\unfat\lang"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unfat\lang\dutch.slg"; DestDir: "{app}\plugins\unfat\lang"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unfat\lang\english.slg"; DestDir: "{app}\plugins\unfat\lang"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unfat\lang\french.slg"; DestDir: "{app}\plugins\unfat\lang"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unfat\lang\german.slg"; DestDir: "{app}\plugins\unfat\lang"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unfat\lang\hungarian.slg"; DestDir: "{app}\plugins\unfat\lang"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unfat\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\unfat\lang"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unfat\lang\romanian.slg"; DestDir: "{app}\plugins\unfat\lang"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unfat\lang\russian.slg"; DestDir: "{app}\plugins\unfat\lang"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unfat\lang\slovak.slg"; DestDir: "{app}\plugins\unfat\lang"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unfat\lang\spanish.slg"; DestDir: "{app}\plugins\unfat\lang"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unfat\unfat.spl"; DestDir: "{app}\plugins\unfat"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unchm\chmlib.dll"; DestDir: "{app}\plugins\unchm"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\lang\czech.slg"; DestDir: "{app}\plugins\unchm\lang"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\lang\dutch.slg"; DestDir: "{app}\plugins\unchm\lang"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\lang\english.slg"; DestDir: "{app}\plugins\unchm\lang"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\lang\french.slg"; DestDir: "{app}\plugins\unchm\lang"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\lang\german.slg"; DestDir: "{app}\plugins\unchm\lang"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\lang\hungarian.slg"; DestDir: "{app}\plugins\unchm\lang"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\unchm\lang"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\lang\romanian.slg"; DestDir: "{app}\plugins\unchm\lang"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\lang\russian.slg"; DestDir: "{app}\plugins\unchm\lang"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\lang\slovak.slg"; DestDir: "{app}\plugins\unchm\lang"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\lang\spanish.slg"; DestDir: "{app}\plugins\unchm\lang"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\unchm.spl"; DestDir: "{app}\plugins\unchm"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\uniso\lang\czech.slg"; DestDir: "{app}\plugins\uniso\lang"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\uniso\lang\dutch.slg"; DestDir: "{app}\plugins\uniso\lang"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\uniso\lang\english.slg"; DestDir: "{app}\plugins\uniso\lang"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\uniso\lang\french.slg"; DestDir: "{app}\plugins\uniso\lang"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\uniso\lang\german.slg"; DestDir: "{app}\plugins\uniso\lang"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\uniso\lang\hungarian.slg"; DestDir: "{app}\plugins\uniso\lang"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\uniso\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\uniso\lang"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\uniso\lang\romanian.slg"; DestDir: "{app}\plugins\uniso\lang"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\uniso\lang\russian.slg"; DestDir: "{app}\plugins\uniso\lang"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\uniso\lang\slovak.slg"; DestDir: "{app}\plugins\uniso\lang"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\uniso\lang\spanish.slg"; DestDir: "{app}\plugins\uniso\lang"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\uniso\uniso.spl"; DestDir: "{app}\plugins\uniso"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\unlha\lang\czech.slg"; DestDir: "{app}\plugins\unlha\lang"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unlha\lang\dutch.slg"; DestDir: "{app}\plugins\unlha\lang"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unlha\lang\english.slg"; DestDir: "{app}\plugins\unlha\lang"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unlha\lang\french.slg"; DestDir: "{app}\plugins\unlha\lang"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unlha\lang\german.slg"; DestDir: "{app}\plugins\unlha\lang"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unlha\lang\hungarian.slg"; DestDir: "{app}\plugins\unlha\lang"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unlha\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\unlha\lang"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unlha\lang\romanian.slg"; DestDir: "{app}\plugins\unlha\lang"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unlha\lang\russian.slg"; DestDir: "{app}\plugins\unlha\lang"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unlha\lang\slovak.slg"; DestDir: "{app}\plugins\unlha\lang"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unlha\lang\spanish.slg"; DestDir: "{app}\plugins\unlha\lang"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unlha\unlha.spl"; DestDir: "{app}\plugins\unlha"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unmime\lang\czech.slg"; DestDir: "{app}\plugins\unmime\lang"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unmime\lang\dutch.slg"; DestDir: "{app}\plugins\unmime\lang"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unmime\lang\english.slg"; DestDir: "{app}\plugins\unmime\lang"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unmime\lang\french.slg"; DestDir: "{app}\plugins\unmime\lang"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unmime\lang\german.slg"; DestDir: "{app}\plugins\unmime\lang"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unmime\lang\hungarian.slg"; DestDir: "{app}\plugins\unmime\lang"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unmime\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\unmime\lang"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unmime\lang\romanian.slg"; DestDir: "{app}\plugins\unmime\lang"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unmime\lang\russian.slg"; DestDir: "{app}\plugins\unmime\lang"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unmime\lang\slovak.slg"; DestDir: "{app}\plugins\unmime\lang"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unmime\lang\spanish.slg"; DestDir: "{app}\plugins\unmime\lang"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unmime\unmime.spl"; DestDir: "{app}\plugins\unmime"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unole\lang\czech.slg"; DestDir: "{app}\plugins\unole\lang"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unole\lang\dutch.slg"; DestDir: "{app}\plugins\unole\lang"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unole\lang\english.slg"; DestDir: "{app}\plugins\unole\lang"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unole\lang\french.slg"; DestDir: "{app}\plugins\unole\lang"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unole\lang\german.slg"; DestDir: "{app}\plugins\unole\lang"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unole\lang\hungarian.slg"; DestDir: "{app}\plugins\unole\lang"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unole\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\unole\lang"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unole\lang\romanian.slg"; DestDir: "{app}\plugins\unole\lang"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unole\lang\russian.slg"; DestDir: "{app}\plugins\unole\lang"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unole\lang\slovak.slg"; DestDir: "{app}\plugins\unole\lang"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unole\lang\spanish.slg"; DestDir: "{app}\plugins\unole\lang"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unole\unole.spl"; DestDir: "{app}\plugins\unole"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unrar\lang\czech.slg"; DestDir: "{app}\plugins\unrar\lang"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\lang\dutch.slg"; DestDir: "{app}\plugins\unrar\lang"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\lang\english.slg"; DestDir: "{app}\plugins\unrar\lang"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\lang\french.slg"; DestDir: "{app}\plugins\unrar\lang"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\lang\german.slg"; DestDir: "{app}\plugins\unrar\lang"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\lang\hungarian.slg"; DestDir: "{app}\plugins\unrar\lang"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\unrar\lang"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\lang\romanian.slg"; DestDir: "{app}\plugins\unrar\lang"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\lang\russian.slg"; DestDir: "{app}\plugins\unrar\lang"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\lang\slovak.slg"; DestDir: "{app}\plugins\unrar\lang"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\lang\spanish.slg"; DestDir: "{app}\plugins\unrar\lang"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\unrar.dll"; DestDir: "{app}\plugins\unrar"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\unrar.spl"; DestDir: "{app}\plugins\unrar"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\webview2renderviewer\DEPENDENCIES.md"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\lang\czech.slg"; DestDir: "{app}\plugins\webview2renderviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\lang\dutch.slg"; DestDir: "{app}\plugins\webview2renderviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\lang\english.slg"; DestDir: "{app}\plugins\webview2renderviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\lang\french.slg"; DestDir: "{app}\plugins\webview2renderviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\lang\german.slg"; DestDir: "{app}\plugins\webview2renderviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\lang\hungarian.slg"; DestDir: "{app}\plugins\webview2renderviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\webview2renderviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\lang\romanian.slg"; DestDir: "{app}\plugins\webview2renderviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\lang\russian.slg"; DestDir: "{app}\plugins\webview2renderviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\lang\slovak.slg"; DestDir: "{app}\plugins\webview2renderviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\lang\spanish.slg"; DestDir: "{app}\plugins\webview2renderviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\Markdig.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\Microsoft.Web.WebView2.Core.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\Microsoft.Web.WebView2.WinForms.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\Microsoft.Web.WebView2.Wpf.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\runtimes\win-arm64\native\WebView2Loader.dll"; DestDir: "{app}\plugins\webview2renderviewer\runtimes\win-arm64\native"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\runtimes\win-x64\native\WebView2Loader.dll"; DestDir: "{app}\plugins\webview2renderviewer\runtimes\win-x64\native"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\runtimes\win-x86\native\WebView2Loader.dll"; DestDir: "{app}\plugins\webview2renderviewer\runtimes\win-x86\native"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\SUPPORTED_FILE_TYPES.md"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\System.Buffers.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\System.Memory.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\System.Numerics.Vectors.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\System.Runtime.CompilerServices.Unsafe.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\System.Text.Encoding.CodePages.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\runtimes\win-x64\native\WebView2Loader.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\WebView2RenderViewer.Managed.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\webview2renderviewer.spl"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\wmobile\lang\czech.slg"; DestDir: "{app}\plugins\wmobile\lang"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\wmobile\lang\dutch.slg"; DestDir: "{app}\plugins\wmobile\lang"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\wmobile\lang\english.slg"; DestDir: "{app}\plugins\wmobile\lang"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\wmobile\lang\french.slg"; DestDir: "{app}\plugins\wmobile\lang"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\wmobile\lang\german.slg"; DestDir: "{app}\plugins\wmobile\lang"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\wmobile\lang\hungarian.slg"; DestDir: "{app}\plugins\wmobile\lang"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\wmobile\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\wmobile\lang"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\wmobile\lang\romanian.slg"; DestDir: "{app}\plugins\wmobile\lang"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\wmobile\lang\russian.slg"; DestDir: "{app}\plugins\wmobile\lang"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\wmobile\lang\slovak.slg"; DestDir: "{app}\plugins\wmobile\lang"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\wmobile\lang\spanish.slg"; DestDir: "{app}\plugins\wmobile\lang"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\wmobile\wmobile.spl"; DestDir: "{app}\plugins\wmobile"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\zip\lang\czech.slg"; DestDir: "{app}\plugins\zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\lang\dutch.slg"; DestDir: "{app}\plugins\zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\lang\english.slg"; DestDir: "{app}\plugins\zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\lang\french.slg"; DestDir: "{app}\plugins\zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\lang\german.slg"; DestDir: "{app}\plugins\zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\lang\hungarian.slg"; DestDir: "{app}\plugins\zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\lang\chinesesimplified.slg"; DestDir: "{app}\plugins\zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\lang\romanian.slg"; DestDir: "{app}\plugins\zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\lang\russian.slg"; DestDir: "{app}\plugins\zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\lang\slovak.slg"; DestDir: "{app}\plugins\zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\lang\spanish.slg"; DestDir: "{app}\plugins\zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\zip.spl"; DestDir: "{app}\plugins\zip"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\zip2sfx\readme.txt"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\zip2sfx\sam_cz.set"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\zip2sfx\sample.set"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\zip2sfx\zip2sfx.exe"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\salamatrix\salamatrix.spl"; DestDir: "{app}\plugins\salamatrix"; Flags: ignoreversion; Check: IsPluginSelected('salamatrix')
Source: "{#PayloadDir}\plugins\demoplug\demoplug.spl"; DestDir: "{app}\plugins\demoplug"; Flags: ignoreversion; Check: IsPluginSelected('demoplug')
Source: "{#PayloadDir}\plugins\demoplug\lang\*"; DestDir: "{app}\plugins\demoplug\lang"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('demoplug')
Source: "{#PayloadDir}\salamand.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\salamand.exe.config"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\configstorage.ini"; DestDir: "{app}"; Flags: onlyifdoesntexist; Permissions: users-modify; Check: ShouldInstallConfigStorage
Source: "{#PayloadDir}\toolbars\Back.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\AzureCloudShell.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\CommandPrompt.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\PowerShell.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\VisualStudio.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\WindowsPowerShell.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\WindowsTerminal.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\CalculateDirectorySizes.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\CalculateOccupiedSpace.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ClipboardCut.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ClipboardCopy.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ClipboardPaste.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\CommandShell.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\CompareDirectories.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Configuration.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ConnectNetworkDrive.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Convert.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Copy.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\CreateDirectory.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Delete.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Disconnect.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\DriveInformation.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Edit.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\EditNewFile.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Email.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Filter.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\FindFilesAndDirectories.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\FocusNameInOtherPanel.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Forward.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\GoToHotPath.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\GoToPathfromOtherPanel.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\GoToShortcutTarget.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\HelpContents.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\HideSelectedNames.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\HideUnselectedNames.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\InvertSelection.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\RestoreSelection.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Select.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SelectAll.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Unselect.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\UnselectAll.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ChangeAttributes.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ChangeCase.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ChangeDirectory.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Modify.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Move.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\MoveItemBottom.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\MoveItemDown.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\MoveItemTop.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\MoveItemUp.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\New.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\NTFSCompress.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\NTFSUncompress.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\OpenFolder.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\OpenNameinOtherPanel.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Pack.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ParentDirectory.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\PasteShortcut.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Properties.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\QuickRename.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Refresh.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\RootDirectory.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Salamand.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Security.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SharedDirectories.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ShowHiddenNames.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SmartColumnMode.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SortByAttributes.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SortByDate.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SortByExtension.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SortByName.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SortBySize.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SwapPanels.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\TabsClose.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\TabsDuplicate.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\TabsNew.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\TabsNext.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\TabsPrevious.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Unpack.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\UserMenu.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\View.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Views.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\WhatIsThis.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Windows.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Blue.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Copy.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Crop.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\First.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\FlipHorizontal.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\FlipVertical.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\FullScreen.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Green.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Hand.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Help.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Last.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Luminosity.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Next.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\NextPage.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\NextSelectedFile.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Open.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\OtherChannels.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Paste.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Pipette.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\PrevPage.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Previous.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\PreviousSelectedFile.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Print.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Properties.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\RGBSum.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Red.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Rotate180.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\RotateLeft.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\RotateRight.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Save.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Select.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\SelectSourceFile.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\StatusAnchor.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\StatusCursor.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\StatusPipette.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\StatusSize.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\Zoom.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\ZoomActual.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\ZoomIn.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\ZoomOut.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\ZoomWhole.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\pictview\ZoomWidth.svg"; DestDir: "{app}\toolbars\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Back.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\AzureCloudShell.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\CalculateDirectorySizes.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\CalculateOccupiedSpace.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ClipboardCut.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ClipboardCopy.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ClipboardPaste.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\CommandShell.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\CompareDirectories.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Configuration.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ConnectNetworkDrive.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Convert.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Copy.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\CreateDirectory.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Delete.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Disconnect.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\DriveInformation.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Edit.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\EditNewFile.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Email.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Filter.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\FindFilesAndDirectories.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\FocusNameInOtherPanel.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Forward.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\GoToHotPath.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\GoToPathfromOtherPanel.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\GoToShortcutTarget.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\HelpContents.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\HideSelectedNames.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\HideUnselectedNames.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\InvertSelection.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\RestoreSelection.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Select.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\SelectAll.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Unselect.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\UnselectAll.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ChangeAttributes.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ChangeCase.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ChangeDirectory.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Modify.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Move.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\MoveItemBottom.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\MoveItemDown.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\MoveItemTop.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\MoveItemUp.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\New.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\NTFSCompress.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\NTFSUncompress.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\OpenFolder.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\OpenNameinOtherPanel.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Pack.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ParentDirectory.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\PasteShortcut.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Properties.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\QuickRename.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Refresh.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\RootDirectory.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Salamand.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Security.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\SharedDirectories.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ShowHiddenNames.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\SmartColumnMode.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\SortByAttributes.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\SortByDate.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\SortByExtension.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\SortByName.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\SortBySize.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\SwapPanels.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\TabsClose.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\TabsDuplicate.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\TabsNew.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\TabsNext.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\TabsPrevious.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Unpack.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\UserMenu.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\View.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Views.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\WhatIsThis.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Windows.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Blue.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Copy.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Crop.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\First.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\FlipHorizontal.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\FlipVertical.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\FullScreen.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Green.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Hand.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Help.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Last.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Luminosity.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Next.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\NextPage.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\NextSelectedFile.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Open.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\OtherChannels.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Paste.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Pipette.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\PrevPage.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Previous.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\PreviousSelectedFile.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Print.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Properties.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\RGBSum.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Red.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Rotate180.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\RotateLeft.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\RotateRight.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Save.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Select.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\SelectSourceFile.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\StatusAnchor.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\StatusCursor.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\StatusPipette.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\StatusSize.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\Zoom.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\ZoomActual.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\ZoomIn.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\ZoomOut.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\ZoomWhole.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\pictview\ZoomWidth.svg"; DestDir: "{app}\toolbars\darkmode\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\ucrtbase.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\dbghelp.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\libeay32.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\muires.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\salextx64.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\salextx86.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\salmon.exe"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\salopen.exe"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\salspawn.exe"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\sqlite.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\ssleay32.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\vcruntime140.dll"; DestDir: "{app}"; Flags: ignoreversion
[Icons]
Name: "{autodesktop}\Open Salamander Samandarin (x64)"; Filename: "{app}\{#AppToInstallExeName}"; WorkingDir: "{app}"; Check: ShouldCreateShortcut('desktop')
Name: "{group}\Open Salamander Samandarin (x64)"; Filename: "{app}\{#AppToInstallExeName}"; WorkingDir: "{app}"; Check: ShouldCreateShortcut('startmenu')

[Run]
Filename: "{app}\{#AppToInstallExeName}"; Parameters: "-welcome -language ""{language}"""; Description: "{cm:LaunchProgram}"; Flags: nowait postinstall; Check: ShouldRunApplication


[UninstallRun]
; INF [DelShellExts] listed these shell-extension DLLs for cleanup.
Filename: "{sys}\regsvr32.exe"; Parameters: "/u /s ""{app}\utils\salextx64.dll"""; Flags: runhidden; RunOnceId: "UnregisterSalExtX64"
Filename: "{sys}\regsvr32.exe"; Parameters: "/u /s ""{app}\utils\salextx86.dll"""; Flags: runhidden; RunOnceId: "UnregisterSalExtX86"

[Code]


var
  InstallModePage: TInputOptionWizardPage;
  PluginSelectionPage: TWizardPage;
  PluginList: TNewCheckListBox;
  PluginIds: array of String;
  DeleteUserConfiguration: Boolean;
  DeleteUserConfigurationFromFile: Boolean;
  DeleteUserConfigurationFilePath: String;
  PreviousVersionUninstallKeys: array of String;
  SilentInstallMode: String;
  SilentPluginSelection: String;
  SilentShortcuts: String;
  SilentOverwriteDir: String;
  SilentRunApp: String;




function IsPortableInstall(): Boolean; forward;

function GetLowerParam(const Name, Default: String): String;
begin
  Result := Lowercase(ExpandConstant('{param:' + Name + '|' + Default + '}'));
end;

function ListContainsToken(const List, Token: String): Boolean;
var
  NormalizedList: String;
  NormalizedToken: String;
begin
  NormalizedList := Lowercase(List);
  StringChangeEx(NormalizedList, ';', ',', True);
  StringChangeEx(NormalizedList, ' ', '', True);
  NormalizedToken := Lowercase(Token);
  Result := Pos(',' + NormalizedToken + ',', ',' + NormalizedList + ',') > 0;
end;

function IsDefaultPlugin(const PluginId: String): Boolean;
begin
  Result := not (
    (CompareText(PluginId, 'folders') = 0) or
    (CompareText(PluginId, 'ieviewer') = 0) or
    (CompareText(PluginId, 'wmobile') = 0) or
    (CompareText(PluginId, 'demoplug') = 0));
end;

function ShouldCreateShortcut(const ShortcutId: String): Boolean;
begin
  Result := False;
  if IsPortableInstall() then
    Exit;

  if SilentShortcuts <> '' then
  begin
    Result := (SilentShortcuts = 'all') or ListContainsToken(SilentShortcuts, ShortcutId);
    Exit;
  end;

  if CompareText(ShortcutId, 'desktop') = 0 then
    Result := IsTaskSelected('desktopicon')
  else if CompareText(ShortcutId, 'startmenu') = 0 then
    Result := IsTaskSelected('startmenuicon')
  else
    Result := False;
end;

function ShouldRunApplication(): Boolean;
begin
  Result := (not WizardSilent) or (SilentRunApp = 'yes') or (SilentRunApp = 'y') or (SilentRunApp = '1') or (SilentRunApp = 'true');
end;

function PadLeft(const Value: String; const Width: Integer): String;
begin
  Result := Value;
  while Length(Result) < Width do
    Result := ' ' + Result;
end;

function PadRight(const Value: String; const Width: Integer): String;
begin
  Result := Value;
  while Length(Result) < Width do
    Result := Result + ' ';
end;

function GetInstalledPluginVersion(const PluginId: String): String;
var
  I: Integer;
  Count: Integer;
  PluginPath: String;
  ExpectedPath: String;
begin
  Result := '';
  ExpectedPath := 'plugins/' + PluginId + '/' + PluginId + '.spl';
  Count := StrToIntDef(GetIniString('InstalledPlugins', 'Count', '0', ExpandConstant('{app}\configstorage.ini')), 0);

  for I := 1 to Count do
  begin
    PluginPath := GetIniString('InstalledPlugins', 'Plugin' + IntToStr(I) + 'Path', '', ExpandConstant('{app}\configstorage.ini'));
    StringChangeEx(PluginPath, '\', '/', True);
    if CompareText(PluginPath, ExpectedPath) = 0 then
    begin
      Result := GetIniString('InstalledPlugins', 'Plugin' + IntToStr(I) + 'Version', '', ExpandConstant('{app}\configstorage.ini'));
      Exit;
    end;
  end;
end;

function EnsureX64Suffix(const Version: String): String;
begin
  Result := Version;
  if Pos('(x64)', Result) = 0 then
    Result := Result + ' (x64)';
end;

function StripX64Suffix(const Version: String): String;
begin
  Result := Version;
  StringChangeEx(Result, '(x64)', '', True);
  Result := Trim(Result);
end;

function ExtractNextVersionNumber(var Version: String): Integer;
var
  I: Integer;
  Part: String;
begin
  I := Pos('.', Version);
  if I = 0 then
  begin
    Part := Version;
    Version := '';
  end
  else
  begin
    Part := Copy(Version, 1, I - 1);
    Version := Copy(Version, I + 1, Length(Version) - I);
  end;

  Result := StrToIntDef(Part, 0);
end;

function ComparePluginVersions(LeftVersion, RightVersion: String): Integer;
var
  LeftPart: Integer;
  RightPart: Integer;
begin
  LeftVersion := StripX64Suffix(LeftVersion);
  RightVersion := StripX64Suffix(RightVersion);
  Result := 0;

  while (LeftVersion <> '') or (RightVersion <> '') do
  begin
    LeftPart := ExtractNextVersionNumber(LeftVersion);
    RightPart := ExtractNextVersionNumber(RightVersion);

    if LeftPart > RightPart then
    begin
      Result := 1;
      Exit;
    end
    else if LeftPart < RightPart then
    begin
      Result := -1;
      Exit;
    end;
  end;
end;

function IsInstallerPluginVersionNewer(const InstallerVersion, InstalledVersion: String): Boolean;
begin
  Result := (InstalledVersion <> '') and (ComparePluginVersions(InstallerVersion, InstalledVersion) > 0);
end;

function FormatPluginVersionText(const InstallerVersion, InstalledVersion: String): String;
begin
  if InstalledVersion <> '' then
    Result := PadRight(CustomMessage('InstalledVersionLabel') + ' ' + EnsureX64Suffix(InstalledVersion), 28)
  else
    Result := PadRight('', 28);
  Result := Result + '🗜︎ ' + EnsureX64Suffix(InstallerVersion);
end;

procedure AddPlugin(const PluginId, DisplayName, Version: String; const CheckedByDefault: Boolean);
var
  InstalledVersion: String;
  Checked: Boolean;
begin
  InstalledVersion := GetInstalledPluginVersion(PluginId);
  Checked := CheckedByDefault or (InstalledVersion <> '');
  PluginList.AddCheckBox(
    DisplayName,
    FormatPluginVersionText(Version, InstalledVersion),
    0,
    Checked,
    True,
    False,
    False,
    nil);
  if IsInstallerPluginVersionNewer(Version, InstalledVersion) then
    PluginList.SubItemFontStyle[PluginList.Items.Count - 1] := [fsBold];
  SetArrayLength(PluginIds, GetArrayLength(PluginIds) + 1);
  PluginIds[GetArrayLength(PluginIds) - 1] := PluginId;
end;

function IsPluginSelected(const PluginId: String): Boolean;
var
  I: Integer;
begin
  if SilentPluginSelection <> '' then
  begin
    if SilentPluginSelection = 'all' then
      Result := True
    else if SilentPluginSelection = 'none' then
      Result := False
    else if SilentPluginSelection = 'default' then
      Result := IsDefaultPlugin(PluginId)
    else
      Result := ListContainsToken(SilentPluginSelection, PluginId);
    Exit;
  end;

  Result := True;
  if not Assigned(PluginList) then
    Exit;

  for I := 0 to GetArrayLength(PluginIds) - 1 do
  begin
    if CompareText(PluginIds[I], PluginId) = 0 then
    begin
      Result := PluginList.Checked[I];
      Exit;
    end;
  end;
end;

function IsPortableInstall(): Boolean;
begin
  if SilentInstallMode <> '' then
  begin
    Result := (SilentInstallMode = 'portable') or (SilentInstallMode = 'extract');
    Exit;
  end;

  Result := Assigned(InstallModePage) and (InstallModePage.SelectedValueIndex = 1);
end;

function GetStandardDefaultDir(): String;
begin
  Result := ExpandConstant('{autopf}\Open Salamander Samandarin');
end;

function GetPortableDefaultDir(): String;
begin
  Result := ExpandConstant('{userdocs}\') + CustomMessage('PortableDirName');
end;

function GetDefaultDirName(Param: String): String;
begin
  if IsPortableInstall() then
    Result := GetPortableDefaultDir()
  else
    Result := GetStandardDefaultDir();
end;

function ShouldInstallConfigStorage(): Boolean;
var
  ConfigPath: String;
  BackupPath: String;
  Msg: String;
begin
  Result := True;
  ConfigPath := ExpandConstant('{app}\configstorage.ini');

  if FileExists(ConfigPath) then
  begin
    //BackupPath := ConfigPath + '.BAK';
    //DeleteFile(BackupPath);
    //RenameFile(ConfigPath, BackupPath);
    Result := False;

    if not WizardSilent then
    begin
      //Msg := CustomMessage('KeepConfigQuestion');
      //StringChangeEx(Msg, '\n', #13#10, True);
      //MsgBox(Msg, mbInformation, MB_OK);
    end;
  end;
end;

procedure AddPreviousVersionUninstallKey(const RootKey: Integer; const SubKeyName: String);
var
  InstallLocation: String;
  DisplayName: String;
  UninstallString: String;
begin
  if RegQueryStringValue(RootKey, SubKeyName, 'InstallLocation', InstallLocation) and
     (CompareText(RemoveBackslashUnlessRoot(InstallLocation), RemoveBackslashUnlessRoot(ExpandConstant('{app}'))) = 0) and
     RegQueryStringValue(RootKey, SubKeyName, 'DisplayName', DisplayName) and
     (Pos('Open Salamander', DisplayName) = 1) and
     RegQueryStringValue(RootKey, SubKeyName, 'UninstallString', UninstallString) and
     FileExists(RemoveQuotes(UninstallString)) then
  begin
    SetArrayLength(PreviousVersionUninstallKeys, GetArrayLength(PreviousVersionUninstallKeys) + 1);
    PreviousVersionUninstallKeys[GetArrayLength(PreviousVersionUninstallKeys) - 1] := SubKeyName;
  end;
end;

procedure CollectPreviousVersionUninstallKeys(const RootKey: Integer; const UninstallRoot: String);
var
  Names: array of String;
  I: Integer;
begin
  if not RegGetSubkeyNames(RootKey, UninstallRoot, Names) then
    Exit;

  for I := 0 to GetArrayLength(Names) - 1 do
  begin
    if (CompareText(Names[I], ExpandConstant('{#SetupSetting("AppId")}') + '_is1') <> 0) and
       (Pos('OpenSalamanderSamandarin-x64-', Names[I]) = 1) then
    begin
      AddPreviousVersionUninstallKey(RootKey, UninstallRoot + '\' + Names[I]);
    end;
  end;
end;

procedure CollectPreviousVersionUninstallKeysForAppDir;
begin
  SetArrayLength(PreviousVersionUninstallKeys, 0);
  if IsPortableInstall() then
    Exit;

  CollectPreviousVersionUninstallKeys(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall');
  CollectPreviousVersionUninstallKeys(HKLM64, 'Software\Microsoft\Windows\CurrentVersion\Uninstall');
end;

procedure RemovePreviousVersionUninstallKeys;
var
  I: Integer;
begin
  for I := 0 to GetArrayLength(PreviousVersionUninstallKeys) - 1 do
  begin
    RegDeleteKeyIncludingSubkeys(HKLM, PreviousVersionUninstallKeys[I]);
    RegDeleteKeyIncludingSubkeys(HKLM64, PreviousVersionUninstallKeys[I]);
  end;
end;

function GetACP: DWORD;
  external 'GetACP@kernel32.dll stdcall';

function GetExpectedCodePageForLanguage(const LanguageName: String): DWORD;
begin
  // Map language names to expected Windows code pages
  // English is not included here because ASCII works on all code pages
  if (CompareText(LanguageName, 'czech') = 0) or
     (CompareText(LanguageName, 'slovak') = 0) or
     (CompareText(LanguageName, 'hungarian') = 0) or
     (CompareText(LanguageName, 'polish') = 0) or
     (CompareText(LanguageName, 'croatian') = 0) or
     (CompareText(LanguageName, 'slovenian') = 0) or
     (CompareText(LanguageName, 'latvian') = 0) or
     (CompareText(LanguageName, 'lithuanian') = 0) or
     (CompareText(LanguageName, 'estonian') = 0) or
     (CompareText(LanguageName, 'romanian') = 0) then
    Result := 1250  // Central Europe
  else if (CompareText(LanguageName, 'russian') = 0) or
          (CompareText(LanguageName, 'ukrainian') = 0) or
          (CompareText(LanguageName, 'bulgarian') = 0) then
    Result := 1251  // Cyrillic
  else if (CompareText(LanguageName, 'german') = 0) or
          (CompareText(LanguageName, 'french') = 0) or
          (CompareText(LanguageName, 'spanish') = 0) or
          (CompareText(LanguageName, 'italian') = 0) or
          (CompareText(LanguageName, 'dutch') = 0) or
          (CompareText(LanguageName, 'portuguese') = 0) or
          (CompareText(LanguageName, 'swedish') = 0) or
          (CompareText(LanguageName, 'norwegian') = 0) or
          (CompareText(LanguageName, 'danish') = 0) or
          (CompareText(LanguageName, 'finnish') = 0) then
    Result := 1252  // Western Europe
  else if (CompareText(LanguageName, 'japanese') = 0) then
    Result := 932   // Shift-JIS
  else if (CompareText(LanguageName, 'chinesesimplified') = 0) then
    Result := 936   // GBK
  else if (CompareText(LanguageName, 'chinesetraditional') = 0) then
    Result := 950   // Big5
  else if (CompareText(LanguageName, 'korean') = 0) then
    Result := 949   // Korean
  else
    Result := 0;    // unknown or English (ASCII works everywhere)
end;

procedure CheckCodePageCompatibility;
var
  SystemCP: DWORD;
  ExpectedCP: DWORD;
  Msg: String;
  LanguageName: String;
begin
  SystemCP := GetACP;
  // Skip check if system code page is UTF-8 (65001) - Windows 11 Unicode beta support works with all languages
  if SystemCP = 65001 then
    Exit;

  LanguageName := ExpandConstant('{language}');
  ExpectedCP := GetExpectedCodePageForLanguage(LanguageName);
  if (ExpectedCP <> 0) and (SystemCP <> ExpectedCP) then
  begin
    Msg := Format(CustomMessage('CodePageWarning'), [SystemCP, ExpectedCP]);
    // Replace \n with actual newlines
    StringChangeEx(Msg, '\n', #13#10, True);
    MsgBox(Msg, mbError, MB_OK);
  end;
end;

procedure PopulatePluginList();
begin
  PluginList.Items.Clear;
  SetArrayLength(PluginIds, 0);

  AddPlugin('7zip', '7-Zip', '1.34 (x64)', True);
  AddPlugin('automation', 'Automation', '2.0 (x64)', True);
  AddPlugin('checksum', 'Checksum', '2.3 (x64)', True);
  AddPlugin('dbviewer', 'Database Viewer', '1.25 (x64)', True);
  AddPlugin('diskmap', 'DiskMap', '1.13 (x64)', True);
  AddPlugin('filecomp', 'File Comparator', '1.20 (x64)', True);
  AddPlugin('folders', 'Folders', '0.2 (x64)', False);
  AddPlugin('ftp', 'FTP Client', '1.36 (x64)', True);
  AddPlugin('hypervm', 'Hyper-V Machines', '1.07 (x64)', True);
  AddPlugin('ieviewer', 'Internet Explorer Viewer', '1.12 (x64)', False);
  AddPlugin('jsonviewer', 'JSON Viewer .NET', '1.02 (x64)', True);
  AddPlugin('mmviewer', 'Multimedia Viewer', '1.16 (x64)', True);
  AddPlugin('nethood', 'Network', '1.09 (x64)', True);
  AddPlugin('pak', 'PAK', '1.72 (x64)', True);
  AddPlugin('pictview', 'PictView', '2.24 (x64)', True);
  AddPlugin('portables', 'Portable Devices', '0.4 (x64)', True);
  AddPlugin('peviewer', 'Portable Executable Viewer', '3.0 (x64)', True);
  AddPlugin('textviewer', 'PrismSharp Text Viewer .NET', '1.02 (x64)', True);
  AddPlugin('regedt', 'Registry Editor', '1.15 (x64)', True);
  AddPlugin('renamer', 'Renamer', '1.15 (x64)', True);
  AddPlugin('samandarin', 'Samandarin Update Notifier', '0.7 (x64)', True);
  AddPlugin('serviceexplorer', 'Service Explorer', '0.013 (x64)', True);
  AddPlugin('splitcbn', 'Split & Combine', '1.11 (x64)', True);
  AddPlugin('tar', 'TAR', '3.34 (x64)', True);
  AddPlugin('unarj', 'UnARJ', '1.22 (x64)', True);
  AddPlugin('uncab', 'UnCAB', '1.28 (x64)', True);
  AddPlugin('unchm', 'UnCHM', '1.05 (x64)', True);
  AddPlugin('unfat', 'UnFAT', '1.2 (x64)', True);
  AddPlugin('uniso', 'UnISO', '1.38 (x64)', True);
  AddPlugin('unlha', 'UnLHA', '1.14 (x64)', True);
  AddPlugin('unmime', 'UnMIME', '1.15 (x64)', True);
  AddPlugin('unole', 'UnOLE', '1.02 (x64)', True);
  AddPlugin('unrar', 'UnRAR', '3.03 (x64)', True);
  AddPlugin('undelete', 'Undelete', '1.12 (x64)', True);
  AddPlugin('webview2renderviewer', 'WebView2 Render Viewer .NET', '1.03 (x64)', True);
  AddPlugin('wmobile', 'Windows Mobile', '1.09 (x64)', False);
  AddPlugin('zip', 'ZIP', '1.7 (x64)', True);
  AddPlugin('demoplug', '[DEV] DemoPlug', '1.96 (x64)', False);
  AddPlugin('salamatrix', '[DEV] Salamatrix Framework', '0.1 (x64)', True);
end;

function InitializeSetup(): Boolean;
begin
  Result := True;
  SilentInstallMode := GetLowerParam('INSTALLMODE', '');
  SilentPluginSelection := GetLowerParam('PLUGINS', '');
  SilentShortcuts := GetLowerParam('SHORTCUTS', '');
  SilentOverwriteDir := GetLowerParam('OVERWRITEDIR', 'yes');
  SilentRunApp := GetLowerParam('RUNAPP', 'no');

  if WizardSilent and (SilentOverwriteDir = 'no') and DirExists(ExpandConstant('{app}')) then
  begin
    Log('Silent setup aborted because /OVERWRITEDIR=no and the destination directory exists.');
    Result := False;
    Exit;
  end;

  if not WizardSilent then
    CheckCodePageCompatibility;
end;

procedure InitializeWizard();
begin
  InstallModePage := CreateInputOptionPage(
    wpLicense,
    SetupMessage(msgWizardSelectTasks),
    '',
    CustomMessage('InstallMode'),
    True,
    False);
  InstallModePage.Add(CustomMessage('StandardInstall'));
  InstallModePage.Add(CustomMessage('PortableInstall'));
  if (SilentInstallMode = 'portable') or (SilentInstallMode = 'extract') then
    InstallModePage.SelectedValueIndex := 1
  else
    InstallModePage.SelectedValueIndex := 0;

  PluginSelectionPage := CreateCustomPage(
    wpSelectDir,
    CustomMessage('PluginSelectionTitle'),
    CustomMessage('PluginSelectionDescription'));

  PluginList := TNewCheckListBox.Create(PluginSelectionPage);
  PluginList.Parent := PluginSelectionPage.Surface;
  PluginList.Left := 0;
  PluginList.Top := 0;
  PluginList.Width := PluginSelectionPage.SurfaceWidth;
  PluginList.Height := PluginSelectionPage.SurfaceHeight - PluginList.Top - ScaleY(8);

end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;

  if CurPageID = InstallModePage.ID then
  begin
    if IsPortableInstall() and (WizardDirValue = GetStandardDefaultDir()) then
      WizardForm.DirEdit.Text := GetPortableDefaultDir()
    else if (not IsPortableInstall()) and (WizardDirValue = GetPortableDefaultDir()) then
      WizardForm.DirEdit.Text := GetStandardDefaultDir();
  end;
end;

function IsFileConfigurationStorageSelected(): Boolean;
var
  StorageType: String;
begin
  StorageType := GetIniString(
    'Configuration',
    'StorageType',
    'Registry',
    ExpandConstant('{app}\configstorage.ini'));
  Result := CompareText(StorageType, 'RegFile') = 0;
end;

function GetFileConfigurationPath(): String;
begin
  Result := GetIniString(
    'Configuration',
    'RegFilePath',
    '',
    ExpandConstant('{app}\configstorage.ini'));

  if Result = '' then
  begin
    Result := ExpandConstant('{app}\config.reg');
  end;
end;


function InitializeUninstall(): Boolean;
begin
  Result := True;
  DeleteUserConfiguration := False;
  DeleteUserConfigurationFilePath := '';
  DeleteUserConfigurationFromFile := IsFileConfigurationStorageSelected();

  if UninstallSilent then
    Exit;

  if DeleteUserConfigurationFromFile then
  begin
    DeleteUserConfigurationFilePath := GetFileConfigurationPath();

    if FileExists(DeleteUserConfigurationFilePath) or FileExists(ExpandConstant('{app}\configstorage.ini')) then
    begin
      DeleteUserConfiguration :=
        MsgBox(
          CustomMessage('RemoveUserConfigQuestion') + #13#10#13#10 +
          CustomMessage('FileStorage') + #13#10 +
          DeleteUserConfigurationFilePath + #13#10#13#10 +
          CustomMessage('RemoveUserConfigFiles'),
          mbConfirmation,
          MB_YESNO) = IDYES;
    end;
  end
  else if RegKeyExists(HKCU, {#AppToInstallRegPath}) then
  begin
    DeleteUserConfiguration :=
      MsgBox(
        CustomMessage('RemoveUserConfigQuestion') + #13#10#13#10 +
        CustomMessage('RegistryKey') + #13#10 +
        {#AppToInstallRegPathRem} + #13#10#13#10 +
        CustomMessage('RemoveUserConfigRegistry'),
        mbConfirmation,
        MB_YESNO) = IDYES;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if (CurUninstallStep = usPostUninstall) and DeleteUserConfiguration then
  begin
    if DeleteUserConfigurationFromFile then
    begin
      DeleteFile(DeleteUserConfigurationFilePath);
      DeleteFile(ExpandConstant('{app}\configstorage.ini'));
    end
    else
    begin
      RegDeleteKeyIncludingSubkeys(HKCU, {#AppToInstallRegPath});
      DeleteFile(ExpandConstant('{app}\configstorage.ini'));
    end;
  end;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if Assigned(PluginSelectionPage) and (CurPageID = PluginSelectionPage.ID) then
    PopulatePluginList();

  { The finished page otherwise shows Inno Setup's default large bitmap on the left. }
  WizardForm.WizardBitmapImage.Visible := CurPageID <> wpFinished;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  PluginsVer: String;
begin
  if CurStep = ssPostInstall then
  begin
    CollectPreviousVersionUninstallKeysForAppDir;
    RemovePreviousVersionUninstallKeys;

    { Mirrors the setup_x64.inf IncrementFileContent metadata by ensuring plugins.ver exists.
      The legacy installer used the registry value
      HKCU\Software\Open Salamander Samandarin\5.0-samandarin-(samandarinVersion)\Configuration\Plugins.ver Version (x64)
      to decide whether selected plugins should be appended. Inno installs the staged plugins directly. }
    PluginsVer := ExpandConstant('{app}\plugins\plugins.ver');
    if not FileExists(PluginsVer) then
      SaveStringToFile(PluginsVer, '', False);

  end;
end;
