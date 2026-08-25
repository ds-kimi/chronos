PROJECT_GENERATOR_VERSION = 2

newoption({
	trigger = "gmcommon",
	description = "Sets the path to the garrysmod_common (https://github.com/danielga/garrysmod_common) directory",
	value = "../garrysmod_common"
})

local gmcommon = assert(_OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON"),
	"you didn't provide a path to your garrysmod_common (https://github.com/danielga/garrysmod_common) directory")
include(gmcommon)

CreateWorkspace({name = "chronos", abi_compatible = false, path = "projects/" .. os.target() .. "/" .. _ACTION})
	CreateProject({serverside = true, source_path = "source", manual_files = true})
		files({
			"source/**.h",
			"source/**.hpp",
			"source/**.cpp",
		})
		warnings("Default")
		IncludeSDKCommon()
		IncludeSDKTier0()
		IncludeSDKTier1()
		IncludeSteamAPI()
		IncludeHelpersExtended()
		IncludeDetouring()
		IncludeScanning()
		filter("system:windows")
			links({"psapi", "ws2_32"})
		filter("system:linux")
			links({"pthread"})
		filter({})