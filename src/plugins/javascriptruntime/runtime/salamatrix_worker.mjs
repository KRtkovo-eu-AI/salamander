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
  const name = payload?.event || payload?.name;
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
  constructor(title = "Salamander", options = {}) {
    this.id = null;
    this.title = title || "Salamander";
    this.options = options;
    this.changeHandlers = [];
  }

  async create() {
    const result = await hostCall("salamander.ui.dialog.create", {
      title: this.title,
      width: Number(this.options.width ?? 320),
      height: Number(this.options.height ?? 180),
    });
    this.id = result.dialogId;
    return this;
  }

  async addControl(kind, controlId, text = "", layout = null, options = {}) {
    const payload = {
      dialogId: this.id,
      kind,
      controlId,
      text,
      readOnly: Boolean(options.readOnly),
      checked: Boolean(options.checked),
      dialogResult: Number(options.dialogResult || 0),
      keepOpen: Boolean(options.keepOpen),
      multiline: Boolean(options.multiline),
    };
    if (layout) {
      for (const name of ["x", "y", "width", "height"]) {
        if (layout[name] !== undefined && layout[name] !== null)
          payload[name] = Number(layout[name]);
      }
    }
    if (kind === "filepicker") {
      payload.filter = String(options.filter || "");
      payload.save = Boolean(options.save);
    }
    for (const name of ["styleFlags", "pathSeparator", "toolTip",
                        "actionOpen", "actionCommand", "actionHint",
                        "progress", "progressCurrent", "progressTotal",
                        "progressText", "indeterminateDuration",
                        "indeterminateInterval", "textColor",
                        "backgroundColor", "alignControlId", "buttonMask"]) {
      if (options[name] !== undefined && options[name] !== null)
        payload[name] = options[name];
    }
    return hostCall("salamander.ui.dialog.add", payload);
  }

  // Convenience methods mirror the Python, PowerShell, and PHP worker
  // facades. They all go through the same native Salamatrix.UI control
  // contract; addControl remains available for generated/dynamic UIs.
  async addLabel(controlId, text, layout = null) {
    return this.addControl("label", controlId, text, layout);
  }

  async addTextBox(controlId, text = "", readOnly = false,
                   multiline = false, layout = null) {
    return this.addControl("textbox", controlId, text, layout,
      { readOnly, multiline });
  }

  async addFolderPicker(controlId, path = "", layout = null) {
    return this.addControl("folderpicker", controlId, path, layout);
  }

  async addFilePicker(controlId, path = "", layout = null,
                      filter = "", save = false) {
    return this.addControl("filepicker", controlId, path, layout,
      { filter, save });
  }

  async addCheckBox(controlId, text, checked = false, layout = null) {
    return this.addControl("checkbox", controlId, text, layout, { checked });
  }

  async addRadioButton(controlId, text, checked = false, layout = null) {
    return this.addControl("radio", controlId, text, layout, { checked });
  }

  async addComboBox(controlId, text = "", layout = null) {
    return this.addControl("combobox", controlId, text, layout);
  }

  async addListView(controlId, layout = null) {
    return this.addControl("listview", controlId, "", layout);
  }

  async addTreeView(controlId, layout = null) {
    return this.addControl("treeview", controlId, "", layout);
  }

  async addTabControl(controlId, layout = null) {
    return this.addControl("tabcontrol", controlId, "", layout);
  }

  async addButton(controlId, text, dialogResult = 1,
                  keepOpen = false, layout = null) {
    return this.addControl("button", controlId, text, layout,
      { dialogResult, keepOpen });
  }

  async addItem(controlId, text, parentIndex = -1) {
    const result = await hostCall("salamander.ui.dialog.item", {
      dialogId: this.id,
      controlId,
      text,
      parentIndex,
    });
    return Number(result.itemCount || 0);
  }

  async addNode(controlId, text, parentIndex = -1) {
    return this.addItem(controlId, text, parentIndex);
  }

  async addTab(controlId, text) {
    return this.addItem(controlId, text, -1);
  }

  async addColumn(controlId, title, width = 180) {
    await hostCall("salamander.ui.dialog.column", {
      dialogId: this.id,
      controlId,
      title,
      width,
    });
  }

  async setSelectedIndex(controlId, index) {
    const result = await hostCall("salamander.ui.dialog.selection", {
      dialogId: this.id,
      controlId,
      index,
    });
    return Number(result.selectedIndex ?? -1);
  }

  async setValidation(controlId, required, message = "") {
    await hostCall("salamander.ui.dialog.validation", {
      dialogId: this.id,
      controlId,
      required,
      message,
    });
  }

  async clearItems(controlId) {
    await hostCall("salamander.ui.dialog.clearItems", {
      dialogId: this.id,
      controlId,
    });
  }

  onChange(handler) {
    const event = `salamander.ui.dialog.${this.id}.changed`;
    const handlers = eventHandlers.get(event) || [];
    if (typeof handler === "function") handlers.push(handler);
    eventHandlers.set(event, handlers);
    this.changeHandlers.push(handler);
    return hostCall("salamander.ui.dialog.events", {
      dialogId: this.id,
      enabled: true,
      event,
    }).then(() => this);
  }

  async show() {
    const result = await hostCall("salamander.ui.dialog.show", { dialogId: this.id });
    return result.result ?? result;
  }

  async showModal() {
    return this.show();
  }

  async get(controlId) {
    return hostCall("salamander.ui.dialog.get", {
      dialogId: this.id,
      controlId,
    });
  }

  async getValue(controlId) {
    return this.get(controlId);
  }

  async set(controlId, value) {
    await hostCall("salamander.ui.dialog.set", {
      dialogId: this.id,
      controlId,
      value,
    });
  }

  async setValue(controlId, value) {
    await this.set(controlId, value);
  }

  async close() {
    if (this.id === null) return;
    await hostCall("salamander.ui.dialog.destroy", { dialogId: this.id });
    this.id = null;
  }

  async destroy() {
    await this.close();
  }

  async offChange() {
    const event = `salamander.ui.dialog.${this.id}.changed`;
    await hostCall("salamander.ui.dialog.events", {
      dialogId: this.id,
      enabled: false,
      event,
    });
    this.changeHandlers.length = 0;
    eventHandlers.delete(event);
  }
}

class Progress {
  constructor(title = "Salamatrix", total = 0, options = {}) {
    this.title = title;
    this.total = total;
    this.options = options;
    this.id = null;
  }

  async create() {
    const result = await hostCall("salamander.ui.progress.create", {
      title: this.title,
      total: this.total,
      ...this.options,
    });
    this.id = result.progressId;
    return this;
  }

  async update(position, total = undefined, text = "", delayedPaint = true,
               position2 = undefined, total2 = undefined) {
    const payload = {
      progressId: this.id,
      position,
      text,
      delayedPaint,
    };
    if (total !== undefined && total !== null) payload.total = total;
    if (position2 !== undefined && position2 !== null) payload.position2 = position2;
    if (total2 !== undefined && total2 !== null) payload.total2 = total2;
    const result = await hostCall("salamander.ui.progress.update", payload);
    return result.continued !== false;
  }

  async step(amount = 1, delayedPaint = true) {
    const result = await hostCall("salamander.ui.progress.step", {
      progressId: this.id,
      amount,
      delayedPaint,
    });
    return result.continued !== false;
  }

  async setTotals(total, total2) {
    return hostCall("salamander.ui.progress.setTotals", {
      progressId: this.id,
      total,
      total2,
    });
  }

  async setPositions(position, position2, delayedPaint = true) {
    const result = await hostCall("salamander.ui.progress.setPositions", {
      progressId: this.id,
      position,
      position2,
      delayedPaint,
    });
    return result.continued !== false;
  }

  async setTitle(title) {
    return hostCall("salamander.ui.progress.setTitle", {
      progressId: this.id,
      title,
    });
  }

  async setCancelEnabled(enabled) {
    return hostCall("salamander.ui.progress.setCancelEnabled", {
      progressId: this.id,
      enabled,
    });
  }

  async isCancelled() {
    const result = await hostCall("salamander.ui.progress.cancelled", {
      progressId: this.id,
    });
    return result.cancelled === true;
  }

  async close() {
    if (this.id === null) return;
    await hostCall("salamander.ui.progress.close", { progressId: this.id });
    this.id = null;
  }
}

const ui = {
  progress: async (title = "Salamatrix", total = 0, options = {}) =>
    new Progress(title, total, options).create(),
  messageBox: (message, title = "Salamander", buttons = "OK",
               icon = "Information") =>
    hostCall("salamander.ui.messageBox", {
      message, title, buttons, icon,
    }).then((result) => Number(result.result || 0)),
  notify: (message, title = "Salamander", timeoutMs = 5000) =>
    hostCall("salamander.ui.notify", {
      message, title, timeoutMs: Math.max(0, Number(timeoutMs)),
    }).then((result) => result.shown === true),
  controls: () => hostCall("salamander.ui.controls")
    .then((result) => result.shown === true),
  uptime: () => hostCall("salamander.host.uptime")
    .then((result) => String(result.milliseconds)),
  inputBox: (prompt, initial = "", title = "Salamander") =>
    hostCall("salamander.ui.inputBox", { prompt, title, initial }),
  pickFile: (options = {}) => hostCall("salamander.ui.pickFile", options),
  pickFolder: (options = {}) => hostCall("salamander.ui.pickFolder", options),
  dialog: (title, options = {}) => new Dialog(title, options),
};

class Side {
  constructor(name) {
    this.name = name;
  }

  activeTab() {
    return hostCall("salamander.sides.activeTab", { side: this.name });
  }

  context() {
    return hostCall("salamander.sides.context", { side: this.name });
  }

  tabs() {
    return hostCall("salamander.sides.tabs", { side: this.name })
      .then((result) => result.tabs || []);
  }

  activateTab(tabId, focus = true) {
    return hostCall("salamander.sides.activateTab", {
      tabId: String(tabId), focus,
    }).then((result) => result.activated === true);
  }

  changePath(path) {
    return hostCall("salamander.sides.changePath", {
      side: this.name, path,
    });
  }

  refresh(force = false, focusFirstNewItem = false) {
    return hostCall("salamander.sides.refresh", {
      side: this.name, force, focusFirstNewItem,
    }).then((result) => result.ok === true);
  }

  selectItem(index, select = true, repaint = true) {
    return hostCall("salamander.sides.selectItem", {
      side: this.name, index: Number(index), select, repaint,
    }).then((result) => result.changed === true);
  }

  selectAll(select = true, repaint = true) {
    return hostCall("salamander.sides.selectAll", {
      side: this.name, select, repaint,
    }).then((result) => result.changed === true);
  }

  focusItem(index, partVisible = true) {
    return hostCall("salamander.sides.focusItem", {
      side: this.name, index: Number(index), partVisible,
    }).then((result) => result.changed === true);
  }
  createTab(path = null, index = undefined) {
    const args = { side: this.name, path };
    if (index !== undefined) args.index = Number(index);
    return hostCall("salamander.sides.createTab", args);
  }
  closeTab(tabId) {
    return hostCall("salamander.sides.closeTab", { tabId: String(tabId) })
      .then((result) => result.ok === true);
  }
  reorderTab(tabId, index) {
    return hostCall("salamander.sides.reorderTab", {
      tabId: String(tabId), index: Number(index),
    }).then((result) => result.ok === true);
  }
  moveTab(tabId, side = this.name, index = undefined) {
    const args = { tabId: String(tabId), side };
    if (index !== undefined) args.index = Number(index);
    return hostCall("salamander.sides.moveTab", args)
      .then((result) => result.ok === true);
  }
  setDetached(detached) {
    return hostCall("salamander.sides.setDetached", { detached: !!detached })
      .then((result) => result.ok === true);
  }
}

const fileOperations = {};
for (const operation of ["rename", "copy", "move", "delete",
                         "createDirectory", "refresh", "properties"]) {
  fileOperations[operation] = () =>
    hostCall(`salamander.fileOperations.${operation}`).then(
      (result) => result.result);
}

const subscriptions = new Map();
const events = {
  async subscribe(name, handler) {
    const handlers = eventHandlers.get(name) || [];
    if (typeof handler === "function") handlers.push(handler);
    eventHandlers.set(name, handlers);
    const result = await hostCall("salamander.events.subscribe", { event: name });
    subscriptions.set(String(result.subscriptionId), name);
    return String(result.subscriptionId);
  },
  async unsubscribe(subscriptionId) {
    const id = String(subscriptionId);
    await hostCall("salamander.events.unsubscribe", { subscriptionId: id });
    const name = subscriptions.get(id);
    subscriptions.delete(id);
    if (name) eventHandlers.delete(name);
  },
};

const Salamander = {
  commandId: "",
  command_id: "",
  commandHandler: "",
  command_handler: "",
  ui,
  commands: {
    execute: (commandId) => hostCall(
      "salamander.commands.execute", { commandId }
    ).then((result) => result.result),
    register: (commandId, title, pluginMenu = true, contextMenu = false,
               hotKey = 0, toolbar = false, handler = "", enabled = true,
               visible = true) =>
      hostCall("salamander.commands.register", {
        commandId, title, pluginMenu, contextMenu, hotKey: Number(hotKey),
        toolbar, handler, enabled: Boolean(enabled), visible: Boolean(visible),
      }).then((result) => result.registered === true),
    unregister: (commandId) => hostCall("salamander.commands.unregister", {
      commandId,
    }).then((result) => result.unregistered === true),
    setState: (commandId, enabled = undefined, visible = undefined) => {
      const payload = { commandId };
      if (enabled !== undefined) payload.enabled = Boolean(enabled);
      if (visible !== undefined) payload.visible = Boolean(visible);
      return hostCall("salamander.commands.setState", payload)
        .then((result) => result.updated === true);
    },
  },
  fileOperations,
  file_operations: fileOperations,
  sides: {
    activeTab: (side = "source") =>
      hostCall("salamander.sides.activeTab", { side }),
    context: (side = "source") =>
      hostCall("salamander.sides.context", { side }),
    tabs: (side = "source") =>
      hostCall("salamander.sides.tabs", { side })
        .then((result) => result.tabs || []),
    activateTab: (tabId, focus = true) =>
      hostCall("salamander.sides.activateTab", {
        tabId: String(tabId), focus,
      }).then((result) => result.activated === true),
    changePath: (path, side = "source") =>
      hostCall("salamander.sides.changePath", { side, path }),
    refresh: (side = "source", force = false,
              focusFirstNewItem = false) =>
      hostCall("salamander.sides.refresh", {
        side, force, focusFirstNewItem,
      }).then((result) => result.ok === true),
    selectItem: (index, select = true, side = "source", repaint = true) =>
      hostCall("salamander.sides.selectItem", {
        side, index: Number(index), select, repaint,
      }).then((result) => result.changed === true),
    selectAll: (select = true, side = "source", repaint = true) =>
      hostCall("salamander.sides.selectAll", {
        side, select, repaint,
      }).then((result) => result.changed === true),
    focusItem: (index, side = "source", partVisible = true) =>
      hostCall("salamander.sides.focusItem", {
        side, index: Number(index), partVisible,
      }).then((result) => result.changed === true),
    createTab: (side = "source", path = null, index = undefined) => {
      const args = { side, path };
      if (index !== undefined) args.index = Number(index);
      return hostCall("salamander.sides.createTab", args);
    },
    closeTab: (tabId) => hostCall("salamander.sides.closeTab", {
      tabId: String(tabId),
    }).then((result) => result.ok === true),
    reorderTab: (tabId, index) => hostCall("salamander.sides.reorderTab", {
      tabId: String(tabId), index: Number(index),
    }).then((result) => result.ok === true),
    moveTab: (tabId, side = "source", index = undefined) => {
      const args = { tabId: String(tabId), side };
      if (index !== undefined) args.index = Number(index);
      return hostCall("salamander.sides.moveTab", args)
        .then((result) => result.ok === true);
    },
    setDetached: (detached) => hostCall("salamander.sides.setDetached", {
      detached: Boolean(detached),
    }).then((result) => result.ok === true),
  },
  leftSide: new Side("left"),
  rightSide: new Side("right"),
  sourceSide: new Side("source"),
  targetSide: new Side("target"),
  left_side: new Side("left"),
  right_side: new Side("right"),
  source_side: new Side("source"),
  target_side: new Side("target"),
  storage: {
    get: (key, defaultValue = null) => hostCall(
      "salamander.storage.get", { key }
    ).then((result) => {
      const type = result?.type;
      return type === "string" || type === "integer" || type === "boolean"
        ? result.value
        : defaultValue;
    }),
    set: (key, value) => hostCall(
      "salamander.storage.set", { key, value }
    ).then(() => undefined),
    remove: (key) => hostCall(
      "salamander.storage.remove", { key }
    ).then((result) => result.removed === true),
    clear: () => hostCall("salamander.storage.clear").then(
      (result) => result.ok === true
    ),
    schema: () => hostCall("salamander.storage.schema", {}).then(
      (result) => result.settings || []
    ),
    keys: () => hostCall("salamander.storage.keys", {}).then(
      (result) => Array.isArray(result?.keys) ? result.keys : []
    ),
  },
  clipboard: {
    copyText: (text, showEcho = false) => hostCall(
      "salamander.clipboard.copyText", { text, showEcho }
    ).then((result) => result.copied === true),
  },
  runtimes: {
    list: async () => (await hostCall("salamander.runtimes.list")).runtimes || [],
  },
  application: {
    language: () => hostCall("salamander.host.language"),
    appearance: () => hostCall("salamander.host.appearance"),
  },
  events,
  ai: {
    api: (topic = null) => hostCall("salamander.ai.api", topic ? { topic } : {}),
    apiDescription: (topic = null) => hostCall("salamander.ai.api", topic ? { topic } : {}),
    generate: (prompt, options = {}) => hostCall("salamander.ai.generate", {
      prompt, ...options,
    }),
    preview: (prompt, options = {}) => hostCall("salamander.ai.preview", {
      prompt, ...options,
    }),
  },
};

const commandIdIndex = process.argv.indexOf("--command-id");
const commandId = commandIdIndex >= 0 ? process.argv[commandIdIndex + 1] || "" : "";
const commandHandlerIndex = process.argv.indexOf("--command-handler");
const commandHandler = commandHandlerIndex >= 0
  ? process.argv[commandHandlerIndex + 1] || "" : "";
Salamander.commandId = commandId;
Salamander.command_id = commandId;
Salamander.commandHandler = commandHandler;
Salamander.command_handler = commandHandler;

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
