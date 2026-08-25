#include "bench/counters.h"
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
void EmitGone( std::vector<uint8_t> &out, uint16_t index, EntitySlot &slot )
{
	if ( !slot.live )
		return;

	EmitHeader( out, index, 0, REC_GONE );
	slot = EntitySlot( );
	if ( g_bench.on )
		BenchCountGone( );
}

// Interning hashes the string, and for a model path that means building a
// std::string first. Both engine pointers are stable for the entity's life, so
// almost every keyframe answers from the pair cached on the slot instead.
void RefreshIdentity( edict_t *edict, EntitySlot &slot )
{
	const char *className = edict->GetClassName( );
	if ( className != slot.classNamePtr )
	{
		slot.classNameId = InternString( className );
		slot.classNamePtr = className;
	}

	IServerEntity *serverEnt = edict->GetIServerEntity( );
	const char *model = serverEnt != nullptr ? STRING( serverEnt->GetModelName( ) ) : "";
	if ( model != slot.modelNamePtr )
	{
		slot.modelNameId = InternString( model );
		slot.modelNamePtr = model;
	}
}

}
