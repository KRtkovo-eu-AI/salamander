if (Salamander.command_handler === "run") {
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

  // Keep the controls showcase last so it is the final, user-controlled step.
  const dialog = await Salamander.ui.dialog(
    "Salamatrix UI capabilities", { width: 520, height: 315 },
  ).create();
  try {
    await dialog.addLabel("intro", "Controls provided by Salamatrix",
      { x: 10, y: 8, width: 500, height: 12 });
    await dialog.addLabel("text-heading", "Text and picker controls",
      { x: 10, y: 28, width: 240, height: 12 });
    await dialog.addTextBox("description",
      "Native controls are shared by every Salamatrix runtime.\r\nThe dialog follows the current Salamander theme and DPI.",
      true, true, { x: 10, y: 42, width: 240, height: 42 });
    await dialog.addFilePicker("file", "C:\\Example\\document.txt",
      { x: 10, y: 94, width: 240, height: 18 },
      "Text files|*.txt|All files|*.*");
    await dialog.addFolderPicker("folder", "Choose a folder...",
      { x: 10, y: 118, width: 240, height: 18 });
    await dialog.addCheckBox("checkbox", "Check box", true,
      { x: 10, y: 146, width: 110, height: 14 });
    await dialog.addRadioButton("radio", "Radio button", true,
      { x: 130, y: 146, width: 120, height: 14 });
    await dialog.addTabControl("tabs",
      { x: 10, y: 174, width: 240, height: 70 });
    await dialog.addTab("tabs", "Overview");
    await dialog.addTab("tabs", "Details");
    await dialog.setSelectedIndex("tabs", 0);

    await dialog.addLabel("collection-heading", "Choice and collection controls",
      { x: 270, y: 28, width: 240, height: 12 });
    await dialog.addComboBox("choice", "",
      { x: 270, y: 42, width: 240, height: 80 });
    for (const item of ["Salamatrix UI", "Native Win32 controls", "Runtime-neutral API"])
      await dialog.addItem("choice", item);
    await dialog.setSelectedIndex("choice", 0);
    await dialog.addListView("list",
      { x: 270, y: 70, width: 240, height: 78 });
    await dialog.addColumn("list", "Capability", 210);
    for (const item of ["Explicit layouts", "Validation and events", "Accessible metadata"])
      await dialog.addItem("list", item);
    await dialog.setSelectedIndex("list", 0);
    await dialog.addTreeView("tree",
      { x: 270, y: 158, width: 240, height: 86 });
    await dialog.addNode("tree", "Salamatrix UI");
    await dialog.addNode("tree", "Dialogs", 0);
    await dialog.addNode("tree", "Controls", 0);
    await dialog.addButton("close", "Close", 1, false,
      { x: 440, y: 276, width: 70, height: 22 });
    await dialog.showModal();
  } finally {
    await dialog.close();
  }
}
