from pathlib import Path
import time

handler = Salamander.command_handler

if handler == "viewDemo":
    path = str(Salamander.invocation.get("path", ""))
    try:
        contents = Path(path).read_text(encoding="utf-8")
        preview = contents[:3000]
        if len(contents) > 3000:
            preview += "\n\n[preview truncated]"
        Salamander.ui.message_box(
            preview or "[empty file]",
            f"Salamatrix Python Viewer demo — {path}")
    except Exception as error:
        Salamander.ui.message_box(
            str(error), "Salamatrix Python Viewer demo", "OK", "Error")
    raise SystemExit(0)

if handler == "listDemoMachines":
    machines = [
        ("development", "Development VM", True),
        ("test-lab", "Test lab", False),
        ("build-agent", "Build agent", True),
    ]
    for machine_id, name, default_running in machines:
        running = Salamander.storage.get(
            f"machine.{machine_id}.running", default_running)
        Salamander.file_system.add_item(
            machine_id, f"{name} — {'Running' if running else 'Stopped'}",
            icon="icon.svg", directory=False, enabled=True)
    raise SystemExit(0)

if handler in ("inspectDemoMachine", "toggleDemoMachine"):
    item = Salamander.invocation.get("item") or {}
    if handler == "inspectDemoMachine":
        Salamander.ui.message_box(
            f"Id: {item.get('id', '')}\nName: {item.get('name', '')}",
            "Salamatrix FS item")
    else:
        item_id = str(item.get("id", "unknown"))
        key = f"machine.{item_id}.running"
        default_running = {
            "development": True,
            "test-lab": False,
            "build-agent": True,
        }.get(item_id, False)
        running = Salamander.storage.get(key, default_running)
        Salamander.storage.set(key, not running)
        Salamander.ui.notify(
            f"{item.get('name', item_id)}: {'Running' if not running else 'Stopped'}",
            "Salamatrix FS demo", 2500)
    raise SystemExit(0)

if handler == "run":
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

    # Build the complete gallery here, through the public runtime-neutral API.
    dialog = Salamander.ui.dialog("Salamatrix UI capabilities", 463, 236)
    def add(kind, control_id, text, x, y, width, height, **options):
        dialog.add_control(kind, control_id, text,
                           layout={"x": x, "y": y, "width": width, "height": height},
                           options=options)

    uptime = f"System was started {Salamander.ui.uptime()} ms ago."
    add("groupbox", "static-group", "CGUIStaticTextAbstract", 6, 4, 254, 108)
    add("label", "not-attached-label", "Not attached static text", 14, 17, 80, 8)
    add("label", "uptime-plain", uptime, 102, 17, 152, 8)
    rows = [
        ("static-none", "0 (no flags)", uptime, 27, 0),
        ("static-cache", "STF_CACHED_PAINT", uptime, 37, 1),
        ("static-bold", "STF_BOLD", "Bold &text", 47, 0x10082),
        ("static-underline", "STF_UNDERLINE", "Underlined text", 56, 0x20004),
        ("static-end", "STF_END_ELLIPSIS", "Long long long long long long long long long string.", 66, 0x20),
        ("static-path", "STF_PATH_ELLIPSIS", r"C:\Program Files\Some Application With Long Path\example.exe", 76, 0x40),
        ("static-path-url", "STF_PATH_ELLIPSIS", "ftp://ftp.altap.cz/pub/salamander/example.exe", 87, 0x40),
    ]
    for control_id, caption, value, y, flags in rows:
        add("label", control_id + "-label", caption, 14, y, 75, 8)
        extra = {"styleFlags": flags}
        if control_id == "static-path-url": extra["pathSeparator"] = "/"
        add("statictext", control_id, value, 102, y, 152, 8, **extra)
    add("label", "drag-hint", "Drag texts to change their size.", 151, 97, 103, 8)
    add("groupbox", "progress-group", "CGUIProgressBarAbstract", 6, 118, 254, 66)
    add("label", "progress-label", "Progress label", 15, 129, 60, 8)
    add("progressbar", "progress", "", 15, 138, 235, 12, progress=120)
    add("label", "unknown-label", "Unknown progress", 15, 154, 67, 8)
    add("progressbar", "unknown-progress", "", 15, 163, 235, 12, progress=-1, indeterminateDuration=-1, indeterminateInterval=100)
    add("groupbox", "buttons-group", "Button, CGUITextArrowButtonAbstract, CGUIColorArrowButtonAbstract", 6, 188, 254, 40)
    add("button", "more", "...", 15, 204, 15, 14, keepOpen=True)
    add("arrowbutton", "arrow", "", 37, 204, 15, 14)
    add("textarrowbutton", "choose", "&Choose", 60, 204, 50, 14, styleFlags=8)
    add("textarrowbutton", "drop", "&Drop", 117, 204, 50, 14, styleFlags=2)
    add("colorarrowbutton", "color", "", 174, 204, 33, 14, textColor=0xff8000, backgroundColor=0xff8000)
    add("colorarrowbutton", "color-text", "ABC", 215, 204, 33, 14, textColor=0, backgroundColor=0xffff)
    add("groupbox", "hyperlink-group", "CGUIHyperLinkAbstract", 269, 4, 185, 48)
    add("label", "open-label", "SetActionOpen", 277, 17, 75, 8)
    add("hyperlink", "open-link", "www.altap.cz", 365, 17, 47, 8, styleFlags=0x14, actionOpen="https://www.altap.cz")
    add("label", "command-label", "SetActionPostCommand", 277, 27, 81, 8)
    add("hyperlink", "command-link", "Say something!", 365, 27, 55, 8, styleFlags=0x14, actionCommand=0x7f01)
    add("label", "hint-label", "SetActionShowHint", 277, 37, 81, 8)
    add("hyperlink", "hint-link", "mask hints", 365, 37, 40, 8, styleFlags=8, actionHint="text 1 text 1 text 1 text 1\ntext 2 text 2 text 2")
    add("groupbox", "tooltip-group", "SetCurrentToolTip", 269, 59, 185, 31)
    add("statictext", "tooltip", "Pause the mouse pointer over this text.", 278, 73, 130, 8, styleFlags=0x40000, toolTip="ToolTip")
    add("listview", "header-list", "", 269, 113, 185, 50, styleFlags=0x01e00000)
    add("toolbarheader", "toolbar-header", "CGUIToolbarHeaderAbstract", 269, 102, 96, 8, alignControlId="header-list", buttonMask=0x31)
    add("groupbox", "origin-group", "Created by", 269, 169, 185, 38)
    add("label", "runtime-label", "Runtime:", 277, 181, 42, 8)
    add("statictext", "runtime-value", "Python.CPython", 323, 181, 122, 8, styleFlags=2)
    add("label", "extension-label", "Extension:", 277, 192, 42, 8)
    add("statictext", "extension-value", "Salamatrix Python Demo", 323, 192, 122, 8, styleFlags=2)
    add("button", "close", "Close", 403, 213, 50, 14, dialogResult=1, styleFlags=0x100000)
    try:
        dialog.show()
    finally:
        dialog.close()
