CHRONOS.Colors = {
    bg = Color(19, 20, 24, 246),
    header = Color(27, 29, 35, 255),
    line = Color(44, 47, 56),
    track = Color(42, 45, 53),
    fill = Color(88, 166, 255),
    knob = Color(232, 238, 246),
    text = Color(226, 230, 238),
    dim = Color(126, 133, 147),
    rec = Color(235, 87, 87),
    button = Color(36, 39, 47),
    hover = Color(52, 57, 68),
    active = Color(88, 166, 255),
    warnbg = Color(45, 26, 28, 255),
    go = Color(76, 175, 106)
}

surface.CreateFont("ChronosUI", { font = "Roboto", size = 15, weight = 500, antialias = true })
surface.CreateFont("ChronosBold", { font = "Roboto", size = 15, weight = 700, antialias = true })
surface.CreateFont("ChronosLabel", { font = "Roboto", size = 13, weight = 600, antialias = true })

---@param name string
---@param value number
function CHRONOS.Send(name, value)
    net.Start("chronos_ctl")
    net.WriteString(name)
    net.WriteFloat(value or 0)
    net.SendToServer()
end

---Plain words rather than transport jargon: the header answers "what is
---happening and to whom", which is the only question a reviewer actually has.
---@return string label
---@return Color color
function CHRONOS.StatusText()
    local colors = CHRONOS.Colors

    if CHRONOS.Mode == "recording" then
        return string.format("Recording  %d frames", CHRONOS.Frames), colors.rec
    end

    if CHRONOS.Mode == "replay" then
        local at = string.format("  %d / %d", math.floor(CHRONOS.SmoothCursor), CHRONOS.Range.last)
        if CHRONOS.Staged then
            return "Private review" .. at, colors.rec
        end

        return "Replay, server held" .. at, colors.fill
    end

    return "Live", colors.dim
end

---@param parent Panel
---@param label string|function A function relabels the button as state changes
---@param onClick function
---@param highlight function|nil Returns true when the button should read active
---@param enabled function|nil Returns false when the action cannot be taken now
---@param suggest function|nil Returns true when this is the obvious next action
---@return Panel
function CHRONOS.Button(parent, label, onClick, highlight, enabled, suggest)
    local button = vgui.Create("DButton", parent)
    button:SetText("")
    button:SetTall(30)

    -- A button that cannot do anything is drawn dim and does nothing when
    -- clicked, rather than being hidden: a control that appears and disappears
    -- as state changes is harder to learn than one that is visibly unavailable.
    button.DoClick = function()
        if enabled and not enabled() then return end
        onClick()
    end

    button.Paint = function(self, w, h)
        local colors = CHRONOS.Colors
        local live = enabled == nil or enabled()
        local on = live and highlight ~= nil and highlight()
        local next = live and not on and suggest ~= nil and suggest()
        local back = on and colors.active or (next and colors.go
            or (live and self:IsHovered() and colors.hover or colors.button))

        draw.RoundedBox(4, 0, 0, w, h, back)

        -- The suggested action breathes rather than just sitting there coloured:
        -- after a recording stops, the eye goes back to the button it just
        -- pressed, and this is what pulls it to the one that comes next.
        if next then
            surface.SetDrawColor(255, 255, 255, 40 + math.sin(CurTime() * 4) * 35)
            surface.DrawOutlinedRect(0, 0, w, h, 2)
        end

        draw.SimpleText(isfunction(label) and label() or label, "ChronosUI", w * 0.5, h * 0.5,
            (on or next) and colors.header or (live and colors.text or colors.dim),
            TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER)
    end

    return button
end
