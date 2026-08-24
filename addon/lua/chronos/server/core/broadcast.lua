---@param mode string Mode string as this client should see it
---@param staged boolean Whether this client is watching a private stage
local function writeState(mode, staged)
    local first, last = chronos.GetRange()
    local stats = chronos.GetStats()

    net.Start("chronos_state")
    net.WriteString(mode)
    net.WriteInt(first or 0, 32)
    net.WriteInt(last or 0, 32)
    net.WriteInt(CHRONOS.Cursor, 32)
    net.WriteBool(CHRONOS.Playing)
    net.WriteInt(stats.frames, 32)
    net.WriteFloat(CHRONOS.Speed)
    net.WriteBool(CHRONOS.ViewLock)
    net.WriteBool(CHRONOS.UseGhosts)
    net.WriteFloat(stats.bytes / 1048576)
    net.WriteFloat(stats.cap / 1048576)
    net.WriteInt(CHRONOS.EventCount, 32)
    net.WriteInt(chronos.EffectCount(), 32)
    net.WriteBool(staged)
end

---Pushes the current scrubber state to every client. A stage viewer is scrubbing
---while everybody else is still playing, so the mode is decided per client:
---one broadcast string would put the whole server's HUD into replay for one
---admin's log review.
function CHRONOS.Broadcast()
    if not CHRONOS.Stage then
        writeState(CHRONOS.Mode, false)
        net.Broadcast()
        return
    end

    for _, ply in ipairs(player.GetAll()) do
        local staged = CHRONOS.Stage.Viewers[ply] ~= nil
        writeState(staged and "replay" or CHRONOS.Mode, staged)
        net.Send(ply)
    end
end
