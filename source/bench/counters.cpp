#include "bench/counters.h"

namespace Chronos
{

BenchState g_bench;

const char *PhaseName( int phase )
{
	static const char *kNames[BP_COUNT] = {
		"gameframe.total", "gameframe.gap", "capture.tick",
		"capture.scrape", "capture.emit", "capture.plan",
		"restore.tick", "rebuild.seek", "tempent.hook", "lua.tick"
	};

	return phase >= 0 && phase < BP_COUNT ? kNames[phase] : "?";
}

// rdtsc counts cycles, not time, and its rate is not the nominal clock speed on
// any modern part. Pin it against QPC once so the deep numbers are in
// milliseconds like everything else, and measure the probe pair itself so the
// report can state its own error bar rather than pretending it has none.
void CalibrateClock( )
{
	int64_t qpcStart = QpcNow( );
	uint64_t cycStart = Cycles( );
	while ( QpcToMs( QpcNow( ) - qpcStart ) < 20.0 )
		;

	double elapsed = QpcToMs( QpcNow( ) - qpcStart );
	g_bench.cyclesPerMs = elapsed > 0.0 ? ( double )( Cycles( ) - cycStart ) / elapsed : 0.0;

	// Summed rather than discarded: a loop that throws the reads away optimises
	// out, and the report then claims a probe cost of a fifth of a nanosecond.
	uint64_t probeStart = Cycles( );
	uint64_t sink = 0;
	for ( int i = 0; i < 10000; ++i )
		sink += Cycles( );

	g_bench.probeSink = sink;

	double probeCycles = ( double )( Cycles( ) - probeStart ) / 10000.0;
	g_bench.probeNs = g_bench.cyclesPerMs > 0.0 ? probeCycles / g_bench.cyclesPerMs * 1000000.0 : 0.0;
}

void BenchReset( )
{
	for ( int i = 0; i < BP_COUNT; ++i )
	{
		g_bench.phase[i].Reset( );
		g_bench.cycles[i] = 0;
	}

	for ( int i = 0; i < BC_COUNT; ++i )
	{
		g_bench.counter[i].Reset( );
		g_bench.volume[i] = 0;
	}

	g_bench.nativeMs = 0.0;
	g_bench.lastFrameQpc = 0;
	g_bench.resetQpc = QpcNow( );
	SampleProcess( g_bench.baseline );
}

// The GameFrame detour only exists while measuring: an idle server should carry
// no patched vtable entry it did not ask for.
void BenchEnable( bool on, bool deep )
{
	if ( on && g_bench.cyclesPerMs == 0.0 )
		CalibrateClock( );

	g_bench.on = on;
	g_bench.deep = on && deep;

	if ( on )
	{
		InstallGameFrameHook( );
		BenchReset( );
	}
	else
	{
		RemoveGameFrameHook( );
	}
}

}
