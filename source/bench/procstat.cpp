#include "bench/clock.h"
#include "bench/procstat.h"

#if defined( _WIN32 )
	#include <psapi.h>
	#include <tlhelp32.h>
#else
	#include <cstdio>
	#include <cstring>
	#include <dirent.h>
	#include <unistd.h>
	#include <sys/sysinfo.h>
#endif

namespace Chronos
{

#if defined( _WIN32 )

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

#else

// One entry per thread, so counting directory entries is counting threads. This
// is the same thing /proc/self/status's Threads: field reports, read directly
// instead of parsing another file.
static uint32_t CountThreads( )
{
	DIR *dir = opendir( "/proc/self/task" );
	if ( dir == nullptr )
		return 0;

	uint32_t count = 0;
	while ( readdir( dir ) != nullptr )
		++count;

	closedir( dir );

	// "." and ".." are not threads.
	return count > 2 ? count - 2 : 0;
}

// /proc/self/status carries VmRSS and VmPeak in kB lines; this is the same data
// psapi's PROCESS_MEMORY_COUNTERS gives on Windows, just line-oriented text here.
static void SampleMemory( ProcSnapshot &out )
{
	FILE *f = fopen( "/proc/self/status", "r" );
	if ( f != nullptr )
	{
		char line[256];
		while ( fgets( line, sizeof( line ), f ) != nullptr )
		{
			unsigned long long kb;
			if ( sscanf( line, "VmRSS: %llu kB", &kb ) == 1 )
				out.workingSet = kb * 1024ull;
			else if ( sscanf( line, "VmPeak: %llu kB", &kb ) == 1 )
				out.peakWorkingSet = kb * 1024ull;
			else if ( sscanf( line, "VmData: %llu kB", &kb ) == 1 )
				out.privateBytes = kb * 1024ull;
		}

		fclose( f );
	}

	struct sysinfo info;
	if ( sysinfo( &info ) == 0 )
	{
		out.sysAvail = ( uint64_t )info.freeram * ( uint64_t )info.mem_unit;
		out.sysTotal = ( uint64_t )info.totalram * ( uint64_t )info.mem_unit;
	}
}

// /proc/self/stat's utime/stime fields are in clock ticks (usually 100/s), the
// same cumulative-since-start shape as Windows' GetProcessTimes.
static void SampleCpuTimes( ProcSnapshot &out )
{
	FILE *f = fopen( "/proc/self/stat", "r" );
	if ( f == nullptr )
		return;

	char buf[1024];
	size_t got = fread( buf, 1, sizeof( buf ) - 1, f );
	fclose( f );
	buf[got] = '\0';

	// Field 2 is the executable name in parens and may itself contain spaces or
	// parens, so the safe split point is the last ')' rather than a token index.
	char *afterName = strrchr( buf, ')' );
	if ( afterName == nullptr )
		return;

	unsigned long long utime = 0, stime = 0;
	// Fields after the name, 1-indexed from state (3): state(3) ... utime(14) stime(15).
	int rc = sscanf( afterName + 2,
		"%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu",
		&utime, &stime );
	if ( rc != 2 )
		return;

	long ticksPerSec = sysconf( _SC_CLK_TCK );
	if ( ticksPerSec <= 0 )
		ticksPerSec = 100;

	out.userMs = ( double )utime * 1000.0 / ( double )ticksPerSec;
	out.kernelMs = ( double )stime * 1000.0 / ( double )ticksPerSec;
}

void SampleProcess( ProcSnapshot &out )
{
	out = ProcSnapshot( );
	out.wallMs = QpcToMs( QpcNow( ) );

	SampleCpuTimes( out );
	SampleMemory( out );
	out.threads = CountThreads( );
	out.valid = true;
}

#endif

}
