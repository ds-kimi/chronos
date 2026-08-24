---Sounds are the one event class the module cannot see: they travel through the
---engine sound interface, not the temp entity path, so they are replayed here
---while every visual effect is replayed by chronos.PlayEffects.
---@param event table
local function emitSound(event)
    if not event.name then return end

    local ent = event.ent and Entity(event.ent)
    if IsValid(ent) then
        ent:EmitSound(event.name, event.level, event.pitch, event.volume)
    elseif event.pos then
        sound.Play(event.name, event.pos, event.level or 75, event.pitch or 100, event.volume or 1)
    end
end

---Fires everything recorded in (from, to]. Only called while playing forward: a
---scrub would otherwise dump hundreds of gunshots at once.
---@param from number
---@param to number
function CHRONOS.PlayEvents(from, to)
    if to <= from then return end

    -- Effects and sounds are bursty: replaying a big jump at once would dump a
    -- whole firefight in a single frame, so they stay windowed.
    local burst = to - from <= 32
    if burst then
        chronos.PlayEffects(from, to)
    end

    for tick = from + 1, to do
        local bucket = CHRONOS.Events[tick]
        for i = 1, bucket and #bucket or 0 do
            local event = bucket[i]
            -- Voice is the exception: a clip is one discrete utterance, and it
            -- is filed hundreds of frames behind where it was transcribed, so
            -- windowing it out is what silences a normal jump into playback.
            if event.kind == "voice" then
                if CHRONOS.EmitVoice then CHRONOS.EmitVoice(event) end
            elseif burst then
                emitSound(event)
            end
        end
    end
end
