const handler = Salamander.commandHandler || Salamander.command_handler;

if (handler === "viewDemo") {
  const { readFile } = await import("node:fs/promises");
  const path = String(Salamander.invocation.path || "");
  try {
    const contents = await readFile(path, "utf8");
    const preview = contents.length > 3000
      ? `${contents.slice(0, 3000)}\n\n[preview truncated]`
      : contents;
    await Salamander.ui.messageBox(
      preview || "[empty file]",
      `Salamatrix Viewer demo — ${path}`,
      "OK",
      "Information",
    );
  } catch (error) {
    await Salamander.ui.messageBox(
      String(error), "Salamatrix Viewer demo", "OK", "Error");
  }
} else if (handler === "listDemoMachines") {
  const machines = [
    { id: "development", name: "Development VM", running: true },
    { id: "test-lab", name: "Test lab", running: false },
    { id: "build-agent", name: "Build agent", running: true },
  ];
  for (const machine of machines) {
    const running = await Salamander.storage.get(
      `machine.${machine.id}.running`, machine.running);
    await Salamander.fileSystem.addItem(
      machine.id,
      `${machine.name} — ${running ? "Running" : "Stopped"}`,
      { icon: "icon.svg", directory: false, enabled: true },
    );
  }
} else if (handler === "inspectDemoMachine") {
  const item = Salamander.invocation.item || {};
  await Salamander.ui.messageBox(
    `Id: ${item.id || ""}\nName: ${item.name || ""}`,
    "Salamatrix FS item",
  );
} else if (handler === "toggleDemoMachine") {
  const item = Salamander.invocation.item || {};
  const key = `machine.${item.id || "unknown"}.running`;
  const defaultRunning = {
    development: true,
    "test-lab": false,
    "build-agent": true,
  }[item.id] ?? false;
  const running = await Salamander.storage.get(key, defaultRunning);
  await Salamander.storage.set(key, !running);
  await Salamander.ui.notify(
    `${item.name || item.id}: ${!running ? "Running" : "Stopped"}`,
    "Salamatrix FS demo",
    2500,
  );
} else if (handler === "run") {
  await Salamander.ui.notify(
    "Node.js extension package is running through Salamatrix.",
    "Salamatrix Node Demo",
    2500,
  );
  const progress = await Salamander.ui.progress(
    "Salamatrix Node Progress Demo", 5,
  );
  try {
    for (let step = 1; step <= 5; step += 1) {
      await progress.update(step, undefined, `Step ${step} of 5`);
      await new Promise((resolve) => setTimeout(resolve, 150));
      if (await progress.isCancelled()) break;
    }
  } finally {
    await progress.close();
  }
  await Salamander.storage.set("lastRun", "JavaScript.Node");

  // Build the complete gallery here, through the public runtime-neutral API.
  const dialog = await Salamander.ui.dialog(
    "Salamatrix UI capabilities", { width: 463, height: 236 }).create();
  const add = (kind, id, text, x, y, width, height, options = {}) =>
    dialog.addControl(kind, id, text, { x, y, width, height }, options);
  const uptime = `System was started ${await Salamander.ui.uptime()} ms ago.`;
  await add("groupbox", "static-group", "CGUIStaticTextAbstract", 6, 4, 254, 108);
  await add("label", "not-attached-label", "Not attached static text", 14, 17, 80, 8);
  await add("label", "uptime-plain", uptime, 102, 17, 152, 8);
  const rows = [
    ["static-none", "0 (no flags)", uptime, 27, 0],
    ["static-cache", "STF_CACHED_PAINT", uptime, 37, 1],
    ["static-bold", "STF_BOLD", "Bold &text", 47, 0x10082],
    ["static-underline", "STF_UNDERLINE", "Underlined text", 56, 0x20004],
    ["static-end", "STF_END_ELLIPSIS", "Long long long long long long long long long string.", 66, 0x20],
    ["static-path", "STF_PATH_ELLIPSIS", "C:\\Program Files\\Some Application With Long Path\\example.exe", 76, 0x40],
    ["static-path-url", "STF_PATH_ELLIPSIS", "ftp://ftp.altap.cz/pub/salamander/example.exe", 87, 0x40],
  ];
  for (const [id, caption, value, y, styleFlags] of rows) {
    await add("label", `${id}-label`, caption, 14, y, 75, 8);
    await add("statictext", id, value, 102, y, 152, 8,
      { styleFlags, ...(id === "static-path-url" ? { pathSeparator: "/" } : {}) });
  }
  await add("label", "drag-hint", "Drag texts to change their size.", 151, 97, 103, 8);
  await add("groupbox", "progress-group", "CGUIProgressBarAbstract", 6, 118, 254, 66);
  await add("label", "progress-label", "Progress label", 15, 129, 60, 8);
  await add("progressbar", "progress", "", 15, 138, 235, 12, { progress: 120 });
  await add("label", "unknown-label", "Unknown progress", 15, 154, 67, 8);
  await add("progressbar", "unknown-progress", "", 15, 163, 235, 12,
    { progress: -1, indeterminateDuration: -1, indeterminateInterval: 100 });
  await add("groupbox", "buttons-group", "Button, CGUITextArrowButtonAbstract, CGUIColorArrowButtonAbstract", 6, 188, 254, 40);
  await add("button", "more", "...", 15, 204, 15, 14, { keepOpen: true });
  await add("arrowbutton", "arrow", "", 37, 204, 15, 14);
  await add("textarrowbutton", "choose", "&Choose", 60, 204, 50, 14, { styleFlags: 8 });
  await add("textarrowbutton", "drop", "&Drop", 117, 204, 50, 14, { styleFlags: 2 });
  await add("colorarrowbutton", "color", "", 174, 204, 33, 14, { textColor: 0xff8000, backgroundColor: 0xff8000 });
  await add("colorarrowbutton", "color-text", "ABC", 215, 204, 33, 14, { textColor: 0, backgroundColor: 0xffff });
  await add("groupbox", "hyperlink-group", "CGUIHyperLinkAbstract", 269, 4, 185, 48);
  await add("label", "open-label", "SetActionOpen", 277, 17, 75, 8);
  await add("hyperlink", "open-link", "www.altap.cz", 365, 17, 47, 8, { styleFlags: 0x14, actionOpen: "https://www.altap.cz" });
  await add("label", "command-label", "SetActionPostCommand", 277, 27, 81, 8);
  await add("hyperlink", "command-link", "Say something!", 365, 27, 55, 8, { styleFlags: 0x14, actionCommand: 0x7f01 });
  await add("label", "hint-label", "SetActionShowHint", 277, 37, 81, 8);
  await add("hyperlink", "hint-link", "mask hints", 365, 37, 40, 8, { styleFlags: 8, actionHint: "text 1 text 1 text 1 text 1\ntext 2 text 2 text 2" });
  await add("groupbox", "tooltip-group", "SetCurrentToolTip", 269, 59, 185, 31);
  await add("statictext", "tooltip", "Pause the mouse pointer over this text.", 278, 73, 130, 8, { styleFlags: 0x40000, toolTip: "ToolTip" });
  await add("listview", "header-list", "", 269, 113, 185, 50, { styleFlags: 0x01e00000 });
  await add("toolbarheader", "toolbar-header", "CGUIToolbarHeaderAbstract", 269, 102, 96, 8, { alignControlId: "header-list", buttonMask: 0x31 });
  await add("groupbox", "origin-group", "Created by", 269, 169, 185, 38);
  await add("label", "runtime-label", "Runtime:", 277, 181, 42, 8);
  await add("statictext", "runtime-value", "JavaScript.Node", 323, 181, 122, 8, { styleFlags: 2 });
  await add("label", "extension-label", "Extension:", 277, 192, 42, 8);
  await add("statictext", "extension-value", "Salamatrix Node Demo", 323, 192, 122, 8, { styleFlags: 2 });
  await add("button", "close", "Close", 403, 213, 50, 14, { dialogResult: 1, styleFlags: 0x100000 });
  try { await dialog.show(); } finally { await dialog.close(); }
}
