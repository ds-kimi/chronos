#include "bench/counters.h"
#include "capture/recorder.h"
#include "capture/workstate.h"

namespace Chronos
{

// Tick the working state currently describes, or this when it describes none.
static const int32_t kNoBuild = -2000000000;
static int32_t s_builtTick = kNoBuild;

void InvalidateRebuild( )
{
	for ( int i = 0; i <= g_workMax && i < ( int )g_work.size( ); ++i )
	{
		g_work[i].valid = false;
		g_work[i].born = kBornUnknown;
	}

	g_workMax = 0;
	s_builtTick = kNoBuild;
}

// Frame the incremental path may resume from, or -1 when the key window has to
// be replayed from scratch. Playback walks forward one tick at a time and asks
// for the state twice on each of them -- once for the entity manifest, once for
// the restore -- so both of those land here rather than replaying 128 frames.
// Matching on the tick rather than the frame index is what survives the ring
// pruning its front out from under an index taken a moment ago.
//
// The distance bound is not an optimisation, it is the whole correctness of the
// choice: a full rebuild costs one key window, so resuming is only cheaper than
// starting over while the gap is smaller than that. Without it a seek that
// jumped forward an hour replayed every frame in between, and the seek phase
// went from a fifth of a millisecond to five.
static int ResumeFrom( const Recorder &rec, int target )
{
	if ( s_builtTick == kNoBuild )
		return -1;

	int built = FrameIndexAtTick( s_builtTick );
	if ( built < 0 || built > target || target - built > rec.keyInterval * 2 ||
		rec.frames[built].tick != s_builtTick )
		return -1;

	return built;
}

// Replays keyframes and deltas from a window guaranteed to contain at least one
// keyframe per entity, leaving the reconstructed world in g_work.
bool BuildStateAtTick( int32_t tick )
{
	Recorder &rec = Rec( );
	int target = FrameIndexAtTick( tick );
	if ( target < 0 )
		return false;

	int64_t began = g_bench.on ? QpcNow( ) : 0;
	int resume = ResumeFrom( rec, target );
	int start = resume + 1;

	if ( resume < 0 )
	{
		InvalidateRebuild( );
		start = target - rec.keyInterval * 2;
		if ( start < 0 )
			start = 0;
	}

	for ( int i = start; i <= target; ++i )
		ApplyFrame( rec.frames[i] );

	s_builtTick = rec.frames[target].tick;
	BenchCharge( BP_REBUILD, began );
	return true;
}

}
