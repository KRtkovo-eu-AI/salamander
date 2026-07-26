if Salamander.command_handler == "run":
    Salamander.ui.notify("CPython extension package is running through Salamatrix.", "Salamatrix Python Demo", 2500)
    Salamander.storage.set("lastRun", "Python.CPython")
