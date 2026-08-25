#include "bench/counters.h"
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

// __fastcall with a dummy edx parameter is a 32-bit MSVC-only quirk: old MSVC
// thiscall passes `this` in ECX with a spare EDX slot, and matching that shape
// with __fastcall is how a free function can stand in for a member function
// pointer on that ABI. Neither the register nor the calling convention exists
// on the Itanium ABI (Linux) or on x86-64 (both compilers), where `this` is
// simply the first argument.
#if defined( COMPILER_VC ) && defined( ARCHITECTURE_X86 )
typedef void( __fastcall *PlaybackFn )( void *self, void *edx, IRecipientFilter &filter,
	float delay, const void *sender, const SendTable *table, int classID );
#else
typedef void( *PlaybackFn )( void *self, IRecipientFilter &filter,
	float delay, const void *sender, const SendTable *table, int classID );
#endif

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
	FinalizePlan( *plan );

	s_tablePlans[table] = plan;
	return plan;
}

#if defined( COMPILER_VC ) && defined( ARCHITECTURE_X86 )
static void __fastcall Hook_Playback( void *self, void *edx, IRecipientFilter &filter,
	float delay, const void *sender, const SendTable *table, int classID )
#else
static void Hook_Playback( void *self, IRecipientFilter &filter,
	float delay, const void *sender, const SendTable *table, int classID )
#endif
{
	// A stage replay keeps recording while it plays effects back through the
	// same engine call, so without this an old firefight is filed as a new one.
	int64_t began = g_bench.on ? QpcNow( ) : 0;
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

	// Only our own scrape is charged here; the engine call below is its cost.
	if ( began != 0 )
		g_bench.phase[BP_TEMPENT].Add( QpcToMs( QpcNow( ) - began ) );

#if defined( COMPILER_VC ) && defined( ARCHITECTURE_X86 )
	s_original( self, edx, filter, delay, sender, table, classID );
#else
	s_original( self, filter, delay, sender, table, classID );
#endif
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
