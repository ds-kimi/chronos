#include "http/httpserve.h"

#include <atomic>
#include <mutex>
#include <thread>

#include <winsock2.h>
#include <ws2tcpip.h>

namespace Chronos
{

std::unordered_map<uint32_t, std::vector<uint8_t> > s_clips;
std::mutex s_lock;
std::thread s_thread;
std::atomic<bool> s_running( false );
SOCKET s_listen = INVALID_SOCKET;
int s_port = 0;
static std::deque<uint32_t> s_order;
static uint32_t s_nextId = 1;
static size_t s_bytes = 0;
static size_t s_byteCap = 256u * 1024u * 1024u;

void StartServeThread( );

// Clips are evicted oldest first once the budget is blown. Unlike the snapshot
// ring these are never pruned by frame, so without a cap a long session with
// heavy voice would grow until the server ran out of memory.
uint32_t AddClip( const uint8_t *data, size_t size )
{
	if ( data == nullptr || size == 0 )
		return 0;

	std::lock_guard<std::mutex> guard( s_lock );
	uint32_t id = s_nextId++;
	s_clips[id].assign( data, data + size );
	s_order.push_back( id );
	s_bytes += size;

	while ( s_bytes > s_byteCap && s_order.size( ) > 1 )
	{
		uint32_t oldest = s_order.front( );
		s_order.pop_front( );

		std::unordered_map<uint32_t, std::vector<uint8_t> >::iterator it = s_clips.find( oldest );
		if ( it != s_clips.end( ) )
		{
			s_bytes -= it->second.size( );
			s_clips.erase( it );
		}
	}

	return id;
}

void SetClipCap( size_t bytes )
{
	std::lock_guard<std::mutex> guard( s_lock );
	s_byteCap = bytes < 1048576 ? 1048576 : bytes;
}

void ClearClips( )
{
	std::lock_guard<std::mutex> guard( s_lock );
	s_clips.clear( );
	s_order.clear( );
	s_bytes = 0;
}

size_t ClipCount( )
{
	std::lock_guard<std::mutex> guard( s_lock );
	return s_clips.size( );
}

size_t ClipBytes( )
{
	std::lock_guard<std::mutex> guard( s_lock );
	return s_bytes;
}

// Binds loopback-free so clients on the LAN or internet can reach it, but only
// ever answers /clip/<id>. Closing the listening socket is what unblocks the
// accept() in the serve thread; without it, unload would hang the server.
bool StartClipServer( int port )
{
	if ( s_running.load( ) )
		return true;

	WSADATA wsa;
	if ( WSAStartup( MAKEWORD( 2, 2 ), &wsa ) != 0 )
		return false;

	s_listen = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( s_listen == INVALID_SOCKET )
		return false;

	BOOL reuse = TRUE;
	setsockopt( s_listen, SOL_SOCKET, SO_REUSEADDR, ( const char * )&reuse, sizeof( reuse ) );

	sockaddr_in address;
	memset( &address, 0, sizeof( address ) );
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( ( unsigned short )port );

	if ( bind( s_listen, ( sockaddr * )&address, sizeof( address ) ) != 0 ||
		listen( s_listen, SOMAXCONN ) != 0 )
	{
		closesocket( s_listen );
		s_listen = INVALID_SOCKET;
		return false;
	}

	s_port = port;
	s_running.store( true );
	StartServeThread( );
	return true;
}

void StopClipServer( )
{
	if ( !s_running.load( ) )
		return;

	s_running.store( false );
	closesocket( s_listen );
	s_listen = INVALID_SOCKET;

	if ( s_thread.joinable( ) )
		s_thread.join( );

	WSACleanup( );
}

size_t ClipCap( )
{
	std::lock_guard<std::mutex> guard( s_lock );
	return s_byteCap;
}

bool ClipServerRunning( )
{
	return s_running.load( );
}

int ClipServerPort( )
{
	return s_port;
}

}
