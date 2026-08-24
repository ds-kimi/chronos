---Starts the clip server and sets the address clients will fetch from. Host is
---explicit because a server behind NAT cannot discover its own reachable name.
---@param args string[]
function CHRONOS.Actions.voiceport(ply, args)
    local port = math.floor(tonumber(args[1]) or 0)

    if port <= 0 then
        chronos.StopClipServer()
        CHRONOS.ClipPort = 0
        CHRONOS.Actions.voicestats(ply)
        return
    end

    CHRONOS.ClipPort = chronos.StartClipServer(port) and port or 0
    RunConsoleCommand("chronos_voiceport", tostring(CHRONOS.ClipPort))
    CHRONOS.Actions.voicestats(ply)
end

---@param args string[]
function CHRONOS.Actions.voicehost(ply, args)
    -- Hostname or dotted IP only. The old pattern accepted trailing words, so a
    -- mistyped command silently produced an unreachable URL.
    local host = string.Trim(args[1] or "")
    CHRONOS.ClipHost = (string.match(host, "^%d+%.%d+%.%d+%.%d+$")
        or string.match(host, "^[%w%-]+%.[%w%.%-]+$")
        or string.match(host, "^localhost$")) or ""
    RunConsoleCommand("chronos_voicehost", CHRONOS.ClipHost)
    CHRONOS.Actions.voicestats(ply)
end

---@param args string[]
function CHRONOS.Actions.voicecap(ply, args)
    chronos.SetClipCap(tonumber(args[1]) or 256)
    CHRONOS.Actions.voicestats(ply)
end

---Files a silent clip at the current frame and immediately emits it, so the
---clip server, the URL and client playback can be proven without speaking or
---waiting on whisper.
function CHRONOS.Actions.voicetest(ply)
    if not IsValid(ply) then return end

    local samples = 16000
    -- A 440 Hz tone rather than silence: a silent test clip proves the transport
    -- but not that anything is audible at the other end.
    local pcm = {}
    for i = 1, samples do
        local v = math.sin(i / 16000 * 440 * 2 * math.pi) * 0.3
        local n = math.floor(v * 8388608) + 1065353216
        pcm[i] = string.char(n % 256, math.floor(n / 256) % 256,
            math.floor(n / 65536) % 256, math.floor(n / 16777216) % 256)
    end

    local wav = Auris and isfunction(Auris.PCMToWAV) and Auris.PCMToWAV(table.concat(pcm))
    local clip = wav and chronos.AddClip(wav) or 0

    CHRONOS.EmitVoice({ ent = ply:EntIndex(), name = ply:Nick(),
        text = "voice test", clip = clip })
    ply:PrintMessage(HUD_PRINTCONSOLE, string.format(
        "[chronos] emitted test clip id %d at %s", clip, CHRONOS.ClipURL() .. clip))
end

---@param args string[]
function CHRONOS.Actions.voicedebug(ply, args)
    CHRONOS.VoiceDebug = tonumber(args[1]) ~= 0
    CHRONOS.Actions.voicestats(ply)
end

---Traces the whole voice chain in one place. Auris only transcribes when the
---client sends auris_end_voice, which the client only sends when it hears the
---player speak, which needs voice enabled and the speaker audible.
function CHRONOS.Actions.voicechain(ply)
    local out = {}
    local function say(fmt, ...)
        out[#out + 1] = string.format(fmt, ...)
    end

    say("auris module: %s", auris and "loaded" or "MISSING")
    say("auris ready: %s", (Auris and isfunction(Auris.IsReady) and Auris.IsReady())
        and "yes" or "NO")
    say("auris.FlushRaw: %s", (auris and isfunction(auris.FlushRaw)) and "present" or "MISSING")
    say("chronos hooked: %s", CHRONOS.VoiceHooked and "yes" or "NO")
    say("sv_alltalk: %s   (0 means only teammates hear, so no flush)",
        GetConVar("sv_alltalk") and GetConVar("sv_alltalk"):GetString() or "?")
    say("sv_voiceenable: %s  (0 disables voice entirely)",
        GetConVar("sv_voiceenable") and GetConVar("sv_voiceenable"):GetString() or "?")

    -- Another addon on this hook that returns true first would consume the
    -- utterance before chronos ever sees it.
    local interceptors = hook.GetTable()["Auris_VoiceEnd"]
    say("Auris_VoiceEnd hooks: %s",
        interceptors and table.concat(table.GetKeys(interceptors), ", ") or "none")

    local key = GetConVar("chronos_voicehost")
    say("clip host convar: %q", key and key:GetString() or "")

    for _, line in ipairs(out) do
        if IsValid(ply) then ply:PrintMessage(HUD_PRINTCONSOLE, line) else print(line) end
    end
end

function CHRONOS.Actions.voicestats(ply)
    local stats = chronos.ClipStats()
    local url = CHRONOS.ClipURL()
    -- VoiceHooked only says Subscribe was called. This checks the hook is still
    -- on the table, which is what actually decides whether callbacks arrive.
    local live = hook.GetTable()["Auris_VoiceEnd"]
    local attached = live and live["Chronos_Voice"] ~= nil

    local line = string.format(
        "chronos voice: %s port %d, %d clips, %.1f / %.0f MB, auris %s, hook %s, ready %s, debug %s, url %s",
        stats.running and "running" or "stopped", stats.port, stats.clips,
        stats.bytes / 1048576, stats.cap / 1048576,
        CHRONOS.VoiceHooked and "subscribed" or "absent",
        attached and "attached" or "MISSING",
        (Auris and isfunction(Auris.IsReady) and Auris.IsReady()) and "yes" or "NO",
        CHRONOS.VoiceDebug and "on" or "off", url ~= "" and url or "unset")

    if IsValid(ply) then
        ply:PrintMessage(HUD_PRINTCONSOLE, line)
    else
        print(line)
    end
end
