---The engine hands a freed edict index to the next spawn, so a recorded index
---is not proof of identity: a prop spawned after the recorded one was deleted
---inherits its slot. Restoring into it wrote a dead prop's whole state --
---owner fields included -- onto somebody's new prop, and hid the new prop on
---every tick the recording says nothing was there.
---
---Each entity therefore carries the tick it was born on, scoped to a recording
---session so stamps from an earlier recording cannot pass as current ones.
CHRONOS.RecSession = CHRONOS.RecSession or 0
---Indices this replay muted because a stranger took a recorded slot. Tracked
---separately from the module's ignore list, which spectators also write to.
CHRONOS.Impostors = CHRONOS.Impostors or {}

---Opens a new session and marks everything already standing as older than
---frame zero, so the world a recording starts with counts as recorded.
function CHRONOS.StampWorld()
    CHRONOS.RecSession = CHRONOS.RecSession + 1

    for _, ent in ipairs(ents.GetAll()) do
        ent.ChronosSession = CHRONOS.RecSession
        ent.ChronosBorn = -1
    end
end

---@param ent Entity
function CHRONOS.StampEntity(ent)
    ent.ChronosSession = CHRONOS.RecSession
    -- Anything spawned outside a recording is newer than every recorded tick,
    -- so no cursor position can ever claim it.
    ent.ChronosBorn = CHRONOS.Mode == "recording" and CHRONOS.RecTick or math.huge
end

---Ticks of slack between the tick an entity was created on and the tick the
---capture first saw it: a spawn lands between two captures.
local BIRTH_SLACK = 2

---Whether the entity sitting at a recorded index is the one that was recorded.
---Class and model prove nothing here -- spawning the same prop twice reuses
---both -- so the birth ticks have to agree: the recording's, carried in the
---manifest, and the live one stamped when the entity was created.
---@param index number Recorded edict index
---@param info table { class = string, model = string, born = number }
---@param tick number Tick being restored
---@return Entity|nil live The original, or nil if the slot changed hands
function CHRONOS.LiveOriginal(index, info, tick)
    local live = Entity(index)
    if not IsValid(live) then return nil end
    if live.ChronosSession ~= CHRONOS.RecSession then return nil end
    if live:GetClass() ~= info.class then return nil end
    if live:GetModel() ~= info.model then return nil end

    local born = live.ChronosBorn or math.huge
    -- Everything that predates frame zero is stamped -1 on both sides, so the
    -- map's own entities match without a birth tick ever being recorded.
    if info.born < 0 then return born < 0 and live or nil end

    return math.abs(born - info.born) <= BIRTH_SLACK and live or nil
end

---Lifts the mutes from the previous sync, so an index that has since been
---ghosted or handed back to its original can be written to again.
function CHRONOS.ReleaseImpostors()
    for index in pairs(CHRONOS.Impostors) do
        chronos.SetIgnore(index, false)
        CHRONOS.Impostors[index] = nil
    end
end

hook.Add("OnEntityCreated", "chronos_stamp", function(ent)
    if IsValid(ent) then CHRONOS.StampEntity(ent) end
end)
