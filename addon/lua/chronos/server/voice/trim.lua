---Bytes of raw float32 PCM that cover one recorded frame.
---@return number
local function bytesPerFrame()
    return 4 * 16000 * engine.TickInterval()
end

---Audio Auris buffered before this recording began belongs to no frame in it.
---Clearing the ring restarts frame numbering at zero while Auris keeps holding
---whatever it has captured, so the next utterance flushes minutes of audio into
---a recording seconds long: the mark is rejected for being impossibly short and
---the clip lands on frame zero, playing over the wrong moment.
function CHRONOS.DropVoiceBuffers()
    if not auris or not isfunction(auris.FlushRaw) then return end

    for _, ply in ipairs(player.GetAll()) do
        auris.FlushRaw(ply:AccountID())
    end

    CHRONOS.VoiceStart = {}
    CHRONOS.VoiceLast = {}
end

---Cuts the head off a clip that starts before the recording does. Dragging the
---whole clip back to frame zero instead would play the words at the wrong time
---and hold the channel busy for the length of the part that cannot be shown.
---@param raw string Raw float32 PCM
---@return string raw
---@return number seconds
---@return number frames
function CHRONOS.TrimVoice(raw)
    local seconds = #raw / 4 / 16000
    local frames = math.floor(seconds / engine.TickInterval())
    local overrun = frames - (CHRONOS.RecTick + 1)

    if overrun > 0 then
        -- Cut on a sample boundary: float32 PCM sliced mid-sample is noise, and
        -- a frame is not a whole number of bytes at any tickrate worth using.
        local offset = math.floor(overrun * bytesPerFrame() / 4) * 4
        raw = string.sub(raw, offset + 1)
        seconds = #raw / 4 / 16000
        frames = math.floor(seconds / engine.TickInterval())

        if CHRONOS.VoiceDebug then
            MsgC(Color(255, 165, 0), string.format(
                "[chronos] voice: trimmed %d frames that predate the recording\n", overrun))
        end
    end

    return raw, seconds, frames
end
