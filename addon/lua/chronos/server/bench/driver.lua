---A run that pushes the server past these is measuring a stall rather than a
---workload, so it stops and the report names the guard that tripped.
---@return string|nil reason
function CHRONOS.BenchGuard()
    local b = CHRONOS.Bench
    local sample = chronos.BenchSample()
    local target = 1 / engine.TickInterval()

    if sample.frames > 200 and sample.tps > 0 and sample.tps < target * b.MinTpsFraction then
        return string.format("tickrate collapsed to %.1f of %.1f", sample.tps, target)
    end

    if chronos.ProcStats().ws / 1048576 > b.MaxWorkingSetMB then
        return string.format("working set past %d MB", b.MaxWorkingSetMB)
    end

    return nil
end

---@param rounds number|nil
---@param seconds number|nil
---@param deep boolean|nil
function CHRONOS.BenchStart(rounds, seconds, deep)
    local b = CHRONOS.Bench
    if b.Running then return print("chronos bench: already running") end

    b.Rounds = math.Clamp(tonumber(rounds) or b.Rounds, 1, 20)
    b.Record = math.Clamp(tonumber(seconds) or b.Record, 5, 600)
    b.Deep = deep and true or false
    b.Running, b.Round, b.Blocks, b.Rows, b.Aborted = true, 0, {}, {}, nil

    -- An empty srcds hibernates: frames stop, timers crawl, and the first round
    -- would be measuring a sleeping server rather than an idle one.
    RunConsoleCommand("sv_hibernate_think", "1")
    CHRONOS.BenchLog(CHRONOS.BenchHeader())
    CHRONOS.BenchBeginRound()
end

---@param reason string|nil
function CHRONOS.BenchStop(reason)
    local b = CHRONOS.Bench
    if not b.Running then return end

    b.Aborted = reason
    b.Running, b.Phase = false, "idle"
    if CHRONOS.Mode == "recording" then CHRONOS.Actions.stoponly() end

    CHRONOS.BenchEnable(false, false)
    RunConsoleCommand("sv_hibernate_think", "0")
    CHRONOS.BenchRestoreHumans()
    CHRONOS.BenchClearProps()
    CHRONOS.BenchClearBots()
    CHRONOS.BenchLog(CHRONOS.BenchScaling())
    if reason then CHRONOS.BenchLog("\nABORTED: " .. reason) end

    print("chronos bench: report written to " .. CHRONOS.BenchWrite())
end

-- Half a second is fine granularity for phase boundaries measured in seconds,
-- and keeps the driver itself out of the numbers it is collecting.
timer.Create("chronos_bench", 0.5, 0, function()
    local b = CHRONOS.Bench
    if not b.Running then return end

    local reason = CHRONOS.BenchGuard()
    if reason then return CHRONOS.BenchStop(reason) end
    if b.Phase == "seek" then return CHRONOS.BenchSeekStep() end
    if CurTime() < b.Until then return end

    if b.Phase == "warmup" then return CHRONOS.BenchEndWarmup() end
    if b.Phase == "baseline" then return CHRONOS.BenchEndBaseline() end
    if b.Phase == "record" then return CHRONOS.BenchEndRecord() end
end)
