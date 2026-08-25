#include "bench/counters.h"
#include "lua/luaapi.h"

#include <cstring>

namespace Chronos
{

// Deep timing probes every entity of every tick; it costs real budget, so it is
// opt-in and the report says which mode produced it.
LUA_FUNCTION( Lua_BenchEnable )
{
	bool on = LUA->GetBool( 1 );
	bool deep = LUA->GetBool( 2 );

	BenchEnable( on, deep );
	LUA->PushBool( g_bench.on );
	return 1;
}

LUA_FUNCTION( Lua_BenchReset )
{
	if ( g_bench.on )
		BenchReset( );

	return 0;
}

// Lets the Lua Tick hook file its own cost into the same table as the C++
// phases, so one report accounts for the whole tick instead of half of it.
LUA_FUNCTION( Lua_BenchMark )
{
	const char *name = LUA->CheckString( 1 );
	double ms = LUA->CheckNumber( 2 );
	if ( !g_bench.on || name == nullptr )
		return 0;

	for ( int i = 0; i < BP_COUNT; ++i )
	{
		if ( strcmp( name, PhaseName( i ) ) != 0 )
			continue;

		// The Lua hook body wraps the native calls, so charge Lua only what is
		// left after the capture and restore already accounted for themselves.
		if ( i == BP_LUATICK )
		{
			ms -= g_bench.nativeMs;
			g_bench.nativeMs = 0.0;
		}

		g_bench.phase[i].Add( ms > 0.0 ? ms : 0.0 );
		break;
	}

	return 0;
}

static void PushField( GarrysMod::Lua::ILua *LUA, const char *name, double value )
{
	LUA->PushNumber( value );
	LUA->SetField( -2, name );
}

// The numbers the round driver watches while a run is in flight: enough to
// abort on a server that is falling over, without rendering a whole report.
LUA_FUNCTION( Lua_BenchSample )
{
	const Sample &gap = g_bench.phase[BP_GAMEGAP];
	double mean = gap.Mean( );

	LUA->CreateTable( );
	LUA->PushBool( g_bench.on );
	LUA->SetField( -2, "on" );
	LUA->PushBool( g_bench.deep );
	LUA->SetField( -2, "deep" );
	PushField( LUA, "frames", ( double )g_bench.phase[BP_GAMEFRAME].n );
	PushField( LUA, "frame_mean", g_bench.phase[BP_GAMEFRAME].Mean( ) );
	PushField( LUA, "frame_p99", g_bench.phase[BP_GAMEFRAME].Percentile( 0.99 ) );
	PushField( LUA, "frame_max", g_bench.phase[BP_GAMEFRAME].high );
	PushField( LUA, "capture_mean", g_bench.phase[BP_CAPTURE].Mean( ) );
	PushField( LUA, "capture_p99", g_bench.phase[BP_CAPTURE].Percentile( 0.99 ) );
	PushField( LUA, "gap_mean", mean );
	PushField( LUA, "tps", mean > 0.0 ? 1000.0 / mean : 0.0 );
	PushField( LUA, "live", g_bench.counter[BC_LIVE].Mean( ) );
	PushField( LUA, "framebytes", g_bench.counter[BC_FRAMEBYTES].Mean( ) );
	return 1;
}

}
