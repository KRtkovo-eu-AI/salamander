/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-26 | Read consent copy from the locale catalogs. | The banner had its own per-language object, separate from the page catalogs. | | Grok 4.7 |
*/
(() => {
  "use strict";

  const STORAGE_KEY = "samandarin-analytics-consent";
  const GA_ID = "G-TEYDRMS9P4";
  const FALLBACK = {
    "consent.title": "Privacy settings",
    "consent.message": "Google Analytics helps us understand how the website is used. You can disable analytics at any time.",
    "consent.accept": "Accept",
    "consent.reject": "Reject",
    "consent.privacy": "Read the privacy policy"
  };

  const getStoredConsent = () => {
    try { return window.localStorage.getItem(STORAGE_KEY); } catch (_) { return null; }
  };

  const setStoredConsent = (value) => {
    try { window.localStorage.setItem(STORAGE_KEY, value); } catch (_) { /* Continue without persistence. */ }
  };

  const clearAnalyticsCookies = () => {
    document.cookie.split(";").forEach((cookie) => {
      const name = cookie.split("=")[0].trim();
      if (!/^_(ga|gid)/i.test(name)) return;
      document.cookie = `${name}=; Max-Age=0; path=/`;
    });
  };

  const loadAnalytics = () => {
    if (window.__samandarinAnalyticsLoaded) return;
    window.__samandarinAnalyticsLoaded = true;
    window.dataLayer = window.dataLayer || [];
    window.gtag = window.gtag || function gtag() { window.dataLayer.push(arguments); };
    window.gtag("js", new Date());
    window.gtag("config", GA_ID);
    const script = document.createElement("script");
    script.async = true;
    script.src = `https://www.googletagmanager.com/gtag/js?id=${encodeURIComponent(GA_ID)}`;
    document.head.appendChild(script);
  };

  const localesBase = new URL("../locales/", document.currentScript.src);
  let standaloneCatalog = null;

  const lookup = (catalog, key) => {
    if (!catalog || !key) return "";
    let node = catalog;
    for (const part of key.split(".")) {
      if (!node || typeof node !== "object" || !Object.prototype.hasOwnProperty.call(node, part)) return "";
      node = node[part];
    }
    return typeof node === "string" ? node : "";
  };

  const detectStandaloneLocale = () => {
    const codes = new Set(["ar", "bg", "ca", "hr", "cs", "da", "nl", "fi", "fr", "de", "el", "he", "hu", "zh-CN", "zh-TW", "id", "it", "ja", "ko", "lt", "nb", "pl", "pt", "ro", "ru", "sk", "sl", "es", "sv", "tr", "uk", "vi"]);
    let stored = null;
    try { stored = window.localStorage.getItem("samandarin-language"); } catch (_) { stored = null; }
    if (stored === "zhHans") stored = "zh-CN";
    if (codes.has(stored)) return stored;
    const language = (navigator.language || "").toLowerCase();
    if (language.startsWith("zh")) return language.includes("tw") || language.includes("hk") || language.includes("hant") ? "zh-TW" : "zh-CN";
    if (language.startsWith("pt")) return "pt";
    if (language === "no" || language.startsWith("nb") || language.startsWith("nn")) return "nb";
    const base = language.split("-")[0];
    return codes.has(base) ? base : "en";
  };

  const text = (key) => window.SamandarinI18n?.t(key) || lookup(standaloneCatalog, key) || FALLBACK[key] || "";

  const updateBackToTopPosition = (banner, settings) => {
    const visibleControl = banner.hidden ? (settings.hidden ? null : settings) : banner;
    const height = visibleControl ? Math.ceil(visibleControl.getBoundingClientRect().height) : 0;
    document.body.style.setProperty("--consent-back-to-top-bottom", `calc(var(--consent-edge, 18px) + ${height + 10}px)`);
  };

  const init = () => {
    const privacyHref = location.pathname.includes("/salamatrix/") || location.pathname.includes("/catalogs/")
      ? "../privacy.html"
      : location.pathname.includes("privacy.html") ? "#" : "privacy.html";
    const banner = document.createElement("section");
    banner.className = "consent-banner";
    banner.setAttribute("role", "dialog");
    banner.setAttribute("aria-labelledby", "consent-title");
    banner.hidden = true;
    banner.innerHTML = `
      <h2 id="consent-title"></h2>
      <p data-consent-copy></p>
      <div class="consent-banner__actions">
        <button type="button" data-consent="accept"></button>
        <button type="button" data-consent="reject"></button>
      <a href="${privacyHref}" data-consent-privacy></a>
      </div>`;
    document.body.appendChild(banner);

    const settings = document.createElement("button");
    settings.type = "button";
    settings.className = "consent-settings";
    settings.hidden = true;
    document.body.appendChild(settings);

    const render = () => {
      banner.querySelector("h2").textContent = text("consent.title");
      banner.querySelector("[data-consent-copy]").textContent = text("consent.message");
      banner.querySelector('[data-consent="accept"]').textContent = text("consent.accept");
      banner.querySelector('[data-consent="reject"]').textContent = text("consent.reject");
      banner.querySelector("[data-consent-privacy]").textContent = text("consent.privacy");
      settings.textContent = text("consent.title");
    };

    const showBanner = () => {
      render();
      banner.hidden = false;
      settings.hidden = true;
      updateBackToTopPosition(banner, settings);
      banner.querySelector('[data-consent="accept"]').focus();
    };

    const hideBanner = () => {
      banner.hidden = true;
      settings.hidden = false;
      updateBackToTopPosition(banner, settings);
    };

    banner.querySelector('[data-consent="accept"]').addEventListener("click", () => {
      setStoredConsent("granted");
      loadAnalytics();
      hideBanner();
    });
    banner.querySelector('[data-consent="reject"]').addEventListener("click", () => {
      setStoredConsent("denied");
      clearAnalyticsCookies();
      hideBanner();
    });
    settings.addEventListener("click", showBanner);
    window.addEventListener("resize", () => updateBackToTopPosition(banner, settings));
    window.addEventListener("samandarin:locale-change", render);
    render();
    if (!window.SamandarinI18n) {
      const code = detectStandaloneLocale();
      if (code !== "en") {
        fetch(new URL(`${code}.json?v=2`, localesBase))
          .then((response) => response.ok ? response.json() : null)
          .then((catalog) => {
            if (!catalog) return;
            standaloneCatalog = catalog;
            render();
          })
          .catch(() => { /* The English banner stays available. */ });
      }
    }

    const consent = getStoredConsent();
    if (consent === "granted") {
      loadAnalytics();
      settings.hidden = false;
    } else if (consent === "denied") {
      settings.hidden = false;
    } else {
      loadAnalytics();
      showBanner();
    }
    updateBackToTopPosition(banner, settings);
  };

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init, { once: true });
  else init();
})();
