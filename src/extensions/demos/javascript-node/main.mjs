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

  // The framework-owned native controls showcase is the final demo step.
  if (!await Salamander.ui.controls())
    throw new Error("Salamatrix UI controls showcase is unavailable");
}
