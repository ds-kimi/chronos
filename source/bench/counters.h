#pragma once

#include <string>

#include "bench/clock.h"
#include "bench/procstat.h"
#include "bench/sample.h"

namespace Chronos
{

// Timed spans, all recorded in milliseconds.
enum BenchPhase
{
	BP_GAMEFRAME = 0,	// full engine server frame, work only
	BP_GAMEGAP,			// wall time between frames, so real tickrate is measurable
	BP_CAPTURE,			// CaptureTick
	BP_SCRAPE,			// deep only: ScrapeEntity, summed over one tick
	BP_EMIT,			// deep only: EmitRecord, summed over one tick
	BP_PLAN,			// deep only: GetPlan, summed over one tick
	BP_RESTORE,			// RestoreTick
	BP_REBUILD,			// BuildStateAtTick
	BP_TEMPENT,			// our work inside the PlaybackTempEntity hook
	BP_LUATICK,			// fed in from the Lua Tick hook
	BP_COUNT
};

// Volume counters, all recorded per captured tick.
enum BenchCounter
{
	BC_FRAMEBYTES = 0,
	BC_LIVE,
	BC_KEYREC,
	BC_DELTAREC,
	BC_GONEREC,
	BC_EMITBYTES,
	BC_BLOBBYTES,
	BC_COUNT
};

struct BenchState
{
	Sample phase[BP_COUNT];
	Sample counter[BC_COUNT];
	ProcSnapshot baseline;

	// Per-tick accumulators, folded into the samples when the tick ends.
	uint64_t cycles[BP_COUNT];
	uint64_t volume[BC_COUNT];

	double cyclesPerMs;
	double probeNs;

	// Native time charged since the last lua.tick mark, so the Lua figure can
	// be the Lua work alone instead of the whole hook body it sits inside.
	double nativeMs;

	// Keeps the calibration loop's reads observable so it is not optimised out.
	uint64_t probeSink;
	int64_t lastFrameQpc;
	int64_t resetQpc;
	bool on;
	bool deep;

	BenchState( ) : cyclesPerMs( 0.0 ), probeNs( 0.0 ), nativeMs( 0.0 ), probeSink( 0 ), lastFrameQpc( 0 ),
		resetQpc( 0 ), on( false ), deep( false )
	{
		for ( int i = 0; i < BP_COUNT; ++i ) cycles[i] = 0;
		for ( int i = 0; i < BC_COUNT; ++i ) volume[i] = 0;
	}
};

extern BenchState g_bench;

const char *PhaseName( int phase );
void BenchEnable( bool on, bool deep );
void BenchReset( );
void CalibrateClock( );

// Per-tick probes, defined in bench/tick.cpp.
int64_t BenchTickBegin( );
void BenchTickEnd( int64_t began, size_t frameBytes );
void BenchCountEntity( bool key, size_t wrote, size_t blobSize );
void BenchCountGone( );

bool InstallGameFrameHook( );
void RemoveGameFrameHook( );

std::string BuildReport( const std::string &label, double tickIntervalMs );

// Cheap enough to sit inside the per-entity loop: one predictable branch when
// deep timing is off, which is every run that is not chasing a hot member.
inline uint64_t BenchCycles( )
{
	return g_bench.deep ? Cycles( ) : 0;
}

inline void BenchAddCycles( BenchPhase phase, uint64_t began )
{
	if ( g_bench.deep )
		g_bench.cycles[phase] += Cycles( ) - began;
}

}
