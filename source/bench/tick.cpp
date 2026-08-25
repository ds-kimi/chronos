#include "bench/counters.h"
#include "core/chronos.h"

namespace Chronos
{

int64_t BenchTickBegin( )
{
	if ( !g_bench.on )
		return 0;

	for ( int i = BP_SCRAPE; i <= BP_PLAN; ++i )
		g_bench.cycles[i] = 0;
	for ( int i = 0; i < BC_COUNT; ++i )
		g_bench.volume[i] = 0;

	return QpcNow( );
}

// Deep phases are summed in cycles across the 8192-edict loop and converted
// once here, so the conversion cost is paid per tick rather than per entity.
static void FlushDeep( )
{
	if ( g_bench.cyclesPerMs <= 0.0 )
		return;

	for ( int i = BP_SCRAPE; i <= BP_PLAN; ++i )
		g_bench.phase[i].Add( ( double )g_bench.cycles[i] / g_bench.cyclesPerMs );
}

void BenchTickEnd( int64_t began, size_t frameBytes )
{
	if ( !g_bench.on || began == 0 )
		return;

	double ms = QpcToMs( QpcNow( ) - began );
	g_bench.phase[BP_CAPTURE].Add( ms );
	g_bench.nativeMs += ms;
	if ( g_bench.deep )
		FlushDeep( );

	g_bench.volume[BC_FRAMEBYTES] = frameBytes;
	for ( int i = 0; i < BC_COUNT; ++i )
		g_bench.counter[i].Add( ( double )g_bench.volume[i] );
}

// Emitted bytes against blob size is the delta ratio: how much of each entity
// actually moved, which is what decides whether the ring fills in a minute.
void BenchCountEntity( bool key, size_t wrote, const ClassPlan *plan )
{
	if ( !g_bench.on )
		return;

	++g_bench.volume[BC_LIVE];
	++g_bench.volume[key ? BC_KEYREC : BC_DELTAREC];
	g_bench.volume[BC_EMITBYTES] += wrote;
	g_bench.volume[BC_BLOBBYTES] += plan->blobSize;

	// Entries against runs says how far coalescing got: the closer runs are to
	// one, the fewer memcpy and memcmp calls the scan makes per entity.
	g_bench.volume[BC_ENTRIES] += plan->entries.size( );
	g_bench.volume[BC_RUNS] += plan->runs.size( );
}

void BenchCountGone( )
{
	if ( g_bench.on )
		++g_bench.volume[BC_GONEREC];
}

}
