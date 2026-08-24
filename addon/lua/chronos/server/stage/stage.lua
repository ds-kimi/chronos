---A stage replay is the experimental private review: the recording keeps
---running, the live world is never written to, and everything the viewer sees
---is a stand-in spawned for them and hidden from everybody else.
---@class ChronosStage
---@field Viewers table<Player, table> Saved movetype and position per viewer
---@field Clones table<number, Entity> Exact-class stand-ins, driven by the module
---@field Puppets table<number, Entity> prop_dynamic stand-ins, driven from Lua
---@field Tried table<number, boolean> Indices already attempted, so a class that
---       cannot be recreated is not retried on every sync
CHRONOS.Stage = nil
---Stand-ins compete with the live world for edicts, and running that pool dry
---is an instant crash, so the stage keeps a hard ceiling and never spawns more
---than a handful per sync.
CHRONOS.MaxStageEnts = 320
CHRONOS.MaxStageSpawnsPerSync = 24
CHRONOS.EdictHeadroom = 1024

---Opens a stage that hides the live world but spawns nothing, which is how the
---transmit split is told apart from a stand-in that takes the server down.
CreateConVar("chronos_stagespawn", "1", FCVAR_ARCHIVE, "Spawn stand-ins on a stage")

---@return Player[]
function CHRONOS.StageViewers()
    local out = {}
    if not CHRONOS.Stage then return out end

    for ply in pairs(CHRONOS.Stage.Viewers) do
        if IsValid(ply) then out[#out + 1] = ply end
    end

    return out
end

---@param ply Player
---@param atEnd boolean|nil Pin the cursor to the newest tick instead of keeping it
---@return boolean started
function CHRONOS.EnterStage(ply, atEnd)
    local first, last = chronos.GetRange()
    if not first or not IsValid(ply) then return false end

    -- A frozen scrub is the thing a stage replay exists to avoid, so asking for
    -- one hands the world back to everybody else first.
    if CHRONOS.Mode == "replay" then CHRONOS.ExitReplay() end

    if not CHRONOS.Stage then
        CHRONOS.Stage = { Viewers = {}, Clones = {}, Puppets = {}, Tried = {}, Present = {}, NextSync = 0 }
        CHRONOS.Cursor = math.Clamp(CHRONOS.Cursor, first, last)
        CHRONOS.Accum = 0
        CHRONOS.LastSync = -1e9
    end

    if atEnd then CHRONOS.Cursor = last end
    CHRONOS.Playing = false

    -- The viewer keeps their own body: it is hidden from everybody else, so
    -- flying it around the replay is invisible to the players still living in
    -- the world it was recorded from. Where they stood is saved on the way in
    -- and only then: re-entering an open stage would otherwise overwrite it
    -- with wherever they have flown to, and leaving would strand them there.
    if not CHRONOS.Stage.Viewers[ply] then
        CHRONOS.Stage.Viewers[ply] = { pos = ply:GetPos(), move = ply:GetMoveType() }
        ply:SetMoveType(MOVETYPE_NOCLIP)
    end

    CHRONOS.SyncStageClones(CHRONOS.Cursor)
    CHRONOS.SyncStageTransmit()
    CHRONOS.SendVoiceManifest(ply)
    CHRONOS.Broadcast()
    return true
end

---@param ply Player
function CHRONOS.LeaveStage(ply)
    local stage = CHRONOS.Stage
    local saved = stage and stage.Viewers[ply]
    if not saved then return end

    stage.Viewers[ply] = nil
    if IsValid(ply) then
        ply:SetMoveType(saved.move or MOVETYPE_WALK)
        ply:SetPos(saved.pos)
    end

    if table.Count(stage.Viewers) == 0 then
        CHRONOS.StageTeardown()
    else
        CHRONOS.SyncStageTransmit()
    end

    CHRONOS.Broadcast()
end

function CHRONOS.StageTeardown()
    if not CHRONOS.Stage then return end

    CHRONOS.ClearStageTransmit()
    CHRONOS.ClearStageClones()
    CHRONOS.Stage = nil
    CHRONOS.Playing = false
end
