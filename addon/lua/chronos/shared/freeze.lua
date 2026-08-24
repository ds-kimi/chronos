---StartCommand runs on the client too, inside prediction. Clearing the command
---here stops the client predicting movement the server is about to overwrite,
---which is what made frozen players slide left and right.
hook.Add("StartCommand", "chronos_freeze", function(ply, cmd)
    if not ply:GetNWBool("ChronosFrozen", false) then return end

    cmd:ClearMovement()
    cmd:ClearButtons()
end)
