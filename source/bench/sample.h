#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Chronos
{

// Percentiles come from a bounded ring of recent values rather than the whole
// run, so a long benchmark cannot grow the sampler into the thing it measures.
static const size_t kSampleRing = 4096;

struct Sample
{
	std::vector<double> ring;
	double sum;
	double low;
	double high;
	uint64_t n;
	size_t head;
	size_t filled;

	Sample( ) : ring( kSampleRing, 0.0 ), sum( 0.0 ), low( 0.0 ), high( 0.0 ),
		n( 0 ), head( 0 ), filled( 0 ) { }

	void Add( double value )
	{
		if ( n == 0 || value < low )
			low = value;
		if ( n == 0 || value > high )
			high = value;

		sum += value;
		++n;
		ring[head] = value;
		head = ( head + 1 ) % kSampleRing;
		if ( filled < kSampleRing )
			++filled;
	}

	void Reset( )
	{
		sum = 0.0;
		low = 0.0;
		high = 0.0;
		n = 0;
		head = 0;
		filled = 0;
	}

	double Mean( ) const
	{
		return n > 0 ? sum / ( double )n : 0.0;
	}

	// Sorts a copy of the ring, so this is only ever called to render a report.
	double Percentile( double fraction ) const
	{
		if ( filled == 0 )
			return 0.0;

		std::vector<double> sorted( ring.begin( ), ring.begin( ) + filled );
		std::sort( sorted.begin( ), sorted.end( ) );

		size_t at = ( size_t )( fraction * ( double )( filled - 1 ) + 0.5 );
		return sorted[at];
	}
};

}
