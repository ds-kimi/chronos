#include "bench/counters.h"
#include "bench/report.h"
#include "capture/recorder.h"

#include <cmath>

namespace Chronos
{

// Gap between server frames is the only honest tickrate: the tick counter the
// recorder keeps is incremented by us and would report a perfect rate even on a
// server that is visibly stuttering.
void AppendTickrate( std::string &out )
{
	const Sample &gap = g_bench.phase[BP_GAMEGAP];
	if ( gap.n == 0 || gap.Mean( ) <= 0.0 )
		return;

	double mean = gap.Mean( );
	double variance = 0.0;
	for ( size_t i = 0; i < gap.filled; ++i )
		variance += ( gap.ring[i] - mean ) * ( gap.ring[i] - mean );

	double jitter = gap.filled > 1 ? sqrt( variance / ( double )gap.filled ) : 0.0;
	double slowest = gap.Percentile( 0.99 );

	Fmt( out, "tickrate   actual %.2f tps   1%%-low %.2f tps   jitter %.3f ms   longest gap %.3f ms\n",
		1000.0 / mean, slowest > 0.0 ? 1000.0 / slowest : 0.0, jitter, gap.high );
}

void AppendMemory( std::string &out )
{
	Recorder &rec = Rec( );
	const Sample &bytes = g_bench.counter[BC_FRAMEBYTES];
	ProcSnapshot now;
	SampleProcess( now );

	double megabyte = 1024.0 * 1024.0;
	Fmt( out, "memory     ring %.1f MB / %.0f MB (%llu frames, %.1f KB/tick, p99 %.1f KB)  pool %llu\n",
		( double )rec.bytes / megabyte, ( double )rec.byteCap / megabyte,
		( unsigned long long )rec.frames.size( ), bytes.Mean( ) / 1024.0,
		bytes.Percentile( 0.99 ) / 1024.0, ( unsigned long long )rec.pool.size( ) );

	double growth = ( double )now.workingSet - ( double )g_bench.baseline.workingSet;
	Fmt( out, "           process WS %.0f MB (%+.0f MB since reset)  private %.0f MB  peak %.0f MB  threads %u\n",
		( double )now.workingSet / megabyte, growth / megabyte,
		( double )now.privateBytes / megabyte, ( double )now.peakWorkingSet / megabyte, now.threads );
	Fmt( out, "           system %.1f GB free of %.1f GB\n",
		( double )now.sysAvail / megabyte / 1024.0, ( double )now.sysTotal / megabyte / 1024.0 );
}

void AppendCpu( std::string &out )
{
	ProcSnapshot now;
	SampleProcess( now );
	if ( !g_bench.baseline.valid )
		return;

	double wall = QpcToMs( QpcNow( ) - g_bench.resetQpc );
	double user = now.userMs - g_bench.baseline.userMs;
	double kernel = now.kernelMs - g_bench.baseline.kernelMs;
	if ( wall <= 0.0 )
		return;

	Fmt( out, "cpu        user %.2f s  kernel %.2f s  over %.1f s -> %.1f%% of one core\n",
		user / 1000.0, kernel / 1000.0, wall / 1000.0, ( user + kernel ) / wall * 100.0 );
}

// Emitted bytes against blob bytes is the delta ratio; it is the number that
// decides how long the ring lasts, and the one a class-level regression moves.
void AppendCapture( std::string &out )
{
	const Sample &live = g_bench.counter[BC_LIVE];
	if ( live.n == 0 )
		return;

	double emitted = g_bench.counter[BC_EMITBYTES].Mean( );
	double blob = g_bench.counter[BC_BLOBBYTES].Mean( );

	Fmt( out, "capture    scanned %d/tick  live %.1f  key %.1f/tick  delta %.1f/tick  gone %.1f/tick\n",
		kMaxEdicts, live.Mean( ), g_bench.counter[BC_KEYREC].Mean( ),
		g_bench.counter[BC_DELTAREC].Mean( ), g_bench.counter[BC_GONEREC].Mean( ) );
	Fmt( out, "           emitted %.1f KB/tick of %.1f KB scanned (%.1f%% of blob)\n",
		emitted / 1024.0, blob / 1024.0, blob > 0.0 ? emitted / blob * 100.0 : 0.0 );

	// Entries against runs says how far coalescing got: the closer runs get to
	// one, the fewer memcpy and memcmp calls the scan makes per entity.
	double entries = g_bench.counter[BC_ENTRIES].Mean( );
	double runs = g_bench.counter[BC_RUNS].Mean( );
	Fmt( out, "           %.0f plan entries in %.0f runs (%.1fx fewer copies)\n",
		entries, runs, runs > 0.0 ? entries / runs : 0.0 );
}

}
