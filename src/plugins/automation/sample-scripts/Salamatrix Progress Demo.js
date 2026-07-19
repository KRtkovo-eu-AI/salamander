// Salamatrix.CommandId: Salamatrix.ProgressDemo
// Salamatrix.CommandTitle: Salamatrix Progress Demo
// Demonstrates the Salamatrix-backed Automation API.
// Requires the Salamatrix Runtime plugin to be installed and loaded.

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
Salamander.MsgBox("Salamatrix command result: " + commandResult, 0, "Salamatrix Demo");
