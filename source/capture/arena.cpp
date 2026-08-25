#include "capture/recorder.h"
#include "tempents/tempents.h"

namespace Chronos
{

Recorder &Rec( )
{
	static Recorder recorder;
	return recorder;
}

// Frame buffers are recycled instead of freed: delta frames are small and
// churn at tickrate, so per-tick heap traffic would dominate the capture cost.
std::vector<uint8_t> AcquireBuffer( )
{
	Recorder &rec = Rec( );
	std::vector<uint8_t> buffer;
	if ( !rec.pool.empty( ) )
	{
		buffer.swap( rec.pool.back( ) );
		rec.pool.pop_back( );
		buffer.clear( );
	}

	if ( buffer.capacity( ) < rec.lastFrameBytes )
		buffer.reserve( rec.lastFrameBytes );

	return buffer;
}

void PushFrame( int32_t tick, float curTime, std::vector<uint8_t> &data )
{
	Recorder &rec = Rec( );

	rec.frames.push_back( Frame( ) );
	Frame &frame = rec.frames.back( );
	frame.tick = tick;
	frame.curTime = curTime;
	frame.data.swap( data );
	rec.bytes += frame.data.size( );
	rec.lastFrameBytes = frame.data.size( );

	while ( rec.bytes > rec.byteCap && rec.frames.size( ) > 1 )
	{
		std::vector<uint8_t> &oldest = rec.frames.front( ).data;
		rec.bytes -= oldest.size( );
		if ( rec.pool.size( ) < 256 )
		{
			rec.pool.push_back( std::vector<uint8_t>( ) );
			rec.pool.back( ).swap( oldest );
		}
		rec.frames.pop_front( );
	}

	if ( !rec.frames.empty( ) )
		PruneTempEnts( rec.frames.front( ).tick );
}

// Returns the index of the newest frame at or before `tick`, or -1.
int FrameIndexAtTick( int32_t tick )
{
	Recorder &rec = Rec( );
	int low = 0, high = ( int )rec.frames.size( ) - 1, best = -1;

	while ( low <= high )
	{
		int mid = ( low + high ) / 2;
		if ( rec.frames[mid].tick <= tick )
		{
			best = mid;
			low = mid + 1;
		}
		else
		{
			high = mid - 1;
		}
	}

	return best;
}

}
