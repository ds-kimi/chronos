#include "bench/counters.h"
#include "core/bytes.h"
#include "props/propplan.h"
#include "capture/recorder.h"

namespace Chronos
{

static std::vector<WorkSlot> s_work( kMaxEdicts );

// Folds one entity record into the working state. Advances `cursor` past the
// record even when the record is unusable, so the frame stays parseable.
static void ApplyRecord( const uint8_t *&cursor, uint16_t index )
{
	uint16_t classId = Get<uint16_t>( cursor );
	uint8_t type = Get<uint8_t>( cursor );
	WorkSlot &work = s_work[index];

	if ( type == REC_GONE )
	{
		work.valid = false;
		work.born = kBornUnknown;
		return;
	}


	const ClassPlan *plan = PlanById( classId );
	if ( plan == nullptr )
		return;

	if ( type == REC_KEY )
	{
		work.classNameId = Get<uint16_t>( cursor );
		work.modelNameId = Get<uint16_t>( cursor );
		work.born = Get<int32_t>( cursor );
		work.blob.assign( cursor, cursor + plan->blobSize );
		cursor += plan->blobSize;
		work.classId = classId;
		work.valid = true;
		return;
	}

	uint16_t count = Get<uint16_t>( cursor );
	for ( uint16_t i = 0; i < count; ++i )
	{
		uint16_t slot = Get<uint16_t>( cursor );
		if ( slot >= plan->entries.size( ) )
			return;

		uint16_t size = plan->entries[slot].size;
		if ( work.valid && work.classId == classId && work.blob.size( ) == plan->blobSize )
			memcpy( &work.blob[plan->prefix[slot]], cursor, size );
		cursor += size;
	}
}

static void ApplyFrame( const Frame &frame )
{
	if ( frame.data.empty( ) )
		return;

	const uint8_t *cursor = &frame.data[0];
	const uint8_t *end = cursor + frame.data.size( );

	while ( cursor + sizeof( uint16_t ) <= end )
	{
		uint16_t index = Get<uint16_t>( cursor );
		if ( index >= kMaxEdicts )
			break;

		ApplyRecord( cursor, index );
	}
}

// Replays keyframes and deltas from a window guaranteed to contain at least one
// keyframe per entity, leaving the reconstructed world in s_work.
bool BuildStateAtTick( int32_t tick )
{
	Recorder &rec = Rec( );
	int target = FrameIndexAtTick( tick );
	if ( target < 0 )
		return false;

	int64_t began = g_bench.on ? QpcNow( ) : 0;
	int start = target - rec.keyInterval * 2;
	if ( start < 0 )
		start = 0;

	for ( size_t i = 0; i < s_work.size( ); ++i )
	{
		s_work[i].valid = false;
		s_work[i].born = kBornUnknown;
	}

	for ( int i = start; i <= target; ++i )
		ApplyFrame( rec.frames[i] );

	if ( began != 0 )
	{
		double ms = QpcToMs( QpcNow( ) - began );
		g_bench.phase[BP_REBUILD].Add( ms );
		g_bench.nativeMs += ms;
	}

	return true;
}

const WorkSlot *WorkAt( int index )
{
	return index >= 0 && index < ( int )s_work.size( ) ? &s_work[index] : nullptr;
}

}
