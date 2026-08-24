---Splits a line to fit the space left of the button. Copy that reaches under a
---docked control reads as a rendering fault, and this box is the one place on
---the panel whose whole job is being read.
---@param text string
---@param width number
---@return string[]
local function wrap(text, width)
    surface.SetFont("ChronosUI")

    local lines, line = {}, ""
    for word in string.gmatch(text, "%S+") do
        local try = line == "" and word or line .. " " .. word
        if surface.GetTextSize(try) > width and line ~= "" then
            lines[#lines + 1] = line
            line = word
        else
            line = try
        end
    end

    lines[#lines + 1] = line
    return lines
end

---@param panel Panel
---@param title string
---@param body table Each entry is { text, colorKey }
local function paintBox(panel, title, body)
    panel.Paint = function(_, w, h)
        local colors = CHRONOS.Colors
        draw.RoundedBox(4, 0, 0, w, h, colors.warnbg)
        surface.SetDrawColor(colors.rec)
        surface.DrawOutlinedRect(0, 0, w, h, 1)
        draw.SimpleText(title, "ChronosLabel", 12, 14, colors.rec, 0, TEXT_ALIGN_CENTER)

        local y = 34
        for _, entry in ipairs(body) do
            for _, line in ipairs(wrap(entry[1], w - 236)) do
                draw.SimpleText(line, "ChronosUI", 12, y, colors[entry[2]], 0, TEXT_ALIGN_CENTER)
                y = y + 19
            end
        end
    end
end

---@param box Panel
local function repaint(box)
    if CHRONOS.View == "experimental" then
        paintBox(box, "YOU ARE IN EXPERIMENTAL MODE", {
            { "The server can and probably will crash.", "rec" },
            { "Props replay as stand-ins; carried weapons are missing.", "dim" },
            { "Recording keeps running, and nobody else is frozen.", "dim" }
        })
        return
    end

    paintBox(box, "EXPERIMENTAL MODE", {
        { "Watch the recording alone. Everybody else keeps playing.", "dim" },
        { "The server can and probably will crash.", "rec" },
        { "Do not use it on a server you care about.", "rec" }
    })
end

---@param box Panel
local function switch(box)
    CHRONOS.View = CHRONOS.View == "experimental" and "normal" or "experimental"

    -- A replay belongs to the mode that opened it, and a recording made under
    -- one set of rules is not worth carrying into the other: the two stop the
    -- world differently, so the switch starts from nothing.
    if CHRONOS.Staged then CHRONOS.Send("leave", 0) end
    if CHRONOS.Mode == "replay" then CHRONOS.Send("exit", 0) end
    if CHRONOS.Mode == "recording" then CHRONOS.Send("stoponly", 0) end
    CHRONOS.Send("clear", 0)

    CHRONOS.StopVoice()
    CHRONOS.Frames, CHRONOS.Cursor, CHRONOS.SmoothCursor = 0, 0, 0
    CHRONOS.Range.first, CHRONOS.Range.last = 0, 0
    repaint(box)
end

---@param frame Panel
function CHRONOS.BuildModeSwitch(frame)
    local box = CHRONOS.Row(frame, 96)
    repaint(box)

    local button = CHRONOS.Button(box,
        function() return CHRONOS.View == "experimental" and "Back to normal mode" or "Enter experimental mode" end,
        function() switch(box) end,
        function() return CHRONOS.View == "experimental" end)

    button:Dock(RIGHT)
    button:DockMargin(0, 20, 12, 20)
    button:SetWide(200)
end
