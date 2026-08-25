#pragma once

#include <cstdint>

#if defined( _WIN32 )
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <intrin.h>
#else
	#include <time.h>
	#if defined( __i386__ ) || defined( __x86_64__ )
		#include <x86intrin.h>
	#endif
#endif

namespace Chronos
{

// Frame and tick boundaries are timed with QueryPerformanceCounter (clock_gettime
// on Linux). The probes inside the capture loop use rdtsc instead: at 8192 edicts
// a tick, QPC's call cost is larger than the per-entity work it would be measuring.

#if defined( _WIN32 )

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

#else

// CLOCK_MONOTONIC already ticks in nanoseconds, so it plays the role of both the
// counter and its own fixed-billion frequency -- no calibration step needed.
inline int64_t QpcFreq( )
{
	return 1000000000LL;
}

inline int64_t QpcNow( )
{
	struct timespec ts;
	clock_gettime( CLOCK_MONOTONIC, &ts );
	return ( int64_t )ts.tv_sec * 1000000000LL + ( int64_t )ts.tv_nsec;
}

inline double QpcToMs( int64_t ticks )
{
	return ( double )ticks / 1000000.0;
}

#endif

inline uint64_t Cycles( )
{
#if defined( __i386__ ) || defined( __x86_64__ ) || defined( _M_IX86 ) || defined( _M_X64 )
	return __rdtsc( );
#else
	return ( uint64_t )QpcNow( );
#endif
}

}
