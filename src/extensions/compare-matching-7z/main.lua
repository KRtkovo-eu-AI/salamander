local function lower_name(name)
    return string.lower(name or "")
end

local function is_7z(item)
    return not item.isDirectory and lower_name(item.name):match("%.7z$") ~= nil
end

local function select_all(side)
    if not Salamander.sides.select_all(true, side, false) then
        error("Nelze označit všechny položky na straně " .. side .. ".")
    end

    local context = Salamander.sides.context(side)
    if context.selectedCount > #context.selectedItems then
        error("Strana " .. side .. " obsahuje více položek, než lze načíst najednou.")
    end
    return context
end

local function select_only_matching(context, side, matching)
    -- After select_all(), selectedItems are in Salamander's panel order.
    -- With no directories, the first file has panel index 0.
    for panel_index, item in ipairs(context.selectedItems) do
        local keep = is_7z(item) and matching[lower_name(item.name)] == true
        Salamander.sides.select_item(panel_index - 1, keep, side, false)
    end
end

local function compare_files()
    local source = select_all("source")
    local target = select_all("target")
    local source_names = {}
    local target_names = {}

    for _, item in ipairs(source.selectedItems) do
        if is_7z(item) then
            source_names[lower_name(item.name)] = true
        end
    end
    for _, item in ipairs(target.selectedItems) do
        if is_7z(item) then
            target_names[lower_name(item.name)] = true
        end
    end

    local matching = {}
    local count = 0
    for name in pairs(source_names) do
        if target_names[name] then
            matching[name] = true
            count = count + 1
        end
    end

    select_only_matching(source, "source", matching)
    select_only_matching(target, "target", matching)
    Salamander.ui.notify(
        string.format("Označeno shodných 7z archivů: %d", count),
        "Porovnání názvů 7z",
        4000)
end

if Salamander.command_handler == "run" then
    local ok, message = pcall(compare_files)
    if not ok then
        Salamander.ui.message_box(tostring(message), "Porovnání názvů 7z")
    end
end
