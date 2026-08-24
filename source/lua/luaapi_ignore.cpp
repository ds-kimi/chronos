#include "lua/luaapi.h"
#include "capture/recorder.h"

namespace Chronos
{

// Ignored edicts are still recorded but never written back during a seek, which
// is how a spectator keeps control of their own player while the world rewinds.
LUA_FUNCTION( Lua_SetIgnore )
{
	int index = ( int )LUA->CheckNumber( 1 );
	bool ignored = LUA->GetBool( 2 );
	if ( index < 0 || index >= kMaxEdicts )
		return 0;

	Rec( ).ignore[index] = ignored ? 1 : 0;
	return 0;
}

// Skipped edicts are never captured at all, which is what keeps the stand-ins a
// stage replay spawns out of the recording still running underneath it.
LUA_FUNCTION( Lua_SetSkip )
{
	int index = ( int )LUA->CheckNumber( 1 );
	bool skipped = LUA->GetBool( 2 );
	if ( index < 0 || index >= kMaxEdicts )
		return 0;

	Rec( ).skip[index] = skipped ? 1 : 0;
	return 0;
}

LUA_FUNCTION( Lua_ClearSkip )
{
	Recorder &rec = Rec( );
	for ( size_t i = 0; i < rec.skip.size( ); ++i )
		rec.skip[i] = 0;

	return 0;
}

LUA_FUNCTION( Lua_ClearIgnore )
{
	Recorder &rec = Rec( );
	for ( size_t i = 0; i < rec.ignore.size( ); ++i )
		rec.ignore[i] = 0;

	return 0;
}

}
