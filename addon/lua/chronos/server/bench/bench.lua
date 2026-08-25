---@class ChronosBench
---@field Running boolean Whether a scheduled run is in flight
---@field Phase string "idle" | "warmup" | "baseline" | "record" | "seek"
---@field Round number 1-based index of the round being measured
---@field Blocks string[] Rendered report blocks, in order
---@field Rows table[] One scaling-matrix row per recorded round
CHRONOS.Bench = CHRONOS.Bench or {
    Running = false,
    Phase = "idle",
    Round = 0,
    Until = 0,
    SeeksLeft = 0,
    Bots = 0,
    Props = 0,
    Blocks = {},
    Rows = {},
    Aborted = nil,

    -- Defaults. Every one of these is overridable from the bench command.
    Rounds = 3,
    Warmup = 4,
    Baseline = 6,
    Record = 20,
    Seeks = 50,
    SeeksPerStep = 10,
    BotsPerRound = 10,
    PropsPerRound = 100,
    Deep = false,

    -- A run that pushes the server under these is measuring a stall, not a
    -- workload, so it stops and says which guard tripped.
    MinTpsFraction = 0.6,
    MaxWorkingSetMB = 3072
}

---Bench output goes to the server console as it happens and into the file at
---the end, because a run that crashes the server still has to leave evidence.
---@param line string
function CHRONOS.BenchLog(line)
    print(line)
    CHRONOS.Bench.Blocks[#CHRONOS.Bench.Blocks + 1] = line
end

---@return number milliseconds One server tick at the configured tickrate
function CHRONOS.BenchTickMs()
    return engine.TickInterval() * 1000
end

---Mirrors the module flag into Lua so the Tick hook can gate on a boolean
---instead of allocating a sample table sixty-six times a second.
---@param on boolean
---@param deep boolean|nil
function CHRONOS.BenchEnable(on, deep)
    CHRONOS.BenchClock = on and true or false
    chronos.BenchEnable(on, deep)
end
