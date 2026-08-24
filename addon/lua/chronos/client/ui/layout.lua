---Which half of the panel is on screen. The normal mode drives the replay the
---whole server watches together; experimental drives the private one. Controls
---keep their place across the two, so only their meaning changes.
CHRONOS.View = CHRONOS.View or "normal"

---@param parent Panel
---@param tall number
---@param label string|nil
---@return Panel
function CHRONOS.Row(parent, tall, label)
    local panel = vgui.Create("DPanel", parent)
    panel:Dock(TOP)
    panel:SetTall(tall)
    panel:DockMargin(0, 0, 0, 8)

    panel.Paint = function(_, _, h)
        if label then
            draw.SimpleText(label, "ChronosLabel", 0, h * 0.5, CHRONOS.Colors.dim, 0, TEXT_ALIGN_CENTER)
        end
    end

    return panel
end

---The third button is the one that differs: normal hands the world back to
---everybody, experimental steps you in and out of your own replay.
---@param bar Panel
local function buildThird(bar)
    local lab = function()
        if CHRONOS.View ~= "experimental" then return "Unfreeze and resume" end
        return CHRONOS.Staged and "Return to the game" or "Review privately"
    end

    local button = CHRONOS.Button(bar, lab, function()
        if CHRONOS.View ~= "experimental" then return CHRONOS.Send("exit", 0) end
        CHRONOS.Send(CHRONOS.Staged and "leave" or "review", 0)
    end, function() return CHRONOS.Staged end, function()
        if CHRONOS.View ~= "experimental" then return CHRONOS.Mode == "replay" end
        return CHRONOS.Staged or CHRONOS.Frames > 0
    end, function()
        return CHRONOS.View == "experimental" and not CHRONOS.Staged
            and CHRONOS.Mode == "idle" and CHRONOS.Frames > 0
    end)

    button:Dock(LEFT)
    button:SetWide(190)
end

---@param frame Panel
function CHRONOS.BuildTransport(frame)
    local bar = CHRONOS.Row(frame, 30)
    local recording = function() return CHRONOS.Mode == "recording" end

    local record = CHRONOS.Button(bar, function()
        if not recording() then return "Start recording" end
        return CHRONOS.View == "experimental" and "Stop recording" or "Stop and review"
    end, function()
        if not recording() then return CHRONOS.Send("record", 0) end
        CHRONOS.Send(CHRONOS.View == "experimental" and "stoponly" or "stop", 0)
    end, recording)
    record:Dock(LEFT)
    record:DockMargin(0, 0, 8, 0)
    record:SetWide(170)

    local play = CHRONOS.Button(bar,
        function() return CHRONOS.Playing and "Pause" or "Play" end,
        function() CHRONOS.Send(CHRONOS.Playing and "pause" or "play", 0) end,
        function() return CHRONOS.Playing end,
        function() return CHRONOS.Mode == "replay" end)
    play:Dock(LEFT)
    play:DockMargin(0, 0, 8, 0)
    play:SetWide(110)

    buildThird(bar)
end
