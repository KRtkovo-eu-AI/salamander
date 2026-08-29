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
;   /PLUGINS=<default|all|none|id,..> Mirrors the plugin selection page (takes precedence over /PLUGINCONFIG).
;   /PLUGINCONFIG=<base|standard|advanced|expert|custom> Selects a plugin preset when /PLUGINS is not supplied.
;   /SHORTCUTS=<none|startmenu|desktop|startmenu,desktop> Mirrors the shortcuts tasks; ignored for portable mode.
;   /RUNAPP=<yes|no>                 Controls launching the app after install; defaults to no during silent setup.
;
; Custom [Code] message boxes are guarded with WizardSilent/UninstallSilent so
; unattended installs and uninstalls do not block on application prompts.
;
; If /DPayloadDir is not supplied, OPENSAL_BUILD_DIR is used when present; otherwise
; the fallback path below is relative to this .iss file.

#define AppToInstallName "Open Salamander Samandarin"
#define SamandarinVersion "0.15.1"
#define AppToInstallDisplayName "Open Salamander 5.0 Samandarin " + SamandarinVersion + " (x64)"
#define AppToInstallVersion "5.0-samandarin-" + SamandarinVersion
#define AppToInstallPublisher "Ondřej Kotas (KRtekTM)"
#define AppToInstallCopyright "© 2026 " + AppToInstallPublisher + " KRtkovo.eu"
#define AppToInstallURL "https://samandarin.net/"
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
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"; LicenseFile: "{#SourcePath}\license.italian.txt"
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
italian.RemoveUserConfigQuestion=Vuoi rimuovere la configurazione utente di Open Salamander Samandarin?
italian.FileStorage=Archiviazione file:
italian.RegistryKey=Chiave del Registro di sistema:
italian.RemoveUserConfigFiles=Scegli Sì per eliminare i file di configurazione oppure No per mantenere le impostazioni.
italian.RemoveUserConfigRegistry=Scegli Sì per eliminare la chiave e tutto il suo contenuto oppure No per mantenere le impostazioni.
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
italian.InstallMode=Modalità di installazione:
italian.StandardInstall=Installa normalmente (menu Start, Programmi e programma di disinstallazione)
italian.PortableInstall=Estrai come versione portatile (nessuna voce in Programmi né programma di disinstallazione)
italian.PortableDirName=samandarin
italian.StartMenuShortcut=Crea un collegamento nel menu &Start
italian.DesktopShortcut=Crea un collegamento sul &desktop
italian.Shortcuts=Collegamenti:
italian.LaunchProgram=Avvia {#AppToInstallName}
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
italian.CodePageWarningTitle=Avviso di compatibilità della pagina di codice
italian.CodePageWarning=La lingua selezionata potrebbe non visualizzare correttamente tutti i caratteri perché la pagina di codice delle impostazioni locali di sistema di Windows (%u) non corrisponde a quella prevista per questa lingua (%u).\n\nPer risolvere il problema, modifica le impostazioni locali di sistema in Windows: Impostazioni > Data/ora e lingua > Lingua e area geografica > Impostazioni lingua amministrative > Modifica impostazioni locali di sistema.
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
italian.KeepConfigQuestion=È stato trovato un file di configurazione esistente (configstorage.ini).\n\nVerrà rinominato configstorage.ini.BAK e verrà installata la nuova configurazione predefinita.
romanian.KeepConfigQuestion=A fost găsit un fișier de configurare existent (configstorage.ini).\n\nAcesta va fi redenumit în configstorage.ini.BAK și va fi instalată noua configurație implicită.
russian.KeepConfigQuestion=Обнаружен существующий файл конфигурации (configstorage.ini).\n\nОн будет переименован в configstorage.ini.BAK и установлена новая конфигурация по умолчанию.
slovak.KeepConfigQuestion=Bol nájdený existujúci konfiguračný súbor (configstorage.ini).\n\nBude premenovaný na configstorage.ini.BAK a nainštaluje sa nová predvolená konfigurácia.
spanish.KeepConfigQuestion=Se encontró un archivo de configuración existente (configstorage.ini).\n\nSerá renombrado a configstorage.ini.BAK y se instalará la nueva configuración predeterminada.
english.PluginSelectionTitle=Select plugins
english.PluginSelectionDescription=Choose which bundled plugins will be installed or extracted.
english.PluginConfigurationLabel=Plugin configuration:
english.PluginConfigurationBase=Base (local only, no Internet access)
english.PluginConfigurationStandard=Standard (for most users)
english.PluginConfigurationAdvanced=Advanced (for advanced users)
english.PluginConfigurationExpert=Expert (for experts and developers)
english.PluginConfigurationCustom=Custom
chinesesimplified.PluginSelectionTitle=选择插件
chinesesimplified.PluginSelectionDescription=选择要安装或解压的随附插件。
chinesesimplified.PluginConfigurationLabel=插件配置：
chinesesimplified.PluginConfigurationBase=基础（仅本地，无 Internet 访问）
chinesesimplified.PluginConfigurationStandard=标准（适合大多数用户）
chinesesimplified.PluginConfigurationAdvanced=高级（适合高级用户）
chinesesimplified.PluginConfigurationExpert=专家（适合专家和开发者）
chinesesimplified.PluginConfigurationCustom=自定义
dutch.PluginSelectionTitle=Plug-ins selecteren
dutch.PluginSelectionDescription=Kies welke meegeleverde plug-ins worden geïnstalleerd of uitgepakt.
dutch.PluginConfigurationLabel=Pluginconfiguratie:
dutch.PluginConfigurationBase=Basis (alleen lokaal, geen internettoegang)
dutch.PluginConfigurationStandard=Standaard (voor de meeste gebruikers)
dutch.PluginConfigurationAdvanced=Geavanceerd (voor gevorderde gebruikers)
dutch.PluginConfigurationExpert=Expert (voor experts en ontwikkelaars)
dutch.PluginConfigurationCustom=Aangepast
french.PluginSelectionTitle=Sélection des modules
french.PluginSelectionDescription=Choisissez les modules inclus à installer ou extraire.
french.PluginConfigurationLabel=Configuration des modules :
french.PluginConfigurationBase=Base (local uniquement, sans accès Internet)
french.PluginConfigurationStandard=Standard (pour la plupart des utilisateurs)
french.PluginConfigurationAdvanced=Avancée (pour les utilisateurs avancés)
french.PluginConfigurationExpert=Expert (pour les experts et développeurs)
french.PluginConfigurationCustom=Personnalisée
german.PluginSelectionTitle=Plugins auswählen
german.PluginSelectionDescription=Wählen Sie aus, welche mitgelieferten Plugins installiert oder entpackt werden.
german.PluginConfigurationLabel=Plugin-Konfiguration:
german.PluginConfigurationBase=Basis (nur lokal, kein Internetzugriff)
german.PluginConfigurationStandard=Standard (für die meisten Benutzer)
german.PluginConfigurationAdvanced=Erweitert (für fortgeschrittene Benutzer)
german.PluginConfigurationExpert=Experte (für Experten und Entwickler)
german.PluginConfigurationCustom=Benutzerdefiniert
hungarian.PluginSelectionTitle=Bővítmények kiválasztása
hungarian.PluginSelectionDescription=Válassza ki a telepítendő vagy kicsomagolandó mellékelt bővítményeket.
hungarian.PluginConfigurationLabel=Bővítménykonfiguráció:
hungarian.PluginConfigurationBase=Alap (csak helyi, internet-hozzáférés nélkül)
hungarian.PluginConfigurationStandard=Normál (a legtöbb felhasználónak)
hungarian.PluginConfigurationAdvanced=Haladó (haladó felhasználóknak)
hungarian.PluginConfigurationExpert=Szakértő (szakértőknek és fejlesztőknek)
hungarian.PluginConfigurationCustom=Egyéni
italian.PluginSelectionTitle=Seleziona plugin
italian.PluginSelectionDescription=Scegli quali plugin inclusi installare o estrarre.
italian.PluginConfigurationLabel=Configurazione dei plugin:
italian.PluginConfigurationBase=Base (solo locale, senza accesso a Internet)
italian.PluginConfigurationStandard=Standard (per la maggior parte degli utenti)
italian.PluginConfigurationAdvanced=Avanzata (per utenti avanzati)
italian.PluginConfigurationExpert=Esperto (per esperti e sviluppatori)
italian.PluginConfigurationCustom=Personalizzata
romanian.PluginSelectionTitle=Selectare pluginuri
romanian.PluginSelectionDescription=Alegeți pluginurile incluse care vor fi instalate sau extrase.
romanian.PluginConfigurationLabel=Configurația pluginurilor:
romanian.PluginConfigurationBase=De bază (doar local, fără acces la Internet)
romanian.PluginConfigurationStandard=Standard (pentru majoritatea utilizatorilor)
romanian.PluginConfigurationAdvanced=Avansat (pentru utilizatori avansați)
romanian.PluginConfigurationExpert=Expert (pentru experți și dezvoltatori)
romanian.PluginConfigurationCustom=Personalizat
russian.PluginSelectionTitle=Выбор плагинов
russian.PluginSelectionDescription=Выберите, какие включенные плагины будут установлены или извлечены.
russian.PluginConfigurationLabel=Конфигурация плагинов:
russian.PluginConfigurationBase=Базовая (только локальные, без доступа к Интернету)
russian.PluginConfigurationStandard=Стандартная (для большинства пользователей)
russian.PluginConfigurationAdvanced=Расширенная (для опытных пользователей)
russian.PluginConfigurationExpert=Экспертная (для экспертов и разработчиков)
russian.PluginConfigurationCustom=Пользовательская
slovak.PluginSelectionTitle=Výber pluginov
slovak.PluginSelectionDescription=Zvoľte, ktoré pribalené pluginy sa nainštalujú alebo rozbalia.
slovak.PluginConfigurationLabel=Konfigurácia pluginov:
slovak.PluginConfigurationBase=Základná (iba lokálne, bez prístupu na Internet)
slovak.PluginConfigurationStandard=Štandardná (pre väčšinu používateľov)
slovak.PluginConfigurationAdvanced=Pokročilá (pre pokročilých používateľov)
slovak.PluginConfigurationExpert=Expertná (pre expertov a vývojárov)
slovak.PluginConfigurationCustom=Vlastná
spanish.PluginSelectionTitle=Seleccionar complementos
spanish.PluginSelectionDescription=Elija qué complementos incluidos se instalarán o extraerán.
spanish.PluginConfigurationLabel=Configuración de complementos:
spanish.PluginConfigurationBase=Base (solo local, sin acceso a Internet)
spanish.PluginConfigurationStandard=Estándar (para la mayoría de los usuarios)
spanish.PluginConfigurationAdvanced=Avanzada (para usuarios avanzados)
spanish.PluginConfigurationExpert=Experta (para expertos y desarrolladores)
spanish.PluginConfigurationCustom=Personalizada
czech.PluginSelectionTitle=Výběr pluginů
czech.PluginSelectionDescription=Zvolte, které přibalené pluginy se nainstalují nebo rozbalí.
czech.PluginConfigurationLabel=Konfigurace pluginů:
czech.PluginConfigurationBase=Base (pouze lokální, bez přístupu k Internetu)
czech.PluginConfigurationStandard=Standard (pro většinu uživatelů)
czech.PluginConfigurationAdvanced=Advanced (pro pokročilé uživatele)
czech.PluginConfigurationExpert=Expert (pro experty a vývojáře)
czech.PluginConfigurationCustom=Vlastní
english.InstalledVersionLabel=installed
chinesesimplified.InstalledVersionLabel=已安装
dutch.InstalledVersionLabel=geïnstalleerd
french.InstalledVersionLabel=installé
german.InstalledVersionLabel=installiert
hungarian.InstalledVersionLabel=telepítve
italian.InstalledVersionLabel=installato
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
Name: "{app}\plugins\automation\scripts\Salamatrix Progress Demo"
Name: "{app}\plugins\dbviewer"
Name: "{app}\plugins\dbviewer\lang"
Name: "{app}\plugins\demoplug"
Name: "{app}\plugins\demoplug\lang"
Name: "{app}\plugins\diskdir"
Name: "{app}\plugins\diskdir\lang"
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
Name: "{app}\plugins\salamatrixai"
Name: "{app}\plugins\salamatrixai\runtime"
Name: "{app}\plugins\salamatrixailocalllama"
Name: "{app}\plugins\salamatrixailocalllama\runtime"
Name: "{app}\plugins\sftp"
Name: "{app}\plugins\sftp\lang"
Name: "{app}\plugins\extension-runtimes"
Name: "{app}\plugins\extension-runtimes\javascriptruntime"
Name: "{app}\plugins\extension-runtimes\javascriptruntime\runtime"
Name: "{app}\plugins\extension-runtimes\luaruntime"
Name: "{app}\plugins\extension-runtimes\luaruntime\runtime"
Name: "{app}\plugins\extension-runtimes\phpruntime"
Name: "{app}\plugins\extension-runtimes\phpruntime\runtime"
Name: "{app}\plugins\extension-runtimes\powershellruntime"
Name: "{app}\plugins\extension-runtimes\powershellruntime\runtime"
Name: "{app}\plugins\extension-runtimes\pythonruntime"
Name: "{app}\plugins\extension-runtimes\pythonruntime\runtime"
Name: "{app}\extensions"
Name: "{app}\extensions\demos"
Name: "{app}\extensions\extension-menu-builder"
Name: "{app}\extensions\git-worktree-navigator"
Name: "{app}\extensions\file-lock-inspector"
Name: "{app}\extensions\process-explorer"
Name: "{app}\extensions\hardware-monitor"
Name: "{app}\extensions\event-viewer"
Name: "{app}\plugins\serviceexplorer"
Name: "{app}\plugins\serviceexplorer\lang"
Name: "{app}\plugins\splitcbn"
Name: "{app}\plugins\splitcbn\lang"
Name: "{app}\plugins\tar"
Name: "{app}\plugins\tar\lang"
Name: "{app}\plugins\textviewer"
Name: "{app}\plugins\textviewer\lang"
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

[InstallDelete]
Type: files; Name: "{app}\MarkdigRenderer.exe"
Type: files; Name: "{app}\WebView2Loader.dll"
Type: files; Name: "{app}\plugins\salamatrix\WebView2Loader.dll"
Type: files; Name: "{app}\plugins\salamatrix\MarkdigRenderer.exe"
Type: files; Name: "{app}\plugins\textviewer\WebView2Loader.dll"
Type: filesandordirs; Name: "{app}\plugins\textviewer\prism"
Type: files; Name: "{app}\plugins\webview2renderviewer\WebView2Loader.dll"
Type: files; Name: "{app}\plugins\webview2renderviewer\MarkdigRenderer.exe"

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
Source: "{#PayloadDir}\doc\third_party_italian.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_chinesesimplified.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_romanian.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_russian.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_slovak.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\third_party_spanish.md"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\translations.txt"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\7zip.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\automation.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\dbviewer.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\diskmap.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\filecomp.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\ftp.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\hypervm.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\checksum.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
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
Source: "{#PayloadDir}\lang\italian.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\chinesesimplified.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\romanian.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\russian.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\slovak.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\spanish.slg"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\msvcp140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\7zip\7za.dll"; DestDir: "{app}\plugins\7zip"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\7zip.spl"; DestDir: "{app}\plugins\7zip"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\7zwrapper.dll"; DestDir: "{app}\plugins\7zip"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\lang\*.slg"; DestDir: "{app}\plugins\7zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\automation\automation.spl"; DestDir: "{app}\plugins\automation"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\lang\*.slg"; DestDir: "{app}\plugins\automation\lang"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Convert Images.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Count Lines.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Launch Elevated Command Prompt.vbs"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Make Link.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Make List (JScript).js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Make List (VBScript).vbs"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Unpack Multiple Archives.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Salamatrix Progress Demo\*"; DestDir: "{app}\plugins\automation\scripts\Salamatrix Progress Demo"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('salamatrixdemos')
Source: "{#PayloadDir}\plugins\dbviewer\dbviewer.spl"; DestDir: "{app}\plugins\dbviewer"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\dbviewer\lang\*.slg"; DestDir: "{app}\plugins\dbviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\diskdir\diskdir.spl"; DestDir: "{app}\plugins\diskdir"; Flags: ignoreversion; Check: IsPluginSelected('diskdir')
Source: "{#PayloadDir}\plugins\diskdir\lang\*.slg"; DestDir: "{app}\plugins\diskdir\lang"; Flags: ignoreversion; Check: IsPluginSelected('diskdir')
Source: "{#PayloadDir}\plugins\diskmap\diskmap.spl"; DestDir: "{app}\plugins\diskmap"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\diskmap\lang\*.slg"; DestDir: "{app}\plugins\diskmap\lang"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\filecomp\fcremote.exe"; DestDir: "{app}\plugins\filecomp"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\filecomp.spl"; DestDir: "{app}\plugins\filecomp"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\lang\*.slg"; DestDir: "{app}\plugins\filecomp\lang"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\folders\folders.spl"; DestDir: "{app}\plugins\folders"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\folders\lang\*.slg"; DestDir: "{app}\plugins\folders\lang"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\ftp\ftp.spl"; DestDir: "{app}\plugins\ftp"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\ftp\lang\*.slg"; DestDir: "{app}\plugins\ftp\lang"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\sftp\libcrypto-3-x64.dll"; DestDir: "{app}\plugins\sftp"; Flags: ignoreversion; Check: IsPluginSelected('sftp')
Source: "{#PayloadDir}\plugins\sftp\libssh2.dll"; DestDir: "{app}\plugins\sftp"; Flags: ignoreversion; Check: IsPluginSelected('sftp')
Source: "{#PayloadDir}\plugins\sftp\lang\*.slg"; DestDir: "{app}\plugins\sftp\lang"; Flags: ignoreversion; Check: IsPluginSelected('sftp')
Source: "{#PayloadDir}\plugins\sftp\sftp.spl"; DestDir: "{app}\plugins\sftp"; Flags: ignoreversion; Check: IsPluginSelected('sftp')
Source: "{#PayloadDir}\plugins\sftp\z.dll"; DestDir: "{app}\plugins\sftp"; Flags: ignoreversion; Check: IsPluginSelected('sftp')
Source: "{#PayloadDir}\plugins\hypervm\HyperVM.Managed.dll"; DestDir: "{app}\plugins\hypervm"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\hypervm.spl"; DestDir: "{app}\plugins\hypervm"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\lang\*.slg"; DestDir: "{app}\plugins\hypervm\lang"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\checksum\checksum.spl"; DestDir: "{app}\plugins\checksum"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\checksum\lang\*.slg"; DestDir: "{app}\plugins\checksum\lang"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\ieviewer\css\githubmd.css"; DestDir: "{app}\plugins\ieviewer\css"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\ieviewer.spl"; DestDir: "{app}\plugins\ieviewer"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\lang\*.slg"; DestDir: "{app}\plugins\ieviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\DEPENDENCIES.md"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\JsonViewer.Managed.dll"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\jsonviewer.spl"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\lang\*.slg"; DestDir: "{app}\plugins\jsonviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\Newtonsoft.Json.dll"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\OpenSalamander.ManagedBootstrap.dll"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\SUPPORTED_FILE_TYPES.md"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\mmviewer\lang\*.slg"; DestDir: "{app}\plugins\mmviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\mmviewer\mmviewer.spl"; DestDir: "{app}\plugins\mmviewer"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\nethood\lang\*.slg"; DestDir: "{app}\plugins\nethood\lang"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\nethood\nethood.spl"; DestDir: "{app}\plugins\nethood"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\pak\lang\*.slg"; DestDir: "{app}\plugins\pak\lang"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\pak\pak.spl"; DestDir: "{app}\plugins\pak"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\peviewer\lang\*.slg"; DestDir: "{app}\plugins\peviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\peviewer\peviewer.spl"; DestDir: "{app}\plugins\peviewer"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\pictview\exif.dll"; DestDir: "{app}\plugins\pictview"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\lang\*.slg"; DestDir: "{app}\plugins\pictview\lang"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\pictview-previewhost.exe"; DestDir: "{app}\plugins\pictview"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\pictview.spl"; DestDir: "{app}\plugins\pictview"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\plugins.ver"; DestDir: "{app}\plugins"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\portables\lang\*.slg"; DestDir: "{app}\plugins\portables\lang"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\portables\portables.spl"; DestDir: "{app}\plugins\portables"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\regedt\lang\*.slg"; DestDir: "{app}\plugins\regedt\lang"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\regedt\regedt.spl"; DestDir: "{app}\plugins\regedt"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\renamer\lang\*.slg"; DestDir: "{app}\plugins\renamer\lang"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\renamer\renamer.spl"; DestDir: "{app}\plugins\renamer"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
Source: "{#PayloadDir}\plugins\samandarin\lang\*.slg"; DestDir: "{app}\plugins\samandarin\lang"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\Samandarin.Managed.dll"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\samandarin.spl"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\OpenSalamander.ManagedBootstrap.dll"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\SharpCompress.dll"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\Microsoft.Bcl.AsyncInterfaces.dll"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\System.Buffers.dll"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\System.Memory.dll"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\System.Numerics.Vectors.dll"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\System.Runtime.CompilerServices.Unsafe.dll"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\System.Text.Encoding.CodePages.dll"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\samandarin\System.Threading.Tasks.Extensions.dll"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion; Check: IsPluginSelected('samandarin')
Source: "{#PayloadDir}\plugins\serviceexplorer\lang\*.slg"; DestDir: "{app}\plugins\serviceexplorer\lang"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\serviceexplorer\serviceexplorer.spl"; DestDir: "{app}\plugins\serviceexplorer"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\splitcbn\lang\*.slg"; DestDir: "{app}\plugins\splitcbn\lang"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\splitcbn\splitcbn.spl"; DestDir: "{app}\plugins\splitcbn"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\tar\lang\*.slg"; DestDir: "{app}\plugins\tar\lang"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\tar\tar.spl"; DestDir: "{app}\plugins\tar"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\textviewer\DEPENDENCIES.md"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\lang\*.slg"; DestDir: "{app}\plugins\textviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\SUPPORTED_FILE_TYPES.md"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\textviewer.spl"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\unarj\lang\*.slg"; DestDir: "{app}\plugins\unarj\lang"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\unarj\unarj.spl"; DestDir: "{app}\plugins\unarj"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\uncab\lang\*.slg"; DestDir: "{app}\plugins\uncab\lang"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\uncab\uncab.spl"; DestDir: "{app}\plugins\uncab"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\undelete\lang\*.slg"; DestDir: "{app}\plugins\undelete\lang"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\undelete\undelete.spl"; DestDir: "{app}\plugins\undelete"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\unfat\lang\*.slg"; DestDir: "{app}\plugins\unfat\lang"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unfat\unfat.spl"; DestDir: "{app}\plugins\unfat"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unchm\chmlib.dll"; DestDir: "{app}\plugins\unchm"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\lang\*.slg"; DestDir: "{app}\plugins\unchm\lang"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\unchm.spl"; DestDir: "{app}\plugins\unchm"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\uniso\lang\*.slg"; DestDir: "{app}\plugins\uniso\lang"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\uniso\uniso.spl"; DestDir: "{app}\plugins\uniso"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\unlha\lang\*.slg"; DestDir: "{app}\plugins\unlha\lang"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unlha\unlha.spl"; DestDir: "{app}\plugins\unlha"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unmime\lang\*.slg"; DestDir: "{app}\plugins\unmime\lang"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unmime\unmime.spl"; DestDir: "{app}\plugins\unmime"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unole\lang\*.slg"; DestDir: "{app}\plugins\unole\lang"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unole\unole.spl"; DestDir: "{app}\plugins\unole"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unrar\lang\*.slg"; DestDir: "{app}\plugins\unrar\lang"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\unrar.dll"; DestDir: "{app}\plugins\unrar"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\unrar.spl"; DestDir: "{app}\plugins\unrar"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\webview2renderviewer\DEPENDENCIES.md"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\lang\*.slg"; DestDir: "{app}\plugins\webview2renderviewer\lang"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\SUPPORTED_FILE_TYPES.md"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\webview2renderviewer.spl"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\wmobile\lang\*.slg"; DestDir: "{app}\plugins\wmobile\lang"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\wmobile\wmobile.spl"; DestDir: "{app}\plugins\wmobile"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
Source: "{#PayloadDir}\plugins\zip\lang\*.slg"; DestDir: "{app}\plugins\zip\lang"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\zip.spl"; DestDir: "{app}\plugins\zip"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\sfx\chinesesimplified.sfx"; DestDir: "{app}\plugins\zip\sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\sfx\czech.sfx"; DestDir: "{app}\plugins\zip\sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\sfx\dutch.sfx"; DestDir: "{app}\plugins\zip\sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\sfx\english.sfx"; DestDir: "{app}\plugins\zip\sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\sfx\french.sfx"; DestDir: "{app}\plugins\zip\sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\sfx\german.sfx"; DestDir: "{app}\plugins\zip\sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\sfx\hungarian.sfx"; DestDir: "{app}\plugins\zip\sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\sfx\italian.sfx"; DestDir: "{app}\plugins\zip\sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\sfx\romanian.sfx"; DestDir: "{app}\plugins\zip\sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\sfx\russian.sfx"; DestDir: "{app}\plugins\zip\sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\sfx\slovak.sfx"; DestDir: "{app}\plugins\zip\sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\sfx\spanish.sfx"; DestDir: "{app}\plugins\zip\sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\zip2sfx\license.txt"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\zip2sfx\license_cz.txt"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\zip2sfx\readme.cz"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\zip2sfx\readme.txt"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\zip2sfx\sam_cz.set"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\zip2sfx\sample.set"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\zip\zip2sfx\zip2sfx.exe"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion; Check: IsPluginSelected('zip')
Source: "{#PayloadDir}\plugins\salamatrix\prism\*"; DestDir: "{app}\plugins\salamatrix\prism"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('salamatrix')
Source: "{#PayloadDir}\plugins\salamatrix\salamatrix.spl"; DestDir: "{app}\plugins\salamatrix"; Flags: ignoreversion; Check: IsPluginSelected('salamatrix')
Source: "{#PayloadDir}\plugins\salamatrix\salamatrix-automation-api.html"; DestDir: "{app}\plugins\salamatrix"; Flags: ignoreversion; Check: IsPluginSelected('salamatrix')
Source: "{#PayloadDir}\plugins\salamatrix\salamatrix-ui.html"; DestDir: "{app}\plugins\salamatrix"; Flags: ignoreversion; Check: IsPluginSelected('salamatrix')
Source: "{#PayloadDir}\plugins\salamatrix\salamatrix-platform.html"; DestDir: "{app}\plugins\salamatrix"; Flags: ignoreversion; Check: IsPluginSelected('salamatrix')
Source: "{#PayloadDir}\plugins\salamatrix\salamatrix-runtime-providers.html"; DestDir: "{app}\plugins\salamatrix"; Flags: ignoreversion; Check: IsPluginSelected('salamatrix')
Source: "{#PayloadDir}\plugins\salamatrix\salamatrix-runtime-provider-development.html"; DestDir: "{app}\plugins\salamatrix"; Flags: ignoreversion; Check: IsPluginSelected('salamatrix')
Source: "{#PayloadDir}\plugins\salamatrix\salamatrix-gap-analysis.html"; DestDir: "{app}\plugins\salamatrix"; Flags: ignoreversion; Check: IsPluginSelected('salamatrix')
Source: "{#PayloadDir}\plugins\salamatrixai\salamatrixai.spl"; DestDir: "{app}\plugins\salamatrixai"; Flags: ignoreversion; Check: IsPluginSelected('salamatrixai')
Source: "{#PayloadDir}\plugins\salamatrixai\runtime\salamatrix_ai_local.py"; DestDir: "{app}\plugins\salamatrixai\runtime"; Flags: ignoreversion; Check: IsPluginSelected('salamatrixai')
Source: "{#PayloadDir}\plugins\salamatrixailocalllama\salamatrixailocalllama.spl"; DestDir: "{app}\plugins\salamatrixailocalllama"; Flags: ignoreversion; Check: IsPluginSelected('salamatrixailocalllama')
Source: "{#PayloadDir}\plugins\salamatrixailocalllama\runtime\install_llama.ps1"; DestDir: "{app}\plugins\salamatrixailocalllama\runtime"; Flags: ignoreversion; Check: IsPluginSelected('salamatrixailocalllama')
Source: "{#PayloadDir}\plugins\demoplug\demoplug.spl"; DestDir: "{app}\plugins\demoplug"; Flags: ignoreversion; Check: IsPluginSelected('demoplug')
Source: "{#PayloadDir}\plugins\demoplug\lang\*.slg"; DestDir: "{app}\plugins\demoplug\lang"; Flags: ignoreversion; Check: IsPluginSelected('demoplug')
Source: "{#PayloadDir}\extensions\demos\*"; DestDir: "{app}\extensions\demos"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('salamatrixdemos')
Source: "{#PayloadDir}\extensions\extension-menu-builder\*"; DestDir: "{app}\extensions\extension-menu-builder"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('extensionmenubuilder')
Source: "{#PayloadDir}\extensions\git-worktree-navigator\*"; DestDir: "{app}\extensions\git-worktree-navigator"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('gitworktreenavigator')
Source: "{#PayloadDir}\extensions\file-lock-inspector\*"; DestDir: "{app}\extensions\file-lock-inspector"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('filelockinspector')
Source: "{#PayloadDir}\extensions\process-explorer\*"; DestDir: "{app}\extensions\process-explorer"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('processexplorer')
Source: "{#PayloadDir}\extensions\hardware-monitor\*"; DestDir: "{app}\extensions\hardware-monitor"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('hardwaremonitor')
Source: "{#PayloadDir}\extensions\event-viewer\*"; DestDir: "{app}\extensions\event-viewer"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('eventviewer')
Source: "{#PayloadDir}\plugins\extension-runtimes\javascriptruntime\javascriptruntime.spl"; DestDir: "{app}\plugins\extension-runtimes\javascriptruntime"; Flags: ignoreversion; Check: IsPluginSelected('javascriptruntime')
Source: "{#PayloadDir}\plugins\extension-runtimes\javascriptruntime\runtime\salamatrix_worker.mjs"; DestDir: "{app}\plugins\extension-runtimes\javascriptruntime\runtime"; Flags: ignoreversion; Check: IsPluginSelected('javascriptruntime')
Source: "{#PayloadDir}\plugins\extension-runtimes\luaruntime\luaruntime.spl"; DestDir: "{app}\plugins\extension-runtimes\luaruntime"; Flags: ignoreversion; Check: IsPluginSelected('luaruntime')
Source: "{#PayloadDir}\plugins\extension-runtimes\luaruntime\runtime\salamatrix_worker.lua"; DestDir: "{app}\plugins\extension-runtimes\luaruntime\runtime"; Flags: ignoreversion; Check: IsPluginSelected('luaruntime')
Source: "{#PayloadDir}\plugins\extension-runtimes\luaruntime\runtime\lua.exe"; DestDir: "{app}\plugins\extension-runtimes\luaruntime\runtime"; Flags: ignoreversion; Check: IsPluginSelected('luaruntime')
Source: "{#PayloadDir}\plugins\extension-runtimes\luaruntime\runtime\lua.dll"; DestDir: "{app}\plugins\extension-runtimes\luaruntime\runtime"; Flags: ignoreversion; Check: IsPluginSelected('luaruntime')
Source: "{#PayloadDir}\plugins\extension-runtimes\luaruntime\runtime\LICENSE-LUA.txt"; DestDir: "{app}\plugins\extension-runtimes\luaruntime\runtime"; Flags: ignoreversion; Check: IsPluginSelected('luaruntime')
Source: "{#PayloadDir}\plugins\extension-runtimes\phpruntime\phpruntime.spl"; DestDir: "{app}\plugins\extension-runtimes\phpruntime"; Flags: ignoreversion; Check: IsPluginSelected('phpruntime')
Source: "{#PayloadDir}\plugins\extension-runtimes\phpruntime\runtime\salamatrix_worker.php"; DestDir: "{app}\plugins\extension-runtimes\phpruntime\runtime"; Flags: ignoreversion; Check: IsPluginSelected('phpruntime')
Source: "{#PayloadDir}\plugins\extension-runtimes\powershellruntime\powershellruntime.spl"; DestDir: "{app}\plugins\extension-runtimes\powershellruntime"; Flags: ignoreversion; Check: IsPluginSelected('powershellruntime')
Source: "{#PayloadDir}\plugins\extension-runtimes\powershellruntime\runtime\salamatrix_worker.ps1"; DestDir: "{app}\plugins\extension-runtimes\powershellruntime\runtime"; Flags: ignoreversion; Check: IsPluginSelected('powershellruntime')
Source: "{#PayloadDir}\plugins\extension-runtimes\pythonruntime\pythonruntime.spl"; DestDir: "{app}\plugins\extension-runtimes\pythonruntime"; Flags: ignoreversion; Check: IsPluginSelected('pythonruntime')
Source: "{#PayloadDir}\plugins\extension-runtimes\pythonruntime\runtime\salamatrix_worker.py"; DestDir: "{app}\plugins\extension-runtimes\pythonruntime\runtime"; Flags: ignoreversion; Check: IsPluginSelected('pythonruntime')
Source: "{#PayloadDir}\salamand.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\salamand.exe.config"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\configstorage.ini"; DestDir: "{app}"; Flags: ignoreversion; Permissions: users-modify; Check: ShouldInstallConfigStorage
Source: "{#SourcePath}plugin-receipts.json"; DestDir: "{app}"; Flags: ignoreversion onlyifdoesntexist uninsneveruninstall; Permissions: users-modify
Source: "{#SourcePath}plugin-capabilities.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\bundled-plugin-metadata.json"; DestDir: "{app}"; Flags: ignoreversion
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
Source: "{#PayloadDir}\toolbars\ExplorerCategoryCommon.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ExplorerCategoryFavorites.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ExplorerCategoryFavoritesRed.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ExplorerCategoryFileSystem.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ExplorerCategoryDocument.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ExplorerCategoryImage.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ExplorerCategoryAudio.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ExplorerCategoryVideo.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ExplorerCategoryOther.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ExplorerCategoryExecutable.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ExplorerCategoryArchive.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
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
Source: "{#PayloadDir}\toolbars\darkmode\ExplorerCategoryCommon.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ExplorerCategoryFavorites.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ExplorerCategoryFavoritesRed.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ExplorerCategoryFileSystem.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ExplorerCategoryDocument.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ExplorerCategoryImage.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ExplorerCategoryAudio.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ExplorerCategoryVideo.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ExplorerCategoryOther.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ExplorerCategoryExecutable.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\ExplorerCategoryArchive.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
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
Source: "{#PayloadDir}\utils\salmon.exe"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\salopen.exe"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\salspawn.exe"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\sqlite.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\ssleay32.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\vcruntime140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\vcruntime140_1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\MarkdigRenderer.exe"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\WebView2Loader.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
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


type
  TPluginDependency = record
    PluginId: String;
    DependencyId: String;
  end;

var
  InstallModePage: TInputOptionWizardPage;
  PluginSelectionPage: TWizardPage;
  PluginList: TNewCheckListBox;
  PluginConfigurationLabel: TLabel;
  PluginConfigurationList: TNewComboBox;
  ApplyingPluginConfiguration: Boolean;
  UpgradeInstall: Boolean;
  PluginIds: array of String;
  PluginConfigurations: array of String;
  PluginDependencies: array of TPluginDependency;
  DeleteUserConfiguration: Boolean;
  DeleteUserConfigurationFromFile: Boolean;
  DeleteUserConfigurationFilePath: String;
  PreviousVersionUninstallKeys: array of String;
  SilentInstallMode: String;
  SilentPluginSelection: String;
  SilentPluginConfiguration: String;
  SilentShortcuts: String;
  SilentOverwriteDir: String;
  SilentRunApp: String;




function IsPortableInstall(): Boolean; forward;
procedure SelectAllPluginDependencies; forward;

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

function IsExtensionRuntimePlugin(const PluginId: String): Boolean;
begin
  Result :=
    (CompareText(PluginId, 'javascriptruntime') = 0) or
    (CompareText(PluginId, 'luaruntime') = 0) or
    (CompareText(PluginId, 'phpruntime') = 0) or
    (CompareText(PluginId, 'powershellruntime') = 0) or
    (CompareText(PluginId, 'pythonruntime') = 0);
end;

procedure AddPluginDependency(const PluginId, DependencyId: String);
var
  Index: Integer;
begin
  Index := GetArrayLength(PluginDependencies);
  SetArrayLength(PluginDependencies, Index + 1);
  PluginDependencies[Index].PluginId := PluginId;
  PluginDependencies[Index].DependencyId := DependencyId;
end;

procedure InitializePluginDependencies;
begin
  SetArrayLength(PluginDependencies, 0);

  { Text Viewer, runtime providers, and Salamatrix AI use the Salamatrix Framework. }
  AddPluginDependency('textviewer', 'salamatrix');
  AddPluginDependency('webview2renderviewer', 'salamatrix');
  AddPluginDependency('javascriptruntime', 'salamatrix');
  AddPluginDependency('luaruntime', 'salamatrix');
  AddPluginDependency('phpruntime', 'salamatrix');
  AddPluginDependency('powershellruntime', 'salamatrix');
  AddPluginDependency('pythonruntime', 'salamatrix');
  AddPluginDependency('salamatrixai', 'salamatrix');

  { The local LLaMA provider extends Salamatrix AI. }
  AddPluginDependency('salamatrixailocalllama', 'salamatrixai');

  { These PowerShell extensions need the PowerShell runtime. }
  AddPluginDependency('extensionmenubuilder', 'powershellruntime');
  AddPluginDependency('gitworktreenavigator', 'powershellruntime');
  AddPluginDependency('filelockinspector', 'powershellruntime');
  AddPluginDependency('processexplorer', 'powershellruntime');
  AddPluginDependency('hardwaremonitor', 'powershellruntime');
  AddPluginDependency('eventviewer', 'powershellruntime');

  { The demo package contains one Automation extension and five runtime demos. }
  AddPluginDependency('salamatrixdemos', 'automation');
  AddPluginDependency('salamatrixdemos', 'javascriptruntime');
  AddPluginDependency('salamatrixdemos', 'luaruntime');
  AddPluginDependency('salamatrixdemos', 'phpruntime');
  AddPluginDependency('salamatrixdemos', 'powershellruntime');
  AddPluginDependency('salamatrixdemos', 'pythonruntime');
end;

function PluginDependsOn(const PluginId, DependencyId: String): Boolean;
var
  I: Integer;
begin
  Result := False;
  for I := 0 to GetArrayLength(PluginDependencies) - 1 do
  begin
    if CompareText(PluginDependencies[I].PluginId, PluginId) = 0 then
    begin
      if CompareText(PluginDependencies[I].DependencyId, DependencyId) = 0 then
      begin
        Result := True;
        Exit;
      end;

      if PluginDependsOn(PluginDependencies[I].DependencyId, DependencyId) then
      begin
        Result := True;
        Exit;
      end;
    end;
  end;
end;

function IsDefaultPlugin(const PluginId: String): Boolean;
begin
  Result := not (
    (CompareText(PluginId, 'demoplug') = 0) or
    (CompareText(PluginId, 'folders') = 0) or
    (CompareText(PluginId, 'ieviewer') = 0) or
    (CompareText(PluginId, 'javascriptruntime') = 0) or
    (CompareText(PluginId, 'luaruntime') = 0) or
    (CompareText(PluginId, 'phpruntime') = 0) or
    (CompareText(PluginId, 'pythonruntime') = 0) or
    (CompareText(PluginId, 'salamatrixai') = 0) or
    (CompareText(PluginId, 'salamatrixailocalllama') = 0) or
    (CompareText(PluginId, 'salamatrixdemos') = 0) or
    (CompareText(PluginId, 'wmobile') = 0));
end;

function IsExpertPlugin(const PluginId: String): Boolean;
begin
  Result := IsExtensionRuntimePlugin(PluginId) or
    (CompareText(PluginId, 'demoplug') = 0) or
    (CompareText(PluginId, 'salamatrixai') = 0) or
    (CompareText(PluginId, 'salamatrixailocalllama') = 0) or
    (CompareText(PluginId, 'salamatrixdemos') = 0) or
    (CompareText(PluginId, 'wmobile') = 0);
end;

function IsPluginInConfiguration(const PluginId, Configuration: String): Boolean;
var
  I: Integer;
begin
  Result := False;
  for I := 0 to GetArrayLength(PluginIds) - 1 do
  begin
    if CompareText(PluginIds[I], PluginId) = 0 then
    begin
      Result := ListContainsToken(PluginConfigurations[I], Configuration);
      Exit;
    end;
  end;
end;

function IsValidPluginConfiguration(const Configuration: String): Boolean;
begin
  Result := (Configuration = 'base') or (Configuration = 'standard') or
    (Configuration = 'advanced') or (Configuration = 'expert') or
    (Configuration = 'custom');
end;

function GetSelectedPluginConfiguration(): String;
begin
  if SilentPluginConfiguration <> '' then
    Result := SilentPluginConfiguration
  else if Assigned(PluginConfigurationList) and (PluginConfigurationList.ItemIndex >= 0) then
    case PluginConfigurationList.ItemIndex of
      0: Result := 'base';
      1: Result := 'standard';
      2: Result := 'advanced';
      3: Result := 'expert';
    else
      Result := 'custom';
    end
  else
    Result := 'standard';
end;

procedure SetPluginConfiguration(const Configuration: String);
var
  I: Integer;
begin
  if Configuration = 'custom' then
    Exit;
  ApplyingPluginConfiguration := True;
  try
    for I := 0 to GetArrayLength(PluginIds) - 1 do
      PluginList.Checked[I] := IsPluginInConfiguration(PluginIds[I], Configuration);
    SelectAllPluginDependencies;
  finally
    ApplyingPluginConfiguration := False;
  end;
end;

procedure PluginConfigurationChange(Sender: TObject);
begin
  if Assigned(PluginConfigurationList) and (PluginConfigurationList.ItemIndex >= 0) then
    SetPluginConfiguration(GetSelectedPluginConfiguration());
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
    Result := WizardIsTaskSelected('desktopicon')
  else if CompareText(ShortcutId, 'startmenu') = 0 then
    Result := WizardIsTaskSelected('startmenuicon')
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

function GetInstalledExtensionVersion(const PluginId: String): String;
var
  I: Integer;
  Count: Integer;
  PluginPath: String;
  ExpectedPath: String;
begin
  Result := '';
  if CompareText(PluginId, 'extensionmenubuilder') = 0 then ExpectedPath := 'extensions/extension-menu-builder/'
  else if CompareText(PluginId, 'gitworktreenavigator') = 0 then ExpectedPath := 'extensions/git-worktree-navigator/'
  else if CompareText(PluginId, 'filelockinspector') = 0 then ExpectedPath := 'extensions/file-lock-inspector/'
  else if CompareText(PluginId, 'processexplorer') = 0 then ExpectedPath := 'extensions/process-explorer/'
  else if CompareText(PluginId, 'hardwaremonitor') = 0 then ExpectedPath := 'extensions/hardware-monitor/'
  else if CompareText(PluginId, 'eventviewer') = 0 then ExpectedPath := 'extensions/event-viewer/'
  else if CompareText(PluginId, 'salamatrixdemos') = 0 then ExpectedPath := 'plugins/automation/scripts/Salamatrix Progress Demo/'
  else Exit;
  Count := StrToIntDef(GetIniString('InstalledPlugins', 'Count', '0', ExpandConstant('{app}\configstorage.ini')), 0);
  for I := 1 to Count do
  begin
    PluginPath := GetIniString('InstalledPlugins', 'Plugin' + IntToStr(I) + 'Path', '', ExpandConstant('{app}\configstorage.ini'));
    StringChangeEx(PluginPath, '\', '/', True);
    if Pos(ExpectedPath, PluginPath) > 0 then
    begin
      Result := GetIniString('InstalledPlugins', 'Plugin' + IntToStr(I) + 'Version', '', ExpandConstant('{app}\configstorage.ini'));
      Exit;
    end;
  end;
end;

function GetInstalledPluginVersion(const PluginId: String): String;
var
  I: Integer;
  Count: Integer;
  PluginPath: String;
  ExpectedPath: String;
begin
  Result := '';
  if (CompareText(PluginId, 'extensionmenubuilder') = 0) or (CompareText(PluginId, 'gitworktreenavigator') = 0) or
     (CompareText(PluginId, 'filelockinspector') = 0) or (CompareText(PluginId, 'processexplorer') = 0) or
     (CompareText(PluginId, 'hardwaremonitor') = 0) or (CompareText(PluginId, 'eventviewer') = 0) or
     (CompareText(PluginId, 'salamatrixdemos') = 0) then
  begin
    Result := GetInstalledExtensionVersion(PluginId);
    Exit;
  end;

  if IsExtensionRuntimePlugin(PluginId) then
    ExpectedPath := 'plugins/extension-runtimes/' + PluginId + '/' + PluginId + '.spl'
  else
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

procedure AddPlugin(const PluginId, DisplayName, Version: String; const Configurations: String);
var
  InstalledVersion: String;
  Checked: Boolean;
begin
  InstalledVersion := GetInstalledPluginVersion(PluginId);
  if InstalledVersion <> '' then
    UpgradeInstall := True;
  { Keep installed plugins selected during upgrades; new plugins follow Standard. }
  Checked := ListContainsToken(Configurations, 'standard') or (InstalledVersion <> '');
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
  SetArrayLength(PluginConfigurations, GetArrayLength(PluginConfigurations) + 1);
  PluginConfigurations[GetArrayLength(PluginConfigurations) - 1] := Configurations;
end;

function IsPluginExplicitlySelected(const PluginId: String): Boolean;
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

  if (SilentPluginSelection = '') and (SilentPluginConfiguration <> '') and (SilentPluginConfiguration <> 'custom') then
  begin
    Result := IsPluginInConfiguration(PluginId, SilentPluginConfiguration);
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

function IsPluginSelected(const PluginId: String): Boolean;
var
  I: Integer;
begin
  Result := IsPluginExplicitlySelected(PluginId);

  if Result then
    Exit;

  for I := 0 to GetArrayLength(PluginDependencies) - 1 do
  begin
    if IsPluginExplicitlySelected(PluginDependencies[I].PluginId) and
       PluginDependsOn(PluginDependencies[I].PluginId, PluginId) then
    begin
      Result := True;
      Exit;
    end;
  end;
end;

procedure SelectPlugin(const PluginId: String);
var
  I: Integer;
begin
  if not Assigned(PluginList) then
    Exit;

  for I := 0 to GetArrayLength(PluginIds) - 1 do
  begin
    if CompareText(PluginIds[I], PluginId) = 0 then
    begin
      PluginList.Checked[I] := True;
      Exit;
    end;
  end;
end;

procedure SelectPluginDependencies(const PluginId: String);
var
  I: Integer;
begin
  for I := 0 to GetArrayLength(PluginDependencies) - 1 do
  begin
    if CompareText(PluginDependencies[I].PluginId, PluginId) = 0 then
    begin
      SelectPlugin(PluginDependencies[I].DependencyId);
      SelectPluginDependencies(PluginDependencies[I].DependencyId);
    end;
  end;
end;

procedure SelectAllPluginDependencies;
var
  I: Integer;
begin
  if not Assigned(PluginList) then
    Exit;

  for I := 0 to GetArrayLength(PluginIds) - 1 do
  begin
    if PluginList.Checked[I] then
      SelectPluginDependencies(PluginIds[I]);
  end;
end;

procedure PluginListClickCheck(Sender: TObject);
begin
  { Re-select required dependencies even if the user tries to uncheck one. }
  SelectAllPluginDependencies;
  if not ApplyingPluginConfiguration and Assigned(PluginConfigurationList) then
    PluginConfigurationList.ItemIndex := 4;
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
    BackupPath := ConfigPath + '.BAK';
    DeleteFile(BackupPath);
    RenameFile(ConfigPath, BackupPath);
    Result := False;

    if not WizardSilent then
    begin
      Msg := CustomMessage('KeepConfigQuestion');
      StringChangeEx(Msg, '\n', #13#10, True);
      MsgBox(Msg, mbInformation, MB_OK);
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
  SetArrayLength(PluginConfigurations, 0);
  UpgradeInstall := False;

  AddPlugin('7zip', '7-Zip', '1.35 (x64)', 'base,standard,advanced,expert');
  AddPlugin('automation', 'Automation', '2.8 (x64)', 'advanced,expert');
  AddPlugin('checksum', 'Checksum', '2.4 (x64)', 'base,standard,advanced,expert');
  AddPlugin('dbviewer', 'Database Viewer', '1.27 (x64)', 'base,standard,advanced,expert');
  AddPlugin('demoplug', 'Demo Plugin', '2.0 (x64)', 'expert');
  AddPlugin('diskdir', 'DiskDir', '1.1 (x64)', 'base,standard,advanced,expert');
  AddPlugin('diskmap', 'DiskMap', '1.14 (x64)', 'advanced,expert');
  AddPlugin('extensionmenubuilder', 'Extension Menu Builder', '1.1.2 (x64)', 'advanced,expert');
  AddPlugin('filecomp', 'File Comparator', '1.22 (x64)', 'base,standard,advanced,expert');
  AddPlugin('filelockinspector', 'File Lock Inspector', '1.0.2 (x64)', 'advanced,expert');
  AddPlugin('processexplorer', 'Process Explorer', '1.3.0 (x64)', 'advanced,expert');
  AddPlugin('hardwaremonitor', 'Hardware Monitor', '1.0.0 (x64)', 'advanced,expert');
  AddPlugin('eventviewer', 'Event Viewer', '1.0.0 (x64)', 'advanced,expert');
  AddPlugin('folders', 'Folders', '0.3 (x64)', 'advanced,expert');
  AddPlugin('ftp', 'FTP Client', '1.37 (x64)', 'standard,advanced,expert');
  AddPlugin('gitworktreenavigator', 'Git Worktree Navigator', '1.0.2 (x64)', 'advanced,expert');
  AddPlugin('hypervm', 'Hyper-V Machines', '1.10 (x64)', 'advanced,expert');
  AddPlugin('ieviewer', 'Internet Explorer Viewer', '1.13 (x64)', 'expert');
  AddPlugin('javascriptruntime', 'JavaScript Runtime', '0.3 (x64)', 'expert');
  AddPlugin('jsonviewer', 'JSON Viewer .NET', '1.5 (x64)', 'base,standard,advanced,expert');
  AddPlugin('luaruntime', 'Lua Runtime', '0.3.1 (x64)', 'expert');
  AddPlugin('mmviewer', 'Multimedia Viewer', '1.17 (x64)', 'base,standard,advanced,expert');
  AddPlugin('nethood', 'Network', '1.10 (x64)', 'standard,advanced,expert');
  AddPlugin('pak', 'PAK', '1.73 (x64)', 'base,standard,advanced,expert');
  AddPlugin('phpruntime', 'PHP Runtime', '0.3 (x64)', 'expert');
  AddPlugin('pictview', 'PictView', '2.26 (x64)', 'base,standard,advanced,expert');
  AddPlugin('portables', 'Portable Devices', '0.5 (x64)', 'base,standard,advanced,expert');
  AddPlugin('peviewer', 'Portable Executable Viewer', '3.1 (x64)', 'base,standard,advanced,expert');
  AddPlugin('pythonruntime', 'Python Runtime', '0.3.1 (x64)', 'expert');
  AddPlugin('textviewer', 'Prism Text Viewer', '1.5 (x64)', 'base,standard,advanced,expert');
  AddPlugin('regedt', 'Registry Editor', '1.16 (x64)', 'base,standard,advanced,expert');
  AddPlugin('renamer', 'Renamer', '1.16 (x64)', 'base,standard,advanced,expert');
  AddPlugin('salamatrix', 'Salamatrix Framework', '0.7.13 (x64)', 'advanced,expert');
  AddPlugin('salamatrixai', 'Salamatrix AI', '0.2 (x64)', 'expert');
  AddPlugin('salamatrixailocalllama', 'Salamatrix AI Local LLaMA', '0.2 (x64)', 'expert');
  AddPlugin('salamatrixdemos', 'Salamatrix Demo Sample Scripts', '1.4.2 (x64)', 'expert');
  AddPlugin('sftp', 'SFTP/SCP Client', '1.3 beta (x64)', 'advanced,expert');
  AddPlugin('powershellruntime', 'PowerShell Runtime', '0.3 (x64)', 'expert');
  AddPlugin('samandarin', 'Samandarin Update Notifier', '0.9 (x64)', 'standard,advanced,expert');
  AddPlugin('serviceexplorer', 'Service Explorer', '0.14 (x64)', 'advanced,expert');
  AddPlugin('splitcbn', 'Split & Combine', '1.12 (x64)', 'base,standard,advanced,expert');
  AddPlugin('tar', 'TAR', '3.35 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unarj', 'UnARJ', '1.23 (x64)', 'base,standard,advanced,expert');
  AddPlugin('uncab', 'UnCAB', '1.29 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unchm', 'UnCHM', '1.6 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unfat', 'UnFAT', '1.3 (x64)', 'base,standard,advanced,expert');
  AddPlugin('uniso', 'UnISO', '1.39 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unlha', 'UnLHA', '1.15 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unmime', 'UnMIME', '1.16 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unole', 'UnOLE', '1.3 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unrar', 'UnRAR', '3.4 (x64)', 'base,standard,advanced,expert');
  AddPlugin('undelete', 'Undelete', '1.13 (x64)', 'base,standard,advanced,expert');
  AddPlugin('webview2renderviewer', 'WebView2 Render Viewer', '1.5 (x64)', 'base,standard,advanced,expert');
  AddPlugin('wmobile', 'Windows Mobile', '1.11 (x64)', 'expert');
  AddPlugin('zip', 'ZIP', '1.8 (x64)', 'base,standard,advanced,expert');
  SelectAllPluginDependencies;
  if (SilentPluginSelection = '') and (SilentPluginConfiguration <> '') then
    SetPluginConfiguration(SilentPluginConfiguration);
  if (SilentPluginConfiguration = '') and UpgradeInstall and
     Assigned(PluginConfigurationList) then
    PluginConfigurationList.ItemIndex := 4;
end;

function InitializeSetup(): Boolean;
begin
  Result := True;
  SilentInstallMode := GetLowerParam('INSTALLMODE', '');
  SilentPluginSelection := GetLowerParam('PLUGINS', '');
  SilentPluginConfiguration := GetLowerParam('PLUGINCONFIG', '');
  if not IsValidPluginConfiguration(SilentPluginConfiguration) then
    SilentPluginConfiguration := '';
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
  InitializePluginDependencies;

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

  PluginConfigurationLabel := TLabel.Create(PluginSelectionPage);
  PluginConfigurationLabel.Parent := PluginSelectionPage.Surface;
  PluginConfigurationLabel.Caption := CustomMessage('PluginConfigurationLabel');
  PluginConfigurationLabel.Left := 0;
  PluginConfigurationLabel.Top := 0;
  PluginConfigurationList := TNewComboBox.Create(PluginSelectionPage);
  PluginConfigurationList.Parent := PluginSelectionPage.Surface;
  PluginConfigurationList.Left := 0;
  PluginConfigurationList.Top := PluginConfigurationLabel.Top + PluginConfigurationLabel.Height + ScaleY(4);
  PluginConfigurationList.Width := PluginSelectionPage.SurfaceWidth;
  PluginConfigurationList.Style := csDropDownList;
  PluginConfigurationList.Items.Add(CustomMessage('PluginConfigurationBase'));
  PluginConfigurationList.Items.Add(CustomMessage('PluginConfigurationStandard'));
  PluginConfigurationList.Items.Add(CustomMessage('PluginConfigurationAdvanced'));
  PluginConfigurationList.Items.Add(CustomMessage('PluginConfigurationExpert'));
  PluginConfigurationList.Items.Add(CustomMessage('PluginConfigurationCustom'));
  if SilentPluginConfiguration = 'base' then
    PluginConfigurationList.ItemIndex := 0
  else if SilentPluginConfiguration = 'advanced' then
    PluginConfigurationList.ItemIndex := 2
  else if SilentPluginConfiguration = 'expert' then
    PluginConfigurationList.ItemIndex := 3
  else if SilentPluginConfiguration = 'custom' then
    PluginConfigurationList.ItemIndex := 4
  else
    PluginConfigurationList.ItemIndex := 1;
  PluginConfigurationList.OnChange := @PluginConfigurationChange;
  PluginList := TNewCheckListBox.Create(PluginSelectionPage);
  PluginList.Parent := PluginSelectionPage.Surface;
  PluginList.Left := 0;
  PluginList.Top := PluginConfigurationList.Top + PluginConfigurationList.Height + ScaleY(8);
  PluginList.Width := PluginSelectionPage.SurfaceWidth;
  PluginList.Height := PluginSelectionPage.SurfaceHeight - PluginList.Top - ScaleY(8);
  PluginList.OnClickCheck := @PluginListClickCheck;

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

  if CurPageID = PluginSelectionPage.ID then
    SelectAllPluginDependencies;
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
      DeleteFile(ExpandConstant('{app}\plugin-receipts.json'));
    end
    else
    begin
      RegDeleteKeyIncludingSubkeys(HKCU, {#AppToInstallRegPath});
      DeleteFile(ExpandConstant('{app}\configstorage.ini'));
      DeleteFile(ExpandConstant('{app}\plugin-receipts.json'));
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
