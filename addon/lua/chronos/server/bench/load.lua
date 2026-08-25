local PROP_MODEL = "models/props_c17/oildrum001.mdl"
CHRONOS.Bench.PropList = CHRONOS.Bench.PropList or {}

---@return Vector centre of the prop swarm
function CHRONOS.BenchSpawnOrigin()
    local starts = ents.FindByClass("info_player_start")
    if #starts > 0 then return starts[1]:GetPos() end

    local human = player.GetHumans()[1]
    return IsValid(human) and human:GetPos() or Vector(0, 0, 0)
end

---Props scale the edict scan independently of player count: they settle and
---stop changing, so they cost the loop without inflating the delta volume.
---@param count number
function CHRONOS.BenchSpawnProps(count)
    local origin = CHRONOS.BenchSpawnOrigin()

    -- Seeded per round so every run of the ladder lays the props out
    -- identically. Two runs comparing different worlds is not a comparison.
    math.randomseed(1000 + #CHRONOS.Bench.PropList)

    for _ = 1, count do
        local prop = ents.Create("prop_physics")
        if not IsValid(prop) then break end

        prop:SetModel(PROP_MODEL)
        prop:SetPos(origin + Vector(math.random(-900, 900), math.random(-900, 900), math.random(40, 500)))
        prop:Spawn()
        CHRONOS.Bench.PropList[#CHRONOS.Bench.PropList + 1] = prop
    end
end

function CHRONOS.BenchClearProps()
    for _, prop in ipairs(CHRONOS.Bench.PropList) do
        if IsValid(prop) then prop:Remove() end
    end

    CHRONOS.Bench.PropList = {}
end
