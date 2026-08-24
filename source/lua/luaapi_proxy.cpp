#include "lua/luaapi.h"
#include "props/propplan.h"
#include "capture/recorder.h"

#include "mathlib/vector.h"

namespace Chronos
{

// Finds a plan slot by prop name once, then remembers the answer on the plan.
static int ResolveSlot( const ClassPlan *plan, const char *name, int &cache )
{
	if ( cache != -2 )
		return cache;

	cache = -1;
	for ( size_t i = 0; i < plan->names.size( ); ++i )
	{
		if ( plan->names[i] == name )
		{
			cache = ( int )i;
			break;
		}
	}

	return cache;
}

// Redirects one recorded edict onto a different live edict. The target is also
// ignored, so a spawned stand-in never fights its own recorded history.
LUA_FUNCTION( Lua_BindProxy )
{
	int recorded = ( int )LUA->CheckNumber( 1 );
	int live = ( int )LUA->CheckNumber( 2 );
	if ( recorded < 0 || recorded >= kMaxEdicts || live >= kMaxEdicts )
		return 0;

	// A negative target drops the binding, which a stage replay needs when a
	// stand-in is removed: the edict index is reused by the live world.
	if ( live < 0 )
	{
		Rec( ).proxy[recorded] = 0xFFFF;
		return 0;
	}

	Rec( ).proxy[recorded] = ( uint16_t )live;
	Rec( ).ignore[live] = 1;
	return 0;
}

LUA_FUNCTION( Lua_ClearProxies )
{
	Recorder &rec = Rec( );
	for ( size_t i = 0; i < rec.proxy.size( ); ++i )
		rec.proxy[i] = 0xFFFF;

	return 0;
}

// Position and angles of a recorded entity at the tick last built, so Lua can
// drive ghosts or cameras without owning a matching live entity.
LUA_FUNCTION( Lua_GetTransform )
{
	int index = ( int )LUA->CheckNumber( 1 );
	const WorkSlot *work = WorkAt( index );
	const ClassPlan *plan = work != nullptr && work->valid ? PlanById( work->classId ) : nullptr;
	if ( plan == nullptr || work->blob.size( ) != plan->blobSize )
		return 0;

	int origin = ResolveSlot( plan, "m_vecOrigin", plan->originSlot );
	int angles = ResolveSlot( plan, "m_angRotation", plan->anglesSlot );
	if ( origin < 0 )
		return 0;

	const float *pos = ( const float * )&work->blob[plan->prefix[origin]];
	LUA->PushVector( Vector( pos[0], pos[1], pos[2] ) );

	const float *ang = angles >= 0 ? ( const float * )&work->blob[plan->prefix[angles]] : nullptr;
	LUA->PushAngle( ang != nullptr ? QAngle( ang[0], ang[1], ang[2] ) : QAngle( 0, 0, 0 ) );
	return 2;
}

}
