#include "props/propplan.h"
#include "capture/recorder.h"

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
const std::vector<uint8_t> &ProxyMask( const ClassPlan *plan )
{
	if ( plan->proxySkip.size( ) != plan->entries.size( ) )
	{
		plan->proxySkip.assign( plan->entries.size( ), 0 );
		for ( size_t i = 0; i < plan->names.size( ); ++i )
			plan->proxySkip[i] = ProxySafe( plan->names[i] ) ? 0 : 1;
	}

	return plan->proxySkip;
}

}
