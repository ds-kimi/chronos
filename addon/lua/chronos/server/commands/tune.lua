---@param args string[]
function CHRONOS.Actions.memcap(_, args)
    chronos.SetMemoryCap(tonumber(args[1]) or 512)
    CHRONOS.Actions.stats()
end

---Also the seek window: a jump can only be resolved from the last keyframe, so
---raising this trades memory for a longer scan on every scrub.
---@param args string[]
function CHRONOS.Actions.keyinterval(_, args)
    chronos.SetKeyInterval(math.Clamp(math.floor(tonumber(args[1]) or 64), 1, 512))
    CHRONOS.Actions.stats()
end

---@param args string[]
function CHRONOS.Actions.ghosts(_, args)
    CHRONOS.UseGhosts = tonumber(args[1]) ~= 0
    if not CHRONOS.UseGhosts and CHRONOS.Mode == "replay" then
        CHRONOS.ClearGhosts()
        CHRONOS.ClearHidden()
    end

    CHRONOS.LastSync = -1e9
end

---@param args string[]
function CHRONOS.Actions.viewlock(_, args)
    CHRONOS.ViewLock = tonumber(args[1]) ~= 0
end
