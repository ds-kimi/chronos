#include "bench/counters.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#if defined( _WIN32 )
	#define chronos_vsnprintf _vsnprintf
	#define chronos_snprintf _snprintf
#else
	#define chronos_vsnprintf vsnprintf
	#define chronos_snprintf snprintf
#endif

namespace Chronos
{

void Fmt( std::string &out, const char *format, ... )
{
	char line[512];
	va_list args;
	va_start( args, format );
	int wrote = chronos_vsnprintf( line, sizeof( line ) - 1, format, args );
	va_end( args );

	if ( wrote < 0 )
		wrote = ( int )sizeof( line ) - 1;

	line[wrote] = 0;
	out += line;
}

// `share` is this phase's total against the total time the engine spent inside
// server frames, which is the only denominator that means anything: wall time
// includes the sleep srcds does to hold tickrate. The gap row is wall time by
// definition, so it has no share and says so rather than printing 1275%.
static void AppendPhaseRow( std::string &out, const char *label, const Sample &sample,
	double frameTotal, bool shareable )
{
	if ( sample.n == 0 )
		return;

	char share[16];
	if ( shareable && frameTotal > 0.0 )
		chronos_snprintf( share, sizeof( share ) - 1, "%.1f", sample.sum / frameTotal * 100.0 );
	else
		strncpy( share, "-", sizeof( share ) - 1 );

	share[sizeof( share ) - 1] = 0;
	Fmt( out, "%-18s %7llu %9.3f %8.3f %8.3f %8.3f %9.3f %10.1f %7s\n",
		label, ( unsigned long long )sample.n, sample.Mean( ),
		sample.Percentile( 0.50 ), sample.Percentile( 0.95 ), sample.Percentile( 0.99 ),
		sample.high, sample.sum, share );
}

void AppendPhases( std::string &out, double frameTotal )
{
	Fmt( out, "%-18s %7s %9s %8s %8s %8s %9s %10s %7s\n",
		"phase", "n", "mean", "p50", "p95", "p99", "max", "total", "%frame" );

	for ( int i = 0; i < BP_COUNT; ++i )
	{
		// The deep sub-phases are parts of capture.tick, not siblings of it.
		bool nested = i >= BP_SCRAPE && i <= BP_PLAN;
		std::string label = nested ? std::string( "  " ) + PhaseName( i ) : PhaseName( i );
		AppendPhaseRow( out, label.c_str( ), g_bench.phase[i], frameTotal, i != BP_GAMEGAP );
	}
}

void AppendBudget( std::string &out, double tickIntervalMs )
{
	const Sample &frame = g_bench.phase[BP_GAMEFRAME];
	if ( frame.n == 0 || tickIntervalMs <= 0.0 )
		return;

	uint64_t over = 0;
	for ( size_t i = 0; i < frame.filled; ++i )
	{
		if ( frame.ring[i] > tickIntervalMs )
			++over;
	}

	Fmt( out, "budget     %.1f%% of the %.2f ms tick   over-budget ticks %llu of last %llu   worst %.3f ms\n",
		frame.Mean( ) / tickIntervalMs * 100.0, tickIntervalMs,
		( unsigned long long )over, ( unsigned long long )frame.filled, frame.high );
}

}
