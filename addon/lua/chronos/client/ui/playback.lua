local SPEEDS = { 0.25, 0.5, 1, 2 }

-- Named for what they change about the replay rather than for the convar behind
-- them. All three only mean anything while the server is held at a tick.
local TOGGLES = {
    { "Fly instead of being replayed", "spectate",
        function() return CHRONOS.Mode == "replay" and not LocalPlayer():GetNWBool("ChronosFrozen", false) end },
    { "Force players' own view", "viewlock", function() return CHRONOS.ViewLock end },
    { "Rebuild deleted props", "ghosts", function() return CHRONOS.Ghosts end }
}

---@return boolean
local function replaying()
    return CHRONOS.Mode == "replay"
end

---@param frame Panel
function CHRONOS.BuildSpeed(frame)
    local bar = CHRONOS.Row(frame, 30, "SPEED")

    for _, speed in ipairs(SPEEDS) do
        local button = CHRONOS.Button(bar, speed .. "x",
            function() CHRONOS.Send("speed", speed) end,
            function() return math.abs(CHRONOS.Speed - speed) < 0.01 end, replaying)
        button:Dock(LEFT)
        button:DockMargin(6, 0, 0, 0)
        button:SetWide(60)
    end

    bar:GetChildren()[1]:DockMargin(56, 0, 0, 0)
end

---@param frame Panel
function CHRONOS.BuildToggles(frame)
    local bar = CHRONOS.Row(frame, 30, "VIEW")

    for _, entry in ipairs(TOGGLES) do
        local label, command, state = entry[1], entry[2], entry[3]
        local button = CHRONOS.Button(bar, label,
            function() CHRONOS.Send(command, state() and 0 or 1) end, state,
            function() return replaying() and CHRONOS.View ~= "experimental" end)
        button:Dock(LEFT)
        button:DockMargin(6, 0, 0, 0)
        button:SetWide(196)
    end

    bar:GetChildren()[1]:DockMargin(56, 0, 0, 0)
end
