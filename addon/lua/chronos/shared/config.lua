---Defaults applied on load. Anything set here can still be overridden at
---runtime with the matching chronos command, and the convars persist, so a
---value changed in game survives a restart and wins over these.
CHRONOS.Config = {
    -- Address clients fetch voice clips from. Must be reachable BY THE CLIENT,
    -- so a LAN address only works for LAN players and a public server needs its
    -- public address with the port forwarded.
    voicehost = "192.168.1.53",

    -- TCP port the module serves clips on. Zero disables voice capture.
    voiceport = 27020,

    -- Prints every voice capture and why any were dropped.
    voicedebug = true,

    -- Snapshot ring size in megabytes.
    memcap = 512,

    -- Keyframe stagger, and therefore the seek window.
    keyinterval = 64,

    -- Voice clip budget in megabytes, evicted oldest first.
    voicecap = 256
}
