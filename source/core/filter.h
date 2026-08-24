#pragma once

#include "core/chronos.h"

#include "edict.h"
#include "eiface.h"
#include "irecipientfilter.h"
#include "iservernetworkable.h"

#include <cstring>

namespace Chronos
{

// The server's own CRecipientFilter is not linkable from a module, so replay
// ships one. Recipients are found by scanning player edicts rather than assuming
// indices 1..N, which breaks as soon as somebody disconnects.
class BroadcastFilter : public IRecipientFilter
{
public:
	// A stage replay is watched by one player while everybody else keeps
	// playing, so the same filter has to be able to address a single client.
	explicit BroadcastFilter( int only = 0 )
	{
		if ( only > 0 )
		{
			m_recipients.push_back( only );
			return;
		}

		for ( int i = 1; i < 130 && g_engine != nullptr; ++i )
		{
			edict_t *edict = g_engine->PEntityOfEntIndex( i );
			if ( edict == nullptr || edict->IsFree( ) )
				continue;

			const char *name = edict->GetClassName( );
			if ( name != nullptr && strcmp( name, "player" ) == 0 )
				m_recipients.push_back( i );
		}
	}

	bool IsReliable( ) const override { return false; }
	bool IsInitMessage( ) const override { return false; }
	int GetRecipientCount( ) const override { return ( int )m_recipients.size( ); }

	int GetRecipientIndex( int slot ) const override
	{
		return slot >= 0 && slot < ( int )m_recipients.size( ) ? m_recipients[slot] : -1;
	}

private:
	std::vector<int> m_recipients;
};

}
