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
    this.classList = { add() {}, toggle() {} };
    this.innerHTML = this.textContent = "";
    this.clientWidth = 800;
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
  contains(node) { return node === this || this.children.some(child => child.contains(node)); }
  text() { return this.innerHTML + this.textContent + this.children.map(child => child.text()).join(""); }
}

function frontend() {
  let id = 0;
  let receive;
  const timers = new Map(), frames = new Map(), messages = [], workers = [], observers = [], loads = [];
  const body = new Element("body"), spacer = new Element("main"), status = new Element("div");
  body.appendChild(spacer); body.appendChild(status);
  const selection = { rangeCount: 0, isCollapsed: true, removeAllRanges() { this.rangeCount = 0; this.isCollapsed = true; } };
  const context = {
    console: { log() {}, warn() {}, error() {} }, URL,
    document: {
      readyState: "complete", baseURI: "https://prism.example/viewer/virtual-viewer.html",
      body, documentElement: { style: { setProperty() {} }, clientHeight: 600, scrollTop: 0 },
      getElementById(name) { return name === "virtual-spacer" ? spacer : status; },
      createElement(tag) { return new Element(tag); },
      getSelection() { return selection; }, addEventListener() {}
    },
    chrome: { webview: { postMessage(value) { messages.push(value); }, addEventListener(type, handler) { if (type === "message") receive = handler; } } },
    setTimeout(fn, delay) { const key = ++id; timers.set(key, { fn, delay }); return key; },
    clearTimeout(key) { timers.delete(key); },
    requestAnimationFrame(fn) { const key = ++id; frames.set(key, fn); return key; },
    cancelAnimationFrame(key) { frames.delete(key); },
    addEventListener() {}, innerHeight: 600, scrollY: 0,
    scrollTo(x, y) { this.scrollY = y; },
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
  return { context, send, init, chunk, frame, ready, queued, timers, frames, messages, workers, observers, loads, spacer, selection };
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

async function main() {
  testResetAndStaleDelivery();
  await testDelayedFallback();
  testWhitespaceTransition();
  testBusyWorkerReset();
  console.log(`${checks} Prism frontend reset checks passed.`);
}
main().catch(error => { console.error(error); process.exitCode = 1; });
