---Load is added before the warmup that settles it, so the measured window never
---contains the spawn burst that created the load.
function CHRONOS.BenchBeginRound()
    local b = CHRONOS.Bench
    b.Round = b.Round + 1
    if b.Round > b.Rounds then return CHRONOS.BenchStop(nil) end

    if b.Round > 1 then
        CHRONOS.BenchAddBots(b.BotsPerRound)
    end

    CHRONOS.BenchSpawnProps(b.PropsPerRound)
    b.Props = #b.PropList
    b.Phase = "warmup"
    b.Until = CurTime() + b.Warmup
end

---@param what string
---@return string label for the report block
function CHRONOS.BenchLabel(what)
    local b = CHRONOS.Bench
    return string.format("ROUND %d %s  bots %d  props %d  players %d",
        b.Round, what, b.Bots, b.Props, #player.GetAll())
end

---A restore writes recorded bytes back over live edicts, and a human watching
---the run is a live edict. Ignoring them keeps the seek phase from rewinding
---whoever started it.
---@param ignored boolean
function CHRONOS.BenchIgnoreHumans(ignored)
    for _, ply in ipairs(player.GetHumans()) do
        chronos.SetIgnore(ply:EntIndex(), ignored)
    end
end

function CHRONOS.BenchRestoreHumans()
    CHRONOS.BenchIgnoreHumans(false)
end
