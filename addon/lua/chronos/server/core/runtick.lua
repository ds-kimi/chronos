---The whole per-tick body, lifted out of the hook so the benchmark can time it
---as one span. The native calls inside it account for themselves, and the
---module subtracts them, leaving lua.tick as the Lua work alone.
function CHRONOS.RunTick()
    if CHRONOS.Mode == "recording" then
        CHRONOS.RecTick = CHRONOS.RecTick + 1
        chronos.Capture(CurTime())
        CHRONOS.CaptureAim(CHRONOS.RecTick)
        CHRONOS.MarkSpeakers()
    end

    -- A stage replay runs alongside the recording rather than in place of it,
    -- so the capture above still happened on this tick.
    if CHRONOS.Stage then return CHRONOS.StageTick() end
    if CHRONOS.Mode ~= "replay" then return end

    local previous = CHRONOS.Cursor
    CHRONOS.AdvanceCursor()

    if CHRONOS.Playing then
        CHRONOS.PlayEvents(previous, CHRONOS.Cursor)
    end

    -- Rebuilding the manifest is far more expensive than a restore, so the
    -- entity population only resyncs on a jump or every SyncEvery ticks.
    if math.abs(CHRONOS.Cursor - CHRONOS.LastSync) >= CHRONOS.SyncEvery then
        CHRONOS.LastSync = CHRONOS.Cursor
        CHRONOS.SyncWorld(CHRONOS.Cursor)
    end

    chronos.Restore(CHRONOS.Cursor)
    CHRONOS.PinPlayers()
    CHRONOS.RestoreAim(CHRONOS.Cursor)
    CHRONOS.UpdateBodies()
end
