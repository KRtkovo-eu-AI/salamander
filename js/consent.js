(() => {
  "use strict";

  const STORAGE_KEY = "samandarin-analytics-consent";
  const GA_ID = "G-TEYDRMS9P4";
  const copy = {
    en: {
      title: "Privacy settings",
      message: "Google Analytics helps us understand how the website is used. You can disable analytics at any time.",
      accept: "Accept",
      reject: "Reject",
      settings: "Privacy settings",
      privacy: "Read the privacy policy"
    },
    cs: {
      title: "Nastavení soukromí",
      message: "Google Analytics nám pomáhá pochopit používání webu. Analytiku můžete kdykoli vypnout.",
      accept: "Přijmout",
      reject: "Zakázat",
      settings: "Nastavení soukromí",
      privacy: "Přečíst zásady ochrany soukromí"
    },
    nl: {
      title: "Privacyinstellingen",
      message: "Google Analytics helpt ons te begrijpen hoe de website wordt gebruikt. U kunt analytics altijd uitschakelen.",
      accept: "Behouden",
      reject: "Uitschakelen",
      settings: "Privacyinstellingen",
      privacy: "Privacybeleid lezen"
    },
    fr: {
      title: "Paramètres de confidentialité",
      message: "Google Analytics nous aide à comprendre l'utilisation du site. Vous pouvez désactiver les statistiques à tout moment.",
      accept: "Garder",
      reject: "Désactiver",
      settings: "Paramètres de confidentialité",
      privacy: "Lire la politique de confidentialité"
    },
    de: {
      title: "Datenschutzeinstellungen",
      message: "Google Analytics hilft uns zu verstehen, wie die Website genutzt wird. Sie können die Analyse jederzeit deaktivieren.",
      accept: "Beibehalten",
      reject: "Deaktivieren",
      settings: "Datenschutzeinstellungen",
      privacy: "Datenschutzerklärung lesen"
    },
    hu: {
      title: "Adatvédelmi beállítások",
      message: "A Google Analytics segít megérteni a webhely használatát. Az analitika bármikor kikapcsolható.",
      accept: "Megtartása",
      reject: "Kikapcsolása",
      settings: "Adatvédelmi beállítások",
      privacy: "Adatvédelmi tájékoztató"
    },
    zhHans: {
      title: "隐私设置",
      message: "Google Analytics 帮助我们了解网站的使用情况。您可以随时停用分析功能。",
      accept: "保留分析功能",
      reject: "停用分析功能",
      settings: "隐私设置",
      privacy: "阅读隐私政策"
    },
    ro: {
      title: "Setări de confidențialitate",
      message: "Google Analytics ne ajută să înțelegem modul în care este utilizat site-ul. Puteți dezactiva analiza oricând.",
      accept: "Păstrează",
      reject: "Dezactivează",
      settings: "Setări de confidențialitate",
      privacy: "Citește politica de confidențialitate"
    },
    ru: {
      title: "Настройки конфиденциальности",
      message: "Google Analytics помогает понять, как используется сайт. Аналитику можно отключить в любое время.",
      accept: "Оставить",
      reject: "Отключить",
      settings: "Настройки конфиденциальности",
      privacy: "Читать политику конфиденциальности"
    },
    sk: {
      title: "Nastavenia súkromia",
      message: "Google Analytics nám pomáha pochopiť používanie webu. Analytiku môžete kedykoľvek vypnúť.",
      accept: "Ponechať",
      reject: "Vypnúť",
      settings: "Nastavenia súkromia",
      privacy: "Prečítať zásady ochrany súkromia"
    },
    es: {
      title: "Configuración de privacidad",
      message: "Google Analytics nos ayuda a entender cómo se utiliza el sitio web. Puedes desactivar la analítica en cualquier momento.",
      accept: "Mantener",
      reject: "Desactivar",
      settings: "Configuración de privacidad",
      privacy: "Leer la política de privacidad"
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
    if (i18nLocale && copy[i18nLocale]) return i18nLocale;
    const documentLocale = (document.documentElement.lang || navigator.language || "en").toLowerCase();
    if (documentLocale.startsWith("cs")) return "cs";
    if (documentLocale.startsWith("nl")) return "nl";
    if (documentLocale.startsWith("fr")) return "fr";
    if (documentLocale.startsWith("de")) return "de";
    if (documentLocale.startsWith("hu")) return "hu";
    if (documentLocale.startsWith("zh")) return "zhHans";
    if (documentLocale.startsWith("ro")) return "ro";
    if (documentLocale.startsWith("ru")) return "ru";
    if (documentLocale.startsWith("sk")) return "sk";
    if (documentLocale.startsWith("es")) return "es";
    return "en";
  };

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
