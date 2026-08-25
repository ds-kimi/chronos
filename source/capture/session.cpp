#include "props/propplan.h"
#include "capture/recorder.h"
#include "tempents/tempents.h"

namespace Chronos
{

void ClearRecording( )
{
	Recorder &rec = Rec( );
	rec.frames.clear( );
	rec.pool.clear( );
	rec.bytes = 0;
	rec.lastTick = -1;
	rec.highWater = 0;

	for ( size_t i = 0; i < rec.slots.size( ); ++i )
		rec.slots[i] = EntitySlot( );

	// A new session restarts the tick counter, so a rebuilt state cached
	// against a tick number from the old one would answer for the wrong frame.
	InvalidateRebuild( );
	ResetPushPlans( );
	ClearTempEnts( );
	ResetStrings( );
}

void StartRecording( )
{
	ClearRecording( );
	Rec( ).recording = true;
}

void StopRecording( )
{
	Rec( ).recording = false;
}

const Frame *FrameAtTick( int32_t tick )
{
	int index = FrameIndexAtTick( tick );
	return index >= 0 ? &Rec( ).frames[index] : nullptr;
}

}
