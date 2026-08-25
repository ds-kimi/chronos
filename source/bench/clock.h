#pragma once

#include <cstdint>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>

namespace Chronos
{

// Frame and tick boundaries are timed with QueryPerformanceCounter. The probes
// inside the capture loop use rdtsc instead: at 8192 edicts a tick, QPC's call
// cost is larger than the per-entity work it would be measuring.

inline int64_t QpcFreq( )
{
	static int64_t freq = 0;
	if ( freq == 0 )
	{
		LARGE_INTEGER value;
		QueryPerformanceFrequency( &value );
		freq = value.QuadPart;
	}

	return freq;
}

inline int64_t QpcNow( )
{
	LARGE_INTEGER value;
	QueryPerformanceCounter( &value );
	return value.QuadPart;
}

inline double QpcToMs( int64_t ticks )
{
	return ( double )ticks * 1000.0 / ( double )QpcFreq( );
}

inline uint64_t Cycles( )
{
	return __rdtsc( );
}

}
