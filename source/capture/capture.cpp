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

void EmitHeader( std::vector<uint8_t> &out, uint16_t index, uint16_t classId, uint8_t type )
{
	Put<uint16_t>( out, index );
	Put<uint16_t>( out, classId );
	Put<uint8_t>( out, type );
}

// Emits the plan slots inside one run whose bytes moved. Reached only for runs
// that already failed a whole-run compare, so the per-slot work is paid on the
// small fraction of the entity that actually changed.
static void EmitRun( std::vector<uint8_t> &out, const PlanRun &run, const ClassPlan *plan,
	const uint8_t *base, const uint8_t *shadow, uint16_t &count )
{
	const PlanEntry *entries = &plan->entries[0];
	const uint32_t *prefix = &plan->prefix[0];
	uint16_t last = run.first + run.count;

	for ( uint16_t i = run.first; i < last; ++i )
	{
		const uint8_t *live = base + entries[i].offset;
		if ( SameBytes( live, shadow + prefix[i], entries[i].size ) )
			continue;

		Put<uint16_t>( out, i );
		PutBytes( out, live, entries[i].size );
		++count;
	}
}

// A delta is scanned against the entity itself rather than against a fresh copy
// of it. The blob the recorder keeps per edict is both the previous state and
// the thing being compared, so a tick that changes nothing reads the entity
// once and writes nothing at all -- where scraping into a scratch buffer first
// meant copying the whole blob in, comparing it, and copying it out again for
// every entity every tick, three passes to find the 2% that moved.
void EmitDelta( std::vector<uint8_t> &out, uint16_t index, EntitySlot &slot,
	const ClassPlan *plan, const uint8_t *base )
{
	EmitHeader( out, index, plan->id, REC_DELTA );

	size_t countPos = out.size( );
	Put<uint16_t>( out, 0 );
	uint16_t count = 0;

	uint8_t *shadow = &slot.blob[0];
	const PlanRun *runs = &plan->runs[0];
	size_t runCount = plan->runs.size( );

	for ( size_t i = 0; i < runCount; ++i )
	{
		const PlanRun &run = runs[i];
		const uint8_t *live = base + run.offset;
		if ( SameBytes( live, shadow + run.blobAt, run.size ) )
			continue;

		EmitRun( out, run, plan, base, shadow, count );
		memcpy( shadow + run.blobAt, live, run.size );
	}

	memcpy( &out[countPos], &count, sizeof( count ) );
}

}
