#include "props/propplan.h"

#include "dt_common.h"
#include "dt_send.h"
#include "eiface.h"
#include "server_class.h"

#include <cstring>

namespace Chronos
{

static std::vector<ClassPlan *> s_plans;
static std::unordered_map<ServerClass *, uint16_t> s_planByClass;

// SPROP_COLLAPSIBLE is only set on inline sub-tables, so whatever proxy sits on
// one of those is the identity proxy for this build of the server binary.
static void DetectIdentityProxy( )
{
	std::unordered_map<void *, int> histogram;
	void *best = nullptr;
	int bestCount = 0;

	for ( ServerClass *sc = g_serverDLL->GetAllServerClasses( ); sc != nullptr; sc = sc->m_pNext )
	{
		SendTable *table = sc->m_pTable;
		for ( int i = 0; table != nullptr && i < table->GetNumProps( ); ++i )
		{
			SendProp *prop = table->GetProp( i );
			if ( prop == nullptr || prop->GetType( ) != DPT_DataTable )
				continue;

			void *fn = ( void * )prop->GetDataTableProxyFn( );
			if ( ( prop->GetFlags( ) & SPROP_COLLAPSIBLE ) != 0 )
			{
				g_identityProxy = fn;
				return;
			}

			int count = ++histogram[fn];
			if ( count > bestCount )
			{
				bestCount = count;
				best = fn;
			}
		}
	}

	g_identityProxy = best;
}

const ClassPlan *GetPlan( ServerClass *sc, const void *entity )
{
	if ( sc == nullptr || sc->m_pTable == nullptr )
		return nullptr;

	std::unordered_map<ServerClass *, uint16_t>::iterator it = s_planByClass.find( sc );
	if ( it != s_planByClass.end( ) )
		return s_plans[it->second];

	if ( g_identityProxy == nullptr )
		DetectIdentityProxy( );

	static std::vector<uint8_t> seen( kMaxPropOffset );
	memset( &seen[0], 0, seen.size( ) );

	ClassPlan *plan = new ClassPlan( );
	plan->id = ( uint16_t )s_plans.size( );
	plan->netName = sc->GetName( );
	FlattenTable( sc->m_pTable, 0, *plan, &seen[0], 0, entity );
	FinalizePlan( *plan );

	s_planByClass[sc] = plan->id;
	s_plans.push_back( plan );
	return plan;
}

const ClassPlan *PlanById( uint16_t id )
{
	return id < s_plans.size( ) ? s_plans[id] : nullptr;
}

void ResetPlans( )
{
	for ( size_t i = 0; i < s_plans.size( ); ++i )
		delete s_plans[i];

	s_plans.clear( );
	s_planByClass.clear( );
	g_identityProxy = nullptr;
}

}
