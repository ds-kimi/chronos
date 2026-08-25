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

// One memcpy per contiguous stretch rather than one per member. Members are
// sorted by offset when the plan is built, so most classes collapse into a
// handful of runs instead of a hundred four-byte copies.
void ScrapeEntity( CBaseEntity *ent, const ClassPlan *plan, std::vector<uint8_t> &dst )
{
	dst.resize( plan->blobSize );
	const uint8_t *base = reinterpret_cast<const uint8_t *>( ent );

	for ( size_t i = 0; i < plan->runs.size( ); ++i )
	{
		const PlanRun &run = plan->runs[i];
		memcpy( &dst[run.blobAt], base + run.offset, run.size );
	}
}

// Emits the plan slots inside one run whose bytes moved. Reached only for runs
// that already failed a whole-run compare, so the per-slot work is paid on the
// small fraction of the entity that actually changed.
static void EmitRun( std::vector<uint8_t> &out, const PlanRun &run, const ClassPlan *plan,
	const std::vector<uint8_t> &cur, const std::vector<uint8_t> &prev, uint16_t &count )
{
	for ( uint16_t i = run.first; i < run.first + run.count; ++i )
	{
		size_t at = plan->prefix[i];
		uint16_t size = plan->entries[i].size;
		if ( memcmp( &cur[at], &prev[at], size ) == 0 )
			continue;

		Put<uint16_t>( out, i );
		PutBytes( out, &cur[at], size );
		++count;
	}
}

// A keyframe carries the whole blob plus identity strings; a delta carries only
// the plan slots whose bytes moved since the previous capture of this edict.
void EmitRecord( std::vector<uint8_t> &out, uint16_t index, const EntitySlot &slot,
	const ClassPlan *plan, const std::vector<uint8_t> &cur, bool key )
{
	Put<uint16_t>( out, index );
	Put<uint16_t>( out, plan->id );
	Put<uint8_t>( out, key ? REC_KEY : REC_DELTA );

	if ( key )
	{
		Put<uint16_t>( out, slot.classNameId );
		Put<uint16_t>( out, slot.modelNameId );

		// Identity is the pair (index, birth tick), never the index alone: the
		// engine hands a freed index to the next spawn, and a rebuild window
		// only reaches back a couple of key intervals, so the birth tick has to
		// travel in the record rather than be inferred from where a seek began.
		Put<int32_t>( out, slot.born );
		PutBytes( out, cur.empty( ) ? nullptr : &cur[0], cur.size( ) );
		return;
	}

	size_t countPos = out.size( );
	Put<uint16_t>( out, 0 );
	uint16_t count = 0;

	// Around 98% of scanned bytes are unchanged, so one compare per run
	// discards most of the entity before any per-slot work happens. The wire
	// format is untouched: what gets emitted is still per plan slot.
	for ( size_t i = 0; i < plan->runs.size( ); ++i )
	{
		const PlanRun &run = plan->runs[i];
		if ( memcmp( &cur[run.blobAt], &slot.blob[run.blobAt], run.size ) == 0 )
			continue;

		EmitRun( out, run, plan, cur, slot.blob, count );
	}

	memcpy( &out[countPos], &count, sizeof( count ) );
}

}
