#include "bench/counters.h"
#include "bench/report.h"
#include "capture/recorder.h"

namespace Chronos
{

// A seek is the expensive operation in the whole system: every one of them
// replays keyInterval*2 frames over all 8192 work slots before it can push.
static void AppendSeek( std::string &out )
{
	const Sample &rebuild = g_bench.phase[BP_REBUILD];
	if ( rebuild.n == 0 )
		return;

	Fmt( out, "seek       %llu seeks  mean %.3f ms  p99 %.3f ms  max %.3f ms  %d frames replayed each\n",
		( unsigned long long )rebuild.n, rebuild.Mean( ), rebuild.Percentile( 0.99 ),
		rebuild.high, Rec( ).keyInterval * 2 );
}

std::string BuildReport( const std::string &label, double tickIntervalMs )
{
	std::string out;
	if ( !g_bench.on )
		return "chronos bench: instrumentation is off\n";

	Fmt( out, "\n== %s ==\n", label.c_str( ) );
	Fmt( out, "clock      rdtsc %.3f GHz  probe %.1f ns  qpc %.3f MHz  deep %s\n",
		g_bench.cyclesPerMs / 1000000.0, g_bench.probeNs,
		( double )QpcFreq( ) / 1000000.0, g_bench.deep ? "on" : "off" );

	AppendPhases( out, g_bench.phase[BP_GAMEFRAME].sum );
	AppendBudget( out, tickIntervalMs );
	AppendTickrate( out );
	AppendMemory( out );
	AppendCpu( out );
	AppendCapture( out );
	AppendSeek( out );
	return out;
}

}
