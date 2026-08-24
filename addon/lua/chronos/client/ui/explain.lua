---One sentence, always on screen, answering the only question this panel really
---has to answer: who is affected by what is happening right now.
---@return string
---@return Color
function CHRONOS.ExplainText()
    local colors = CHRONOS.Colors

    if CHRONOS.Staged then
        return "Only you see this. Everyone else is playing normally.", colors.rec
    end

    if CHRONOS.Mode == "replay" then
        return "The server is held at this tick. Everyone is watching it with you.", colors.fill
    end

    if CHRONOS.Mode == "recording" then
        return "Recording. Stop when you have what you need, and the server holds there.", colors.dim
    end

    if CHRONOS.Frames > 0 then
        if CHRONOS.View == "experimental" then
            return "Recording stopped. Press Review privately to watch it back.", colors.fill
        end

        return "Recording stopped and the server has resumed.", colors.dim
    end

    return "Nothing captured yet. Press Start recording.", colors.dim
end

---@param frame Panel
function CHRONOS.BuildExplain(frame)
    local bar = CHRONOS.Row(frame, 20)

    bar.Paint = function(_, _, h)
        local text, color = CHRONOS.ExplainText()
        draw.SimpleText(text, "ChronosUI", 0, h * 0.5, color, 0, TEXT_ALIGN_CENTER)
    end
end

---@param frame Panel
function CHRONOS.BuildStats(frame)
    local bar = CHRONOS.Row(frame, 16)

    bar.Paint = function(_, w, h)
        local colors = CHRONOS.Colors
        local text = string.format("%d frames   %.1f / %.0f MB   %d sounds   %d effects",
            CHRONOS.Frames, CHRONOS.Megabytes, CHRONOS.MegabyteCap,
            CHRONOS.EventCount, CHRONOS.EffectCount)

        draw.SimpleText(text, "ChronosLabel", 0, h * 0.5, colors.dim, 0, TEXT_ALIGN_CENTER)
        draw.SimpleText(CHRONOS.CursorOn and "chronos_ui frees the mouse" or "",
            "ChronosLabel", w, h * 0.5, colors.dim, TEXT_ALIGN_RIGHT, TEXT_ALIGN_CENTER)
    end
end
