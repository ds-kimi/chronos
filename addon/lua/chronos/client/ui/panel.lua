---Mouse capture is separate from visibility: the panel stays on screen while
---the cursor goes back to the game, so a scrub can be watched while flying.
---@param enabled boolean
function CHRONOS.SetCursor(enabled)
    local frame = CHRONOS.Panel
    if not IsValid(frame) then return end

    CHRONOS.CursorOn = enabled
    frame:SetMouseInputEnabled(enabled)
    frame:SetKeyboardInputEnabled(false)
    gui.EnableScreenClicker(enabled)
end

---@param frame Panel
local function paintFrame(frame)
    local colors = CHRONOS.Colors

    frame.Paint = function(_, w, h)
        local label, color = CHRONOS.StatusText()
        local loading, total = CHRONOS.VoiceLoading or 0, CHRONOS.VoiceTotal or 0
        if loading > 0 and total > 0 then
            label = string.format("loading voice %d/%d", total - loading, total)
            color = colors.dim
        end
        draw.RoundedBox(6, 0, 0, w, h, colors.bg)
        draw.RoundedBoxEx(6, 0, 0, w, 32, colors.header, true, true, false, false)
        draw.RoundedBox(0, 0, 31, w, 1, colors.line)
        draw.SimpleText("CHRONOS", "ChronosBold", 14, 16, colors.text, 0, TEXT_ALIGN_CENTER)
        draw.SimpleText(label, "ChronosUI", w - 14, 16, color, TEXT_ALIGN_RIGHT, TEXT_ALIGN_CENTER)
    end
end

---Reading order is the order of the work: decide what to do, see where you are
---in the recording, drive it, read what it means, and only then the controls
---that reach other people.
CHRONOS.PanelTall = 354

function CHRONOS.OpenPanel()
    local frame = vgui.Create("DFrame")
    frame:SetSize(680, CHRONOS.PanelTall)
    frame:Center()
    frame:SetTitle("")
    frame:ShowCloseButton(false)
    frame:DockPadding(14, 40, 14, 12)
    paintFrame(frame)

    CHRONOS.BuildTransport(frame)

    frame.Timeline = vgui.Create("ChronosTimeline", frame)
    frame.Timeline:Dock(TOP)
    frame.Timeline:DockMargin(0, 0, 0, 8)
    frame.Timeline:SetTall(30)

    CHRONOS.BuildSpeed(frame)
    CHRONOS.BuildToggles(frame)
    CHRONOS.BuildExplain(frame)
    CHRONOS.BuildModeSwitch(frame)
    CHRONOS.BuildStats(frame)

    CHRONOS.Panel = frame
    CHRONOS.SetCursor(true)
end

---One command drives everything: open, then toggle the mouse, and close from
---the console when it is no longer wanted.
function CHRONOS.TogglePanel()
    if not IsValid(CHRONOS.Panel) then
        CHRONOS.OpenPanel()
        return
    end

    CHRONOS.SetCursor(not CHRONOS.CursorOn)
end
