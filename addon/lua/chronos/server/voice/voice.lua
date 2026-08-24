---Set by chronos voiceport / voicehost. Port zero disables voice capture.
---Persisted as convars so a restart does not silently drop voice, which looks
---identical to a bug.
CreateConVar("chronos_voiceport", "0", FCVAR_ARCHIVE, "Clip server port, 0 disables voice")
CreateConVar("chronos_voicehost", "", FCVAR_ARCHIVE, "Address clients fetch clips from")

CHRONOS.ClipPort = CHRONOS.ClipPort or 0
CHRONOS.ClipHost = CHRONOS.ClipHost or ""

---Clients need an address they can actually reach, which is not something the
---server can work out for itself behind NAT, so it is configured.
---@return string
function CHRONOS.ClipURL()
    if CHRONOS.ClipPort <= 0 or CHRONOS.ClipHost == "" then return "" end

    return string.format("http://%s:%d/clip/", CHRONOS.ClipHost, CHRONOS.ClipPort)
end

---Every clip in the recording, sent when a scrub begins so clients can fetch
---and decode all of them up front. Streaming on demand is what made playback
---start late and get cut off: the fetch only began at the frame it was due.
function CHRONOS.SendVoiceManifest(ply)
    local clips = {}

    for _, bucket in pairs(CHRONOS.Events) do
        for i = 1, #bucket do
            if bucket[i].kind == "voice" and (bucket[i].clip or 0) > 0 then
                clips[#clips + 1] = bucket[i].clip
            end
        end
    end

    net.Start("chronos_voice_manifest")
    net.WriteString(CHRONOS.ClipURL())
    net.WriteUInt(#clips, 16)
    for i = 1, #clips do
        net.WriteUInt(clips[i], 32)
    end

    if IsValid(ply) then net.Send(ply) else net.Broadcast() end
end

---@param event table
function CHRONOS.EmitVoice(event)
    net.Start("chronos_voice")
    net.WriteUInt(event.ent, 13)
    net.WriteString(event.name)
    net.WriteString(event.text or "")
    net.WriteUInt(event.clip or 0, 32)
    net.WriteString(CHRONOS.ClipURL())

    -- A stage replay is private to its viewers: the living should not hear a
    -- conversation from ten minutes ago play out around them.
    if CHRONOS.Stage then
        net.Send(CHRONOS.StageViewers())
    else
        net.Broadcast()
    end
end

-- Applies config defaults, then lets a previously saved convar override them so
-- a value changed in game is not silently reverted on the next map load.
timer.Simple(1, function()
    local config = CHRONOS.Config or {}
    local host = GetConVar("chronos_voicehost"):GetString()
    local port = GetConVar("chronos_voiceport"):GetInt()

    if host == "" then host = config.voicehost or "" end
    if port <= 0 then port = config.voiceport or 0 end

    CHRONOS.VoiceDebug = CHRONOS.VoiceDebug or config.voicedebug == true
    chronos.SetMemoryCap(config.memcap or 512)
    chronos.SetKeyInterval(config.keyinterval or 64)
    chronos.SetClipCap(config.voicecap or 256)

    if host == "" or port <= 0 then return end

    CHRONOS.ClipHost = host
    CHRONOS.ClipPort = chronos.StartClipServer(port) and port or 0
    MsgC(Color(120, 200, 255), "[chronos] voice: clip server on " .. CHRONOS.ClipURL() .. "\n")
end)
