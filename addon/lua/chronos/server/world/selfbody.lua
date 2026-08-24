---A spectator is excluded from restores so they can fly, which also means their
---own recorded body is never shown. CBasePlayer cannot be spawned as a stand-in,
---so the body is a prop_dynamic driven straight from the rebuilt state.
CHRONOS.Bodies = CHRONOS.Bodies or {}

---@param ply Player
---@return Entity|nil
local function acquire(ply)
    local body = CHRONOS.Bodies[ply:EntIndex()]
    if IsValid(body) then return body end

    body = ents.Create("prop_dynamic")
    if not IsValid(body) then return nil end

    body:SetModel(ply:GetModel())
    body:Spawn()
    body:SetSolid(SOLID_NONE)
    body:SetMoveType(MOVETYPE_NONE)
    body.ChronosBody = ply:EntIndex()

    -- Networked so the client can route that player's voice onto the stand-in:
    -- the original player entity is somewhere else entirely while spectating.
    body:SetNWInt("ChronosSpeaker", ply:EntIndex())

    CHRONOS.Bodies[ply:EntIndex()] = body
    return body
end

---Drives every spectator's own body from the tick that was just rebuilt.
function CHRONOS.UpdateBodies()
    for _, ply in ipairs(player.GetAll()) do
        if not ply.ChronosSpectate then continue end

        local index = ply:EntIndex()
        local pos, ang = chronos.GetTransform(index)
        local body = pos and acquire(ply)
        if not IsValid(body) then continue end

        body:SetPos(pos)
        body:SetAngles(ang)
        body:SetSequence(chronos.ReadProp(index, "m_nSequence") or 0)
        body:SetCycle(chronos.ReadProp(index, "m_flCycle") or 0)
    end
end

function CHRONOS.ClearBodies()
    for index, body in pairs(CHRONOS.Bodies) do
        if IsValid(body) then body:Remove() end
        CHRONOS.Bodies[index] = nil
    end
end
