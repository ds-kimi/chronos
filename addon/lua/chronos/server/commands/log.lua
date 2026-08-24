---Experimental mode: review the recording on a private stage while the
---recording, and everybody else's game, keeps running. Nothing in the live
---world is written to.
---@param ply Player|nil
function CHRONOS.Actions.log(ply)
    if not IsValid(ply) then
        print("chronos: log is per player, run it in game")
        return
    end

    if CHRONOS.Stage and CHRONOS.Stage.Viewers[ply] then
        CHRONOS.LeaveStage(ply)
        return
    end

    if not CHRONOS.EnterStage(ply) then
        ply:PrintMessage(HUD_PRINTCONSOLE, "[chronos] nothing recorded")
        return
    end

    local stage = CHRONOS.Stage
    ply:PrintMessage(HUD_PRINTCONSOLE, string.format(
        "[chronos] stage open: %d clones, %d puppets, nobody else is affected",
        table.Count(stage.Clones), table.Count(stage.Puppets)))
end

---@param ply Player|nil
function CHRONOS.Actions.logexit(ply)
    if IsValid(ply) then CHRONOS.LeaveStage(ply) end
end

-- Named for what they do rather than for what the feature is called. "log" and
-- "logexit" stay because they are what the first version answered to.
CHRONOS.Actions.review = CHRONOS.Actions.log
CHRONOS.Actions.leave = CHRONOS.Actions.logexit
