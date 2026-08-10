if Salamander.command_handler == "viewDemo" then
    local path = tostring(Salamander.invocation.path or "")
    local file, open_error = io.open(path, "rb")
    if not file then
        Salamander.ui.message_box(open_error or "Unable to read file.",
            "Salamatrix Lua Viewer demo", "OK", "Error")
    else
        local contents = file:read("*a") or ""
        file:close()
        local preview = contents
        if #contents > 3000 then
            preview = contents:sub(1, 3000) .. "\n\n[preview truncated]"
        end
        Salamander.ui.message_box(
            preview ~= "" and preview or "[empty file]",
            "Salamatrix Lua Viewer demo — " .. path)
    end
    return
end

if Salamander.command_handler == "listDemoMachines" then
    local machines = {
        {id="development", name="Development VM", running=true},
        {id="test-lab", name="Test lab", running=false},
        {id="build-agent", name="Build agent", running=true}}
    for _, machine in ipairs(machines) do
        local running = Salamander.storage.get(
            "machine." .. machine.id .. ".running", machine.running)
        Salamander.file_system.add_item(
            machine.id,
            machine.name .. " — " .. (running and "Running" or "Stopped"),
            {icon="icon.svg", directory=false, enabled=true})
    end
    return
end

if Salamander.command_handler == "inspectDemoMachine" or
   Salamander.command_handler == "toggleDemoMachine" then
    local item = Salamander.invocation.item or {}
    local item_id = tostring(item.id or "unknown")
    local item_name = tostring(item.name or item_id)
    if Salamander.command_handler == "inspectDemoMachine" then
        Salamander.ui.message_box(
            "Id: " .. item_id .. "\nName: " .. item_name,
            "Salamatrix FS item")
    else
        local key = "machine." .. item_id .. ".running"
        local defaults = {
            development=true, ["test-lab"]=false, ["build-agent"]=true}
        local default_running = defaults[item_id]
        if default_running == nil then default_running = false end
        local running = Salamander.storage.get(key, default_running)
        Salamander.storage.set(key, not running)
        Salamander.ui.notify(
            item_name .. ": " .. (not running and "Running" or "Stopped"),
            "Salamatrix FS demo", 2500)
    end
    return
end

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

    -- Build the complete gallery here, through the public runtime-neutral API.
    local dialog = Salamander.ui.dialog("Salamatrix UI capabilities", 463, 236)
    local function add(kind, id, text, x, y, width, height, options)
        options = options or {}; options.x=x; options.y=y
        options.width=width; options.height=height
        dialog.add_control(kind, id, text, options)
    end
    local uptime = "System was started " .. Salamander.ui.uptime() .. " ms ago."
    add("groupbox","static-group","CGUIStaticTextAbstract",6,4,254,108)
    add("label","not-attached-label","Not attached static text",14,17,80,8)
    add("label","uptime-plain",uptime,102,17,152,8)
    local rows = {
        {"static-none","0 (no flags)",uptime,27,0},
        {"static-cache","STF_CACHED_PAINT",uptime,37,1},
        {"static-bold","STF_BOLD","Bold &text",47,0x10082},
        {"static-underline","STF_UNDERLINE","Underlined text",56,0x20004},
        {"static-end","STF_END_ELLIPSIS","Long long long long long long long long long string.",66,0x20},
        {"static-path","STF_PATH_ELLIPSIS","C:\\Program Files\\Some Application With Long Path\\example.exe",76,0x40},
        {"static-path-url","STF_PATH_ELLIPSIS","ftp://ftp.altap.cz/pub/salamander/example.exe",87,0x40}}
    for _, row in ipairs(rows) do
        add("label",row[1].."-label",row[2],14,row[4],75,8)
        local extra={style_flags=row[5]}; if row[1]=="static-path-url" then extra.path_separator="/" end
        add("statictext",row[1],row[3],102,row[4],152,8,extra)
    end
    add("label","drag-hint","Drag texts to change their size.",151,97,103,8)
    add("groupbox","progress-group","CGUIProgressBarAbstract",6,118,254,66)
    add("label","progress-label","Progress label",15,129,60,8)
    add("progressbar","progress","",15,138,235,12,{progress=120})
    add("label","unknown-label","Unknown progress",15,154,67,8)
    add("progressbar","unknown-progress","",15,163,235,12,{progress=-1,indeterminate_duration=-1,indeterminate_interval=100})
    add("groupbox","buttons-group","Button, CGUITextArrowButtonAbstract, CGUIColorArrowButtonAbstract",6,188,254,40)
    add("button","more","...",15,204,15,14,{keep_open=true})
    add("arrowbutton","arrow","",37,204,15,14)
    add("textarrowbutton","choose","&Choose",60,204,50,14,{style_flags=8})
    add("textarrowbutton","drop","&Drop",117,204,50,14,{style_flags=2})
    add("colorarrowbutton","color","",174,204,33,14,{text_color=0xff8000,background_color=0xff8000})
    add("colorarrowbutton","color-text","ABC",215,204,33,14,{text_color=0,background_color=0xffff})
    add("groupbox","hyperlink-group","CGUIHyperLinkAbstract",269,4,185,48)
    add("label","open-label","SetActionOpen",277,17,75,8)
    add("hyperlink","open-link","www.altap.cz",365,17,47,8,{style_flags=0x14,action_open="https://www.altap.cz"})
    add("label","command-label","SetActionPostCommand",277,27,81,8)
    add("hyperlink","command-link","Say something!",365,27,55,8,{style_flags=0x14,action_command=0x7f01})
    add("label","hint-label","SetActionShowHint",277,37,81,8)
    add("hyperlink","hint-link","mask hints",365,37,40,8,{style_flags=8,action_hint="text 1 text 1 text 1 text 1\ntext 2 text 2 text 2"})
    add("groupbox","tooltip-group","SetCurrentToolTip",269,59,185,31)
    add("statictext","tooltip","Pause the mouse pointer over this text.",278,73,130,8,{style_flags=0x40000,tool_tip="ToolTip"})
    add("listview","header-list","",269,113,185,50,{style_flags=0x01e00000})
    add("toolbarheader","toolbar-header","CGUIToolbarHeaderAbstract",269,102,96,8,{align_control_id="header-list",button_mask=0x31})
    add("groupbox","origin-group","Created by",269,169,185,38)
    add("label","runtime-label","Runtime:",277,181,42,8)
    add("statictext","runtime-value","Lua",323,181,122,8,{style_flags=2})
    add("label","extension-label","Extension:",277,192,42,8)
    add("statictext","extension-value","Salamatrix Lua Demo",323,192,122,8,{style_flags=2})
    add("button","close","Close",403,213,50,14,{dialog_result=1,style_flags=0x100000})
    local shown, show_error = pcall(dialog.show)
    dialog.close()
    if not shown then error(show_error) end
end
