-- Classes that can be recreated as themselves and driven by the module. Anything
-- else gets a prop_dynamic puppet instead: spawning a live NPC or weapon would
-- run its AI and its pickup logic inside a world that is still being played in.
-- prop_ragdoll is deliberately not here: spawning one on a model with no ragdoll
-- data takes the server down without a dump, and a puppet shows the same pose.
local CLONEABLE = {
    prop_physics = true,
    prop_physics_multiplayer = true,
    prop_physics_override = true,
    prop_dynamic = true,
    prop_dynamic_override = true
}

---A spawn that takes the server down leaves no dump, so tracing is what named
---the clone path as the one that crashed. Off by default, kept for the next one.
CreateConVar("chronos_stagedebug", "0", FCVAR_ARCHIVE, "Log every stage stand-in as it is created")
CreateConVar("chronos_stageclones", "1", FCVAR_ARCHIVE, "Recreate props as their own class instead of puppets")

---@param what string
---@param index number
---@param info table
local function trace(what, index, info)
    if GetConVar("chronos_stagedebug"):GetInt() == 0 then return end

    print(string.format("[chronos] stage %s #%d %s %s", what, index,
        tostring(info.class), tostring(info.model)))
end

---@param index number Recorded edict index
---@param info table { class = string, model = string }
---@return Entity|nil
function CHRONOS.SpawnStageClone(index, info)
    if GetConVar("chronos_stageclones"):GetInt() == 0 then return nil end
    if not CLONEABLE[info.class] then return nil end

    -- Spawning a prop on an empty or unprecached model is a hard crash in the
    -- engine, not a Lua error, so the model is proven before anything exists.
    if not info.model or info.model == "" or not util.IsValidModel(info.model) then return nil end

    trace("clone", index, info)
    local clone = ents.Create(info.class)
    if not IsValid(clone) then return nil end

    clone:SetModel(info.model)
    clone:Spawn()
    trace("clone ok", index, info)
    clone.ChronosStage = index

    -- Replay furniture standing in a live world: never solid to the players who
    -- are still in it, and never simulated, since the module writes its origin.
    -- The physics object is destroyed rather than frozen. A stand-in that keeps
    -- one is carrying collision state the restore no longer writes, and the two
    -- disagreeing is what took the server down inside a later Spawn.
    clone:PhysicsDestroy()
    clone:SetSolid(SOLID_NONE)
    clone:SetMoveType(MOVETYPE_NONE)
    clone:SetCollisionGroup(COLLISION_GROUP_IN_VEHICLE)

    chronos.SetSkip(clone:EntIndex(), true)
    chronos.BindProxy(index, clone:EntIndex())
    return clone
end

-- Entities that only exist attached to something else. A carried weapon and a
-- pair of viewmodel arms are parented to their player, so their recorded origin
-- is the parent-relative zero: puppeting them piles hundreds of models on the
-- map origin until the edict pool runs dry and the server goes down.
local NEVER_STAGED = {
    viewmodel = true,
    predicted_viewmodel = true,
    gmod_hands = true,
    hands = true,
    physgun_beam = true,
    player_manager = true
}

---@param class string
---@return boolean
local function stageable(class)
    return not NEVER_STAGED[class] and string.sub(class, 1, 7) ~= "weapon_"
end

---Everything the module cannot safely restore into is shown as a model on a
---prop_dynamic driven from the rebuilt state: players above all, since
---CBasePlayer cannot be spawned, but NPCs and ragdolls too.
---@param index number
---@param info table
---@return Entity|nil puppet
---@return boolean|nil retry True when the entity may be worth puppeting later
function CHRONOS.SpawnStagePuppet(index, info)
    if not stageable(info.class) then return nil end
    if not info.model or info.model == "" or not util.IsValidModel(info.model) then return nil end

    -- An entity sitting exactly on the map origin at this tick is parented to
    -- something, not standing there. It may get a real position later, so this
    -- is a retry rather than a refusal.
    local pos = chronos.GetTransform(index)
    if not pos or pos:IsZero() then return nil, true end

    trace("puppet", index, info)

    -- A recorded model is not necessarily still precached: the entity that
    -- carried it may have been gone long before the stage opened.
    util.PrecacheModel(info.model)

    local puppet = ents.Create("prop_dynamic")
    if not IsValid(puppet) then return nil end

    puppet:SetModel(info.model)
    puppet:Spawn()
    trace("puppet ok", index, info)
    puppet:SetSolid(SOLID_NONE)
    puppet:SetMoveType(MOVETYPE_NONE)
    puppet:SetCollisionGroup(COLLISION_GROUP_IN_VEHICLE)
    puppet.ChronosStage = index

    -- Networked so the client routes that player's recorded voice onto the
    -- stand-in rather than onto whoever holds that edict index right now.
    puppet:SetNWInt("ChronosSpeaker", index)

    chronos.SetSkip(puppet:EntIndex(), true)
    return puppet
end
