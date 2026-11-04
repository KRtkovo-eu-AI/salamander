// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

"use strict";

// Run the complete production frontend. DOM layout and asynchronous delivery are
// controlled so callbacks queued before reset can deliberately arrive afterward.
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const { execFileSync } = require("child_process");
const root = path.resolve(__dirname, "../..");
const relative = "src/plugins/shared/webviewviewer/prism/viewer/virtual-viewer.js";
const revision = process.argv[2] === "--baseline-ref" ? process.argv[3] : null;
const source = revision
  ? execFileSync("git", ["show", `${revision}:${relative}`], { cwd: root, encoding: "utf8", timeout: 10000 })
  : fs.readFileSync(path.join(root, relative), "utf8");
let checks = 0;
function check(condition, message) {
  checks += 1;
  if (!condition) throw new Error(message);
}

class Element {
  constructor(tag) {
    this.tag = tag;
    this.children = [];
    this.parentNode = null;
    this.dataset = {};
    this.attributes = {};
    this.style = { setProperty() {} };
    const classes = new Set();
    this.classList = {
      add(name) { classes.add(name); },
      remove(name) { classes.delete(name); },
      toggle(name, force) { if (force === undefined ? !classes.has(name) : force) classes.add(name); else classes.delete(name); },
      contains(name) { return classes.has(name); }
    };
    this.value = "";
    this.innerHTML = this.textContent = "";
    this.clientWidth = 800;
    this.offsetTop = 0;
  }
  get isConnected() { return this.tag === "body" || Boolean(this.parentNode && this.parentNode.isConnected); }
  appendChild(child) { child.parentNode = this; this.children.push(child); return child; }
  replaceChildren(...children) {
    this.children.forEach(child => { child.parentNode = null; });
    this.children = [];
    children.forEach(child => this.appendChild(child));
  }
  setAttribute(name, value) { this.attributes[name] = value; }
  removeAttribute(name) { delete this.attributes[name]; }
  remove() {
    if (this.parentNode) this.parentNode.children = this.parentNode.children.filter(child => child !== this);
    this.parentNode = null;
  }
  querySelector(tag) {
    for (const child of this.children) {
      if (child.tag === tag) return child;
      const found = child.querySelector(tag);
      if (found) return found;
    }
    return null;
  }
  querySelectorAll() { return []; }
  getBoundingClientRect() { return { top: 0, bottom: 32, height: 32 }; }
  addEventListener(type, handler) { (this.listeners = this.listeners || {})[type] = [...((this.listeners || {})[type] || []), handler]; }
  fire(type, event) { ((this.listeners || {})[type] || []).forEach(handler => handler(event)); }
  focus() {}
  select() {}
  contains(node) { return node === this || this.children.some(child => child.contains(node)); }
  text() { return this.innerHTML + this.textContent + this.children.map(child => child.text()).join(""); }
}

function frontend() {
  let id = 0;
  let receive;
  const timers = new Map(), frames = new Map(), messages = [], workers = [], observers = [], loads = [];
  const body = new Element("body"), spacer = new Element("main"), status = new Element("div");
  const named = new Map();  // find bar controls and any other id the page looks up
  const findBar = new Element("div");
  named.set("viewer-find-bar", findBar);
  body.appendChild(spacer); body.appendChild(status);
  const selection = {
    rangeCount: 0, isCollapsed: true, text: "",
    removeAllRanges() { this.rangeCount = 0; this.isCollapsed = true; },
    addRange() { this.rangeCount = 1; this.isCollapsed = false; },
    toString() { return this.text; }
  };
  const windowListeners = {}, documentListeners = {};
  const context = {
    console: { log() {}, warn() {}, error() {} }, URL,
    document: {
      readyState: "complete", baseURI: "https://prism.example/viewer/virtual-viewer.html",
      body, documentElement: { style: { setProperty() {} }, clientHeight: 600, scrollTop: 0 },
      getElementById(name) {
        if (name === "virtual-spacer") return spacer;
        if (name === "viewer-status") return status;
        if (!named.has(name)) named.set(name, new Element("div"));
        return named.get(name);
      },
      createElement(tag) { return new Element(tag); },
      createRange() { return { selectNodeContents() {} }; },
      getSelection() { return selection; },
      addEventListener(type, handler) { (documentListeners[type] = documentListeners[type] || []).push(handler); }
    },
    chrome: { webview: { postMessage(value) { messages.push(value); }, addEventListener(type, handler) { if (type === "message") receive = handler; } } },
    setTimeout(fn, delay) { const key = ++id; timers.set(key, { fn, delay }); return key; },
    clearTimeout(key) { timers.delete(key); },
    requestAnimationFrame(fn) { const key = ++id; frames.set(key, fn); return key; },
    cancelAnimationFrame(key) { frames.delete(key); },
    addEventListener(type, handler) { (windowListeners[type] = windowListeners[type] || []).push(handler); },
    focus() {}, innerHeight: 600, scrollY: 0, getSelection() { return selection; },
    scrollTo(x, y) { this.scrollY = typeof x === "object" ? x.top : y; },
    getComputedStyle() { return { color: "rgb(255,255,255)", lineHeight: "16px" }; },
    Prism: {
      languages: {}, highlight(text) { return `<span class="token">${text}</span>`; },
      plugins: { autoloader: { loadLanguages(languages, success, failure) { loads.push({ languages, success, failure }); } } }
    }
  };
  context.Worker = class {
    constructor() {
      if (context.failWorkerCreation) throw new Error("simulated unavailable worker");
      this.handlers = {}; this.requests = []; this.terminated = false; workers.push(this);
    }
    addEventListener(type, handler) { this.handlers[type] = handler; }
    postMessage(message) { this.requests.push(message); }
    terminate() { this.terminated = true; }
    deliver(request, html) { this.handlers.message({ data: { ...request, type: "highlighted", ok: true, html } }); }
    fail() { this.handlers.error({ message: "simulated worker failure" }); }
  };
  context.IntersectionObserver = class {
    constructor(callback) { this.callback = callback; this.disconnected = false; observers.push(this); }
    observe() {}
    disconnect() { this.disconnected = true; }
  };
  context.window = context;
  vm.createContext(context);
  vm.runInContext(source, context, { timeout: 1000 });
  function send(message) { receive({ data: message }); }
  function init(generation, extra = {}) { send({ type: "init", generation, lineCount: 1, chunkLines: 80, ...extra }); }
  function chunk(generation, text) { send({ type: "chunk", generation, startLine: 0, lineCount: 1, text }); }
  function frame() {
    const batch = Array.from(frames.entries());
    frames.clear();
    batch.forEach(([, fn]) => fn());
  }
  function ready() { for (let i = 0; i < 5; ++i) frame(); }
  function queued() { return [...Array.from(timers.values(), timer => timer.fn), ...frames.values()]; }
  return { context, send, init, chunk, frame, ready, queued, timers, frames, messages, workers, observers, loads, spacer, selection, named, windowListeners, documentListeners };
}

function testResetAndStaleDelivery() {
  const f = frontend();
  check(f.messages.includes("salamander-virtual-ready"), "the initial page handshake is retained");
  f.init(1);
  const delayed = f.queued();
  f.chunk(1, "OLD FILE");
  delayed.push(...f.queued());
  f.frame();
  delayed.push(...f.queued());
  const oldRequest = f.workers[0].requests[0];
  f.workers[0].deliver(oldRequest, "OLD HIGHLIGHT");
  delayed.push(...f.queued());
  const oldObserver = f.observers[0];
  const oldSlot = f.spacer.children[0];
  f.selection.rangeCount = 1; f.selection.isCollapsed = false;
  f.context.scrollY = 700;
  f.send({ type: "reset" });
  check(f.spacer.children.length === 0, "reset removes all old document content");
  check(f.selection.rangeCount === 0 && f.context.scrollY === 0, "reset clears selection and scroll position");
  check(oldObserver.disconnected, "reset disconnects the old intersection observer");
  check(f.timers.size === 0 && f.frames.size === 0, "reset cancels document timers and animation frames");
  check(f.workers.length === 1 && !f.workers[0].terminated, "idle reset preserves the loaded worker");
  f.messages.length = 0;
  delayed.forEach(fn => fn());
  f.workers[0].deliver(oldRequest, "OLD HIGHLIGHT");
  check(f.spacer.children.length === 0 && f.messages.length === 0, "late callbacks cannot paint or announce an idle document");
  f.init(2);
  f.messages.length = 0;
  delayed.forEach(fn => fn());
  oldObserver.callback([{ target: oldSlot, isIntersecting: true }]);
  f.workers[0].deliver(oldRequest, "OLD HIGHLIGHT");
  f.chunk(1, "OLD CHUNK");
  check(!f.spacer.text().includes("OLD"), "late old-generation chunks and highlights cannot appear in the new document");
  check(!f.messages.some(value => value.startsWith("salamander-prism-ready")), "old ready callbacks cannot mark the new generation ready");
  f.chunk(2, "NEW FILE"); f.ready();
  check(f.spacer.text().includes("NEW FILE") && !f.spacer.text().includes("OLD"), "reused page displays only the new file");
  check(f.messages.filter(value => value === "salamander-prism-ready:2").length === 1, "new document announces its generation once after paint");
  check(!f.messages.includes("salamander-prism-ready:1"), "old ready notification is never delivered after reset");
  f.send({ type: "reset" });
  f.init(3, { lineCount: 0 });
  Array.from(f.timers.values()).filter(timer => timer.delay === 400).forEach(timer => timer.fn());
  f.ready();
  check(f.spacer.children.length === 0 && f.messages.includes("salamander-prism-ready:3"), "empty file also becomes ready without old content");
  check(f.messages.includes("salamander-prism-theme-ready:3"), "theme readiness carries the new generation");
}

function testFindStateIsClearedOnReset() {
  const f = frontend();
  const input = f.named.get("find-input"), count = f.named.get("find-count"), bar = f.named.get("viewer-find-bar");
  f.init(40, { searchPhrase: "needle", searchCaseSensitive: true });
  check(input.value === "needle" && !bar.classList.contains("hidden"), "an external search phrase opens the find bar");
  count.textContent = "1 of 3"; input.classList.add("no-results");
  f.send({ type: "reset" });
  check(input.value === "" && count.textContent === "" && !input.classList.contains("no-results"),
        "reset clears the previous query, match count and no-results state");
  check(bar.classList.contains("hidden"), "reset hides the find bar for the next document");
  f.init(41);
  check(input.value === "" && bar.classList.contains("hidden"), "a reused page without a search phrase starts with a closed, empty find bar");
}

function testGoToLineBar() {
  const f = frontend();
  const bar = f.named.get("viewer-goto-bar"), input = f.named.get("goto-input");
  f.init(50, { lineCount: 500 });
  f.send({ type: "openGoto", label: "Go to Line", total: 500 });
  check(!bar.classList.contains("hidden"), "openGoto from the host shows the go to line bar");
  check(input.placeholder === "Go to Line (1-500)", "the placeholder carries the localized label and the line count");
  input.value = "1";
  input.fire("keydown", { key: "Enter", preventDefault() {} });
  check(bar.classList.contains("hidden") && f.context.scrollY === 0, "Enter on line 1 scrolls to the top and closes the bar");
  f.send({ type: "openGoto", label: "Go to Line", total: 500 });
  input.value = "99999";
  input.fire("keydown", { key: "Enter", preventDefault() {} });
  check(bar.classList.contains("hidden") && f.context.scrollY > 0, "a line past the end is clamped to the last line and scrolled to");
  f.send({ type: "openGoto", label: "Go to Line", total: 500 });
  input.value = "";
  input.fire("keydown", { key: "Enter", preventDefault() {} });
  check(!bar.classList.contains("hidden"), "an empty entry keeps the bar open");
  input.fire("keydown", { key: "Escape", preventDefault() {}, stopPropagation() {} });
  check(bar.classList.contains("hidden"), "Escape closes the go to line bar");
}

function testSelectAllAndCopy() {
  const f = frontend();
  f.init(60);
  f.messages.length = 0;
  f.send({ type: "selectAll" });
  check(f.selection.rangeCount === 1, "Select All selects the document container");
  f.send({ type: "copy" });
  check(f.messages.includes("salamander-copy-all"), "Copy after Select All asks the host for the whole document");
  f.messages.length = 0;
  let prevented = false;
  f.documentListeners.copy.forEach(handler => handler({ clipboardData: {}, preventDefault() { prevented = true; } }));
  check(prevented && f.messages.includes("salamander-copy-all"), "Ctrl+C after Select All is redirected to the whole-document copy");
  f.windowListeners.mousedown.forEach(handler => handler({}));
  f.messages.length = 0;
  f.selection.text = "picked text";
  f.send({ type: "copy" });
  check(f.messages.includes("salamander-copy-text:picked text") && !f.messages.includes("salamander-copy-all"),
        "after a new selection Copy sends only the selected text");
}

async function testDelayedFallback() {
  const f = frontend();
  f.init(10, { language: "ruby" }); f.chunk(10, "OLD FALLBACK");
  f.workers[0].fail();
  check(f.loads.length === 1, "worker failure starts the language fallback");
  f.context.failWorkerCreation = true;
  f.send({ type: "reset" }); f.init(11, { language: "ruby" }); f.chunk(11, "NEW FALLBACK");
  check(f.loads.length === 2, "reset gives a new document its own language-load promise");
  f.context.Prism.languages.ruby = {};
  f.loads[0].success(); await Promise.resolve();
  check(!f.spacer.text().includes("OLD"), "old fallback promise cannot repaint after reinitialization");
  f.loads[1].success(); await Promise.resolve(); f.ready();
  check(f.spacer.text().includes("NEW FALLBACK") && f.spacer.text().includes('class="token"'), "new fallback promise still highlights the new file");
  check(f.messages.includes("salamander-prism-ready:11"), "fallback rendering announces the current generation");
  f.send({ type: "reset" }); f.init(12, { language: "python" }); f.chunk(12, "PYTHON");
  f.loads[2].failure(); await Promise.resolve();
  f.send({ type: "reset" }); f.init(13, { language: "python" }); f.chunk(13, "RETRY");
  check(f.loads.length === 4, "failed language loads can retry in a later document");
  f.send({ type: "reset" });
  f.loads[3].success(); await Promise.resolve();
  check(f.spacer.children.length === 0, "a pending fallback also stays inert while the page is idle");
}

function testWhitespaceTransition() {
  const f = frontend();
  f.init(20, { showWhitespace: true }); f.chunk(20, "spaces here");
  const oldWorker = f.workers[0], oldRequest = oldWorker.requests[0];
  oldWorker.deliver(oldRequest, "spaces highlighted");
  f.send({ type: "reset" }); f.init(21, { showWhitespace: false }); f.chunk(21, "normal text");
  check(oldWorker.terminated && f.workers.length === 2, "disabling whitespace after reset replaces the modified worker grammar");
  oldWorker.deliver(oldRequest, "OLD INVISIBLES");
  check(!f.spacer.text().includes("OLD"), "terminated worker callbacks are ignored");
  f.ready();
  check(f.messages.includes("salamander-prism-ready:21"), "whitespace transition still renders the new document");
}

function testBusyWorkerReset() {
  const f = frontend();
  f.init(30); f.chunk(30, "expensive old input");
  const oldWorker = f.workers[0], oldRequest = oldWorker.requests[0];
  Array.from(f.timers.values()).filter(timer => timer.delay === 5000).forEach(timer => timer.fn());
  f.send({ type: "reset" });
  check(oldWorker.terminated && f.workers.length === 1, "reset terminates in-flight work even after its warning timer, without starting an idle worker");
  oldWorker.deliver(oldRequest, "STALE RESULT");
  check(f.spacer.children.length === 0, "a terminated busy worker cannot publish its old result");
  f.init(31); f.chunk(31, "fresh input");
  check(f.workers.length === 2 && f.workers[1].requests.length === 1, "new initialization starts a fresh worker after cancellation");
  f.workers[1].deliver(f.workers[1].requests[0], "fresh highlight");
  f.send({ type: "reset" }); f.init(32);
  check(f.workers.length === 2 && !f.workers[1].terminated, "a fully completed worker remains reusable on later opens");
}

// Opening from Find Files selects the first match while the chunk is still
// plain text. The worker result must still be painted (it was deferred
// behind the find selection forever), and the scroll must center the match,
// not the whole <code> block of the chunk.
function testInitialSearchMatchKeepsHighlightAndView() {
  const f = frontend();
  const ctx = f.context;
  ctx.NodeFilter = { SHOW_TEXT: 4, FILTER_ACCEPT: 1, FILTER_REJECT: 2 };
  // One text node per <code>, its value is the markup without tags.
  ctx.document.createTreeWalker = function (code) {
    const node = { nodeValue: code.innerHTML.replace(/<[^>]*>/g, ""), parentElement: code,
                   get isConnected() { return code.isConnected; } };
    let done = false;
    return { nextNode() { if (done) return null; done = true; return node; } };
  };
  const matchTopInViewport = 900;  // the match rect, relative to the viewport
  ctx.document.createRange = function () {
    return { setStart(node, offset) { this.startContainer = node; this.startOffset = offset; },
             setEnd(node, offset) { this.endContainer = node; this.endOffset = offset; },
             getBoundingClientRect() { return { top: matchTopInViewport, height: 16, width: 48 }; } };
  };
  const selection = f.selection;
  selection.addRange = function (range) { this.range = range; this.rangeCount = 1; this.isCollapsed = false; };
  selection.getRangeAt = function () { return this.range; };
  ctx.getSelection = () => selection;
  let selectionListener = null;
  ctx.document.addEventListener = function (type, handler) { if (type === "selectionchange") selectionListener = handler; };
  ctx.scrollTo = function (x, y) { this.scrollY = typeof x === "object" ? x.top : y; };
  // Re-run so the page picks up the extended document.
  vm.runInContext(source, ctx, { timeout: 1000 });
  const text = Array.from({ length: 80 }, (_, i) => (i === 10 ? "let needle = 1;" : `line ${i}`)).join("\n");
  f.send({ type: "init", generation: 50, lineCount: 80, chunkLines: 80, language: "javascript", searchPhrase: "needle" });
  f.send({ type: "chunk", generation: 50, startLine: 0, lineCount: 80, text });
  f.frame();
  ctx.scrollY = 0;
  f.send({ type: "findResult", query: "needle", matchIndex: 1, totalCount: 1, line: 10, col: 4, length: 6 });
  check(selection.rangeCount === 1 && selection.range.startOffset > 0, "the initial match is selected");
  // navigateToMatch first scrolls to the line estimate (160 - 120), the rect is relative to that.
  check(ctx.scrollY === 40 + matchTopInViewport + 8 - 300, "the scroll centers the match rectangle itself");
  selection.range.intersectsNode = () => true;
  f.spacer.contains = () => true;
  selectionListener();  // selectionchange: the find selection is now active
  const worker = f.workers[f.workers.length - 1];
  const request = worker.requests.find(r => r.generation === 50);
  worker.deliver(request, text.replace("let ", '<span class="token keyword">let</span> '));
  check(f.spacer.text().includes('class="token keyword"'), "highlighted HTML is painted despite the find selection");
  f.frame();
  check(selection.rangeCount === 1 && selection.range.startContainer.isConnected,
        "the match is selected again in the highlighted chunk");
}

// Opening from Find Files with the WebView unfocused: the first match must be
// marked with the custom highlight (a selection alone is not reliably visible
// without focus) and stay marked after the worker repaint. Real Ranges are
// live: removing their nodes collapses them to (parent, 0) while the
// container stays connected, so an isConnected test never re-marked them.
function testInitialSearchMatchMarkedWithoutFocus() {
  const f = frontend();
  const ctx = f.context;
  ctx.NodeFilter = { SHOW_TEXT: 4, FILTER_ACCEPT: 1, FILTER_REJECT: 2 };
  ctx.document.createTreeWalker = function (code) {
    const node = { nodeValue: code.innerHTML.replace(/<[^>]*>/g, ""), parentElement: code,
                   get isConnected() { return code.isConnected; } };
    let done = false;
    return { nextNode() { if (done) return null; done = true; return node; } };
  };
  const liveRanges = [];
  ctx.document.createRange = function () {
    const range = { collapsed: true,
      setStart(node, offset) { this.startContainer = node; this.startOffset = offset; },
      setEnd(node, offset) { this.endContainer = node; this.endOffset = offset; this.collapsed = false; },
      getBoundingClientRect() { return { top: 300, height: 16, width: 48 }; } };
    liveRanges.push(range);
    return range;
  };
  const highlights = new Map();
  ctx.CSS = { highlights };
  ctx.Highlight = class { constructor(range) { this.range = range; } };
  const selection = f.selection;
  selection.addRange = function (range) { this.range = range; this.rangeCount = 1; this.isCollapsed = false; };
  selection.getRangeAt = function () { return this.range; };
  ctx.getSelection = () => selection;
  let selectionListener = null;
  ctx.document.addEventListener = function (type, handler) { if (type === "selectionchange") selectionListener = handler; };
  vm.runInContext(source, ctx, { timeout: 1000 });
  const originalReplace = Element.prototype.replaceChildren;
  Element.prototype.replaceChildren = function (...children) {
    const removed = this.children.slice();
    liveRanges.forEach(range => {
      const owner = range.startContainer && range.startContainer.parentElement;
      if (owner && removed.some(child => child.contains(owner))) {
        range.startContainer = range.endContainer = this;
        range.startOffset = range.endOffset = 0;
        range.collapsed = true;
      }
    });
    return originalReplace.apply(this, children);
  };
  try {
    const text = Array.from({ length: 80 }, (_, i) => (i === 10 || i === 20 ? "let needle = 1;" : `line ${i}`)).join("\n");
    const first = text.indexOf("needle"), second = text.indexOf("needle", first + 1);
    const marked = () => highlights.get("viewer-find-match");
    f.send({ type: "init", generation: 60, lineCount: 80, chunkLines: 80, language: "javascript", searchPhrase: "needle" });
    f.send({ type: "chunk", generation: 60, startLine: 0, lineCount: 80, text });
    f.frame();
    f.send({ type: "findResult", query: "needle", matchIndex: 1, totalCount: 2, line: 10, col: 4, length: 6 });
    check(marked() && marked().range.startOffset === first && !marked().range.collapsed,
          "the first match is highlighted although the find bar has no focus");
    check(selection.range === marked().range, "the unfocused first match is also selected for copying");
    selection.range.intersectsNode = () => true;
    f.spacer.contains = () => true;
    selectionListener();
    const worker = f.workers[f.workers.length - 1];
    const request = worker.requests.find(r => r.generation === 60);
    worker.deliver(request, text.replace(/let /g, '<span class="token keyword">let</span> '));
    check(marked().range.collapsed, "the repaint collapsed the live range of the first match");
    f.frame();
    check(marked() && !marked().range.collapsed && marked().range.startOffset === first &&
          marked().range.startContainer.isConnected, "the first match is highlighted again after the worker repaint");
    check(selection.range === marked().range && !selection.range.collapsed, "the first match is selected again after the repaint");
    // F3: the host answers with the next match after the current one.
    f.send({ type: "findResult", query: "needle", matchIndex: 2, totalCount: 2, line: 20, col: 4, length: 6 });
    check(marked().range.startOffset === second && selection.range === marked().range, "F3 marks the second match");
  } finally {
    Element.prototype.replaceChildren = originalReplace;
  }
}

async function main() {
  testResetAndStaleDelivery();
  testInitialSearchMatchKeepsHighlightAndView();
  testInitialSearchMatchMarkedWithoutFocus();
  testFindStateIsClearedOnReset();
  testGoToLineBar();
  testSelectAllAndCopy();
  await testDelayedFallback();
  testWhitespaceTransition();
  testBusyWorkerReset();
  console.log(`${checks} Prism frontend reset checks passed.`);
}
main().catch(error => { console.error(error); process.exitCode = 1; });
