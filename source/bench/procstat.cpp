#include "bench/clock.h"
#include "bench/procstat.h"

#include <psapi.h>
#include <tlhelp32.h>

namespace Chronos
{

static double FileTimeMs( const FILETIME &value )
{
	ULARGE_INTEGER wide;
	wide.LowPart = value.dwLowDateTime;
	wide.HighPart = value.dwHighDateTime;

	// FILETIME counts 100ns intervals.
	return ( double )wide.QuadPart / 10000.0;
}

// Chronos runs a clip-server thread of its own alongside whatever the engine
// keeps, so a thread count that quietly climbs is a leak worth seeing.
static uint32_t CountThreads( )
{
	HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
	if ( snapshot == INVALID_HANDLE_VALUE )
		return 0;

	THREADENTRY32 entry;
	entry.dwSize = sizeof( entry );
	DWORD self = GetCurrentProcessId( );
	uint32_t count = 0;

	if ( Thread32First( snapshot, &entry ) )
	{
		do
		{
			if ( entry.th32OwnerProcessID == self )
				++count;
		} while ( Thread32Next( snapshot, &entry ) );
	}

	CloseHandle( snapshot );
	return count;
}

static void SampleMemory( ProcSnapshot &out )
{
	PROCESS_MEMORY_COUNTERS_EX counters;
	counters.cb = sizeof( counters );
	if ( GetProcessMemoryInfo( GetCurrentProcess( ),
		reinterpret_cast<PROCESS_MEMORY_COUNTERS *>( &counters ), sizeof( counters ) ) )
	{
		out.workingSet = counters.WorkingSetSize;
		out.privateBytes = counters.PrivateUsage;
		out.peakWorkingSet = counters.PeakWorkingSetSize;
	}

	MEMORYSTATUSEX status;
	status.dwLength = sizeof( status );
	if ( GlobalMemoryStatusEx( &status ) )
	{
		out.sysAvail = status.ullAvailPhys;
		out.sysTotal = status.ullTotalPhys;
	}
}

void SampleProcess( ProcSnapshot &out )
{
	out = ProcSnapshot( );
	out.wallMs = QpcToMs( QpcNow( ) );

	FILETIME created, exited, kernel, user;
	if ( GetProcessTimes( GetCurrentProcess( ), &created, &exited, &kernel, &user ) )
	{
		out.kernelMs = FileTimeMs( kernel );
		out.userMs = FileTimeMs( user );
	}

	SampleMemory( out );
	out.threads = CountThreads( );
	out.valid = true;
}

}
