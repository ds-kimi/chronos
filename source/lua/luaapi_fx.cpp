#include "lua/luaapi.h"
#include "tempents/tempents.h"

namespace Chronos
{

LUA_FUNCTION( Lua_PlayEffects )
{
	int32_t from = ( int32_t )LUA->CheckNumber( 1 );
	int32_t to = ( int32_t )LUA->CheckNumber( 2 );
	PlayTempEnts( from, to, ( int )LUA->GetNumber( 3 ) );
	return 0;
}

LUA_FUNCTION( Lua_ClearEffects )
{
	ClearTempEnts( );
	return 0;
}

LUA_FUNCTION( Lua_EffectCount )
{
	LUA->PushNumber( ( double )g_tempEnts.size( ) );
	return 1;
}

}
