CHRONOS.Mode = CHRONOS.Mode or "idle"
CHRONOS.Cursor = CHRONOS.Cursor or 0
CHRONOS.Playing = false
CHRONOS.Speed = 1
CHRONOS.Accum = 0

---Physics objects keep integrating between restores and drift the world out
---from under the replay, so motion is disabled for the whole scrub session.
---@param frozen boolean
function CHRONOS.SetFrozen(frozen)
    for _, ent in ipairs(ents.GetAll()) do
        local phys = ent:GetPhysicsObject()
        if IsValid(phys) then
            phys:EnableMotion(not frozen)
        end
    end
end

---A frozen player is replayed onto their own body; a spectator is excluded from
---restores and freed to fly around instead. Frozen is the default: a paused
---server means nobody moves.
---@param ply Player
---@param spectate boolean
function CHRONOS.SetSpectator(ply, spectate)
    ply.ChronosSpectate = spectate
    chronos.SetIgnore(ply:EntIndex(), spectate)

    -- The networked flag is what the shared StartCommand hook reads, so the
    -- client stops predicting movement at the same moment the server stops it.
    ply:SetNWBool("ChronosFrozen", not spectate)

    -- Leaving spectator hands the player back their own restored body, so the
    -- stand-in has to go or there would be two of them.
    local body = CHRONOS.Bodies and CHRONOS.Bodies[ply:EntIndex()]
    if not spectate and IsValid(body) then
        body:Remove()
        CHRONOS.Bodies[ply:EntIndex()] = nil
    end
    ply:Freeze(not spectate)
    ply:SetMoveType(spectate and MOVETYPE_NOCLIP or MOVETYPE_NONE)
end

---A restore writes the recorded movetype and velocity back onto players, so
---client prediction integrates them for one tick before the next restore
---corrects it. Re-pinning right after the restore removes that wobble.
function CHRONOS.PinPlayers()
    for _, ply in ipairs(player.GetAll()) do
        if ply:GetNWBool("ChronosFrozen", false) then
            ply:SetMoveType(MOVETYPE_NONE)
            ply:SetLocalVelocity(vector_origin)
        end
    end
end
