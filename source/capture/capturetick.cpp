#include "core/bytes.h"
#include "props/propplan.h"
#include "capture/recorder.h"

#include "edict.h"
#include "eiface.h"
#include "iserverentity.h"
#include "iservernetworkable.h"
#include "server_class.h"

namespace Chronos
{

// Written once when an edict stops existing so a seek can hide or delete the
// entity instead of leaving it frozen at its last recorded transform.
static void EmitGone( std::vector<uint8_t> &out, uint16_t index, EntitySlot &slot )
{
	if ( !slot.live )
		return;

	Put<uint16_t>( out, index );
	Put<uint16_t>( out, 0 );
	Put<uint8_t>( out, REC_GONE );
	slot = EntitySlot( );
}

static void RefreshIdentity( edict_t *edict, EntitySlot &slot )
{
	slot.classNameId = InternString( edict->GetClassName( ) );

	IServerEntity *serverEnt = edict->GetIServerEntity( );
	slot.modelNameId = InternString( serverEnt != nullptr ? STRING( serverEnt->GetModelName( ) ) : "" );
}

static void CaptureEntity( std::vector<uint8_t> &out, int index, int32_t tick )
{
	Recorder &rec = Rec( );
	EntitySlot &slot = rec.slots[index];
	edict_t *edict = g_engine->PEntityOfEntIndex( index );
	IServerUnknown *unknown = edict != nullptr && !edict->IsFree( ) ? edict->GetUnknown( ) : nullptr;
	CBaseEntity *ent = unknown != nullptr ? unknown->GetBaseEntity( ) : nullptr;
	const ClassPlan *plan = ent != nullptr ? GetPlan( unknown->GetNetworkable( )->GetServerClass( ), ent ) : nullptr;

	if ( plan == nullptr || plan->blobSize == 0 )
	{
		EmitGone( out, ( uint16_t )index, slot );
		return;
	}

	static std::vector<uint8_t> current;
	ScrapeEntity( ent, plan, current );

	bool key = slot.needKey || slot.classId != plan->id || slot.blob.size( ) != current.size( ) ||
		( ( tick + index ) % rec.keyInterval ) == 0;
	if ( key )
		RefreshIdentity( edict, slot );

	EmitRecord( out, ( uint16_t )index, slot, plan, current, key );
	slot.blob = current;
	slot.classId = plan->id;
	slot.live = true;
	slot.needKey = false;
}

// Ticks are counted here rather than read from CGlobalVars: that struct's
// layout differs between this SDK and the shipped engine, and reading the wrong
// field froze the counter at a constant.
void CaptureTick( float curTime )
{
	Recorder &rec = Rec( );
	if ( !rec.recording || g_engine == nullptr )
		return;

	int32_t tick = rec.lastTick + 1;
	rec.lastTick = tick;
	std::vector<uint8_t> out = AcquireBuffer( );

	// Stage clones are replay furniture standing in the live world, so
	// recording them would file a replay back into its own recording.
	for ( int i = 0; i < kMaxEdicts; ++i )
	{
		if ( rec.skip[i] != 0 )
			EmitGone( out, ( uint16_t )i, rec.slots[i] );
		else
			CaptureEntity( out, i, tick );
	}

	Put<uint16_t>( out, kEndOfFrame );
	PushFrame( tick, curTime, out );
}

}
