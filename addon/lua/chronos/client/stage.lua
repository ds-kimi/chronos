---Stage sounds arrive per client instead of through the engine sound interface,
---which has no way to address one player, and are played at the position the
---original entity was standing in when it made them.
net.Receive("chronos_stagesnd", function()
    local name = net.ReadString()
    local pos = net.ReadVector()
    local level = net.ReadUInt(8)
    local pitch = net.ReadUInt(8)
    local volume = net.ReadFloat()

    sound.Play(name, pos, level, pitch, volume)
end)
