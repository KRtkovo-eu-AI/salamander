// Demonstrates the Salamatrix-backed Automation API.
// Requires the Salamatrix Framework plugin to be installed and loaded.

var progress = Salamander.UI.progress("Salamatrix progress demo");
progress.Maximum = 5;
progress.Show();

try {
    for (var i = 1; i <= 5; ++i) {
        progress.AddText("Step " + i + " of 5");
        progress.Position = i;
        Salamander.Sleep(150);
        if (progress.IsCancelled) {
            break;
        }
    }
}
finally {
    progress.CanCancel = false;
    progress.Hide();
}

var commandResult = Salamander.Commands.execute("QuickRename");
var sourceSide = Salamander.Sides.Source;
var activeTab = sourceSide.ActiveTab;
var previousSourcePath = "(Storage unavailable in legacy Automation)";
var storageNamespace = "(legacy Automation)";
try {
    previousSourcePath =
        Salamander.Storage.get("lastSourcePath", "(first run)");
    Salamander.Storage.set("lastSourcePath", activeTab.Path);
    storageNamespace = Salamander.Storage.Namespace;
}
catch (storageError) {
    // The same script can be launched from Automation's legacy script list,
    // where an older installation may not have the adjacent extension.json.
    // The progress/API demonstration remains useful without package storage.
}
var sideSummary =
    "\n\nSource side: " + sourceSide.Name +
    "\nTabs on side: " + sourceSide.TabCount +
    "\nActive tab index: " + activeTab.Index +
    "\nActive tab path: " + activeTab.Path +
    "\nPath saved by previous run: " + previousSourcePath +
    "\nStorage namespace: " + storageNamespace;
if (commandResult == "not_available") {
    Salamander.MsgBox("Quick Rename is not available in the current panel context. Select or focus a file first and run the demo again." + sideSummary, 0, "Salamatrix Demo");
}
else {
    Salamander.MsgBox("Salamatrix command result: " + commandResult + sideSummary, 0, "Salamatrix Demo");
}

// Keep the native controls showcase as the final step of the demo.
Salamander.UI.controls();
