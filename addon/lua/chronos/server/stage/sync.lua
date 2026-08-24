---A stand-in's edict index goes back to the live world when it dies, so every
---claim the module holds on it has to be dropped in the same breath.
---@param index number Recorded edict index
---@param ent Entity
local function release(index, ent)
    if not IsValid(ent) then return end

    chronos.BindProxy(index, -1)
    chronos.SetSkip(ent:EntIndex(), false)
    chronos.SetIgnore(ent:EntIndex(), false)
    ent:Remove()
end

---@param tick number
function CHRONOS.SyncStageClones(tick)
    local stage = CHRONOS.Stage
    if not stage then return end

    local manifest = chronos.GetEntities(tick)
    local live = #ents.GetAll()
    local budget = math.min(CHRONOS.MaxStageSpawnsPerSync,
        CHRONOS.MaxStageEnts - table.Count(stage.Clones) - table.Count(stage.Puppets))

    -- Edicts are shared with the live world, and running that pool dry crashes
    -- the server for everybody, which is the one thing this mode must not do.
    if live > 8192 - CHRONOS.EdictHeadroom then budget = 0 end
    if GetConVar("chronos_stagespawn"):GetInt() == 0 then budget = 0 end

    for index, info in pairs(manifest) do
        if not stage.Tried[index] and budget > 0 then
            budget = budget - 1
            stage.Clones[index] = CHRONOS.SpawnStageClone(index, info)

            local retry
            if not stage.Clones[index] then
                stage.Puppets[index], retry = CHRONOS.SpawnStagePuppet(index, info)
            end

            -- Anything refused outright is never asked about again; an entity
            -- that was merely parented at this tick is left open for the next.
            stage.Tried[index] = not retry
        end
    end

    stage.Present = manifest
    CHRONOS.ShowStageClones()
end

---A stand-in is created once and kept for the whole session, hidden on the ticks
---its original did not exist. Removing and respawning them instead is what took
---the server down on a timeline drag: every tick of the drag churned the whole
---population, and Source does not hand an edict back the moment it is freed.
function CHRONOS.ShowStageClones()
    local stage = CHRONOS.Stage

    for index, clone in pairs(stage.Clones) do
        if IsValid(clone) then
            clone:SetNoDraw(not stage.Present[index])
        end
    end
end

function CHRONOS.ClearStageClones()
    local stage = CHRONOS.Stage
    if not stage then return end

    for index, clone in pairs(stage.Clones) do
        release(index, clone)
        stage.Clones[index] = nil
    end

    for index, puppet in pairs(stage.Puppets) do
        release(index, puppet)
        stage.Puppets[index] = nil
    end

    stage.Tried = {}
    stage.Present = {}
end
