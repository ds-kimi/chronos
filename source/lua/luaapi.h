#pragma once

#include "GarrysMod/Lua/Interface.h"

namespace Chronos
{

LUA_FUNCTION_DECLARE( Lua_Start );
LUA_FUNCTION_DECLARE( Lua_Stop );
LUA_FUNCTION_DECLARE( Lua_Clear );
LUA_FUNCTION_DECLARE( Lua_IsRecording );

LUA_FUNCTION_DECLARE( Lua_Capture );
LUA_FUNCTION_DECLARE( Lua_Restore );
LUA_FUNCTION_DECLARE( Lua_GetRange );
LUA_FUNCTION_DECLARE( Lua_GetEntities );

LUA_FUNCTION_DECLARE( Lua_GetStats );
LUA_FUNCTION_DECLARE( Lua_SetMemoryCap );
LUA_FUNCTION_DECLARE( Lua_SetKeyInterval );
LUA_FUNCTION_DECLARE( Lua_GetPlan );

LUA_FUNCTION_DECLARE( Lua_SetIgnore );
LUA_FUNCTION_DECLARE( Lua_ClearIgnore );
LUA_FUNCTION_DECLARE( Lua_SetSkip );
LUA_FUNCTION_DECLARE( Lua_ClearSkip );

LUA_FUNCTION_DECLARE( Lua_BindProxy );
LUA_FUNCTION_DECLARE( Lua_ClearProxies );
LUA_FUNCTION_DECLARE( Lua_GetTransform );
LUA_FUNCTION_DECLARE( Lua_ReadProp );

LUA_FUNCTION_DECLARE( Lua_StartClipServer );
LUA_FUNCTION_DECLARE( Lua_StopClipServer );
LUA_FUNCTION_DECLARE( Lua_AddClip );
LUA_FUNCTION_DECLARE( Lua_ClearClips );
LUA_FUNCTION_DECLARE( Lua_SetClipCap );
LUA_FUNCTION_DECLARE( Lua_ClipStats );

LUA_FUNCTION_DECLARE( Lua_PlayEffects );
LUA_FUNCTION_DECLARE( Lua_ClearEffects );
LUA_FUNCTION_DECLARE( Lua_EffectCount );

LUA_FUNCTION_DECLARE( Lua_BenchEnable );
LUA_FUNCTION_DECLARE( Lua_BenchReset );
LUA_FUNCTION_DECLARE( Lua_BenchMark );
LUA_FUNCTION_DECLARE( Lua_BenchSample );
LUA_FUNCTION_DECLARE( Lua_BenchReport );
LUA_FUNCTION_DECLARE( Lua_ProcStats );

void RegisterAPI( GarrysMod::Lua::ILua *LUA );

}
