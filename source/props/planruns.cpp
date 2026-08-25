#include "props/propplan.h"

#include <algorithm>

namespace Chronos
{

// SendTable walk order interleaves sub-tables, so members that sit next to each
// other in the object arrive scattered through the entry list. Sorting by
// offset is what lets them coalesce; nothing depends on entry order, because
// every consumer of a named slot resolves it by name at runtime.
static void SortPlan( ClassPlan &plan )
{
	std::vector<uint16_t> order( plan.entries.size( ) );
	for ( size_t i = 0; i < order.size( ); ++i )
		order[i] = ( uint16_t )i;

	const std::vector<PlanEntry> &entries = plan.entries;
	for ( size_t i = 1; i < order.size( ); ++i )
	{
		uint16_t key = order[i];
		size_t j = i;
		while ( j > 0 && entries[order[j - 1]].offset > entries[key].offset )
		{
			order[j] = order[j - 1];
			--j;
		}

		order[j] = key;
	}

	std::vector<PlanEntry> sortedEntries( order.size( ) );
	std::vector<std::string> sortedNames( order.size( ) );
	for ( size_t i = 0; i < order.size( ); ++i )
	{
		sortedEntries[i] = plan.entries[order[i]];
		sortedNames[i] = plan.names[order[i]];
	}

	plan.entries.swap( sortedEntries );
	plan.names.swap( sortedNames );
}

// The blob is contiguous by construction, so a run only needs the entity
// offsets to butt up against each other.
static void BuildRuns( ClassPlan &plan )
{
	plan.prefix.assign( plan.entries.size( ), 0 );
	plan.runs.clear( );
	plan.blobSize = 0;

	for ( size_t i = 0; i < plan.entries.size( ); ++i )
	{
		const PlanEntry &entry = plan.entries[i];
		plan.prefix[i] = plan.blobSize;

		PlanRun *last = plan.runs.empty( ) ? nullptr : &plan.runs.back( );
		if ( last != nullptr && last->offset + last->size == entry.offset )
		{
			last->size += entry.size;
			++last->count;
		}
		else
		{
			PlanRun run = { plan.blobSize, entry.size, entry.offset, ( uint16_t )i, 1 };
			plan.runs.push_back( run );
		}

		plan.blobSize += entry.size;
	}
}

void FinalizePlan( ClassPlan &plan )
{
	SortPlan( plan );
	BuildRuns( plan );
}

}
