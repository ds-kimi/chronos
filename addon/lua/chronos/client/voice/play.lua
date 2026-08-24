---Clips are fetched once, up front, and kept paused at the start. Playing then
---costs nothing: no request, no decode, no stutter, and no late start.
CHRONOS.VoiceReady = CHRONOS.VoiceReady or {}
CHRONOS.VoiceLoading = 0
CHRONOS.VoiceTotal = 0

-- Clips still in flight, and any cue that landed before they arrived. A second
-- sound.PlayURL for a clip already downloading was what forced the old catch-up
-- seek, which is what cut the front off every late utterance.
local loading = {}
local queued = {}

---@param id number
---@param channel IGModAudioChannel
local function flushQueued(id, channel)
    local waiting = queued[id]
    queued[id] = nil
    if not waiting or not IsValid(channel) then return end

    CHRONOS.VoiceSeek(channel, 0)
    channel:SetPos(LocalPlayer():GetPos())
    channel:Play()

    local channels = CHRONOS.VoiceChannels
    channels[#channels + 1] = { channel = channel, index = waiting.index,
        name = waiting.name, preloaded = true, start = CHRONOS.SmoothCursor }
end

---@param base string
---@param id number
local function preload(base, id)
    CHRONOS.VoiceLoading = CHRONOS.VoiceLoading + 1
    loading[id] = true

    -- "noblock" is REQUIRED for SetTime: it turns block streaming off, which is
    -- what makes a channel seekable. Preloading is what removes the stutter, so
    -- there is no reason to stream in blocks here anyway.
    sound.PlayURL(base .. id, "3d noblock", function(channel)
        CHRONOS.VoiceLoading = CHRONOS.VoiceLoading - 1
        loading[id] = nil

        if not IsValid(channel) then
            queued[id] = nil
            return
        end

        channel:Pause()
        CHRONOS.VoiceSeek(channel, 0)
        CHRONOS.VoiceReady[id] = channel
        flushQueued(id, channel)
    end)
end

---@param url string
---@param index number
---@param name string
---@param id number
local function playClip(url, index, name, id)
    local ready = CHRONOS.VoiceReady[id]

    if IsValid(ready) then
        CHRONOS.VoiceSeek(ready, 0)
        ready:SetPos(LocalPlayer():GetPos())
        ready:Play()

        local channels = CHRONOS.VoiceChannels
        channels[#channels + 1] = { channel = ready, index = index,
            name = name, preloaded = true, start = CHRONOS.SmoothCursor }
        return
    end

    -- Cued before the download finished: wait for it rather than starting a
    -- second fetch. Whatever the wait costs, the utterance plays whole.
    if loading[id] then
        queued[id] = { index = index, name = name }
        return
    end

    -- A clip recorded after the manifest was sent, so nothing is fetching it.
    sound.PlayURL(url, "3d noblock", function(channel)
        if not IsValid(channel) then return end

        channel:SetPos(LocalPlayer():GetPos())
        channel:Play()

        local channels = CHRONOS.VoiceChannels
        channels[#channels + 1] = { channel = channel, index = index, name = name,
            start = CHRONOS.SmoothCursor }
    end)
end

net.Receive("chronos_voice_manifest", function()
    local base = net.ReadString()
    local count = net.ReadUInt(16)

    CHRONOS.StopVoice()
    queued = {}
    CHRONOS.VoiceTotal = count

    for i = 1, count do
        local id = net.ReadUInt(32)
        if base ~= "" and not CHRONOS.VoiceReady[id] and not loading[id] then
            preload(base, id)
        end
    end
end)

net.Receive("chronos_voice", function()
    local index = net.ReadUInt(13)
    local name = net.ReadString()
    local text = net.ReadString()
    local clip = net.ReadUInt(32)
    local base = net.ReadString()

    if clip > 0 and base ~= "" then
        playClip(base .. clip, index, name, clip)
    end
end)

---Keeps every live stream glued to its speaker, and drops channels that have
---finished so the table does not grow for the whole session.
hook.Add("Think", "chronos_voice_follow", function()
    local channels = CHRONOS.VoiceChannels

    for i = #channels, 1, -1 do
        local entry = channels[i]
        local channel = entry.channel

        if not IsValid(channel) or channel:GetState() == GMOD_CHANNEL_STOPPED then
            -- Preloaded channels are reused for the next play, so they are
            -- rewound and parked rather than destroyed.
            if IsValid(channel) and entry.preloaded then
                channel:Pause()
                CHRONOS.VoiceSeek(channel, 0)
            elseif IsValid(channel) then
                channel:Stop()
            end

            table.remove(channels, i)
        else
            local ent = CHRONOS.VoiceSpeaker(entry.index)
            if IsValid(ent) then
                channel:SetPos(ent:GetPos() + Vector(0, 0, 40))
            end
        end
    end
end)
