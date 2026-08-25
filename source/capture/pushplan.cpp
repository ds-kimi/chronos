#include "props/propplan.h"
#include "capture/recorder.h"

#include "edict.h"
#include "eiface.h"
#include "iservernetworkable.h"
#include "iserverunknown.h"
#include "server_class.h"

namespace Chronos
{

static const void *s_pushClass[kMaxEdicts] = { nullptr };
static const ClassPlan *s_pushPlan[kMaxEdicts] = { nullptr };

// Same trick the capture scan uses on its own slots: an edict keeps its class
// for life, so the hash lookup collapses into a pointer compare against
// whatever this index resolved to last time. A recycled index that came back as
// a different class has a different ServerClass pointer and misses the cache.
const ClassPlan *PushPlan( int target, IServerUnknown *unknown, CBaseEntity *ent )
{
	ServerClass *sc = unknown->GetNetworkable( )->GetServerClass( );
	if ( sc == s_pushClass[target] && s_pushPlan[target] != nullptr )
		return s_pushPlan[target];

	const ClassPlan *plan = GetPlan( sc, ent );
	s_pushClass[target] = sc;
	s_pushPlan[target] = plan;
	return plan;
}

// Plans are owned by the registry and freed wholesale, so anything holding one
// has to let go at the same moment.
void ResetPushPlans( )
{
	for ( int i = 0; i < kMaxEdicts; ++i )
	{
		s_pushClass[i] = nullptr;
		s_pushPlan[i] = nullptr;
	}
}

}
