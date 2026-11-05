/* Prism Sharp–compatible Find bar for WebView2 document viewers.
 * Same chrome as virtual-viewer.html / virtual-viewer.js openFindBar.
 * Live match paint uses CSS Custom Highlight API (viewer-find-match), so
 * typing keeps focus in #find-input just like Prism Text Viewer. */
(function () {
  "use strict";

  const FIND_HIGHLIGHT_NAME = "viewer-find-match";

  function host() {
    return window.chrome && window.chrome.webview ? window.chrome.webview : null;
  }

  function postHost(message) {
    const webview = host();
    if (webview) {
      webview.postMessage(message);
    }
  }

  // Persist across accidental re-injects of this file (ExecuteScript). Fresh
  // `let` state on every inject was resetting F3 back to the first match.
  function runtime() {
    if (!window.__salamanderFindBarRuntime) {
      window.__salamanderFindBarRuntime = {
        isFindBarOpen: false,
        matchIndex: 0,
        matchTotal: 0,
        lastQuery: "",
        listenersBound: false,
        findHighlightRange: null,
        cachedMatchRanges: [],
      };
    }
    return window.__salamanderFindBarRuntime;
  }

  function ensureStyle() {
    let style = document.getElementById("salamander-document-find-style");
    if (!style) {
      style = document.createElement("style");
      style.id = "salamander-document-find-style";
      (document.head || document.documentElement).appendChild(style);
    }
    style.textContent =
      ".find-bar{position:fixed;top:58px;right:18px;z-index:2147483647;display:flex;align-items:center;gap:3px;" +
      "background:var(--viewer-gutter-background,#f5f5f5);color:var(--viewer-foreground,CanvasText);" +
      "border:1px solid var(--viewer-gutter-foreground,#606060);border-radius:4px;padding:3px 5px;" +
      "box-shadow:0 4px 12px rgba(0,0,0,.25);font-family:\"MS Shell Dlg\",Tahoma,Arial,sans-serif;font-size:12px}" +
      ".find-bar.hidden{display:none !important}" +
      "#find-input{width:170px;height:22px;padding:1px 5px;background:var(--viewer-background,Canvas);" +
      "color:var(--viewer-foreground,CanvasText);border:1px solid var(--viewer-gutter-foreground,#606060);" +
      "border-radius:2px;font-size:12px;font-family:inherit;outline:none}" +
      "#find-input:focus{border-color:var(--viewer-selected-background,#316ac5)}" +
      "#find-input.no-results{border-color:#d9534f;background-color:rgba(217,83,79,.18)}" +
      ".find-count{min-width:54px;text-align:center;font-size:11px;color:var(--viewer-gutter-foreground,#606060);user-select:none}" +
      ".find-btn{display:inline-flex;align-items:center;justify-content:center;min-width:22px;height:22px;padding:0 3px;" +
      "background:transparent;color:var(--viewer-foreground,CanvasText);border:1px solid transparent;border-radius:2px;" +
      "cursor:pointer;font-size:11px;user-select:none}" +
      ".find-btn:hover{background:var(--viewer-background,Canvas);border-color:var(--viewer-gutter-foreground,#606060)}" +
      ".find-toggle.active{background:var(--viewer-selected-background,#316ac5);color:var(--viewer-selected-foreground,#fff);" +
      "border-color:var(--viewer-selected-background,#316ac5)}" +
      /* Same paint as Prism Sharp virtual-viewer.html ::highlight(viewer-find-match). */
      "::highlight(viewer-find-match){color:var(--viewer-selected-foreground,#fff);" +
      "background:var(--viewer-selected-background,#316ac5)}";
  }

  function wireOnce(el, type, handler, key) {
    if (!el || el[key]) {
      return;
    }
    el[key] = true;
    el.addEventListener(type, handler);
  }

  function refs() {
    return {
      findBar: document.getElementById("viewer-find-bar"),
      findInput: document.getElementById("find-input"),
      findCount: document.getElementById("find-count"),
      findCase: document.getElementById("find-case"),
      findWord: document.getElementById("find-word"),
      findPrev: document.getElementById("find-prev"),
      findNext: document.getElementById("find-next"),
      findClose: document.getElementById("find-close"),
    };
  }

  function ensureBar() {
    ensureStyle();
    let findBar = document.getElementById("viewer-find-bar");
    if (!findBar) {
      findBar = document.createElement("div");
      findBar.id = "viewer-find-bar";
      findBar.className = "find-bar hidden";
      findBar.setAttribute("role", "search");
      findBar.innerHTML =
        '<input type="text" id="find-input" placeholder="Find..." spellcheck="false" autocomplete="off" />' +
        '<span id="find-count" class="find-count"></span>' +
        '<button type="button" id="find-case" class="find-btn find-toggle" title="Match Case (Alt+C)">Aa</button>' +
        '<button type="button" id="find-word" class="find-btn find-toggle" title="Match Whole Word (Alt+W)">\\b</button>' +
        '<button type="button" id="find-prev" class="find-btn" title="Previous Match (Shift+F3)">▲</button>' +
        '<button type="button" id="find-next" class="find-btn" title="Next Match (F3 or Enter)">▼</button>' +
        '<button type="button" id="find-close" class="find-btn" title="Close (Escape)">✕</button>';
      (document.documentElement || document.body).appendChild(findBar);
      runtime().listenersBound = false;
    } else if (!findBar.parentNode) {
      (document.documentElement || document.body).appendChild(findBar);
    }
    bindListeners();
    return refs();
  }

  function showFindBar(focusInput) {
    const ui = ensureBar();
    ui.findBar.classList.remove("hidden");
    ui.findBar.style.display = "flex";
    runtime().isFindBarOpen = true;
    if (focusInput && ui.findInput) {
      ui.findInput.focus();
      ui.findInput.select();
    }
  }

  function setFindHighlight(range) {
    if (typeof CSS === "undefined" || !CSS.highlights || typeof Highlight === "undefined") {
      return false;
    }
    CSS.highlights.set(FIND_HIGHLIGHT_NAME, new Highlight(range));
    runtime().findHighlightRange = range;
    return true;
  }

  function clearFindHighlight() {
    runtime().findHighlightRange = null;
    if (typeof CSS !== "undefined" && CSS.highlights) {
      CSS.highlights.delete(FIND_HIGHLIGHT_NAME);
    }
  }

  function rangeClientRect(range) {
    if (!range) {
      return null;
    }
    let rect = range.getBoundingClientRect ? range.getBoundingClientRect() : null;
    if (rect && (rect.width > 0 || rect.height > 0 || rect.top !== 0 || rect.bottom !== 0)) {
      return rect;
    }
    const rects = range.getClientRects ? range.getClientRects() : null;
    if (rects && rects.length) {
      return rects[0];
    }
    return rect;
  }

  function isVerticallyScrollable(el) {
    if (!el || el.nodeType !== Node.ELEMENT_NODE) {
      return false;
    }
    const style = window.getComputedStyle(el);
    const overflowY = style.overflowY;
    if (overflowY !== "auto" && overflowY !== "scroll" && overflowY !== "overlay") {
      return false;
    }
    return el.scrollHeight > el.clientHeight + 1;
  }

  function scrollParentsFor(node) {
    const parents = [];
    let el = node && node.nodeType === Node.TEXT_NODE ? node.parentElement : node;
    while (el && el !== document.documentElement) {
      if (isVerticallyScrollable(el)) {
        parents.push(el);
      }
      el = el.parentElement;
    }
    const root = document.scrollingElement || document.documentElement;
    if (root && parents.indexOf(root) < 0 && root.scrollHeight > root.clientHeight + 1) {
      parents.push(root);
    }
    return parents;
  }

  function centerRange(range) {
    if (!range) {
      return;
    }
    // JSON Viewer (and other document pages) often scroll inside nested
    // overflow:auto panels, not the window. scrollIntoView walks those.
    try {
      let node = range.startContainer;
      if (node && node.nodeType === Node.TEXT_NODE) {
        node = node.parentElement;
      }
      if (node && typeof node.scrollIntoView === "function") {
        node.scrollIntoView({ block: "center", inline: "nearest", behavior: "auto" });
      }
    } catch (_) {
      /* continue with manual parent scroll */
    }

    const parents = scrollParentsFor(range.startContainer);
    for (let i = 0; i < parents.length; i += 1) {
      const parent = parents[i];
      const rect = rangeClientRect(range);
      if (!rect) {
        break;
      }
      if (
        parent === document.scrollingElement ||
        parent === document.documentElement ||
        parent === document.body
      ) {
        const viewHeight = window.innerHeight || document.documentElement.clientHeight || 0;
        const scrollTop = window.scrollY || document.documentElement.scrollTop || 0;
        window.scrollTo({
          top: Math.max(0, scrollTop + rect.top + rect.height / 2 - viewHeight / 2),
          behavior: "auto",
        });
        continue;
      }
      const parentRect = parent.getBoundingClientRect();
      parent.scrollTop = Math.max(
        0,
        parent.scrollTop + (rect.top - parentRect.top) - parent.clientHeight / 2 + rect.height / 2,
      );
    }
  }

  function hideFindBar() {
    const ui = ensureBar();
    // Leaving the find bar turns the highlighted match into a real selection
    // so it can be copied, matching Prism Sharp hideFindBar.
    const range = runtime().findHighlightRange;
    clearFindHighlight();
    ui.findBar.classList.add("hidden");
    ui.findBar.style.display = "none";
    runtime().isFindBarOpen = false;
    window.focus();
    if (range && !range.collapsed) {
      try {
        const sel = window.getSelection();
        if (sel) {
          sel.removeAllRanges();
          sel.addRange(range);
        }
      } catch (_) {
        /* ignore stale ranges after DOM updates */
      }
    }
  }

  function openFindBar(message) {
    const ui = ensureBar();
    message = message || {};
    const query = typeof message.query === "string" ? message.query : "";
    if (runtime().isFindBarOpen || !query || !ui.findInput) {
      showFindBar(true);
      return;
    }
    ui.findInput.value = query;
    if (ui.findCase) {
      ui.findCase.classList.toggle("active", Boolean(message.matchCase));
    }
    if (ui.findWord) {
      ui.findWord.classList.toggle("active", Boolean(message.wholeWord));
    }
    showFindBar(true);
    triggerSearch();
  }

  function remember(query, matchCase, wholeWord) {
    postHost(
      "salamander-find:" +
        JSON.stringify({
          action: "remember",
          query: query,
          matchCase: !!matchCase,
          wholeWord: !!wholeWord,
        }),
    );
  }

  function isWordChar(ch) {
    return /[A-Za-z0-9_]/.test(ch);
  }

  function isSearchableTextNode(node) {
    if (!node || !node.nodeValue) {
      return false;
    }
    const findBar = document.getElementById("viewer-find-bar");
    if (findBar && findBar.contains(node)) {
      return false;
    }
    const parent = node.parentElement;
    if (!parent) {
      return true;
    }
    const tag = parent.tagName;
    if (tag === "SCRIPT" || tag === "STYLE" || tag === "NOSCRIPT") {
      return false;
    }
    if (parent.closest && parent.closest("script,style,noscript,#viewer-find-bar")) {
      return false;
    }
    return true;
  }

  function collectTextMap() {
    const root = document.body || document.documentElement;
    const pieces = [];
    let haystack = "";
    if (!root) {
      return { haystack: haystack, pieces: pieces };
    }
    const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT, {
      acceptNode: function (node) {
        return isSearchableTextNode(node) ? NodeFilter.FILTER_ACCEPT : NodeFilter.FILTER_REJECT;
      },
    });
    let node;
    while ((node = walker.nextNode())) {
      const text = node.nodeValue;
      pieces.push({ node: node, start: haystack.length, length: text.length });
      haystack += text;
    }
    return { haystack: haystack, pieces: pieces };
  }

  function rangeFromOffset(pieces, start, length) {
    if (!pieces.length || length <= 0) {
      return null;
    }
    const end = start + length;
    let startNode = null;
    let startOffset = 0;
    let endNode = null;
    let endOffset = 0;
    for (let i = 0; i < pieces.length; i += 1) {
      const piece = pieces[i];
      const pieceEnd = piece.start + piece.length;
      if (!startNode && start >= piece.start && start < pieceEnd) {
        startNode = piece.node;
        startOffset = start - piece.start;
      }
      if (start === pieceEnd && i + 1 < pieces.length && !startNode) {
        continue;
      }
      if (!endNode && end > piece.start && end <= pieceEnd) {
        endNode = piece.node;
        endOffset = end - piece.start;
      }
      if (startNode && endNode) {
        break;
      }
    }
    if (!startNode) {
      return null;
    }
    if (!endNode) {
      endNode = startNode;
      endOffset = Math.min(startNode.nodeValue.length, startOffset + length);
    }
    try {
      const range = document.createRange();
      range.setStart(startNode, startOffset);
      range.setEnd(endNode, endOffset);
      return range;
    } catch (_) {
      return null;
    }
  }

  function collectMatchRanges(query, matchCase, wholeWord) {
    const ranges = [];
    if (!query) {
      return ranges;
    }
    const map = collectTextMap();
    const haystack = map.haystack;
    if (!haystack) {
      return ranges;
    }
    const source = matchCase ? haystack : haystack.toLowerCase();
    const needle = matchCase ? query : query.toLowerCase();
    let pos = 0;
    while (pos <= source.length - needle.length) {
      const found = source.indexOf(needle, pos);
      if (found < 0) {
        break;
      }
      if (wholeWord) {
        const before = found === 0 ? "" : haystack.charAt(found - 1);
        const after =
          found + query.length >= haystack.length ? "" : haystack.charAt(found + query.length);
        if ((before && isWordChar(before)) || (after && isWordChar(after))) {
          pos = found + 1;
          continue;
        }
      }
      const range = rangeFromOffset(map.pieces, found, query.length);
      if (range) {
        ranges.push(range);
      }
      pos = found + Math.max(1, needle.length);
    }
    return ranges;
  }

  function updateCount() {
    const ui = ensureBar();
    const rt = runtime();
    if (!ui.findCount) {
      return;
    }
    if (!rt.lastQuery || rt.matchTotal === 0) {
      ui.findCount.textContent = rt.lastQuery ? "0 of 0" : "";
      if (ui.findInput) {
        ui.findInput.classList.toggle("no-results", Boolean(rt.lastQuery));
      }
      return;
    }
    if (ui.findInput) {
      ui.findInput.classList.remove("no-results");
    }
    ui.findCount.textContent = rt.matchIndex + " of " + rt.matchTotal;
  }

  function restoreFindInputFocus(caret) {
    const ui = ensureBar();
    if (!ui.findInput || !runtime().isFindBarOpen) {
      return;
    }
    try {
      ui.findInput.focus({ preventScroll: true });
    } catch (_) {
      ui.findInput.focus();
    }
    if (typeof caret === "number" && ui.findInput.setSelectionRange) {
      const length = ui.findInput.value.length;
      const pos = Math.max(0, Math.min(caret, length));
      ui.findInput.setSelectionRange(pos, pos);
    }
  }

  function paintCurrentMatch(scrollIntoView) {
    const rt = runtime();
    if (rt.matchTotal <= 0 || rt.matchIndex <= 0 || rt.matchIndex > rt.cachedMatchRanges.length) {
      clearFindHighlight();
      return false;
    }
    const range = rt.cachedMatchRanges[rt.matchIndex - 1];
    if (!range) {
      clearFindHighlight();
      return false;
    }
    const findBar = document.getElementById("viewer-find-bar");
    const typing =
      rt.isFindBarOpen &&
      document.activeElement &&
      findBar &&
      findBar.contains(document.activeElement);
    const highlighted = setFindHighlight(range);
    if (!highlighted && !typing) {
      try {
        const sel = window.getSelection();
        if (sel) {
          sel.removeAllRanges();
          sel.addRange(range);
        }
      } catch (_) {
        /* ignore */
      }
    }
    if (scrollIntoView) {
      centerRange(range);
    }
    return highlighted;
  }

  function triggerSearch() {
    const ui = ensureBar();
    if (!ui.findInput) {
      return;
    }
    const rt = runtime();
    const caret =
      typeof ui.findInput.selectionStart === "number"
        ? ui.findInput.selectionStart
        : ui.findInput.value.length;
    const query = ui.findInput.value;
    const matchCase = ui.findCase ? ui.findCase.classList.contains("active") : false;
    const wholeWord = ui.findWord ? ui.findWord.classList.contains("active") : false;
    rt.lastQuery = query;
    remember(query, matchCase, wholeWord);
    rt.cachedMatchRanges = collectMatchRanges(query, matchCase, wholeWord);
    rt.matchTotal = rt.cachedMatchRanges.length;
    rt.matchIndex = rt.matchTotal > 0 ? 1 : 0;
    updateCount();
    // Live highlight like Prism Sharp: paint the current match while typing
    // via CSS.highlights so focus stays in #find-input.
    if (query && rt.matchTotal > 0) {
      paintCurrentMatch(true);
    } else {
      clearFindHighlight();
    }
    restoreFindInputFocus(caret);
  }

  function triggerNextMatch(forward) {
    const ui = ensureBar();
    if (!ui.findInput) {
      return;
    }
    const rt = runtime();
    const query = ui.findInput.value || rt.lastQuery;
    if (!query) {
      showFindBar(true);
      return;
    }
    if (!rt.isFindBarOpen) {
      showFindBar(false);
    }
    const matchCase = ui.findCase ? ui.findCase.classList.contains("active") : false;
    const wholeWord = ui.findWord ? ui.findWord.classList.contains("active") : false;
    rt.lastQuery = query;
    remember(query, matchCase, wholeWord);
    if (rt.matchTotal <= 0 || rt.cachedMatchRanges.length === 0) {
      rt.cachedMatchRanges = collectMatchRanges(query, matchCase, wholeWord);
      rt.matchTotal = rt.cachedMatchRanges.length;
      rt.matchIndex = 0;
    }
    if (rt.matchTotal > 0) {
      if (forward) {
        rt.matchIndex = rt.matchIndex >= rt.matchTotal ? 1 : rt.matchIndex + 1;
      } else {
        rt.matchIndex = rt.matchIndex <= 1 ? rt.matchTotal : rt.matchIndex - 1;
      }
      paintCurrentMatch(true);
    } else {
      clearFindHighlight();
    }
    updateCount();
    restoreFindInputFocus();
  }

  function bindListeners() {
    const rt = runtime();
    if (rt.listenersBound) {
      return;
    }
    const ui = refs();
    if (!ui.findInput) {
      return;
    }
    rt.listenersBound = true;
    wireOnce(ui.findInput, "input", function () {
      triggerSearch();
    }, "__salFindInput");
    wireOnce(
      ui.findInput,
      "keydown",
      function (event) {
        if (event.key === "Enter") {
          event.preventDefault();
          triggerNextMatch(!event.shiftKey);
        } else if (event.key === "Escape") {
          event.preventDefault();
          event.stopPropagation();
          hideFindBar();
        }
      },
      "__salFindKey",
    );
    wireOnce(ui.findNext, "click", function () {
      triggerNextMatch(true);
    }, "__salFindNext");
    wireOnce(ui.findPrev, "click", function () {
      triggerNextMatch(false);
    }, "__salFindPrev");
    wireOnce(ui.findClose, "click", function () {
      hideFindBar();
    }, "__salFindClose");
    wireOnce(ui.findCase, "click", function () {
      ui.findCase.classList.toggle("active");
      triggerSearch();
    }, "__salFindCase");
    wireOnce(ui.findWord, "click", function () {
      ui.findWord.classList.toggle("active");
      triggerSearch();
    }, "__salFindWord");
  }

  if (!window.__salamanderDocumentFindKeys) {
    window.__salamanderDocumentFindKeys = true;
    window.addEventListener("keydown", function (event) {
      const inInput = event.target && event.target.tagName === "INPUT";
      if (event.ctrlKey && (event.key === "f" || event.key === "F")) {
        event.preventDefault();
        showFindBar(true);
      } else if (event.key === "F3") {
        // Host AcceleratorKeyPressed already owns F3 (Handled=TRUE). Ignore
        // a duplicate page delivery so we do not advance twice.
        if (event.defaultPrevented) {
          return;
        }
        event.preventDefault();
        triggerNextMatch(!event.shiftKey);
      } else if (event.key === "Escape") {
        // Host leaves Esc to the page for RenderDocument (same as Prism Text).
        // Close Find first; only ask the host to close the viewer when Find is down.
        event.preventDefault();
        if (runtime().isFindBarOpen) {
          hideFindBar();
        } else {
          postHost("salamander-close");
        }
      } else if (event.altKey && !event.ctrlKey && !event.metaKey && inInput && runtime().isFindBarOpen) {
        const ui = ensureBar();
        if (event.key === "c" || event.key === "C") {
          event.preventDefault();
          if (ui.findCase) {
            ui.findCase.classList.toggle("active");
            triggerSearch();
          }
        } else if (event.key === "w" || event.key === "W") {
          event.preventDefault();
          if (ui.findWord) {
            ui.findWord.classList.toggle("active");
            triggerSearch();
          }
        }
      }
    });
  }

  function onHostMessage(event) {
    let message = event && event.data;
    if (typeof message === "string") {
      try {
        message = JSON.parse(message);
      } catch (_) {
        return;
      }
    }
    if (!message || typeof message !== "object") {
      return;
    }
    if (message.type === "openFind") {
      openFindBar(message);
    } else if (message.type === "findNext") {
      triggerNextMatch(true);
    } else if (message.type === "findPrev") {
      triggerNextMatch(false);
    }
  }

  if (!window.__salamanderDocumentFindHost) {
    window.__salamanderDocumentFindHost = true;
    const webview = host();
    if (webview) {
      webview.addEventListener("message", onHostMessage);
    }
  }

  window.__salamanderOpenFind = openFindBar;
  window.__salamanderHideFind = hideFindBar;
  window.__salamanderFindNext = function () {
    triggerNextMatch(true);
  };
  window.__salamanderFindPrev = function () {
    triggerNextMatch(false);
  };

  ensureBar();
})();
