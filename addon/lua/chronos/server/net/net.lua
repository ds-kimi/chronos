CreateConVar("chronos_adminonly", "1", FCVAR_ARCHIVE, "Restrict chronos commands to admins")

---Silent denial made every failure look like a dead button, so a refusal always
---says so on the caller's console.
---@param ply Player|nil
---@return boolean
local function allowed(ply)
    if not IsValid(ply) then return true end
    if GetConVar("chronos_adminonly"):GetInt() == 0 then return true end
    if ply:IsAdmin() or ply:IsSuperAdmin() then return true end

    ply:PrintMessage(HUD_PRINTCONSOLE, "[chronos] admin only (see chronos_adminonly)")
    return false
end

---@param ply Player|nil
---@param name string
---@param value string
local function dispatch(ply, name, value)
    local action = CHRONOS.Actions[string.lower(name or "")]
    if not action then
        if IsValid(ply) then
            ply:PrintMessage(HUD_PRINTCONSOLE, "[chronos] unknown command: " .. tostring(name))
        else
            print("[chronos] unknown command: " .. tostring(name))
        end
        return
    end

    if allowed(ply) then
        action(ply, { value })
    end
end

concommand.Add("chronos", function(ply, _, args)
    dispatch(ply, args[1] or "help", args[2])
end)

-- The scrubber panel talks over net rather than concommand so a drag can push
-- one message per tick without spamming the client's command buffer.
net.Receive("chronos_ctl", function(_, ply)
    local name = net.ReadString()
    local value = net.ReadFloat()
    if #name > 32 then return end

    dispatch(ply, name, tostring(value))
end)
