#include "lua/luaapi.h"

namespace Chronos
{

static void Bind( GarrysMod::Lua::ILua *LUA, const char *name, GarrysMod::Lua::CFunc fn )
{
	LUA->PushString( name );
	LUA->PushCFunction( fn );
	LUA->SetTable( -3 );
}

void RegisterAPI( GarrysMod::Lua::ILua *LUA )
{
	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB );
	LUA->CreateTable( );

	Bind( LUA, "Start", Lua_Start );
	Bind( LUA, "Stop", Lua_Stop );
	Bind( LUA, "Clear", Lua_Clear );
	Bind( LUA, "IsRecording", Lua_IsRecording );
	Bind( LUA, "Capture", Lua_Capture );
	Bind( LUA, "Restore", Lua_Restore );
	Bind( LUA, "GetRange", Lua_GetRange );
	Bind( LUA, "GetEntities", Lua_GetEntities );
	Bind( LUA, "GetStats", Lua_GetStats );
	Bind( LUA, "SetMemoryCap", Lua_SetMemoryCap );
	Bind( LUA, "SetKeyInterval", Lua_SetKeyInterval );
	Bind( LUA, "GetPlan", Lua_GetPlan );
	Bind( LUA, "SetIgnore", Lua_SetIgnore );
	Bind( LUA, "ClearIgnore", Lua_ClearIgnore );
	Bind( LUA, "SetSkip", Lua_SetSkip );
	Bind( LUA, "ClearSkip", Lua_ClearSkip );
	Bind( LUA, "BindProxy", Lua_BindProxy );
	Bind( LUA, "ClearProxies", Lua_ClearProxies );
	Bind( LUA, "GetTransform", Lua_GetTransform );
	Bind( LUA, "ReadProp", Lua_ReadProp );
	Bind( LUA, "StartClipServer", Lua_StartClipServer );
	Bind( LUA, "StopClipServer", Lua_StopClipServer );
	Bind( LUA, "AddClip", Lua_AddClip );
	Bind( LUA, "ClearClips", Lua_ClearClips );
	Bind( LUA, "SetClipCap", Lua_SetClipCap );
	Bind( LUA, "ClipStats", Lua_ClipStats );
	Bind( LUA, "PlayEffects", Lua_PlayEffects );
	Bind( LUA, "ClearEffects", Lua_ClearEffects );
	Bind( LUA, "EffectCount", Lua_EffectCount );

	LUA->SetField( -2, "chronos" );
	LUA->Pop( );
}

}
