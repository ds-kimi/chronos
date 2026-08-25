---@param args string[]
function CHRONOS.Actions.speed(_, args)
    CHRONOS.Speed = math.Clamp(tonumber(args[1]) or 1, 0.05, 8)
    CHRONOS.Broadcast()
end

---@param ply Player|nil
---@param args string[]
function CHRONOS.Actions.spectate(ply, args)
    if not IsValid(ply) then return end

    local spectate = tonumber(args[1]) ~= 0
    CHRONOS.SetSpectator(ply, spectate)
end

function CHRONOS.Actions.stats(ply)
    local stats = chronos.GetStats()
    local first, last = chronos.GetRange()
    local line = string.format("chronos: %d frames, %.1f MB / %.0f MB, ticks %d-%d, %d events",
        stats.frames, stats.bytes / 1048576, stats.cap / 1048576, first or 0, last or 0,
        CHRONOS.EventCount + chronos.EffectCount())

    if IsValid(ply) then
        ply:PrintMessage(HUD_PRINTCONSOLE, line)
    else
        print(line)
    end
end

function CHRONOS.Actions.help(ply)
    local lines = {
        "chronos record | stop | clear | stats",
        "chronos replay  freeze the WHOLE server so everyone watches together",
        "chronos play | pause | exit | seek <tick> | speed <n>",
        "chronos review | leave | stoponly  EXPERIMENTAL private replay",
        "chronos spectate <0|1> | ghosts <0|1> | viewlock <0|1>",
        "chronos memcap <mb> | keyinterval <ticks>",
        "chronos voiceport <port> | voicehost <ip> | voicecap <mb>",
        "chronos voicestats | voicetest | voicedebug <0|1> | voicechain",
        "chronos diag  print recording and stage state",
        "chronos bench run|stop|now|on|off|reset  profile the server under load",
        "chronos_ui opens the scrubber"
    }

    for _, line in ipairs(lines) do
        if IsValid(ply) then ply:PrintMessage(HUD_PRINTCONSOLE, line) else print(line) end
    end
end
