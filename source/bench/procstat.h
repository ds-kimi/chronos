#pragma once

#include <cstdint>

namespace Chronos
{

// One reading of what the srcds process and the machine are doing. CPU is
// stored as cumulative process time; a rate only means something as the
// difference between two snapshots.
struct ProcSnapshot
{
	double wallMs;
	double userMs;
	double kernelMs;
	uint64_t workingSet;
	uint64_t privateBytes;
	uint64_t peakWorkingSet;
	uint64_t sysAvail;
	uint64_t sysTotal;
	uint32_t threads;
	bool valid;

	ProcSnapshot( ) : wallMs( 0.0 ), userMs( 0.0 ), kernelMs( 0.0 ), workingSet( 0 ),
		privateBytes( 0 ), peakWorkingSet( 0 ), sysAvail( 0 ), sysTotal( 0 ),
		threads( 0 ), valid( false ) { }
};

void SampleProcess( ProcSnapshot &out );

}
