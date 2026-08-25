-- Only classes that spawn cleanly from a class name plus a model are ghosted.
-- Anything else needs its own keyvalues and would come back broken.
local GHOSTABLE = {
    prop_physics = true,
    prop_physics_multiplayer = true,
    prop_physics_override = true,
    prop_dynamic = true,
    prop_dynamic_override = true,
    prop_ragdoll = true
}

CHRONOS.Ghosts = CHRONOS.Ghosts or {}
CHRONOS.MaxGhosts = 256

---@param index number Recorded edict index
---@param info table { class = string, model = string }
---@return boolean spawned
function CHRONOS.SpawnGhost(index, info)
    if not GHOSTABLE[info.class] then return false end
    if table.Count(CHRONOS.Ghosts) >= CHRONOS.MaxGhosts then return false end
    if not info.model or info.model == "" then return false end
    if not util.IsValidModel(info.model) then return false end

    -- The entity that carried this model may have been gone since long before
    -- the replay opened, and spawning a prop on an unprecached model is an
    -- engine crash rather than a Lua error.
    util.PrecacheModel(info.model)

    local ghost = ents.Create(info.class)
    if not IsValid(ghost) then return false end

    ghost:SetModel(info.model)
    ghost:Spawn()
    ghost.ChronosGhost = index

    -- No physics object at all: a ghost is driven by the module, and one that
    -- keeps a live physics object under a collision property the restore no
    -- longer writes is the pair that crashes the server inside a later spawn.
    ghost:PhysicsDestroy()
    ghost:SetSolid(SOLID_NONE)
    ghost:SetMoveType(MOVETYPE_NONE)
    ghost:SetCollisionGroup(COLLISION_GROUP_IN_VEHICLE)

    -- Which occupant of this index the ghost stands for. The index alone gets
    -- handed to the next spawn, and a ghost left bound across that handover
    -- draws the dead entity on top of the live one.
    ghost.ChronosBorn = info.born

    chronos.BindProxy(index, ghost:EntIndex())
    CHRONOS.Ghosts[index] = ghost
    return true
end

---Recreates recorded entities whose original edict is gone or now holds a
---different class, so the replay is not silently missing everything that died.
---@param tick number
---@return table<number, boolean> seen Recorded indices present at this tick
function CHRONOS.SyncGhosts(tick)
    local manifest = chronos.GetEntities(tick)
    local seen = {}
    CHRONOS.ReleaseImpostors()
    CHRONOS.DropStaleGhosts(manifest)

    for index, info in pairs(manifest) do
        local ghost = CHRONOS.Ghosts[index]
        if IsValid(ghost) then
            -- The recorded index belongs to whatever the live world put there
            -- since; the ghost carries the recording, so only it stays visible.
            seen[ghost:EntIndex()] = true
        elseif CHRONOS.LiveOriginal(index, info, tick) then
            seen[index] = true
        elseif CHRONOS.SpawnGhost(index, info) then
            seen[CHRONOS.Ghosts[index]:EntIndex()] = true
        else
            -- No ghost and not the original: nothing may be written into it,
            -- or the restore lands on a stranger that took the same index.
            chronos.SetIgnore(index, true)
            CHRONOS.Impostors[index] = true
        end
    end

    CHRONOS.HideGhosts(manifest)
    return seen
end

function CHRONOS.ClearGhosts()
    CHRONOS.ReleaseImpostors()

    for index, ghost in pairs(CHRONOS.Ghosts) do
        if IsValid(ghost) then ghost:Remove() end
        CHRONOS.Ghosts[index] = nil
    end

    chronos.ClearProxies()
end
