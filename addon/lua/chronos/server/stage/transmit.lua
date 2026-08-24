---Whether an entity belongs to the world a given player is living in. Stage
---stand-ins belong to viewers, everything spawned since the map loaded belongs
---to everybody else, and map geometry belongs to both: hiding that would leave
---a viewer standing in an empty skybox.
---@param ent Entity
---@param ply Player
---@param viewer boolean
---@return boolean hide
local function shouldHide(ent, ply, viewer)
    if ent == ply then return false end

    -- A viewer's own weapon is still predicted by their client, and cutting it
    -- off mid-session makes the active weapon go invalid rather than invisible.
    if ent.GetOwner and ent:GetOwner() == ply then return false end
    if ent.ChronosStage ~= nil then return not viewer end

    -- A viewer's own body is flying around the replay; the living should not
    -- see it walk through walls somewhere they cannot follow.
    if ent:IsPlayer() and CHRONOS.Stage.Viewers[ent] then return not viewer end

    return viewer
end

---Splits the two populations apart per client. Called on join, leave and every
---sync, since anything spawned in the meantime defaults to being transmitted.
function CHRONOS.SyncStageTransmit()
    local stage = CHRONOS.Stage
    if not stage then return end

    local players = player.GetAll()
    for _, ent in ipairs(ents.GetAll()) do
        if ent:EntIndex() > 0 and (ent.ChronosStage ~= nil or not ent:CreatedByMap()) then
            for i = 1, #players do
                local ply = players[i]
                ent:SetPreventTransmit(ply, shouldHide(ent, ply, stage.Viewers[ply] ~= nil))
            end
        end
    end
end

function CHRONOS.ClearStageTransmit()
    local players = player.GetAll()

    for _, ent in ipairs(ents.GetAll()) do
        for i = 1, #players do
            ent:SetPreventTransmit(players[i], false)
        end
    end
end
