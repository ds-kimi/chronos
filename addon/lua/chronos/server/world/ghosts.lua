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

    for index, info in pairs(manifest) do
        seen[index] = true
        local ghost = CHRONOS.Ghosts[index]
        if IsValid(ghost) then
            seen[ghost:EntIndex()] = true
        else
            local live = Entity(index)
            if not IsValid(live) or live:GetClass() ~= info.class then
                if CHRONOS.SpawnGhost(index, info) then
                    seen[CHRONOS.Ghosts[index]:EntIndex()] = true
                end
            end
        end
    end

    return seen
end

function CHRONOS.ClearGhosts()
    for index, ghost in pairs(CHRONOS.Ghosts) do
        if IsValid(ghost) then ghost:Remove() end
        CHRONOS.Ghosts[index] = nil
    end

    chronos.ClearProxies()
end
