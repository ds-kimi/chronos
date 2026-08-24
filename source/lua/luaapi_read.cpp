#include "lua/luaapi.h"
#include "props/propplan.h"
#include "capture/recorder.h"

#include "dt_common.h"
#include "mathlib/vector.h"

namespace Chronos
{

// Linear name lookup, kept honest by how rarely Lua reads individual props.
// Returns the plan slot or -1
static int SlotByName( const ClassPlan *plan, const char *name )
{
	for ( size_t i = 0; i < plan->names.size( ); ++i )
	{
		if ( plan->names[i] == name )
			return ( int )i;
	}

	return -1;
}

// Reads one named prop out of the state rebuilt by the last seek. Lets Lua drive
// stand-ins for entities it cannot restore into, players above all.
LUA_FUNCTION( Lua_ReadProp )
{
	int index = ( int )LUA->CheckNumber( 1 );
	const char *name = LUA->CheckString( 2 );
	const WorkSlot *work = WorkAt( index );
	const ClassPlan *plan = work != nullptr && work->valid ? PlanById( work->classId ) : nullptr;
	if ( plan == nullptr || work->blob.size( ) != plan->blobSize )
		return 0;

	int slot = SlotByName( plan, name );
	if ( slot < 0 )
		return 0;

	const uint8_t *at = &work->blob[plan->prefix[slot]];
	const PlanEntry &entry = plan->entries[slot];

	if ( entry.type == DPT_Vector || entry.type == DPT_VectorXY )
	{
		const float *values = reinterpret_cast<const float *>( at );
		LUA->PushVector( Vector( values[0], values[1], values[2] ) );
	}
	else if ( entry.type == DPT_Float )
	{
		LUA->PushNumber( *reinterpret_cast<const float *>( at ) );
	}
	else
	{
		LUA->PushNumber( *reinterpret_cast<const int *>( at ) );
	}

	return 1;
}

}
