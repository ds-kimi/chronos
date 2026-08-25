---Drives one fake client. Bots that only stand there scale the edict count and
---nothing else; the delta volume, the sounds and the temp entities that make a
---recording expensive all come from movement and shooting.
---@param ply Player
---@param cmd CUserCmd
---@param now number
local function DriveBot(ply, cmd, now)
    local seed = ply:EntIndex()

    cmd:SetViewAngles(Angle(math.sin(now + seed) * 25, (now * 40 + seed * 37) % 360, 0))
    cmd:SetForwardMove(math.cos(now * 0.7 + seed) * 400)
    cmd:SetSideMove(math.sin(now * 0.5 + seed * 2) * 400)

    local buttons = 0
    if math.sin(now * 1.3 + seed) > 0.9 then buttons = buttons + IN_JUMP end
    if math.sin(now * 4.0 + seed * 3) > 0.5 then buttons = buttons + IN_ATTACK end

    cmd:SetButtons(buttons)
end

hook.Add("StartCommand", "chronos_benchbot", function(ply, cmd)
    if not CHRONOS.Bench.Running or not ply:IsBot() then return end

    DriveBot(ply, cmd, CurTime())
end)

-- Without a weapon the attack button produces nothing, and the temp entity and
-- sound paths through the recorder never get exercised at all.
hook.Add("PlayerSpawn", "chronos_benchbot_arm", function(ply)
    if not CHRONOS.Bench.Running or not ply:IsBot() then return end

    timer.Simple(0.5, function()
        if IsValid(ply) then ply:Give("weapon_pistol") end
    end)
end)
