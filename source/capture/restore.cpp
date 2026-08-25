#include "bench/counters.h"
#include "props/propplan.h"
#include "capture/recorder.h"
#include "capture/workstate.h"

#include "edict.h"
#include "eiface.h"
#include "iservernetworkable.h"
#include "server_class.h"

#include <cstring>

namespace Chronos
{

// An entity getting its own bytes back takes every run in one memcpy; a stand-in
// goes member by member, because the whole point of the mask is that most
// members must not be written onto an entity standing in for another.
static void PushBlob( uint8_t *base, const WorkSlot &work, const ClassPlan *plan,
	const uint8_t *skip )
{
	if ( skip == nullptr )
	{
		for ( size_t i = 0; i < plan->runs.size( ); ++i )
		{
			const PlanRun &run = plan->runs[i];
			memcpy( base + run.offset, &work.blob[run.blobAt], run.size );
		}

		return;
	}

	for ( size_t i = 0; i < plan->entries.size( ); ++i )
	{
		if ( skip[i] == 0 )
			memcpy( base + plan->entries[i].offset, &work.blob[plan->prefix[i]],
				plan->entries[i].size );
	}
}

// Writes the reconstructed blob straight back over the entity's members, then
// dirties the edict so the engine reships the props to every client.
static void PushEntity( int target, const WorkSlot &work, bool proxied )
{
	edict_t *edict = g_engine->PEntityOfEntIndex( target );
	if ( edict == nullptr || edict->IsFree( ) )
		return;

	IServerUnknown *unknown = edict->GetUnknown( );
	CBaseEntity *ent = unknown != nullptr ? unknown->GetBaseEntity( ) : nullptr;
	const ClassPlan *plan = ent != nullptr ? PushPlan( target, unknown, ent ) : nullptr;
	if ( plan == nullptr || plan->entries.empty( ) || plan->id != work.classId ||
		work.blob.size( ) != plan->blobSize )
		return;

	uint8_t *base = reinterpret_cast<uint8_t *>( ent );
	PushBlob( base, work, plan, proxied ? &ProxyMask( plan )[0] : nullptr );

	// StateChanged() lives in the server binary; setting the flags directly does
	// the same job and makes the engine reship every prop of this edict.
	edict->m_fStateFlags |= ( FL_EDICT_CHANGED | FL_FULL_EDICT_CHANGED );
}

bool RestoreTick( int32_t tick, bool proxyOnly )
{
	if ( g_engine == nullptr || !BuildStateAtTick( tick ) )
		return false;

	// The rebuild above is timed on its own, so this span is the push cost only.
	int64_t began = g_bench.on ? QpcNow( ) : 0;
	Recorder &rec = Rec( );
	int scan = WorkHighWater( ) + 1;

	for ( int i = 0; i < scan; ++i )
	{
		const WorkSlot &work = g_work[i];
		if ( !work.valid || rec.ignore[i] != 0 )
			continue;

		// A bound proxy redirects a recorded edict onto a different live one,
		// which is how Lua brings back entities that were deleted since.
		// A stage replay owns nothing but its own clones: writing into the
		// unbound originals is what froze the live world for everybody.
		uint16_t target = rec.proxy[i];
		if ( target == 0xFFFF && proxyOnly )
			continue;

		PushEntity( target == 0xFFFF ? i : target, work, target != 0xFFFF );
	}

	BenchCharge( BP_RESTORE, began );
	return true;
}

}
