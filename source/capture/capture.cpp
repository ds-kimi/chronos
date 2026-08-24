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

void ScrapeEntity( CBaseEntity *ent, const ClassPlan *plan, std::vector<uint8_t> &dst )
{
	dst.resize( plan->blobSize );
	const uint8_t *base = reinterpret_cast<const uint8_t *>( ent );

	for ( size_t i = 0; i < plan->entries.size( ); ++i )
	{
		const PlanEntry &entry = plan->entries[i];
		memcpy( &dst[plan->prefix[i]], base + entry.offset, entry.size );
	}
}

// A keyframe carries the whole blob plus identity strings; a delta carries only
// the plan slots whose bytes moved since the previous capture of this edict.
void EmitRecord( std::vector<uint8_t> &out, uint16_t index, const EntitySlot &slot,
	const ClassPlan *plan, const std::vector<uint8_t> &cur, bool key )
{
	Put<uint16_t>( out, index );
	Put<uint16_t>( out, plan->id );
	Put<uint8_t>( out, key ? REC_KEY : REC_DELTA );

	if ( key )
	{
		Put<uint16_t>( out, slot.classNameId );
		Put<uint16_t>( out, slot.modelNameId );
		PutBytes( out, cur.empty( ) ? nullptr : &cur[0], cur.size( ) );
		return;
	}

	size_t countPos = out.size( );
	Put<uint16_t>( out, 0 );
	uint16_t count = 0;

	for ( size_t i = 0; i < plan->entries.size( ); ++i )
	{
		size_t at = plan->prefix[i];
		uint16_t size = plan->entries[i].size;
		if ( memcmp( &cur[at], &slot.blob[at], size ) == 0 )
			continue;

		Put<uint16_t>( out, ( uint16_t )i );
		PutBytes( out, &cur[at], size );
		++count;
	}

	memcpy( &out[countPos], &count, sizeof( count ) );
}

}
