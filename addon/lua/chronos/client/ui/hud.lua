-- The panel already reports state while it is open, so the HUD strip only fills
-- in when it is not, keeping the screen clean during normal play.
hook.Add("HUDPaint", "chronos_hud", function()
    if CHRONOS.Mode == "idle" or IsValid(CHRONOS.Panel) then return end

    local label, color = CHRONOS.StatusText()
    surface.SetFont("ChronosBold")

    local width, height = surface.GetTextSize(label)
    local x = ScrW() * 0.5 - width * 0.5

    draw.RoundedBox(4, x - 12, 22, width + 24, height + 10, CHRONOS.Colors.bg)
    draw.SimpleText(label, "ChronosBold", x, 27, color)
end)
