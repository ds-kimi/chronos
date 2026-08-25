---The control measurement: instrumentation on, recording off. Everything the
---engine costs without Chronos in the loop, so the recording rows below can be
---read as a difference instead of a guess.
function CHRONOS.BenchEndWarmup()
    local b = CHRONOS.Bench
    if CHRONOS.Mode == "recording" then CHRONOS.Actions.stoponly() end

    -- The bot console command is deferred, so the only count worth recording is
    -- the one taken after the warmup they were spawned for.
    b.Bots = #CHRONOS.BenchBots()

    CHRONOS.BenchEnable(true, b.Deep)
    b.Phase = "baseline"
    b.Until = CurTime() + b.Baseline
end

function CHRONOS.BenchEndBaseline()
    local b = CHRONOS.Bench
    CHRONOS.BenchLog(chronos.BenchReport(CHRONOS.BenchLabel("BASELINE (recording off)"), CHRONOS.BenchTickMs()))

    CHRONOS.Actions.record()
    chronos.BenchReset()
    b.Phase = "record"
    b.Until = CurTime() + b.Record
end

function CHRONOS.BenchEndRecord()
    local b = CHRONOS.Bench
    local live = chronos.BenchSample().live
    CHRONOS.BenchLog(chronos.BenchReport(CHRONOS.BenchLabel("RECORDING"), CHRONOS.BenchTickMs()))
    CHRONOS.BenchAddRow(live)

    CHRONOS.Actions.stoponly()
    CHRONOS.BenchIgnoreHumans(true)
    chronos.BenchReset()
    b.Phase = "seek"
    b.SeeksLeft = b.Seeks
end

---Seeks are spread over driver steps rather than run in one burst: fifty of
---them back to back is a second-long freeze that the tickrate sample would
---then have to be explained around.
function CHRONOS.BenchSeekStep()
    local b = CHRONOS.Bench
    local first, last = chronos.GetRange()
    math.randomseed(2000 + b.SeeksLeft)

    for _ = 1, math.min(b.SeeksPerStep, b.SeeksLeft) do
        if first and last and last > first then chronos.Restore(math.random(first, last)) end
        b.SeeksLeft = b.SeeksLeft - 1
    end

    if b.SeeksLeft > 0 then return end

    CHRONOS.BenchLog(chronos.BenchReport(CHRONOS.BenchLabel("SEEK"), CHRONOS.BenchTickMs()))
    CHRONOS.BenchIgnoreHumans(false)
    CHRONOS.Actions.clear()
    CHRONOS.BenchBeginRound()
end
