local USAGE = {
    "chronos bench run [rounds] [seconds] [deep]  scheduled load ladder",
    "chronos bench stop                           end the run and write the report",
    "chronos bench now                            print the current window immediately",
    "chronos bench on [deep] | off                instrumentation without a schedule",
    "chronos bench reset                          restart the measurement window"
}

---@param ply Player|nil
---@param lines string[]
local function Send(ply, lines)
    for _, line in ipairs(lines) do
        if IsValid(ply) then ply:PrintMessage(HUD_PRINTCONSOLE, line) else print(line) end
    end
end

---Prints the window as it stands right now, with no schedule around it.
local function Immediate()
    if not chronos.BenchSample().on then
        return print("chronos bench: instrumentation is off, run 'chronos bench on' first")
    end

    print(chronos.BenchReport("IMMEDIATE  players " .. #player.GetAll(), CHRONOS.BenchTickMs()))
end

---@param ply Player|nil
---@param args string[]
function CHRONOS.Actions.bench(ply, args)
    local what = string.lower(args[1] or "")

    if what == "run" then
        CHRONOS.BenchStart(args[2], args[3], string.lower(args[4] or "") == "deep")
    elseif what == "stop" then
        CHRONOS.BenchStop(nil)
    elseif what == "now" then
        Immediate()
    elseif what == "on" then
        CHRONOS.BenchEnable(true, string.lower(args[2] or "") == "deep")
        print("chronos bench: instrumentation on")
    elseif what == "off" then
        CHRONOS.BenchEnable(false, false)
        print("chronos bench: instrumentation off")
    elseif what == "reset" then
        chronos.BenchReset()
    else
        Send(ply, USAGE)
    end
end
