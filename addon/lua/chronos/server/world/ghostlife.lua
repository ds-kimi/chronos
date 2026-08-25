---An edict index is not an identity: the engine hands a freed one to the next
---spawn. These two keep the ghost population honest about which occupant of an
---index each ghost stands for, and about which ticks it existed on.

---Releases ghosts whose index has since changed hands, either to a different
---recorded occupant or back to a live entity that is the original. Keeping one
---past that point is what drew a deleted prop over the top of the new prop
---that inherited its index, with the new one hidden and frozen underneath.
---@param manifest table<number, table>
function CHRONOS.DropStaleGhosts(manifest)
    for index, ghost in pairs(CHRONOS.Ghosts) do
        local info = manifest[index]
        local stale = info ~= nil and ghost.ChronosBorn ~= info.born
        if stale or (info and CHRONOS.LiveOriginal(index, info, CHRONOS.Cursor)) then
            chronos.BindProxy(index, -1)
            CHRONOS.Ghosts[index] = nil
            if IsValid(ghost) then ghost:Remove() end
        end
    end
end

---A ghost is created once and kept for the rest of the replay, drawn only on
---the ticks its original existed. Removing and respawning it per sync churns
---the edict pool, and Source does not hand an edict back the moment it is
---freed, so a drag across the timeline ran the pool dry.
---@param manifest table<number, table>
function CHRONOS.HideGhosts(manifest)
    for index, ghost in pairs(CHRONOS.Ghosts) do
        if IsValid(ghost) then ghost:SetNoDraw(manifest[index] == nil) end
    end
end
