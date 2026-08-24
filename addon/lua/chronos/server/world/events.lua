---@class ChronosEvent
---@field kind string "sound" | "bullets" | "fx"
CHRONOS.Events = CHRONOS.Events or {}
CHRONOS.EventCount = 0
CHRONOS.MaxEvents = 20000
CHRONOS.RecTick = -1

---Events are keyed by the same frame number the module stamps on snapshots, so
---replaying a frame can fire everything that happened during it.
---@param kind string
---@param data table
function CHRONOS.PushEvent(kind, data)
    if CHRONOS.Mode ~= "recording" or CHRONOS.RecTick < 0 then return end

    CHRONOS.PushEventAt(CHRONOS.RecTick, kind, data)
end

---Files an event against an explicit frame. Voice needs this: it arrives whole
---seconds after it was spoken, so it cannot use the current frame.
---@param tick number
---@param kind string
---@param data table
function CHRONOS.PushEventAt(tick, kind, data)
    if tick < 0 then return end

    local bucket = CHRONOS.Events[tick]
    if not bucket then
        bucket = {}
        CHRONOS.Events[tick] = bucket
    end

    data.kind = kind
    bucket[#bucket + 1] = data
    CHRONOS.EventCount = CHRONOS.EventCount + 1

    if CHRONOS.EventCount > CHRONOS.MaxEvents then
        CHRONOS.PruneEvents()
    end
end

---Drops buckets the snapshot ring has already forgotten, then the oldest ones
---if the event budget is still blown.
function CHRONOS.PruneEvents()
    local first = chronos.GetRange() or 0

    for tick, bucket in pairs(CHRONOS.Events) do
        if tick < first then
            CHRONOS.EventCount = CHRONOS.EventCount - #bucket
            CHRONOS.Events[tick] = nil
        end
    end

    while CHRONOS.EventCount > CHRONOS.MaxEvents do
        local oldest = math.huge
        for tick in pairs(CHRONOS.Events) do
            oldest = math.min(oldest, tick)
        end

        if oldest == math.huge then break end
        CHRONOS.EventCount = CHRONOS.EventCount - #CHRONOS.Events[oldest]
        CHRONOS.Events[oldest] = nil
    end
end

function CHRONOS.ClearEvents()
    CHRONOS.Events = {}
    CHRONOS.EventCount = 0
    CHRONOS.RecTick = -1
end

-- One hook catches every sound on the server, engine or Lua, for every entity.
hook.Add("EntityEmitSound", "chronos_sound", function(data)
    -- Stand-ins are furniture a stage replay spawned; whatever noise they make
    -- coming into and out of the world is not part of the live recording.
    if IsValid(data.Entity) and data.Entity.ChronosStage ~= nil then return end

    CHRONOS.PushEvent("sound", {
        name = data.SoundName,
        pos = data.Pos or (IsValid(data.Entity) and data.Entity:GetPos()),
        level = data.SoundLevel,
        pitch = data.Pitch,
        volume = data.Volume,
        ent = IsValid(data.Entity) and data.Entity:EntIndex() or nil
    })
end)
