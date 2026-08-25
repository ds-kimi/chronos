CHRONOS.UseGhosts = CHRONOS.UseGhosts ~= false
CHRONOS.LastSync = -1e9
CHRONOS.SyncEvery = 16

---Brings the entity population in line with the scrubbed tick before the
---module writes state, so freshly bound ghosts are filled on the same tick.
---@param tick number
function CHRONOS.SyncWorld(tick)
    if not CHRONOS.UseGhosts then return end

    local seen = CHRONOS.SyncGhosts(tick)
    for _, ply in ipairs(player.GetAll()) do
        seen[ply:EntIndex()] = true
    end

    CHRONOS.SyncVisibility(seen)
end

---Stops recording and pins the world to a tick in the ring.
---@param atEnd boolean|nil Pin to the newest tick instead of the oldest
---@return boolean started
function CHRONOS.EnterReplay(atEnd)
    local first, last = chronos.GetRange()
    if not first then return false end

    -- The two reviews cannot both own the world, and the server-wide one wins:
    -- it is the mode everything else here was built around.
    CHRONOS.StageTeardown()

    chronos.Stop()
    CHRONOS.Mode = "replay"
    CHRONOS.Cursor = atEnd and last or first
    CHRONOS.Playing = false
    CHRONOS.Accum = 0
    CHRONOS.LastSync = -1e9
    CHRONOS.SetFrozen(true)

    for _, ply in ipairs(player.GetAll()) do
        CHRONOS.SetSpectator(ply, ply.ChronosSpectate == true)
    end

    CHRONOS.SendVoiceManifest()
    CHRONOS.Broadcast()
    return true
end

---Handing the world back at whatever tick the cursor happened to sit on left
---every prop somewhere in its own past, unfrozen, often inside geometry: props
---that could not be pushed, and a burst of collision noise the moment physics
---woke up. The newest recorded tick is the last moment the live world was real,
---so the world is settled onto it before anybody gets control back.
local function settle()
    local _, last = chronos.GetRange()
    if not last then return end

    CHRONOS.SyncWorld(last)
    chronos.Restore(last)

    for _, ent in ipairs(ents.GetAll()) do
        local phys = ent:GetPhysicsObject()
        if IsValid(phys) then phys:Wake() end
    end
end

function CHRONOS.ExitReplay()
    settle()
    CHRONOS.Mode = "idle"
    CHRONOS.Playing = false
    CHRONOS.ClearGhosts()
    CHRONOS.ClearBodies()
    CHRONOS.ClearHidden()
    chronos.ClearIgnore()
    CHRONOS.SetFrozen(false)

    for _, ply in ipairs(player.GetAll()) do
        ply.ChronosSpectate = nil
        ply:SetNWBool("ChronosFrozen", false)
        ply:Freeze(false)
        ply:SetMoveType(MOVETYPE_WALK)
    end

    CHRONOS.Broadcast()
end

---Speed is fractional, so whole ticks are drained from an accumulator rather
---than rounding the cursor every frame and losing slow-motion precision.
function CHRONOS.AdvanceCursor()
    local first, last = chronos.GetRange()
    if not first or not CHRONOS.Playing then return end

    CHRONOS.Accum = CHRONOS.Accum + CHRONOS.Speed
    while CHRONOS.Accum >= 1 do
        CHRONOS.Cursor = CHRONOS.Cursor + 1
        CHRONOS.Accum = CHRONOS.Accum - 1
    end

    if CHRONOS.Cursor >= last then
        CHRONOS.Cursor = last
        CHRONOS.Playing = false
        CHRONOS.Broadcast()
    end
end
