if Salamander.command_handler == "run" then
    local run_count = Salamander.storage.get("runCount", 0) + 1
    Salamander.ui.notify(
        "Lua extension package is running through Salamatrix (run " ..
            tostring(run_count) .. ").",
        "Salamatrix Lua Demo",
        2500)
    Salamander.storage.set("runCount", run_count)
end
