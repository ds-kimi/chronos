CHRONOS.Hidden = CHRONOS.Hidden or {}

---@param ent Entity
---@param hidden boolean
local function apply(ent, hidden)
    if hidden then
        ent:AddEffects(EF_NODRAW)
        ent:SetNotSolid(true)
    else
        ent:RemoveEffects(EF_NODRAW)
        ent:SetNotSolid(false)
    end
end

---Entities that did not exist at the scrubbed tick are hidden rather than
---removed, because removing them would destroy the live world for good.
---@param seen table<number, boolean>
function CHRONOS.SyncVisibility(seen)
    for _, ent in ipairs(ents.GetAll()) do
        local index = ent:EntIndex()

        -- Ghosts and stand-in bodies are created by the replay itself, so they
        -- are never in the recorded manifest. Hiding them is what made a
        -- spectator's own body vanish the moment playback resumed.
        local ours = ent.ChronosGhost ~= nil or ent.ChronosBody ~= nil
        if index > 0 and not ours and not ent:IsPlayer() and not ent:IsWeapon() then
            local shouldHide = not seen[index]
            if shouldHide ~= (CHRONOS.Hidden[index] ~= nil) then
                CHRONOS.Hidden[index] = shouldHide and ent or nil
                apply(ent, shouldHide)
            end
        end
    end
end

function CHRONOS.ClearHidden()
    for index, ent in pairs(CHRONOS.Hidden) do
        if IsValid(ent) then apply(ent, false) end
        CHRONOS.Hidden[index] = nil
    end
end
