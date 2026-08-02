import time

Salamander.ui.notify("CPython extension package is running through Salamatrix.", "Salamatrix Python Demo", 2500)
progress = Salamander.ui.progress("Salamatrix Python Progress Demo", 5)
try:
    for step in range(1, 6):
        progress.update(step, text=f"Step {step} of 5")
        time.sleep(0.15)
        if progress.is_cancelled():
            break
finally:
    progress.close()
Salamander.storage.set("lastRun", "Python.CPython")

# Keep the controls showcase last so it is the final, user-controlled step.
dialog = Salamander.ui.dialog("Salamatrix UI capabilities", 520, 315)
try:
    dialog.add_control("label", "intro",
                       "Controls provided by Salamatrix", layout={"x": 10, "y": 8, "width": 500, "height": 12})
    dialog.add_control("label", "text-heading", "Text and picker controls",
                       layout={"x": 10, "y": 28, "width": 240, "height": 12})
    dialog.add_control("textbox", "description",
                       "Native controls are shared by every Salamatrix runtime.\r\nThe dialog follows the current Salamander theme and DPI.",
                       read_only=True, multiline=True,
                       layout={"x": 10, "y": 42, "width": 240, "height": 42})
    dialog.add_control("filepicker", "file", "C:\\Example\\document.txt",
                       layout={"x": 10, "y": 94, "width": 240, "height": 18},
                       filter="Text files|*.txt|All files|*.*")
    dialog.add_control("folderpicker", "folder", "Choose a folder...",
                       layout={"x": 10, "y": 118, "width": 240, "height": 18})
    dialog.add_control("checkbox", "checkbox", "Check box", checked=True,
                       layout={"x": 10, "y": 146, "width": 110, "height": 14})
    dialog.add_control("radio", "radio", "Radio button", checked=True,
                       layout={"x": 130, "y": 146, "width": 120, "height": 14})
    dialog.add_control("tabcontrol", "tabs", "",
                       layout={"x": 10, "y": 174, "width": 240, "height": 70})
    dialog.add_item("tabs", "Overview")
    dialog.add_item("tabs", "Details")
    dialog.set_selected_index("tabs", 0)

    dialog.add_control("label", "collection-heading", "Choice and collection controls",
                       layout={"x": 270, "y": 28, "width": 240, "height": 12})
    dialog.add_control("combobox", "choice", "",
                       layout={"x": 270, "y": 42, "width": 240, "height": 80})
    for item in ("Salamatrix UI", "Native Win32 controls", "Runtime-neutral API"):
        dialog.add_item("choice", item)
    dialog.set_selected_index("choice", 0)
    dialog.add_control("listview", "list", "",
                       layout={"x": 270, "y": 70, "width": 240, "height": 78})
    dialog.add_column("list", "Capability", 210)
    for item in ("Explicit layouts", "Validation and events", "Accessible metadata"):
        dialog.add_item("list", item)
    dialog.set_selected_index("list", 0)
    dialog.add_control("treeview", "tree", "",
                       layout={"x": 270, "y": 158, "width": 240, "height": 86})
    dialog.add_item("tree", "Salamatrix UI")
    dialog.add_item("tree", "Dialogs", 0)
    dialog.add_item("tree", "Controls", 0)
    dialog.add_control("button", "close", "Close", dialog_result=1,
                       layout={"x": 440, "y": 276, "width": 70, "height": 22})
    dialog.show()
finally:
    dialog.close()
