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
    nl: { "download": "download", "downloads": "downloads", "Latest release:": "Nieuwste release:", "Total downloads:": "Totale downloads:", "Latest release downloads:": "Downloads van nieuwste release:", "across all releases": "over alle releases", "Installer/Extract Portable": "Installer/portable uitpakken", "ZIP Archive": "ZIP-archief" },
    fr: { "download": "téléchargement", "downloads": "téléchargements", "Latest release:": "Dernière version :", "Total downloads:": "Téléchargements totaux :", "Latest release downloads:": "Téléchargements de la dernière version :", "across all releases": "sur toutes les versions", "Installer/Extract Portable": "Installateur / portable extractible", "ZIP Archive": "Archive ZIP" },
    de: { "download": "Download", "downloads": "Downloads", "Latest release:": "Neueste Version:", "Total downloads:": "Downloads insgesamt:", "Latest release downloads:": "Downloads der neuesten Version:", "across all releases": "über alle Versionen", "Installer/Extract Portable": "Installer / portable entpacken", "ZIP Archive": "ZIP-Archiv" },
    sk: { "download": "stiahnutie", "downloads": "stiahnutí", "Latest release:": "Najnovšie vydanie:", "Total downloads:": "Celkom stiahnutí:", "Latest release downloads:": "Stiahnutia najnovšieho vydania:", "across all releases": "naprieč všetkými vydaniami", "Installer/Extract Portable": "Inštalátor / rozbaliteľná portable verzia", "ZIP Archive": "Archív ZIP" },
    es: { "download": "descarga", "downloads": "descargas", "Latest release:": "Última versión:", "Total downloads:": "Descargas totales:", "Latest release downloads:": "Descargas de la última versión:", "across all releases": "en todas las versiones", "Installer/Extract Portable": "Instalador / portable extraíble", "ZIP Archive": "Archivo ZIP" }
  };
  Object.entries(releaseTranslations).forEach(([locale, values]) => Object.assign(dictionaries[locale], values));

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
    const selector = document.getElementById("language-select");
    Object.entries(locales).forEach(([code, locale]) => {
      const option = document.createElement("option");
      option.value = code;
      option.textContent = `${locale.flag} ${locale.name}`;
      selector.append(option);
    });

    const browserLocale = navigator.language?.toLowerCase() || "";
    const initial = localStorage.getItem(STORAGE_KEY) ||
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
