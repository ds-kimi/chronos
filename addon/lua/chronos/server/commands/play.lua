---@param ply Player|nil
function CHRONOS.Actions.exit(ply)
    if CHRONOS.Stage and IsValid(ply) and CHRONOS.Stage.Viewers[ply] then
        CHRONOS.LeaveStage(ply)
        return
    end

    CHRONOS.ExitReplay()
end

---Scrubbing implies pausing: the cursor is being driven by hand until the
---client says otherwise.
---@param args string[]
function CHRONOS.Actions.seek(_, args)
    local first, last = chronos.GetRange()
    if not first then return end

    local target = math.Clamp(math.floor(tonumber(args[1]) or first), first, last)

    -- A seek onto the tick already playing is an echo of a broadcast, not a
    -- scrub, and must not stop playback.
    if target == CHRONOS.Cursor and CHRONOS.Playing then return end

    CHRONOS.Cursor = target
    CHRONOS.Playing = false
    CHRONOS.Accum = 0
    CHRONOS.LastSync = -1e9

    -- The resync is left to the next tick: a slider drag fires seek every
    -- frame, and rebuilding the manifest that often would stall the server.
    -- A stage restores once per tick on its own, so a drag firing seek every
    -- frame must not rebuild the whole recorded state on top of that.
    if CHRONOS.Mode == "replay" and not CHRONOS.Stage then
        chronos.Restore(CHRONOS.Cursor)
    end
end

function CHRONOS.Actions.play()
    if CHRONOS.Mode ~= "replay" and not CHRONOS.Stage then return end

    local _, last = chronos.GetRange()
    if last and CHRONOS.Cursor >= last then
        CHRONOS.Cursor = select(1, chronos.GetRange())
    end

    CHRONOS.Playing = true
    CHRONOS.Broadcast()
end

function CHRONOS.Actions.pause()
    CHRONOS.Playing = false
    CHRONOS.Broadcast()
end
