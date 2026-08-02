if Salamander.command_handler == "run" then
    Salamander.ui.notify(
        "Lua extension package is running through Salamatrix.",
        "Salamatrix Lua Demo",
        2500)

    local progress = Salamander.ui.progress(
        "Salamatrix Lua Progress Demo", 5)
    local succeeded, failure = pcall(function()
        for step = 1, 5 do
            progress.update(step, {text = "Step " .. step .. " of 5"})

            -- Standard Lua has no sleep function. Keep the short demo progress
            -- visible without starting an external process.
            local resume_at = os.clock() + 0.15
            while os.clock() < resume_at do end

            if progress.is_cancelled() then break end
        end
    end)
    progress.close()
    if not succeeded then error(failure) end

    Salamander.storage.set("lastRun", "Lua")

    -- Keep the controls showcase last so it is the final, user-controlled step.
    local dialog = Salamander.ui.dialog("Salamatrix UI capabilities", 520, 315)
    local shown, dialog_failure = pcall(function()
        dialog.add_control("label", "intro", "Controls provided by Salamatrix",
            {x = 10, y = 8, width = 500, height = 12})
        dialog.add_control("label", "text-heading", "Text and picker controls",
            {x = 10, y = 28, width = 240, height = 12})
        dialog.add_control("textbox", "description",
            "Native controls are shared by every Salamatrix runtime.\r\nThe dialog follows the current Salamander theme and DPI.",
            {read_only = true, multiline = true, x = 10, y = 42, width = 240, height = 42})
        dialog.add_control("filepicker", "file", "C:\\Example\\document.txt",
            {filter = "Text files|*.txt|All files|*.*", x = 10, y = 94, width = 240, height = 18})
        dialog.add_control("folderpicker", "folder", "Choose a folder...",
            {x = 10, y = 118, width = 240, height = 18})
        dialog.add_control("checkbox", "checkbox", "Check box",
            {checked = true, x = 10, y = 146, width = 110, height = 14})
        dialog.add_control("radio", "radio", "Radio button",
            {checked = true, x = 130, y = 146, width = 120, height = 14})
        dialog.add_control("tabcontrol", "tabs", "",
            {x = 10, y = 174, width = 240, height = 70})
        dialog.add_item("tabs", "Overview")
        dialog.add_item("tabs", "Details")
        dialog.set_selected_index("tabs", 0)

        dialog.add_control("label", "collection-heading", "Choice and collection controls",
            {x = 270, y = 28, width = 240, height = 12})
        dialog.add_control("combobox", "choice", "",
            {x = 270, y = 42, width = 240, height = 80})
        for _, item in ipairs({"Salamatrix UI", "Native Win32 controls", "Runtime-neutral API"}) do
            dialog.add_item("choice", item)
        end
        dialog.set_selected_index("choice", 0)
        dialog.add_control("listview", "list", "",
            {x = 270, y = 70, width = 240, height = 78})
        dialog.add_column("list", "Capability", 210)
        for _, item in ipairs({"Explicit layouts", "Validation and events", "Accessible metadata"}) do
            dialog.add_item("list", item)
        end
        dialog.set_selected_index("list", 0)
        dialog.add_control("treeview", "tree", "",
            {x = 270, y = 158, width = 240, height = 86})
        dialog.add_item("tree", "Salamatrix UI")
        dialog.add_item("tree", "Dialogs", 0)
        dialog.add_item("tree", "Controls", 0)
        dialog.add_control("button", "close", "Close",
            {dialog_result = 1, x = 440, y = 276, width = 70, height = 22})
        dialog.show()
    end)
    dialog.close()
    if not shown then error(dialog_failure) end
end
