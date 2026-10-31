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
  const findBar = document.getElementById("viewer-find-bar");
  const findInput = document.getElementById("find-input");
  const findCount = document.getElementById("find-count");
  const findCase = document.getElementById("find-case");
  const findWord = document.getElementById("find-word");
  const findPrev = document.getElementById("find-prev");
  const findNext = document.getElementById("find-next");
  const findClose = document.getElementById("find-close");
  const gotoBar = document.getElementById("viewer-goto-bar");
  const gotoInput = document.getElementById("goto-input");
  const gotoClose = document.getElementById("goto-close");
  let isFindBarOpen = false;
  let isGotoBarOpen = false;
  let gotoTotal = 1;
  // After Edit > Select All the DOM only holds the mounted chunks, so Copy
  // asks the host for the whole document while this flag is set.
  let selectAllActive = false;
  let pendingMatch = null;
  const FIND_HIGHLIGHT_NAME = "viewer-find-match";
  let findHighlightRange = null;
  let findHighlightMatch = null;
  // Document selection made by find navigation (not by the user); it must not
  // block repainting its chunk with highlighted HTML.
  let findSelectionRange = null;
  let findSelectionMatch = null;
  // Match kept centered while chunk heights settle, until the user scrolls.
  let matchAnchor = null;
  let worker = null;
  const workerRequests = new Set();
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
  let readyNotifyTimer = 0;
  let readyFrame = 0;
  let documentEpoch = 0;
  let workerHasInvisibles = false;

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
      palette: normalizePalette(message.palette),
      searchPhrase: typeof message.searchPhrase === "string" ? message.searchPhrase : "",
      searchCaseSensitive: Boolean(message.searchCaseSensitive),
      searchWholeWords: Boolean(message.searchWholeWords)
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
      paintFrame: 0,
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

  function resetDocument() {
    // A running lexer cannot be cancelled by a message on its own busy thread.
    // Keep a completed worker warm, but release queued text/work on close.
    if (worker && workerRequests.size) {
      worker.terminate();
      worker = null;
      workerRequests.clear();
      workerHasInvisibles = false;
    }
    settings = null;
    documentEpoch += 1;
    readySent = false;
    selectionActive = false;
    selectAllActive = false;
    hideGotoBar();
    gotoTotal = 1;
    pendingPalette = null;
    // Find state belongs to the previous document; a reused viewer must not
    // show its query, count or a queued highlight for the new one.
    clearFindHighlight();
    hideFindBar();
    pendingMatch = null;
    findSelectionRange = null;
    findSelectionMatch = null;
    matchAnchor = null;
    if (findInput) {
      findInput.value = "";
      findInput.classList.remove("no-results");
    }
    if (findCount) {
      findCount.textContent = "";
    }
    requestSequence += 1;
    window.clearTimeout(handshakeTimer);
    window.clearTimeout(readyFallbackTimer);
    window.clearTimeout(readyNotifyTimer);
    window.cancelAnimationFrame(readyFrame);
    window.cancelAnimationFrame(resizeFrame);
    window.cancelAnimationFrame(syncFrame);
    handshakeTimer = readyFallbackTimer = readyNotifyTimer = 0;
    readyFrame = resizeFrame = syncFrame = 0;
    if (observer) {
      observer.disconnect();
      observer = null;
    }
    chunksByStart.forEach(function (state) {
      window.clearTimeout(state.highlightTimer);
      window.clearTimeout(state.requestTimer);
      window.cancelAnimationFrame(state.paintFrame);
      state.requestId = 0;
      state.mounted = false;
      state.pendingText = state.cachedHtml = "";
      state.lineTexts = state.lineColumns = [];
      state.deferredHtml = null;
    });
    chunksByStart.clear();
    languageLoaders = Object.create(null);
    spacer.replaceChildren();
    spacer.setAttribute("aria-busy", "false");
    status.textContent = "";
    const selection = document.getSelection();
    if (selection) {
      selection.removeAllRanges();
    }
    window.scrollTo(0, 0);
  }

  function isCurrentChunk(state) {
    return settings && chunksByStart.get(state.startLine) === state;
  }

  function initialize(message) {
    const palette = pendingPalette;
    resetDocument();
    applyIncomingCustomPalette(message);
    settings = normalizeSettings(message);
    if (workerHasInvisibles && !settings.showWhitespace && worker) {
      worker.terminate();
      worker = null;
    }
    if (!worker) {
      startWorker();
    }
    if (palette) {
      settings.palette = palette;
    }
    const generation = settings.generation;
    const epoch = documentEpoch;
    spacer.setAttribute("aria-busy", "true");

    applyTheme();
    configureAutoloader();
    postHost(THEME_READY_MESSAGE + ":" + generation);
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
      readyFrame = requestAnimationFrame(function () {
        markPrismReady(generation, epoch);
      });
    }
    readyFallbackTimer = window.setTimeout(function () {
      markPrismReady(generation, epoch);
    }, READY_FALLBACK_MS);
    scheduleVisibleSync();

    if (settings.searchPhrase && settings.searchPhrase.length > 0) {
      if (findInput) {
        findInput.value = settings.searchPhrase;
      }
      if (findCase) {
        findCase.classList.toggle("active", Boolean(settings.searchCaseSensitive));
      }
      if (findWord) {
        findWord.classList.toggle("active", Boolean(settings.searchWholeWords));
      }
      showFindBar(false);
      triggerSearch();
    }
  }

  function onIntersection(entries) {
    entries.forEach(function (entry) {
      const startLine = Number(entry.target.dataset.startLine);
      const state = chunksByStart.get(startLine);
      if (!state || state.slot !== entry.target) {
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
    // Resolve lexers beside this viewer on the same mapped asset origin.
    Prism.plugins.autoloader.languages_path = new URL("../components/", document.baseURI).href;
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
    if (!isCurrentChunk(state) || state.mounted) {
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
      if (!isCurrentChunk(state) || !state.requesting) {
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
        if (!isCurrentChunk(state) || settings.generation !== message.generation || state.requestId !== requestId) {
          return;
        }
        state.highlightPending = false;
        paintChunk(state, highlightOnMainThread(state.pendingText, state.insideMarkupComment));
      });
      return;
    }

    workerHasInvisibles = workerHasInvisibles || settings.showWhitespace;
    workerRequests.add(state.requestId);
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
      if (!isCurrentChunk(state) || !state.highlightPending) {
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
    if (!isCurrentChunk(state)) {
      return;
    }
    const generation = settings.generation;
    const epoch = documentEpoch;
    highlightedHtml = normalizeInvisibleLineEndings(highlightedHtml);
    state.cachedHtml = highlightedHtml;
    if (state.mounted && selectionActive && selectionTouchesSlot(state.slot) && !selectionIsFindMatch()) {
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

    // Replacing the children moves the boundaries of every live Range inside
    // the old nodes to (slot, 0): the find selection/highlight collapses but
    // its startContainer stays connected. Decide before the swap whether the
    // current match lives in this chunk and has to be marked again.
    const inChunk = function (match) {
      return Boolean(match) && state.startLine <= match.line && match.line < state.startLine + state.lineCount;
    };
    let remark = null;
    if (inChunk(findHighlightMatch)) {
      remark = { match: findHighlightMatch, select: selectionIsFindMatch() };
    } else if (inChunk(findSelectionMatch) && selectionIsFindMatch()) {
      remark = { match: findSelectionMatch, select: true };
    }
    state.slot.replaceChildren(pre);
    state.slot.style.height = "auto";
    state.slot.removeAttribute("aria-busy");
    state.requesting = false;
    state.mounted = true;
    state.cachedHtml = highlightedHtml;

    window.cancelAnimationFrame(state.paintFrame);
    state.paintFrame = requestAnimationFrame(function () {
      if (
        !isCurrentChunk(state) || documentEpoch !== epoch ||
        !state.mounted ||
        !state.slot.isConnected ||
        state.renderRevision !== renderRevision ||
        pre.parentNode !== state.slot
      ) {
        return;
      }
      state.paintFrame = 0;
      layoutLineNumbers(pre, state, true);
      const codeBox = pre.querySelector("code") || pre;
      state.measuredHeight = Math.max(
        settings.lineHeight,
        Math.ceil(codeBox.getBoundingClientRect().height)
      );
      state.slot.style.height = state.measuredHeight + "px";
      markPrismReady(generation, epoch);
      scheduleVisibleSync();
      if (pendingMatch && (pendingMatch.state === state || (state.startLine <= pendingMatch.line && pendingMatch.line < state.startLine + state.lineCount))) {
        highlightMatchInChunk(state, pendingMatch.line, pendingMatch.col, pendingMatch.length);
        pendingMatch = null;
      } else if (remark) {
        // The repaint replaced the text nodes of the current match.
        highlightMatchInChunk(state, remark.match.line, remark.match.col, remark.match.length, true, remark.select);
      }
      if (matchAnchor && state.startLine <= matchAnchor.line) {
        // A chunk at or above the match changed height after measurement.
        centerMatchAnchor();
      }
    });
  }

  function selectionIsFindMatch() {
    const selection = document.getSelection();
    if (!findSelectionRange || !selection || selection.rangeCount === 0) {
      return false;
    }
    const range = selection.getRangeAt(0);
    return range === findSelectionRange || (
      range.startContainer === findSelectionRange.startContainer &&
      range.startOffset === findSelectionRange.startOffset &&
      range.endContainer === findSelectionRange.endContainer &&
      range.endOffset === findSelectionRange.endOffset);
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

  function markPrismReady(generation, epoch) {
    if (!settings || settings.generation !== generation || documentEpoch !== epoch || readySent) {
      return;
    }
    readySent = true;
    window.clearTimeout(readyFallbackTimer);
    spacer.setAttribute("aria-busy", "false");
    let posted = false;
    const notify = function () {
      if (posted || !settings || settings.generation !== generation || documentEpoch !== epoch) {
        return;
      }
      posted = true;
      postHost(PRISM_READY_MESSAGE + ":" + generation);
    };
    readyFrame = requestAnimationFrame(function () {
      if (documentEpoch === epoch) {
        readyFrame = requestAnimationFrame(notify);
      }
    });
    readyNotifyTimer = window.setTimeout(notify, 100);
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
    if (!settings || syncFrame) {
      return;
    }
    const epoch = documentEpoch;
    syncFrame = requestAnimationFrame(function () {
      if (documentEpoch !== epoch) {
        return;
      }
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
    if (selectAllActive) {
      const current = window.getSelection();
      if (!current || current.isCollapsed) {
        selectAllActive = false;
      }
    }
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
    if (!settings || resizeFrame) {
      return;
    }
    const epoch = documentEpoch;
    resizeFrame = requestAnimationFrame(function () {
      if (documentEpoch !== epoch) {
        return;
      }
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
          workerRequests.delete(message.requestId);
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
      workerRequests.clear();
      workerHasInvisibles = false;
    } catch (error) {
      console.error("Unable to start Prism worker.", error);
      worker = null;
      workerRequests.clear();
    }
  }

  function failWorker() {
    const failedWorker = worker;
    worker = null;
    workerRequests.clear();
    if (failedWorker) {
      failedWorker.terminate();
    }
    chunksByStart.forEach(function (state) {
      if (state.highlightPending && state.requestId) {
        window.clearTimeout(state.highlightTimer);
        state.highlightTimer = 0;
        const pending = state.pendingText;
        const requestId = state.requestId;
        const generation = settings.generation;
        const epoch = documentEpoch;
        ensureLanguage(canonicalLanguage()).then(function () {
          if (!isCurrentChunk(state) || settings.generation !== generation || documentEpoch !== epoch || state.requestId !== requestId) {
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
    if (message.type === "reset") {
      resetDocument();
    } else if (message.type === "init") {
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
    } else if (message.type === "findResult") {
      handleFindResult(message);
    } else if (message.type === "openFind") {
      openFindBar(message);
    } else if (message.type === "openGoto") {
      openGotoBar(message);
    } else if (message.type === "selectAll") {
      selectWholeDocument();
    } else if (message.type === "copy") {
      copySelection();
    }
  }

  function selectWholeDocument() {
    if (!spacer) {
      return;
    }
    const range = document.createRange();
    range.selectNodeContents(spacer);
    const sel = window.getSelection();
    sel.removeAllRanges();
    sel.addRange(range);
    selectAllActive = true;
  }

  // Edit > Copy. The host owns the clipboard: it has the full text, and
  // execCommand("copy") would need a user gesture the menu click does not give the page.
  function copySelection() {
    if (selectAllActive) {
      postHost("salamander-copy-all");
      return;
    }
    // Always reply, even with nothing selected, so the host disarms its copy request.
    postHost("salamander-copy-text:" + String(window.getSelection()));
  }

  function openGotoBar(message) {
    if (!gotoBar || !gotoInput) {
      return;
    }
    const total = Number(message.total);
    gotoTotal = total >= 1 ? Math.floor(total) : 1;
    const label = typeof message.label === "string" && message.label ? message.label : "Go to line";
    gotoInput.placeholder = label + " (1-" + gotoTotal + ")";
    gotoInput.setAttribute("aria-label", label);
    gotoInput.value = "";
    gotoBar.classList.remove("hidden");
    isGotoBarOpen = true;
    gotoInput.focus();
  }

  function hideGotoBar() {
    if (!gotoBar) {
      return;
    }
    gotoBar.classList.add("hidden");
    isGotoBarOpen = false;
    window.focus();
  }

  // Scrolls to the 1-based line (clamped to the document) and selects it.
  function goToLine(value) {
    const requested = parseInt(value, 10);
    if (!isFinite(requested)) {
      return false;
    }
    const line = Math.min(gotoTotal, Math.max(1, requested)) - 1;
    clearFindHighlight();
    selectAllActive = false;
    navigateToMatch(line, 0, -1);
    return true;
  }

  function showFindBar(focusInput) {
    if (!findBar) {
      return;
    }
    findBar.classList.remove("hidden");
    isFindBarOpen = true;
    if (focusInput && findInput) {
      findInput.focus();
      findInput.select();
    }
  }

  // Ctrl+F: a hidden find bar opens with the last text searched in this run
  // (empty before the first search); an open one just takes focus.
  function openFindBar(message) {
    const query = typeof message.query === "string" ? message.query : "";
    if (isFindBarOpen || !query || !findInput) {
      showFindBar(true);
      return;
    }
    findInput.value = query;
    if (findCase) {
      findCase.classList.toggle("active", Boolean(message.matchCase));
    }
    if (findWord) {
      findWord.classList.toggle("active", Boolean(message.wholeWord));
    }
    showFindBar(true);
    triggerSearch();
  }

  function setFindHighlight(range) {
    if (typeof CSS === "undefined" || !CSS.highlights || typeof Highlight === "undefined") {
      return false;
    }
    CSS.highlights.set(FIND_HIGHLIGHT_NAME, new Highlight(range));
    return true;
  }

  function clearFindHighlight() {
    findHighlightRange = null;
    findHighlightMatch = null;
    if (typeof CSS !== "undefined" && CSS.highlights) {
      CSS.highlights.delete(FIND_HIGHLIGHT_NAME);
    }
  }

  function hideFindBar() {
    if (!findBar) {
      return;
    }
    // Leaving the find bar turns the highlighted match into a real selection
    // so it can be copied, as it would be without the find bar.
    const range = findHighlightRange;
    const match = findHighlightMatch;
    clearFindHighlight();
    findBar.classList.add("hidden");
    isFindBarOpen = false;
    window.focus();
    if (range && !range.collapsed && spacer.contains(range.startContainer)) {
      const sel = window.getSelection();
      sel.removeAllRanges();
      sel.addRange(range);
      // A later repaint of the chunk selects the match again.
      findSelectionRange = range;
      findSelectionMatch = match;
    }
  }

  function getFirstVisibleLine() {
    if (!settings || !settings.lineHeight) {
      return 0;
    }
    const scrollTop = window.scrollY || document.documentElement.scrollTop || 0;
    return Math.max(0, Math.floor(scrollTop / settings.lineHeight));
  }

  function triggerSearch() {
    if (!findInput) {
      return;
    }
    const query = findInput.value;
    const matchCase = findCase ? findCase.classList.contains("active") : false;
    const wholeWord = findWord ? findWord.classList.contains("active") : false;
    const fromLine = getFirstVisibleLine();
    postHost("salamander-find:" + JSON.stringify({
      action: "search",
      query: query,
      matchCase: matchCase,
      wholeWord: wholeWord,
      fromLine: fromLine
    }));
  }

  function triggerNextMatch(forward) {
    postHost("salamander-find:" + JSON.stringify({
      action: forward ? "next" : "prev"
    }));
  }

  function handleFindResult(msg) {
    if (!findCount) {
      return;
    }
    if (!msg.query || msg.totalCount === 0) {
      findCount.textContent = msg.query ? "0 of 0" : "";
      if (findInput) {
        findInput.classList.toggle("no-results", Boolean(msg.query));
      }
      return;
    }
    if (findInput) {
      findInput.classList.remove("no-results");
    }
    findCount.textContent = msg.matchIndex + " of " + msg.totalCount;
    navigateToMatch(msg.line, msg.col, msg.length);
  }

  function navigateToMatch(line, col, length) {
    let targetState = null;
    chunksByStart.forEach(function (state) {
      if (state.startLine <= line && line < state.startLine + state.lineCount) {
        targetState = state;
      }
    });

    let targetY = 0;
    if (targetState && targetState.slot) {
      targetY = targetState.slot.offsetTop + (line - targetState.startLine) * (settings ? settings.lineHeight : 16);
    } else {
      targetY = line * (settings ? settings.lineHeight : 16);
    }

    matchAnchor = { line: line, col: col, length: length };
    window.scrollTo({ top: Math.max(0, targetY - 120), behavior: "auto" });
    syncVisibleChunks();

    if (targetState && targetState.mounted) {
      highlightMatchInChunk(targetState, line, col, length);
    } else {
      pendingMatch = { line: line, col: col, length: length, state: targetState };
    }
  }

  // select === false re-marks the match without touching the document
  // selection (the user has moved it elsewhere since).
  function highlightMatchInChunk(state, line, col, length, keepScroll, select) {
    const pre = state.slot.querySelector("pre");
    if (!pre) {
      return;
    }
    const code = pre.querySelector("code");
    if (!code) {
      return;
    }

    const lineInChunk = line - state.startLine;
    let curLine = 0;
    let curCol = 0;
    let startNode = null;
    let startOffset = 0;
    let endNode = null;
    let endOffset = 0;

    const walker = document.createTreeWalker(code, NodeFilter.SHOW_TEXT, {
      acceptNode: function (node) {
        if (node.parentElement && node.parentElement.closest(".line-numbers-rows")) {
          return NodeFilter.FILTER_REJECT;
        }
        return NodeFilter.FILTER_ACCEPT;
      }
    });

    let node;
    let lastNode = null;
    while ((node = walker.nextNode())) {
      lastNode = node;
      const text = node.nodeValue;
      for (let i = 0; i < text.length; i += 1) {
        if (curLine === lineInChunk && curCol === col && !startNode) {
          startNode = node;
          startOffset = i;
        }
        if (curLine === lineInChunk && curCol === col + length && !endNode) {
          endNode = node;
          endOffset = i;
          break;
        }
        if (length < 0 && startNode && !endNode && text[i] === "\n") {
          // length -1 selects up to the end of the line
          endNode = node;
          endOffset = i;
          break;
        }
        if (text[i] === "\n") {
          curLine += 1;
          curCol = 0;
        } else {
          curCol += 1;
        }
      }
      if (startNode && endNode) {
        break;
      }
    }

    if (startNode) {
      if (!endNode) {
        if (length < 0) {
          // Whole line requested (go to line) and it is the last one: end at the last text node.
          endNode = lastNode || startNode;
          endOffset = endNode.nodeValue.length;
        } else {
          endNode = startNode;
          endOffset = Math.min(startNode.nodeValue.length, startOffset + length);
        }
      }
      try {
        const range = document.createRange();
        range.setStart(startNode, startOffset);
        range.setEnd(endNode, endOffset);
        // While the find bar is open the current match is always marked with
        // the custom highlight: a document selection alone is not reliably
        // visible when the WebView lacks focus (opened from Find Files). A
        // selection would pull focus out of the find input, so while the
        // user types there the match is only highlighted; otherwise it is
        // selected as well so it can be copied.
        const typing = findBar && findBar.contains(document.activeElement);
        const highlighted = length >= 0 && (isFindBarOpen || typing) && setFindHighlight(range);
        if (highlighted) {
          findHighlightRange = range;
          findHighlightMatch = { line: line, col: col, length: length };
        } else {
          clearFindHighlight();
        }
        if ((!typing || !highlighted) && select !== false) {
          const sel = window.getSelection();
          sel.removeAllRanges();
          sel.addRange(range);
          findSelectionRange = range;
          findSelectionMatch = { line: line, col: col, length: length };
        }
        if (!keepScroll) {
          centerRange(range, state, line);
        }
      } catch (e) {
        // selection range fallback
      }
    }
  }

  // Center the match itself. Scrolling its parent element into view centered
  // the whole <code> block of an unhighlighted chunk, far below the match.
  function centerRange(range, state, line) {
    const viewHeight = window.innerHeight || document.documentElement.clientHeight || 0;
    const scrollTop = window.scrollY || document.documentElement.scrollTop || 0;
    let top = null;
    let height = 0;
    const rect = range && range.getBoundingClientRect ? range.getBoundingClientRect() : null;
    if (rect && (rect.height > 0 || rect.width > 0)) {
      top = rect.top;
      height = rect.height;
    } else if (state && state.slot) {
      top = state.slot.getBoundingClientRect().top + (line - state.startLine) * settings.lineHeight;
      height = settings.lineHeight;
    }
    if (top === null) {
      return;
    }
    window.scrollTo({ top: Math.max(0, scrollTop + top + height / 2 - viewHeight / 2), behavior: "auto" });
  }

  function centerMatchAnchor() {
    if (!matchAnchor || !settings) {
      return;
    }
    let target = null;
    chunksByStart.forEach(function (state) {
      if (state.startLine <= matchAnchor.line && matchAnchor.line < state.startLine + state.lineCount) {
        target = state;
      }
    });
    let range = null;
    if (findSelectionMatch && findSelectionMatch.line === matchAnchor.line && findSelectionRange &&
        findSelectionRange.startContainer.isConnected) {
      range = findSelectionRange;
    } else if (findHighlightMatch && findHighlightMatch.line === matchAnchor.line && findHighlightRange &&
        findHighlightRange.startContainer.isConnected) {
      range = findHighlightRange;
    }
    centerRange(range, target, matchAnchor.line);
  }

  function releaseMatchAnchor() {
    matchAnchor = null;
  }
  ["wheel", "mousedown", "touchstart", "keydown"].forEach(function (type) {
    window.addEventListener(type, releaseMatchAnchor, { passive: true, capture: true });
  });

  if (findInput) {
    findInput.addEventListener("input", function () {
      triggerSearch();
    });
    findInput.addEventListener("keydown", function (event) {
      if (event.key === "Enter") {
        event.preventDefault();
        triggerNextMatch(!event.shiftKey);
      } else if (event.key === "Escape") {
        event.preventDefault();
        event.stopPropagation();
        hideFindBar();
      }
    });
  }
  if (findNext) {
    findNext.addEventListener("click", function () {
      triggerNextMatch(true);
    });
  }
  if (findPrev) {
    findPrev.addEventListener("click", function () {
      triggerNextMatch(false);
    });
  }
  if (findClose) {
    findClose.addEventListener("click", function () {
      hideFindBar();
    });
  }
  if (findCase) {
    findCase.addEventListener("click", function () {
      findCase.classList.toggle("active");
      triggerSearch();
    });
  }
  if (findWord) {
    findWord.addEventListener("click", function () {
      findWord.classList.toggle("active");
      triggerSearch();
    });
  }
  if (gotoInput) {
    gotoInput.addEventListener("input", function () {
      const digits = gotoInput.value.replace(/[^0-9]/g, "");
      if (digits !== gotoInput.value) {
        gotoInput.value = digits;
      }
    });
    gotoInput.addEventListener("keydown", function (event) {
      if (event.key === "Enter") {
        event.preventDefault();
        if (goToLine(gotoInput.value)) {
          hideGotoBar();
        }
      } else if (event.key === "Escape") {
        event.preventDefault();
        event.stopPropagation();
        hideGotoBar();
      }
    });
  }
  if (gotoClose) {
    gotoClose.addEventListener("click", function () {
      hideGotoBar();
    });
  }
  document.addEventListener("copy", function (event) {
    const tag = event.target && event.target.tagName;
    if (tag === "INPUT" || tag === "TEXTAREA") {
      return;
    }
    if (selectAllActive && event.clipboardData) {
      // Ctrl+C or the context menu after Select All: give the host the whole text.
      event.preventDefault();
      postHost("salamander-copy-all");
    }
  });
  window.addEventListener("mousedown", function () {
    selectAllActive = false;
  }, true);
  window.addEventListener("keydown", function (event) {
    const inInput = event.target && event.target.tagName === "INPUT";
    if (event.ctrlKey && !event.altKey && (event.key === "a" || event.key === "A") && !inInput) {
      event.preventDefault();
      selectWholeDocument();
      return;
    }
    if (!event.ctrlKey && !event.metaKey && !event.altKey &&
        event.key !== "Shift" && event.key !== "Control" && event.key !== "Alt" && !inInput) {
      selectAllActive = false;
    }
    if (event.ctrlKey && (event.key === "f" || event.key === "F")) {
      event.preventDefault();
      showFindBar(true);
    } else if (event.key === "F3") {
      event.preventDefault();
      triggerNextMatch(!event.shiftKey);
    } else if (event.key === "Escape" && isGotoBarOpen) {
      event.preventDefault();
      hideGotoBar();
    } else if (event.key === "Escape" && isFindBarOpen) {
      event.preventDefault();
      hideFindBar();
    } else if (event.key === "Escape" && !event.repeat) {
      // No bar open (and not a held key that just closed a bar): the host does not intercept Esc for this viewer, so ask it to close.
      postHost("salamander-close");
    }
  });

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
    const epoch = documentEpoch;
    handshakeTimer = window.setTimeout(function () {
      if (documentEpoch === epoch && !settings) {
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
