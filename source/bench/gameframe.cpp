#include "bench/counters.h"
#include "core/chronos.h"

#include "detouring/helpers.hpp"
#include "eiface.h"

namespace Chronos
{

// The only exact answer to "how much tick is this eating" comes from timing the
// whole server frame: srcds sleeps to hold tickrate, so wall time between ticks
// says nothing about how much of the budget was actually spent working.
// Same vtable-patch shape as the PlaybackTempEntity hook in tempents.cpp.
//
// __fastcall with a dummy edx parameter is a 32-bit MSVC-only quirk: old MSVC
// thiscall passes `this` in ECX with a spare EDX slot, and matching that shape
// with __fastcall is how a free function can stand in for a member function
// pointer on that ABI. Neither the register nor the calling convention exists
// on the Itanium ABI (Linux) or on x86-64 (both compilers), where `this` is
// simply the first argument.
#if defined( COMPILER_VC ) && defined( ARCHITECTURE_X86 )

typedef void ( __fastcall *GameFrameFn )( void *self, void *edx, bool simulating );

static GameFrameFn s_original = nullptr;
static void **s_vtable = nullptr;
static size_t s_index = 0;

static void __fastcall Hook_GameFrame( void *self, void *edx, bool simulating )
{
	int64_t entry = QpcNow( );
	if ( g_bench.lastFrameQpc != 0 )
		g_bench.phase[BP_GAMEGAP].Add( QpcToMs( entry - g_bench.lastFrameQpc ) );

	g_bench.lastFrameQpc = entry;

	s_original( self, edx, simulating );

	g_bench.phase[BP_GAMEFRAME].Add( QpcToMs( QpcNow( ) - entry ) );
}

#else

typedef void ( *GameFrameFn )( void *self, bool simulating );

static GameFrameFn s_original = nullptr;
static void **s_vtable = nullptr;
static size_t s_index = 0;

static void Hook_GameFrame( void *self, bool simulating )
{
	int64_t entry = QpcNow( );
	if ( g_bench.lastFrameQpc != 0 )
		g_bench.phase[BP_GAMEGAP].Add( QpcToMs( entry - g_bench.lastFrameQpc ) );

	g_bench.lastFrameQpc = entry;

	s_original( self, simulating );

	g_bench.phase[BP_GAMEFRAME].Add( QpcToMs( QpcNow( ) - entry ) );
}

#endif

bool InstallGameFrameHook( )
{
	if ( s_original != nullptr || g_serverDLL == nullptr )
		return s_original != nullptr;

	s_vtable = *reinterpret_cast<void ***>( g_serverDLL );
	Detouring::Member member = Detouring::GetVirtualAddress( s_vtable, 256,
		&IServerGameDLL::GameFrame );
	if ( !member.IsValid( ) )
		return false;

	s_index = member.index;
	s_original = reinterpret_cast<GameFrameFn>( s_vtable[s_index] );
	return Detouring::ProtectMemory( &s_vtable[s_index], sizeof( void * ), false ) &&
		( s_vtable[s_index] = reinterpret_cast<void *>( &Hook_GameFrame ) ) != nullptr;
}

void RemoveGameFrameHook( )
{
	if ( s_original == nullptr || s_vtable == nullptr )
		return;

	s_vtable[s_index] = reinterpret_cast<void *>( s_original );
	s_original = nullptr;
	g_bench.lastFrameQpc = 0;
}

}
