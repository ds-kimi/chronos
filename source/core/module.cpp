#include "lua/luaapi.h"
#include "props/propplan.h"
#include "capture/recorder.h"
#include "http/httpserve.h"
#include "tempents/tempents.h"

using namespace GarrysMod::Lua;

GMOD_MODULE_OPEN( )
{
	if ( !Chronos::InitInterfaces( ) )
		LUA->ThrowError( "chronos: failed to resolve engine interfaces" );

	Chronos::ResetStrings( );
	Chronos::InstallTempEntHook( );
	Chronos::RegisterAPI( LUA );
	return 0;
}

GMOD_MODULE_CLOSE( )
{
	Chronos::StopClipServer( );
	Chronos::ClearClips( );
	Chronos::RemoveTempEntHook( );
	Chronos::StopRecording( );
	Chronos::ClearRecording( );
	Chronos::ClearTempEnts( );
	Chronos::ResetPlans( );
	return 0;
}
