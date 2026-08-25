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

---Why an entity is or is not on screen at the cursor, per recorded index. The
---ghost path has three ways to end in nothing visible -- no manifest entry, a
---refused spawn, a muted slot -- and they look identical from in front of it.
---@param ply Player|nil
function CHRONOS.Actions.diagents(ply)
    local manifest = chronos.GetEntities(CHRONOS.Cursor)
    say(ply, string.format("[chronos] manifest at tick %d: %d entries",
        CHRONOS.Cursor, table.Count(manifest)))

    for index, info in pairs(manifest) do
        local ghost = CHRONOS.Ghosts[index]
        local live = Entity(index)
        say(ply, string.format("  #%d %s %s recborn=%s | ghost=%s nodraw=%s | live=%s born=%s | mine=%s muted=%s",
            index, tostring(info.class), tostring(info.model), tostring(info.born),
            IsValid(ghost) and ghost:EntIndex() or "none",
            IsValid(ghost) and tostring(ghost:GetNoDraw()) or "-",
            IsValid(live) and live:GetClass() or "none",
            IsValid(live) and tostring(live.ChronosBorn) or "-",
            tostring(CHRONOS.LiveOriginal(index, info, CHRONOS.Cursor) ~= nil),
            tostring(CHRONOS.Impostors[index] ~= nil)))
    end
end
