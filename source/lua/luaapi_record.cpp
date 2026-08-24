#include "lua/luaapi.h"
#include "capture/recorder.h"

namespace Chronos
{

LUA_FUNCTION( Lua_Start )
{
	StartRecording( );
	return 0;
}

LUA_FUNCTION( Lua_Stop )
{
	StopRecording( );
	return 0;
}

LUA_FUNCTION( Lua_Clear )
{
	ClearRecording( );
	return 0;
}

LUA_FUNCTION( Lua_IsRecording )
{
	LUA->PushBool( Rec( ).recording );
	return 1;
}

}
