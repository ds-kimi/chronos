CHRONOS = CHRONOS or {}
CHRONOS.Mode = "idle"
CHRONOS.Cursor = 0
CHRONOS.Playing = false
CHRONOS.Frames = 0
CHRONOS.Speed = 1
CHRONOS.ViewLock = false
CHRONOS.Ghosts = true
CHRONOS.Megabytes = 0
CHRONOS.MegabyteCap = 0
CHRONOS.EventCount = 0
CHRONOS.EffectCount = 0
---True while this client is watching a private stage replay rather than a
---scrub that has the whole server pinned.
CHRONOS.Staged = false
---Interpolated cursor: the server only reports 10 times a second, so the
---timeline is advanced locally in between to keep playback smooth.
CHRONOS.SmoothCursor = 0
---@class ChronosRange
---@field first number
---@field last number
CHRONOS.Range = CHRONOS.Range or { first = 0, last = 0 }

include("chronos/client/ui/theme.lua")
include("chronos/client/ui/timeline.lua")
include("chronos/client/ui/layout.lua")
include("chronos/client/ui/playback.lua")
include("chronos/client/ui/explain.lua")
include("chronos/client/ui/modeswitch.lua")
include("chronos/client/ui/panel.lua")
include("chronos/client/ui/hud.lua")
include("chronos/client/voice/voice.lua")
include("chronos/client/voice/sync.lua")
include("chronos/client/voice/play.lua")

net.Receive("chronos_state", function()
    local mode, cursor, playing = CHRONOS.Mode, CHRONOS.Cursor, CHRONOS.Playing
    CHRONOS.Mode = net.ReadString()
    CHRONOS.Range.first = net.ReadInt(32)
    CHRONOS.Range.last = net.ReadInt(32)
    CHRONOS.Cursor = net.ReadInt(32)
    CHRONOS.Playing = net.ReadBool()
    CHRONOS.Frames = net.ReadInt(32)
    CHRONOS.Speed = net.ReadFloat()
    CHRONOS.ViewLock = net.ReadBool()
    CHRONOS.Ghosts = net.ReadBool()
    CHRONOS.Megabytes = net.ReadFloat()
    CHRONOS.MegabyteCap = net.ReadFloat()
    CHRONOS.EventCount = net.ReadInt(32)
    CHRONOS.EffectCount = net.ReadInt(32)
    CHRONOS.Staged = net.ReadBool()

    -- Voice resync reads the cursor, so it has to be current before the
    -- branch below rather than after it.
    CHRONOS.SmoothCursor = CHRONOS.Cursor

    -- A jump backwards, or leaving replay, means whatever is still streaming
    -- belongs to a moment that is no longer on screen. Scrubbing while already
    -- paused counts too: the cursor only moves on its own during playback, so a
    -- move with playback stopped is a hand on the slider.
    local scrubbed = not playing and not CHRONOS.Playing and CHRONOS.Cursor ~= cursor
    if CHRONOS.Mode ~= mode or CHRONOS.Cursor < cursor or scrubbed then
        CHRONOS.StopVoice()
    elseif CHRONOS.Playing ~= playing then
        -- Pausing holds the sentence where it is rather than letting it finish.
        CHRONOS.PauseVoice(not CHRONOS.Playing)
    end
end)

hook.Add("Think", "chronos_interpolate", function()
    if CHRONOS.Mode ~= "replay" or not CHRONOS.Playing then
        CHRONOS.SmoothCursor = CHRONOS.Cursor
        return
    end

    local step = FrameTime() / engine.TickInterval() * CHRONOS.Speed
    CHRONOS.SmoothCursor = math.min(CHRONOS.SmoothCursor + step, CHRONOS.Range.last)
end)

concommand.Add("chronos_ui", function()
    CHRONOS.TogglePanel()
end)

concommand.Add("chronos_ui_close", function()
    if IsValid(CHRONOS.Panel) then
        CHRONOS.SetCursor(false)
        CHRONOS.Panel:Remove()
    end
end)
