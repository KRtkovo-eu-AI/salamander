-- SPDX-FileCopyrightText: 2026 Open Salamander Authors
-- SPDX-License-Identifier: GPL-2.0-or-later

local MAX_FRAME_BYTES = 1024 * 1024
local json_null = {}
local array_mt = {}

local function json_array(value)
    return setmetatable(value or {}, array_mt)
end

local function utf8_char(code)
    if code <= 0x7f then
        return string.char(code)
    elseif code <= 0x7ff then
        return string.char(
            0xc0 + math.floor(code / 0x40),
            0x80 + (code % 0x40))
    elseif code <= 0xffff then
        return string.char(
            0xe0 + math.floor(code / 0x1000),
            0x80 + (math.floor(code / 0x40) % 0x40),
            0x80 + (code % 0x40))
    end
    return string.char(
        0xf0 + math.floor(code / 0x40000),
        0x80 + (math.floor(code / 0x1000) % 0x40),
        0x80 + (math.floor(code / 0x40) % 0x40),
        0x80 + (code % 0x40))
end

local function json_escape(value)
    return value:gsub('[%z\1-\31\\"]', function(ch)
        local escapes = {
            ['"'] = '\\"', ['\\'] = '\\\\', ['\b'] = '\\b',
            ['\f'] = '\\f', ['\n'] = '\\n', ['\r'] = '\\r',
            ['\t'] = '\\t'
        }
        return escapes[ch] or string.format("\\u%04x", string.byte(ch))
    end)
end

local function is_array(value)
    if getmetatable(value) == array_mt then
        return true
    end
    local count = 0
    local maximum = 0
    for key in pairs(value) do
        if type(key) ~= "number" or key < 1 or key % 1 ~= 0 then
            return false
        end
        count = count + 1
        if key > maximum then maximum = key end
    end
    return count > 0 and maximum == count
end

local function encode_json(value, stack)
    if value == json_null or value == nil then return "null" end
    local kind = type(value)
    if kind == "boolean" then return value and "true" or "false" end
    if kind == "number" then
        if value ~= value or value == math.huge or value == -math.huge then
            error("JSON cannot encode a non-finite number")
        end
        return tostring(value)
    end
    if kind == "string" then return '"' .. json_escape(value) .. '"' end
    if kind ~= "table" then
        error("JSON cannot encode " .. kind)
    end
    stack = stack or {}
    if stack[value] then error("JSON cannot encode a table cycle") end
    stack[value] = true
    local parts = {}
    if is_array(value) then
        for index = 1, #value do
            parts[#parts + 1] = encode_json(value[index], stack)
        end
        stack[value] = nil
        return "[" .. table.concat(parts, ",") .. "]"
    end
    for key, item in pairs(value) do
        if type(key) ~= "string" then
            error("JSON object keys must be strings")
        end
        parts[#parts + 1] =
            '"' .. json_escape(key) .. '":' .. encode_json(item, stack)
    end
    table.sort(parts)
    stack[value] = nil
    return "{" .. table.concat(parts, ",") .. "}"
end

local function decode_json(text)
    local position = 1

    local function skip_space()
        local _, last = text:find("^[ \t\r\n]*", position)
        position = (last or position - 1) + 1
    end

    local parse_value

    local function parse_string()
        if text:sub(position, position) ~= '"' then
            error("Expected JSON string")
        end
        position = position + 1
        local output = {}
        local start = position
        while position <= #text do
            local ch = text:sub(position, position)
            if ch == '"' then
                output[#output + 1] = text:sub(start, position - 1)
                position = position + 1
                return table.concat(output)
            end
            if ch == "\\" then
                output[#output + 1] = text:sub(start, position - 1)
                position = position + 1
                local escape = text:sub(position, position)
                local simple = {
                    ['"'] = '"', ['\\'] = '\\', ['/'] = '/',
                    b = '\b', f = '\f', n = '\n', r = '\r', t = '\t'
                }
                if simple[escape] then
                    output[#output + 1] = simple[escape]
                    position = position + 1
                elseif escape == "u" then
                    local hex = text:sub(position + 1, position + 4)
                    if not hex:match("^%x%x%x%x$") then
                        error("Invalid JSON Unicode escape")
                    end
                    local code = tonumber(hex, 16)
                    position = position + 5
                    if code >= 0xd800 and code <= 0xdbff and
                       text:sub(position, position + 1) == "\\u" then
                        local low_hex = text:sub(position + 2, position + 5)
                        local low = tonumber(low_hex, 16)
                        if low and low >= 0xdc00 and low <= 0xdfff then
                            code = 0x10000 +
                                (code - 0xd800) * 0x400 + (low - 0xdc00)
                            position = position + 6
                        end
                    end
                    output[#output + 1] = utf8_char(code)
                else
                    error("Invalid JSON escape")
                end
                start = position
            else
                if string.byte(ch) < 32 then
                    error("Control character in JSON string")
                end
                position = position + 1
            end
        end
        error("Unterminated JSON string")
    end

    local function parse_array()
        position = position + 1
        skip_space()
        local result = json_array()
        if text:sub(position, position) == "]" then
            position = position + 1
            return result
        end
        while true do
            result[#result + 1] = parse_value()
            skip_space()
            local ch = text:sub(position, position)
            if ch == "]" then
                position = position + 1
                return result
            end
            if ch ~= "," then error("Expected ',' or ']' in JSON array") end
            position = position + 1
            skip_space()
        end
    end

    local function parse_object()
        position = position + 1
        skip_space()
        local result = {}
        if text:sub(position, position) == "}" then
            position = position + 1
            return result
        end
        while true do
            local key = parse_string()
            skip_space()
            if text:sub(position, position) ~= ":" then
                error("Expected ':' in JSON object")
            end
            position = position + 1
            skip_space()
            result[key] = parse_value()
            skip_space()
            local ch = text:sub(position, position)
            if ch == "}" then
                position = position + 1
                return result
            end
            if ch ~= "," then error("Expected ',' or '}' in JSON object") end
            position = position + 1
            skip_space()
        end
    end

    function parse_value()
        skip_space()
        local ch = text:sub(position, position)
        if ch == '"' then return parse_string() end
        if ch == "{" then return parse_object() end
        if ch == "[" then return parse_array() end
        local literals = {
            ["true"] = true, ["false"] = false, ["null"] = json_null
        }
        for literal, value in pairs(literals) do
            if text:sub(position, position + #literal - 1) == literal then
                position = position + #literal
                return value
            end
        end
        local number = text:match(
            "^-?%d+%.?%d*[eE]?[+-]?%d*", position)
        if number and number ~= "" then
            local value = tonumber(number)
            if value == nil then error("Invalid JSON number") end
            position = position + #number
            return value
        end
        error("Invalid JSON value")
    end

    local result = parse_value()
    skip_space()
    if position <= #text then error("Trailing data after JSON value") end
    return result
end

local options = {
    entry = nil,
    command_id = "",
    command_handler = "",
    invocation_json = "{}",
    invocation_json_base64 = nil,
    one_shot = false
}
local index = 1
while index <= #arg do
    local name = arg[index]
    if name == "--entry" or name == "--command-id" or
       name == "--command-handler" or name == "--invocation-json" or
       name == "--invocation-json-base64" then
        if index == #arg then error("Missing value for " .. name) end
        local key = name:sub(3):gsub("-", "_")
        options[key] = arg[index + 1]
        index = index + 2
    elseif name == "--one-shot" then
        options.one_shot = true
        index = index + 1
    else
        error("Unknown worker argument: " .. tostring(name))
    end
end
if not options.entry or options.entry == "" then
    error("The Lua worker requires --entry")
end

local function decode_base64(value)
    if type(value) ~= "string" or #value % 4 ~= 0 then
        error("Invalid Base64 invocation JSON")
    end
    local alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
    local decoded = {}
    for offset = 1, #value, 4 do
        local chunk = value:sub(offset, offset + 3)
        local numbers = {}
        local padding = 0
        for index = 1, 4 do
            local character = chunk:sub(index, index)
            if character == "=" then
                numbers[index] = 0
                padding = padding + 1
            else
                local position = alphabet:find(character, 1, true)
                if not position or padding ~= 0 then
                    error("Invalid Base64 invocation JSON")
                end
                numbers[index] = position - 1
            end
        end
        if padding > 2 or (padding ~= 0 and offset + 3 ~= #value) then
            error("Invalid Base64 invocation JSON")
        end
        decoded[#decoded + 1] = string.char(
            numbers[1] * 4 + math.floor(numbers[2] / 16))
        if padding < 2 then
            decoded[#decoded + 1] = string.char(
                (numbers[2] % 16) * 16 + math.floor(numbers[3] / 4))
        end
        if padding == 0 then
            decoded[#decoded + 1] = string.char(
                (numbers[3] % 4) * 64 + numbers[4])
        end
    end
    return table.concat(decoded)
end

if options.invocation_json_base64 then
    options.invocation_json = decode_base64(options.invocation_json_base64)
end

local function send_frame(kind, id, payload)
    local frame = "SMX1\t" .. kind .. "\t" .. tostring(id) ..
        "\t" .. encode_json(payload)
    if #frame + 1 > MAX_FRAME_BYTES then
        error("SMX1 frame exceeds the 1 MiB limit")
    end
    io.stdout:write(frame, "\n")
    io.stdout:flush()
end

local function read_frame()
    local line = io.stdin:read("*l")
    if line == nil then error("Salamander host closed the worker channel") end
    if #line + 1 > MAX_FRAME_BYTES then
        error("SMX1 frame exceeds the 1 MiB limit")
    end
    local kind, id, payload =
        line:match("^SMX1\t([^\t]+)\t(%d+)\t(.*)$")
    if not kind then error("Invalid SMX1 frame") end
    return {kind = kind, id = tonumber(id), payload = decode_json(payload)}
end

local event_handlers = {}
local function dispatch_event(payload)
    local handlers = event_handlers[payload.event or payload.name]
    if not handlers then return end
    for _, handler in ipairs(handlers) do handler(payload) end
end

local next_request_id = 1
local function host_call(method, arguments)
    local id = next_request_id
    next_request_id = next_request_id + 1
    arguments = arguments or {}
    arguments.method = method
    send_frame("call", id, arguments)
    while true do
        local frame = read_frame()
        if frame.kind == "event" then
            dispatch_event(frame.payload)
        elseif frame.id == id then
            if frame.kind == "error" or frame.payload.ok == false then
                error(tostring(frame.payload.error or "Salamander host call failed"))
            end
            if frame.kind ~= "result" then
                error("Unexpected SMX1 response: " .. tostring(frame.kind))
            end
            return frame.payload
        end
    end
end

send_frame("hello", 0, {protocol = 1, runtime = "lua"})
local hello
repeat
    hello = read_frame()
    if hello.kind == "event" then dispatch_event(hello.payload) end
until hello.kind == "result" and hello.id == 0
if hello.payload.ok == false then error("Salamander host rejected the worker") end

local commands = {}
function commands.execute(command_id)
    return host_call("salamander.commands.execute",
        {commandId = command_id}).result
end
function commands.register(command_id, title, options)
    options = options or {}
    return host_call("salamander.commands.register", {
        commandId = command_id, title = title,
        pluginMenu = options.plugin_menu ~= false,
        contextMenu = options.context_menu == true,
        hotKey = options.hot_key or 0,
        toolbar = options.toolbar == true,
        handler = options.handler or "",
        enabled = options.enabled ~= false,
        visible = options.visible ~= false
    }).registered
end
function commands.unregister(command_id)
    return host_call("salamander.commands.unregister",
        {commandId = command_id}).unregistered
end
function commands.set_state(command_id, enabled, visible)
    local arguments = {commandId = command_id}
    if enabled ~= nil then arguments.enabled = enabled end
    if visible ~= nil then arguments.visible = visible end
    return host_call("salamander.commands.setState", arguments).updated
end

local storage = {}
function storage.get(key, default)
    local result = host_call("salamander.storage.get", {key = key})
    if result.type == "string" or result.type == "integer" or
       result.type == "boolean" then
        return result.value
    end
    return default
end
function storage.set(key, value)
    host_call("salamander.storage.set", {key = key, value = value})
end
function storage.remove(key)
    return host_call("salamander.storage.remove", {key = key}).removed
end
function storage.clear()
    return host_call("salamander.storage.clear", {}).ok
end
function storage.schema()
    return host_call("salamander.storage.schema", {}).settings or json_array()
end
function storage.keys()
    return host_call("salamander.storage.keys", {}).keys or json_array()
end

local file_operations = {}
for lua_name, host_name in pairs({
    rename = "rename", copy = "copy", move = "move", delete = "delete",
    create_directory = "createDirectory", refresh = "refresh",
    properties = "properties"
}) do
    file_operations[lua_name] = function()
        return host_call("salamander.fileOperations." .. host_name, {}).result
    end
end

local file_system = {}
function file_system.add_item(id, name, options)
    local arguments = options or {}
    arguments.id = tostring(id)
    arguments.name = tostring(name)
    return host_call("salamander.fileSystem.addItem", arguments).added == true
end
function file_system.add_items(items)
    local result = host_call("salamander.fileSystem.addItems", {items = items or {}})
    return tonumber(result.addedCount) or 0
end

local sides = {}
function sides.active_tab(side)
    return host_call("salamander.sides.activeTab",
        {side = side or "source"})
end
function sides.context(side)
    return host_call("salamander.sides.context",
        {side = side or "source"})
end
function sides.tabs(side)
    return host_call("salamander.sides.tabs",
        {side = side or "source"}).tabs
end
function sides.activate_tab(tab_id, focus)
    return host_call("salamander.sides.activateTab",
        {tabId = tab_id, focus = focus ~= false}).activated
end
function sides.change_path(path, side)
    return host_call("salamander.sides.changePath",
        {side = side or "source", path = path})
end
function sides.refresh(side, force, focus_first_new_item)
    return host_call("salamander.sides.refresh", {
        side = side or "source", force = force == true,
        focusFirstNewItem = focus_first_new_item == true
    }).ok
end
function sides.select_item(item_index, select, side, repaint)
    return host_call("salamander.sides.selectItem", {
        side = side or "source", index = item_index,
        select = select ~= false, repaint = repaint ~= false
    }).changed
end
function sides.select_all(select, side, repaint)
    return host_call("salamander.sides.selectAll", {
        side = side or "source", select = select ~= false,
        repaint = repaint ~= false
    }).changed
end
function sides.focus_item(item_index, side, part_visible)
    return host_call("salamander.sides.focusItem", {
        side = side or "source", index = item_index,
        partVisible = part_visible ~= false
    }).changed
end
function sides.create_tab(side, path, tab_index)
    return host_call("salamander.sides.createTab",
        {side = side or "source", path = path or json_null, index = tab_index})
end
function sides.close_tab(tab_id)
    return host_call("salamander.sides.closeTab", {tabId = tab_id}).ok
end
function sides.reorder_tab(tab_id, tab_index)
    return host_call("salamander.sides.reorderTab",
        {tabId = tab_id, index = tab_index}).ok
end
function sides.move_tab(tab_id, side, tab_index)
    return host_call("salamander.sides.moveTab", {
        tabId = tab_id, side = side or "source", index = tab_index
    }).ok
end
function sides.set_detached(detached)
    return host_call("salamander.sides.setDetached",
        {detached = detached == true}).ok
end

local function side_view(name)
    return {
        active_tab = function() return sides.active_tab(name) end,
        context = function() return sides.context(name) end,
        tabs = function() return sides.tabs(name) end,
        activate_tab = sides.activate_tab,
        change_path = function(path) return sides.change_path(path, name) end,
        refresh = function(force, focus)
            return sides.refresh(name, force, focus)
        end,
        select_item = function(item_index, select, repaint)
            return sides.select_item(item_index, select, name, repaint)
        end,
        select_all = function(select, repaint)
            return sides.select_all(select, name, repaint)
        end,
        focus_item = function(item_index, visible)
            return sides.focus_item(item_index, name, visible)
        end,
        create_tab = function(path, tab_index)
            return sides.create_tab(name, path, tab_index)
        end,
        close_tab = sides.close_tab,
        reorder_tab = sides.reorder_tab,
        move_tab = sides.move_tab,
        set_detached = sides.set_detached
    }
end

local ui = {}
function ui.message_box(message, title, buttons, icon)
    return host_call("salamander.ui.messageBox", {
        message = message, title = title or "Salamander",
        buttons = buttons or "OK", icon = icon or "Information"
    }).result
end
function ui.notify(message, title, timeout_ms)
    return host_call("salamander.ui.notify", {
        message = message, title = title or "Salamander",
        timeoutMs = math.max(0, timeout_ms or 5000)
    }).shown
end
function ui.controls()
    return host_call("salamander.ui.controls", {}).shown
end
function ui.file_properties(path)
    return host_call("salamander.ui.fileProperties", {
        path = tostring(path)
    })
end
function ui.viewer(path, renderer)
    return host_call("salamander.ui.viewer.open", {
        path = tostring(path), renderer = renderer or "auto"
    }).opened
end
function ui.uptime() return tostring(host_call("salamander.host.uptime", {}).milliseconds) end
function ui.input_box(prompt, title, initial)
    return host_call("salamander.ui.inputBox", {
        prompt = prompt, title = title or "Salamander", initial = initial or ""
    })
end
function ui.pick_file(save, title, filter, initial)
    return host_call("salamander.ui.pickFile", {
        save = save == true, title = title or "", filter = filter or "",
        initial = initial or ""
    })
end
function ui.pick_folder(title, initial)
    return host_call("salamander.ui.pickFolder",
        {title = title or "", initial = initial or ""})
end

function ui.progress(title, total, options)
    options = options or {}
    local created = host_call("salamander.ui.progress.create", {
        title = title or "Salamatrix", total = total or 0,
        twoProgressBars = options.two_progress_bars == true,
        fileProgress = options.file_progress == true,
        cancelEnabled = options.cancel_enabled ~= false,
        total2 = options.total2
    })
    local progress = {id = created.progressId, closed = false}
    function progress.update(position, update)
        update = update or {}
        return host_call("salamander.ui.progress.update", {
            progressId = progress.id, position = position,
            total = update.total, text = update.text or "",
            delayedPaint = update.delayed_paint ~= false,
            position2 = update.position2, total2 = update.total2
        }).continued
    end
    function progress.step(amount, delayed_paint)
        return host_call("salamander.ui.progress.step", {
            progressId = progress.id, amount = amount or 1,
            delayedPaint = delayed_paint ~= false
        }).continued
    end
    function progress.set_totals(total1, total2)
        host_call("salamander.ui.progress.setTotals",
            {progressId = progress.id, total = total1, total2 = total2})
    end
    function progress.set_positions(position1, position2, delayed_paint)
        return host_call("salamander.ui.progress.setPositions", {
            progressId = progress.id, position = position1,
            position2 = position2, delayedPaint = delayed_paint ~= false
        }).continued
    end
    function progress.set_title(value)
        host_call("salamander.ui.progress.setTitle",
            {progressId = progress.id, title = value})
    end
    function progress.set_cancel_enabled(enabled)
        host_call("salamander.ui.progress.setCancelEnabled",
            {progressId = progress.id, enabled = enabled == true})
    end
    function progress.is_cancelled()
        return host_call("salamander.ui.progress.cancelled",
            {progressId = progress.id}).cancelled
    end
    function progress.close()
        if not progress.closed then
            host_call("salamander.ui.progress.close", {progressId = progress.id})
            progress.closed = true
        end
    end
    return progress
end

function ui.dialog(title, width, height, resizable)
    local created = host_call("salamander.ui.dialog.create", {
        title = title or "Salamander", width = width or 320,
        height = height or 180, resizable = resizable == true
    })
    local dialog = {id = created.dialogId}
    function dialog.add_control(kind, control_id, text, options)
        options = options or {}
        host_call("salamander.ui.dialog.add", {
            dialogId = dialog.id, kind = kind, controlId = control_id,
            text = text or "", readOnly = options.read_only == true,
            checked = options.checked == true,
            dialogResult = options.dialog_result or 0,
            keepOpen = options.keep_open == true,
            multiline = options.multiline == true,
            filter = options.filter, save = options.save,
            x = options.x, y = options.y, width = options.width,
            height = options.height, styleFlags = options.style_flags,
            pathSeparator = options.path_separator, toolTip = options.tool_tip,
            actionOpen = options.action_open, actionCommand = options.action_command,
            actionHint = options.action_hint, progress = options.progress,
            progressCurrent = options.progress_current,
            progressTotal = options.progress_total,
            progressText = options.progress_text,
            indeterminateDuration = options.indeterminate_duration,
            indeterminateInterval = options.indeterminate_interval,
            textColor = options.text_color,
            backgroundColor = options.background_color,
            alignControlId = options.align_control_id,
            buttonMask = options.button_mask
        })
    end
    function dialog.set_validation(control_id, required, message)
        host_call("salamander.ui.dialog.validation", {
            dialogId = dialog.id, controlId = control_id,
            required = required == true, message = message or ""
        })
    end
    function dialog.on_change(handler)
        local event = "salamander.ui.dialog." .. dialog.id .. ".changed"
        event_handlers[event] = event_handlers[event] or {}
        event_handlers[event][#event_handlers[event] + 1] = handler
        host_call("salamander.ui.dialog.events",
            {dialogId = dialog.id, enabled = true, event = event})
        return event
    end
    function dialog.off_change(event)
        event = event or
            ("salamander.ui.dialog." .. dialog.id .. ".changed")
        host_call("salamander.ui.dialog.events",
            {dialogId = dialog.id, enabled = false, event = event})
        event_handlers[event] = nil
    end
    function dialog.add_column(control_id, title_value, column_width)
        host_call("salamander.ui.dialog.column", {
            dialogId = dialog.id, controlId = control_id,
            title = title_value, width = column_width or 180
        })
    end
    function dialog.set_selected_index(control_id, selected_index)
        return host_call("salamander.ui.dialog.selection", {
            dialogId = dialog.id, controlId = control_id,
            index = selected_index or -1
        }).selectedIndex
    end
    function dialog.add_label(id, text)
        dialog.add_control("label", id, text)
    end
    function dialog.add_text_box(id, text, read_only, multiline)
        dialog.add_control("textbox", id, text, {
            read_only = read_only, multiline = multiline
        })
    end
    function dialog.add_folder_picker(id, path)
        dialog.add_control("folderpicker", id, path)
    end
    function dialog.add_file_picker(id, path, filter, save)
        dialog.add_control("filepicker", id, path,
            {filter = filter, save = save})
    end
    function dialog.add_check_box(id, text, checked)
        dialog.add_control("checkbox", id, text, {checked = checked})
    end
    function dialog.add_radio_button(id, text, checked)
        dialog.add_control("radio", id, text, {checked = checked})
    end
    function dialog.add_combo_box(id, text)
        dialog.add_control("combobox", id, text)
    end
    function dialog.add_list_view(id) dialog.add_control("listview", id) end
    function dialog.add_tree_view(id) dialog.add_control("treeview", id) end
    function dialog.add_tab_control(id) dialog.add_control("tabcontrol", id) end
    function dialog.add_item(control_id, text, parent_index)
        return host_call("salamander.ui.dialog.item", {
            dialogId = dialog.id, controlId = control_id, text = text,
            parentIndex = parent_index or -1
        }).itemCount
    end
    function dialog.clear_items(control_id)
        host_call("salamander.ui.dialog.clearItems",
            {dialogId = dialog.id, controlId = control_id})
    end
    function dialog.add_button(id, text, result, keep_open)
        dialog.add_control("button", id, text, {
            dialog_result = result or 1, keep_open = keep_open
        })
    end
    function dialog.show()
        return host_call("salamander.ui.dialog.show",
            {dialogId = dialog.id}).result
    end
    function dialog.get(control_id)
        return host_call("salamander.ui.dialog.get",
            {dialogId = dialog.id, controlId = control_id})
    end
    function dialog.set(control_id, value)
        host_call("salamander.ui.dialog.set", {
            dialogId = dialog.id, controlId = control_id, value = value
        })
    end
    function dialog.close()
        host_call("salamander.ui.dialog.destroy", {dialogId = dialog.id})
    end
    return dialog
end

local clipboard = {}
function clipboard.copy_text(text, show_echo)
    return host_call("salamander.clipboard.copyText",
        {text = text, showEcho = show_echo == true}).copied
end

local ai = {}
function ai.api(topic)
    return host_call("salamander.ai.api", {topic = topic})
end
ai.api_description = ai.api
local function ai_request(method, prompt, options)
    options = options or {}
    return host_call(method, {
        prompt = prompt, context = options.context,
        provider = options.provider, runtime = options.runtime,
        existingScript = options.existing_script,
        feedback = options.feedback
    })
end
function ai.generate(prompt, options)
    return ai_request("salamander.ai.generate", prompt, options)
end
function ai.preview(prompt, options)
    return ai_request("salamander.ai.preview", prompt, options)
end

local events = {}
function events.subscribe(name, handler)
    event_handlers[name] = event_handlers[name] or {}
    event_handlers[name][#event_handlers[name] + 1] = handler
    return host_call("salamander.events.subscribe",
        {event = name}).subscriptionId
end
function events.unsubscribe(subscription_id)
    host_call("salamander.events.unsubscribe",
        {subscriptionId = subscription_id})
end

local runtimes = {}
function runtimes.list()
    return host_call("salamander.runtimes.list", {}).runtimes
end

local application = {}
function application.language()
    return host_call("salamander.host.language", {})
end
function application.appearance()
    return host_call("salamander.host.appearance", {})
end

Salamander = {
    command_id = options.command_id,
    command_handler = options.command_handler,
    invocation = decode_json(options.invocation_json),
    commands = commands,
    storage = storage,
    file_operations = file_operations,
    file_system = file_system,
    sides = sides,
    left_side = side_view("left"),
    right_side = side_view("right"),
    source_side = side_view("source"),
    target_side = side_view("target"),
    ui = ui,
    clipboard = clipboard,
    ai = ai,
    events = events,
    runtimes = runtimes,
    application = application,
    json_null = json_null
}

-- stdout belongs to SMX1. Route ordinary extension diagnostics away from it.
function print(...)
    local values = {}
    for value_index = 1, select("#", ...) do
        values[value_index] = tostring(select(value_index, ...))
    end
    io.stderr:write(table.concat(values, "\t"), "\n")
    io.stderr:flush()
end

local loaded, load_error = pcall(dofile, options.entry)
if not loaded then
    io.stderr:write("Lua extension failed: ", tostring(load_error), "\n")
    io.stderr:flush()
    os.exit(1)
end
if options.one_shot then os.exit(0) end

while true do
    local frame = read_frame()
    if frame.kind == "event" then
        dispatch_event(frame.payload)
    elseif frame.kind == "shutdown" then
        break
    end
end
