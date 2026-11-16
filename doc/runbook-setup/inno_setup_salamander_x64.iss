; | Date | Summary | Reason | Ticket | Model |
; | 2026-11-15 | Add Catalan, Danish, Finnish, Lithuanian, Norwegian and Slovenian installer languages. | Register new target languages in [Languages], wire up Inno Setup message files and localize custom installer strings. | | nvidia/nemotron-3.5-super-vl:free |
; | 2026-11-15 | Keep plugin selection on the full expert list. | Remove the mode switch and simple category selector from the installer. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-15 | Use Brazilian Portuguese in the installer language list. | Match the requested Portuguese locale and its terminology. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-15 | Declare web-package lookup outputs only in web builds. | Avoid unused-local hints when compiling the offline installer. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-15 | Translate installer and upgrade messages for all newly supported languages. | Avoid Inno Setup fallback-to-English warnings and keep each language's wizard text localized. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-15 | Use an explicit false comparison for the offline preprocessor branch. | Inno Setup 7 rejects unary negation when the web build flag is defined. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-15 | Keep demo-path matching compatible with Inno Pascal Script declarations. | The script parser requires a function body to start with `begin` after its signature. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-15 | Use Win32 to clear attributes before replacing extracted package files. | Inno Setup Pascal Script does not provide the Delphi `FileSetAttr` routine. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-15 | Bundle translations for installer languages missing from Inno Setup 7. | Keep Croatian, Greek, Indonesian, and Vietnamese builds reproducible across machines. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-15 | Let Arabic and Hebrew message files declare RTL layout. | Inno Setup 7 rejects RTL flags as language-entry parameters and both built-in files already set them. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-15 | Keep automatic archive extraction for Inno Setup 7. | The web installer needs automatic format detection for its downloaded packages. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-15 | Use supported boolean negation in offline Inno sections. | Inno Setup rejected the Pascal-style `not` token in preprocessor conditions. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-14 | Keep the Salamatrix Framework visible and mandatory in every installer mode. | Preserve the required framework while preventing configuration changes or explicit removal from omitting it. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-14 | Add a catalog-driven web installer that downloads only selected packages. | Offer a smaller installer while verifying every selected archive against the catalog SHA-256. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-14 | Expand the offline installer with localized categories, runtime notices, managed upgrades, and explicit removal. | Deliver the offline-first installer workflow while preserving component-owned files and dependencies. | | openrouter/openai/gpt-6-luna-pro |
; | 2026-11-06 | Rename JSON Web Viewer to JSON Viewer (nodeJS). | Match the requested product name in the installer plugin list. |  | Composer |
; | 2026-11-05 | Ship the Salamatrix JSON Web Viewer extension. | Offer a lightweight WebView2 alternative to JSON Viewer .NET. |  | Composer |
; | 2026-11-04 | Ship one optional langpack.dll instead of per-language .slg files. | English is embedded in host binaries; non-English languages are optional. | | Composer |
; | 2026-11-04 | Rename the product to Open Samandarin. | Distinguish this community fork from upstream Open Salamander. | https://github.com/KRtkovo-eu-AI/salamander/issues/776 | Grok 4.7 |
; Inno Setup script for Open Samandarin x64.
;
; This file is intentionally kept in the runbook folder next to setup_x64.inf.
; It mirrors the file list, shortcut locations, registry keys and private setup
; metadata from doc/runbook-setup/setup_x64.inf as closely as Inno Setup allows.
;
; Build example:
;   python tools\catalogs\generate_inno_package_catalog.py
;   iscc.exe "doc\runbook-setup\inno_setup_salamander_x64.iss" /DPayloadDir="H:\_projects\salamander\output\salamander\Release_x64"
;   iscc.exe "doc\runbook-setup\inno_setup_salamander_x64.iss" /DPayloadDir="H:\_projects\salamander\output\salamander\Release_x64" /DWebInstaller=True
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

#define AppToInstallName "Open Samandarin"
#define SamandarinVersion "0.16"
#define AppToInstallDisplayName "Open Samandarin " + SamandarinVersion + " (x64)"
#define AppToInstallVersion "5.0-samandarin-" + SamandarinVersion
#define AppToInstallPublisher "Ondřej Kotas (KRtekTM)"
#define AppToInstallCopyright "© 2026 " + AppToInstallPublisher + " KRtkovo.eu"
#define AppToInstallURL "https://samandarin.net/"
#define AppToInstallExeName "salamand.exe"
#define AppToInstallRegPath = "'Software\Open Samandarin\5.0-samandarin-" + SamandarinVersion + "'"
#define AppToInstallRegPathRem = "'HKCU\Software\Open Samandarin\5.0-samandarin-" + SamandarinVersion + "'"
#ifndef WebInstaller
  #define WebInstaller False
#endif
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
ArchiveExtraction=auto
Compression=lzma2/ultra64
ChangesAssociations=yes
CreateUninstallRegKey=yes
DefaultDirName={code:GetDefaultDirName}
DefaultGroupName={#AppToInstallName}
DisableDirPage=no
DisableProgramGroupPage=yes
DisableFinishedPage=yes
#if WebInstaller
OutputBaseFilename={#AppToInstallVersion}_web_win_x64
#else
OutputBaseFilename={#AppToInstallVersion}_win_x64
#endif
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
Name: "arabic"; MessagesFile: "compiler:Languages\Arabic.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "bulgarian"; MessagesFile: "compiler:Languages\Bulgarian.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "chinesetraditional"; MessagesFile: "compiler:Languages\ChineseTraditional.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "croatian"; MessagesFile: "compiler:Languages\Croatian.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "greek"; MessagesFile: "compiler:Languages\Greek.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "hebrew"; MessagesFile: "compiler:Languages\Hebrew.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "indonesian"; MessagesFile: "compiler:Languages\Indonesian.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "swedish"; MessagesFile: "compiler:Languages\Swedish.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "vietnamese"; MessagesFile: "compiler:Languages\Vietnamese.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "catalan"; MessagesFile: "compiler:Languages\Catalan.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "lithuanian"; MessagesFile: "compiler:Languages\Lithuanian.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"; LicenseFile: "{#SourcePath}\license.txt"
Name: "slovenian"; MessagesFile: "compiler:Languages\Slovenian.isl"; LicenseFile: "{#SourcePath}\license.txt"

[CustomMessages]
english.RemoveUserConfigQuestion=Do you want to remove the Open Samandarin user configuration?
english.AdditionalLanguages=Additional UI languages (language pack)
english.LanguagesGroup=Languages:
chinesesimplified.AdditionalLanguages=其他界面语言（语言包）
chinesesimplified.LanguagesGroup=语言：
czech.AdditionalLanguages=Další jazyky rozhraní (jazykový balíček)
czech.LanguagesGroup=Jazyky:
dutch.AdditionalLanguages=Extra UI-talen (taalpakket)
dutch.LanguagesGroup=Talen:
french.AdditionalLanguages=Langues d''interface supplémentaires (pack de langues)
french.LanguagesGroup=Langues :
german.AdditionalLanguages=Zusätzliche UI-Sprachen (Sprachpaket)
german.LanguagesGroup=Sprachen:
hungarian.AdditionalLanguages=További felületi nyelvek (nyelvi csomag)
hungarian.LanguagesGroup=Nyelvek:
italian.AdditionalLanguages=Lingue UI aggiuntive (language pack)
italian.LanguagesGroup=Lingue:
romanian.AdditionalLanguages=Limbi UI suplimentare (pachet de limbi)
romanian.LanguagesGroup=Limbi:
russian.AdditionalLanguages=Дополнительные языки интерфейса (языковой пакет)
russian.LanguagesGroup=Языки:
slovak.AdditionalLanguages=Ďalšie jazyky rozhrania (jazykový balík)
slovak.LanguagesGroup=Jazyky:
spanish.AdditionalLanguages=Idiomas de interfaz adicionales (paquete de idiomas)
spanish.LanguagesGroup=Idiomas:
english.FileStorage=File storage:
english.RegistryKey=Registry key:
english.RemoveUserConfigFiles=Choose Yes to delete the configuration files, or No to keep your settings.
english.RemoveUserConfigRegistry=Choose Yes to delete the key including all contents, or No to keep your settings.
chinesesimplified.RemoveUserConfigQuestion=是否要删除 Open Samandarin 用户配置？
chinesesimplified.FileStorage=文件存储:
chinesesimplified.RegistryKey=注册表项:
chinesesimplified.RemoveUserConfigFiles=选择“是”删除配置文件，选择“否”保留您的设置。
chinesesimplified.RemoveUserConfigRegistry=选择“是”删除该项及其所有内容，选择“否”保留您的设置。
czech.RemoveUserConfigQuestion=Chcete odstranit uživatelskou konfiguraci Open Samandarin?
czech.FileStorage=Úložiště souborů:
czech.RegistryKey=Klíč registru:
czech.RemoveUserConfigFiles=Zvolte Ano pro odstranění konfiguračních souborů, nebo Ne pro zachování nastavení.
czech.RemoveUserConfigRegistry=Zvolte Ano pro odstranění klíče včetně celého obsahu, nebo Ne pro zachování nastavení.
dutch.RemoveUserConfigQuestion=Wilt u de gebruikersconfiguratie van Open Samandarin verwijderen?
dutch.FileStorage=Bestandsopslag:
dutch.RegistryKey=Registersleutel:
dutch.RemoveUserConfigFiles=Kies Ja om de configuratiebestanden te verwijderen, of Nee om uw instellingen te behouden.
dutch.RemoveUserConfigRegistry=Kies Ja om de sleutel inclusief alle inhoud te verwijderen, of Nee om uw instellingen te behouden.
french.RemoveUserConfigQuestion=Voulez-vous supprimer la configuration utilisateur d’Open Samandarin ?
french.FileStorage=Stockage des fichiers :
french.RegistryKey=Clé de registre :
french.RemoveUserConfigFiles=Choisissez Oui pour supprimer les fichiers de configuration, ou Non pour conserver vos paramètres.
french.RemoveUserConfigRegistry=Choisissez Oui pour supprimer la clé avec tout son contenu, ou Non pour conserver vos paramètres.
german.RemoveUserConfigQuestion=Möchten Sie die Benutzerkonfiguration von Open Samandarin entfernen?
german.FileStorage=Dateispeicher:
german.RegistryKey=Registrierungsschlüssel:
german.RemoveUserConfigFiles=Wählen Sie Ja, um die Konfigurationsdateien zu löschen, oder Nein, um Ihre Einstellungen beizubehalten.
german.RemoveUserConfigRegistry=Wählen Sie Ja, um den Schlüssel einschließlich aller Inhalte zu löschen, oder Nein, um Ihre Einstellungen beizubehalten.
hungarian.RemoveUserConfigQuestion=Szeretné eltávolítani az Open Samandarin felhasználói konfigurációját?
hungarian.FileStorage=Fájltároló:
hungarian.RegistryKey=Beállításkulcs:
hungarian.RemoveUserConfigFiles=Válassza az Igen lehetőséget a konfigurációs fájlok törléséhez, vagy a Nem lehetőséget a beállítások megtartásához.
hungarian.RemoveUserConfigRegistry=Válassza az Igen lehetőséget a kulcs és teljes tartalmának törléséhez, vagy a Nem lehetőséget a beállítások megtartásához.
italian.RemoveUserConfigQuestion=Vuoi rimuovere la configurazione utente di Open Samandarin?
italian.FileStorage=Archiviazione file:
italian.RegistryKey=Chiave del Registro di sistema:
italian.RemoveUserConfigFiles=Scegli Sì per eliminare i file di configurazione oppure No per mantenere le impostazioni.
italian.RemoveUserConfigRegistry=Scegli Sì per eliminare la chiave e tutto il suo contenuto oppure No per mantenere le impostazioni.
romanian.RemoveUserConfigQuestion=Doriți să eliminați configurația de utilizator Open Samandarin?
romanian.FileStorage=Stocare în fișiere:
romanian.RegistryKey=Cheie de registru:
romanian.RemoveUserConfigFiles=Alegeți Da pentru a șterge fișierele de configurare sau Nu pentru a păstra setările.
romanian.RemoveUserConfigRegistry=Alegeți Da pentru a șterge cheia, inclusiv tot conținutul, sau Nu pentru a păstra setările.
russian.RemoveUserConfigQuestion=Удалить пользовательскую конфигурацию Open Samandarin?
russian.FileStorage=Файловое хранилище:
russian.RegistryKey=Раздел реестра:
russian.RemoveUserConfigFiles=Выберите «Да», чтобы удалить файлы конфигурации, или «Нет», чтобы сохранить настройки.
russian.RemoveUserConfigRegistry=Выберите «Да», чтобы удалить раздел со всем содержимым, или «Нет», чтобы сохранить настройки.
slovak.RemoveUserConfigQuestion=Chcete odstrániť používateľskú konfiguráciu Open Samandarin?
slovak.FileStorage=Úložisko súborov:
slovak.RegistryKey=Kľúč registra:
slovak.RemoveUserConfigFiles=Zvoľte Áno na odstránenie konfiguračných súborov alebo Nie na zachovanie nastavení.
slovak.RemoveUserConfigRegistry=Zvoľte Áno na odstránenie kľúča vrátane celého obsahu alebo Nie na zachovanie nastavení.
spanish.RemoveUserConfigQuestion=¿Desea eliminar la configuración de usuario de Open Samandarin?
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
english.PluginSelectionMode=Selection mode:
english.SimpleSelectionMode=Simple categories
english.ExpertSelectionMode=Expert: full plugin list
english.CategoryViewers=View files
english.CategoryArchives=Archives and formats
english.CategoryNetwork=Network and devices
english.CategorySystem=System tools
english.PluginRemovalTitle=Remove installed components
english.PluginRemovalDescription=Select components to remove. Only setup-managed files are removed; other files are kept.
english.PluginRemovalDependencyError=%s is required by another selected component. Select that dependent component for removal first.
english.RuntimeDependencyNotice=Some selected components require an external runtime that was not detected. The component will still be installed; use the official links below.
english.OfficialRuntimeDownload=Official download:
english.ManagedInventorySaveError=The managed component inventory could not be saved. Future upgrades may leave obsolete component files behind.
english.WebPackageUnavailable=%s is unavailable in the online installer because its package has no published SHA-256 digest. Use the offline installer or try again after the package catalog is updated.
english.WebInstallFailure=The selected packages could not be downloaded, verified, or prepared. Check your connection and try again.
chinesesimplified.WebPackageUnavailable=在线安装程序无法提供 %s，因为该软件包尚未发布 SHA-256 摘要。请使用离线安装程序，或在软件包目录更新后重试。
chinesesimplified.WebInstallFailure=无法下载、验证或准备所选软件包。请检查网络连接后重试。
chinesetraditional.WebPackageUnavailable=線上安裝程式無法提供 %s，因為該套件尚未發佈 SHA-256 摘要。請使用離線安裝程式，或在套件目錄更新後重試。
chinesetraditional.WebInstallFailure=無法下載、驗證或準備所選套件。請檢查網路連線後再試。
czech.WebPackageUnavailable=%s není v online instalačním programu dostupný, protože k balíčku nebyl zveřejněn kontrolní součet SHA-256. Použijte offline instalační program nebo to zkuste znovu po aktualizaci katalogu balíčků.
czech.WebInstallFailure=Vybrané balíčky se nepodařilo stáhnout, ověřit ani připravit. Zkontrolujte připojení k internetu a zkuste to znovu.
dutch.WebPackageUnavailable=%s is niet beschikbaar in het online installatieprogramma, omdat er geen SHA-256-controlesom voor het pakket is gepubliceerd. Gebruik het offline installatieprogramma of probeer het opnieuw nadat de pakketcatalogus is bijgewerkt.
dutch.WebInstallFailure=De geselecteerde pakketten konden niet worden gedownload, geverifieerd of voorbereid. Controleer uw verbinding en probeer het opnieuw.
french.WebPackageUnavailable=%s n’est pas disponible dans le programme d’installation en ligne, car aucune empreinte SHA-256 n’a été publiée pour ce paquet. Utilisez le programme d’installation hors ligne ou réessayez après la mise à jour du catalogue.
french.WebInstallFailure=Les paquets sélectionnés n’ont pas pu être téléchargés, vérifiés ou préparés. Vérifiez votre connexion et réessayez.
german.WebPackageUnavailable=%s ist im Online-Installationsprogramm nicht verfügbar, da für das Paket kein SHA-256-Prüfwert veröffentlicht wurde. Verwenden Sie das Offline-Installationsprogramm oder versuchen Sie es nach einer Aktualisierung des Paketkatalogs erneut.
german.WebInstallFailure=Die ausgewählten Pakete konnten nicht heruntergeladen, überprüft oder vorbereitet werden. Überprüfen Sie die Verbindung und versuchen Sie es erneut.
hungarian.WebPackageUnavailable=%s nem érhető el az online telepítőben, mert a csomaghoz nem tettek közzé SHA-256 ellenőrzőösszeget. Használja az offline telepítőt, vagy próbálja újra a csomagkatalógus frissítése után.
hungarian.WebInstallFailure=A kiválasztott csomagokat nem sikerült letölteni, ellenőrizni vagy előkészíteni. Ellenőrizze a kapcsolatot, majd próbálja újra.
italian.WebPackageUnavailable=%s non è disponibile nel programma di installazione online perché non è stato pubblicato un digest SHA-256 per il pacchetto. Usa il programma offline oppure riprova dopo l''aggiornamento del catalogo.
italian.WebInstallFailure=Impossibile scaricare, verificare o preparare i pacchetti selezionati. Controlla la connessione e riprova.
romanian.WebPackageUnavailable=%s nu este disponibil în programul de instalare online deoarece pachetul nu are un digest SHA-256 publicat. Utilizați programul offline sau încercați din nou după actualizarea catalogului.
romanian.WebInstallFailure=Pachetele selectate nu au putut fi descărcate, verificate sau pregătite. Verificați conexiunea și încercați din nou.
russian.WebPackageUnavailable=%s недоступен в онлайн-установщике, поскольку для пакета не опубликована контрольная сумма SHA-256. Используйте автономный установщик или повторите попытку после обновления каталога.
russian.WebInstallFailure=Не удалось загрузить, проверить или подготовить выбранные пакеты. Проверьте подключение и повторите попытку.
slovak.WebPackageUnavailable=%s nie je dostupný v online inštalátore, pretože pre balík nebol zverejnený kontrolný súčet SHA-256. Použite offline inštalátor alebo to skúste znova po aktualizácii katalógu.
slovak.WebInstallFailure=Vybrané balíky sa nepodarilo stiahnuť, overiť ani pripraviť. Skontrolujte pripojenie a skúste to znova.
spanish.WebPackageUnavailable=%s no está disponible en el instalador en línea porque el paquete no tiene un hash SHA-256 publicado. Use el instalador sin conexión o inténtelo de nuevo cuando se actualice el catálogo.
spanish.WebInstallFailure=No se pudieron descargar, verificar o preparar los paquetes seleccionados. Compruebe la conexión e inténtelo de nuevo.
arabic.WebPackageUnavailable=المكون %s غير متاح في برنامج التثبيت عبر الإنترنت لأنه لم يُنشر له ملخص SHA-256. استخدم برنامج التثبيت دون اتصال أو أعد المحاولة بعد تحديث الكتالوج.
arabic.WebInstallFailure=تعذر تنزيل الحزم المحددة أو التحقق منها أو تجهيزها. تحقق من الاتصال ثم أعد المحاولة.
bulgarian.WebPackageUnavailable=%s не е наличен в онлайн инсталатора, защото за пакета няма публикуван SHA-256 хеш. Използвайте офлайн инсталатора или опитайте отново след актуализиране на каталога.
bulgarian.WebInstallFailure=Избраните пакети не можаха да бъдат изтеглени, проверени или подготвени. Проверете връзката и опитайте отново.
croatian.WebPackageUnavailable=%s nije dostupan u mrežnom instalacijskom programu jer za paket nije objavljen SHA-256 sažetak. Upotrijebite izvanmrežni instalacijski program ili pokušajte ponovno nakon ažuriranja kataloga.
croatian.WebInstallFailure=Odabrane pakete nije moguće preuzeti, provjeriti ili pripremiti. Provjerite vezu i pokušajte ponovno.
greek.WebPackageUnavailable=Το %s δεν είναι διαθέσιμο στο διαδικτυακό πρόγραμμα εγκατάστασης, επειδή δεν έχει δημοσιευτεί αποτύπωμα SHA-256 για το πακέτο. Χρησιμοποιήστε το πρόγραμμα εγκατάστασης εκτός σύνδεσης ή δοκιμάστε ξανά μετά την ενημέρωση του καταλόγου.
greek.WebInstallFailure=Δεν ήταν δυνατή η λήψη, η επαλήθευση ή η προετοιμασία των επιλεγμένων πακέτων. Ελέγξτε τη σύνδεσή σας και δοκιμάστε ξανά.
hebrew.WebPackageUnavailable=%s אינו זמין בתוכנית ההתקנה המקוונת משום שלא פורסם עבור החבילה תקציר SHA-256. השתמש בתוכנית ההתקנה הלא מקוונת או נסה שוב לאחר עדכון הקטלוג.
hebrew.WebInstallFailure=לא ניתן להוריד, לאמת או להכין את החבילות שנבחרו. בדוק את החיבור ונסה שוב.
indonesian.WebPackageUnavailable=%s tidak tersedia di penginstal daring karena paketnya belum memiliki digest SHA-256 yang diterbitkan. Gunakan penginstal luring atau coba lagi setelah katalog paket diperbarui.
indonesian.WebInstallFailure=Paket yang dipilih tidak dapat diunduh, diverifikasi, atau disiapkan. Periksa koneksi Anda lalu coba lagi.
japanese.WebPackageUnavailable=%s はパッケージの SHA-256 ダイジェストが公開されていないため、オンラインインストーラーでは利用できません。オフラインインストーラーを使用するか、カタログ更新後に再試行してください。
japanese.WebInstallFailure=選択したパッケージをダウンロード、検証、または準備できませんでした。接続を確認して再試行してください。
korean.WebPackageUnavailable=%s 패키지의 SHA-256 다이제스트가 게시되지 않아 온라인 설치 프로그램에서 사용할 수 없습니다. 오프라인 설치 프로그램을 사용하거나 패키지 카탈로그가 업데이트된 후 다시 시도하세요.
korean.WebInstallFailure=선택한 패키지를 다운로드, 검증 또는 준비하지 못했습니다. 연결을 확인한 후 다시 시도하세요.
polish.WebPackageUnavailable=%s jest niedostępny w instalatorze online, ponieważ nie opublikowano sumy SHA-256 pakietu. Użyj instalatora offline lub spróbuj ponownie po aktualizacji katalogu.
polish.WebInstallFailure=Nie udało się pobrać, zweryfikować ani przygotować wybranych pakietów. Sprawdź połączenie i spróbuj ponownie.
  brazilianportuguese.WebPackageUnavailable=%s não está disponível no instalador online porque ainda não foi publicada a soma de verificação SHA-256 do pacote. Use o instalador offline ou tente novamente após a atualização do catálogo de pacotes.
  brazilianportuguese.WebInstallFailure=Não foi possível baixar, verificar ou preparar os pacotes selecionados. Verifique sua conexão e tente novamente.
swedish.WebPackageUnavailable=%s är inte tillgängligt i onlineinstallationsprogrammet eftersom inget SHA-256-sammandrag har publicerats för paketet. Använd offlineinstallationsprogrammet eller försök igen när katalogen har uppdaterats.
swedish.WebInstallFailure=De valda paketen kunde inte laddas ned, verifieras eller förberedas. Kontrollera anslutningen och försök igen.
turkish.WebPackageUnavailable=%s, paket için SHA-256 özeti yayımlanmadığından çevrimiçi yükleyicide kullanılamıyor. Çevrimdışı yükleyiciyi kullanın veya paket kataloğu güncellendikten sonra yeniden deneyin.
turkish.WebInstallFailure=Seçilen paketler indirilemedi, doğrulanamadı veya hazırlanamadı. Bağlantınızı kontrol edip yeniden deneyin.
ukrainian.WebPackageUnavailable=%s недоступний в онлайн-інсталяторі, оскільки для пакета не опубліковано дайджест SHA-256. Скористайтеся офлайн-інсталятором або повторіть спробу після оновлення каталогу.
ukrainian.WebInstallFailure=Не вдалося завантажити, перевірити або підготувати вибрані пакети. Перевірте з’єднання та спробуйте ще раз.
vietnamese.WebPackageUnavailable=%s không có trong trình cài đặt trực tuyến vì gói chưa có mã SHA-256 được công bố. Hãy dùng trình cài đặt ngoại tuyến hoặc thử lại sau khi danh mục được cập nhật.
vietnamese.WebInstallFailure=Không thể tải xuống, xác minh hoặc chuẩn bị các gói đã chọn. Hãy kiểm tra kết nối rồi thử lại.
english.SalamatrixFrameworkRequired=Salamatrix Framework (required)
chinesesimplified.SalamatrixFrameworkRequired=Salamatrix Framework（必需）
chinesetraditional.SalamatrixFrameworkRequired=Salamatrix Framework（必要）
czech.SalamatrixFrameworkRequired=Salamatrix Framework (povinný)
dutch.SalamatrixFrameworkRequired=Salamatrix Framework (vereist)
french.SalamatrixFrameworkRequired=Salamatrix Framework (obligatoire)
german.SalamatrixFrameworkRequired=Salamatrix Framework (erforderlich)
hungarian.SalamatrixFrameworkRequired=Salamatrix Framework (kötelező)
italian.SalamatrixFrameworkRequired=Salamatrix Framework (obbligatorio)
romanian.SalamatrixFrameworkRequired=Salamatrix Framework (obligatoriu)
russian.SalamatrixFrameworkRequired=Salamatrix Framework (обязательно)
slovak.SalamatrixFrameworkRequired=Salamatrix Framework (povinný)
spanish.SalamatrixFrameworkRequired=Salamatrix Framework (obligatorio)
arabic.SalamatrixFrameworkRequired=Salamatrix Framework (مطلوب)
bulgarian.SalamatrixFrameworkRequired=Salamatrix Framework (задължителен)
croatian.SalamatrixFrameworkRequired=Salamatrix Framework (obavezan)
greek.SalamatrixFrameworkRequired=Salamatrix Framework (απαιτείται)
hebrew.SalamatrixFrameworkRequired=Salamatrix Framework (נדרש)
indonesian.SalamatrixFrameworkRequired=Salamatrix Framework (wajib)
japanese.SalamatrixFrameworkRequired=Salamatrix Framework（必須）
korean.SalamatrixFrameworkRequired=Salamatrix Framework (필수)
polish.SalamatrixFrameworkRequired=Salamatrix Framework (wymagany)
brazilianportuguese.SalamatrixFrameworkRequired=Salamatrix Framework (obrigatório)
swedish.SalamatrixFrameworkRequired=Salamatrix Framework (obligatoriskt)
turkish.SalamatrixFrameworkRequired=Salamatrix Framework (gerekli)
ukrainian.SalamatrixFrameworkRequired=Salamatrix Framework (обов’язковий)
vietnamese.SalamatrixFrameworkRequired=Salamatrix Framework (bắt buộc)
english.SalamatrixFrameworkNotice=Salamatrix Framework is required and will always be included.
chinesesimplified.SalamatrixFrameworkNotice=Salamatrix Framework 为必需组件，始终会安装。
chinesetraditional.SalamatrixFrameworkNotice=Salamatrix Framework 為必要元件，將一律安裝。
czech.SalamatrixFrameworkNotice=Salamatrix Framework je povinný a bude vždy zahrnut.
dutch.SalamatrixFrameworkNotice=Salamatrix Framework is vereist en wordt altijd geïnstalleerd.
french.SalamatrixFrameworkNotice=Salamatrix Framework est obligatoire et sera toujours inclus.
german.SalamatrixFrameworkNotice=Salamatrix Framework ist erforderlich und wird immer mitinstalliert.
hungarian.SalamatrixFrameworkNotice=A Salamatrix Framework kötelező, és mindig települ.
italian.SalamatrixFrameworkNotice=Salamatrix Framework è obbligatorio e verrà sempre incluso.
romanian.SalamatrixFrameworkNotice=Salamatrix Framework este obligatoriu și va fi inclus întotdeauna.
russian.SalamatrixFrameworkNotice=Salamatrix Framework обязателен и будет установлен в любом случае.
slovak.SalamatrixFrameworkNotice=Salamatrix Framework je povinný a vždy bude zahrnutý.
spanish.SalamatrixFrameworkNotice=Salamatrix Framework es obligatorio y siempre se incluirá.
arabic.SalamatrixFrameworkNotice=إطار Salamatrix مطلوب وسيتم تضمينه دائمًا.
bulgarian.SalamatrixFrameworkNotice=Salamatrix Framework е задължителен и винаги ще бъде включен.
croatian.SalamatrixFrameworkNotice=Salamatrix Framework je obavezan i uvijek će biti uključen.
greek.SalamatrixFrameworkNotice=Το Salamatrix Framework είναι υποχρεωτικό και θα περιλαμβάνεται πάντα.
hebrew.SalamatrixFrameworkNotice=Salamatrix Framework נדרש וייכלל תמיד.
indonesian.SalamatrixFrameworkNotice=Salamatrix Framework wajib dan akan selalu disertakan.
japanese.SalamatrixFrameworkNotice=Salamatrix Framework は必須で、常にインストールされます。
korean.SalamatrixFrameworkNotice=Salamatrix Framework는 필수이며 항상 포함됩니다.
polish.SalamatrixFrameworkNotice=Salamatrix Framework jest wymagany i będzie zawsze dołączany.
brazilianportuguese.SalamatrixFrameworkNotice=O Salamatrix Framework é obrigatório e sempre será incluído.
swedish.SalamatrixFrameworkNotice=Salamatrix Framework krävs och kommer alltid att inkluderas.
turkish.SalamatrixFrameworkNotice=Salamatrix Framework gereklidir ve her zaman kurulacaktır.
ukrainian.SalamatrixFrameworkNotice=Salamatrix Framework є обов’язковим і завжди буде включений.
vietnamese.SalamatrixFrameworkNotice=Salamatrix Framework là bắt buộc và sẽ luôn được cài đặt.
chinesesimplified.PluginRemovalTitle=移除已安装组件
chinesesimplified.PluginRemovalDescription=选择要移除的组件。仅删除安装程序管理的文件；其他文件将保留。
chinesesimplified.PluginRemovalDependencyError=%s 是其他已选组件的依赖项。请先选择移除该依赖组件。
chinesetraditional.PluginRemovalTitle=移除已安裝元件
chinesetraditional.PluginRemovalDescription=選擇要移除的元件。僅刪除安裝程式管理的檔案；其他檔案將予以保留。
chinesetraditional.PluginRemovalDependencyError=%s 是其他已選元件的相依項目。請先選擇移除該相依元件。
czech.PluginRemovalTitle=Odebrat nainstalované komponenty
czech.PluginRemovalDescription=Zvolte komponenty k odebrání. Odstraní se pouze soubory spravované instalátorem; ostatní soubory zůstanou zachovány.
czech.PluginRemovalDependencyError=%s vyžaduje jiná vybraná komponenta. Nejprve zvolte odebrání této závislé komponenty.
dutch.PluginRemovalTitle=Geïnstalleerde componenten verwijderen
dutch.PluginRemovalDescription=Selecteer componenten om te verwijderen. Alleen door het installatieprogramma beheerde bestanden worden verwijderd; andere bestanden blijven behouden.
dutch.PluginRemovalDependencyError=%s is vereist door een andere geselecteerde component. Selecteer eerst die afhankelijke component om te verwijderen.
french.PluginRemovalTitle=Supprimer les composants installés
french.PluginRemovalDescription=Sélectionnez les composants à supprimer. Seuls les fichiers gérés par le programme d’installation seront supprimés ; les autres seront conservés.
french.PluginRemovalDependencyError=%s est requis par un autre composant sélectionné. Sélectionnez d’abord ce composant dépendant pour le supprimer.
german.PluginRemovalTitle=Installierte Komponenten entfernen
german.PluginRemovalDescription=Wählen Sie zu entfernende Komponenten aus. Nur vom Setup verwaltete Dateien werden gelöscht; andere Dateien bleiben erhalten.
german.PluginRemovalDependencyError=%s wird von einer anderen ausgewählten Komponente benötigt. Wählen Sie zuerst diese abhängige Komponente zum Entfernen aus.
hungarian.PluginRemovalTitle=Telepített összetevők eltávolítása
hungarian.PluginRemovalDescription=Válassza ki az eltávolítandó összetevőket. Csak a telepítő által kezelt fájlok törlődnek; a többi fájl megmarad.
hungarian.PluginRemovalDependencyError=A(z) %s összetevőre egy másik kiválasztott összetevőnek szüksége van. Először azt a függő összetevőt válassza ki eltávolításra.
italian.PluginRemovalTitle=Rimuovi i componenti installati
italian.PluginRemovalDescription=Seleziona i componenti da rimuovere. Verranno eliminati solo i file gestiti dal programma di installazione; gli altri saranno conservati.
italian.PluginRemovalDependencyError=%s è richiesto da un altro componente selezionato. Seleziona prima quel componente dipendente per rimuoverlo.
romanian.PluginRemovalTitle=Eliminarea componentelor instalate
romanian.PluginRemovalDescription=Selectați componentele de eliminat. Vor fi șterse doar fișierele gestionate de programul de instalare; celelalte vor fi păstrate.
romanian.PluginRemovalDependencyError=%s este necesar pentru o altă componentă selectată. Selectați mai întâi acea componentă dependentă pentru eliminare.
russian.PluginRemovalTitle=Удаление установленных компонентов
russian.PluginRemovalDescription=Выберите компоненты для удаления. Будут удалены только файлы, управляемые установщиком; остальные файлы сохранятся.
russian.PluginRemovalDependencyError=Компонент %s требуется другой выбранной компоненте. Сначала выберите для удаления зависимый компонент.
slovak.PluginRemovalTitle=Odstrániť nainštalované komponenty
slovak.PluginRemovalDescription=Vyberte komponenty na odstránenie. Odstránia sa iba súbory spravované inštalátorom; ostatné súbory zostanú zachované.
slovak.PluginRemovalDependencyError=%s vyžaduje iná vybraná komponenta. Najprv vyberte na odstránenie túto závislú komponentu.
spanish.PluginRemovalTitle=Eliminar componentes instalados
spanish.PluginRemovalDescription=Seleccione los componentes que desea eliminar. Solo se eliminarán los archivos gestionados por el instalador; los demás se conservarán.
spanish.PluginRemovalDependencyError=%s es necesario para otro componente seleccionado. Seleccione primero ese componente dependiente para eliminarlo.
arabic.PluginRemovalTitle=إزالة المكونات المثبتة
arabic.PluginRemovalDescription=حدد المكونات التي تريد إزالتها. ستتم إزالة الملفات التي يديرها برنامج التثبيت فقط؛ وستبقى الملفات الأخرى.
arabic.PluginRemovalDependencyError=يتطلب مكون آخر محدد المكون %s. حدد المكون التابع لإزالته أولاً.
bulgarian.PluginRemovalTitle=Премахване на инсталирани компоненти
bulgarian.PluginRemovalDescription=Изберете компонентите за премахване. Ще се изтрият само файловете, управлявани от инсталатора; останалите ще бъдат запазени.
bulgarian.PluginRemovalDependencyError=%s се изисква от друг избран компонент. Първо изберете зависимия компонент за премахване.
croatian.PluginRemovalTitle=Uklanjanje instaliranih komponenti
croatian.PluginRemovalDescription=Odaberite komponente za uklanjanje. Uklonit će se samo datoteke kojima upravlja instalacijski program; ostale će se sačuvati.
croatian.PluginRemovalDependencyError=%s je potreban drugoj odabranoj komponenti. Najprije odaberite tu ovisnu komponentu za uklanjanje.
greek.PluginRemovalTitle=Κατάργηση εγκατεστημένων στοιχείων
greek.PluginRemovalDescription=Επιλέξτε τα στοιχεία που θέλετε να καταργήσετε. Θα διαγραφούν μόνο τα αρχεία που διαχειρίζεται το πρόγραμμα εγκατάστασης· τα υπόλοιπα θα διατηρηθούν.
greek.PluginRemovalDependencyError=Το %s απαιτείται από άλλο επιλεγμένο στοιχείο. Επιλέξτε πρώτα αυτό το εξαρτώμενο στοιχείο για κατάργηση.
hebrew.PluginRemovalTitle=הסרת רכיבים מותקנים
hebrew.PluginRemovalDescription=בחר את הרכיבים להסרה. יוסרו רק קבצים המנוהלים על ידי תוכנית ההתקנה; קבצים אחרים יישמרו.
hebrew.PluginRemovalDependencyError=רכיב אחר שנבחר זקוק ל־%s. בחר תחילה את הרכיב התלוי להסרה.
indonesian.PluginRemovalTitle=Hapus komponen terpasang
indonesian.PluginRemovalDescription=Pilih komponen yang akan dihapus. Hanya berkas yang dikelola oleh pemasang yang dihapus; berkas lain tetap disimpan.
indonesian.PluginRemovalDependencyError=%s diperlukan oleh komponen lain yang dipilih. Pilih komponen yang bergantung tersebut untuk dihapus terlebih dahulu.
japanese.PluginRemovalTitle=インストール済みコンポーネントの削除
japanese.PluginRemovalDescription=削除するコンポーネントを選択します。インストーラーが管理するファイルのみ削除され、その他のファイルは保持されます。
japanese.PluginRemovalDependencyError=%s は別の選択済みコンポーネントに必要です。先に依存するコンポーネントを削除対象に選択してください。
korean.PluginRemovalTitle=설치된 구성 요소 제거
korean.PluginRemovalDescription=제거할 구성 요소를 선택하세요. 설치 프로그램이 관리하는 파일만 제거되고 다른 파일은 유지됩니다.
korean.PluginRemovalDependencyError=%s은(는) 다른 선택된 구성 요소에 필요합니다. 먼저 해당 종속 구성 요소를 제거하도록 선택하세요.
polish.PluginRemovalTitle=Usuń zainstalowane składniki
polish.PluginRemovalDescription=Wybierz składniki do usunięcia. Zostaną usunięte tylko pliki zarządzane przez instalator; pozostałe pliki zostaną zachowane.
polish.PluginRemovalDependencyError=%s jest wymagany przez inny wybrany składnik. Najpierw wybierz ten zależny składnik do usunięcia.
brazilianportuguese.PluginRemovalTitle=Remover componentes instalados
brazilianportuguese.PluginRemovalDescription=Selecione os componentes que deseja remover. Somente os arquivos gerenciados pelo instalador serão excluídos; os demais serão mantidos.
brazilianportuguese.PluginRemovalDependencyError=O componente %s é dependência de outro componente selecionado. Selecione primeiro esse componente dependente para remoção.
swedish.PluginRemovalTitle=Ta bort installerade komponenter
swedish.PluginRemovalDescription=Välj komponenter som ska tas bort. Endast installationshanterade filer tas bort; övriga filer behålls.
swedish.PluginRemovalDependencyError=%s krävs av en annan vald komponent. Välj först den beroende komponenten för borttagning.
turkish.PluginRemovalTitle=Yüklü bileşenleri kaldır
turkish.PluginRemovalDescription=Kaldırılacak bileşenleri seçin. Yalnızca yükleyicinin yönettiği dosyalar kaldırılır; diğer dosyalar korunur.
turkish.PluginRemovalDependencyError=%s başka bir seçili bileşen tarafından gereklidir. Önce bu bağımlı bileşeni kaldırılmak üzere seçin.
ukrainian.PluginRemovalTitle=Видалення встановлених компонентів
ukrainian.PluginRemovalDescription=Виберіть компоненти для видалення. Буде видалено лише файли, якими керує інсталятор; інші файли залишаться.
ukrainian.PluginRemovalDependencyError=%s потрібен іншому вибраному компоненту. Спочатку виберіть для видалення залежний компонент.
vietnamese.PluginRemovalTitle=Gỡ cài đặt các thành phần
vietnamese.PluginRemovalDescription=Chọn các thành phần cần gỡ. Chỉ tệp do trình cài đặt quản lý mới bị xóa; các tệp khác được giữ lại.
vietnamese.PluginRemovalDependencyError=%s là thành phần cần thiết cho một thành phần khác đã chọn. Hãy chọn gỡ thành phần phụ thuộc đó trước.
chinesesimplified.RuntimeDependencyNotice=部分所选组件需要未检测到的外部运行时。仍会安装这些组件；请使用下方的官方下载链接。
chinesesimplified.OfficialRuntimeDownload=官方下载：
chinesetraditional.RuntimeDependencyNotice=部分所選元件需要未偵測到的外部執行階段。仍會安裝這些元件；請使用下方的官方下載連結。
chinesetraditional.OfficialRuntimeDownload=官方下載：
czech.RuntimeDependencyNotice=Některé vybrané komponenty vyžadují nedetekované externí prostředí. Komponenta se přesto nainstaluje; použijte níže uvedené oficiální odkazy.
czech.OfficialRuntimeDownload=Oficiální stažení:
dutch.RuntimeDependencyNotice=Sommige geselecteerde componenten vereisen een externe runtime die niet is gevonden. De component wordt toch geïnstalleerd; gebruik de officiële links hieronder.
dutch.OfficialRuntimeDownload=Officiële download:
french.RuntimeDependencyNotice=Certains composants sélectionnés nécessitent un environnement d’exécution externe non détecté. Le composant sera tout de même installé ; utilisez les liens officiels ci-dessous.
french.OfficialRuntimeDownload=Téléchargement officiel :
german.RuntimeDependencyNotice=Einige ausgewählte Komponenten benötigen eine nicht erkannte externe Laufzeit. Die Komponente wird trotzdem installiert; verwenden Sie die offiziellen Links unten.
german.OfficialRuntimeDownload=Offizieller Download:
hungarian.RuntimeDependencyNotice=Néhány kiválasztott összetevőhöz nem észlelt külső futtatókörnyezet szükséges. Az összetevő ettől még települ; használja az alábbi hivatalos hivatkozásokat.
hungarian.OfficialRuntimeDownload=Hivatalos letöltés:
italian.RuntimeDependencyNotice=Alcuni componenti selezionati richiedono un runtime esterno non rilevato. Il componente verrà comunque installato; usa i collegamenti ufficiali qui sotto.
italian.OfficialRuntimeDownload=Download ufficiale:
romanian.RuntimeDependencyNotice=Unele componente selectate necesită un mediu extern care nu a fost detectat. Componenta va fi instalată oricum; utilizați linkurile oficiale de mai jos.
romanian.OfficialRuntimeDownload=Descărcare oficială:
russian.RuntimeDependencyNotice=Некоторым выбранным компонентам требуется не обнаруженная внешняя среда выполнения. Компонент всё равно будет установлен; используйте официальные ссылки ниже.
russian.OfficialRuntimeDownload=Официальная загрузка:
slovak.RuntimeDependencyNotice=Niektoré vybrané komponenty vyžadujú nezistené externé prostredie. Komponent sa napriek tomu nainštaluje; použite oficiálne odkazy nižšie.
slovak.OfficialRuntimeDownload=Oficiálne stiahnutie:
spanish.RuntimeDependencyNotice=Algunos componentes seleccionados requieren un entorno externo que no se detectó. El componente se instalará igualmente; utilice los enlaces oficiales siguientes.
spanish.OfficialRuntimeDownload=Descarga oficial:
arabic.RemoveUserConfigQuestion=هل تريد إزالة تكوين مستخدم Open Samandarin؟
arabic.AdditionalLanguages=لغات واجهة إضافية (حزمة لغات)
arabic.LanguagesGroup=اللغات:
arabic.FileStorage=تخزين الملفات:
arabic.RegistryKey=مفتاح السجل:
arabic.RemoveUserConfigFiles=اختر نعم لحذف ملفات التكوين، أو لا للاحتفاظ بإعداداتك.
arabic.RemoveUserConfigRegistry=اختر نعم لحذف المفتاح بما في ذلك جميع محتوياته، أو لا للاحتفاظ بإعداداتك.
arabic.InstallMode=وضع التثبيت:
arabic.StandardInstall=تثبيت عادي (قائمة ابدأ، البرامج وبرنامج إزالة التثبيت)
arabic.PortableInstall=استخراج كنسخة محمولة (بدون إدخال في البرامج أو برنامج إزالة التثبيت)
arabic.PortableDirName=samandarin
arabic.StartMenuShortcut=إنشاء اختصار في قائمة &ابدأ
arabic.DesktopShortcut=إنشاء اختصار على &سطح المكتب
arabic.Shortcuts=الاختصارات:
arabic.CodePageWarningTitle=تحذير توافق صفحة الرموز
arabic.CodePageWarning=قد لا تعرض اللغة المحددة جميع الأحرف بشكل صحيح لأن صفحة رموز إعدادات النظام في Windows (%u) لا تطابق صفحة الرموز المتوقعة لهذه اللغة (%u).\n\nلحل هذه المشكلة، غيّر إعدادات النظام المحلية في Windows: الإعدادات > الوقت واللغة > المنطقة > الإدارية > تغيير إعدادات النظام المحلية.
arabic.KeepConfigQuestion=تم العثور على ملف تكوين موجود (configstorage.ini).\n\nستتم إعادة تسميته إلى configstorage.ini.BAK وسيتم تثبيت التكوين الافتراضي الجديد.
arabic.PluginSelectionTitle=تحديد المكونات الإضافية
arabic.PluginSelectionDescription=اختر المكونات الإضافية المضمنة التي تريد تثبيتها أو استخراجها.
arabic.PluginConfigurationLabel=تكوين المكونات الإضافية:
arabic.PluginConfigurationBase=أساسي (محلي فقط، دون اتصال بالإنترنت)
arabic.PluginConfigurationStandard=قياسي (لمعظم المستخدمين)
arabic.PluginConfigurationAdvanced=متقدم (للمستخدمين المتقدمين)
arabic.PluginConfigurationExpert=خبير (للمتخصصين والمطورين)
arabic.PluginConfigurationCustom=مخصص
arabic.InstalledVersionLabel=الإصدار المثبت:
bulgarian.RemoveUserConfigQuestion=Искате ли да премахнете потребителската конфигурация на Open Samandarin?
bulgarian.AdditionalLanguages=Допълнителни езици на интерфейса (езиков пакет)
bulgarian.LanguagesGroup=Езици:
bulgarian.FileStorage=Файлово хранилище:
bulgarian.RegistryKey=Ключ в системния регистър:
bulgarian.RemoveUserConfigFiles=Изберете Да, за да изтриете конфигурационните файлове, или Не, за да запазите настройките.
bulgarian.RemoveUserConfigRegistry=Изберете Да, за да изтриете ключа с цялото му съдържание, или Не, за да запазите настройките.
bulgarian.InstallMode=Режим на инсталиране:
bulgarian.StandardInstall=Нормално инсталиране (меню „Старт“, Програми и деинсталатор)
bulgarian.PortableInstall=Извличане като преносима версия (без запис в „Програми“ и без деинсталатор)
bulgarian.PortableDirName=samandarin
bulgarian.StartMenuShortcut=Създаване на пряк път в менюто „&Старт“
bulgarian.DesktopShortcut=Създаване на пряк път на &работния плот
bulgarian.Shortcuts=Преки пътища:
bulgarian.CodePageWarningTitle=Предупреждение за съвместимост на кодовата страница
bulgarian.CodePageWarning=Избраният език може да не показва правилно всички знаци, защото кодовата страница на системния локал на Windows (%u) не съвпада с очакваната за този език кодова страница (%u).\n\nЗа да коригирате това, променете системния локал в Windows: Настройки > Час и език > Регион > Административни настройки > Промяна на системния локал.
bulgarian.KeepConfigQuestion=Открит е съществуващ конфигурационен файл (configstorage.ini).\n\nТой ще бъде преименуван на configstorage.ini.BAK и ще бъде инсталирана новата конфигурация по подразбиране.
bulgarian.PluginSelectionTitle=Избор на добавки
bulgarian.PluginSelectionDescription=Изберете кои включени добавки да бъдат инсталирани или извлечени.
bulgarian.PluginConfigurationLabel=Конфигурация на добавките:
bulgarian.PluginConfigurationBase=Основна (само локално, без интернет)
bulgarian.PluginConfigurationStandard=Стандартна (за повечето потребители)
bulgarian.PluginConfigurationAdvanced=Разширена (за напреднали потребители)
bulgarian.PluginConfigurationExpert=Експертна (за специалисти и разработчици)
bulgarian.PluginConfigurationCustom=Персонализирана
bulgarian.InstalledVersionLabel=Инсталирана версия:
chinesetraditional.RemoveUserConfigQuestion=要移除 Open Samandarin 使用者設定嗎？
chinesetraditional.AdditionalLanguages=其他介面語言（語言套件）
chinesetraditional.LanguagesGroup=語言：
chinesetraditional.FileStorage=檔案儲存：
chinesetraditional.RegistryKey=登錄機碼：
chinesetraditional.RemoveUserConfigFiles=選擇「是」以刪除設定檔，或選擇「否」以保留設定。
chinesetraditional.RemoveUserConfigRegistry=選擇「是」以刪除包含所有內容的機碼，或選擇「否」以保留設定。
chinesetraditional.InstallMode=安裝模式：
chinesetraditional.StandardInstall=一般安裝（開始功能表、程式集和解除安裝程式）
chinesetraditional.PortableInstall=解壓縮為可攜式版本（不建立程式集項目或解除安裝程式）
chinesetraditional.PortableDirName=samandarin
chinesetraditional.StartMenuShortcut=建立「開始」功能表捷徑(&S)
chinesetraditional.DesktopShortcut=建立桌面捷徑(&D)
chinesetraditional.Shortcuts=捷徑：
chinesetraditional.CodePageWarningTitle=字碼頁相容性警告
chinesetraditional.CodePageWarning=選取的語言可能無法正確顯示所有字元，因為 Windows 系統地區設定的字碼頁 (%u) 與此語言預期的字碼頁 (%u) 不符。\n\n若要修正，請在 Windows 中變更系統地區設定：設定 > 時間與語言 > 地區 > 系統管理 > 變更系統地區設定。
chinesetraditional.KeepConfigQuestion=找到現有的設定檔 (configstorage.ini)。\n\n此檔案將重新命名為 configstorage.ini.BAK，並安裝新的預設設定。
chinesetraditional.PluginSelectionTitle=選取外掛程式
chinesetraditional.PluginSelectionDescription=選擇要安裝或解壓縮的隨附外掛程式。
chinesetraditional.PluginConfigurationLabel=外掛程式設定：
chinesetraditional.PluginConfigurationBase=基本（僅本機，不連線網際網路）
chinesetraditional.PluginConfigurationStandard=標準（適合大多數使用者）
chinesetraditional.PluginConfigurationAdvanced=進階（適合進階使用者）
chinesetraditional.PluginConfigurationExpert=專家（適合專家與開發人員）
chinesetraditional.PluginConfigurationCustom=自訂
chinesetraditional.InstalledVersionLabel=已安裝版本：
croatian.RemoveUserConfigQuestion=Želite li ukloniti korisničku konfiguraciju Open Samandarin?
croatian.AdditionalLanguages=Dodatni jezici sučelja (jezični paket)
croatian.LanguagesGroup=Jezici:
croatian.FileStorage=Pohrana datoteka:
croatian.RegistryKey=Ključ registra:
croatian.RemoveUserConfigFiles=Odaberite Da za brisanje konfiguracijskih datoteka ili Ne za zadržavanje postavki.
croatian.RemoveUserConfigRegistry=Odaberite Da za brisanje ključa sa svim sadržajem ili Ne za zadržavanje postavki.
croatian.InstallMode=Način instalacije:
croatian.StandardInstall=Uobičajena instalacija (izbornik Start, Programi i deinstalacijski program)
croatian.PortableInstall=Raspakiraj kao prijenosnu verziju (bez stavke u Programima i deinstalacijskog programa)
croatian.PortableDirName=samandarin
croatian.StartMenuShortcut=Stvori prečac u izborniku &Start
croatian.DesktopShortcut=Stvori prečac na &radnoj površini
croatian.Shortcuts=Prečaci:
croatian.CodePageWarningTitle=Upozorenje o kompatibilnosti kodne stranice
croatian.CodePageWarning=Odabrani jezik možda neće ispravno prikazivati sve znakove jer se kodna stranica regionalnih postavki sustava Windows (%u) ne podudara s kodnom stranicom koju očekuje ovaj jezik (%u).\n\nDa biste to ispravili, promijenite regionalne postavke sustava u sustavu Windows: Postavke > Vrijeme i jezik > Regija > Administrativno > Promjena regionalnih postavki sustava.
croatian.KeepConfigQuestion=Pronađena je postojeća konfiguracijska datoteka (configstorage.ini).\n\nBit će preimenovana u configstorage.ini.BAK i instalirat će se nova zadana konfiguracija.
croatian.PluginSelectionTitle=Odabir dodataka
croatian.PluginSelectionDescription=Odaberite koje uključene dodatke želite instalirati ili raspakirati.
croatian.PluginConfigurationLabel=Konfiguracija dodataka:
croatian.PluginConfigurationBase=Osnovna (samo lokalno, bez interneta)
croatian.PluginConfigurationStandard=Standardna (za većinu korisnika)
croatian.PluginConfigurationAdvanced=Napredna (za napredne korisnike)
croatian.PluginConfigurationExpert=Stručna (za stručnjake i programere)
croatian.PluginConfigurationCustom=Prilagođena
croatian.InstalledVersionLabel=Instalirana verzija:
greek.RemoveUserConfigQuestion=Θέλετε να καταργήσετε τη διαμόρφωση χρήστη του Open Samandarin;
greek.AdditionalLanguages=Πρόσθετες γλώσσες περιβάλλοντος εργασίας (πακέτο γλώσσας)
greek.LanguagesGroup=Γλώσσες:
greek.FileStorage=Αποθήκευση αρχείων:
greek.RegistryKey=Κλειδί μητρώου:
greek.RemoveUserConfigFiles=Επιλέξτε Ναι για να διαγράψετε τα αρχεία διαμόρφωσης ή Όχι για να διατηρήσετε τις ρυθμίσεις σας.
greek.RemoveUserConfigRegistry=Επιλέξτε Ναι για να διαγράψετε το κλειδί μαζί με όλα τα περιεχόμενά του ή Όχι για να διατηρήσετε τις ρυθμίσεις σας.
greek.InstallMode=Λειτουργία εγκατάστασης:
greek.StandardInstall=Κανονική εγκατάσταση (μενού Έναρξη, Προγράμματα και πρόγραμμα απεγκατάστασης)
greek.PortableInstall=Εξαγωγή ως φορητή έκδοση (χωρίς καταχώριση στα Προγράμματα ή πρόγραμμα απεγκατάστασης)
greek.PortableDirName=samandarin
greek.StartMenuShortcut=Δημιουργία συντόμευσης στο μενού &Έναρξη
greek.DesktopShortcut=Δημιουργία συντόμευσης στην &επιφάνεια εργασίας
greek.Shortcuts=Συντομεύσεις:
greek.CodePageWarningTitle=Προειδοποίηση συμβατότητας σελίδας κωδικών
greek.CodePageWarning=Η επιλεγμένη γλώσσα ενδέχεται να μην εμφανίζει σωστά όλους τους χαρακτήρες, επειδή η σελίδα κωδικών της τοπικής ρύθμισης των Windows (%u) δεν αντιστοιχεί στη σελίδα κωδικών που αναμένει αυτή η γλώσσα (%u).\n\nΓια να το διορθώσετε, αλλάξτε την τοπική ρύθμιση συστήματος στα Windows: Ρυθμίσεις > Ώρα και γλώσσα > Περιοχή > Διαχείριση > Αλλαγή τοπικών ρυθμίσεων συστήματος.
greek.KeepConfigQuestion=Βρέθηκε υπάρχον αρχείο διαμόρφωσης (configstorage.ini).\n\nΘα μετονομαστεί σε configstorage.ini.BAK και θα εγκατασταθεί η νέα προεπιλεγμένη διαμόρφωση.
greek.PluginSelectionTitle=Επιλογή πρόσθετων
greek.PluginSelectionDescription=Επιλέξτε ποια ενσωματωμένα πρόσθετα θα εγκατασταθούν ή θα εξαχθούν.
greek.PluginConfigurationLabel=Διαμόρφωση πρόσθετων:
greek.PluginConfigurationBase=Βασική (μόνο τοπικά, χωρίς πρόσβαση στο Internet)
greek.PluginConfigurationStandard=Τυπική (για τους περισσότερους χρήστες)
greek.PluginConfigurationAdvanced=Για προχωρημένους χρήστες
greek.PluginConfigurationExpert=Ειδική (για ειδικούς και προγραμματιστές)
greek.PluginConfigurationCustom=Προσαρμοσμένη
greek.InstalledVersionLabel=Εγκατεστημένη έκδοση:
hebrew.RemoveUserConfigQuestion=האם להסיר את תצורת המשתמש של Open Samandarin?
hebrew.AdditionalLanguages=שפות ממשק נוספות (חבילת שפה)
hebrew.LanguagesGroup=שפות:
hebrew.FileStorage=אחסון קבצים:
hebrew.RegistryKey=מפתח רישום:
hebrew.RemoveUserConfigFiles=בחר כן כדי למחוק את קובצי התצורה, או לא כדי לשמור את ההגדרות.
hebrew.RemoveUserConfigRegistry=בחר כן כדי למחוק את המפתח ואת כל תוכנו, או לא כדי לשמור את ההגדרות.
hebrew.InstallMode=מצב התקנה:
hebrew.StandardInstall=התקנה רגילה (תפריט התחלה, תוכניות ומסיר התקנה)
hebrew.PortableInstall=חלץ כגרסה ניידת (ללא רשומה בתוכניות וללא מסיר התקנה)
hebrew.PortableDirName=samandarin
hebrew.StartMenuShortcut=צור קיצור דרך בתפריט &התחלה
hebrew.DesktopShortcut=צור קיצור דרך ב&שולחן העבודה
hebrew.Shortcuts=קיצורי דרך:
hebrew.CodePageWarningTitle=אזהרת תאימות של דף קוד
hebrew.CodePageWarning=ייתכן שהשפה שנבחרה לא תציג את כל התווים כראוי, משום שדף הקוד של האזור במערכת Windows (%u) אינו תואם לדף הקוד הנדרש לשפה זו (%u).\n\nכדי לתקן זאת, שנה את אזור המערכת ב-Windows: הגדרות > זמן ושפה > אזור > ניהול > שינוי אזור מערכת.
hebrew.KeepConfigQuestion=נמצא קובץ תצורה קיים (configstorage.ini).\n\nשמו ישונה ל-configstorage.ini.BAK והתצורה החדשה המוגדרת כברירת מחדל תותקן.
hebrew.PluginSelectionTitle=בחירת תוספים
hebrew.PluginSelectionDescription=בחר אילו תוספים הכלולים בחבילה יותקנו או ייחלצו.
hebrew.PluginConfigurationLabel=תצורת תוספים:
hebrew.PluginConfigurationBase=בסיסית (מקומית בלבד, ללא גישה לאינטרנט)
hebrew.PluginConfigurationStandard=רגילה (לרוב המשתמשים)
hebrew.PluginConfigurationAdvanced=מתקדמת (למשתמשים מתקדמים)
hebrew.PluginConfigurationExpert=מומחית (למומחים ולמפתחים)
hebrew.PluginConfigurationCustom=מותאמת אישית
hebrew.InstalledVersionLabel=גרסה מותקנת:
indonesian.RemoveUserConfigQuestion=Apakah Anda ingin menghapus konfigurasi pengguna Open Samandarin?
indonesian.AdditionalLanguages=Bahasa antarmuka tambahan (paket bahasa)
indonesian.LanguagesGroup=Bahasa:
indonesian.FileStorage=Penyimpanan berkas:
indonesian.RegistryKey=Kunci registri:
indonesian.RemoveUserConfigFiles=Pilih Ya untuk menghapus berkas konfigurasi, atau Tidak untuk mempertahankan pengaturan Anda.
indonesian.RemoveUserConfigRegistry=Pilih Ya untuk menghapus kunci beserta seluruh isinya, atau Tidak untuk mempertahankan pengaturan Anda.
indonesian.InstallMode=Mode instalasi:
indonesian.StandardInstall=Instalasi normal (Menu Mulai, Program, dan penghapus instalasi)
indonesian.PortableInstall=Ekstrak sebagai versi portabel (tanpa entri Program atau penghapus instalasi)
indonesian.PortableDirName=samandarin
indonesian.StartMenuShortcut=Buat pintasan Menu &Mulai
indonesian.DesktopShortcut=Buat pintasan &desktop
indonesian.Shortcuts=Pintasan:
indonesian.CodePageWarningTitle=Peringatan kompatibilitas halaman kode
indonesian.CodePageWarning=Bahasa yang dipilih mungkin tidak menampilkan semua karakter dengan benar karena halaman kode lokal sistem Windows (%u) tidak sesuai dengan halaman kode yang diharapkan bahasa ini (%u).\n\nUntuk memperbaikinya, ubah lokal sistem di Windows: Pengaturan > Waktu & Bahasa > Wilayah > Administratif > Ubah lokal sistem.
indonesian.KeepConfigQuestion=Berkas konfigurasi yang sudah ada (configstorage.ini) ditemukan.\n\nBerkas tersebut akan diganti namanya menjadi configstorage.ini.BAK dan konfigurasi bawaan yang baru akan dipasang.
indonesian.PluginSelectionTitle=Pilih plugin
indonesian.PluginSelectionDescription=Pilih plugin bawaan yang akan dipasang atau diekstrak.
indonesian.PluginConfigurationLabel=Konfigurasi plugin:
indonesian.PluginConfigurationBase=Dasar (hanya lokal, tanpa akses Internet)
indonesian.PluginConfigurationStandard=Standar (untuk sebagian besar pengguna)
indonesian.PluginConfigurationAdvanced=Lanjutan (untuk pengguna tingkat lanjut)
indonesian.PluginConfigurationExpert=Ahli (untuk pakar dan pengembang)
indonesian.PluginConfigurationCustom=Kustom
indonesian.InstalledVersionLabel=Versi terpasang:
japanese.RemoveUserConfigQuestion=Open Samandarin のユーザー設定を削除しますか？
japanese.AdditionalLanguages=追加の UI 言語（言語パック）
japanese.LanguagesGroup=言語:
japanese.FileStorage=ファイル保存先:
japanese.RegistryKey=レジストリキー:
japanese.RemoveUserConfigFiles=設定ファイルを削除するには「はい」を、設定を保持するには「いいえ」を選択してください。
japanese.RemoveUserConfigRegistry=内容を含めてキーを削除するには「はい」を、設定を保持するには「いいえ」を選択してください。
japanese.InstallMode=インストールモード:
japanese.StandardInstall=通常インストール（スタートメニュー、プログラム一覧、アンインストーラー）
japanese.PortableInstall=ポータブル版として展開（プログラム一覧への登録やアンインストーラーなし）
japanese.PortableDirName=samandarin
japanese.StartMenuShortcut=スタートメニューのショートカットを作成(&S)
japanese.DesktopShortcut=デスクトップのショートカットを作成(&D)
japanese.Shortcuts=ショートカット:
japanese.CodePageWarningTitle=コードページ互換性の警告
japanese.CodePageWarning=Windows のシステムロケールのコードページ (%u) がこの言語で必要なコードページ (%u) と一致しないため、選択した言語ですべての文字を正しく表示できない場合があります。\n\n修正するには、Windows のシステムロケールを変更してください: 設定 > 時刻と言語 > 地域 > 管理用の設定 > システムロケールの変更。
japanese.KeepConfigQuestion=既存の設定ファイル (configstorage.ini) が見つかりました。\n\nconfigstorage.ini.BAK に名前を変更し、新しい既定の設定をインストールします。
japanese.PluginSelectionTitle=プラグインの選択
japanese.PluginSelectionDescription=インストールまたは展開する同梱プラグインを選択してください。
japanese.PluginConfigurationLabel=プラグイン構成:
japanese.PluginConfigurationBase=基本（ローカルのみ、インターネット接続なし）
japanese.PluginConfigurationStandard=標準（ほとんどのユーザー向け）
japanese.PluginConfigurationAdvanced=上級（上級ユーザー向け）
japanese.PluginConfigurationExpert=エキスパート（専門家と開発者向け）
japanese.PluginConfigurationCustom=カスタム
japanese.InstalledVersionLabel=インストール済みバージョン:
korean.RemoveUserConfigQuestion=Open Samandarin 사용자 구성을 제거하시겠습니까?
korean.AdditionalLanguages=추가 UI 언어(언어 팩)
korean.LanguagesGroup=언어:
korean.FileStorage=파일 저장소:
korean.RegistryKey=레지스트리 키:
korean.RemoveUserConfigFiles=구성 파일을 삭제하려면 예를, 설정을 유지하려면 아니요를 선택하세요.
korean.RemoveUserConfigRegistry=모든 내용과 함께 키를 삭제하려면 예를, 설정을 유지하려면 아니요를 선택하세요.
korean.InstallMode=설치 모드:
korean.StandardInstall=일반 설치 (시작 메뉴, 프로그램 항목 및 제거 프로그램)
korean.PortableInstall=휴대용 버전으로 압축 해제 (프로그램 항목 및 제거 프로그램 없음)
korean.PortableDirName=samandarin
korean.StartMenuShortcut=시작 메뉴 바로 가기 만들기(&S)
korean.DesktopShortcut=바탕 화면 바로 가기 만들기(&D)
korean.Shortcuts=바로 가기:
korean.CodePageWarningTitle=코드 페이지 호환성 경고
korean.CodePageWarning=Windows 시스템 로캘 코드 페이지(%u)가 이 언어에 필요한 코드 페이지(%u)와 일치하지 않아 선택한 언어에서 일부 문자가 올바르게 표시되지 않을 수 있습니다.\n\n해결하려면 Windows에서 시스템 로캘을 변경하세요: 설정 > 시간 및 언어 > 지역 > 관리 > 시스템 로캘 변경.
korean.KeepConfigQuestion=기존 구성 파일(configstorage.ini)을 찾았습니다.\n\n파일 이름을 configstorage.ini.BAK로 변경하고 새 기본 구성을 설치합니다.
korean.PluginSelectionTitle=플러그인 선택
korean.PluginSelectionDescription=설치하거나 압축 해제할 기본 제공 플러그인을 선택하세요.
korean.PluginConfigurationLabel=플러그인 구성:
korean.PluginConfigurationBase=기본 (로컬 전용, 인터넷 연결 없음)
korean.PluginConfigurationStandard=표준 (대부분의 사용자용)
korean.PluginConfigurationAdvanced=고급 (고급 사용자용)
korean.PluginConfigurationExpert=전문가 (전문가 및 개발자용)
korean.PluginConfigurationCustom=사용자 지정
korean.InstalledVersionLabel=설치된 버전:
polish.RemoveUserConfigQuestion=Czy usunąć konfigurację użytkownika Open Samandarin?
polish.AdditionalLanguages=Dodatkowe języki interfejsu (pakiet językowy)
polish.LanguagesGroup=Języki:
polish.FileStorage=Magazyn plików:
polish.RegistryKey=Klucz rejestru:
polish.RemoveUserConfigFiles=Wybierz Tak, aby usunąć pliki konfiguracji, lub Nie, aby zachować ustawienia.
polish.RemoveUserConfigRegistry=Wybierz Tak, aby usunąć klucz wraz z całą zawartością, lub Nie, aby zachować ustawienia.
polish.InstallMode=Tryb instalacji:
polish.StandardInstall=Zainstaluj normalnie (menu Start, Programy i deinstalator)
polish.PortableInstall=Wypakuj jako wersję przenośną (bez wpisu w Programach i bez deinstalatora)
polish.PortableDirName=samandarin
polish.StartMenuShortcut=Utwórz skrót w menu &Start
polish.DesktopShortcut=Utwórz skrót na &pulpicie
polish.Shortcuts=Skróty:
polish.CodePageWarningTitle=Ostrzeżenie o zgodności strony kodowej
polish.CodePageWarning=Wybrany język może nie wyświetlać poprawnie wszystkich znaków, ponieważ strona kodowa ustawień regionalnych systemu Windows (%u) nie jest zgodna ze stroną kodową wymaganą przez ten język (%u).\n\nAby to naprawić, zmień ustawienia regionalne systemu Windows: Ustawienia > Czas i język > Region > Administracyjne > Zmień ustawienia regionalne systemu.
polish.KeepConfigQuestion=Znaleziono istniejący plik konfiguracji (configstorage.ini).\n\nZostanie on przemianowany na configstorage.ini.BAK i zostanie zainstalowana nowa konfiguracja domyślna.
polish.PluginSelectionTitle=Wybierz wtyczki
polish.PluginSelectionDescription=Wybierz, które dołączone wtyczki mają zostać zainstalowane lub wypakowane.
polish.PluginConfigurationLabel=Konfiguracja wtyczek:
polish.PluginConfigurationBase=Podstawowa (tylko lokalnie, bez dostępu do Internetu)
polish.PluginConfigurationStandard=Standardowa (dla większości użytkowników)
polish.PluginConfigurationAdvanced=Zaawansowana (dla zaawansowanych użytkowników)
polish.PluginConfigurationExpert=Ekspercka (dla ekspertów i programistów)
polish.PluginConfigurationCustom=Niestandardowa
polish.InstalledVersionLabel=Zainstalowana wersja:
brazilianportuguese.RemoveUserConfigQuestion=Você deseja remover a configuração de usuário do Open Samandarin?
brazilianportuguese.AdditionalLanguages=Idiomas adicionais da interface (pacote de idiomas)
brazilianportuguese.LanguagesGroup=Idiomas:
brazilianportuguese.FileStorage=Armazenamento de arquivos:
brazilianportuguese.RegistryKey=Chave do Registro:
brazilianportuguese.RemoveUserConfigFiles=Selecione Sim para excluir os arquivos de configuração ou Não para manter as configurações.
brazilianportuguese.RemoveUserConfigRegistry=Selecione Sim para excluir a chave e todo o seu conteúdo ou Não para manter as configurações.
brazilianportuguese.InstallMode=Modo de instalação:
brazilianportuguese.StandardInstall=Instalar normalmente (menu Iniciar, Programas e desinstalador)
brazilianportuguese.PortableInstall=Extrair como versão portátil (sem entrada em Programas nem desinstalador)
brazilianportuguese.PortableDirName=samandarin
brazilianportuguese.StartMenuShortcut=Criar um atalho no menu &Iniciar
brazilianportuguese.DesktopShortcut=Criar um atalho na &Área de Trabalho
brazilianportuguese.Shortcuts=Atalhos:
brazilianportuguese.CodePageWarningTitle=Aviso de compatibilidade da página de código
brazilianportuguese.CodePageWarning=O idioma selecionado pode não exibir corretamente todos os caracteres porque a página de código da localidade do sistema Windows (%u) não corresponde à página de código esperada por este idioma (%u).\n\nPara corrigir, altere a localidade do sistema no Windows: Configurações > Hora e idioma > Idioma e região > Configurações administrativas de idioma > Alterar localidade do sistema.
brazilianportuguese.KeepConfigQuestion=Foi encontrado um arquivo de configuração existente (configstorage.ini).\n\nEle será renomeado para configstorage.ini.BAK e a nova configuração padrão será instalada.
brazilianportuguese.PluginSelectionTitle=Selecionar plugins
brazilianportuguese.PluginSelectionDescription=Escolha os plugins incluídos que deseja instalar ou extrair.
brazilianportuguese.PluginConfigurationLabel=Configuração dos plugins:
brazilianportuguese.PluginConfigurationBase=Básica (somente local, sem acesso à Internet)
brazilianportuguese.PluginConfigurationStandard=Padrão (para a maioria dos usuários)
brazilianportuguese.PluginConfigurationAdvanced=Avançada (para usuários avançados)
brazilianportuguese.PluginConfigurationExpert=Especialista (para especialistas e desenvolvedores)
brazilianportuguese.PluginConfigurationCustom=Personalizada
brazilianportuguese.InstalledVersionLabel=Versão instalada:
swedish.RemoveUserConfigQuestion=Vill du ta bort användarkonfigurationen för Open Samandarin?
swedish.AdditionalLanguages=Ytterligare gränssnittsspråk (språkpaket)
swedish.LanguagesGroup=Språk:
swedish.FileStorage=Fillagring:
swedish.RegistryKey=Registernyckel:
swedish.RemoveUserConfigFiles=Välj Ja för att ta bort konfigurationsfilerna eller Nej för att behålla inställningarna.
swedish.RemoveUserConfigRegistry=Välj Ja för att ta bort nyckeln inklusive allt innehåll eller Nej för att behålla inställningarna.
swedish.InstallMode=Installationsläge:
swedish.StandardInstall=Installera normalt (Start-menyn, Program och avinstallationsprogram)
swedish.PortableInstall=Packa upp som en portabel version (ingen post i Program och inget avinstallationsprogram)
swedish.PortableDirName=samandarin
swedish.StartMenuShortcut=Skapa en genväg i &Start-menyn
swedish.DesktopShortcut=Skapa en genväg på &skrivbordet
swedish.Shortcuts=Genvägar:
swedish.CodePageWarningTitle=Varning om kodsidans kompatibilitet
swedish.CodePageWarning=Det valda språket kanske inte visar alla tecken korrekt eftersom Windows-systemets nationella kodsida (%u) inte överensstämmer med kodsidan som språket förväntar sig (%u).\n\nÄndra systemets nationella inställning i Windows för att åtgärda detta: Inställningar > Tid och språk > Region > Administrativa inställningar > Ändra systemnationella inställningar.
swedish.KeepConfigQuestion=En befintlig konfigurationsfil (configstorage.ini) hittades.\n\nDen byter namn till configstorage.ini.BAK och den nya standardkonfigurationen installeras.
swedish.PluginSelectionTitle=Välj insticksprogram
swedish.PluginSelectionDescription=Välj vilka medföljande insticksprogram som ska installeras eller packas upp.
swedish.PluginConfigurationLabel=Insticksprogramskonfiguration:
swedish.PluginConfigurationBase=Grundläggande (endast lokalt, ingen internetåtkomst)
swedish.PluginConfigurationStandard=Standard (för de flesta användare)
swedish.PluginConfigurationAdvanced=Avancerad (för avancerade användare)
swedish.PluginConfigurationExpert=Expert (för experter och utvecklare)
swedish.PluginConfigurationCustom=Anpassad
swedish.InstalledVersionLabel=Installerad version:
turkish.RemoveUserConfigQuestion=Open Samandarin kullanıcı yapılandırmasını kaldırmak istiyor musunuz?
turkish.AdditionalLanguages=Ek kullanıcı arayüzü dilleri (dil paketi)
turkish.LanguagesGroup=Diller:
turkish.FileStorage=Dosya depolama:
turkish.RegistryKey=Kayıt defteri anahtarı:
turkish.RemoveUserConfigFiles=Yapılandırma dosyalarını silmek için Evet'i, ayarlarınızı korumak için Hayır'ı seçin.
turkish.RemoveUserConfigRegistry=Tüm içeriğiyle birlikte anahtarı silmek için Evet'i, ayarlarınızı korumak için Hayır'ı seçin.
turkish.InstallMode=Yükleme modu:
turkish.StandardInstall=Normal yükle (Başlat menüsü, Programlar ve kaldırıcı)
turkish.PortableInstall=Taşınabilir sürüm olarak çıkar (Programlar girdisi veya kaldırıcı yok)
turkish.PortableDirName=samandarin
turkish.StartMenuShortcut=Başlat menüsünde kısayol oluştur(&S)
turkish.DesktopShortcut=Masaüstünde kısayol oluştur(&D)
turkish.Shortcuts=Kısayollar:
turkish.CodePageWarningTitle=Kod sayfası uyumluluk uyarısı
turkish.CodePageWarning=Windows sistem yerel ayarınızın kod sayfası (%u) bu dilin beklediği kod sayfasıyla (%u) eşleşmediğinden seçilen dil tüm karakterleri doğru görüntüleyemeyebilir.\n\nBunu düzeltmek için Windows sistem yerel ayarını değiştirin: Ayarlar > Saat ve Dil > Bölge > Yönetim > Sistem yerel ayarını değiştir.
turkish.KeepConfigQuestion=Mevcut bir yapılandırma dosyası (configstorage.ini) bulundu.\n\nDosyanın adı configstorage.ini.BAK olarak değiştirilecek ve yeni varsayılan yapılandırma yüklenecek.
turkish.PluginSelectionTitle=Eklentileri seçin
turkish.PluginSelectionDescription=Yüklenecek veya çıkarılacak paket içindeki eklentileri seçin.
turkish.PluginConfigurationLabel=Eklenti yapılandırması:
turkish.PluginConfigurationBase=Temel (yalnızca yerel, İnternet erişimi yok)
turkish.PluginConfigurationStandard=Standart (çoğu kullanıcı için)
turkish.PluginConfigurationAdvanced=Gelişmiş (ileri düzey kullanıcılar için)
turkish.PluginConfigurationExpert=Uzman (uzmanlar ve geliştiriciler için)
turkish.PluginConfigurationCustom=Özel
turkish.InstalledVersionLabel=Yüklü sürüm:
ukrainian.RemoveUserConfigQuestion=Видалити конфігурацію користувача Open Samandarin?
ukrainian.AdditionalLanguages=Додаткові мови інтерфейсу (мовний пакет)
ukrainian.LanguagesGroup=Мови:
ukrainian.FileStorage=Файлове сховище:
ukrainian.RegistryKey=Ключ реєстру:
ukrainian.RemoveUserConfigFiles=Виберіть «Так», щоб видалити файли конфігурації, або «Ні», щоб зберегти налаштування.
ukrainian.RemoveUserConfigRegistry=Виберіть «Так», щоб видалити ключ разом із усім вмістом, або «Ні», щоб зберегти налаштування.
ukrainian.InstallMode=Режим встановлення:
ukrainian.StandardInstall=Звичайне встановлення (меню «Пуск», Програми та засіб видалення)
ukrainian.PortableInstall=Розпакувати як портативну версію (без запису в «Програмах» і без засобу видалення)
ukrainian.PortableDirName=samandarin
ukrainian.StartMenuShortcut=Створити ярлик у меню «&Пуск»
ukrainian.DesktopShortcut=Створити ярлик на &робочому столі
ukrainian.Shortcuts=Ярлики:
ukrainian.CodePageWarningTitle=Попередження про сумісність кодової сторінки
ukrainian.CodePageWarning=Вибрана мова може відображати не всі символи правильно, оскільки кодова сторінка системної локалі Windows (%u) не відповідає кодовій сторінці, потрібній цій мові (%u).\n\nЩоб це виправити, змініть системну локаль у Windows: Параметри > Час і мова > Регіон > Адміністративні параметри > Змінити системну локаль.
ukrainian.KeepConfigQuestion=Знайдено наявний файл конфігурації (configstorage.ini).\n\nЙого буде перейменовано на configstorage.ini.BAK і встановлено нову конфігурацію за замовчуванням.
ukrainian.PluginSelectionTitle=Вибір плагінів
ukrainian.PluginSelectionDescription=Виберіть, які вбудовані плагіни встановити або розпакувати.
ukrainian.PluginConfigurationLabel=Конфігурація плагінів:
ukrainian.PluginConfigurationBase=Базова (лише локально, без доступу до Інтернету)
ukrainian.PluginConfigurationStandard=Стандартна (для більшості користувачів)
ukrainian.PluginConfigurationAdvanced=Розширена (для досвідчених користувачів)
ukrainian.PluginConfigurationExpert=Експертна (для фахівців і розробників)
ukrainian.PluginConfigurationCustom=Власна
ukrainian.InstalledVersionLabel=Встановлена версія:
vietnamese.RemoveUserConfigQuestion=Bạn có muốn xóa cấu hình người dùng Open Samandarin không?
vietnamese.AdditionalLanguages=Ngôn ngữ giao diện bổ sung (gói ngôn ngữ)
vietnamese.LanguagesGroup=Ngôn ngữ:
vietnamese.FileStorage=Lưu trữ tệp:
vietnamese.RegistryKey=Khóa đăng ký:
vietnamese.RemoveUserConfigFiles=Chọn Có để xóa các tệp cấu hình hoặc Không để giữ lại cài đặt.
vietnamese.RemoveUserConfigRegistry=Chọn Có để xóa khóa cùng toàn bộ nội dung hoặc Không để giữ lại cài đặt.
vietnamese.InstallMode=Chế độ cài đặt:
vietnamese.StandardInstall=Cài đặt thông thường (Menu Bắt đầu, Chương trình và trình gỡ cài đặt)
vietnamese.PortableInstall=Giải nén thành phiên bản di động (không có mục Chương trình hoặc trình gỡ cài đặt)
vietnamese.PortableDirName=samandarin
vietnamese.StartMenuShortcut=Tạo lối tắt trong menu &Bắt đầu
vietnamese.DesktopShortcut=Tạo lối tắt trên &màn hình nền
vietnamese.Shortcuts=Lối tắt:
vietnamese.CodePageWarningTitle=Cảnh báo tương thích trang mã
vietnamese.CodePageWarning=Ngôn ngữ đã chọn có thể không hiển thị chính xác tất cả ký tự vì trang mã vùng hệ thống Windows (%u) không khớp với trang mã mà ngôn ngữ này yêu cầu (%u).\n\nĐể khắc phục, hãy thay đổi vùng hệ thống trong Windows: Cài đặt > Thời gian & Ngôn ngữ > Khu vực > Quản trị > Thay đổi vùng hệ thống.
vietnamese.KeepConfigQuestion=Đã tìm thấy tệp cấu hình hiện có (configstorage.ini).\n\nTệp sẽ được đổi tên thành configstorage.ini.BAK và cấu hình mặc định mới sẽ được cài đặt.
vietnamese.PluginSelectionTitle=Chọn plugin
vietnamese.PluginSelectionDescription=Chọn plugin đi kèm cần cài đặt hoặc giải nén.
vietnamese.PluginConfigurationLabel=Cấu hình plugin:
vietnamese.PluginConfigurationBase=Cơ bản (chỉ cục bộ, không truy cập Internet)
vietnamese.PluginConfigurationStandard=Tiêu chuẩn (cho hầu hết người dùng)
vietnamese.PluginConfigurationAdvanced=Nâng cao (cho người dùng nâng cao)
vietnamese.PluginConfigurationExpert=Chuyên gia (cho chuyên gia và nhà phát triển)
vietnamese.PluginConfigurationCustom=Tùy chỉnh
vietnamese.InstalledVersionLabel=Phiên bản đã cài đặt:
arabic.RuntimeDependencyNotice=تتطلب بعض المكونات المحددة بيئة تشغيل خارجية لم يتم اكتشافها. سيظل تثبيت المكون مستمراً؛ استخدم الروابط الرسمية أدناه.
arabic.OfficialRuntimeDownload=التنزيل الرسمي:
bulgarian.RuntimeDependencyNotice=Някои избрани компоненти изискват външна среда за изпълнение, която не е открита. Компонентът ще бъде инсталиран въпреки това; използвайте официалните връзки по-долу.
bulgarian.OfficialRuntimeDownload=Официално изтегляне:
croatian.RuntimeDependencyNotice=Neke odabrane komponente zahtijevaju vanjsko okruženje koje nije otkriveno. Komponenta će se ipak instalirati; upotrijebite službene poveznice u nastavku.
croatian.OfficialRuntimeDownload=Službeno preuzimanje:
greek.RuntimeDependencyNotice=Ορισμένα επιλεγμένα στοιχεία απαιτούν εξωτερικό περιβάλλον εκτέλεσης που δεν εντοπίστηκε. Το στοιχείο θα εγκατασταθεί ούτως ή άλλως· χρησιμοποιήστε τους επίσημους συνδέσμους παρακάτω.
greek.OfficialRuntimeDownload=Επίσημη λήψη:
hebrew.RuntimeDependencyNotice=חלק מהרכיבים שנבחרו דורשים סביבת ריצה חיצונית שלא זוהתה. הרכיב יותקן בכל זאת; השתמש בקישורים הרשמיים שלהלן.
hebrew.OfficialRuntimeDownload=הורדה רשמית:
indonesian.RuntimeDependencyNotice=Beberapa komponen yang dipilih memerlukan runtime eksternal yang tidak terdeteksi. Komponen tetap akan dipasang; gunakan tautan resmi di bawah.
indonesian.OfficialRuntimeDownload=Unduhan resmi:
japanese.RuntimeDependencyNotice=選択したコンポーネントの一部には検出されなかった外部ランタイムが必要です。コンポーネントはインストールされます。以下の公式リンクを使用してください。
japanese.OfficialRuntimeDownload=公式ダウンロード:
korean.RuntimeDependencyNotice=선택한 일부 구성 요소에 필요한 외부 런타임이 검색되지 않았습니다. 구성 요소는 계속 설치됩니다. 아래 공식 링크를 사용하세요.
korean.OfficialRuntimeDownload=공식 다운로드:
polish.RuntimeDependencyNotice=Niektóre wybrane składniki wymagają niewykrytego środowiska zewnętrznego. Składnik zostanie mimo to zainstalowany; użyj poniższych oficjalnych łączy.
polish.OfficialRuntimeDownload=Oficjalne pobieranie:
brazilianportuguese.RuntimeDependencyNotice=Alguns componentes selecionados precisam de um runtime externo que não foi detectado. O componente será instalado mesmo assim; use os links oficiais abaixo.
brazilianportuguese.OfficialRuntimeDownload=Download oficial:
swedish.RuntimeDependencyNotice=Vissa valda komponenter kräver en extern körmiljö som inte hittades. Komponenten installeras ändå; använd de officiella länkarna nedan.
swedish.OfficialRuntimeDownload=Officiell nedladdning:
turkish.RuntimeDependencyNotice=Seçilen bazı bileşenler algılanmayan bir dış çalışma zamanı gerektiriyor. Bileşen yine de yüklenecek; aşağıdaki resmi bağlantıları kullanın.
turkish.OfficialRuntimeDownload=Resmî indirme:
ukrainian.RuntimeDependencyNotice=Деяким вибраним компонентам потрібне зовнішнє середовище виконання, яке не виявлено. Компонент усе одно буде встановлено; скористайтеся офіційними посиланнями нижче.
ukrainian.OfficialRuntimeDownload=Офіційне завантаження:
vietnamese.RuntimeDependencyNotice=Một số thành phần đã chọn cần môi trường chạy bên ngoài nhưng không được phát hiện. Thành phần vẫn sẽ được cài đặt; hãy dùng các liên kết chính thức bên dưới.
vietnamese.OfficialRuntimeDownload=Tải xuống chính thức:
chinesesimplified.ManagedInventorySaveError=无法保存受管理的组件清单。未来升级可能会遗留过时的组件文件。
chinesetraditional.ManagedInventorySaveError=無法儲存受管理的元件清單。未來升級可能會留下過時的元件檔案。
czech.ManagedInventorySaveError=Seznam spravovaných komponent se nepodařilo uložit. Budoucí aktualizace mohou ponechat zastaralé soubory komponent.
dutch.ManagedInventorySaveError=De inventaris van beheerde componenten kon niet worden opgeslagen. Toekomstige upgrades kunnen verouderde componentbestanden achterlaten.
french.ManagedInventorySaveError=L’inventaire des composants gérés n’a pas pu être enregistré. De futures mises à niveau pourraient laisser des fichiers de composants obsolètes.
german.ManagedInventorySaveError=Das Inventar der verwalteten Komponenten konnte nicht gespeichert werden. Künftige Upgrades könnten veraltete Komponentendateien zurücklassen.
hungarian.ManagedInventorySaveError=A felügyelt összetevők leltárát nem sikerült menteni. A későbbi frissítések elavult összetevőfájlokat hagyhatnak hátra.
italian.ManagedInventorySaveError=Impossibile salvare l’inventario dei componenti gestiti. Gli aggiornamenti futuri potrebbero lasciare file di componenti obsoleti.
romanian.ManagedInventorySaveError=Inventarul componentelor gestionate nu a putut fi salvat. Actualizările viitoare pot lăsa fișiere de componente învechite.
russian.ManagedInventorySaveError=Не удалось сохранить список управляемых компонентов. При будущих обновлениях могут остаться устаревшие файлы компонентов.
slovak.ManagedInventorySaveError=Zoznam spravovaných komponentov sa nepodarilo uložiť. Budúce aktualizácie môžu ponechať zastarané súbory komponentov.
spanish.ManagedInventorySaveError=No se pudo guardar el inventario de componentes administrados. Las futuras actualizaciones podrían dejar archivos de componentes obsoletos.
arabic.ManagedInventorySaveError=تعذر حفظ قائمة المكونات المُدارة. قد تترك الترقيات المستقبلية ملفات مكونات قديمة.
bulgarian.ManagedInventorySaveError=Инвентарният списък на управляваните компоненти не можа да бъде записан. Бъдещи надстройки може да оставят остарели файлове.
croatian.ManagedInventorySaveError=Nije moguće spremiti popis upravljanih komponenti. Buduće nadogradnje mogu ostaviti zastarjele datoteke komponenti.
greek.ManagedInventorySaveError=Δεν ήταν δυνατή η αποθήκευση του καταλόγου διαχειριζόμενων στοιχείων. Μελλοντικές αναβαθμίσεις ενδέχεται να αφήσουν παλιά αρχεία στοιχείων.
hebrew.ManagedInventorySaveError=לא ניתן לשמור את רשימת הרכיבים המנוהלים. שדרוגים עתידיים עלולים להשאיר קובצי רכיבים מיושנים.
indonesian.ManagedInventorySaveError=Inventaris komponen terkelola tidak dapat disimpan. Peningkatan berikutnya mungkin meninggalkan berkas komponen usang.
japanese.ManagedInventorySaveError=管理対象コンポーネントの一覧を保存できませんでした。今後のアップグレードで古いコンポーネントファイルが残る可能性があります。
korean.ManagedInventorySaveError=관리되는 구성 요소 목록을 저장하지 못했습니다. 향후 업그레이드 후 오래된 구성 요소 파일이 남을 수 있습니다.
polish.ManagedInventorySaveError=Nie można zapisać spisu zarządzanych składników. Przyszłe aktualizacje mogą pozostawić nieaktualne pliki składników.
brazilianportuguese.ManagedInventorySaveError=Não foi possível salvar o inventário dos componentes gerenciados. Atualizações futuras podem deixar arquivos de componentes obsoletos.
swedish.ManagedInventorySaveError=Det gick inte att spara inventeringen av hanterade komponenter. Framtida uppgraderingar kan lämna kvar inaktuella komponentfiler.
turkish.ManagedInventorySaveError=Yönetilen bileşen envanteri kaydedilemedi. Gelecekteki yükseltmeler eski bileşen dosyalarını bırakabilir.
ukrainian.ManagedInventorySaveError=Не вдалося зберегти перелік керованих компонентів. Під час майбутніх оновлень можуть залишитися застарілі файли компонентів.
vietnamese.ManagedInventorySaveError=Không thể lưu danh mục thành phần được quản lý. Các lần nâng cấp sau có thể để lại tệp thành phần lỗi thời.
chinesesimplified.PluginSelectionMode=选择模式：
chinesesimplified.SimpleSelectionMode=简易分类
chinesesimplified.ExpertSelectionMode=专家：完整插件列表
chinesesimplified.CategoryViewers=查看文件
chinesesimplified.CategoryArchives=压缩包和格式
chinesesimplified.CategoryNetwork=网络和设备
chinesesimplified.CategorySystem=系统工具
czech.PluginSelectionMode=Režim výběru:
czech.SimpleSelectionMode=Jednoduché kategorie
czech.ExpertSelectionMode=Expert: úplný seznam pluginů
czech.CategoryViewers=Prohlížení souborů
czech.CategoryArchives=Archivy a formáty
czech.CategoryNetwork=Síť a zařízení
czech.CategorySystem=Systémové nástroje
dutch.PluginSelectionMode=Selectiemodus:
dutch.SimpleSelectionMode=Eenvoudige categorieën
dutch.ExpertSelectionMode=Expert: volledige pluginlijst
dutch.CategoryViewers=Bestanden bekijken
dutch.CategoryArchives=Archieven en indelingen
dutch.CategoryNetwork=Netwerk en apparaten
dutch.CategorySystem=Systeemhulpmiddelen
french.PluginSelectionMode=Mode de sélection :
french.SimpleSelectionMode=Catégories simples
french.ExpertSelectionMode=Expert : liste complète des modules
french.CategoryViewers=Afficher les fichiers
french.CategoryArchives=Archives et formats
french.CategoryNetwork=Réseau et appareils
french.CategorySystem=Outils système
german.PluginSelectionMode=Auswahlmodus:
german.SimpleSelectionMode=Einfache Kategorien
german.ExpertSelectionMode=Experte: vollständige Pluginliste
german.CategoryViewers=Dateien anzeigen
german.CategoryArchives=Archive und Formate
german.CategoryNetwork=Netzwerk und Geräte
german.CategorySystem=Systemwerkzeuge
hungarian.PluginSelectionMode=Kiválasztási mód:
hungarian.SimpleSelectionMode=Egyszerű kategóriák
hungarian.ExpertSelectionMode=Szakértő: teljes bővítménylista
hungarian.CategoryViewers=Fájlok megtekintése
hungarian.CategoryArchives=Archívumok és formátumok
hungarian.CategoryNetwork=Hálózat és eszközök
hungarian.CategorySystem=Rendszereszközök
italian.PluginSelectionMode=Modalità di selezione:
italian.SimpleSelectionMode=Categorie semplici
italian.ExpertSelectionMode=Esperto: elenco completo dei plugin
italian.CategoryViewers=Visualizzazione file
italian.CategoryArchives=Archivi e formati
italian.CategoryNetwork=Rete e dispositivi
italian.CategorySystem=Strumenti di sistema
romanian.PluginSelectionMode=Mod de selecție:
romanian.SimpleSelectionMode=Categorii simple
romanian.ExpertSelectionMode=Expert: lista completă de pluginuri
romanian.CategoryViewers=Vizualizare fișiere
romanian.CategoryArchives=Arhive și formate
romanian.CategoryNetwork=Rețea și dispozitive
romanian.CategorySystem=Instrumente de sistem
russian.PluginSelectionMode=Режим выбора:
russian.SimpleSelectionMode=Простые категории
russian.ExpertSelectionMode=Эксперт: полный список плагинов
russian.CategoryViewers=Просмотр файлов
russian.CategoryArchives=Архивы и форматы
russian.CategoryNetwork=Сеть и устройства
russian.CategorySystem=Системные инструменты
slovak.PluginSelectionMode=Režim výberu:
slovak.SimpleSelectionMode=Jednoduché kategórie
slovak.ExpertSelectionMode=Expert: úplný zoznam pluginov
slovak.CategoryViewers=Prehliadanie súborov
slovak.CategoryArchives=Archívy a formáty
slovak.CategoryNetwork=Sieť a zariadenia
slovak.CategorySystem=Systémové nástroje
spanish.PluginSelectionMode=Modo de selección:
spanish.SimpleSelectionMode=Categorías sencillas
spanish.ExpertSelectionMode=Experto: lista completa de complementos
spanish.CategoryViewers=Ver archivos
spanish.CategoryArchives=Archivos y formatos
spanish.CategoryNetwork=Red y dispositivos
spanish.CategorySystem=Herramientas del sistema
arabic.PluginSelectionMode=وضع التحديد:
arabic.SimpleSelectionMode=فئات مبسطة
arabic.ExpertSelectionMode=خبير: قائمة المكونات الإضافية كاملة
arabic.CategoryViewers=عرض الملفات
arabic.CategoryArchives=الأرشيفات والتنسيقات
arabic.CategoryNetwork=الشبكة والأجهزة
arabic.CategorySystem=أدوات النظام
bulgarian.PluginSelectionMode=Режим на избор:
bulgarian.SimpleSelectionMode=Опростени категории
bulgarian.ExpertSelectionMode=Експерт: пълен списък с плъгини
bulgarian.CategoryViewers=Преглед на файлове
bulgarian.CategoryArchives=Архиви и формати
bulgarian.CategoryNetwork=Мрежа и устройства
bulgarian.CategorySystem=Системни инструменти
chinesetraditional.PluginSelectionMode=選取模式：
chinesetraditional.SimpleSelectionMode=簡易分類
chinesetraditional.ExpertSelectionMode=專家：完整外掛程式清單
chinesetraditional.CategoryViewers=檢視檔案
chinesetraditional.CategoryArchives=封存檔與格式
chinesetraditional.CategoryNetwork=網路與裝置
chinesetraditional.CategorySystem=系統工具
croatian.PluginSelectionMode=Način odabira:
croatian.SimpleSelectionMode=Jednostavne kategorije
croatian.ExpertSelectionMode=Stručni način: potpuni popis dodataka
croatian.CategoryViewers=Pregled datoteka
croatian.CategoryArchives=Arhive i formati
croatian.CategoryNetwork=Mreža i uređaji
croatian.CategorySystem=Alati sustava
greek.PluginSelectionMode=Λειτουργία επιλογής:
greek.SimpleSelectionMode=Απλές κατηγορίες
greek.ExpertSelectionMode=Για προχωρημένους: πλήρης λίστα προσθέτων
greek.CategoryViewers=Προβολή αρχείων
greek.CategoryArchives=Αρχεία και μορφές
greek.CategoryNetwork=Δίκτυο και συσκευές
greek.CategorySystem=Εργαλεία συστήματος
hebrew.PluginSelectionMode=מצב בחירה:
hebrew.SimpleSelectionMode=קטגוריות פשוטות
hebrew.ExpertSelectionMode=מומחה: רשימת תוספים מלאה
hebrew.CategoryViewers=הצגת קבצים
hebrew.CategoryArchives=ארכיונים ופורמטים
hebrew.CategoryNetwork=רשת והתקנים
hebrew.CategorySystem=כלי מערכת
indonesian.PluginSelectionMode=Mode pilihan:
indonesian.SimpleSelectionMode=Kategori sederhana
indonesian.ExpertSelectionMode=Pakar: daftar plugin lengkap
indonesian.CategoryViewers=Melihat berkas
indonesian.CategoryArchives=Arsip dan format
indonesian.CategoryNetwork=Jaringan dan perangkat
indonesian.CategorySystem=Alat sistem
japanese.PluginSelectionMode=選択モード:
japanese.SimpleSelectionMode=簡易カテゴリ
japanese.ExpertSelectionMode=エキスパート: プラグイン一覧
japanese.CategoryViewers=ファイルの表示
japanese.CategoryArchives=アーカイブと形式
japanese.CategoryNetwork=ネットワークとデバイス
japanese.CategorySystem=システムツール
korean.PluginSelectionMode=선택 모드:
korean.SimpleSelectionMode=간단한 범주
korean.ExpertSelectionMode=전문가: 전체 플러그인 목록
korean.CategoryViewers=파일 보기
korean.CategoryArchives=압축 파일 및 형식
korean.CategoryNetwork=네트워크 및 장치
korean.CategorySystem=시스템 도구
polish.PluginSelectionMode=Tryb wyboru:
polish.SimpleSelectionMode=Proste kategorie
polish.ExpertSelectionMode=Ekspert: pełna lista wtyczek
polish.CategoryViewers=Przeglądanie plików
polish.CategoryArchives=Archiwa i formaty
polish.CategoryNetwork=Sieć i urządzenia
polish.CategorySystem=Narzędzia systemowe
brazilianportuguese.PluginSelectionMode=Modo de seleção:
brazilianportuguese.SimpleSelectionMode=Categorias simples
brazilianportuguese.ExpertSelectionMode=Especialista: lista completa de plugins
brazilianportuguese.CategoryViewers=Visualizadores de arquivos
brazilianportuguese.CategoryArchives=Arquivos e formatos
brazilianportuguese.CategoryNetwork=Rede e dispositivos
brazilianportuguese.CategorySystem=Ferramentas do sistema
swedish.PluginSelectionMode=Valläge:
swedish.SimpleSelectionMode=Enkla kategorier
swedish.ExpertSelectionMode=Expert: fullständig pluginlista
swedish.CategoryViewers=Visa filer
swedish.CategoryArchives=Arkiv och format
swedish.CategoryNetwork=Nätverk och enheter
swedish.CategorySystem=Systemverktyg
turkish.PluginSelectionMode=Seçim modu:
turkish.SimpleSelectionMode=Basit kategoriler
turkish.ExpertSelectionMode=Uzman: tüm eklenti listesi
turkish.CategoryViewers=Dosyaları görüntüle
turkish.CategoryArchives=Arşivler ve biçimler
turkish.CategoryNetwork=Ağ ve cihazlar
turkish.CategorySystem=Sistem araçları
ukrainian.PluginSelectionMode=Режим вибору:
ukrainian.SimpleSelectionMode=Прості категорії
ukrainian.ExpertSelectionMode=Експерт: повний список плагінів
ukrainian.CategoryViewers=Перегляд файлів
ukrainian.CategoryArchives=Архіви та формати
ukrainian.CategoryNetwork=Мережа та пристрої
ukrainian.CategorySystem=Системні інструменти
vietnamese.PluginSelectionMode=Chế độ lựa chọn:
vietnamese.SimpleSelectionMode=Danh mục đơn giản
vietnamese.ExpertSelectionMode=Chuyên gia: danh sách plugin đầy đủ
vietnamese.CategoryViewers=Xem tệp
vietnamese.CategoryArchives=Lưu trữ và định dạng
vietnamese.CategoryNetwork=Mạng và thiết bị
vietnamese.CategorySystem=Công cụ hệ thống
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

catalan.RemoveUserConfigQuestion=Voleu eliminar la configuració d'usuari d'Open Samandarin?
catalan.AdditionalLanguages=Idiomes d'interfície addicionals (paquet d'idiomes)
catalan.LanguagesGroup=Idiomes:
catalan.FileStorage=Emmagatzematge de fitxers:
catalan.RegistryKey=Clau del registre:
catalan.RemoveUserConfigFiles=Trieu Sí per eliminar els fitxers de configuració, o No per conservar la configuració.
catalan.RemoveUserConfigRegistry=Trieu Sí per eliminar la clau i tot el seu contingut, o No per conservar la configuració.
catalan.InstallMode=Mode d'instal·lació:
catalan.StandardInstall=Instal·la normalment (Menú Inici, Programes i desinstal·lador)
catalan.PortableInstall=Extreu com una versió portàtil (sense entrada a Programes ni desinstal·lador)
catalan.PortableDirName=samandarin
catalan.StartMenuShortcut=Crea un accés directe al Menú &Inici
catalan.DesktopShortcut=Crea un accés directe a l'escriptori
catalan.Shortcuts=Accés directes:
catalan.LaunchProgram=Executa {#AppToInstallName}
catalan.CodePageWarningTitle=Avís de compatibilitat de la pàgina de codis
catalan.CodePageWarning=L'idioma seleccionat podria no mostrar tots els caràcters correctament perquè la pàgina de codis de la configuració regional del sistema Windows (%u) no coincideix amb la pàgina de codis esperada per aquest idioma (%u).\n\nPer solucionar-ho, canvieu la configuració regional del sistema a Windows: Configuració > Hora i idioma > Regió > Administració > Canviar la configuració regional del sistema.
catalan.KeepConfigQuestion=S'ha trobat un fitxer de configuració existent (configstorage.ini).\n\nEs renombrarà a configstorage.ini.BAK i s'instal·larà la configuració per defecte nova.
catalan.PluginSelectionTitle=Selecciona els connectors
catalan.PluginSelectionDescription=Triu els connectors englobats que voleu instal·lar o extreure.
catalan.PluginConfigurationLabel=Configuració de connectors:
catalan.PluginConfigurationBase=Base (només local, sense accés a Internet)
catalan.PluginConfigurationStandard=Estàndard (per a la majoria d'usuaris)
catalan.PluginConfigurationAdvanced=Avançat (per a usuaris avançats)
catalan.PluginConfigurationExpert=Expert (per a experts i desenvolupadors)
catalan.PluginConfigurationCustom=Personalitzat
catalan.PluginSelectionMode=Mode de selecció:
catalan.SimpleSelectionMode=Categories simples
catalan.ExpertSelectionMode=Expert: llista completa de connectors
catalan.CategoryViewers=Veure fitxers
catalan.CategoryArchives=Arxius i formats
catalan.CategoryNetwork=Xarxes i dispositius
catalan.CategorySystem=Eines del sistema
catalan.PluginRemovalTitle=Elimina els connectors instal·lats
catalan.PluginRemovalDescription=Selecciona els components a eliminar. Només s'eliminen els fitxers gestionats per l'instal·lador; la resta es mantenen.
catalan.PluginRemovalDependencyError=%s és necessari per un altre component seleccionat. Selecciona primer el component depenent.
catalan.RuntimeDependencyNotice=Alguns components seleccionats requereixen un entorn extern que no s'ha detectat. El component s'instal·larà de totes maneres; utilitzeu els enllaços oficials de sota.
catalan.OfficialRuntimeDownload=Descarrega oficial:
catalan.ManagedInventorySaveError=No s'ha pogut desar l'inventari de components gestionats. Les futures actualitzacions podrien deixar fitxers de components obsolets.
catalan.WebPackageUnavailable=%s no està disponible en l'instal·lador en línia perquè el seu paquet no té un resum SHA-256 publicat. Utilitzeu l'instal·lador fora de línia o torneu-ho a provar quan s'hagi actualitzat el catàleg de paquets.
catalan.WebInstallFailure=Els paquets seleccionats no s'han pogut descarregar, verificar ni preparar. Comproveu la connexió i torneu-ho a provar.
catalan.SalamatrixFrameworkRequired=Salamatrix Framework (obligatori)
catalan.SalamatrixFrameworkNotice=Salamatrix Framework és obligatori i s'inclourà sempre.
catalan.InstalledVersionLabel=instal·lat

danish.RemoveUserConfigQuestion=Vil du fjerne Open Samandarin-brugerkonfigurationen?
danish.AdditionalLanguages=Yderligere UI-sprog (sprogpakke)
danish.LanguagesGroup=Sprog:
danish.FileStorage=Filopbevaring:
danish.RegistryKey=Registreringsnøgle:
danish.RemoveUserConfigFiles=Vælg Ja for at slette konfigurationsfilerne, eller Nej for at bevare dine indstillinger.
danish.RemoveUserConfigRegistry=Vælg Ja for at slette nøglen inklusive alt indhold, eller Nej for at bevare dine indstillinger.
danish.InstallMode=Installationstilstand:
danish.StandardInstall=Installer normalt (Startmenu, Programmer og afinstalleringsprogram)
danish.PortableInstall=Ekstraher som en bærbar version (ingen Programs-post eller afinstalleringsprogram)
danish.PortableDirName=samandarin
danish.StartMenuShortcut=Opret en genvej til &Startmenuen
danish.DesktopShortcut=Opret en genvej på skrivebordet
danish.Shortcuts=Genveje:
danish.LaunchProgram=Start {#AppToInstallName}
danish.CodePageWarningTitle=Advarsel om kodepaginkompatibilitet
danish.CodePageWarning=Det valgte sprog viser muligvis ikke alle tegn korrekt, fordi din Windows-systemlocale-kodeside (%u) ikke matcher den kodeside, som dette sprog forventer (%u).\n\nFor at rette dette, ændr din systemlocale i Windows: Indstillinger > Tid og sprog > Region > Administrativ > Ændr systemlocale.
danish.KeepConfigQuestion=Der blev fundet en eksisterende konfigurationsfil (configstorage.ini).\n\nDen omdøbes til configstorage.ini.BAK, og den nye standardkonfiguration installeres.
danish.PluginSelectionTitle=Vælg plugins
danish.PluginSelectionDescription=Vælg hvilke medfølgende plugins der skal installeres eller ekstraheres.
danish.PluginConfigurationLabel=Plugin-konfiguration:
danish.PluginConfigurationBase=Base (kun lokal, ingen internetadgang)
danish.PluginConfigurationStandard=Standard (for de fleste brugere)
danish.PluginConfigurationAdvanced=Avanceret (for avancerede brugere)
danish.PluginConfigurationExpert=Ekspert (for eksperter og udviklere)
danish.PluginConfigurationCustom=Brugerdefineret
danish.PluginSelectionMode=Valgtilstand:
danish.SimpleSelectionMode=Enkle kategorier
danish.ExpertSelectionMode=Ekspert: fuld plugin-liste
danish.CategoryViewers=Vis filer
danish.CategoryArchives=Arkiver og formater
danish.CategoryNetwork=Netværk og enheder
danish.CategorySystem=Systemværktøjer
danish.PluginRemovalTitle=Fjern installerede komponenter
danish.PluginRemovalDescription=Vælg de komponenter, der skal fjernes. Kun installationsstyrerede filer fjernes; andre filer bevares.
danish.PluginRemovalDependencyError=%s kræves af en anden valgt komponent. Vælg den afhængige komponent til fjernelse først.
danish.RuntimeDependencyNotice=Nogle valgte komponenter kræver en ekstern kørselstilstand, som ikke blev registreret. Komponenten installeres alligevel; brug de officielle links nedenfor.
danish.OfficialRuntimeDownload=Officiel download:
danish.ManagedInventorySaveError=Komponentinventaret kunne ikke gemmes. Fremtidige opgraderinger kan efterlade forældede komponentfiler.
danish.WebPackageUnavailable=%s er ikke tilgængelig i den online installer, fordi dets pakke ikke har et offentligt SHA-256-sum. Brug den offline installer, eller prøv igen, når pakkekataloget er opdateret.
danish.WebInstallFailure=De valgte pakker kunne ikke downloades, verificeres eller forberedes. Tjek din forbindelse, og prøv igen.
danish.SalamatrixFrameworkRequired=Salamatrix Framework (påkrævet)
danish.SalamatrixFrameworkNotice=Salamatrix Framework er påkrævet og inkluderes altid.
danish.InstalledVersionLabel=installeret

finnish.RemoveUserConfigQuestion=Haluatko poistaa Open Samandarin käyttäjäasetukset?
finnish.AdditionalLanguages=Lisäliittymäkielet (kielipaketti)
finnish.LanguagesGroup=Kielet:
finnish.FileStorage=Tiedostojen tallennus:
finnish.RegistryKey=Rekisteriavain:
finnish.RemoveUserConfigFiles=Valitse Kyllä poistaaksesi asetustiedostot tai Ei säilyttääksesi asetukset.
finnish.RemoveUserConfigRegistry=Valitse Kyllä poistaaksesi avaimen ja kaiken sen sisällön tai Ei säilyttääksesi asetukset.
finnish.InstallMode=Asennustila:
finnish.StandardInstall=Asenna normaalisti (Käynnistä-valikko, Ohjelmat ja poisto-ohjelma)
finnish.PortableInstall=Pura kannettavana versiona (ei Ohjelmat-valikkotulosta tai poisto-ohjelmaa)
finnish.PortableDirName=samandarin
finnish.StartMenuShortcut=Luo pikanäppäin &Käynnistä-valikkoon
finnish.DesktopShortcut=Luo pikanäppäin työpöydälle
finnish.Shortcuts=Pikanäppäimet:
finnish.LaunchProgram=Käynnistä {#AppToInstallName}
finnish.CodePageWarningTitle=Varoitus merkistösivun yhteensopivuudesta
finnish.CodePageWarning=Valittu kieli ei välttämättä näytä kaikkia merkkejä oikein, koska Windows-järjestelmän paikalliskoodisivu (%u) ei vastaa tämän kielen odottamaa koodisivua (%u).\n\nKorjaa tämä muuttamalla järjestelmän paikalliskoodi Windowsissa: Asetukset > Aika ja kieli > Alue > Hallinto > Muuta järjestelmän paikalliskoodia.
finnish.KeepConfigQuestion=Olemassa oleva asetustiedosto (configstorage.ini) löydettiin.\n\nSe nimetään uudelleen tiedostoksi configstorage.ini.BAK ja uusi oletusasennus asennetaan.
finnish.PluginSelectionTitle=Valitse lisäosat
finnish.PluginSelectionDescription=Valitse mukana tulevat lisäosat, jotka asennetaan tai puretaan.
finnish.PluginConfigurationLabel=Lisäosien asetus:
finnish.PluginConfigurationBase=Perus (vain paikallinen, ei internetyhteyttä)
finnish.PluginConfigurationStandard=Vakio (useimmille käyttäjille)
finnish.PluginConfigurationAdvanced=Lisäasetukset (edistyneille käyttäjille)
finnish.PluginConfigurationExpert=Asiantuntija (asiantuntijoille ja kehittäjille)
finnish.PluginConfigurationCustom=Mukautettu
finnish.PluginSelectionMode=Valintatila:
finnish.SimpleSelectionMode=Yksinkertaiset kategoriat
finnish.ExpertSelectionMode=Asiantuntija: täysi lisäosaluettelo
finnish.CategoryViewers=Tiedostojen katselu
finnish.CategoryArchives=Arkistot ja formaatit
finnish.CategoryNetwork=Verkko ja laitteet
finnish.CategorySystem=Järjestelmätyökalut
finnish.PluginRemovalTitle=Poista asennetut komponentit
finnish.PluginRemovalDescription=Valitse poistettavat komponentit. Vain asennuksen hallinnoimat tiedostot poistetaan; muut tiedostot säilytetään.
finnish.PluginRemovalDependencyError=%s vaaditaan toisen valitun komponentin toimesta. Valitse riippuva komponentti poistettavaksi ensin.
finnish.RuntimeDependencyNotice=Joitakin valittuja komponentteja vaaditaan ulkoinen suoritusaika, jota ei havaittu. Komponentti asennetaan silti; käytä alla olevia virallisia linkkejä.
finnish.OfficialRuntimeDownload=Virallinen lataus:
finnish.ManagedInventorySaveError=Hallinnoitu komponenttiluettelo ei voitu tallentaa. Tulevat päivitykset voivat jättää vanhentuneita komponenttitiedostoja.
finnish.WebPackageUnavailable=%s ei ole käytettävissä verkkopohjaisessa asentimessa, koska sen paketilla ei ole julkista SHA-256-tunnistetta. Käytä offline-asentinta tai yritä uudelleen, kun paketiluettelo on päivitetty.
finnish.WebInstallFailure=Valittuja paketteja ei voitu ladata, tarkistaa tai valmistaa. Tarkista yhteytesi ja yritä uudelleen.
finnish.SalamatrixFrameworkRequired=Salamatrix Framework (pakollinen)
finnish.SalamatrixFrameworkNotice=Salamatrix Framework on pakollinen ja se sisällytetään aina.
finnish.InstalledVersionLabel=asennettu

lithuanian.RemoveUserConfigQuestion=Ar norite pašalinti Open Samandarin vartotojų konfigūraciją?
lithuanian.AdditionalLanguages=Papildomos kalbos (kalbų paketas)
lithuanian.LanguagesGroup=Kalbos:
lithuanian.FileStorage=Failų saugojimas:
lithuanian.RegistryKey=Registracijos raktas:
lithuanian.RemoveUserConfigFiles=Pasirinkite Taip, kad ištrintumėte konfigūracijos failus, arba Ne, kad išlaikytumėte nustatymus.
lithuanian.RemoveUserConfigRegistry=Pasirinkite Taip, kad ištrintumėte raktą su visu turiniu, arba Ne, kad išlaikytumėte nustatymus.
lithuanian.InstallMode=Įdiegimo būdas:
lithuanian.StandardInstall=Įdiekite įprastai (Priešingo meniu, Programos ir išnaikinimo programa)
lithuanian.PortableInstall=Išpakavę kaip nuoseklią versiją (be Programos įrašo ar išnaikinimo programos)
lithuanian.PortableDirName=samandarin
lithuanian.StartMenuShortcut=Sukurkite nuspaudimo klavišą Pradžios &meniu
lithuanian.DesktopShortcut=Sukurkite nuspaudimo klavišą darbastaliui
lithuanian.Shortcuts=Nuspaudimo klavišai:
lithuanian.LaunchProgram=Paleiskite {#AppToInstallName}
lithuanian.CodePageWarningTitle=Kodo puslapio susijungimo įspėjimas
lithuanian.CodePageWarning=Pasirinkta kalba gali neatspėti visų simbolių, nes Windows sistemos lokales kodas (%u) nesutampa su šios kalbos reikiamu kodu (%u).\n\nPakeiskite sistemos lokalę Windows: Asetukset > Vykdymo ir kalbos > Regionas > Administravimas > Pakeisti sistemos lokalę.
lithuanian.KeepConfigQuestion=Rastas egzistuojantis konfigūracijos failas (configstorage.ini).\n\nJis bus pervadintas į configstorage.ini.BAK ir įdiegtas naujas standartinis konfigūracija.
lithuanian.PluginSelectionTitle=Pasirinkite įskiepius
lithuanian.PluginSelectionDescription=Pasirinkite įskiepius, kuriuos norite įdiegti arba išduoti.
lithuanian.PluginConfigurationLabel=Įskiepių konfigūracija:
lithuanian.PluginConfigurationBase=Pagrindinis (tik vietinėje, be interneto prieigos)
lithuanian.PluginConfigurationStandard=Standartinis (daugumai vartotojų)
lithuanian.PluginConfigurationAdvanced=Papildomas (pateiktas vartotojams)
lithuanian.PluginConfigurationExpert=Ekspertas (ekspertams ir kūrėjams)
lithuanian.PluginConfigurationCustom=Pratintintas
lithuanian.PluginSelectionMode=Pasirinkimo būdas:
lithuanian.SimpleSelectionMode=Paprastos kategorijos
lithuanian.ExpertSelectionMode=Ekspertas: visų įskiepių sąrašas
lithuanian.CategoryViewers=Peržiūrėti failus
lithuanian.CategoryArchives=Archyvai ir formatai
lithuanian.CategoryNetwork=Tinklai ir įrenginiai
lithuanian.CategorySystem=Sistemos įrankiai
lithuanian.PluginRemovalTitle=Pašalinti įdiegutes komponentus
lithuanian.PluginRemovalDescription=Pasirinkite pašalintinus komponentus. Pašalintini tik diegimo vadovas tvarkomas failai; kiti failai lieka.
lithuanian.PluginRemovalDependencyError=%s reikalingas kitam pasirinktam komponentui. Pasirinkite priklausomą komponentą pašalintinui pirmiausia.
lithuanian.RuntimeDependencyNotice=Nuklyti pasirinktų komponentų reikalauja išorinis vykdymo dievas, kuris nebuvo aptiktas. Komponentas įdiegsimas vis tiek; naudokite oficialius nuorodas žemiau.
lithuanian.OfficialRuntimeDownload=Oficiali atsisiuntimo nuoroda:
lithuanian.ManagedInventorySaveError=Valdomas komponentų sąrašas negalbės būti išsaugotas. Ateitinai atnaujinimas gali palikti nebūtinus komponentų failus.
lithuanian.WebPackageUnavailable=%s nepasiekiamas online įdiegiame, nes jo pakete nėra viešų SHA-256 rakstymo. Naudokite offline įdiegiame arba bandykite vėl, kai paketų katalogas bus atnaujintas.
lithuanian.WebInstallFailure=Pasirinktos paketų negalbėtų būti atsisiunčiamos, tikrinamos arba ruoštos. Patikrinkite ryšį ir bandykite vėl.
lithuanian.SalamatrixFrameworkRequired=Salamatrix Framework (privaloma)
lithuanian.SalamatrixFrameworkNotice=Salamatrix Framework yra privaloma ir visada išlaikomas.
lithuanian.InstalledVersionLabel=Įdiegtas

norwegian.RemoveUserConfigQuestion=Vil du fjerne Open Samandarin-brukerkonfigurasjonen?
norwegian.AdditionalLanguages=Ytterligere UI-språk (språkpakke)
norwegian.LanguagesGroup=Språk:
norwegian.FileStorage=Filagring:
norwegian.RegistryKey=Registreringsnøkkel:
norwegian.RemoveUserConfigFiles=Velg Ja for å slette konfigurasjonsfilene, eller Nei for å beholde innstillingene dine.
norwegian.RemoveUserConfigRegistry=Velg Ja for å slette nøkkelen inkludert alt innhold, eller Nei for å beholde innstillingene dine.
norwegian.InstallMode=Installasjonsmodus:
norwegian.StandardInstall=Installer normalt (Startmeny, Programmer og avinstalleringsprogram)
norwegian.PortableInstall=Pakke ut som bærbar versjon (ingen Programmer-post eller avinstalleringsprogram)
norwegian.PortableDirName=samandarin
norwegian.StartMenuShortcut=Opprett en &Startmeny-gjenveien
norwegian.DesktopShortcut=Opprett en skrivebordsgjenveien
norwegian.Shortcuts=Gjenveier:
norwegian.LaunchProgram=Start {#AppToInstallName}
norwegian.CodePageWarningTitle=Advarsel om kodepadekompatibilitet
norwegian.CodePageWarning=Det valgte språket viser kanskje ikke alle tegn korrekt, fordi Windows-systemlokales kodepage (%u) ikke samsvarer med den kodepage som dette språket forventer (%u).\n\nFor å rette dette, endre systemlokale i Windows: Innstillinger > Tid og språk > Region > Administrativ > Endre systemlokale.
norwegian.KeepConfigQuestion=En eksisterende konfigurasjonsfil (configstorage.ini) ble funnet.\n\nDen blir omdøpt til configstorage.ini.BAK, og den nye standardkonfigurasjonen installeres.
norwegian.PluginSelectionTitle=Velg plugins
norwegian.PluginSelectionDescription=Velg hvilke inkluderte plugins som skal installeres eller pakkes ut.
norwegian.PluginConfigurationLabel=Plugin-konfigurasjon:
norwegian.PluginConfigurationBase=Grunn (kun lokal, ingen internettadgang)
norwegian.PluginConfigurationStandard=Standard (for de fleste brukere)
norwegian.PluginConfigurationAdvanced=Avansert (for avanserte brukere)
norwegian.PluginConfigurationExpert=Ekspert (for eksperter og utviklere)
norwegian.PluginConfigurationCustom=Tilpasset
norwegian.PluginSelectionMode=Velgmodus:
norwegian.SimpleSelectionMode=Enkle kategorier
norwegian.ExpertSelectionMode=Ekspert: full plugin-liste
norwegian.CategoryViewers=Vis filer
norwegian.CategoryArchives=Arkiv og formater
norwegian.CategoryNetwork=Nettverk og enheter
norwegian.CategorySystem=Systemverktøy
norwegian.PluginRemovalTitle=Fjern installerte komponenter
norwegian.PluginRemovalDescription=Velg komponenter som skal fjernes. Bare installasjonsstyrte filer fjernes; andre filer beholdes.
norwegian.PluginRemovalDependencyError=%s kreves av en annen valgt komponent. Velg den avhengige komponenten til fjernelse først.
norwegian.RuntimeDependencyNotice=Nogle valgte komponenter krever en ekstern kjøretid som ikke ble oppdaget. Komponenten installeres likevel; bruk de offisielle lenkene nedenfor.
norwegian.OfficialRuntimeDownload=Offisiell nedlasting:
norwegian.ManagedInventorySaveError=Komponentinventaret kunne ikke lagres. Fremtidige oppgraderinger kan etterlate utdaterte komponentfiler.
norwegian.WebPackageUnavailable=%s er ikke tilgjengelig i nettbaserte installatoren, fordi pakken hans ikke har en offentlig SHA-256-digest. Bruk offline-installatoren eller prøv igjen når pakkekatalogen er oppdatert.
norwegian.WebInstallFailure=De valgte pakkene kunne ikke lastes ned, verifiseres eller forberedes. Sjekk tilkoblingen din og prøv igjen.
norwegian.SalamatrixFrameworkRequired=Salamatrix Framework (påkrevd)
norwegian.SalamatrixFrameworkNotice=Salamatrix Framework er påkrevd og inkluderes alltid.
norwegian.InstalledVersionLabel=installert

slovenian.RemoveUserConfigQuestion=Ali želite odstraniti uporabniško konfiguracijo Open Samandarina?
slovenian.AdditionalLanguages=Dodatni jeziki uporabniškega vmesnika (jezični paket)
slovenian.LanguagesGroup=Jeziki:
slovenian.FileStorage=Shranjevanje datotek:
slovenian.RegistryKey=Ključ registra:
slovenian.RemoveUserConfigFiles=Izberite Da za brisanje datotek konfiguracije, ali Ne za ohranjanje nastavitev.
slovenian.RemoveUserConfigRegistry=Izberite Da za brisanje ključa vključno z vsebino, ali Ne za ohranjanje nastavitev.
slovenian.InstallMode=Način namestitve:
slovenian.StandardInstall=Namestite navadno (Izberite meni, Programi in deinstaller)
slovenian.PortableInstall=Razpakirajte kot prenosljivo različico (brez vnosa v Programi ali deinstaller)
slovenian.PortableDirName=samandarin
slovenian.StartMenuShortcut=Ustvarite &Izberite meni prekratko
slovenian.DesktopShortcut=Ustvarite nizko na namizju
slovenian.Shortcuts=Prekratke:
slovenian.LaunchProgram=Zagon {#AppToInstallName}
slovenian.CodePageWarningTitle=Opozorilo o združljivosti strani kode
slovenian.CodePageWarning=Izbran jezik morda ne bo prikazal vseh znakov pravilno, ker Windows sistemska lokalna stran kode (%u) se ne ujema z stranjo kode, ki jo pričakuje ta jezik (%u).\n\nZa popravilo, spremenite sistemske lokalne v Windows: Nastavitve > Čas in jezik > Regija > Administrativno > Spremeni sistemske lokalne.
slovenian.KeepConfigQuestion=Obstajajoča datoteka konfiguracije (configstorage.ini) je bila najdena.\n\nBojebno preimenovana v configstorage.ini.BAK in namestitev nove privzete konfiguracije.
slovenian.PluginSelectionTitle=Izbira vtičnikov
slovenian.PluginSelectionDescription=Izberite vključene vtičnike, ki jih želite namestiti ali razpakirati.
slovenian.PluginConfigurationLabel=Konfiguracija vtičnikov:
slovenian.PluginConfigurationBase=Osnova (samo lokalno, brez dostopa do interneta)
slovenian.PluginConfigurationStandard=Standardno (za večino uporabnikov)
slovenian.PluginConfigurationAdvanced=Napredno (za napredne uporabnike)
slovenian.PluginConfigurationExpert=Ekspertno (za eksperte in razvijalce)
slovenian.PluginConfigurationCustom=Prilagojeno
slovenian.PluginSelectionMode=Način izbire:
slovenian.SimpleSelectionMode=Enostavne kategorije
slovenian.ExpertSelectionMode=Ekspertno: celotni seznam vtičnikov
slovenian.CategoryViewers=Poglej datoteke
slovenian.CategoryArchives=Arhivi in formati
slovenian.CategoryNetwork=Omrežja in naprave
slovenian.CategorySystem=Sistemski orodja
slovenian.PluginRemovalTitle=Odstranite nameščene komponente
slovenian.PluginRemovalDescription=Izberite komponente za odstranitev. Odstranjene bodo le datoteke, ki jih upravlja namestitev; druge datoteke se ohranijo.
slovenian.PluginRemovalDependencyError=%s je zahtevan za drugo izbrano komponento. Izberite odvisno komponento za odstranitev najprej.
slovenian.RuntimeDependencyNotice=Nekatere izbrane komponente zahtevajo zunanjo vrednost, ki ni bila zaznana. Komponenta bo nameščena vseeno; uporabljajte officielle povezave spodaj.
slovenian.OfficialRuntimeDownload=Uradesna prenos:
slovenian.ManagedInventorySaveError=Upravljani inventar komponent ni mogoče shraniti. Prihodnje posodobitve lahko ostavijo zastarele datoteke komponent.
slovenian.WebPackageUnavailable=%s ni na voljo v namestitveniku na spletu, ker njegov paket nima objavljenega SHA-256 stržka. Uporabljajte offline-namestitvenik ali poskusite znova, ko je paketni katalog posodobljen.
slovenian.WebInstallFailure=Izbrani paketi ni mogoče prenesati, preveriti ali pripraviti. Preverite povezavo in poskusite znova.
slovenian.SalamatrixFrameworkRequired=Salamatrix Framework (obvezno)
slovenian.SalamatrixFrameworkNotice=Salamatrix Framework je obvezno in bo vedno vključeno.
slovenian.InstalledVersionLabel=nameščeno



[Tasks]
Name: "startmenuicon"; Description: "{cm:StartMenuShortcut}"; GroupDescription: "{cm:Shortcuts}"; Check: not IsPortableInstall
Name: "desktopicon"; Description: "{cm:DesktopShortcut}"; GroupDescription: "{cm:Shortcuts}"; Flags: unchecked; Check: not IsPortableInstall
Name: "additionallanguages"; Description: "{cm:AdditionalLanguages}"; GroupDescription: "{cm:LanguagesGroup}"; Flags: checkedonce

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
Name: "{app}\extensions\json-viewer-nodejs"
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
Type: files; Name: "{autodesktop}\Open Salamander Samandarin (x64).lnk"
Type: files; Name: "{autoprograms}\Open Salamander Samandarin\Open Salamander Samandarin (x64).lnk"
Type: dirifempty; Name: "{autoprograms}\Open Salamander Samandarin"
Type: files; Name: "{app}\MarkdigRenderer.exe"
Type: files; Name: "{app}\WebView2Loader.dll"
Type: files; Name: "{app}\plugins\salamatrix\WebView2Loader.dll"
Type: files; Name: "{app}\plugins\salamatrix\MarkdigRenderer.exe"
Type: files; Name: "{app}\plugins\textviewer\WebView2Loader.dll"
Type: filesandordirs; Name: "{app}\plugins\textviewer\prism"
Type: files; Name: "{app}\plugins\webview2renderviewer\WebView2Loader.dll"
Type: files; Name: "{app}\plugins\webview2renderviewer\MarkdigRenderer.exe"

[UninstallDelete]
Type: files; Name: "{app}\managed-plugin-files.txt"

[Files]
#if WebInstaller == False
Source: "{#PayloadDir}\managed-plugin-files.txt"; DestDir: "{tmp}"; Flags: dontcopy
#endif
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
Source: "{#PayloadDir}\lang\langpack.dll"; DestDir: "{app}\lang"; Flags: ignoreversion skipifsourcedoesntexist; Tasks: additionallanguages
Source: "{#PayloadDir}\msvcp140.dll"; DestDir: "{app}"; Flags: ignoreversion
#if WebInstaller == False
Source: "{#PayloadDir}\plugins\7zip\7za.dll"; DestDir: "{app}\plugins\7zip"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\7zip.spl"; DestDir: "{app}\plugins\7zip"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\7zip\7zwrapper.dll"; DestDir: "{app}\plugins\7zip"; Flags: ignoreversion; Check: IsPluginSelected('7zip')
Source: "{#PayloadDir}\plugins\automation\automation.spl"; DestDir: "{app}\plugins\automation"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Convert Images.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Count Lines.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Launch Elevated Command Prompt.vbs"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Make Link.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Make List (JScript).js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Make List (VBScript).vbs"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Unpack Multiple Archives.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion; Check: IsPluginSelected('automation')
Source: "{#PayloadDir}\plugins\automation\scripts\Salamatrix Progress Demo\*"; DestDir: "{app}\plugins\automation\scripts\Salamatrix Progress Demo"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('salamatrixdemos')
Source: "{#PayloadDir}\plugins\dbviewer\dbviewer.spl"; DestDir: "{app}\plugins\dbviewer"; Flags: ignoreversion; Check: IsPluginSelected('dbviewer')
Source: "{#PayloadDir}\plugins\diskdir\diskdir.spl"; DestDir: "{app}\plugins\diskdir"; Flags: ignoreversion; Check: IsPluginSelected('diskdir')
Source: "{#PayloadDir}\plugins\diskmap\diskmap.spl"; DestDir: "{app}\plugins\diskmap"; Flags: ignoreversion; Check: IsPluginSelected('diskmap')
Source: "{#PayloadDir}\plugins\filecomp\fcremote.exe"; DestDir: "{app}\plugins\filecomp"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\filecomp\filecomp.spl"; DestDir: "{app}\plugins\filecomp"; Flags: ignoreversion; Check: IsPluginSelected('filecomp')
Source: "{#PayloadDir}\plugins\folders\folders.spl"; DestDir: "{app}\plugins\folders"; Flags: ignoreversion; Check: IsPluginSelected('folders')
Source: "{#PayloadDir}\plugins\ftp\ftp.spl"; DestDir: "{app}\plugins\ftp"; Flags: ignoreversion; Check: IsPluginSelected('ftp')
Source: "{#PayloadDir}\plugins\sftp\libcrypto-3-x64.dll"; DestDir: "{app}\plugins\sftp"; Flags: ignoreversion; Check: IsPluginSelected('sftp')
Source: "{#PayloadDir}\plugins\sftp\libssh2.dll"; DestDir: "{app}\plugins\sftp"; Flags: ignoreversion; Check: IsPluginSelected('sftp')
Source: "{#PayloadDir}\plugins\sftp\sftp.spl"; DestDir: "{app}\plugins\sftp"; Flags: ignoreversion; Check: IsPluginSelected('sftp')
Source: "{#PayloadDir}\plugins\sftp\z.dll"; DestDir: "{app}\plugins\sftp"; Flags: ignoreversion; Check: IsPluginSelected('sftp')
Source: "{#PayloadDir}\plugins\hypervm\HyperVM.Managed.dll"; DestDir: "{app}\plugins\hypervm"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\hypervm\hypervm.spl"; DestDir: "{app}\plugins\hypervm"; Flags: ignoreversion; Check: IsPluginSelected('hypervm')
Source: "{#PayloadDir}\plugins\checksum\checksum.spl"; DestDir: "{app}\plugins\checksum"; Flags: ignoreversion; Check: IsPluginSelected('checksum')
Source: "{#PayloadDir}\plugins\ieviewer\css\githubmd.css"; DestDir: "{app}\plugins\ieviewer\css"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\ieviewer\ieviewer.spl"; DestDir: "{app}\plugins\ieviewer"; Flags: ignoreversion; Check: IsPluginSelected('ieviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\DEPENDENCIES.md"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\JsonViewer.Managed.dll"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\jsonviewer.spl"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\Newtonsoft.Json.dll"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\OpenSalamander.ManagedBootstrap.dll"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\jsonviewer\SUPPORTED_FILE_TYPES.md"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion; Check: IsPluginSelected('jsonviewer')
Source: "{#PayloadDir}\plugins\mmviewer\mmviewer.spl"; DestDir: "{app}\plugins\mmviewer"; Flags: ignoreversion; Check: IsPluginSelected('mmviewer')
Source: "{#PayloadDir}\plugins\nethood\nethood.spl"; DestDir: "{app}\plugins\nethood"; Flags: ignoreversion; Check: IsPluginSelected('nethood')
Source: "{#PayloadDir}\plugins\pak\pak.spl"; DestDir: "{app}\plugins\pak"; Flags: ignoreversion; Check: IsPluginSelected('pak')
Source: "{#PayloadDir}\plugins\peviewer\peviewer.spl"; DestDir: "{app}\plugins\peviewer"; Flags: ignoreversion; Check: IsPluginSelected('peviewer')
Source: "{#PayloadDir}\plugins\pictview\exif.dll"; DestDir: "{app}\plugins\pictview"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\pictview-previewhost.exe"; DestDir: "{app}\plugins\pictview"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\pictview\pictview.spl"; DestDir: "{app}\plugins\pictview"; Flags: ignoreversion; Check: IsPluginSelected('pictview')
Source: "{#PayloadDir}\plugins\portables\portables.spl"; DestDir: "{app}\plugins\portables"; Flags: ignoreversion; Check: IsPluginSelected('portables')
Source: "{#PayloadDir}\plugins\regedt\regedt.spl"; DestDir: "{app}\plugins\regedt"; Flags: ignoreversion; Check: IsPluginSelected('regedt')
Source: "{#PayloadDir}\plugins\renamer\renamer.spl"; DestDir: "{app}\plugins\renamer"; Flags: ignoreversion; Check: IsPluginSelected('renamer')
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
Source: "{#PayloadDir}\plugins\serviceexplorer\serviceexplorer.spl"; DestDir: "{app}\plugins\serviceexplorer"; Flags: ignoreversion; Check: IsPluginSelected('serviceexplorer')
Source: "{#PayloadDir}\plugins\splitcbn\splitcbn.spl"; DestDir: "{app}\plugins\splitcbn"; Flags: ignoreversion; Check: IsPluginSelected('splitcbn')
Source: "{#PayloadDir}\plugins\tar\tar.spl"; DestDir: "{app}\plugins\tar"; Flags: ignoreversion; Check: IsPluginSelected('tar')
Source: "{#PayloadDir}\plugins\textviewer\DEPENDENCIES.md"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\SUPPORTED_FILE_TYPES.md"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\textviewer\textviewer.spl"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion; Check: IsPluginSelected('textviewer')
Source: "{#PayloadDir}\plugins\unarj\unarj.spl"; DestDir: "{app}\plugins\unarj"; Flags: ignoreversion; Check: IsPluginSelected('unarj')
Source: "{#PayloadDir}\plugins\uncab\uncab.spl"; DestDir: "{app}\plugins\uncab"; Flags: ignoreversion; Check: IsPluginSelected('uncab')
Source: "{#PayloadDir}\plugins\undelete\undelete.spl"; DestDir: "{app}\plugins\undelete"; Flags: ignoreversion; Check: IsPluginSelected('undelete')
Source: "{#PayloadDir}\plugins\unfat\unfat.spl"; DestDir: "{app}\plugins\unfat"; Flags: ignoreversion; Check: IsPluginSelected('unfat')
Source: "{#PayloadDir}\plugins\unchm\chmlib.dll"; DestDir: "{app}\plugins\unchm"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\unchm\unchm.spl"; DestDir: "{app}\plugins\unchm"; Flags: ignoreversion; Check: IsPluginSelected('unchm')
Source: "{#PayloadDir}\plugins\uniso\uniso.spl"; DestDir: "{app}\plugins\uniso"; Flags: ignoreversion; Check: IsPluginSelected('uniso')
Source: "{#PayloadDir}\plugins\unlha\unlha.spl"; DestDir: "{app}\plugins\unlha"; Flags: ignoreversion; Check: IsPluginSelected('unlha')
Source: "{#PayloadDir}\plugins\unmime\unmime.spl"; DestDir: "{app}\plugins\unmime"; Flags: ignoreversion; Check: IsPluginSelected('unmime')
Source: "{#PayloadDir}\plugins\unole\unole.spl"; DestDir: "{app}\plugins\unole"; Flags: ignoreversion; Check: IsPluginSelected('unole')
Source: "{#PayloadDir}\plugins\unrar\unrar.dll"; DestDir: "{app}\plugins\unrar"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\unrar\unrar.spl"; DestDir: "{app}\plugins\unrar"; Flags: ignoreversion; Check: IsPluginSelected('unrar')
Source: "{#PayloadDir}\plugins\webview2renderviewer\DEPENDENCIES.md"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\SUPPORTED_FILE_TYPES.md"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\webview2renderviewer\webview2renderviewer.spl"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion; Check: IsPluginSelected('webview2renderviewer')
Source: "{#PayloadDir}\plugins\wmobile\wmobile.spl"; DestDir: "{app}\plugins\wmobile"; Flags: ignoreversion; Check: IsPluginSelected('wmobile')
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
Source: "{#PayloadDir}\extensions\demos\*"; DestDir: "{app}\extensions\demos"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('salamatrixdemos')
Source: "{#PayloadDir}\extensions\extension-menu-builder\*"; DestDir: "{app}\extensions\extension-menu-builder"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('extensionmenubuilder')
Source: "{#PayloadDir}\extensions\git-worktree-navigator\*"; DestDir: "{app}\extensions\git-worktree-navigator"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('gitworktreenavigator')
Source: "{#PayloadDir}\extensions\file-lock-inspector\*"; DestDir: "{app}\extensions\file-lock-inspector"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('filelockinspector')
Source: "{#PayloadDir}\extensions\process-explorer\*"; DestDir: "{app}\extensions\process-explorer"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('processexplorer')
Source: "{#PayloadDir}\extensions\hardware-monitor\*"; DestDir: "{app}\extensions\hardware-monitor"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('hardwaremonitor')
Source: "{#PayloadDir}\extensions\event-viewer\*"; DestDir: "{app}\extensions\event-viewer"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('eventviewer')
Source: "{#PayloadDir}\extensions\json-viewer-nodejs\*"; DestDir: "{app}\extensions\json-viewer-nodejs"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsPluginSelected('jsonwebviewer')
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
#endif
Source: "{#PayloadDir}\plugins\plugins.ver"; DestDir: "{app}\plugins"; Flags: ignoreversion
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
Source: "{#PayloadDir}\toolbars\darkmode\GoToNextSelectedName.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\GoToPathfromOtherPanel.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\GoToPreviousSelectedName.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\GoToShortcutTarget.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\HelpContents.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\HideSelectedNames.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\HideUnselectedNames.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\InvertSelection.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\RestoreSelection.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Select.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\SelectAll.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\SelectFilesWithSameExtension.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\SelectFilesWithSameName.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Unselect.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\UnselectAll.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\UnselectFilesWithSameExtension.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\UnselectFilesWithSameName.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
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
Source: "{#PayloadDir}\toolbars\darkmode\LoadSelection.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\Salamand.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\darkmode\SaveSelection.svg"; DestDir: "{app}\toolbars\darkmode"; Flags: ignoreversion
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
Name: "{autodesktop}\Open Samandarin (x64)"; Filename: "{app}\{#AppToInstallExeName}"; WorkingDir: "{app}"; Check: ShouldCreateShortcut('desktop')
Name: "{group}\Open Samandarin (x64)"; Filename: "{app}\{#AppToInstallExeName}"; WorkingDir: "{app}"; Check: ShouldCreateShortcut('startmenu')

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
  TWebPackage = record
    InstallerId: String;
    PackageId: String;
    PackageType: String;
    PackageVersion: String;
    PackageUrl: String;
    PackageSha256: String;
    InstallRelativePath: String;
    ArchivePath: String;
    UnpackPath: String;
  end;

var
  InstallModePage: TInputOptionWizardPage;
  PluginSelectionPage: TWizardPage;
  PluginRemovalPage: TWizardPage;
  PluginList: TNewCheckListBox;
  PluginRemovalList: TNewCheckListBox;
  PluginConfigurationLabel: TLabel;
  PluginConfigurationList: TNewComboBox;
  RuntimeWarningLabel: TLabel;
  RuntimeLinkLabels: array[0..3] of TLabel;
  RuntimeLinkUrls: array[0..3] of String;
  ApplyingPluginConfiguration: Boolean;
  UpgradeInstall: Boolean;
  PluginListPopulated: Boolean;
  PluginIds: array of String;
  RemovePluginIds: array of String;
  PluginConfigurations: array of String;
  PluginDependencies: array of TPluginDependency;
  WebPackages: array of TWebPackage;
  DownloadPage: TDownloadWizardPage;
  ExtractionPage: TExtractionWizardPage;
  WebStageRoot: String;
  WebPreparationError: String;
  PreviousManagedFiles: TArrayOfString;
  InstallerManagedFiles: TArrayOfString;
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



function GetFileAttributesW(const FileName: String): Cardinal;
external 'GetFileAttributesW@kernel32.dll stdcall setuponly';

function SetFileAttributesW(const FileName: String; Attributes: Cardinal): Boolean;
external 'SetFileAttributesW@kernel32.dll stdcall setuponly';


function IsPortableInstall(): Boolean; forward;
function IsPluginMarkedForRemoval(const PluginId: String): Boolean; forward;
procedure SelectAllPluginDependencies; forward;
procedure PopulatePluginRemovalList; forward;
procedure ApplyManagedFileInventory; forward;
procedure UpdateRuntimeNotice; forward;
procedure PrepareWebInstallerPackages; forward;

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
  for Index := 0 to GetArrayLength(PluginDependencies) - 1 do
    if (CompareText(PluginDependencies[Index].PluginId, PluginId) = 0) and
       (CompareText(PluginDependencies[Index].DependencyId, DependencyId) = 0) then
      Exit;
  Index := GetArrayLength(PluginDependencies);
  SetArrayLength(PluginDependencies, Index + 1);
  PluginDependencies[Index].PluginId := PluginId;
  PluginDependencies[Index].DependencyId := DependencyId;
end;

#include "inno_package_catalog.issinc"

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
  AddPluginDependency('jsonwebviewer', 'javascriptruntime');

  { The demo package contains one Automation extension and five runtime demos. }
  AddPluginDependency('salamatrixdemos', 'automation');
  AddPluginDependency('salamatrixdemos', 'javascriptruntime');
  AddPluginDependency('salamatrixdemos', 'luaruntime');
  AddPluginDependency('salamatrixdemos', 'phpruntime');
  AddPluginDependency('salamatrixdemos', 'powershellruntime');
  AddPluginDependency('salamatrixdemos', 'pythonruntime');

  { Catalog metadata supplements installer-specific package dependencies. }
  AddGeneratedInstallerPackageDependencies;
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
      if CompareText(PluginIds[I], 'salamatrix') = 0 then
        PluginList.Checked[I] := True
      else
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
  UpdateRuntimeNotice;
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
  else if CompareText(PluginId, 'jsonwebviewer') = 0 then ExpectedPath := 'extensions/json-viewer-nodejs/'
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
     (CompareText(PluginId, 'jsonwebviewer') = 0) or
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
  PluginDisplayName: String;
  DisplayVersion: String;
#if WebInstaller
  PackageId: String;
  PackageType: String;
  PackageVersion: String;
  PackageUrl: String;
  PackageSha256: String;
  InstallRelativePath: String;
#endif
  Checked: Boolean;
  RequiredPlugin: Boolean;
  ItemIndex: Integer;
begin
  RequiredPlugin := CompareText(PluginId, 'salamatrix') = 0;
  PluginDisplayName := DisplayName;
  if RequiredPlugin then
    PluginDisplayName := CustomMessage('SalamatrixFrameworkRequired');
  DisplayVersion := Version;
#if WebInstaller
  if GetInstallerWebPackage(
       PluginId, PackageId, PackageType, PackageVersion, PackageUrl, PackageSha256, InstallRelativePath) then
    DisplayVersion := PackageVersion;
#endif
  InstalledVersion := GetInstalledPluginVersion(PluginId);
  if InstalledVersion <> '' then
    UpgradeInstall := True;
  { Keep installed plugins selected during upgrades; new plugins follow Standard. }
  Checked := RequiredPlugin or ListContainsToken(Configurations, 'standard') or (InstalledVersion <> '');
  ItemIndex := PluginList.AddCheckBox(
    PluginDisplayName,
    FormatPluginVersionText(DisplayVersion, InstalledVersion),
    0,
    Checked,
    not RequiredPlugin,
    False,
    False,
    nil);
  if IsInstallerPluginVersionNewer(DisplayVersion, InstalledVersion) then
    PluginList.SubItemFontStyle[ItemIndex] := [fsBold];
  SetArrayLength(PluginIds, GetArrayLength(PluginIds) + 1);
  PluginIds[GetArrayLength(PluginIds) - 1] := PluginId;
  SetArrayLength(PluginConfigurations, GetArrayLength(PluginConfigurations) + 1);
  PluginConfigurations[GetArrayLength(PluginConfigurations) - 1] := Configurations;
end;

function HasManagedFilesForPlugin(const PluginId: String): Boolean;
var
  I: Integer;
  SeparatorIndex: Integer;
begin
  Result := False;
  for I := 0 to GetArrayLength(PreviousManagedFiles) - 1 do
  begin
    SeparatorIndex := Pos('|', PreviousManagedFiles[I]);
    if (SeparatorIndex > 1) and
       (CompareText(Copy(PreviousManagedFiles[I], 1, SeparatorIndex - 1), PluginId) = 0) then
    begin
      Result := True;
      Exit;
    end;
  end;
  for I := 0 to GetArrayLength(InstallerManagedFiles) - 1 do
  begin
    SeparatorIndex := Pos('|', InstallerManagedFiles[I]);
    if (SeparatorIndex > 1) and
       (CompareText(Copy(InstallerManagedFiles[I], 1, SeparatorIndex - 1), PluginId) = 0) then
    begin
      Result := True;
      Exit;
    end;
  end;
end;

procedure PopulatePluginRemovalList;
var
  I: Integer;
  InstalledVersion: String;
  Index: Integer;
begin
  if not Assigned(PluginRemovalList) then
    Exit;

  PluginRemovalList.Items.Clear;
  SetArrayLength(RemovePluginIds, 0);
  for I := 0 to GetArrayLength(PluginIds) - 1 do
  begin
    InstalledVersion := GetInstalledPluginVersion(PluginIds[I]);
    if (CompareText(PluginIds[I], 'salamatrix') <> 0) and
       (InstalledVersion <> '') and HasManagedFilesForPlugin(PluginIds[I]) then
    begin
      PluginRemovalList.AddCheckBox(PluginList.Items[I], '', 0, False, True, False, False, nil);
      Index := GetArrayLength(RemovePluginIds);
      SetArrayLength(RemovePluginIds, Index + 1);
      RemovePluginIds[Index] := PluginIds[I];
    end;
  end;
end;

function IsPluginExplicitlySelected(const PluginId: String): Boolean;
var
  I: Integer;
begin
  if IsPluginMarkedForRemoval(PluginId) then
  begin
    Result := False;
    Exit;
  end;

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
  if CompareText(PluginId, 'salamatrix') = 0 then
  begin
    Result := True;
    Exit;
  end;

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

function IsManagedPathOwnedByPlugin(const PluginId, RelativePath: String): Boolean;
var
  NormalizedPath: String;
  ExpectedRoot: String;
begin
  NormalizedPath := RelativePath;
  StringChangeEx(NormalizedPath, '/', '\', True);
  if CompareText(PluginId, 'salamatrixdemos') = 0 then
  begin
    Result := (CompareText(Copy(NormalizedPath, 1, Length('extensions\demos\')), 'extensions\demos\') = 0) or
      (CompareText(Copy(NormalizedPath, 1, Length('plugins\automation\scripts\Salamatrix Progress Demo\')), 'plugins\automation\scripts\Salamatrix Progress Demo\') = 0);
    Exit;
  end;

  if CompareText(PluginId, 'extensionmenubuilder') = 0 then
    ExpectedRoot := 'extensions\extension-menu-builder\'
  else if CompareText(PluginId, 'gitworktreenavigator') = 0 then
    ExpectedRoot := 'extensions\git-worktree-navigator\'
  else if CompareText(PluginId, 'filelockinspector') = 0 then
    ExpectedRoot := 'extensions\file-lock-inspector\'
  else if CompareText(PluginId, 'processexplorer') = 0 then
    ExpectedRoot := 'extensions\process-explorer\'
  else if CompareText(PluginId, 'hardwaremonitor') = 0 then
    ExpectedRoot := 'extensions\hardware-monitor\'
  else if CompareText(PluginId, 'eventviewer') = 0 then
    ExpectedRoot := 'extensions\event-viewer\'
  else if CompareText(PluginId, 'jsonwebviewer') = 0 then
    ExpectedRoot := 'extensions\json-viewer-nodejs\'
  else if IsExtensionRuntimePlugin(PluginId) then
    ExpectedRoot := 'plugins\extension-runtimes\' + PluginId + '\'
  else
    ExpectedRoot := 'plugins\' + PluginId + '\';

  Result := CompareText(Copy(NormalizedPath, 1, Length(ExpectedRoot)), ExpectedRoot) = 0;
end;

function IsExecutableOnPath(const ExecutableName: String): Boolean;
var
  PathList: String;
  Directory: String;
  SeparatorIndex: Integer;
begin
  Result := False;
  PathList := GetEnv('PATH');
  while PathList <> '' do
  begin
    SeparatorIndex := Pos(';', PathList);
    if SeparatorIndex = 0 then
    begin
      Directory := PathList;
      PathList := '';
    end
    else
    begin
      Directory := Copy(PathList, 1, SeparatorIndex - 1);
      PathList := Copy(PathList, SeparatorIndex + 1, Length(PathList) - SeparatorIndex);
    end;

    Directory := Trim(Directory);
    StringChangeEx(Directory, '"', '', True);
    if (Directory <> '') and FileExists(AddBackslash(Directory) + ExecutableName) then
    begin
      Result := True;
      Exit;
    end;
  end;
end;

function IsRuntimeMissing(const RuntimeIndex: Integer): Boolean;
begin
  case RuntimeIndex of
    0: Result := IsPluginSelected('javascriptruntime') and
      not (IsExecutableOnPath('node.exe') or
        FileExists(ExpandConstant('{pf}\nodejs\node.exe')) or
        FileExists(ExpandConstant('{pf32}\nodejs\node.exe')));
    1: Result := IsPluginSelected('pythonruntime') and
      not (IsExecutableOnPath('python.exe') or IsExecutableOnPath('python3.exe'));
    2: Result := IsPluginSelected('phpruntime') and not IsExecutableOnPath('php.exe');
    3: Result := IsPluginSelected('powershellruntime') and
      not (IsExecutableOnPath('pwsh.exe') or
        FileExists(ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe')));
  else
    Result := False;
  end;
end;

procedure RuntimeDownloadClick(Sender: TObject);
var
  RuntimeIndex: Integer;
  ErrorCode: Integer;
begin
  RuntimeIndex := TLabel(Sender).Tag;
  if (RuntimeIndex >= 0) and (RuntimeIndex <= 3) then
    ShellExec('', RuntimeLinkUrls[RuntimeIndex], '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode);
end;

procedure UpdateRuntimeNotice;
var
  I: Integer;
  AnyMissing: Boolean;
  RuntimeName: String;
begin
  if not Assigned(RuntimeWarningLabel) then
    Exit;

  AnyMissing := False;
  for I := 0 to 3 do
  begin
    RuntimeLinkLabels[I].Visible := IsRuntimeMissing(I);
    if RuntimeLinkLabels[I].Visible then
    begin
      AnyMissing := True;
      case I of
        0: RuntimeName := 'Node.js';
        1: RuntimeName := 'Python';
        2: RuntimeName := 'PHP';
        3: RuntimeName := 'PowerShell';
      end;
      RuntimeLinkLabels[I].Caption := CustomMessage('OfficialRuntimeDownload') + ' ' + RuntimeName;
      if WizardSilent then
        Log('Selected components require an external runtime that was not detected: ' + RuntimeName);
    end;
  end;

  RuntimeWarningLabel.Caption := CustomMessage('RuntimeDependencyNotice');
  RuntimeWarningLabel.Visible := AnyMissing;
end;

function IsPluginMarkedForRemoval(const PluginId: String): Boolean;
var
  I: Integer;
begin
  Result := False;
  if CompareText(PluginId, 'salamatrix') = 0 then
    Exit;
  if not Assigned(PluginRemovalList) then
    Exit;
  for I := 0 to GetArrayLength(RemovePluginIds) - 1 do
  begin
    if (CompareText(RemovePluginIds[I], PluginId) = 0) and PluginRemovalList.Checked[I] then
    begin
      Result := True;
      Exit;
    end;
  end;
end;

function ValidatePluginRemovalSelections: Boolean;
var
  I: Integer;
  J: Integer;
begin
  Result := True;
  for I := 0 to GetArrayLength(RemovePluginIds) - 1 do
  begin
    if not PluginRemovalList.Checked[I] then
      Continue;
    for J := 0 to GetArrayLength(PluginIds) - 1 do
    begin
      if PluginList.Checked[J] and PluginDependsOn(PluginIds[J], RemovePluginIds[I]) and
         not IsPluginMarkedForRemoval(PluginIds[J]) then
      begin
        MsgBox(Format(CustomMessage('PluginRemovalDependencyError'), [PluginList.Items[J]]), mbError, MB_OK);
        Result := False;
        Exit;
      end;
    end;
  end;
end;

procedure PluginRemovalListClickCheck(Sender: TObject);
begin
  UpdateRuntimeNotice;
end;

function IsSafeManagedRelativePath(const RelativePath: String): Boolean;
var
  NormalizedPath: String;
begin
  NormalizedPath := RelativePath;
  StringChangeEx(NormalizedPath, '/', '\', True);
  Result := (NormalizedPath <> '') and (NormalizedPath[1] <> '\') and
    (Pos(':', NormalizedPath) = 0) and
    (Pos('\..\', '\' + NormalizedPath + '\') = 0) and
    (Pos('\.\', '\' + NormalizedPath + '\') = 0);
end;

function ParseManagedFileEntry(const Entry: String; var PluginId, RelativePath: String): Boolean;
var
  I: Integer;
  SeparatorIndex: Integer;
  KnownPlugin: Boolean;
begin
  Result := False;
  SeparatorIndex := Pos('|', Entry);
  if (SeparatorIndex <= 1) or (SeparatorIndex >= Length(Entry)) then
    Exit;
  PluginId := Copy(Entry, 1, SeparatorIndex - 1);
  RelativePath := Copy(Entry, SeparatorIndex + 1, Length(Entry) - SeparatorIndex);
  if not IsSafeManagedRelativePath(RelativePath) or
     not IsManagedPathOwnedByPlugin(PluginId, RelativePath) then
    Exit;

  KnownPlugin := CompareText(PluginId, 'salamatrix') = 0;
  for I := 0 to GetArrayLength(PluginIds) - 1 do
    if CompareText(PluginIds[I], PluginId) = 0 then
      KnownPlugin := True;
  Result := KnownPlugin;
end;

function ManagedFileEntryExists(const Entries: TArrayOfString; const Entry: String): Boolean;
var
  I: Integer;
begin
  Result := False;
  for I := 0 to GetArrayLength(Entries) - 1 do
  begin
    if CompareText(Entries[I], Entry) = 0 then
    begin
      Result := True;
      Exit;
    end;
  end;
end;

procedure AddUniqueManagedFileEntry(var Entries: TArrayOfString; const Entry: String);
var
  Index: Integer;
begin
  if ManagedFileEntryExists(Entries, Entry) then
    Exit;
  Index := GetArrayLength(Entries);
  SetArrayLength(Entries, Index + 1);
  Entries[Index] := Entry;
end;

function DeleteManagedFile(const RelativePath: String): Boolean;
var
  FilePath: String;
  NormalizedPath: String;
  RemainingPath: String;
  PathComponent: String;
  SeparatorIndex: Integer;
  Attributes: Cardinal;
begin
  Result := False;
  if not IsSafeManagedRelativePath(RelativePath) then
    Exit;
  NormalizedPath := RelativePath;
  StringChangeEx(NormalizedPath, '/', '\', True);
  FilePath := ExpandConstant('{app}\') + NormalizedPath;
  RemainingPath := NormalizedPath;
  PathComponent := ExpandConstant('{app}');
  Attributes := GetFileAttributesW(PathConvertNormalToSuper(PathComponent));
  if (Attributes <> $FFFFFFFF) and ((Attributes and $400) <> 0) then
    Exit;
  while RemainingPath <> '' do
  begin
    SeparatorIndex := Pos('\', RemainingPath);
    if SeparatorIndex = 0 then
    begin
      PathComponent := AddBackslash(PathComponent) + RemainingPath;
      RemainingPath := '';
    end
    else
    begin
      PathComponent := AddBackslash(PathComponent) + Copy(RemainingPath, 1, SeparatorIndex - 1);
      RemainingPath := Copy(RemainingPath, SeparatorIndex + 1, Length(RemainingPath) - SeparatorIndex);
    end;

    Attributes := GetFileAttributesW(PathConvertNormalToSuper(PathComponent));
    if (Attributes <> $FFFFFFFF) and ((Attributes and $400) <> 0) then
      Exit;
  end;
  Result := not FileExists(FilePath) or DeleteFile(FilePath);
end;

procedure ApplyManagedFileInventory;
var
  UpdatedManagedFiles: TArrayOfString;
  PluginId: String;
  RelativePath: String;
  I: Integer;
begin
  SetArrayLength(UpdatedManagedFiles, 0);

  for I := 0 to GetArrayLength(PreviousManagedFiles) - 1 do
  begin
    if not ParseManagedFileEntry(PreviousManagedFiles[I], PluginId, RelativePath) then
      Continue;

    if IsPluginMarkedForRemoval(PluginId) then
    begin
      if not DeleteManagedFile(RelativePath) then
        AddUniqueManagedFileEntry(UpdatedManagedFiles, PreviousManagedFiles[I]);
    end
    else if IsPluginSelected(PluginId) then
    begin
      if ManagedFileEntryExists(InstallerManagedFiles, PreviousManagedFiles[I]) then
        AddUniqueManagedFileEntry(UpdatedManagedFiles, PreviousManagedFiles[I])
      else if not DeleteManagedFile(RelativePath) then
        AddUniqueManagedFileEntry(UpdatedManagedFiles, PreviousManagedFiles[I]);
    end
    else
      AddUniqueManagedFileEntry(UpdatedManagedFiles, PreviousManagedFiles[I]);
  end;

  for I := 0 to GetArrayLength(InstallerManagedFiles) - 1 do
  begin
    if ParseManagedFileEntry(InstallerManagedFiles[I], PluginId, RelativePath) then
    begin
      if IsPluginMarkedForRemoval(PluginId) then
      begin
        if not DeleteManagedFile(RelativePath) then
          AddUniqueManagedFileEntry(UpdatedManagedFiles, InstallerManagedFiles[I]);
      end
      else if IsPluginSelected(PluginId) then
        AddUniqueManagedFileEntry(UpdatedManagedFiles, InstallerManagedFiles[I]);
    end;
  end;

  if not SaveStringsToFile(ExpandConstant('{app}\managed-plugin-files.txt'), UpdatedManagedFiles, False) then
  begin
    Log('Could not save the managed plugin file inventory.');
    if not WizardSilent then
      MsgBox(CustomMessage('ManagedInventorySaveError'), mbError, MB_OK);
  end;
end;

function AppendRelativePath(const BasePath, ChildPath: String): String;
begin
  if BasePath = '' then
    Result := ChildPath
  else if ChildPath = '' then
    Result := BasePath
  else
    Result := AddBackslash(BasePath) + ChildPath;
end;

procedure CopyDirectoryContents(const SourceDirectory, DestinationDirectory: String);
var
  FindRec: TFindRec;
  SourcePath: String;
  DestinationPath: String;
begin
  if not DirExists(SourceDirectory) then
    RaiseException('A staged web package directory is missing.');
  if not ForceDirectories(DestinationDirectory) then
    RaiseException('The destination directory for a web package could not be created.');
  if not FindFirst(AddBackslash(SourceDirectory) + '*', FindRec) then
    Exit;
  try
    repeat
      if (FindRec.Name = '.') or (FindRec.Name = '..') then
        Continue;
      SourcePath := AddBackslash(SourceDirectory) + FindRec.Name;
      DestinationPath := AddBackslash(DestinationDirectory) + FindRec.Name;
      if (FindRec.Attributes and $400) <> 0 then
        RaiseException('A web package contains a reparse-point entry.');
      if (FindRec.Attributes and $10) <> 0 then
        CopyDirectoryContents(SourcePath, DestinationPath)
      else
      begin
        if not ForceDirectories(ExtractFileDir(DestinationPath)) then
          RaiseException('The destination directory for a web package could not be created.');
        if FileExists(DestinationPath) and
           not SetFileAttributesW(PathConvertNormalToSuper(DestinationPath), $80) then
          RaiseException('A staged package file could not be made writable.');
        if not CopyFile(SourcePath, DestinationPath, False) then
          RaiseException('A downloaded package file could not be installed.');
      end;
    until not FindNext(FindRec);
  finally
    FindClose(FindRec);
  end;
end;

function IsAutomationDemoRelativePath(const RelativePath: String): Boolean;
begin
  Result := (CompareText(RelativePath, 'scripts\Salamatrix Progress Demo') = 0) or
    (CompareText(Copy(RelativePath, 1, Length('scripts\Salamatrix Progress Demo') + 1),
      'scripts\Salamatrix Progress Demo' + '\') = 0);
end;

procedure CopyWebPackageContents(
  const InstallerId, SourceDirectory, DestinationDirectory, ChildRelativePath: String);
var
  FindRec: TFindRec;
  SourcePath: String;
  DestinationPath: String;
  RelativePath: String;
begin
  if not ForceDirectories(DestinationDirectory) then
    RaiseException('The destination directory for a web package could not be created.');
  if not FindFirst(AddBackslash(SourceDirectory) + '*', FindRec) then
    Exit;
  try
    repeat
      if (FindRec.Name = '.') or (FindRec.Name = '..') then
        Continue;
      SourcePath := AddBackslash(SourceDirectory) + FindRec.Name;
      DestinationPath := AddBackslash(DestinationDirectory) + FindRec.Name;
      RelativePath := AppendRelativePath(ChildRelativePath, FindRec.Name);
      if (CompareText(InstallerId, 'automation') = 0) and
         IsAutomationDemoRelativePath(RelativePath) and
         not IsPluginSelected('salamatrixdemos') then
        Continue;
      if (FindRec.Attributes and $400) <> 0 then
        RaiseException('A web package contains a reparse-point entry.');
      if (FindRec.Attributes and $10) <> 0 then
        CopyWebPackageContents(InstallerId, SourcePath, DestinationPath, RelativePath)
      else
      begin
        if not ForceDirectories(ExtractFileDir(DestinationPath)) then
          RaiseException('The destination directory for a web package could not be created.');
        if FileExists(DestinationPath) and
           not SetFileAttributesW(PathConvertNormalToSuper(DestinationPath), $80) then
          RaiseException('A staged package file could not be made writable.');
        if not CopyFile(SourcePath, DestinationPath, False) then
          RaiseException('A downloaded package file could not be installed.');
      end;
    until not FindNext(FindRec);
  finally
    FindClose(FindRec);
  end;
end;

function FindPackageRootDirectory(const UnpackDirectory: String): String;
var
  FindRec: TFindRec;
  DirectoryCount: Integer;
begin
  Result := '';
  DirectoryCount := 0;
  if not FindFirst(AddBackslash(UnpackDirectory) + '*', FindRec) then
    RaiseException('A downloaded package archive is empty.');
  try
    repeat
      if (FindRec.Name = '.') or (FindRec.Name = '..') then
        Continue;
      if (FindRec.Attributes and $400) <> 0 then
        RaiseException('A downloaded package contains a reparse-point entry.');
      if (FindRec.Attributes and $10) = 0 then
        RaiseException('A downloaded package contains files outside its root directory.');
      DirectoryCount := DirectoryCount + 1;
      Result := AddBackslash(UnpackDirectory) + FindRec.Name;
    until not FindNext(FindRec);
  finally
    FindClose(FindRec);
  end;
  if DirectoryCount <> 1 then
    RaiseException('A downloaded package must contain exactly one root directory.');
end;

procedure FindExtensionManifests(const DirectoryPath: String; var Count: Integer; var FirstManifest: String);
var
  FindRec: TFindRec;
  EntryPath: String;
begin
  if not FindFirst(AddBackslash(DirectoryPath) + '*', FindRec) then
    Exit;
  try
    repeat
      if (FindRec.Name = '.') or (FindRec.Name = '..') then
        Continue;
      EntryPath := AddBackslash(DirectoryPath) + FindRec.Name;
      if (FindRec.Attributes and $400) <> 0 then
        RaiseException('A downloaded extension contains a reparse-point entry.');
      if (FindRec.Attributes and $10) <> 0 then
        FindExtensionManifests(EntryPath, Count, FirstManifest)
      else if CompareText(FindRec.Name, 'extension.json') = 0 then
      begin
        Count := Count + 1;
        if Count = 1 then
          FirstManifest := EntryPath;
      end;
    until not FindNext(FindRec);
  finally
    FindClose(FindRec);
  end;
end;

function GetPackageContentRoot(const Package: TWebPackage): String;
var
  PackageRoot: String;
  FirstManifest: String;
  ManifestCount: Integer;
begin
  PackageRoot := FindPackageRootDirectory(Package.UnpackPath);
  if CompareText(Package.PackageType, 'plugin') = 0 then
  begin
    Result := PackageRoot;
    Exit;
  end;

  FirstManifest := '';
  ManifestCount := 0;
  FindExtensionManifests(PackageRoot, ManifestCount, FirstManifest);
  if ManifestCount = 0 then
    RaiseException('A downloaded extension package has no extension.json manifest.');
  if ManifestCount = 1 then
    Result := ExtractFileDir(FirstManifest)
  else
    Result := PackageRoot;
end;

procedure CollectManagedFilesFromDirectory(
  const InstallerId, InstallRelativePath, SourceDirectory, ChildRelativePath: String);
var
  FindRec: TFindRec;
  EntryPath: String;
  RelativePath: String;
  OwnerId: String;
begin
  if not FindFirst(AddBackslash(SourceDirectory) + '*', FindRec) then
    Exit;
  try
    repeat
      if (FindRec.Name = '.') or (FindRec.Name = '..') then
        Continue;
      EntryPath := AddBackslash(SourceDirectory) + FindRec.Name;
      RelativePath := AppendRelativePath(ChildRelativePath, FindRec.Name);
      if (FindRec.Attributes and $400) <> 0 then
        RaiseException('A staged web package contains a reparse-point entry.');
      if (FindRec.Attributes and $10) <> 0 then
        CollectManagedFilesFromDirectory(InstallerId, InstallRelativePath, EntryPath, RelativePath)
      else
      begin
        OwnerId := InstallerId;
        if (CompareText(InstallerId, 'automation') = 0) and
           IsAutomationDemoRelativePath(RelativePath) then
        begin
          if not IsPluginSelected('salamatrixdemos') then
            Continue;
          OwnerId := 'salamatrixdemos';
        end;
        AddUniqueManagedFileEntry(
          InstallerManagedFiles,
          OwnerId + '|' + AppendRelativePath(InstallRelativePath, RelativePath));
      end;
    until not FindNext(FindRec);
  finally
    FindClose(FindRec);
  end;
end;

procedure AddWebPackage(const InstallerId: String);
var
  Package: TWebPackage;
  Index: Integer;
begin
  for Index := 0 to GetArrayLength(WebPackages) - 1 do
    if CompareText(WebPackages[Index].InstallerId, InstallerId) = 0 then
      Exit;

  if not GetInstallerWebPackage(
       InstallerId,
       Package.PackageId,
       Package.PackageType,
       Package.PackageVersion,
       Package.PackageUrl,
       Package.PackageSha256,
       Package.InstallRelativePath) then
  begin
    WebPreparationError := Format(CustomMessage('WebPackageUnavailable'), [InstallerId]);
    RaiseException(WebPreparationError);
  end;

  if not IsSafeManagedRelativePath(Package.InstallRelativePath) then
    RaiseException('The web package catalog contains an unsafe installation path.');

  Package.InstallerId := InstallerId;
  Package.ArchivePath := ExpandConstant('{tmp}\') + Package.PackageId + '.7z';
  Package.UnpackPath := ExpandConstant('{tmp}\web-unpacked\') + InstallerId;
  Index := GetArrayLength(WebPackages);
  SetArrayLength(WebPackages, Index + 1);
  WebPackages[Index] := Package;
end;

procedure SortInstallerManagedFiles;
var
  Items: TStringList;
  I: Integer;
begin
  Items := TStringList.Create;
  try
    for I := 0 to GetArrayLength(InstallerManagedFiles) - 1 do
      Items.Add(InstallerManagedFiles[I]);
    Items.Sort;
    SetArrayLength(InstallerManagedFiles, Items.Count);
    for I := 0 to Items.Count - 1 do
      InstallerManagedFiles[I] := Items[I];
  finally
    Items.Free;
  end;
end;

procedure PrepareWebInstallerPackages;
var
  I: Integer;
  Package: TWebPackage;
  ContentRoot: String;
  DestinationDirectory: String;
begin
  WebPreparationError := '';
  SetArrayLength(WebPackages, 0);
  WebStageRoot := ExpandConstant('{tmp}\web-stage');
  if DirExists(WebStageRoot) and not DelTree(WebStageRoot, True, True, True) then
    RaiseException('The previous web package staging directory could not be cleared.');
  if DirExists(ExpandConstant('{tmp}\web-unpacked')) and
     not DelTree(ExpandConstant('{tmp}\web-unpacked'), True, True, True) then
    RaiseException('The previous web package extraction directory could not be cleared.');
  if not ForceDirectories(ExpandConstant('{tmp}\web-unpacked')) or
     not ForceDirectories(WebStageRoot) then
    RaiseException('The web package staging directories could not be created.');

  for I := 0 to GetArrayLength(PluginIds) - 1 do
    if IsPluginSelected(PluginIds[I]) then
      AddWebPackage(PluginIds[I]);
  { Salamatrix is mandatory and does not have a visible checkbox. }
  AddWebPackage('salamatrix');

  if GetArrayLength(WebPackages) = 0 then
    RaiseException('No web packages were selected.');

  DownloadPage.Clear;
  for I := 0 to GetArrayLength(WebPackages) - 1 do
  begin
    Package := WebPackages[I];
    if FileExists(Package.ArchivePath) and not DeleteFile(Package.ArchivePath) then
      RaiseException('A previous downloaded package archive could not be removed.');
    DownloadPage.Add(
      Package.PackageUrl,
      Package.PackageId + '.7z',
      Package.PackageSha256);
  end;
  if not WizardSilent then
    DownloadPage.Show;
  try
    DownloadPage.Download;
  finally
    if not WizardSilent then
      DownloadPage.Hide;
  end;
  if DownloadPage.AbortedByUser then
    RaiseException('Package download was cancelled.');

  ExtractionPage.Clear;
  for I := 0 to GetArrayLength(WebPackages) - 1 do
  begin
    Package := WebPackages[I];
    if DirExists(Package.UnpackPath) and not DelTree(Package.UnpackPath, True, True, True) then
      RaiseException('A previous extracted package directory could not be cleared.');
    if not ForceDirectories(Package.UnpackPath) then
      RaiseException('A package extraction directory could not be created.');
    ExtractionPage.Add(Package.ArchivePath, Package.UnpackPath, True);
  end;
  if not WizardSilent then
    ExtractionPage.Show;
  try
    ExtractionPage.Extract;
  finally
    if not WizardSilent then
      ExtractionPage.Hide;
  end;
  if ExtractionPage.AbortedByUser then
    RaiseException('Package extraction was cancelled.');

  SetArrayLength(InstallerManagedFiles, 0);
  for I := 0 to GetArrayLength(WebPackages) - 1 do
  begin
    Package := WebPackages[I];
    ContentRoot := GetPackageContentRoot(Package);
    DestinationDirectory := AddBackslash(WebStageRoot) + Package.InstallRelativePath;
    CopyWebPackageContents(Package.InstallerId, ContentRoot, DestinationDirectory, '');
    CollectManagedFilesFromDirectory(
      Package.InstallerId,
      Package.InstallRelativePath,
      DestinationDirectory,
      '');
  end;
  SortInstallerManagedFiles;
end;

procedure CopyWebInstallerPayload;
begin
  CopyDirectoryContents(WebStageRoot, ExpandConstant('{app}'));
end;

procedure PluginListClickCheck(Sender: TObject);
begin
  { Re-select required dependencies even if the user tries to uncheck one. }
  SelectAllPluginDependencies;
  if not ApplyingPluginConfiguration and Assigned(PluginConfigurationList) then
    PluginConfigurationList.ItemIndex := 4;
  UpdateRuntimeNotice;
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
var
  InstallLocation: String;
begin
  if RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenSalamanderSamandarin-x64_is1', 'InstallLocation', InstallLocation) and DirExists(RemoveBackslash(InstallLocation)) then
  begin
    Result := RemoveBackslash(InstallLocation);
    Exit;
  end;
  if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenSalamanderSamandarin-x64_is1', 'InstallLocation', InstallLocation) and DirExists(RemoveBackslash(InstallLocation)) then
  begin
    Result := RemoveBackslash(InstallLocation);
    Exit;
  end;
  Result := ExpandConstant('{autopf}\Open Samandarin');
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
     (Pos('Open Samandarin', DisplayName) = 1) and
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
           (CompareText(LanguageName, 'brazilianportuguese') = 0) or
          (CompareText(LanguageName, 'swedish') = 0) or
          (CompareText(LanguageName, 'catalan') = 0) or
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
  if PluginListPopulated then
    Exit;

  PluginList.Items.Clear;
  SetArrayLength(PluginIds, 0);
  SetArrayLength(PluginConfigurations, 0);
  SetArrayLength(PreviousManagedFiles, 0);
  if FileExists(ExpandConstant('{app}\managed-plugin-files.txt')) then
  begin
    if not LoadStringsFromFile(ExpandConstant('{app}\managed-plugin-files.txt'), PreviousManagedFiles) then
      Log('Could not read the previous managed plugin file inventory.');
  end;
  UpgradeInstall := False;

  AddPlugin('7zip', '7-Zip', '1.37 (x64)', 'base,standard,advanced,expert');
  AddPlugin('automation', 'Automation', '2.9 (x64)', 'advanced,expert');
  AddPlugin('checksum', 'Checksum', '2.5 (x64)', 'base,standard,advanced,expert');
  AddPlugin('dbviewer', 'Database Viewer', '1.28 (x64)', 'base,standard,advanced,expert');
  AddPlugin('demoplug', 'Demo Plugin', '2.1 (x64)', 'expert');
  AddPlugin('diskdir', 'DiskDir', '1.1 (x64)', 'base,standard,advanced,expert');
  AddPlugin('diskmap', 'DiskMap', '1.15 (x64)', 'advanced,expert');
  AddPlugin('extensionmenubuilder', 'Extension Menu Builder', '1.1.2 (x64)', 'advanced,expert');
  AddPlugin('filecomp', 'File Comparator', '1.23 (x64)', 'base,standard,advanced,expert');
  AddPlugin('filelockinspector', 'File Lock Inspector', '1.0.3 (x64)', 'advanced,expert');
  AddPlugin('processexplorer', 'Process Explorer', '1.3.0 (x64)', 'advanced,expert');
  AddPlugin('hardwaremonitor', 'Hardware Monitor', '1.0.0 (x64)', 'advanced,expert');
  AddPlugin('eventviewer', 'Event Viewer', '1.0.0 (x64)', 'advanced,expert');
  AddPlugin('jsonwebviewer', 'JSON Viewer (nodeJS)', '1.2.5 (x64)', 'advanced,expert');
  AddPlugin('folders', 'Folders', '0.4 (x64)', 'advanced,expert');
  AddPlugin('ftp', 'FTP Client', '1.39 (x64)', 'standard,advanced,expert');
  AddPlugin('gitworktreenavigator', 'Git Worktree Navigator', '1.0.2 (x64)', 'advanced,expert');
  AddPlugin('hypervm', 'Hyper-V Machines', '1.11 (x64)', 'advanced,expert');
  AddPlugin('ieviewer', 'Internet Explorer Viewer', '1.14 (x64)', 'expert');
  AddPlugin('javascriptruntime', 'JavaScript Runtime', '0.4 (x64)', 'expert');
  AddPlugin('jsonviewer', 'JSON Viewer .NET', '1.6 (x64)', 'base,standard,advanced,expert');
  AddPlugin('luaruntime', 'Lua Runtime', '0.3.2 (x64)', 'expert');
  AddPlugin('mmviewer', 'Multimedia Viewer', '1.19 (x64)', 'base,standard,advanced,expert');
  AddPlugin('nethood', 'Network', '1.11 (x64)', 'standard,advanced,expert');
  AddPlugin('pak', 'PAK', '1.74 (x64)', 'base,standard,advanced,expert');
  AddPlugin('phpruntime', 'PHP Runtime', '0.4 (x64)', 'expert');
  AddPlugin('pictview', 'PictView', '2.28 (x64)', 'base,standard,advanced,expert');
  AddPlugin('portables', 'Portable Devices', '0.7 (x64)', 'base,standard,advanced,expert');
  AddPlugin('peviewer', 'Portable Executable Viewer', '3.2 (x64)', 'base,standard,advanced,expert');
  AddPlugin('pythonruntime', 'Python Runtime', '0.3.2 (x64)', 'expert');
  AddPlugin('textviewer', 'Prism Text Viewer', '1.6 (x64)', 'base,standard,advanced,expert');
  AddPlugin('regedt', 'Registry Editor', '1.18 (x64)', 'base,standard,advanced,expert');
  AddPlugin('renamer', 'Renamer', '1.17 (x64)', 'base,standard,advanced,expert');
  AddPlugin('salamatrix', 'Salamatrix Framework', '0.7.14 (x64)', 'base,standard,advanced,expert');
  AddPlugin('salamatrixai', 'Salamatrix AI', '0.3 (x64)', 'expert');
  AddPlugin('salamatrixailocalllama', 'Salamatrix AI Local LLaMA', '0.2 (x64)', 'expert');
  AddPlugin('salamatrixdemos', 'Salamatrix Demo Sample Scripts', '1.4.2 (x64)', 'expert');
  AddPlugin('sftp', 'SFTP/SCP Client', '1.4 beta (x64)', 'advanced,expert');
  AddPlugin('powershellruntime', 'PowerShell Runtime', '0.4 (x64)', 'expert');
  AddPlugin('samandarin', 'Samandarin Update Notifier', '0.11 (x64)', 'standard,advanced,expert');
  AddPlugin('serviceexplorer', 'Service Explorer', '0.16 (x64)', 'advanced,expert');
  AddPlugin('splitcbn', 'Split & Combine', '1.13 (x64)', 'base,standard,advanced,expert');
  AddPlugin('tar', 'TAR', '3.36 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unarj', 'UnARJ', '1.24 (x64)', 'base,standard,advanced,expert');
  AddPlugin('uncab', 'UnCAB', '1.30 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unchm', 'UnCHM', '1.7 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unfat', 'UnFAT', '1.4 (x64)', 'base,standard,advanced,expert');
  AddPlugin('uniso', 'UnISO', '1.40 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unlha', 'UnLHA', '1.16 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unmime', 'UnMIME', '1.17 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unole', 'UnOLE', '1.4 (x64)', 'base,standard,advanced,expert');
  AddPlugin('unrar', 'UnRAR', '3.5 (x64)', 'base,standard,advanced,expert');
  AddPlugin('undelete', 'Undelete', '1.14 (x64)', 'base,standard,advanced,expert');
  AddPlugin('webview2renderviewer', 'WebView2 Render Viewer', '1.6 (x64)', 'base,standard,advanced,expert');
  AddPlugin('wmobile', 'Windows Mobile', '1.12 (x64)', 'expert');
  AddPlugin('zip', 'ZIP', '1.9 (x64)', 'base,standard,advanced,expert');
  PopulatePluginRemovalList;
  SelectAllPluginDependencies;
  if (SilentPluginSelection = '') and (SilentPluginConfiguration <> '') then
    SetPluginConfiguration(SilentPluginConfiguration);
  if (SilentPluginConfiguration = '') and UpgradeInstall and
     Assigned(PluginConfigurationList) then
    PluginConfigurationList.ItemIndex := 4;
  UpdateRuntimeNotice;
  PluginListPopulated := True;
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
var
  I: Integer;
begin
  InitializePluginDependencies;
#if WebInstaller == False
  ExtractTemporaryFile('managed-plugin-files.txt');
  if not LoadStringsFromFile(ExpandConstant('{tmp}\managed-plugin-files.txt'), InstallerManagedFiles) then
  begin
    SetArrayLength(InstallerManagedFiles, 0);
    Log('Could not load the installer managed plugin file inventory.');
  end;
#else
  DownloadPage := CreateDownloadPage(
    SetupMessage(msgWizardPreparing),
    SetupMessage(msgPreparingDesc),
    nil);
  DownloadPage.ShowBaseNameInsteadOfUrl := True;
  ExtractionPage := CreateExtractionPage(
    SetupMessage(msgWizardPreparing),
    SetupMessage(msgPreparingDesc),
    nil);
  ExtractionPage.ShowArchiveInsteadOfFile := True;
#endif

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
    CustomMessage('PluginSelectionDescription') + ' ' + CustomMessage('SalamatrixFrameworkNotice'));

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

  PluginList.Height := PluginSelectionPage.SurfaceHeight - ScaleY(100) - PluginList.Top - ScaleY(8);

  RuntimeLinkUrls[0] := 'https://nodejs.org/en/download';
  RuntimeLinkUrls[1] := 'https://www.python.org/downloads/windows/';
  RuntimeLinkUrls[2] := 'https://www.php.net/downloads.php';
  RuntimeLinkUrls[3] := 'https://learn.microsoft.com/powershell/scripting/install/installing-powershell-on-windows';
  RuntimeWarningLabel := TLabel.Create(PluginSelectionPage);
  RuntimeWarningLabel.Parent := PluginSelectionPage.Surface;
  RuntimeWarningLabel.Left := 0;
  RuntimeWarningLabel.Top := PluginSelectionPage.SurfaceHeight - ScaleY(100);
  RuntimeWarningLabel.Width := PluginSelectionPage.SurfaceWidth;
  RuntimeWarningLabel.Height := ScaleY(32);
  RuntimeWarningLabel.AutoSize := False;
  RuntimeWarningLabel.WordWrap := True;
  RuntimeWarningLabel.Visible := False;
  for I := 0 to 3 do
  begin
    RuntimeLinkLabels[I] := TLabel.Create(PluginSelectionPage);
    RuntimeLinkLabels[I].Parent := PluginSelectionPage.Surface;
    RuntimeLinkLabels[I].Left := 0;
    RuntimeLinkLabels[I].Top := RuntimeWarningLabel.Top + RuntimeWarningLabel.Height + ScaleY(16 * I);
    RuntimeLinkLabels[I].AutoSize := True;
    RuntimeLinkLabels[I].Tag := I;
    RuntimeLinkLabels[I].Font.Style := [fsUnderline];
    RuntimeLinkLabels[I].Font.Color := clBlue;
    RuntimeLinkLabels[I].OnClick := @RuntimeDownloadClick;
    RuntimeLinkLabels[I].Visible := False;
  end;
  PluginRemovalPage := CreateCustomPage(
    PluginSelectionPage.ID,
    CustomMessage('PluginRemovalTitle'),
    CustomMessage('PluginRemovalDescription'));
  PluginRemovalList := TNewCheckListBox.Create(PluginRemovalPage);
  PluginRemovalList.Parent := PluginRemovalPage.Surface;
  PluginRemovalList.Left := 0;
  PluginRemovalList.Top := 0;
  PluginRemovalList.Width := PluginRemovalPage.SurfaceWidth;
  PluginRemovalList.Height := PluginRemovalPage.SurfaceHeight;
  PluginRemovalList.OnClickCheck := @PluginRemovalListClickCheck;

  if WizardSilent then
    PopulatePluginList;

end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
#if WebInstaller
  WebPreparationError := '';
  try
    PrepareWebInstallerPackages;
  except
    Log('Web package preparation failed: ' + GetExceptionMessage);
    if WebPreparationError <> '' then
      Result := WebPreparationError
    else
      Result := CustomMessage('WebInstallFailure');
  end;
#endif
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

  if CurPageID = PluginRemovalPage.ID then
  begin
    if not ValidatePluginRemovalSelections then
    begin
      Result := False;
      Exit;
    end;
  end;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := False;
  if Assigned(PluginRemovalPage) and (PageID = PluginRemovalPage.ID) then
    Result := (not UpgradeInstall) or (GetArrayLength(RemovePluginIds) = 0);
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
      HKCU\Software\Open Samandarin\5.0-samandarin-(samandarinVersion)\Configuration\Plugins.ver Version (x64)
      to decide whether selected plugins should be appended. Inno installs the staged plugins directly. }
    PluginsVer := ExpandConstant('{app}\plugins\plugins.ver');
    if not FileExists(PluginsVer) then
      SaveStringToFile(PluginsVer, '', False);

#if WebInstaller
    try
      CopyWebInstallerPayload;
    except
      Log('Web package installation failed: ' + GetExceptionMessage);
      RaiseException(CustomMessage('WebInstallFailure'));
    end;
#endif
    ApplyManagedFileInventory;

  end;
end;

