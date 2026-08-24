---Frame each speaker began talking on, keyed by AccountID.
---The end of an utterance is a terrible clock: PlayerEndVoice is raised on the
---client after its own silence timeout, then travels to the server, so the flush
---lands a few hundred milliseconds after the mouth actually stopped. Deriving
---the start from it put every clip that far late, and steam voice drops silent
---stretches out of the stream entirely, so the decoded audio is shorter than the
---wall clock it covers and the error grew with every pause. The server sees the
---first voice packet within one trip of it being spoken, so that is the clock.
CHRONOS.VoiceStart = CHRONOS.VoiceStart or {}
---Last frame each talker was still speaking on. Only used to report how much
---wall clock a clip covers versus how much audio actually arrived: a clip much
---shorter than the span it covers means audio was lost before chronos saw it,
---which is not something the filing frame can be blamed for.
CHRONOS.VoiceLast = CHRONOS.VoiceLast or {}

---Marks the first recorded frame of each talker, sampled on the recording tick.
---PlayerStartVoice was not enough on its own: the engine re-raises it whenever
---the packet flow hiccups mid-sentence, and each re-raise overwrote the mark
---with a tick well past the real start, which is why every clip fell back to
---stepping backwards from the flush. IsSpeaking is polled state, so the rising
---edge is taken here and nothing after it can move the mark.
function CHRONOS.MarkSpeakers()
    -- A fresh recording starts numbering from zero again, so marks left over
    -- from the previous one would sit in the future and be discarded anyway.
    if CHRONOS.RecTick == 0 then
        CHRONOS.VoiceStart = {}
        CHRONOS.VoiceLast = {}
    end

    for _, ply in ipairs(player.GetAll()) do
        local account = ply:AccountID()

        if ply:IsSpeaking() then
            CHRONOS.VoiceLast[account] = CHRONOS.RecTick

            -- The mark is only cleared once the audio is filed, so a gap
            -- between words cannot restart the clock.
            if not CHRONOS.VoiceStart[account] then
                CHRONOS.VoiceStart[account] = CHRONOS.RecTick

                if CHRONOS.VoiceDebug then
                    MsgC(Color(120, 200, 255), string.format(
                        "[chronos] voice start: %s at frame %d\n", ply:Nick(), CHRONOS.RecTick))
                end
            end
        end
    end
end

hook.Add("PlayerDisconnected", "Chronos_VoiceStart", function(ply)
    CHRONOS.VoiceStart[ply:AccountID()] = nil
end)

---Frame the utterance began on, and where that number came from.
---@param account number
---@param frames number Length of the decoded audio in ticks
---@return number tick
---@return string source
local function startTick(account, frames)
    local marked = CHRONOS.VoiceStart[account]
    CHRONOS.VoiceStart[account] = nil

    -- Steam voice strips silence out of the stream, so the wall clock a clip
    -- spans is always at least as long as the audio in it. A mark that breaks
    -- that never was the start, and filing on it would play the clip early.
    if marked and CHRONOS.RecTick - marked >= frames - 1 then
        return marked, "start"
    end

    if CHRONOS.VoiceDebug and marked then
        MsgC(Color(255, 120, 120), string.format(
            "[chronos] voice: mark %d rejected at flush %d, audio is %d frames\n",
            marked, CHRONOS.RecTick, frames))
    end

    return CHRONOS.RecTick - frames, "end"
end

---Audio arrives once speech ends, so the clip is filed against the frame the
---speaker began on rather than the frame the flush happened to land on.
---@param ply Player
---@param raw string Raw float32 PCM from auris.FlushRaw
local function record(ply, raw)
    if CHRONOS.VoiceDebug then
        MsgC(Color(120, 200, 255), string.format(
            "[chronos] voice in: ply=%s audio=%dB mode=%s port=%d\n",
            ply:Nick(), #raw, CHRONOS.Mode, CHRONOS.ClipPort))
    end

    if CHRONOS.Mode ~= "recording" or CHRONOS.ClipPort <= 0 then return end

    local first, last = chronos.GetRange()
    if not first or not Auris or not isfunction(Auris.PCMToWAV) then return end

    local seconds, frames
    raw, seconds, frames = CHRONOS.TrimVoice(raw)
    if frames <= 0 then return end

    -- The clip is handed to the module as finished WAV bytes and only ever
    -- referenced by the id it returns, so no filename reaches a client.
    local clip = chronos.AddClip(Auris.PCMToWAV(raw)) or 0
    if clip == 0 then return end

    local raw_tick, source = startTick(ply:AccountID(), frames)
    local tick = math.Clamp(raw_tick, first, last)

    CHRONOS.PushEventAt(tick, "voice", {
        ent = ply:EntIndex(),
        name = ply:Nick(),
        text = "",
        clip = clip
    })

    if CHRONOS.VoiceDebug then
        -- span is how long the speaker was actually talking. It should track
        -- the audio length: a span far longer means packets never arrived, so
        -- the clip is short at the source and no filing frame can fix it.
        local stop = CHRONOS.VoiceLast[ply:AccountID()] or CHRONOS.RecTick
        local span = stop - raw_tick

        MsgC(Color(120, 200, 255), string.format(
            "[chronos] voice filed: clip=%d frame=%d via %s (%.2fs audio / %d frames, spoke %d frames to %d, flush at %d, range %d-%d)\n",
            clip, tick, source, seconds, frames, span, stop, CHRONOS.RecTick, first, last))
    end

    CHRONOS.VoiceLast[ply:AccountID()] = nil
end

-- Auris_VoiceEnd fires the moment a player stops speaking, before any whisper
-- work begins. Taking the audio here means there is no transcription latency to
-- compensate for and no GPU cost at all. Returning true consumes the utterance
-- so Auris skips transcribing it: chronos wants the recording, not the text.
function CHRONOS.HookAuris()
    if CHRONOS.VoiceHooked or not auris or not isfunction(auris.FlushRaw) then return end
    if not Auris or not isfunction(Auris.PCMToWAV) then return end

    CHRONOS.VoiceHooked = true

    -- Auris captures voiced audio only by default, which is what transcription
    -- wants and the opposite of what a replay wants: pauses are dropped, so a
    -- clip is shorter than the time it covers and everything after a pause
    -- plays earlier than it was said. Chronos wants the recording, not the
    -- words, so it asks for the silence back.
    if isfunction(auris.SetPreserveTimeline) then
        auris.SetPreserveTimeline(true)
    else
        MsgC(Color(255, 165, 0), "[chronos] voice: this Auris build drops silence " ..
            "from captured audio, so clips will run short of the time they cover\n")
    end

    hook.Add("Auris_VoiceEnd", "Chronos_Voice", function(ply)
        if not IsValid(ply) then return end

        -- The API doc says UserID, but the voice detour keys the buffer by the
        -- low 32 bits of the SteamID64, which is AccountID. UserID looks up an
        -- empty buffer and returns nil, silently, for every utterance.
        local raw = auris.FlushRaw(ply:AccountID())
        if not raw then
            CHRONOS.VoiceStart[ply:AccountID()] = nil
            if CHRONOS.VoiceDebug then
                MsgC(Color(255, 120, 120), string.format(
                    "[chronos] voice: FlushRaw empty for %s (account %d)\n",
                    ply:Nick(), ply:AccountID()))
            end
            return
        end

        record(ply, raw)
        return true
    end)
end

-- Chronos loads before Auris, so the first attempt always fails. Auris exposes
-- no ready hook, so this polls, and never gives up: a bounded retry silently
-- leaves voice dead if the model takes longer than the window to load.
CHRONOS.HookAuris()

if not CHRONOS.VoiceHooked then
    timer.Create("chronos_auris_wait", 2, 0, function()
        CHRONOS.HookAuris()

        if CHRONOS.VoiceHooked then
            timer.Remove("chronos_auris_wait")
            MsgC(Color(120, 200, 255), "[chronos] voice: capturing raw audio from Auris\n")
        end
    end)
end
