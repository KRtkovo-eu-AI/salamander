(() => {
  const STORAGE_KEY = "samandarin-language";
  const DEFAULT_LOCALE = "en";

  const locales = {
    en: { name: "English", flag: "🇬🇧", htmlLang: "en" },
    cs: { name: "Čeština", flag: "🇨🇿", htmlLang: "cs" },
    nl: { name: "Nederlands", flag: "🇳🇱", htmlLang: "nl" },
    fr: { name: "Français", flag: "🇫🇷", htmlLang: "fr" },
    de: { name: "Deutsch", flag: "🇩🇪", htmlLang: "de" },
    hu: { name: "Magyar", flag: "🇭🇺", htmlLang: "hu" },
    zhHans: { name: "简体中文", flag: "🇨🇳", htmlLang: "zh-Hans" },
    ro: { name: "Română", flag: "🇷🇴", htmlLang: "ro" },
    ru: { name: "Русский", flag: "🇷🇺", htmlLang: "ru" },
    sk: { name: "Slovenčina", flag: "🇸🇰", htmlLang: "sk" },
    es: { name: "Español", flag: "🇪🇸", htmlLang: "es" }
  };

  // Keep all editable copy in one place. New page text can be localized by adding
  // the original English text as a key and per-locale values below.
  const dictionaries = {
    cs: {
      "Skip to content": "Přejít na obsah", "Language": "Jazyk", "Page sections": "Sekce stránky", "Download": "Stažení", "Plugins": "Pluginy", "Features": "Funkce", "Fork": "Fork", "Origin": "Původ", "Resources": "Zdroje",
      "Fast and reliable two-panel file manager for Windows 11 enhanced with modern features.": "Rychlý a spolehlivý dvoupanelový správce souborů pro Windows 11 rozšířený o moderní funkce.",
      "Unofficial fork": "Neoficiální fork", "Download latest release": "Stáhnout nejnovější vydání", "Loading latest GitHub release data...": "Načítají se data nejnovějšího vydání z GitHubu...",
      "Plugin Catalog": "Katalog pluginů", "Plugin catalog sources:": "Zdroje katalogu pluginů:", "Stable:": "Stabilní:", "Unofficial 3rd party:": "Neoficiální třetí strany:",
      "Included Features Overview": "Přehled zahrnutých funkcí", "Unicode and long paths support": "Podpora Unicode a dlouhých cest", "Portability": "Přenositelnost", "Manage configurations": "Správa konfigurací", "Split panels into detached window": "Rozdělení panelů do samostatných oken", "Translations": "Překlady", "Tabbed Panels": "Panely s kartami", "Shared/separate History": "Sdílená/samostatná historie", "Dark Mode": "Tmavý režim", "Configurable Command Shell Application": "Konfigurovatelná příkazová aplikace", "Tree View panel": "Panel stromového zobrazení", "User Folders": "Uživatelské složky", "Copy/Move between plugin-FS and archives": "Kopírování/přesun mezi plugin-FS a archivy", "Autocomplete path in Copy/Move/Quick Rename/Create Folder dialog": "Našeptávání cesty v dialogu Kopírovat/Přesunout/Rychle přejmenovat/Vytvořit složku", "Digitally signed": "Digitálně podepsáno",
      "Samandarin Fork Overview": "Přehled forku Samandarin", "Author & Motivation": "Autor a motivace", "Fork Name Inspiration": "Inspirace názvu forku", "Installation Notes": "Poznámky k instalaci", "Origin Overview": "Přehled původu", "License": "Licence",
      "Explore the available plugins in": "Prozkoumejte dostupné pluginy v", "Plugin catalog overview": "přehledu katalogu pluginů",
      "Work with files and folders whose names use characters from different languages, emojis, and other Unicode symbols.": "Pracujte se soubory a složkami, jejichž názvy používají znaky různých jazyků, emoji a další symboly Unicode.",
      "Save the Configuration into file storage instead of Registry.": "Ukládejte konfiguraci do souborového úložiště místo registru.", "Reworked welcome dialog with configuration management options.": "Přepracovaný uvítací dialog s možnostmi správy konfigurací.", "Configuration import and export support.": "Podpora importu a exportu konfigurace.", "Simple wizard for restoring configurations.": "Jednoduchý průvodce obnovením konfigurace.", "Choose between locations for target configuration.": "Možnost výběru umístění cílové konfigurace.", "You can have each side in separate window.": "Každou stranu můžete mít v samostatném okně.", "All binary files including installer are digitally signed.": "Všechny binární soubory včetně instalátoru jsou digitálně podepsané.",
      "Latest release:": "Nejnovější vydání:", ", published": ", vydáno", "Total downloads:": "Celkem stažení:", "Latest release downloads:": "Stažení nejnovějšího vydání:", "across all releases": "napříč všemi vydáními", "download": "stažení", "downloads": "stažení", "Installer/Extract Portable": "Instalátor / rozbalitelná portable verze", "ZIP Archive": "Archiv ZIP", "Could not load latest GitHub release data. Showing fallback download links.": "Nepodařilo se načíst data nejnovějšího vydání z GitHubu. Zobrazují se záložní odkazy ke stažení."
    },
    sk: {}, nl: {}, fr: {}, de: {}, hu: {}, zhHans: {}, ro: {}, ru: {}, es: {}
  };

  const uiTranslations = {
    nl: { "Skip to content": "Naar inhoud", "Language": "Taal", "Page sections": "Paginasecties", "Download": "Downloaden", "Plugins": "Plug-ins", "Features": "Functies", "Fork": "Fork", "Origin": "Oorsprong", "Resources": "Bronnen" },
    fr: { "Skip to content": "Aller au contenu", "Language": "Langue", "Page sections": "Sections de la page", "Download": "Téléchargement", "Plugins": "Plugins", "Features": "Fonctionnalités", "Fork": "Fork", "Origin": "Origine", "Resources": "Ressources" },
    de: { "Skip to content": "Zum Inhalt springen", "Language": "Sprache", "Page sections": "Seitenbereiche", "Download": "Download", "Plugins": "Plugins", "Features": "Funktionen", "Fork": "Fork", "Origin": "Ursprung", "Resources": "Ressourcen" },
    hu: { "Skip to content": "Ugrás a tartalomra", "Language": "Nyelv", "Page sections": "Oldalszakaszok", "Download": "Letöltés", "Plugins": "Bővítmények", "Features": "Funkciók", "Fork": "Fork", "Origin": "Eredet", "Resources": "Források" },
    zhHans: { "Skip to content": "跳到内容", "Language": "语言", "Page sections": "页面部分", "Download": "下载", "Plugins": "插件", "Features": "功能", "Fork": "分支", "Origin": "起源", "Resources": "资源" },
    ro: { "Skip to content": "Sari la conținut", "Language": "Limbă", "Page sections": "Secțiuni pagină", "Download": "Descărcare", "Plugins": "Pluginuri", "Features": "Funcții", "Fork": "Fork", "Origin": "Origine", "Resources": "Resurse" },
    ru: { "Skip to content": "К содержимому", "Language": "Язык", "Page sections": "Разделы страницы", "Download": "Скачать", "Plugins": "Плагины", "Features": "Функции", "Fork": "Форк", "Origin": "Происхождение", "Resources": "Ресурсы" },
    sk: { "Skip to content": "Prejsť na obsah", "Language": "Jazyk", "Page sections": "Sekcie stránky", "Download": "Stiahnuť", "Plugins": "Pluginy", "Features": "Funkcie", "Fork": "Fork", "Origin": "Pôvod", "Resources": "Zdroje" },
    es: { "Skip to content": "Ir al contenido", "Language": "Idioma", "Page sections": "Secciones de la página", "Download": "Descarga", "Plugins": "Plugins", "Features": "Funciones", "Fork": "Fork", "Origin": "Origen", "Resources": "Recursos" }
  };
  Object.entries(uiTranslations).forEach(([locale, values]) => Object.assign(dictionaries[locale], values));


  const releaseTranslations = {
    nl: { "download": "download", "downloads": "downloads", "Latest release:": "Nieuwste release:", "Total downloads:": "Totale downloads:", "Latest release downloads:": "Downloads van nieuwste release:", "across all releases": "over alle releases", "Installer/Extract Portable": "Installer/portable uitpakken", "ZIP Archive": "ZIP-archief", "Installer/Extract Portable ~ 12.6 MB": "Installer/portable uitpakken ~ 12.6 MB", "ZIP Archive ~ 19.5 MB": "ZIP-archief ~ 19.5 MB", "Could not load latest GitHub release data. Showing fallback download links.": "Kon de nieuwste GitHub-releasegegevens niet laden. Fallback-downloadlinks worden getoond." },
    fr: { "download": "téléchargement", "downloads": "téléchargements", "Latest release:": "Dernière version :", "Total downloads:": "Téléchargements totaux :", "Latest release downloads:": "Téléchargements de la dernière version :", "across all releases": "sur toutes les versions", "Installer/Extract Portable": "Installateur / portable extractible", "ZIP Archive": "Archive ZIP", "Installer/Extract Portable ~ 12.6 MB": "Installateur / portable extractible ~ 12.6 MB", "ZIP Archive ~ 19.5 MB": "Archive ZIP ~ 19.5 MB", "Could not load latest GitHub release data. Showing fallback download links.": "Impossible de charger les données de la dernière version GitHub. Les liens de téléchargement de secours sont affichés." },
    de: { "download": "Download", "downloads": "Downloads", "Latest release:": "Neueste Version:", "Total downloads:": "Downloads insgesamt:", "Latest release downloads:": "Downloads der neuesten Version:", "across all releases": "über alle Versionen", "Installer/Extract Portable": "Installer / portable entpacken", "ZIP Archive": "ZIP-Archiv", "Installer/Extract Portable ~ 12.6 MB": "Installer / portable entpacken ~ 12.6 MB", "ZIP Archive ~ 19.5 MB": "ZIP-Archiv ~ 19.5 MB", "Could not load latest GitHub release data. Showing fallback download links.": "Die neuesten GitHub-Release-Daten konnten nicht geladen werden. Ersatz-Downloadlinks werden angezeigt." },
    sk: { "download": "stiahnutie", "downloads": "stiahnutí", "Latest release:": "Najnovšie vydanie:", "Total downloads:": "Celkom stiahnutí:", "Latest release downloads:": "Stiahnutia najnovšieho vydania:", "across all releases": "naprieč všetkými vydaniami", "Installer/Extract Portable": "Inštalátor / rozbaliteľná portable verzia", "ZIP Archive": "Archív ZIP", "Installer/Extract Portable ~ 12.6 MB": "Inštalátor / rozbaliteľná portable verzia ~ 12.6 MB", "ZIP Archive ~ 19.5 MB": "Archív ZIP ~ 19.5 MB", "Could not load latest GitHub release data. Showing fallback download links.": "Nepodarilo sa načítať údaje najnovšieho vydania z GitHubu. Zobrazujú sa záložné odkazy na stiahnutie." },
    es: { "download": "descarga", "downloads": "descargas", "Latest release:": "Última versión:", "Total downloads:": "Descargas totales:", "Latest release downloads:": "Descargas de la última versión:", "across all releases": "en todas las versiones", "Installer/Extract Portable": "Instalador / portable extraíble", "ZIP Archive": "Archivo ZIP", "Installer/Extract Portable ~ 12.6 MB": "Instalador / portable extraíble ~ 12.6 MB", "ZIP Archive ~ 19.5 MB": "Archivo ZIP ~ 19.5 MB", "Could not load latest GitHub release data. Showing fallback download links.": "No se pudieron cargar los datos de la última versión de GitHub. Se muestran enlaces de descarga de reserva." },
    hu: { "download": "letöltés", "downloads": "letöltés", "Latest release:": "Legújabb kiadás:", "Total downloads:": "Letöltések összesen:", "Latest release downloads:": "A legújabb kiadás letöltései:", "across all releases": "az összes kiadásban", "Installer/Extract Portable": "Telepítő / kicsomagolható portable", "ZIP Archive": "ZIP-archívum", "Installer/Extract Portable ~ 12.6 MB": "Telepítő / kicsomagolható portable ~ 12.6 MB", "ZIP Archive ~ 19.5 MB": "ZIP-archívum ~ 19.5 MB", "Could not load latest GitHub release data. Showing fallback download links.": "Nem sikerült betölteni a legújabb GitHub-kiadás adatait. Tartalék letöltési hivatkozások láthatók." },
    zhHans: { "download": "次下载", "downloads": "次下载", "Latest release:": "最新版本：", "Total downloads:": "总下载量：", "Latest release downloads:": "最新版本下载量：", "across all releases": "所有版本", "Installer/Extract Portable": "安装程序 / 可解压便携版", "ZIP Archive": "ZIP 压缩包", "Installer/Extract Portable ~ 12.6 MB": "安装程序 / 可解压便携版 ~ 12.6 MB", "ZIP Archive ~ 19.5 MB": "ZIP 压缩包 ~ 19.5 MB", "Could not load latest GitHub release data. Showing fallback download links.": "无法加载最新的 GitHub 发布数据。正在显示备用下载链接。" },
    ro: { "download": "descărcare", "downloads": "descărcări", "Latest release:": "Cea mai recentă versiune:", "Total downloads:": "Descărcări totale:", "Latest release downloads:": "Descărcări pentru cea mai recentă versiune:", "across all releases": "în toate versiunile", "Installer/Extract Portable": "Instalator / portable extractibil", "ZIP Archive": "Arhivă ZIP", "Installer/Extract Portable ~ 12.6 MB": "Instalator / portable extractibil ~ 12.6 MB", "ZIP Archive ~ 19.5 MB": "Arhivă ZIP ~ 19.5 MB", "Could not load latest GitHub release data. Showing fallback download links.": "Nu s-au putut încărca datele celei mai recente versiuni GitHub. Se afișează linkuri de descărcare de rezervă." },
    ru: { "download": "загрузка", "downloads": "загрузок", "Latest release:": "Последний релиз:", "Total downloads:": "Всего загрузок:", "Latest release downloads:": "Загрузки последнего релиза:", "across all releases": "во всех релизах", "Installer/Extract Portable": "Установщик / распаковываемая portable-версия", "ZIP Archive": "ZIP-архив", "Installer/Extract Portable ~ 12.6 MB": "Установщик / portable-версия ~ 12.6 MB", "ZIP Archive ~ 19.5 MB": "ZIP-архив ~ 19.5 MB", "Could not load latest GitHub release data. Showing fallback download links.": "Не удалось загрузить данные последнего релиза GitHub. Показаны резервные ссылки для скачивания." }
  };
  Object.entries(releaseTranslations).forEach(([locale, values]) => Object.assign(dictionaries[locale], values));



  const pageTranslations = {
    nl: {
      "It evolves the": "Het ontwikkelt het", "original project": "oorspronkelijke project", "with enhancements and new features while staying compatible with the upstream code and plugin ecosystem.": "met verbeteringen en nieuwe functies, terwijl het compatibel blijft met de upstream-code en het plugin-ecosysteem.",
      "Fast and reliable two-panel file manager for Windows 11 enhanced with modern features.": "Snelle en betrouwbare bestandsbeheerder met twee panelen voor Windows 11, uitgebreid met moderne functies.",
      "Unofficial fork": "Onofficiële fork", "Download latest release": "Nieuwste versie downloaden", "Loading latest GitHub release data...": "Nieuwste GitHub-releasegegevens laden...",
      "Plugin Catalog": "Plugin-catalogus", "Plugin catalog sources:": "Bronnen van de plugin-catalogus:", "Stable:": "Stabiel:", "Unofficial 3rd party:": "Onofficiële derde partijen:",
      "Included Features Overview": "Overzicht van inbegrepen functies", "Unicode and long paths support": "Ondersteuning voor Unicode en lange paden", "Portability": "Draagbaarheid", "Manage configurations": "Configuraties beheren", "Split panels into detached window": "Panelen in afzonderlijke vensters splitsen", "Translations": "Vertalingen", "Tabbed Panels": "Panelen met tabbladen", "Shared/separate History": "Gedeelde/aparte geschiedenis", "Dark Mode": "Donkere modus", "Configurable Command Shell Application": "Configureerbare opdrachtprompttoepassing", "Tree View panel": "Boomweergavepaneel", "User Folders": "Gebruikersmappen", "Digitally signed": "Digitaal ondertekend", "Samandarin Fork Overview": "Overzicht van de Samandarin-fork", "Author & Motivation": "Auteur en motivatie", "Origin Overview": "Oorsprong", "License": "Licentie"
    },
    fr: {
      "It evolves the": "Il fait évoluer le", "original project": "projet original", "with enhancements and new features while staying compatible with the upstream code and plugin ecosystem.": "avec des améliorations et de nouvelles fonctionnalités tout en restant compatible avec le code amont et l’écosystème de plugins.",
      "Fast and reliable two-panel file manager for Windows 11 enhanced with modern features.": "Gestionnaire de fichiers rapide et fiable à deux panneaux pour Windows 11, enrichi de fonctionnalités modernes.",
      "Unofficial fork": "Fork non officiel", "Download latest release": "Télécharger la dernière version", "Loading latest GitHub release data...": "Chargement des données de la dernière version GitHub...",
      "Plugin Catalog": "Catalogue des plugins", "Plugin catalog sources:": "Sources du catalogue des plugins :", "Stable:": "Stable :", "Unofficial 3rd party:": "Tiers non officiels :",
      "Included Features Overview": "Aperçu des fonctionnalités incluses", "Unicode and long paths support": "Prise en charge d’Unicode et des chemins longs", "Portability": "Portabilité", "Manage configurations": "Gérer les configurations", "Split panels into detached window": "Séparer les panneaux dans une fenêtre détachée", "Translations": "Traductions", "Tabbed Panels": "Panneaux à onglets", "Shared/separate History": "Historique partagé/séparé", "Dark Mode": "Mode sombre", "Configurable Command Shell Application": "Application de shell configurable", "Tree View panel": "Panneau en arborescence", "User Folders": "Dossiers utilisateur", "Digitally signed": "Signature numérique", "Samandarin Fork Overview": "Aperçu du fork Samandarin", "Author & Motivation": "Auteur et motivation", "Origin Overview": "Origine", "License": "Licence"
    },
    de: {
      "It evolves the": "Es entwickelt das", "original project": "Originalprojekt", "with enhancements and new features while staying compatible with the upstream code and plugin ecosystem.": "mit Verbesserungen und neuen Funktionen weiter und bleibt dabei mit dem Upstream-Code und dem Plugin-Ökosystem kompatibel.",
      "Fast and reliable two-panel file manager for Windows 11 enhanced with modern features.": "Schneller und zuverlässiger Zwei-Fenster-Dateimanager für Windows 11, erweitert um moderne Funktionen.",
      "Unofficial fork": "Inoffizieller Fork", "Download latest release": "Neueste Version herunterladen", "Loading latest GitHub release data...": "Neueste GitHub-Release-Daten werden geladen...",
      "Plugin Catalog": "Plugin-Katalog", "Plugin catalog sources:": "Quellen des Plugin-Katalogs:", "Stable:": "Stabil:", "Unofficial 3rd party:": "Inoffizielle Drittanbieter:",
      "Included Features Overview": "Überblick über enthaltene Funktionen", "Unicode and long paths support": "Unterstützung für Unicode und lange Pfade", "Portability": "Portabilität", "Manage configurations": "Konfigurationen verwalten", "Split panels into detached window": "Panels in separate Fenster aufteilen", "Translations": "Übersetzungen", "Tabbed Panels": "Panels mit Tabs", "Shared/separate History": "Gemeinsamer/getrennter Verlauf", "Dark Mode": "Dunkler Modus", "Configurable Command Shell Application": "Konfigurierbare Kommando-Shell-Anwendung", "Tree View panel": "Baumansicht-Panel", "User Folders": "Benutzerordner", "Digitally signed": "Digital signiert", "Samandarin Fork Overview": "Überblick über den Samandarin-Fork", "Author & Motivation": "Autor und Motivation", "Origin Overview": "Ursprung", "License": "Lizenz"
    },
    hu: {
      "It evolves the": "Továbbfejleszti az", "original project": "eredeti projektet", "with enhancements and new features while staying compatible with the upstream code and plugin ecosystem.": "javításokkal és új funkciókkal, miközben kompatibilis marad az upstream kóddal és a bővítmény-ökoszisztémával.",
      "Fast and reliable two-panel file manager for Windows 11 enhanced with modern features.": "Gyors és megbízható kétpaneles fájlkezelő Windows 11-hez, modern funkciókkal bővítve.",
      "Unofficial fork": "Nem hivatalos fork", "Download latest release": "Legújabb kiadás letöltése", "Loading latest GitHub release data...": "A legújabb GitHub-kiadás adatainak betöltése...",
      "Plugin Catalog": "Bővítménykatalógus", "Plugin catalog sources:": "Bővítménykatalógus forrásai:", "Stable:": "Stabil:", "Unofficial 3rd party:": "Nem hivatalos külső forrás:",
      "Included Features Overview": "Beépített funkciók áttekintése", "Unicode and long paths support": "Unicode és hosszú elérési utak támogatása", "Portability": "Hordozhatóság", "Manage configurations": "Konfigurációk kezelése", "Split panels into detached window": "Panelek külön ablakba választása", "Translations": "Fordítások", "Tabbed Panels": "Füles panelek", "Shared/separate History": "Megosztott/külön előzmények", "Dark Mode": "Sötét mód", "Configurable Command Shell Application": "Konfigurálható parancssori alkalmazás", "Tree View panel": "Fa nézet panel", "User Folders": "Felhasználói mappák", "Digitally signed": "Digitálisan aláírva", "Samandarin Fork Overview": "A Samandarin fork áttekintése", "Author & Motivation": "Szerző és motiváció", "Origin Overview": "Eredet", "License": "Licenc"
    },
    zhHans: {
      "It evolves the": "它在", "original project": "原始项目", "with enhancements and new features while staying compatible with the upstream code and plugin ecosystem.": "基础上加入改进和新功能，同时保持与上游代码和插件生态系统的兼容。",
      "Fast and reliable two-panel file manager for Windows 11 enhanced with modern features.": "适用于 Windows 11 的快速可靠双面板文件管理器，并增强了现代功能。",
      "Unofficial fork": "非官方分支", "Download latest release": "下载最新版本", "Loading latest GitHub release data...": "正在加载最新 GitHub 发布数据...",
      "Plugin Catalog": "插件目录", "Plugin catalog sources:": "插件目录来源：", "Stable:": "稳定版：", "Unofficial 3rd party:": "非官方第三方：",
      "Included Features Overview": "内置功能概览", "Unicode and long paths support": "Unicode 和长路径支持", "Portability": "便携性", "Manage configurations": "管理配置", "Split panels into detached window": "将面板拆分到独立窗口", "Translations": "翻译", "Tabbed Panels": "标签面板", "Shared/separate History": "共享/独立历史", "Dark Mode": "深色模式", "Configurable Command Shell Application": "可配置命令 Shell 应用", "Tree View panel": "树视图面板", "User Folders": "用户文件夹", "Digitally signed": "数字签名", "Samandarin Fork Overview": "Samandarin 分支概览", "Author & Motivation": "作者与动机", "Origin Overview": "起源", "License": "许可证"
    },
    ro: {
      "It evolves the": "Dezvoltă", "original project": "proiectul original", "with enhancements and new features while staying compatible with the upstream code and plugin ecosystem.": "cu îmbunătățiri și funcții noi, păstrând compatibilitatea cu codul upstream și ecosistemul de pluginuri.",
      "Fast and reliable two-panel file manager for Windows 11 enhanced with modern features.": "Manager de fișiere rapid și fiabil cu două panouri pentru Windows 11, îmbunătățit cu funcții moderne.",
      "Unofficial fork": "Fork neoficial", "Download latest release": "Descarcă ultima versiune", "Loading latest GitHub release data...": "Se încarcă datele celei mai recente versiuni GitHub...",
      "Plugin Catalog": "Catalog de pluginuri", "Plugin catalog sources:": "Surse catalog pluginuri:", "Stable:": "Stabil:", "Unofficial 3rd party:": "Terți neoficiali:",
      "Included Features Overview": "Prezentare funcții incluse", "Unicode and long paths support": "Suport Unicode și căi lungi", "Portability": "Portabilitate", "Manage configurations": "Gestionare configurații", "Split panels into detached window": "Separă panourile într-o fereastră detașată", "Translations": "Traduceri", "Tabbed Panels": "Panouri cu file", "Shared/separate History": "Istoric partajat/separat", "Dark Mode": "Mod întunecat", "Configurable Command Shell Application": "Aplicație shell configurabilă", "Tree View panel": "Panou vedere arbore", "User Folders": "Foldere utilizator", "Digitally signed": "Semnat digital", "Samandarin Fork Overview": "Prezentare fork Samandarin", "Author & Motivation": "Autor și motivație", "Origin Overview": "Origine", "License": "Licență"
    },
    ru: {
      "It evolves the": "Он развивает", "original project": "оригинальный проект", "with enhancements and new features while staying compatible with the upstream code and plugin ecosystem.": "с улучшениями и новыми функциями, сохраняя совместимость с исходным кодом upstream и экосистемой плагинов.",
      "Fast and reliable two-panel file manager for Windows 11 enhanced with modern features.": "Быстрый и надежный двухпанельный файловый менеджер для Windows 11 с современными функциями.",
      "Unofficial fork": "Неофициальный форк", "Download latest release": "Скачать последнюю версию", "Loading latest GitHub release data...": "Загрузка данных последнего релиза GitHub...",
      "Plugin Catalog": "Каталог плагинов", "Plugin catalog sources:": "Источники каталога плагинов:", "Stable:": "Стабильные:", "Unofficial 3rd party:": "Неофициальные сторонние:",
      "Included Features Overview": "Обзор включенных функций", "Unicode and long paths support": "Поддержка Unicode и длинных путей", "Portability": "Портативность", "Manage configurations": "Управление конфигурациями", "Split panels into detached window": "Разделение панелей в отдельные окна", "Translations": "Переводы", "Tabbed Panels": "Панели с вкладками", "Shared/separate History": "Общая/отдельная история", "Dark Mode": "Темный режим", "Configurable Command Shell Application": "Настраиваемая командная оболочка", "Tree View panel": "Панель дерева", "User Folders": "Пользовательские папки", "Digitally signed": "Цифровая подпись", "Samandarin Fork Overview": "Обзор форка Samandarin", "Author & Motivation": "Автор и мотивация", "Origin Overview": "Происхождение", "License": "Лицензия"
    },
    sk: {
      "It evolves the": "Rozvíja", "original project": "pôvodný projekt", "with enhancements and new features while staying compatible with the upstream code and plugin ecosystem.": "vylepšeniami a novými funkciami, pričom zostáva kompatibilný s upstream kódom a ekosystémom pluginov.",
      "Fast and reliable two-panel file manager for Windows 11 enhanced with modern features.": "Rýchly a spoľahlivý dvojpanelový správca súborov pre Windows 11 rozšírený o moderné funkcie.",
      "Unofficial fork": "Neoficiálny fork", "Download latest release": "Stiahnuť najnovšie vydanie", "Loading latest GitHub release data...": "Načítavajú sa údaje najnovšieho vydania z GitHubu...",
      "Plugin Catalog": "Katalóg pluginov", "Plugin catalog sources:": "Zdroje katalógu pluginov:", "Stable:": "Stabilné:", "Unofficial 3rd party:": "Neoficiálne tretie strany:",
      "Included Features Overview": "Prehľad zahrnutých funkcií", "Unicode and long paths support": "Podpora Unicode a dlhých ciest", "Portability": "Prenositeľnosť", "Manage configurations": "Správa konfigurácií", "Split panels into detached window": "Rozdelenie panelov do samostatného okna", "Translations": "Preklady", "Tabbed Panels": "Panely s kartami", "Shared/separate History": "Zdieľaná/samostatná história", "Dark Mode": "Tmavý režim", "Configurable Command Shell Application": "Konfigurovateľná príkazová aplikácia", "Tree View panel": "Panel stromového zobrazenia", "User Folders": "Používateľské priečinky", "Digitally signed": "Digitálne podpísané", "Samandarin Fork Overview": "Prehľad forku Samandarin", "Author & Motivation": "Autor a motivácia", "Origin Overview": "Pôvod", "License": "Licencia"
    },
    es: {
      "It evolves the": "Desarrolla el", "original project": "proyecto original", "with enhancements and new features while staying compatible with the upstream code and plugin ecosystem.": "con mejoras y nuevas funciones, manteniendo la compatibilidad con el código upstream y el ecosistema de plugins.",
      "Fast and reliable two-panel file manager for Windows 11 enhanced with modern features.": "Administrador de archivos rápido y fiable de dos paneles para Windows 11, mejorado con funciones modernas.",
      "Unofficial fork": "Fork no oficial", "Download latest release": "Descargar la última versión", "Loading latest GitHub release data...": "Cargando datos de la última versión de GitHub...",
      "Plugin Catalog": "Catálogo de plugins", "Plugin catalog sources:": "Fuentes del catálogo de plugins:", "Stable:": "Estable:", "Unofficial 3rd party:": "Terceros no oficiales:",
      "Included Features Overview": "Resumen de funciones incluidas", "Unicode and long paths support": "Compatibilidad con Unicode y rutas largas", "Portability": "Portabilidad", "Manage configurations": "Administrar configuraciones", "Split panels into detached window": "Separar paneles en una ventana independiente", "Translations": "Traducciones", "Tabbed Panels": "Paneles con pestañas", "Shared/separate History": "Historial compartido/separado", "Dark Mode": "Modo oscuro", "Configurable Command Shell Application": "Aplicación de shell configurable", "Tree View panel": "Panel de vista de árbol", "User Folders": "Carpetas de usuario", "Digitally signed": "Firmado digitalmente", "Samandarin Fork Overview": "Resumen del fork Samandarin", "Author & Motivation": "Autor y motivación", "Origin Overview": "Origen", "License": "Licencia"
    }
  };
  Object.entries(pageTranslations).forEach(([locale, values]) => Object.assign(dictionaries[locale], values));

  const translatableTextNodes = [];

  const rememberOriginalText = (root = document.body) => {
    const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT, {
      acceptNode(node) {
        if (!node.nodeValue.trim()) return NodeFilter.FILTER_REJECT;
        if (["SCRIPT", "STYLE", "CODE"].includes(node.parentElement?.tagName)) return NodeFilter.FILTER_REJECT;
        return NodeFilter.FILTER_ACCEPT;
      }
    });

    while (walker.nextNode()) {
      const node = walker.currentNode;
      node.__samOriginalText = node.nodeValue;
      translatableTextNodes.push(node);
    }
  };

  const translateText = (text, locale) => {
    if (locale === DEFAULT_LOCALE) return text;
    const trimmed = text.trim();
    const translated = dictionaries[locale]?.[trimmed];
    if (!translated) return text;
    return text.replace(trimmed, translated);
  };

  const applyLocale = (locale) => {
    const selected = locales[locale] ? locale : DEFAULT_LOCALE;
    document.documentElement.lang = locales[selected].htmlLang;
    document.getElementById("language-flag").textContent = locales[selected].flag;

    for (const node of translatableTextNodes) {
      node.nodeValue = translateText(node.__samOriginalText, selected);
    }

    for (const element of document.querySelectorAll("[data-i18n]")) {
      const source = element.dataset.i18nSource || element.textContent.trim();
      element.dataset.i18nSource = source;
      element.textContent = translateText(source, selected).trim();
    }

    for (const element of document.querySelectorAll("[data-i18n-attr]")) {
      for (const mapping of element.dataset.i18nAttr.split(",")) {
        const [attr, source] = mapping.split(":").map((part) => part.trim());
        if (attr && source) element.setAttribute(attr, translateText(source, selected).trim());
      }
    }

    localStorage.setItem(STORAGE_KEY, selected);
    window.dispatchEvent(new CustomEvent("samandarin:locale-change", { detail: { locale: selected } }));
  };

  const setupSelector = () => {
    for (const element of document.querySelectorAll("[data-i18n]")) {
      element.dataset.i18nSource = element.textContent.trim();
    }

    const selector = document.getElementById("language-select");
    Object.entries(locales).forEach(([code, locale]) => {
      const option = document.createElement("option");
      option.value = code;
      option.textContent = `${locale.flag} ${locale.name}`;
      selector.append(option);
    });

    const browserLocale = navigator.language?.toLowerCase() || "";
    const requestedLocale = new URLSearchParams(window.location.search).get("lang");
    const initial = (requestedLocale && locales[requestedLocale] ? requestedLocale : null) ||
      localStorage.getItem(STORAGE_KEY) ||
      Object.keys(locales).find((code) => browserLocale.startsWith(locales[code].htmlLang.toLowerCase())) ||
      DEFAULT_LOCALE;

    selector.value = initial;
    selector.addEventListener("change", () => applyLocale(selector.value));
    applyLocale(initial);
  };

  window.SamandarinI18n = { applyLocale, translateText, getLocale: () => localStorage.getItem(STORAGE_KEY) || DEFAULT_LOCALE };

  rememberOriginalText();
  setupSelector();
})();
