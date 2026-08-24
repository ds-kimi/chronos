---Sounds are replayed to the viewers alone, so a firefight from ten minutes ago
---is not heard by the players standing in the live world right now. The client
---plays them locally because the engine sound interface has no per-client path.
---@param event table
---@param viewers Player[]
local function stageSound(event, viewers)
    if not event.name or not event.pos then return end

    net.Start("chronos_stagesnd")
    net.WriteString(event.name)
    net.WriteVector(event.pos)
    net.WriteUInt(math.Clamp(math.floor(event.level or 75), 0, 255), 8)
    net.WriteUInt(math.Clamp(math.floor(event.pitch or 100), 0, 255), 8)
    net.WriteFloat(math.Clamp(event.volume or 1, 0, 1))
    net.Send(viewers)
end

---@param from number
---@param to number
function CHRONOS.PlayStageEvents(from, to)
    if to <= from then return end

    local viewers = CHRONOS.StageViewers()
    if #viewers == 0 then return end

    -- Effects and sounds are bursty: a big jump would dump a whole firefight in
    -- one frame, so they stay windowed. Voice is one discrete utterance filed
    -- hundreds of frames behind, so it plays on any jump.
    local burst = to - from <= 32
    for i = 1, burst and #viewers or 0 do
        chronos.PlayEffects(from, to, viewers[i]:EntIndex())
    end

    for tick = from + 1, to do
        local bucket = CHRONOS.Events[tick]
        for i = 1, bucket and #bucket or 0 do
            local event = bucket[i]
            if event.kind == "voice" then
                CHRONOS.EmitVoice(event)
            elseif burst then
                stageSound(event, viewers)
            end
        end
    end
end

---Puppets carry no recorded state of their own, so they are posed by hand from
---the tick the module just rebuilt.
function CHRONOS.UpdateStagePuppets()
    local stage = CHRONOS.Stage

    for index, puppet in pairs(stage.Puppets) do
        local pos, ang = chronos.GetTransform(index)
        if pos and IsValid(puppet) then
            -- A stand-in whose original was parented at this tick reports the
            -- parent-relative zero, which would park it on the map origin.
            puppet:SetNoDraw(pos:IsZero() or not stage.Present[index])
            puppet:SetPos(pos)
            puppet:SetAngles(ang)

            -- A recorded sequence belongs to the original model's animation
            -- list; pushing an index past the end of a stand-in's own list is
            -- not a Lua error but a crash, so it is clamped to what it has.
            local sequence = math.floor(chronos.ReadProp(index, "m_nSequence") or 0)
            puppet:SetSequence(math.Clamp(sequence, 0, puppet:GetSequenceCount() - 1))
            puppet:SetCycle(math.Clamp(chronos.ReadProp(index, "m_flCycle") or 0, 0, 1))
            puppet:SetSkin(math.Clamp(math.floor(chronos.ReadProp(index, "m_nSkin") or 0),
                0, math.max(puppet:SkinCount() - 1, 0)))
        end
    end
end

---Drives the stage one tick. Unlike a scrub, nothing here touches the live
---world: the restore is proxy-only, so only bound stand-ins are written to.
function CHRONOS.StageTick()
    local first, last = chronos.GetRange()
    if not first then return end

    -- The ring keeps evicting while the stage watches, so a cursor left behind
    -- by recording has to be dragged forward rather than pointing at nothing.
    CHRONOS.Cursor = math.Clamp(CHRONOS.Cursor, first, last)

    local previous = CHRONOS.Cursor
    CHRONOS.AdvanceCursor()

    if CHRONOS.Playing then
        CHRONOS.PlayStageEvents(previous, CHRONOS.Cursor)
    end

    -- Throttled on the clock rather than on cursor distance: a timeline drag
    -- moves the cursor every frame, and a manifest rebuild per frame is what
    -- turned scrubbing into a crash.
    local stage = CHRONOS.Stage
    if CurTime() >= stage.NextSync then
        stage.NextSync = CurTime() + 0.25
        CHRONOS.SyncStageClones(CHRONOS.Cursor)
        CHRONOS.SyncStageTransmit()
    end

    chronos.Restore(CHRONOS.Cursor, true)
    CHRONOS.UpdateStagePuppets()
end
