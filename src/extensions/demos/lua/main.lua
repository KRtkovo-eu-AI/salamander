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
end
