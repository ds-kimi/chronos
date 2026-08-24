---Aim is stored as a flat {index, pitch, yaw, ...} row per tick. A player's own
---view angles are client-authoritative, so restoring the networked prop is not
---enough: the server has to push the angle back with SetEyeAngles.
CHRONOS.Aim = CHRONOS.Aim or {}
---Off by default. The aim a spectator sees comes from the restored networked
---prop; forcing the replayed player's own camera on top of that fights their
---client every tick and reads as stutter.
CHRONOS.ViewLock = false

---@param tick number
function CHRONOS.CaptureAim(tick)
    local players = player.GetAll()
    if #players == 0 then return end

    local row = {}
    local at = 0

    for i = 1, #players do
        local ply = players[i]
        local angle = ply:EyeAngles()
        row[at + 1] = ply:EntIndex()
        row[at + 2] = angle.p
        row[at + 3] = angle.y
        at = at + 3
    end

    CHRONOS.Aim[tick] = row
end

---@param tick number
function CHRONOS.RestoreAim(tick)
    local row = CHRONOS.Aim[tick]
    if not row or not CHRONOS.ViewLock then return end

    for i = 1, #row, 3 do
        local ply = Entity(row[i])
        if IsValid(ply) and ply:IsPlayer() and not ply.ChronosSpectate then
            local current = ply:EyeAngles()

            -- Re-sending an unchanged angle costs a fixangle every tick, which
            -- the client sees as a jitter, so only correct real drift.
            if math.abs(current.p - row[i + 1]) > 0.05 or math.abs(current.y - row[i + 2]) > 0.05 then
                ply:SetEyeAngles(Angle(row[i + 1], row[i + 2], 0))
            end
        end
    end
end

---@param first number Oldest tick the snapshot ring still holds
function CHRONOS.PruneAim(first)
    for tick in pairs(CHRONOS.Aim) do
        if tick < first then
            CHRONOS.Aim[tick] = nil
        end
    end
end

function CHRONOS.ClearAim()
    CHRONOS.Aim = {}
end
