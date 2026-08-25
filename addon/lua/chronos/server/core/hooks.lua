hook.Add("Tick", "chronos_tick", function()
    if not CHRONOS.BenchClock then return CHRONOS.RunTick() end

    local began = SysTime()
    CHRONOS.RunTick()
    chronos.BenchMark("lua.tick", (SysTime() - began) * 1000)
end)

timer.Create("chronos_broadcast", 0.1, 0, function()
    if CHRONOS.Mode == "recording" then
        CHRONOS.PruneEvents()
        CHRONOS.PruneAim(chronos.GetRange() or 0)
    end

    if CHRONOS.Mode ~= "idle" or CHRONOS.Stage then
        CHRONOS.Broadcast()
    end
end)

-- Someone joining mid-scrub would otherwise be the one thing still moving.
hook.Add("PlayerInitialSpawn", "chronos_join", function(ply)
    if CHRONOS.Mode == "replay" then
        CHRONOS.SetSpectator(ply, false)
    end
end)

hook.Add("PlayerDisconnected", "chronos_cleanup", function(ply)
    chronos.SetIgnore(ply:EntIndex(), false)
    CHRONOS.LeaveStage(ply)
end)

-- A stand-in removed by anything other than the stage itself must drop out of
-- the stage tables, or the next sync drives a dead entity forever.
hook.Add("EntityRemoved", "chronos_stage_gone", function(ent)
    local index = ent.ChronosStage
    if not index or not CHRONOS.Stage then return end

    -- The edict goes straight back to the live world, so the binding has to go
    -- with it or the next restore writes recorded bytes into whatever takes it.
    chronos.BindProxy(index, -1)
    chronos.SetSkip(ent:EntIndex(), false)
    chronos.SetIgnore(ent:EntIndex(), false)

    CHRONOS.Stage.Clones[index] = nil
    CHRONOS.Stage.Puppets[index] = nil
    CHRONOS.Stage.Tried[index] = nil
end)

hook.Add("ShutDown", "chronos_shutdown", function()
    CHRONOS.StageTeardown()

    if CHRONOS.Mode == "replay" then
        CHRONOS.ExitReplay()
    end

    chronos.Stop()
    chronos.Clear()
end)

-- A ghost that gets removed by anything other than ExitReplay must drop its
-- proxy binding, or restores keep writing into a dead edict index.
hook.Add("EntityRemoved", "chronos_body_gone", function(ent)
    if not ent.ChronosBody then return end

    for index, body in pairs(CHRONOS.Bodies) do
        if body == ent then CHRONOS.Bodies[index] = nil end
    end
end)

hook.Add("EntityRemoved", "chronos_ghost_gone", function(ent)
    local index = ent.ChronosGhost
    if index then
        CHRONOS.Ghosts[index] = nil
    end
end)
