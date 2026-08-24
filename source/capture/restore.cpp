#include "props/propplan.h"
#include "capture/recorder.h"

#include "edict.h"
#include "eiface.h"
#include "iservernetworkable.h"
#include "server_class.h"

#include <cstring>

namespace Chronos
{

// A stand-in is written to by name, not wholesale. Two reasons, and the second
// is why the blacklist this replaced was not enough:
//
// Some SendProps resolve to offsets past the end of the object they describe --
// custom proxies park them there, and the plan keeps anything under
// kMaxPropOffset. Reading those during capture is harmless, and writing them
// back onto the entity they came from mostly is too, since the bytes go back
// where they were found. Writing them onto a stand-in puts one object's
// trailing memory into another's, which corrupts whatever the allocator has
// after it; the crash then lands in the next spawn, not here.
//
// So a stand-in gets what a stand-in is for: where it is, how it is posed, and
// what it looks like. Everything else stays on the entity that owns it.
static bool ProxySafe( const std::string &name )
{
	static const char *kSafe[] = {
		"m_vecOrigin", "m_angRotation",
		"m_nSequence", "m_flCycle", "m_flPlaybackRate", "m_flModelScale",
		"m_nSkin", "m_nBody", "m_nHitboxSet",
		"m_nRenderMode", "m_nRenderFX", "m_clrRender",
		"m_iHealth", "m_lifeState"
	};

	for ( size_t i = 0; i < sizeof( kSafe ) / sizeof( kSafe[0] ); ++i )
	{
		if ( name == kSafe[i] )
			return true;
	}

	return false;
}

// Built once per class, on the first restore that writes into a stand-in.
static const std::vector<uint8_t> &ProxyMask( const ClassPlan *plan )
{
	if ( plan->proxySkip.size( ) != plan->entries.size( ) )
	{
		plan->proxySkip.assign( plan->entries.size( ), 0 );
		for ( size_t i = 0; i < plan->names.size( ); ++i )
			plan->proxySkip[i] = ProxySafe( plan->names[i] ) ? 0 : 1;
	}

	return plan->proxySkip;
}

// Writes the reconstructed blob straight back over the entity's members, then
// dirties the edict so the engine reships the props to every client.
static void PushEntity( int target, const WorkSlot &work, bool proxied )
{
	edict_t *edict = g_engine->PEntityOfEntIndex( target );
	if ( edict == nullptr || edict->IsFree( ) )
		return;

	IServerUnknown *unknown = edict->GetUnknown( );
	CBaseEntity *ent = unknown != nullptr ? unknown->GetBaseEntity( ) : nullptr;
	const ClassPlan *plan = ent != nullptr ? GetPlan( unknown->GetNetworkable( )->GetServerClass( ), ent ) : nullptr;
	if ( plan == nullptr || plan->id != work.classId || work.blob.size( ) != plan->blobSize )
		return;

	uint8_t *base = reinterpret_cast<uint8_t *>( ent );
	const std::vector<uint8_t> &skip = proxied ? ProxyMask( plan ) : plan->proxySkip;

	for ( size_t i = 0; i < plan->entries.size( ); ++i )
	{
		if ( proxied && skip[i] != 0 )
			continue;

		memcpy( base + plan->entries[i].offset, &work.blob[plan->prefix[i]], plan->entries[i].size );
	}

	// StateChanged() lives in the server binary; setting the flags directly does
	// the same job and makes the engine reship every prop of this edict.
	edict->m_fStateFlags |= ( FL_EDICT_CHANGED | FL_FULL_EDICT_CHANGED );
}

bool RestoreTick( int32_t tick, bool proxyOnly )
{
	if ( g_engine == nullptr || !BuildStateAtTick( tick ) )
		return false;

	for ( int i = 0; i < kMaxEdicts; ++i )
	{
		const WorkSlot *work = WorkAt( i );
		if ( work == nullptr || !work->valid || Rec( ).ignore[i] != 0 )
			continue;

		// A bound proxy redirects a recorded edict onto a different live one,
		// which is how Lua brings back entities that were deleted since.
		// A stage replay owns nothing but its own clones: writing into the
		// unbound originals is what froze the live world for everybody.
		uint16_t target = Rec( ).proxy[i];
		if ( target == 0xFFFF && proxyOnly )
			continue;

		PushEntity( target == 0xFFFF ? i : target, *work, target != 0xFFFF );
	}

	return true;
}

}
