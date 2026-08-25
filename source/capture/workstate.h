#pragma once

#include "capture/recorder.h"

namespace Chronos
{

// The reconstructed world a seek leaves behind. Shared between the frame
// decoder and the seek driver, and by nobody else -- everything outside this
// pair goes through WorkAt.
extern std::vector<WorkSlot> g_work;

// Highest index that has gone valid since the last full reset. Only ever grows
// inside a build, which is what makes it safe as a loop bound: a slot that went
// away is still below it and still gets visited.
extern int g_workMax;

void ApplyFrame( const Frame &frame );

}
