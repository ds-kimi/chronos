---Playback position a clip should be at, derived from the replay cursor rather
---than from how long the channel happened to run. Pause and resume are two
---network round trips, so a channel that is simply resumed is late by however
---long the play command took to come back, and that gap is speech nobody hears.
---@param entry table
---@return number|nil Seconds into the clip, or nil if it cannot be derived
local function expectedTime(entry)
    if not entry.start then return nil end

    local elapsed = (CHRONOS.SmoothCursor - entry.start) * engine.TickInterval()
    return elapsed >= 0 and elapsed or nil
end

---Drops the channel back onto the cursor when the two have drifted apart. The
---threshold exists because a seek is audible: correcting every few milliseconds
---would chop the clip up worse than the drift does.
---@param entry table
function CHRONOS.VoiceSync(entry)
    local channel = entry.channel
    local target = IsValid(channel) and expectedTime(entry)
    if not target then return end

    local ok, current = pcall(channel.GetTime, channel)
    if ok and math.abs(current - target) > 0.05 then
        CHRONOS.VoiceSeek(channel, target)
    end
end

---Pause has to hold mid-utterance. Letting the stream run means a voice keeps
---talking over a frozen world, and stopping it means the sentence restarts from
---the top on resume, so channels are parked exactly where they are.
---@param paused boolean
function CHRONOS.PauseVoice(paused)
    for _, entry in ipairs(CHRONOS.VoiceChannels) do
        local channel = entry.channel
        if IsValid(channel) then
            if paused then
                channel:Pause()
            elseif channel:GetState() == GMOD_CHANNEL_PAUSED then
                -- Resync first: seeking a stopped channel is silent, while
                -- seeking one that is already running is a click.
                CHRONOS.VoiceSync(entry)
                channel:Play()
            end
        end
    end
end
