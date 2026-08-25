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

// Split out of CaptureEntity so the emit probe has its own scope and the
// resolve path above it stays out of the deep timings.
static void CaptureLive( Recorder &rec, std::vector<uint8_t> &out, int index, int32_t tick,
	edict_t *edict, CBaseEntity *ent, const ClassPlan *plan )
{
	EntitySlot &slot = rec.slots[index];

	// An index that was empty last tick is holding somebody new as of this one.
	if ( !slot.live )
		slot.born = tick;

	bool key = slot.needKey || slot.classId != plan->id ||
		slot.blob.size( ) != plan->blobSize || KeyDue( rec, tick, index );
	if ( key )
		RefreshIdentity( edict, slot );

	size_t before = out.size( );
	uint64_t at = BenchCycles( );
	if ( key )
		EmitKey( out, ( uint16_t )index, slot, plan, ent );
	else
		EmitDelta( out, ( uint16_t )index, slot, plan, reinterpret_cast<const uint8_t *>( ent ) );

	BenchAddCycles( BP_EMIT, at );

	// Branched here rather than inside the counter: the call is per live entity
	// per tick, and off is the state a server actually runs in.
	if ( g_bench.on )
		BenchCountEntity( key, out.size( ) - before, plan );

	slot.classId = plan->id;
	slot.live = true;
	slot.needKey = false;
}

static void CaptureEntity( Recorder &rec, std::vector<uint8_t> &out, int index, int32_t tick )
{
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
		return EmitGone( out, ( uint16_t )index, slot );

	CaptureLive( rec, out, index, tick, edict, ent, plan );
}

// How far up the edict array this tick has to look. GetEntityCount is a count
// of used slots, not the highest index in use: the engine parks a freed edict
// before reusing it, so a spawn after a delete lands above the count. Using it
// alone as a bound left those entities uncaptured, and with no REC_GONE for
// them either, a restore wrote one entity's bytes over whatever took its index.
static int ScanBound( const Recorder &rec, int32_t tick )
{
	// The high-water mark can only ever learn about indices the bound already
	// covers, so a full sweep runs periodically to find the ones above it.
	if ( ( tick % kSweepInterval ) == 0 )
		return kMaxEdicts;

	int scan = rec.highWater + 1;
	int used = g_engine->GetEntityCount( ) + 1;
	if ( used > scan )
		scan = used;

	return scan > kMaxEdicts ? kMaxEdicts : scan;
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
	int scan = ScanBound( rec, tick );

	// Stage clones are replay furniture standing in the live world, so
	// recording them would file a replay back into its own recording.
	for ( int i = 0; i < scan; ++i )
	{
		if ( rec.skip[i] != 0 )
			EmitGone( out, ( uint16_t )i, rec.slots[i] );
		else
			CaptureEntity( rec, out, i, tick );
	}

	Put<uint16_t>( out, kEndOfFrame );
	BenchTickEnd( began, out.size( ) );
	PushFrame( tick, curTime, out );
}

}
