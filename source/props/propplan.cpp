#include "props/propplan.h"

#include "dt_common.h"
#include "dt_send.h"
#include "props/proxytest.h"

namespace Chronos
{

void *g_identityProxy = nullptr;

// Maps a non-datatable SendProp onto a raw byte range inside the entity.
// Arrays report their storage through the element prop, not the array prop.
bool ResolveLeaf( SendProp *prop, int base, int &outOffset, int &outSize, int &outType )
{
	int offset = base + prop->GetOffset( );
	int size = 0;
	outType = prop->GetType( );

	switch ( prop->GetType( ) )
	{
		case DPT_Int: size = sizeof( int ); break;
		case DPT_Float: size = sizeof( float ); break;
		case DPT_Vector: size = sizeof( float ) * 3; break;
		case DPT_VectorXY: size = sizeof( float ) * 3; break;
		case DPT_Array:
		{
			SendProp *element = prop->GetArrayProp( );
			if ( element == nullptr )
				return false;

			// A length proxy means the storage is a CUtlVector: the elements
			// live on the heap, so these offsets address the vector header and
			// its data pointer, not an inline array. Copying that back over a
			// live entity restores a stale pointer and corrupts it.
			if ( prop->GetArrayLengthProxy( ) != nullptr )
				return false;

			SendPropType elementType = element->GetType( );
			if ( elementType == DPT_DataTable || elementType == DPT_String )
				return false;

			outType = elementType;

			offset = base + element->GetOffset( );
			size = prop->GetNumElements( ) * prop->GetElementStride( );
			if ( size > 1024 )
				return false;

			break;
		}
		default: return false;
	}

	outOffset = offset;
	outSize = size;
	return size > 0 && offset > 0 && offset + size <= kMaxPropOffset;
}

// Walks a SendTable into a flat offset/size list. `seen` is a byte-per-offset
// map that kills duplicates, mainly the vector-element props that alias the
// vector they were split from.
void FlattenTable( SendTable *table, int base, ClassPlan &plan, uint8_t *seen, int depth,
	const void *entity )
{
	if ( table == nullptr || depth > 8 )
		return;

	for ( int i = 0; i < table->GetNumProps( ); ++i )
	{
		SendProp *prop = table->GetProp( i );
		if ( prop == nullptr || prop->IsExcludeProp( ) || prop->IsInsideArray( ) )
			continue;

		// m_flSimulationTime and m_flAnimTime are encoded relative to the
		// current tick. Restoring old values makes clients interpolate against
		// timestamps from the past, which reads as jitter on every entity.
		if ( ( prop->GetFlags( ) & SPROP_ENCODED_AGAINST_TICKCOUNT ) != 0 )
			continue;

		if ( prop->GetType( ) == DPT_DataTable )
		{
			const void *structBase = reinterpret_cast<const unsigned char *>( entity ) + base;
			if ( IsInlineProxy( prop, structBase ) )
				FlattenTable( prop->GetDataTable( ), base + prop->GetOffset( ), plan, seen,
					depth + 1, entity );
			continue;
		}

		int offset = 0, size = 0, type = 0;
		if ( !ResolveLeaf( prop, base, offset, size, type ) || seen[offset] != 0 )
			continue;

		seen[offset] = 1;
		PlanEntry entry = { ( uint16_t )offset, ( uint16_t )size, ( uint8_t )type };
		plan.entries.push_back( entry );
		plan.names.push_back( prop->GetName( ) != nullptr ? prop->GetName( ) : "?" );
		plan.prefix.push_back( plan.blobSize );
		plan.blobSize += size;
	}
}

}
