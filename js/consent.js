(() => {
  "use strict";

  const STORAGE_KEY = "samandarin-analytics-consent";
  const GA_ID = "G-TEYDRMS9P4";
  const copy = {
    en: {
      title: "Privacy settings",
      message: "We use Google Analytics to understand how the website is used. Analytics cookies are optional and are disabled until you choose to allow them.",
      accept: "Allow analytics",
      reject: "Reject analytics",
      settings: "Privacy settings",
      privacy: "Read the privacy policy"
    },
    cs: {
      title: "Nastavení soukromí",
      message: "Google Analytics používáme k pochopení toho, jak se web používá. Analytické cookies jsou volitelné a do vašeho souhlasu jsou vypnuté.",
      accept: "Povolit analytiku",
      reject: "Odmítnout analytiku",
      settings: "Nastavení soukromí",
      privacy: "Přečíst zásady ochrany soukromí"
    }
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

  const getLocale = () => {
    const i18nLocale = window.SamandarinI18n?.getLocale?.();
    if (i18nLocale) return i18nLocale === "cs" ? "cs" : "en";
    return (document.documentElement.lang || navigator.language || "en").toLowerCase().startsWith("cs") ? "cs" : "en";
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
      const text = copy[getLocale()];
      banner.querySelector("h2").textContent = text.title;
      banner.querySelector("[data-consent-copy]").textContent = text.message;
      banner.querySelector('[data-consent="accept"]').textContent = text.accept;
      banner.querySelector('[data-consent="reject"]').textContent = text.reject;
      banner.querySelector("[data-consent-privacy]").textContent = text.privacy;
      settings.textContent = text.settings;
    };

    const showBanner = () => {
      render();
      banner.hidden = false;
      settings.hidden = true;
      banner.querySelector('[data-consent="accept"]').focus();
    };

    const hideBanner = () => {
      banner.hidden = true;
      settings.hidden = false;
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
    window.addEventListener("samandarin:locale-change", render);
    render();

    const consent = getStoredConsent();
    if (consent === "granted") {
      loadAnalytics();
      settings.hidden = false;
    } else if (consent === "denied") {
      settings.hidden = false;
    } else {
      showBanner();
    }
  };

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init, { once: true });
  else init();
})();
