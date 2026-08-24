#include "core/filter.h"
#include "props/propplan.h"

#include "dt_send.h"
#include "capture/recorder.h"
#include "tempents/tempents.h"

namespace Chronos
{

void ClearTempEnts( )
{
	g_tempEnts.clear( );
}

void PruneTempEnts( int32_t firstTick )
{
	while ( !g_tempEnts.empty( ) && g_tempEnts.front( ).tick < firstTick )
		g_tempEnts.pop_front( );
}

// Records are appended in tick order, so the window for a frame is found by
// binary search instead of walking tens of thousands of effects every tick.
static size_t FirstAfter( int32_t tick )
{
	size_t low = 0, high = g_tempEnts.size( );

	while ( low < high )
	{
		size_t mid = ( low + high ) / 2;
		if ( g_tempEnts[mid].tick <= tick )
			low = mid + 1;
		else
			high = mid;
	}

	return low;
}

// Effects recorded in (from, to] are replayed by writing the captured bytes back
// into the original sender and asking the engine to broadcast it again.
void PlayTempEnts( int32_t from, int32_t to, int only )
{
	if ( to <= from || g_engine == nullptr )
		return;

	BroadcastFilter filter( only );
	if ( filter.GetRecipientCount( ) == 0 )
		return;

	g_replayingTempEnts = true;

	for ( size_t i = FirstAfter( from ); i < g_tempEnts.size( ); ++i )
	{
		const TempEntRecord &record = g_tempEnts[i];
		if ( record.tick > to )
			break;

		const ClassPlan *plan = PlanForTable( record.table, record.sender );
		if ( plan == nullptr || record.blob.size( ) != plan->blobSize )
			continue;

		uint8_t *base = reinterpret_cast<uint8_t *>( record.sender );
		for ( size_t k = 0; k < plan->entries.size( ); ++k )
			memcpy( base + plan->entries[k].offset, &record.blob[plan->prefix[k]],
				plan->entries[k].size );

		g_engine->PlaybackTempEntity( filter, 0.0f, record.sender, record.table, record.classID );
	}

	g_replayingTempEnts = false;
}

}
