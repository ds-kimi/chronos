---@type table<string, fun(ply:Player|nil, args:string[])>
CHRONOS.Actions = CHRONOS.Actions or {}

function CHRONOS.Actions.record()
    if CHRONOS.Mode == "replay" then
        CHRONOS.ExitReplay()
    end

    -- Every stage stand-in is driven from the ring that is about to be dropped,
    -- and any audio Auris is holding was spoken before frame zero exists.
    CHRONOS.StageTeardown()
    CHRONOS.DropVoiceBuffers()

    chronos.Clear()
    CHRONOS.ClearEvents()
    CHRONOS.ClearAim()
    chronos.ClearEffects()
    chronos.ClearClips()
    chronos.Start()
    CHRONOS.StampWorld()
    CHRONOS.Mode = "recording"
    CHRONOS.Broadcast()
    print("[chronos] recording started")
end

---Stopping does not hand the world back: it freezes at the newest recorded tick
---and stays there until exit, so nothing drifts before it can be reviewed.
function CHRONOS.Actions.stop()
    chronos.Stop()
    print("[chronos] recording stopped, " .. tostring(select(3, chronos.GetRange()) or 0) .. " frames kept")

    if CHRONOS.Mode == "recording" and not CHRONOS.EnterReplay(true) then
        CHRONOS.Mode = "idle"
    end

    CHRONOS.Broadcast()
end

---Stops the capture and leaves the world alone. The normal stop freezes the
---server so the tail can be reviewed; experimental mode reviews privately and
---must not touch anybody, so it stops with this instead.
function CHRONOS.Actions.stoponly()
    chronos.Stop()
    CHRONOS.Mode = "idle"
    print("[chronos] recording stopped, " .. tostring(select(3, chronos.GetRange()) or 0) .. " frames kept")
    CHRONOS.Broadcast()
end

function CHRONOS.Actions.clear()
    CHRONOS.StageTeardown()
    CHRONOS.DropVoiceBuffers()
    chronos.Clear()
    CHRONOS.ClearEvents()
    CHRONOS.ClearAim()
    chronos.ClearEffects()
    chronos.ClearClips()
    CHRONOS.Cursor = 0
    CHRONOS.Broadcast()
end

function CHRONOS.Actions.replay()
    if not CHRONOS.EnterReplay() then
        print("chronos: nothing recorded")
    end
end
