---@param ply Player|nil
---@param line string
local function say(ply, line)
    if IsValid(ply) then ply:PrintMessage(HUD_PRINTCONSOLE, line) end
    print(line)
end

---Every state the two replay paths depend on, in one place: a stage that opens
---onto an empty ring and a recording that never started look identical from the
---panel, and both were being read as "the log button does nothing".
---@param ply Player|nil
function CHRONOS.Actions.diag(ply)
    local first, last, frames = chronos.GetRange()
    local stage = CHRONOS.Stage

    say(ply, string.format("[chronos] mode=%s module_recording=%s rectick=%d",
        CHRONOS.Mode, tostring(chronos.IsRecording()), CHRONOS.RecTick))
    say(ply, string.format("[chronos] ring: %s frames, ticks %s-%s, cursor %d",
        tostring(frames or 0), tostring(first or "none"), tostring(last or "none"), CHRONOS.Cursor))
    say(ply, string.format("[chronos] stage: %s, %d viewers, %d clones, %d puppets",
        stage and "open" or "closed",
        stage and table.Count(stage.Viewers) or 0,
        stage and table.Count(stage.Clones) or 0,
        stage and table.Count(stage.Puppets) or 0))
    say(ply, string.format("[chronos] world: %d entities, %d players",
        #ents.GetAll(), #player.GetAll()))
end
