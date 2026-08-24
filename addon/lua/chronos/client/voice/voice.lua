---Live streams and their speakers, shared with cl_voice_play.
CHRONOS.VoiceChannels = CHRONOS.VoiceChannels or {}
CHRONOS.VoiceBodies = CHRONOS.VoiceBodies or {}

---SetTime throws on a channel that is not seekable yet, and an error inside a
---sound callback aborts the rest of it. Every seek goes through here so one bad
---channel cannot take a clip down with it.
---@param channel IGModAudioChannel
---@param time number
function CHRONOS.VoiceSeek(channel, time)
    if not IsValid(channel) then return false end

    return pcall(channel.SetTime, channel, time)
end

---Streams outlive the frame that started them, so a seek or a stop has to kill
---them explicitly or the last scrub keeps talking over the new one.
function CHRONOS.StopVoice()
    local channels = CHRONOS.VoiceChannels

    for i = #channels, 1, -1 do
        local entry = channels[i]
        if IsValid(entry.channel) then
            if entry.preloaded then
                entry.channel:Pause()
                CHRONOS.VoiceSeek(entry.channel, 0)
            else
                entry.channel:Stop()
            end
        end

        channels[i] = nil
    end

    CHRONOS.VoiceBodies = {}
end

---A speaker is not always the entity that was recorded: during replay a player
---who is spectating is represented by a stand-in body, and an entity that no
---longer exists may be a ghost. Follow whichever is actually on screen.
---@param index number Recorded edict index
---@return Entity|nil
function CHRONOS.VoiceSpeaker(index)
    -- A stand-in body claims the recorded index it represents, so prefer it:
    -- while spectating, the real player entity is wherever the viewer flew to.
    -- Scanning for it every frame per channel would be wasteful, so the answer
    -- is cached and only re-resolved when the cached entity goes away.
    local cached = CHRONOS.VoiceBodies[index]
    if IsValid(cached) then return cached end

    for _, ent in ipairs(ents.FindByClass("prop_dynamic")) do
        if ent:GetNWInt("ChronosSpeaker", 0) == index then
            CHRONOS.VoiceBodies[index] = ent
            return ent
        end
    end

    local ent = Entity(index)
    return IsValid(ent) and ent or nil
end

-- Voice is audio only now: transcription is skipped at the source, so there is
-- no text to draw. A marker over the speaker is what ties the sound to a player
-- during a scrub, which a corner list cannot do once several people talk.
hook.Add("HUDPaint", "chronos_voice", function()
    local colors = CHRONOS.Colors
    local channels = CHRONOS.VoiceChannels

    for i = #channels, 1, -1 do
        local entry = channels[i]
        local ent = IsValid(entry.channel) and CHRONOS.VoiceSpeaker(entry.index)
        if IsValid(ent) then
            local screen = (ent:GetPos() + Vector(0, 0, 82)):ToScreen()
            if screen.visible then
                local name = entry.name or "speaking"
                surface.SetFont("ChronosUI")

                local width = surface.GetTextSize(name)
                local x, y = screen.x - width * 0.5 - 10, screen.y - 12

                draw.RoundedBox(4, x, y, width + 20, 22, colors.bg)
                draw.SimpleText(name, "ChronosUI", x + 10, y + 4, colors.fill)
            end
        end
    end
end)
