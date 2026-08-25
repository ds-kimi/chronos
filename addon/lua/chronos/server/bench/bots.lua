---@return Player[] every fake client currently connected
function CHRONOS.BenchBots()
    local bots = {}
    for _, ply in ipairs(player.GetAll()) do
        if ply:IsBot() then bots[#bots + 1] = ply end
    end

    return bots
end

---The console command is deferred and silently does nothing once maxplayers is
---reached, so nothing here can report how many arrived. BenchEndWarmup counts
---them once they have actually connected.
---@param count number
function CHRONOS.BenchAddBots(count)
    for _ = 1, count do
        RunConsoleCommand("bot")
    end
end

function CHRONOS.BenchClearBots()
    for _, ply in ipairs(CHRONOS.BenchBots()) do
        ply:Kick("chronos bench finished")
    end
end
