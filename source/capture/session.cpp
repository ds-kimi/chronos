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

	for ( size_t i = 0; i < rec.slots.size( ); ++i )
		rec.slots[i] = EntitySlot( );

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
