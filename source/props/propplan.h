#pragma once

#include "core/chronos.h"

namespace Chronos
{

// Members further than this into an entity are assumed to be a bad SendProp
// offset (custom proxies sometimes park garbage there) and get dropped.
static const int kMaxPropOffset = 0x4000;

// Proxy used by SendPropDataTable when the sub-table lives inline in the same
// object. Sub-tables behind any other proxy point elsewhere and are skipped.
extern void *g_identityProxy;

bool ResolveLeaf( SendProp *prop, int base, int &outOffset, int &outSize, int &outType );
void FlattenTable( SendTable *table, int base, ClassPlan &plan, uint8_t *seen, int depth,
	const void *entity );

const ClassPlan *GetPlan( ServerClass *sc, const void *entity );
const ClassPlan *PlanById( uint16_t id );
void ResetPlans( );

uint16_t InternString( const char *str );
const char *StringById( uint16_t id );
void ResetStrings( );

}
