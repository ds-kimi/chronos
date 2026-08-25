#include "lua/luaapi.h"
#include "props/propplan.h"
#include "capture/recorder.h"

namespace Chronos
{

LUA_FUNCTION( Lua_Capture )
{
	CaptureTick( ( float )LUA->GetNumber( 1 ) );
	return 0;
}

LUA_FUNCTION( Lua_Restore )
{
	int32_t tick = ( int32_t )LUA->CheckNumber( 1 );
	LUA->PushBool( RestoreTick( tick, LUA->GetBool( 2 ) ) );
	return 1;
}

LUA_FUNCTION( Lua_GetRange )
{
	Recorder &rec = Rec( );
	if ( rec.frames.empty( ) )
		return 0;

	LUA->PushNumber( rec.frames.front( ).tick );
	LUA->PushNumber( rec.frames.back( ).tick );
	LUA->PushNumber( ( double )rec.frames.size( ) );
	return 3;
}

// Entity manifest for a tick, so Lua can recreate props that were deleted after
// recording and remove ones that did not exist yet.
LUA_FUNCTION( Lua_GetEntities )
{
	int32_t tick = ( int32_t )LUA->CheckNumber( 1 );
	LUA->CreateTable( );
	if ( !BuildStateAtTick( tick ) )
		return 1;

	for ( int i = 0; i < kMaxEdicts; ++i )
	{
		const WorkSlot *work = WorkAt( i );
		if ( work == nullptr || !work->valid )
			continue;

		LUA->PushNumber( i );
		LUA->CreateTable( );
		LUA->PushString( StringById( work->classNameId ) );
		LUA->SetField( -2, "class" );
		LUA->PushString( StringById( work->modelNameId ) );
		LUA->SetField( -2, "model" );
		LUA->PushNumber( work->born );
		LUA->SetField( -2, "born" );
		LUA->SetTable( -3 );
	}

	return 1;
}

}
