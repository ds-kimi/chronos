---@class ChronosState
---@field Mode string "idle" | "recording" | "replay"
---@field Cursor number Tick currently pinned on the world during replay
---@field Playing boolean Whether the cursor advances on its own
---@field Speed number Ticks advanced per server tick
CHRONOS = CHRONOS or {}
CHRONOS.Loaded = false

-- A legacy addons/ folder is not sent to clients on its own, so every file the
-- client needs has to be listed here, this loader included.
if SERVER then
    AddCSLuaFile()
    AddCSLuaFile("chronos/shared/config.lua")
    AddCSLuaFile("chronos/shared/freeze.lua")
    AddCSLuaFile("chronos/client/stage.lua")
    AddCSLuaFile("chronos/client/ui/ui.lua")
    AddCSLuaFile("chronos/client/ui/theme.lua")
    AddCSLuaFile("chronos/client/ui/timeline.lua")
    AddCSLuaFile("chronos/client/ui/playback.lua")
    AddCSLuaFile("chronos/client/ui/explain.lua")
    AddCSLuaFile("chronos/client/ui/modeswitch.lua")
    AddCSLuaFile("chronos/client/ui/layout.lua")
    AddCSLuaFile("chronos/client/ui/panel.lua")
    AddCSLuaFile("chronos/client/ui/hud.lua")
    AddCSLuaFile("chronos/client/voice/voice.lua")
    AddCSLuaFile("chronos/client/voice/sync.lua")
    AddCSLuaFile("chronos/client/voice/play.lua")
end

include("chronos/shared/config.lua")
include("chronos/shared/freeze.lua")

if CLIENT then
    include("chronos/client/stage.lua")
    include("chronos/client/ui/ui.lua")
    CHRONOS.Loaded = true
    return
end

local ok, err = pcall(require, "chronos")
if not ok then
    ErrorNoHalt("[chronos] binary module failed to load: " .. tostring(err) .. "\n")
    ErrorNoHalt("[chronos] expected garrysmod/lua/bin/gmsv_chronos_win32.dll\n")
    return
end

if not istable(chronos) then
    ErrorNoHalt("[chronos] module loaded but the chronos table is missing\n")
    return
end

util.AddNetworkString("chronos_state")
util.AddNetworkString("chronos_ctl")
util.AddNetworkString("chronos_voice")
util.AddNetworkString("chronos_voice_manifest")
util.AddNetworkString("chronos_stagesnd")

include("chronos/server/core/state.lua")
include("chronos/server/core/broadcast.lua")
CHRONOS.Actions = CHRONOS.Actions or {}
include("chronos/server/core/aim.lua")
include("chronos/server/world/events.lua")
include("chronos/server/voice/voice.lua")
include("chronos/server/voice/trim.lua")
include("chronos/server/voice/capture.lua")
include("chronos/server/world/events_play.lua")
include("chronos/server/world/selfbody.lua")
include("chronos/server/world/identity.lua")
include("chronos/server/world/ghosts.lua")
include("chronos/server/world/ghostlife.lua")
include("chronos/server/world/visibility.lua")
include("chronos/server/core/replay.lua")
include("chronos/server/stage/stage.lua")
include("chronos/server/stage/clones.lua")
include("chronos/server/stage/sync.lua")
include("chronos/server/stage/transmit.lua")
include("chronos/server/stage/drive.lua")
include("chronos/server/core/runtick.lua")
include("chronos/server/core/hooks.lua")
include("chronos/server/commands/commands.lua")
include("chronos/server/commands/play.lua")
include("chronos/server/commands/log.lua")
include("chronos/server/commands/diag.lua")
include("chronos/server/commands/misc.lua")
include("chronos/server/commands/tune.lua")
include("chronos/server/commands/voice.lua")
include("chronos/server/bench/bench.lua")
include("chronos/server/bench/bots.lua")
include("chronos/server/bench/botai.lua")
include("chronos/server/bench/load.lua")
include("chronos/server/bench/report.lua")
include("chronos/server/bench/rounds.lua")
include("chronos/server/bench/phases.lua")
include("chronos/server/bench/driver.lua")
include("chronos/server/bench/commands.lua")
include("chronos/server/net/net.lua")

-- Both of these used to surface as "attempt to call a nil value" on the first
-- button press: a C++ function without a Lua binding, or an include that never
-- ran. Fail at load instead, naming exactly what is absent.
for _, name in ipairs({ "Start", "Stop", "Clear", "Capture", "Restore", "GetRange",
    "GetEntities", "GetStats", "SetMemoryCap", "SetKeyInterval", "SetIgnore",
    "ClearIgnore", "SetSkip", "ClearSkip", "BindProxy", "ClearProxies", "GetTransform", "ReadProp",
    "PlayEffects", "ClearEffects", "EffectCount", "StartClipServer",
    "StopClipServer", "AddClip", "ClearClips", "SetClipCap", "ClipStats", "GetPlan", "IsRecording",
    "BenchEnable", "BenchReset", "BenchMark", "BenchSample", "BenchReport", "ProcStats" }) do
    if not isfunction(chronos[name]) then
        ErrorNoHalt("[chronos] chronos." .. name .. " is missing from the module" .. "\n")
    end
end

-- A missing include used to surface as "attempt to call a nil value" on the
-- first button press. Fail at load instead, naming what is absent.
for _, name in ipairs({ "Broadcast", "SetSpectator", "PinPlayers", "PushEvent",
    "ClearEvents", "PlayEvents", "SyncGhosts", "SyncVisibility", "SyncWorld", "StampWorld", "StampEntity", "LiveOriginal", "ReleaseImpostors", "HideGhosts", "DropStaleGhosts",
    "EnterReplay", "ExitReplay", "AdvanceCursor", "CaptureAim", "RestoreAim",
    "UpdateBodies", "ClearBodies", "ClipURL", "EmitVoice", "HookAuris", "MarkSpeakers",
    "DropVoiceBuffers", "TrimVoice",
    "EnterStage", "LeaveStage", "StageTeardown", "StageViewers", "StageTick",
    "SyncStageClones", "ShowStageClones", "ClearStageClones", "SpawnStageClone",
    "SpawnStagePuppet", "SyncStageTransmit", "ClearStageTransmit",
    "UpdateStagePuppets", "PlayStageEvents",
    "PushEvent", "PushEventAt",
    "SendVoiceManifest",
    "RunTick", "BenchLog", "BenchTickMs", "BenchEnable", "BenchBots", "BenchAddBots",
    "BenchClearBots", "BenchSpawnOrigin", "BenchSpawnProps", "BenchClearProps",
    "BenchAddRow", "BenchScaling", "BenchHeader", "BenchWrite", "BenchGuard",
    "BenchStart", "BenchStop", "BenchBeginRound", "BenchLabel", "BenchIgnoreHumans",
    "BenchRestoreHumans", "BenchEndWarmup", "BenchEndBaseline", "BenchEndRecord",
    "BenchSeekStep" }) do
    if not isfunction(CHRONOS[name]) then
        ErrorNoHalt("[chronos] CHRONOS." .. name .. " is missing, an include did not run" .. "\n")
    end
end

CHRONOS.Loaded = true
print("[chronos] loaded, " .. table.Count(CHRONOS.Actions) .. " actions registered")
