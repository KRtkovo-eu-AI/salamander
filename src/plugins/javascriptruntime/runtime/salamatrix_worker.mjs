/*
 * Salamatrix JavaScript worker bootstrap.
 *
 * This file deliberately has no third-party dependency.  It implements the
 * same bounded SMX1 line transport used by the Python, PowerShell, and PHP
 * workers and exposes the shared Salamander object model.  Registration of a
 * JavaScriptRuntime.SPL owns this worker and registers JavaScript.Node with
 * the Salamatrix runtime broker.
 */

import readline from "node:readline";
import path from "node:path";
import { pathToFileURL } from "node:url";

const MAX_FRAME_BYTES = 1024 * 1024;
let nextCallId = 1;
const pending = new Map();
const eventHandlers = new Map();

function writeFrame(type, id, payload) {
  const json = JSON.stringify(payload ?? {});
  const line = `SMX1\t${type}\t${id}\t${json}\n`;
  if (Buffer.byteLength(line, "utf8") > MAX_FRAME_BYTES) {
    throw new Error("SMX1 frame exceeds the provider limit");
  }
  process.stdout.write(line);
}

function parseFrame(line) {
  if (Buffer.byteLength(line, "utf8") > MAX_FRAME_BYTES) {
    throw new Error("SMX1 frame exceeds the provider limit");
  }
  const fields = line.split("\t");
  if (fields.length < 4 || fields[0] !== "SMX1") {
    throw new Error("invalid SMX1 frame");
  }
  const id = Number(fields[2]);
  if (!Number.isSafeInteger(id) || id < 0) {
    throw new Error("invalid SMX1 request id");
  }
  return { type: fields[1], id, payload: JSON.parse(fields.slice(3).join("\t")) };
}

function dispatchEvent(payload) {
  const name = payload?.name;
  const handlers = eventHandlers.get(name) || [];
  for (const handler of [...handlers]) {
    try {
      handler(payload?.payload ?? payload);
    } catch (_) {
      // An extension callback must not terminate the worker transport.
    }
  }
}

function handleFrame(line) {
  if (!line) return;
  let frame;
  try {
    frame = parseFrame(line);
  } catch (error) {
    writeFrame("error", 0, { message: String(error?.message || error) });
    return;
  }
  if (frame.type === "result" || frame.type === "error") {
    const waiter = pending.get(frame.id);
    if (!waiter) return;
    pending.delete(frame.id);
    if (frame.type === "error" || frame.payload?.ok === false) {
      waiter.reject(new Error(frame.payload?.error || "Salamatrix host call failed"));
    } else {
      waiter.resolve(frame.payload);
    }
    return;
  }
  if (frame.type === "event") {
    dispatchEvent(frame.payload || {});
    return;
  }
  if (frame.type === "hello") {
    writeFrame("ready", frame.id, { runtime: "JavaScript.Node", protocol: "SMX1" });
  }
}

function hostCall(method, params = {}) {
  const id = nextCallId++;
  return new Promise((resolve, reject) => {
    pending.set(id, { resolve, reject });
    try {
      writeFrame("call", id, { method, ...params });
    } catch (error) {
      pending.delete(id);
      reject(error);
    }
  });
}

class Dialog {
  constructor(title, options = {}) {
    this.id = null;
    this.title = title || "Salamatrix";
    this.options = options;
    this.changeHandlers = [];
  }

  async create() {
    const result = await hostCall("salamander.ui.dialog.create", {
      title: this.title,
      options: this.options,
    });
    this.id = result.dialogId;
    return this;
  }

  async addControl(kind, controlId, text = "", layout = null, options = {}) {
    return hostCall("salamander.ui.dialog.addControl", {
      dialogId: this.id,
      kind,
      controlId,
      text,
      layout,
      options,
    });
  }

  async addItem(controlId, text, value = "") {
    return hostCall("salamander.ui.dialog.addItem", {
      dialogId: this.id,
      controlId,
      text,
      value,
    });
  }

  async addNode(controlId, text, parentIndex = -1, value = "") {
    return hostCall("salamander.ui.dialog.addNode", {
      dialogId: this.id,
      controlId,
      text,
      parentIndex,
      value,
    });
  }

  async addTab(controlId, text, value = "") {
    return hostCall("salamander.ui.dialog.addTab", {
      dialogId: this.id,
      controlId,
      text,
      value,
    });
  }

  async addColumn(controlId, title, width = 120) {
    return hostCall("salamander.ui.dialog.addColumn", {
      dialogId: this.id,
      controlId,
      title,
      width,
    });
  }

  async setSelectedIndex(controlId, index) {
    return hostCall("salamander.ui.dialog.setSelectedIndex", {
      dialogId: this.id,
      controlId,
      index,
    });
  }

  async setValidation(controlId, required, message = "") {
    return hostCall("salamander.ui.dialog.setValidation", {
      dialogId: this.id,
      controlId,
      required,
      message,
    });
  }

  onChange(handler) {
    this.changeHandlers.push(handler);
    return this;
  }

  async showModal() {
    const result = await hostCall("salamander.ui.dialog.showModal", { dialogId: this.id });
    return result.result ?? result;
  }

  async getValue(controlId) {
    const result = await hostCall("salamander.ui.dialog.getValue", {
      dialogId: this.id,
      controlId,
    });
    return result.value;
  }

  async setValue(controlId, value) {
    return hostCall("salamander.ui.dialog.setValue", {
      dialogId: this.id,
      controlId,
      value,
    });
  }

  async destroy() {
    if (this.id === null) return;
    await hostCall("salamander.ui.dialog.destroy", { dialogId: this.id });
    this.id = null;
  }
}

const ui = {
  progress: (title, total = 0) => hostCall("salamander.ui.progress", { title, total }),
  messageBox: (message, title = "Salamatrix") =>
    hostCall("salamander.ui.messageBox", { message, title }),
  inputBox: (prompt, initialValue = "", title = "Salamatrix") =>
    hostCall("salamander.ui.inputBox", { prompt, initialValue, title }),
  pickFile: (options = {}) => hostCall("salamander.ui.pickFile", options),
  pickFolder: (options = {}) => hostCall("salamander.ui.pickFolder", options),
  dialog: (title, options = {}) => new Dialog(title, options),
};

const Salamander = {
  ui,
  commands: {
    execute: (command, args = {}) => hostCall("salamander.commands.execute", { command, args }),
  },
  storage: {
    get: (key) => hostCall("salamander.storage.get", { key }),
    set: (key, value) => hostCall("salamander.storage.set", { key, value }),
    remove: (key) => hostCall("salamander.storage.remove", { key }),
  },
  runtimes: {
    list: async () => (await hostCall("salamander.runtimes.list")).runtimes || [],
  },
  events: {
    subscribe(name, handler) {
      const handlers = eventHandlers.get(name) || [];
      handlers.push(handler);
      eventHandlers.set(name, handlers);
      return hostCall("salamander.events.subscribe", { name });
    },
    unsubscribe: (name) => hostCall("salamander.events.unsubscribe", { name }),
  },
  ai: {
    generate: (prompt, runtime = "", feedback = "") =>
      hostCall("salamander.ai.generate", { prompt, runtime, feedback }),
    preview: (prompt, runtime = "", feedback = "") =>
      hostCall("salamander.ai.preview", { prompt, runtime, feedback }),
  },
};

globalThis.Salamander = Salamander;
globalThis.Salamatrix = { Salamander };

const input = readline.createInterface({ input: process.stdin, crlfDelay: Infinity });
input.on("line", handleFrame);
input.on("close", () => {
  for (const waiter of pending.values()) {
    waiter.reject(new Error("Salamatrix host closed the worker transport"));
  }
  pending.clear();
});

writeFrame("hello", 0, { runtime: "JavaScript.Node", protocol: "SMX1" });

const entryIndex = process.argv.indexOf("--entry");
const entryPoint = entryIndex >= 0 ? process.argv[entryIndex + 1] : null;
const oneShot = process.argv.includes("--one-shot");
if (entryPoint) {
  const resolvedEntry = path.resolve(entryPoint);
  import(pathToFileURL(resolvedEntry).href)
    .then(() => {
      if (oneShot) process.exit(0);
    })
    .catch((error) => {
      writeFrame("error", 0, { message: String(error?.stack || error) });
      if (oneShot) process.exit(1);
    });
}

