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

// A keyframe carries the whole blob plus the identity strings. Scraping into
// the slot's own shadow rather than a scratch buffer is what lets the delta
// path below it compare against the entity directly.
void EmitKey( std::vector<uint8_t> &out, uint16_t index, EntitySlot &slot,
	const ClassPlan *plan, CBaseEntity *ent )
{
	uint64_t at = BenchCycles( );
	ScrapeEntity( ent, plan, slot.blob );
	BenchAddCycles( BP_SCRAPE, at );

	EmitHeader( out, index, plan->id, REC_KEY );
	Put<uint16_t>( out, slot.classNameId );
	Put<uint16_t>( out, slot.modelNameId );

	// Identity is the pair (index, birth tick), never the index alone: the
	// engine hands a freed index to the next spawn, and a rebuild window only
	// reaches back a couple of key intervals, so the birth tick has to travel
	// in the record rather than be inferred from where a seek began.
	Put<int32_t>( out, slot.born );

	PutBytes( out, slot.blob.empty( ) ? nullptr : &slot.blob[0], slot.blob.size( ) );
}

}
