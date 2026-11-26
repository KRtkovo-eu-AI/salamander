/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-26 | Load one JSON catalog per language. | English sentences were the lookup keys, so small copy edits dropped translations. Stable keys match Salamatrix extension locales. | | Grok 4.7 |
*/
/*
  Catalogs live in /locales/<code>.json and use the same shape as Salamatrix
  extension locales: one file per language, identical keys, English in en.json.

  HTML keeps the English copy for readers without JavaScript. Bind it with
  data-i18n="group.key" (or data-i18n-alt, data-i18n-title, data-i18n-aria-label,
  data-i18n-content). A wording change keeps the key, so the other catalogs stay
  attached. Update locales/en.json together with the HTML, then the other locale
  files when the meaning changes. A missing translation falls back to English.
*/
(() => {
  const STORAGE_KEY = "samandarin-language";
  const CACHE_VERSION = "2";
  const LOCALES = [
    { code: "en", name: "English", flag: "🇬🇧", htmlLang: "en", dir: "ltr" },
    { code: "ar", name: "العربية", flag: "🇸🇦", htmlLang: "ar", dir: "rtl" },
    { code: "bg", name: "Български", flag: "🇧🇬", htmlLang: "bg", dir: "ltr" },
    { code: "ca", name: "Català", flag: "🏴", htmlLang: "ca", dir: "ltr" },
    { code: "hr", name: "Hrvatski", flag: "🇭🇷", htmlLang: "hr", dir: "ltr" },
    { code: "cs", name: "Čeština", flag: "🇨🇿", htmlLang: "cs", dir: "ltr" },
    { code: "da", name: "Dansk", flag: "🇩🇰", htmlLang: "da", dir: "ltr" },
    { code: "nl", name: "Nederlands", flag: "🇳🇱", htmlLang: "nl", dir: "ltr" },
    { code: "fi", name: "Suomi", flag: "🇫🇮", htmlLang: "fi", dir: "ltr" },
    { code: "fr", name: "Français", flag: "🇫🇷", htmlLang: "fr", dir: "ltr" },
    { code: "de", name: "Deutsch", flag: "🇩🇪", htmlLang: "de", dir: "ltr" },
    { code: "el", name: "Ελληνικά", flag: "🇬🇷", htmlLang: "el", dir: "ltr" },
    { code: "he", name: "עברית", flag: "🇮🇱", htmlLang: "he", dir: "rtl" },
    { code: "hu", name: "Magyar", flag: "🇭🇺", htmlLang: "hu", dir: "ltr" },
    { code: "zh-CN", name: "简体中文", flag: "🇨🇳", htmlLang: "zh-CN", dir: "ltr" },
    { code: "zh-TW", name: "繁體中文", flag: "🇹🇼", htmlLang: "zh-TW", dir: "ltr" },
    { code: "id", name: "Bahasa Indonesia", flag: "🇮🇩", htmlLang: "id", dir: "ltr" },
    { code: "it", name: "Italiano", flag: "🇮🇹", htmlLang: "it", dir: "ltr" },
    { code: "ja", name: "日本語", flag: "🇯🇵", htmlLang: "ja", dir: "ltr" },
    { code: "ko", name: "한국어", flag: "🇰🇷", htmlLang: "ko", dir: "ltr" },
    { code: "lt", name: "Lietuvių", flag: "🇱🇹", htmlLang: "lt", dir: "ltr" },
    { code: "nb", name: "Norsk", flag: "🇳🇴", htmlLang: "nb", dir: "ltr" },
    { code: "pl", name: "Polski", flag: "🇵🇱", htmlLang: "pl", dir: "ltr" },
    { code: "pt", name: "Português (Brasil)", flag: "🇧🇷", htmlLang: "pt-BR", dir: "ltr" },
    { code: "ro", name: "Română", flag: "🇷🇴", htmlLang: "ro", dir: "ltr" },
    { code: "ru", name: "Русский", flag: "🇷🇺", htmlLang: "ru", dir: "ltr" },
    { code: "sk", name: "Slovenčina", flag: "🇸🇰", htmlLang: "sk", dir: "ltr" },
    { code: "sl", name: "Slovenščina", flag: "🇸🇮", htmlLang: "sl", dir: "ltr" },
    { code: "es", name: "Español", flag: "🇪🇸", htmlLang: "es", dir: "ltr" },
    { code: "sv", name: "Svenska", flag: "🇸🇪", htmlLang: "sv", dir: "ltr" },
    { code: "tr", name: "Türkçe", flag: "🇹🇷", htmlLang: "tr", dir: "ltr" },
    { code: "uk", name: "Українська", flag: "🇺🇦", htmlLang: "uk", dir: "ltr" },
    { code: "vi", name: "Tiếng Việt", flag: "🇻🇳", htmlLang: "vi", dir: "ltr" }
  ];

  const localesBase = new URL("../locales/", document.currentScript.src);
  const catalogs = new Map();
  let current = "en";

  const byCode = (code) => LOCALES.find((locale) => locale.code === code);

  const matchLanguage = (language) => {
    const value = String(language || "").toLowerCase();
    if (!value) return null;
    if (value === "zhhans" || value === "zh-hans") return "zh-CN";
    if (value.startsWith("zh")) {
      return value.includes("tw") || value.includes("hk") || value.includes("mo") || value.includes("hant") ? "zh-TW" : "zh-CN";
    }
    if (value.startsWith("pt")) return "pt";
    if (value === "no" || value.startsWith("nb") || value.startsWith("nn")) return "nb";
    const base = value.split("-")[0];
    return byCode(base) ? base : null;
  };

  const normalize = (value) => {
    if (!value) return null;
    if (value === "zhHans") return "zh-CN";
    return byCode(value) ? value : matchLanguage(value);
  };

  const readStorage = () => {
    try { return window.localStorage.getItem(STORAGE_KEY); } catch (_) { return null; }
  };

  const writeStorage = (code) => {
    try { window.localStorage.setItem(STORAGE_KEY, code); } catch (_) { /* Keep the choice for this view. */ }
  };

  const lookup = (catalog, key) => {
    if (!catalog || !key) return undefined;
    let node = catalog;
    for (const part of key.split(".")) {
      if (!node || typeof node !== "object" || !Object.prototype.hasOwnProperty.call(node, part)) return undefined;
      node = node[part];
    }
    return typeof node === "string" ? node : undefined;
  };

  const t = (key) => lookup(catalogs.get(current), key) || lookup(catalogs.get("en"), key) || "";

  const resolve = (key) => {
    const value = t(key);
    return value || null;
  };

  const paint = (root = document) => {
    const locale = byCode(current) || LOCALES[0];
    document.documentElement.lang = locale.htmlLang;
    document.documentElement.dir = locale.dir;

    root.querySelectorAll("[data-i18n]").forEach((element) => {
      const value = resolve(element.getAttribute("data-i18n"));
      if (value != null) element.textContent = value;
    });

    for (const attr of ["alt", "title", "aria-label", "content", "placeholder"]) {
      root.querySelectorAll(`[data-i18n-${attr}]`).forEach((element) => {
        const value = resolve(element.getAttribute(`data-i18n-${attr}`));
        if (value != null) element.setAttribute(attr, value);
      });
    }
  };

  const loadCatalog = async (code) => {
    if (catalogs.has(code)) return catalogs.get(code);
    const response = await fetch(new URL(`${code}.json?v=${CACHE_VERSION}`, localesBase));
    if (!response.ok) throw new Error(`Locale ${code} returned ${response.status}`);
    const catalog = await response.json();
    catalogs.set(code, catalog);
    return catalog;
  };

  const applyLocale = async (code) => {
    const selected = normalize(code) || "en";
    current = selected;
    try {
      await loadCatalog("en");
      if (selected !== "en") await loadCatalog(selected);
    } catch (error) {
      console.warn("Could not load the language catalog.", error);
      if (selected !== "en") {
        current = "en";
        try { await loadCatalog("en"); } catch (_) { /* English HTML remains visible. */ }
      }
    }

    paint();
    writeStorage(current);
    const selector = document.getElementById("language-select");
    if (selector) selector.value = current;
    window.dispatchEvent(new CustomEvent("samandarin:locale-change", {
      detail: { locale: current, htmlLang: (byCode(current) || LOCALES[0]).htmlLang }
    }));
  };

  const chooseInitial = () => {
    const requested = new URLSearchParams(window.location.search).get("lang");
    const candidates = [requested, readStorage(), ...(navigator.languages || []), navigator.language];
    for (const candidate of candidates) {
      const code = normalize(candidate);
      if (code) return code;
    }
    return "en";
  };

  const selector = document.getElementById("language-select");
  if (selector) {
    for (const locale of LOCALES) {
      const option = document.createElement("option");
      option.value = locale.code;
      option.textContent = `${locale.flag} ${locale.name}`;
      selector.append(option);
    }
    selector.addEventListener("change", () => { applyLocale(selector.value); });
  }

  const active = byCode(chooseInitial()) || LOCALES[0];
  current = active.code;
  document.documentElement.lang = active.htmlLang;
  document.documentElement.dir = active.dir;

  window.SamandarinI18n = {
    t,
    applyLocale,
    getLocale: () => current,
    getIntlLocale: () => (byCode(current) || LOCALES[0]).htmlLang,
    locales: LOCALES.map((locale) => locale.code)
  };

  applyLocale(current);
})();
