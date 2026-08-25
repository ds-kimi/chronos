---One line of the cross-round scaling matrix, taken while the round's samples
---are still live. This is the table that answers "what happens at 40 players".
---@param live number mean live entities captured per tick
function CHRONOS.BenchAddRow(live)
    local b = CHRONOS.Bench
    local sample = chronos.BenchSample()
    local proc = chronos.ProcStats()
    local frame = sample.frame_mean > 0 and sample.frame_mean or 1

    b.Rows[#b.Rows + 1] = {
        bots = b.Bots, props = b.Props, live = live,
        capture = sample.capture_mean,
        pct = sample.capture_mean / frame * 100,
        mbs = sample.framebytes * sample.tps / 1048576,
        tps = sample.tps,
        ws = proc.ws / 1048576
    }
end

---@return string
function CHRONOS.BenchScaling()
    local lines = { "", "== SCALING ==",
        string.format("%5s %6s %7s %13s %8s %8s %9s %8s",
            "bots", "props", "live", "capture.tick", "%frame", "MB/s", "tps", "ws MB") }

    for _, row in ipairs(CHRONOS.Bench.Rows) do
        lines[#lines + 1] = string.format("%5d %6d %7.0f %11.3fms %8.1f %8.2f %9.2f %8.0f",
            row.bots, row.props, row.live, row.capture, row.pct, row.mbs, row.tps, row.ws)
    end

    return table.concat(lines, "\n")
end

---@return string
function CHRONOS.BenchHeader()
    local b = CHRONOS.Bench
    local stats = chronos.GetStats()

    return string.format("CHRONOS BENCH  %s   map %s   tick %.2f (%.2f ms)\nkeyinterval %d   memcap %.0f MB   deep %s   rounds %d x %ds\n",
        os.date("%Y-%m-%d %H:%M:%S"), game.GetMap(), 1 / engine.TickInterval(),
        CHRONOS.BenchTickMs(), stats.keyinterval, stats.cap / 1048576,
        tostring(b.Deep), b.Rounds, b.Record)
end

---@return string path the report was written to
function CHRONOS.BenchWrite()
    local name = "chronos_bench/bench_" .. os.date("%Y%m%d_%H%M%S") .. ".txt"
    file.CreateDir("chronos_bench")
    file.Write(name, table.concat(CHRONOS.Bench.Blocks, "\n"))

    return "garrysmod/data/" .. name
end
