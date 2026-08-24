#include "props/propplan.h"
#include "props/proxytest.h"

#include "dt_send.h"

#include <cstring>

namespace Chronos
{

static std::unordered_map<void *, int> s_verdicts;

// Calling an unknown function pointer is only acceptable because the answer is
// cached per proxy, the recipients buffer is a real zeroed CBitVec-sized block,
// and a faulting proxy is caught rather than taking the server down.
static int TestProxy( SendProp *prop, const void *structBase )
{
	static unsigned char recipients[512];
	memset( recipients, 0, sizeof( recipients ) );

	const unsigned char *expected =
		reinterpret_cast<const unsigned char *>( structBase ) + prop->GetOffset( );
	const void *result = nullptr;

#if defined _WIN32
	__try
	{
		result = prop->GetDataTableProxyFn( )( prop, structBase, expected,
			reinterpret_cast<CSendProxyRecipients *>( recipients ), 1 );
	}
	__except ( 1 )
	{
		return 0;
	}
#else
	return 0;
#endif

	return result == expected ? 1 : 0;
}

bool IsInlineProxy( SendProp *prop, const void *structBase )
{
	void *fn = reinterpret_cast<void *>( prop->GetDataTableProxyFn( ) );
	if ( fn == nullptr || structBase == nullptr )
		return false;

	if ( fn == g_identityProxy )
		return true;

	std::unordered_map<void *, int>::iterator it = s_verdicts.find( fn );
	if ( it != s_verdicts.end( ) )
		return it->second != 0;

	int verdict = TestProxy( prop, structBase );
	s_verdicts[fn] = verdict;
	return verdict != 0;
}

}
