#include "core/chronos.h"

#include "GarrysMod/InterfacePointers.hpp"
#include "eiface.h"

namespace Chronos
{

IVEngineServer *g_engine = nullptr;
IServerGameDLL *g_serverDLL = nullptr;

bool InitInterfaces( )
{
	g_engine = InterfacePointers::VEngineServer( );
	g_serverDLL = InterfacePointers::ServerGameDLL( );
	return g_engine != nullptr && g_serverDLL != nullptr;
}

}
