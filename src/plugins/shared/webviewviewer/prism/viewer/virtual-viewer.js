(function () {
  "use strict";

  const READY_MESSAGE = "salamander-virtual-ready";
  const PRISM_READY_MESSAGE = "salamander-prism-ready";
  const THEME_READY_MESSAGE = "salamander-prism-theme-ready";
  const OVERSCAN_VIEWPORTS = 3;
  const HIGHLIGHT_TIMEOUT_MS = 5000;
  const CHUNK_REQUEST_TIMEOUT_MS = 2500;
  const READY_FALLBACK_MS = 400;
  const MAIN_HIGHLIGHT_LIMIT = 200000;
  const TAB_SIZE = 4;
  const LANGUAGE_ALIASES = {
    xml: "markup",
    html: "markup",
    svg: "markup",
    mathml: "markup",
    cmd: "batch",
    config: "markup",
    targets: "markup",
    props: "markup",
    csproj: "markup",
    fsproj: "markup",
    vbproj: "markup",
    vcxproj: "markup",
    vcproj: "markup"
  };

  const spacer = document.getElementById("virtual-spacer");
  const status = document.getElementById("viewer-status");
  let worker = null;
  const chunksByStart = new Map();

  let settings = null;
  let pendingPalette = null;
  let observer = null;
  let requestSequence = 0;
  let readySent = false;
  let selectionActive = false;
  let resizeFrame = 0;
  let syncFrame = 0;
  let handshakeTimer = 0;
  let readyFallbackTimer = 0;

  function host() {
    return window.chrome && window.chrome.webview ? window.chrome.webview : null;
  }

  function postHost(message) {
    const webview = host();
    if (webview) {
      webview.postMessage(message);
    }
  }

  function escapeHtml(text) {
    return String(text)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }

  function finiteNumber(value, fallback, minimum) {
    const number = Number(value);
    return Number.isFinite(number) && number >= minimum ? number : fallback;
  }

  function cssText(value, fallback) {
    return typeof value === "string" && value.length ? value : fallback;
  }

  function cssColor(value, fallback) {
    if (typeof value !== "string" || !value.length) {
      return fallback;
    }
    if (/^#([0-9a-fA-F]{3}|[0-9a-fA-F]{6}|[0-9a-fA-F]{8})$/.test(value)) {
      return value;
    }
    if (typeof CSS !== "undefined" && CSS.supports && CSS.supports("color", value)) {
      return value;
    }
    return fallback;
  }

  function normalizeSettings(message) {
    return {
      generation: message.generation,
      lineCount: Math.floor(finiteNumber(message.lineCount, 0, 0)),
      chunkLines: Math.floor(finiteNumber(message.chunkLines, 200, 1)),
      language: cssText(message.language, "none").toLowerCase(),
      showLineNumbers: Boolean(message.showLineNumbers),
      wrapLines: Boolean(message.wrapLines),
      showWhitespace: Boolean(message.showWhitespace),
      lineHeight: finiteNumber(message.lineHeight, 16, 1),
      charWidth: finiteNumber(message.charWidth, 8, 1),
      gutterWidth: finiteNumber(message.gutterWidth, 0, 0),
      fontSize: finiteNumber(message.fontSize, 13, 1),
      fontFace: cssText(message.fontFace, "Consolas, monospace"),
      fontWeight: cssText(message.fontWeight, "normal"),
      fontStyle: cssText(message.fontStyle, "normal"),
      textDecoration: cssText(message.textDecoration, "none"),
      foreground: cssColor(message.foreground, "#000000"),
      background: cssColor(message.background, "#ffffff"),
      selectedForeground: cssColor(message.selectedForeground, "#ffffff"),
      selectedBackground: cssColor(message.selectedBackground, "#0066cc"),
      gutterForeground: cssColor(message.gutterForeground, "#606060"),
      gutterBackground: cssColor(message.gutterBackground, "#f3f3f3"),
      palette: normalizePalette(message.palette)
    };
  }

  function normalizePalette(value) {
    const palette = typeof value === "string" ? value.toLowerCase() : "";
    if (palette === "prism" || palette === "custom") {
      return palette;
    }
    return "visual-studio";
  }

  const TOKEN_KEYS = [
    "comment",
    "punctuation",
    "keyword",
    "controlKeyword",
    "className",
    "function",
    "string",
    "number",
    "boolean",
    "variable",
    "namespace",
    "regex"
  ];

  const TOKEN_VARS = {
    comment: "--token-comment",
    punctuation: "--token-punctuation",
    keyword: "--token-keyword",
    controlKeyword: "--token-control-keyword",
    className: "--token-class-name",
    function: "--token-function",
    string: "--token-string",
    number: "--token-number",
    boolean: "--token-boolean",
    variable: "--token-variable",
    namespace: "--token-namespace",
    regex: "--token-regex"
  };

  const PALETTES = {
    "visual-studio": {
      dark: {
        comment: "#6a9955",
        punctuation: "#d4d4d4",
        keyword: "#569cd6",
        controlKeyword: "#c586c0",
        className: "#4ec9b0",
        function: "#dcdcaa",
        string: "#ce9178",
        number: "#b5cea8",
        boolean: "#569cd6",
        variable: "#9cdcfe",
        namespace: "#d4d4d4",
        regex: "#d16969"
      },
      light: {
        comment: "#008000",
        punctuation: "#000000",
        keyword: "#0000ff",
        controlKeyword: "#0000ff",
        className: "#2b91af",
        function: "#000000",
        string: "#a31515",
        number: "#098658",
        boolean: "#0000ff",
        variable: "#000000",
        namespace: "#000000",
        regex: "#811f3f"
      }
    },
    prism: {
      dark: {
        comment: "#999999",
        punctuation: "#cccccc",
        keyword: "#cc99cd",
        controlKeyword: "#cc99cd",
        className: "#f8c555",
        function: "#f8c555",
        string: "#7ec699",
        number: "#f08d49",
        boolean: "#f08d49",
        variable: "#7ec699",
        namespace: "#cccccc",
        regex: "#7ec699"
      },
      light: {
        comment: "#708090",
        punctuation: "#999999",
        keyword: "#0077aa",
        controlKeyword: "#0077aa",
        className: "#dd4a68",
        function: "#dd4a68",
        string: "#669900",
        number: "#990055",
        boolean: "#990055",
        variable: "#ee9900",
        namespace: "#999999",
        regex: "#ee9900"
      }
    }
  };

  PALETTES.custom = {
    dark: Object.assign({}, PALETTES["visual-studio"].dark),
    light: Object.assign({}, PALETTES["visual-studio"].light)
  };

  function tokenColors(palette, dark) {
    const schemes = PALETTES[palette] || PALETTES["visual-studio"];
    return schemes[dark ? "dark" : "light"];
  }

  function rememberCustomPalette(data) {
    if (!data || typeof data !== "object") {
      return;
    }
    const light = data.light && typeof data.light === "object" ? data.light : {};
    const dark = data.dark && typeof data.dark === "object" ? data.dark : {};
    TOKEN_KEYS.forEach(function (key) {
      PALETTES.custom.light[key] = cssColor(light[key], PALETTES.custom.light[key]);
      PALETTES.custom.dark[key] = cssColor(dark[key], PALETTES.custom.dark[key]);
    });
  }

  function parseCustomPaletteJson(value) {
    if (typeof value === "object" && value) {
      rememberCustomPalette(value);
      return;
    }
    if (typeof value !== "string" || !value.length) {
      return;
    }
    try {
      rememberCustomPalette(JSON.parse(value));
    } catch (error) {
    }
  }

  function applyIncomingCustomPalette(message) {
    if (!message || typeof message !== "object") {
      return;
    }
    if (message.custom && typeof message.custom === "object") {
      rememberCustomPalette(message.custom);
      return;
    }
    if (message.light && typeof message.light === "object" &&
        message.dark && typeof message.dark === "object") {
      rememberCustomPalette(message);
      return;
    }
    parseCustomPaletteJson(message.customJson);
  }

  function applyTokenColors(colors) {
    const root = document.documentElement.style;
    TOKEN_KEYS.forEach(function (key) {
      root.setProperty(TOKEN_VARS[key], colors[key]);
    });
  }

  function colorLuminance(color) {
    const probe = document.createElement("span");
    probe.style.color = color;
    probe.style.display = "none";
    document.body.appendChild(probe);
    const match = getComputedStyle(probe).color.match(/\d+(?:\.\d+)?/g);
    probe.remove();
    if (!match || match.length < 3) {
      return 1;
    }
    const channels = match.slice(0, 3).map(function (component) {
      const value = Number(component) / 255;
      return value <= 0.04045 ? value / 12.92 : Math.pow((value + 0.055) / 1.055, 2.4);
    });
    return 0.2126 * channels[0] + 0.7152 * channels[1] + 0.0722 * channels[2];
  }

  function applyTheme() {
    const root = document.documentElement.style;
    const variables = {
      "--viewer-line-height": settings.lineHeight + "px",
      "--viewer-char-width": settings.charWidth + "px",
      "--viewer-gutter-width": (settings.showLineNumbers ? settings.gutterWidth : 0) + "px",
      "--viewer-font-size": settings.fontSize + "px",
      "--viewer-font-face": settings.fontFace,
      "--viewer-font-weight": settings.fontWeight,
      "--viewer-font-style": settings.fontStyle,
      "--viewer-text-decoration": settings.textDecoration,
      "--viewer-foreground": settings.foreground,
      "--viewer-background": settings.background,
      "--viewer-selected-foreground": settings.selectedForeground,
      "--viewer-selected-background": settings.selectedBackground,
      "--viewer-gutter-foreground": settings.gutterForeground,
      "--viewer-gutter-background": settings.gutterBackground
    };

    Object.keys(variables).forEach(function (name) {
      root.setProperty(name, variables[name]);
    });

    const dark = colorLuminance(settings.background) < 0.35;
    applyTokenColors(tokenColors(settings.palette, dark));

    document.body.classList.toggle("wrap-lines", settings.wrapLines);
    document.body.classList.toggle("show-whitespace", settings.showWhitespace);
  }

  function createChunk(startLine, lineCount) {
    const slot = document.createElement("section");
    const state = {
      startLine: startLine,
      lineCount: lineCount,
      slot: slot,
      near: false,
      mounted: false,
      requesting: false,
      requestId: 0,
      highlightPending: false,
      highlightTimer: 0,
      requestTimer: 0,
      pendingText: "",
      cachedHtml: "",
      lineTexts: [],
      lineColumns: [],
      layoutWidth: 0,
      measuredHeight: lineCount * settings.lineHeight,
      renderRevision: 0,
      deferredHtml: null
    };

    slot.className = "chunk-slot";
    slot.style.height = state.measuredHeight + "px";
    slot.dataset.startLine = String(startLine);
    slot.setAttribute("role", "presentation");
    chunksByStart.set(startLine, state);
    spacer.appendChild(slot);
    if (observer) {
      observer.observe(slot);
    }
  }

  function initialize(message) {
    applyIncomingCustomPalette(message);
    const previousShowWhitespace = settings && settings.showWhitespace;
    settings = normalizeSettings(message);
    if (previousShowWhitespace && !settings.showWhitespace && worker) {
      worker.terminate();
      worker = null;
      startWorker();
    }
    if (pendingPalette) {
      settings.palette = pendingPalette;
      pendingPalette = null;
    }
    readySent = false;
    selectionActive = false;
    requestSequence += 1;
    window.clearTimeout(readyFallbackTimer);
    chunksByStart.forEach(function (state) {
      window.clearTimeout(state.highlightTimer);
      window.clearTimeout(state.requestTimer);
    });
    chunksByStart.clear();
    spacer.replaceChildren();
    spacer.setAttribute("aria-busy", "true");
    status.textContent = "";

    applyTheme();
    configureAutoloader();
    postHost(THEME_READY_MESSAGE);
    recreateObserver();
    for (let startLine = 0; startLine < settings.lineCount; startLine += settings.chunkLines) {
      createChunk(startLine, Math.min(settings.chunkLines, settings.lineCount - startLine));
    }

    const first = chunksByStart.get(0);
    const mountAll = settings.lineCount <= settings.chunkLines * 12;
    if (mountAll) {
      chunksByStart.forEach(function (state) {
        materialize(state);
      });
    } else if (first) {
      materialize(first);
    } else {
      requestAnimationFrame(function () {
        markPrismReady();
      });
    }
    readyFallbackTimer = window.setTimeout(markPrismReady, READY_FALLBACK_MS);
    scheduleVisibleSync();
  }

  function onIntersection(entries) {
    entries.forEach(function (entry) {
      const startLine = Number(entry.target.dataset.startLine);
      const state = chunksByStart.get(startLine);
      if (!state) {
        return;
      }
      state.near = entry.isIntersecting;
    });
    scheduleVisibleSync();
  }

  let languageLoaders = Object.create(null);

  function canonicalLanguage() {
    const language = settings && settings.language ? settings.language : "none";
    return LANGUAGE_ALIASES[language] || language;
  }

  function configureAutoloader() {
    if (!window.Prism || !Prism.plugins || !Prism.plugins.autoloader) {
      return;
    }
    // salamatrix/prism is mapped to https://prism.local; every lexer lives in components/.
    Prism.plugins.autoloader.languages_path = "https://prism.local/components/";
  }

  function ensureLanguage(language) {
    const canonical = LANGUAGE_ALIASES[language] || language;
    if (
      !canonical ||
      canonical === "none" ||
      canonical === "plain" ||
      canonical === "plaintext" ||
      canonical === "text"
    ) {
      return Promise.resolve(canonical);
    }
    if (window.Prism && Prism.languages && Prism.languages[canonical]) {
      return Promise.resolve(canonical);
    }
    if (!languageLoaders[canonical]) {
      languageLoaders[canonical] = new Promise(function (resolve) {
        const autoloader = Prism.plugins && Prism.plugins.autoloader;
        if (!autoloader || !autoloader.loadLanguages) {
          resolve(canonical);
          return;
        }
        autoloader.loadLanguages(
          [canonical],
          function () {
            resolve(canonical);
          },
          function () {
            resolve(canonical);
          }
        );
      });
    }
    return languageLoaders[canonical];
  }

  function highlightOnMainThread(text, insideMarkupComment) {
    const language = canonicalLanguage();
    if (!window.Prism || !Prism.highlight || !Prism.languages || text.length > MAIN_HIGHLIGHT_LIMIT) {
      return escapeHtml(text);
    }
    if (language === "none" || language === "plain" || language === "plaintext" || language === "text") {
      if (!settings.showWhitespace || !Prism.hooks || !Prism.hooks.run) {
        return escapeHtml(text);
      }
      const environment = { code: text, grammar: {}, language: "plain" };
      Prism.hooks.run("before-highlight", environment);
      return Prism.highlight(environment.code, environment.grammar, environment.language);
    }
    try {
      if (window.SalamanderPrism) {
        SalamanderPrism.patchLanguage(Prism, language);
      }
      const grammar = Prism.languages[language];
      if (!grammar) {
        return escapeHtml(text);
      }
      const environment = { code: text, grammar: grammar, language: language };
      if (settings.showWhitespace && Prism.hooks && Prism.hooks.run) {
        Prism.hooks.run("before-highlight", environment);
      }
      if (language === "markup" && window.SalamanderPrism && SalamanderPrism.highlightMarkupChunk) {
        return SalamanderPrism.highlightMarkupChunk(
          Prism,
          environment.code,
          environment.grammar,
          environment.language,
          Boolean(insideMarkupComment)
        );
      }
      return Prism.highlight(environment.code, environment.grammar, environment.language);
    } catch (error) {
      console.warn("Main-thread Prism highlighting failed.", error);
      return escapeHtml(text);
    }
  }

  function materialize(state) {
    if (!settings || state.mounted) {
      return;
    }
    if (state.cachedHtml) {
      paintChunk(state, state.cachedHtml);
      return;
    }
    if (state.requesting) {
      return;
    }

    state.requesting = true;
    state.slot.setAttribute("aria-busy", "true");
    window.clearTimeout(state.requestTimer);
    state.requestTimer = window.setTimeout(function () {
      if (!state.requesting) {
        return;
      }
      state.requesting = false;
      paintChunk(state, escapeHtml(state.pendingText || " "));
    }, CHUNK_REQUEST_TIMEOUT_MS);
    postHost(
      "salamander-chunk:" +
        settings.generation +
        ":" +
        state.startLine +
        ":" +
        state.lineCount
    );
  }

  function receiveChunk(message) {
    if (!settings || message.generation !== settings.generation) {
      return;
    }

    const startLine = Math.floor(Number(message.startLine));
    const state = chunksByStart.get(startLine);
    if (!state || Number(message.lineCount) !== state.lineCount || typeof message.text !== "string") {
      return;
    }

    window.clearTimeout(state.requestTimer);
    state.requestTimer = 0;
    state.requesting = false;
    state.requestId = ++requestSequence;
    state.highlightPending = true;
    state.pendingText = message.text;
    state.insideMarkupComment = Boolean(message.insideMarkupComment);
    state.lineTexts = message.text.split(/\r\n|\n|\r/);
    state.lineColumns = measureLineColumns(message.text, state.lineCount);

    if (!state.mounted) {
      paintChunk(state, escapeHtml(message.text));
    }

    if (!worker) {
      const requestId = state.requestId;
      ensureLanguage(canonicalLanguage()).then(function () {
        if (!settings || settings.generation !== message.generation || state.requestId !== requestId) {
          return;
        }
        state.highlightPending = false;
        paintChunk(state, highlightOnMainThread(state.pendingText, state.insideMarkupComment));
      });
      return;
    }

    worker.postMessage({
      type: "highlight",
      generation: settings.generation,
      requestId: state.requestId,
      startLine: state.startLine,
      lineCount: state.lineCount,
      language: canonicalLanguage(),
      showWhitespace: settings.showWhitespace,
      insideMarkupComment: state.insideMarkupComment,
      text: message.text
    });
    window.clearTimeout(state.highlightTimer);
    state.highlightTimer = window.setTimeout(function () {
      if (!state.highlightPending) {
        return;
      }
      console.warn("Prism highlighting is still running; showing available text until it completes.");
      state.highlightTimer = 0;
    }, HIGHLIGHT_TIMEOUT_MS);
  }

  function measureLineColumns(text, lineCount) {
    const columns = [];
    let column = 0;
    for (const character of text) {
      if (character === "\n") {
        columns.push(column);
        column = 0;
      } else if (character === "\r") {
        continue;
      } else if (character === "\t") {
        column += TAB_SIZE - (column % TAB_SIZE);
      } else {
        column += 1;
      }
    }
    if (columns.length < lineCount) {
      columns.push(column);
    }
    while (columns.length < lineCount) {
      columns.push(0);
    }
    return columns.slice(0, lineCount);
  }

  function receiveHighlight(message) {
    if (!settings || message.generation !== settings.generation) {
      return;
    }

    const state = chunksByStart.get(Number(message.startLine));
    if (!state || state.requestId !== message.requestId) {
      return;
    }
    window.clearTimeout(state.highlightTimer);
    state.highlightTimer = 0;
    state.highlightPending = false;

    const html =
      message.ok && typeof message.html === "string"
        ? message.html
        : escapeHtml(typeof message.text === "string" ? message.text : state.pendingText);
    if (
      html.indexOf('class="token') === -1 &&
      state.cachedHtml &&
      state.cachedHtml.indexOf('class="token') !== -1
    ) {
      return;
    }
    state.cachedHtml = html;
    state.pendingText = "";
    if (state.near || state.mounted || slotNearViewport(state.slot, OVERSCAN_VIEWPORTS)) {
      paintChunk(state, html);
    }
  }

  function normalizeInvisibleLineEndings(highlightedHtml) {
    return highlightedHtml
      .replace(/<span class="token crlf">\r\n<\/span>/g, '<span class="token crlf"></span>\r\n')
      .replace(/<span class="token lf">\n<\/span>/g, '<span class="token lf"></span>\n')
      .replace(/<span class="token cr">\r<\/span>/g, '<span class="token cr"></span>\r');
  }

  function selectionTouchesSlot(slot) {
    const selection = document.getSelection();
    if (!selection || selection.isCollapsed || selection.rangeCount === 0) {
      return false;
    }
    try {
      return selection.getRangeAt(0).intersectsNode(slot);
    } catch (error) {
      return false;
    }
  }

  function paintChunk(state, highlightedHtml) {
    highlightedHtml = normalizeInvisibleLineEndings(highlightedHtml);
    state.cachedHtml = highlightedHtml;
    if (state.mounted && selectionActive && selectionTouchesSlot(state.slot)) {
      state.deferredHtml = highlightedHtml;
      return;
    }
    state.deferredHtml = null;
    const renderRevision = ++state.renderRevision;
    const pre = document.createElement("pre");
    const code = document.createElement("code");
    const languageClass = "language-" + canonicalLanguage().replace(/[^a-z0-9_-]/g, "");

    pre.className = "viewer-chunk " + languageClass;
    pre.dataset.start = String(state.startLine + 1);
    pre.style.counterReset = "linenumber " + state.startLine;
    code.className = languageClass;
    code.innerHTML = highlightedHtml;
    pre.appendChild(code);

    if (settings.showLineNumbers) {
      pre.classList.add("line-numbers");
      appendLineNumberRows(pre, code, state.lineCount);
    }

    state.slot.replaceChildren(pre);
    state.slot.style.height = "auto";
    state.slot.removeAttribute("aria-busy");
    state.requesting = false;
    state.mounted = true;
    state.cachedHtml = highlightedHtml;

    requestAnimationFrame(function () {
      if (
        !settings ||
        !state.mounted ||
        !state.slot.isConnected ||
        state.renderRevision !== renderRevision ||
        pre.parentNode !== state.slot
      ) {
        return;
      }
      layoutLineNumbers(pre, state, true);
      const codeBox = pre.querySelector("code") || pre;
      state.measuredHeight = Math.max(
        settings.lineHeight,
        Math.ceil(codeBox.getBoundingClientRect().height)
      );
      state.slot.style.height = state.measuredHeight + "px";
      markPrismReady();
      scheduleVisibleSync();
    });
  }

  function viewportBand() {
    const height = window.innerHeight || document.documentElement.clientHeight || 0;
    const bands = [{ top: 0, bottom: height, height: height }];
    const visual = window.visualViewport;
    if (visual && visual.height > 0) {
      bands.push({
        top: visual.offsetTop,
        bottom: visual.offsetTop + visual.height,
        height: visual.height
      });
    }
    return bands;
  }

  function slotNearViewport(slot, viewports) {
    const bounds = slot.getBoundingClientRect();
    return viewportBand().some(function (band) {
      const overscan = band.height * viewports;
      return bounds.bottom > band.top - overscan && bounds.top < band.bottom + overscan;
    });
  }

  function appendLineNumberRows(pre, code, lineCount) {
    const rows = document.createElement("span");
    rows.className = "line-numbers-rows";
    rows.setAttribute("aria-hidden", "true");
    for (let index = 0; index < lineCount; index += 1) {
      rows.appendChild(document.createElement("span"));
    }
    code.appendChild(rows);
    pre.style.counterReset = "linenumber " + (Number(pre.dataset.start) - 1);
  }

  function layoutLineNumbers(pre, state, force) {
    if (!settings.showLineNumbers || !pre) {
      return;
    }
    const code = pre.querySelector("code");
    const rows = pre.querySelectorAll(".line-numbers-rows > span");
    if (!code || !rows.length) {
      return;
    }
    const width = Math.max(1, code.clientWidth);
    if (!force && state.layoutWidth === width) {
      return;
    }
    state.layoutWidth = width;
    const style = getComputedStyle(code);
    const rowHeight = parseFloat(style.lineHeight) || settings.lineHeight;
    const wrap = settings.wrapLines;
    let sizer = pre.querySelector(".wrap-sizer");
    if (wrap && !sizer) {
      sizer = document.createElement("div");
      sizer.className = "wrap-sizer";
      sizer.setAttribute("aria-hidden", "true");
      pre.appendChild(sizer);
    }
    if (sizer) {
      sizer.style.cssText =
        "position:absolute;left:0;top:0;visibility:hidden;pointer-events:none;white-space:" +
        style.whiteSpace +
        ";overflow-wrap:" +
        style.overflowWrap +
        ";word-break:" +
        style.wordBreak +
        ";font:" +
        style.font +
        ";letter-spacing:" +
        style.letterSpacing +
        ";tab-size:" +
        style.tabSize +
        ";width:" +
        width +
        "px";
    }
    rows.forEach(function (row, index) {
      let visualRows = 1;
      if (wrap && sizer) {
        const text = state.lineTexts[index] && state.lineTexts[index].length ? state.lineTexts[index] : " ";
        sizer.textContent = text;
        visualRows = Math.max(1, Math.round(sizer.getBoundingClientRect().height / rowHeight));
      }
      if (!force && Number(row.dataset.visualRows) === visualRows) {
        return;
      }
      row.dataset.visualRows = String(visualRows);
      row.style.height = visualRows * rowHeight + "px";
      row.querySelectorAll(".wrap-marker").forEach(function (marker) {
        marker.remove();
      });
      if (!wrap) {
        return;
      }
      for (let visualRow = 1; visualRow < visualRows; visualRow += 1) {
        const marker = document.createElement("i");
        marker.className = "wrap-marker";
        marker.style.top = visualRow * rowHeight + "px";
        marker.setAttribute("aria-hidden", "true");
        row.appendChild(marker);
      }
    });
  }

  function estimateChunkHeight(state, width) {
    if (!settings.wrapLines || !state.lineColumns.length) {
      return state.lineCount * settings.lineHeight;
    }
    const columnsPerRow = Math.max(1, Math.floor(width / settings.charWidth));
    const visualRows = state.lineColumns.reduce(function (total, columns) {
      return total + Math.max(1, Math.ceil(columns / columnsPerRow));
    }, 0);
    return Math.max(settings.lineHeight, visualRows * settings.lineHeight);
  }

  function markPrismReady() {
    if (readySent) {
      return;
    }
    readySent = true;
    window.clearTimeout(readyFallbackTimer);
    spacer.setAttribute("aria-busy", "false");
    let posted = false;
    const notify = function () {
      if (posted) {
        return;
      }
      posted = true;
      postHost(PRISM_READY_MESSAGE);
    };
    requestAnimationFrame(function () {
      requestAnimationFrame(notify);
    });
    window.setTimeout(notify, 100);
  }

  function unmount(state) {
    // Keep painted chunks mounted. Unmounting after wrap/resize left empty
    // holes because sibling heights changed and IntersectionObserver did not
    // re-deliver the newly visible slots.
    if (!state || !state.mounted) {
      return;
    }
  }

  function syncVisibleChunks() {
    if (!settings) {
      return;
    }
    const scrollTop = window.scrollY || document.documentElement.scrollTop || 0;
    const layoutHeight = window.innerHeight || document.documentElement.clientHeight || 0;
    const visual = window.visualViewport;
    const viewTop = visual ? scrollTop + visual.offsetTop : scrollTop;
    const viewHeight = visual && visual.height > 0 ? visual.height : layoutHeight;
    const overscan = Math.max(viewHeight, layoutHeight) * OVERSCAN_VIEWPORTS;
    const visibleTop = viewTop - overscan;
    const visibleBottom = viewTop + viewHeight + overscan;

    let y = 0;
    chunksByStart.forEach(function (state) {
      const height = Math.max(settings.lineHeight, state.measuredHeight || 0);
      const chunkTop = y;
      const chunkBottom = y + height;
      y = chunkBottom;
      const nearY = chunkBottom > visibleTop && chunkTop < visibleBottom;
      const nearRect = slotNearViewport(state.slot, OVERSCAN_VIEWPORTS);
      state.near = nearY || nearRect;
      if (state.near) {
        materialize(state);
      }
    });
  }

  function scheduleVisibleSync() {
    if (syncFrame) {
      return;
    }
    syncFrame = requestAnimationFrame(function () {
      syncFrame = 0;
      syncVisibleChunks();
    });
  }

  function selectionTouchesViewer() {
    const selection = document.getSelection();
    if (!selection || selection.isCollapsed || selection.rangeCount === 0) {
      return false;
    }
    const range = selection.getRangeAt(0);
    return spacer.contains(range.startContainer) || spacer.contains(range.endContainer);
  }

  function onSelectionChange() {
    const wasActive = selectionActive;
    selectionActive = selectionTouchesViewer();
    if (wasActive && !selectionActive) {
      chunksByStart.forEach(function (state) {
        if (state.deferredHtml !== null) {
          paintChunk(state, state.deferredHtml);
        }
      });
      scheduleVisibleSync();
    }
  }

  function recreateObserver() {
    if (observer) {
      observer.disconnect();
    }
    observer = new IntersectionObserver(onIntersection, {
      root: null,
      rootMargin: Math.round((window.innerHeight || 0) * OVERSCAN_VIEWPORTS) + "px 0px",
      threshold: 0
    });
    chunksByStart.forEach(function (state) {
      observer.observe(state.slot);
    });
  }

  function relayoutMountedChunks() {
    const contentWidth = Math.max(1, spacer.clientWidth - (settings.showLineNumbers ? settings.gutterWidth : 0));
    const measure = [];

    chunksByStart.forEach(function (state) {
      state.layoutWidth = 0;
      if (state.lineColumns.length) {
        state.measuredHeight = estimateChunkHeight(state, contentWidth);
      }
      state.slot.style.height = state.measuredHeight + "px";
      if (state.mounted && slotNearViewport(state.slot, OVERSCAN_VIEWPORTS)) {
        const pre = state.slot.querySelector("pre");
        if (pre) {
          state.slot.style.height = "auto";
          layoutLineNumbers(pre, state, true);
          measure.push({ state: state, target: pre.querySelector("code") || pre });
        }
      }
    });

    measure.forEach(function (entry) {
      entry.state.measuredHeight = Math.max(
        settings.lineHeight,
        Math.ceil(entry.target.getBoundingClientRect().height)
      );
    });
    measure.forEach(function (entry) {
      entry.state.slot.style.height = entry.state.measuredHeight + "px";
    });
  }

  function scheduleResize() {
    if (resizeFrame) {
      return;
    }
    resizeFrame = requestAnimationFrame(function () {
      resizeFrame = 0;
      if (!settings) {
        return;
      }
      relayoutMountedChunks();
      recreateObserver();
      syncVisibleChunks();
    });
  }
  function startWorker() {
    try {
      const candidate = new Worker(new URL("prism-worker.js", document.baseURI).href);
      candidate.addEventListener("message", function (event) {
        if (worker !== candidate) {
          return;
        }
        const message = event.data;
        if (message && message.type === "highlighted") {
          receiveHighlight(message);
        }
      });
      candidate.addEventListener("error", function (event) {
        if (worker !== candidate) {
          return;
        }
        console.error("Prism worker failed; falling back to the main thread.", event);
        failWorker();
      });
      worker = candidate;
    } catch (error) {
      console.error("Unable to start Prism worker.", error);
      worker = null;
    }
  }

  function failWorker() {
    const failedWorker = worker;
    worker = null;
    if (failedWorker) {
      failedWorker.terminate();
    }
    chunksByStart.forEach(function (state) {
      if (state.highlightPending && state.requestId) {
        window.clearTimeout(state.highlightTimer);
        state.highlightTimer = 0;
        const pending = state.pendingText;
        const requestId = state.requestId;
        ensureLanguage(canonicalLanguage()).then(function () {
          if (state.requestId !== requestId) {
            return;
          }
          state.highlightPending = false;
          paintChunk(state, highlightOnMainThread(pending, state.insideMarkupComment));
        });
      }
    });
  }

  startWorker();
  configureAutoloader();

  function onHostMessage(event) {
    let message = event.data;
    if (message === "salamander-resize") {
      scheduleResize();
      return;
    }
    if (typeof message === "string" && message.length && message.charAt(0) === "{") {
      try {
        message = JSON.parse(message);
      } catch (error) {
        return;
      }
    }
    if (!message || typeof message !== "object") {
      return;
    }
    if (message.type === "init") {
      initialize(message);
    } else if (message.type === "palette") {
      applyIncomingCustomPalette(message);
      const palette = normalizePalette(message.palette);
      if (settings) {
        settings.palette = palette;
        applyTheme();
      } else {
        pendingPalette = palette;
      }
    } else if (message.type === "chunk") {
      receiveChunk(message);
    }
  }

  const webview = host();
  if (webview) {
    webview.addEventListener("message", onHostMessage);
  }

  function notifyHostReady() {
    window.addEventListener("resize", scheduleResize);
    window.addEventListener("scroll", scheduleVisibleSync, { passive: true });
    if (window.visualViewport) {
      window.visualViewport.addEventListener("resize", scheduleResize);
      window.visualViewport.addEventListener("scroll", scheduleVisibleSync);
    }
    postHost(READY_MESSAGE);
    window.clearTimeout(handshakeTimer);
    handshakeTimer = window.setTimeout(function () {
      if (!settings) {
        postHost(READY_MESSAGE);
      }
    }, 250);
  }

  document.addEventListener("selectionchange", onSelectionChange);
  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", notifyHostReady, { once: true });
  } else {
    notifyHostReady();
  }
}());
