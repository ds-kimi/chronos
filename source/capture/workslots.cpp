#include "capture/recorder.h"
#include "capture/workstate.h"

namespace Chronos
{

std::vector<WorkSlot> g_work( kMaxEdicts );
int g_workMax = 0;

const WorkSlot *WorkAt( int index )
{
	return index >= 0 && index < ( int )g_work.size( ) ? &g_work[index] : nullptr;
}

int WorkHighWater( )
{
	return g_workMax;
}

}
