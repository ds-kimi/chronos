#include "props/propplan.h"

#include "dt_send.h"
#include "capture/recorder.h"
#include "tempents/tempents.h"

#include "detouring/helpers.hpp"
#include "eiface.h"
#include "irecipientfilter.h"

#include <cstring>

namespace Chronos
{

std::deque<TempEntRecord> g_tempEnts;
size_t g_tempEntCap = 40000;

typedef void( __fastcall *PlaybackFn )( void *self, void *edx, IRecipientFilter &filter,
	float delay, const void *sender, const SendTable *table, int classID );

static PlaybackFn s_original = nullptr;
bool g_replayingTempEnts = false;
static void **s_vtable = nullptr;
static size_t s_index = 0;
static std::unordered_map<const SendTable *, ClassPlan *> s_tablePlans;

// Temp entity senders are long-lived singletons in the server binary, so the
// table only has to be flattened once per effect type.
const ClassPlan *PlanForTable( const SendTable *table, const void *sender )
{
	std::unordered_map<const SendTable *, ClassPlan *>::iterator it = s_tablePlans.find( table );
	if ( it != s_tablePlans.end( ) )
		return it->second;

	static std::vector<uint8_t> seen( kMaxPropOffset );
	memset( &seen[0], 0, seen.size( ) );

	ClassPlan *plan = new ClassPlan( );
	plan->netName = table->GetName( );
	FlattenTable( const_cast<SendTable *>( table ), 0, *plan, &seen[0], 0, sender );

	s_tablePlans[table] = plan;
	return plan;
}

static void __fastcall Hook_Playback( void *self, void *edx, IRecipientFilter &filter,
	float delay, const void *sender, const SendTable *table, int classID )
{
	// A stage replay keeps recording while it plays effects back through the
	// same engine call, so without this an old firefight is filed as a new one.
	Recorder &rec = Rec( );
	if ( rec.recording && !g_replayingTempEnts && sender != nullptr && table != nullptr )
	{
		const ClassPlan *plan = PlanForTable( table, sender );
		if ( plan != nullptr && plan->blobSize > 0 && g_tempEnts.size( ) < g_tempEntCap )
		{
			g_tempEnts.push_back( TempEntRecord( ) );
			TempEntRecord &record = g_tempEnts.back( );
			record.tick = rec.lastTick;
			record.classID = classID;
			record.table = table;
			record.sender = const_cast<void *>( sender );
			ScrapeEntity( reinterpret_cast<CBaseEntity *>( record.sender ), plan, record.blob );
		}
	}

	s_original( self, edx, filter, delay, sender, table, classID );
}

bool InstallTempEntHook( )
{
	if ( s_original != nullptr || g_engine == nullptr )
		return s_original != nullptr;

	s_vtable = *reinterpret_cast<void ***>( g_engine );
	Detouring::Member member = Detouring::GetVirtualAddress( s_vtable, 256,
		&IVEngineServer::PlaybackTempEntity );
	if ( !member.IsValid( ) )
		return false;

	s_index = member.index;
	s_original = reinterpret_cast<PlaybackFn>( s_vtable[s_index] );
	return Detouring::ProtectMemory( &s_vtable[s_index], sizeof( void * ), false ) &&
		( s_vtable[s_index] = reinterpret_cast<void *>( &Hook_Playback ) ) != nullptr;
}

void RemoveTempEntHook( )
{
	if ( s_original == nullptr || s_vtable == nullptr )
		return;

	s_vtable[s_index] = reinterpret_cast<void *>( s_original );
	s_original = nullptr;
}

}
