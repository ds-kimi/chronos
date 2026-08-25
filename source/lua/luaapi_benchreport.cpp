#include "bench/counters.h"
#include "lua/luaapi.h"

namespace Chronos
{

LUA_FUNCTION( Lua_BenchReport )
{
	const char *label = LUA->CheckString( 1 );
	double tickIntervalMs = LUA->CheckNumber( 2 );

	std::string report = BuildReport( label != nullptr ? label : "bench", tickIntervalMs );
	LUA->PushString( report.c_str( ) );
	return 1;
}

// Available whether or not instrumentation is on: a memory reading is a plain
// question about the process, not something that needs a measurement window.
LUA_FUNCTION( Lua_ProcStats )
{
	ProcSnapshot now;
	SampleProcess( now );

	LUA->CreateTable( );
	LUA->PushNumber( ( double )now.workingSet );
	LUA->SetField( -2, "ws" );
	LUA->PushNumber( ( double )now.privateBytes );
	LUA->SetField( -2, "private" );
	LUA->PushNumber( ( double )now.peakWorkingSet );
	LUA->SetField( -2, "peakws" );
	LUA->PushNumber( now.userMs );
	LUA->SetField( -2, "usercpu" );
	LUA->PushNumber( now.kernelMs );
	LUA->SetField( -2, "kernelcpu" );
	LUA->PushNumber( ( double )now.sysAvail );
	LUA->SetField( -2, "sysavail" );
	LUA->PushNumber( ( double )now.sysTotal );
	LUA->SetField( -2, "systotal" );
	LUA->PushNumber( now.threads );
	LUA->SetField( -2, "threads" );
	return 1;
}

}
