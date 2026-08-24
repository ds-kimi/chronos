#include "lua/luaapi.h"
#include "props/propplan.h"
#include "capture/recorder.h"

namespace Chronos
{

LUA_FUNCTION( Lua_GetStats )
{
	Recorder &rec = Rec( );
	LUA->CreateTable( );
	LUA->PushNumber( ( double )rec.bytes );
	LUA->SetField( -2, "bytes" );
	LUA->PushNumber( ( double )rec.byteCap );
	LUA->SetField( -2, "cap" );
	LUA->PushNumber( ( double )rec.frames.size( ) );
	LUA->SetField( -2, "frames" );
	LUA->PushNumber( rec.keyInterval );
	LUA->SetField( -2, "keyinterval" );
	LUA->PushBool( rec.recording );
	LUA->SetField( -2, "recording" );
	return 1;
}

LUA_FUNCTION( Lua_SetMemoryCap )
{
	double megabytes = LUA->CheckNumber( 1 );
	if ( megabytes < 8.0 )
		megabytes = 8.0;

	Rec( ).byteCap = ( size_t )( megabytes * 1024.0 * 1024.0 );
	return 0;
}

LUA_FUNCTION( Lua_SetKeyInterval )
{
	int interval = ( int )LUA->CheckNumber( 1 );
	if ( interval < 1 )
		interval = 1;

	Rec( ).keyInterval = interval;
	return 0;
}

// Dumps the resolved capture plan of one entity, for verifying that a class's
// members were picked up before trusting a recording of it.
LUA_FUNCTION( Lua_GetPlan )
{
	int index = ( int )LUA->CheckNumber( 1 );
	const WorkSlot *work = WorkAt( index );
	const ClassPlan *plan = work != nullptr ? PlanById( work->classId ) : nullptr;
	if ( plan == nullptr )
		return 0;

	LUA->CreateTable( );
	for ( size_t i = 0; i < plan->names.size( ); ++i )
	{
		LUA->PushNumber( ( double )( i + 1 ) );
		LUA->PushString( plan->names[i].c_str( ) );
		LUA->SetTable( -3 );
	}

	return 1;
}

}
