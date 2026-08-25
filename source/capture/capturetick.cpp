#include "bench/counters.h"
#include "core/bytes.h"
#include "props/propplan.h"
#include "capture/recorder.h"

#include "edict.h"
#include "eiface.h"
#include "iserverentity.h"
#include "iservernetworkable.h"
#include "server_class.h"

namespace Chronos
{

// Written once when an edict stops existing so a seek can hide or delete the
// entity instead of leaving it frozen at its last recorded transform.
static void EmitGone( std::vector<uint8_t> &out, uint16_t index, EntitySlot &slot )
{
	if ( !slot.live )
		return;

	Put<uint16_t>( out, index );
	Put<uint16_t>( out, 0 );
	Put<uint8_t>( out, REC_GONE );
	slot = EntitySlot( );
	BenchCountGone( );
}

static void RefreshIdentity( edict_t *edict, EntitySlot &slot )
{
	slot.classNameId = InternString( edict->GetClassName( ) );

	IServerEntity *serverEnt = edict->GetIServerEntity( );
	slot.modelNameId = InternString( serverEnt != nullptr ? STRING( serverEnt->GetModelName( ) ) : "" );
}

// Split out of CaptureEntity so the scrape and emit probes have their own scope
// and the resolve path above them stays out of the deep timings.
static void CaptureLive( std::vector<uint8_t> &out, int index, int32_t tick,
	edict_t *edict, CBaseEntity *ent, const ClassPlan *plan )
{
	Recorder &rec = Rec( );
	EntitySlot &slot = rec.slots[index];
	static std::vector<uint8_t> current;

	uint64_t at = BenchCycles( );
	ScrapeEntity( ent, plan, current );
	BenchAddCycles( BP_SCRAPE, at );

	// An index that was empty last tick is holding somebody new as of this one.
	if ( !slot.live )
		slot.born = tick;

	bool key = slot.needKey || slot.classId != plan->id || slot.blob.size( ) != current.size( ) ||
		( ( tick + index ) % rec.keyInterval ) == 0;
	if ( key )
		RefreshIdentity( edict, slot );

	size_t before = out.size( );
	at = BenchCycles( );
	EmitRecord( out, ( uint16_t )index, slot, plan, current, key );
	BenchAddCycles( BP_EMIT, at );

	BenchCountEntity( key, out.size( ) - before, plan );
	// Not a swap: `current` is shared by every entity, so swapping hands it a
	// buffer sized for the previous class and the next resize reallocates.
	// Measured 6% slower at 500+ entities than copying into existing capacity.
	slot.blob = current;
	slot.classId = plan->id;
	slot.live = true;
	slot.needKey = false;
}

static void CaptureEntity( std::vector<uint8_t> &out, int index, int32_t tick )
{
	Recorder &rec = Rec( );
	EntitySlot &slot = rec.slots[index];
	edict_t *edict = g_engine->PEntityOfEntIndex( index );
	IServerUnknown *unknown = edict != nullptr && !edict->IsFree( ) ? edict->GetUnknown( ) : nullptr;
	CBaseEntity *ent = unknown != nullptr ? unknown->GetBaseEntity( ) : nullptr;

	uint64_t at = BenchCycles( );
	const ClassPlan *plan = nullptr;
	if ( ent != nullptr )
	{
		ServerClass *sc = unknown->GetNetworkable( )->GetServerClass( );
		plan = sc == slot.planClass && slot.plan != nullptr ? slot.plan : GetPlan( sc, ent );
		slot.plan = plan;
		slot.planClass = sc;
	}

	BenchAddCycles( BP_PLAN, at );

	// Marked from the edict, not from a resolved plan: an edict the scan cannot
	// plan for still proves indices reach this high, and the bound has to cover
	// it or everything above stays invisible for the rest of the recording.
	if ( edict != nullptr && !edict->IsFree( ) && index > rec.highWater )
		rec.highWater = index;

	if ( plan == nullptr || plan->blobSize == 0 )
	{
		EmitGone( out, ( uint16_t )index, slot );
		return;
	}

	CaptureLive( out, index, tick, edict, ent, plan );
}

// Ticks are counted here rather than read from CGlobalVars: that struct's
// layout differs between this SDK and the shipped engine, and reading the wrong
// field froze the counter at a constant.
void CaptureTick( float curTime )
{
	Recorder &rec = Rec( );
	if ( !rec.recording || g_engine == nullptr )
		return;

	int32_t tick = rec.lastTick + 1;
	rec.lastTick = tick;
	std::vector<uint8_t> out = AcquireBuffer( );
	int64_t began = BenchTickBegin( );

	// GetEntityCount is a count of used slots, not the highest index in use:
	// the engine parks a freed edict before reusing it, so a spawn after a
	// delete lands above the count. Using it as a bound left those entities
	// uncaptured, and with no REC_GONE for them either, a restore wrote one
	// entity's bytes over whatever later took its index.
	int scan = rec.highWater + 1;
	int used = g_engine->GetEntityCount( ) + 1;
	if ( used > scan )
		scan = used;

	// The high-water mark can only ever learn about indices the bound already
	// covers, so a full sweep runs periodically to find the ones above it.
	if ( ( tick % kSweepInterval ) == 0 )
		scan = kMaxEdicts;

	if ( scan > kMaxEdicts )
		scan = kMaxEdicts;

	// Stage clones are replay furniture standing in the live world, so
	// recording them would file a replay back into its own recording.
	for ( int i = 0; i < scan; ++i )
	{
		if ( rec.skip[i] != 0 )
			EmitGone( out, ( uint16_t )i, rec.slots[i] );
		else
			CaptureEntity( out, i, tick );
	}

	Put<uint16_t>( out, kEndOfFrame );
	BenchTickEnd( began, out.size( ) );
	PushFrame( tick, curTime, out );
}

}
