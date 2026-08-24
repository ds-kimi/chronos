local PANEL = {}

---Frame under the mouse right now. Read from screen coordinates so a drag keeps
---tracking after the cursor leaves the bar.
---@return number
function PANEL:FrameAtCursor()
    local x = self:ScreenToLocal(gui.MousePos())

    -- The ring keeps growing while a stage is watched, so the live range would
    -- rescale the bar under the finger holding it. A drag reads the range it
    -- started on and the frame under the cursor stays where it was grabbed.
    local first = self.DragFirst or CHRONOS.Range.first
    local last = self.DragLast or CHRONOS.Range.last
    local frac = math.Clamp(x / math.max(self:GetWide(), 1), 0, 1)

    return math.Round(first + frac * math.max(last - first, 1))
end

function PANEL:OnMousePressed()
    self.Dragging = true
    self.LastSent = nil

    -- Scrubbing pauses, so whether playback resumes on release is decided by
    -- what it was doing before the grab. Letting go of a paused replay used to
    -- start it playing, which loses the frame you were looking for.
    self.WasPlaying = CHRONOS.Playing
    self.DragFirst = CHRONOS.Range.first
    self.DragLast = CHRONOS.Range.last
end

function PANEL:Think()
    if not self.Dragging then return end

    -- Release resumes playback: drag to look, let go to keep watching.
    if not input.IsMouseDown(MOUSE_LEFT) then
        self.Dragging = false
        self.LastSent = nil
        self.DragFirst = nil
        self.DragLast = nil

        if self.WasPlaying and CHRONOS.Mode == "replay" then
            CHRONOS.Send("play", 0)
        end

        return
    end

    local frame = self:FrameAtCursor()
    if frame ~= self.LastSent then
        self.LastSent = frame
        CHRONOS.Send("seek", frame)
    end
end

function PANEL:Paint(w, h)
    local colors = CHRONOS.Colors
    local span = math.max(CHRONOS.Range.last - CHRONOS.Range.first, 1)
    local frac = math.Clamp((CHRONOS.SmoothCursor - CHRONOS.Range.first) / span, 0, 1)
    local y = h * 0.5 - 3

    draw.RoundedBox(3, 0, y, w, 6, colors.track)
    draw.RoundedBox(3, 0, y, w * frac, 6, colors.fill)

    -- Voice preload progress rides along the track: playback is usable before
    -- it finishes, so this informs rather than blocks.
    local loading, total = CHRONOS.VoiceLoading or 0, CHRONOS.VoiceTotal or 0
    if loading > 0 and total > 0 then
        local done = (total - loading) / total
        draw.RoundedBox(3, 0, y + 7, w * done, 2, colors.dim)
    end

    local knobX = math.Clamp(w * frac, 7, w - 7)
    draw.RoundedBox(7, knobX - 7, h * 0.5 - 7, 14, 14, colors.knob)
end

vgui.Register("ChronosTimeline", PANEL, "DPanel")
