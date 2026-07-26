if (Salamander.command_handler === "run") {
  await Salamander.ui.messageBox(
    "Node.js extension package is running through Salamatrix.",
    "Salamatrix Node Demo",
  );
  await Salamander.ui.notify(
    "Node.js extension package is running through Salamatrix.",
    "Salamatrix Node Demo",
    2500,
  );
  await Salamander.storage.set("lastRun", "JavaScript.Node");
}
