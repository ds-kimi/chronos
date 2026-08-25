#include "core/bytes.h"
#include "props/propplan.h"
#include "capture/recorder.h"
#include "capture/workstate.h"

namespace Chronos
{

// A keyframe replaces the slot outright: the whole blob plus the identity it
// was recorded under.
static void ApplyKey( const uint8_t *&cursor, uint16_t index, uint16_t classId,
	const ClassPlan *plan )
{
	WorkSlot &work = g_work[index];
	work.classNameId = Get<uint16_t>( cursor );
	work.modelNameId = Get<uint16_t>( cursor );
	work.born = Get<int32_t>( cursor );
	work.blob.assign( cursor, cursor + plan->blobSize );
	cursor += plan->blobSize;

	work.classId = classId;
	work.valid = true;
	if ( index > g_workMax )
		g_workMax = index;
}

// A delta names the plan slots that moved. The cursor is advanced past every
// one of them whether or not the slot can take them, so the frame stays
// parseable when the state underneath it does not match the record.
static void ApplyDelta( const uint8_t *&cursor, uint16_t index, uint16_t classId,
	const ClassPlan *plan )
{
	WorkSlot &work = g_work[index];
	bool usable = work.valid && work.classId == classId &&
		work.blob.size( ) == plan->blobSize;

	uint16_t count = Get<uint16_t>( cursor );
	for ( uint16_t i = 0; i < count; ++i )
	{
		uint16_t slot = Get<uint16_t>( cursor );
		if ( slot >= plan->entries.size( ) )
			return;

		uint16_t size = plan->entries[slot].size;
		if ( usable )
			memcpy( &work.blob[plan->prefix[slot]], cursor, size );

		cursor += size;
	}
}

// Folds one entity record into the working state.
static void ApplyRecord( const uint8_t *&cursor, uint16_t index )
{
	uint16_t classId = Get<uint16_t>( cursor );
	uint8_t type = Get<uint8_t>( cursor );

	if ( type == REC_GONE )
	{
		g_work[index].valid = false;
		g_work[index].born = kBornUnknown;
		return;
	}

	const ClassPlan *plan = PlanById( classId );
	if ( plan == nullptr )
		return;

	if ( type == REC_KEY )
		ApplyKey( cursor, index, classId, plan );
	else
		ApplyDelta( cursor, index, classId, plan );
}

void ApplyFrame( const Frame &frame )
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

}
