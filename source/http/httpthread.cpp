#include "http/httpserve.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>

#if defined( _WIN32 )
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#define chronos_snprintf _snprintf
#else
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <unistd.h>

	typedef int SOCKET;
	#define INVALID_SOCKET ( -1 )
	#define closesocket close
	#define chronos_snprintf snprintf
#endif

namespace Chronos
{

extern std::unordered_map<uint32_t, std::vector<uint8_t> > s_clips;
extern std::mutex s_lock;
extern std::thread s_thread;
extern std::atomic<bool> s_running;
extern SOCKET s_listen;
extern int s_port;

// The only thing parsed out of a request is the clip id. Anything else about
// the request is ignored, so a malformed or hostile line cannot reach further.
static uint32_t ParseClipId( const char *request, size_t length )
{
	const char *prefix = "GET /clip/";
	size_t prefixLength = strlen( prefix );
	if ( length < prefixLength || strncmp( request, prefix, prefixLength ) != 0 )
		return 0;

	uint32_t id = 0;
	for ( size_t i = prefixLength; i < length && request[i] >= '0' && request[i] <= '9'; ++i )
	{
		id = id * 10 + ( uint32_t )( request[i] - '0' );
		if ( id > 100000000u )
			return 0;
	}

	return id;
}

static void SendClip( SOCKET client, uint32_t id )
{
	std::vector<uint8_t> clip;
	{
		std::lock_guard<std::mutex> guard( s_lock );
		std::unordered_map<uint32_t, std::vector<uint8_t> >::iterator it = s_clips.find( id );
		if ( it != s_clips.end( ) )
			clip = it->second;
	}

	char header[256];
	if ( clip.empty( ) )
	{
		int length = chronos_snprintf( header, sizeof( header ),
			"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n" );
		send( client, header, length, 0 );
		return;
	}

	int length = chronos_snprintf( header, sizeof( header ),
		"HTTP/1.1 200 OK\r\nContent-Type: audio/wav\r\nContent-Length: %u\r\n"
		"Access-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n",
		( unsigned )clip.size( ) );
	send( client, header, length, 0 );

	size_t sent = 0;
	while ( sent < clip.size( ) )
	{
		int wrote = send( client, ( const char * )&clip[sent], ( int )( clip.size( ) - sent ), 0 );
		if ( wrote <= 0 )
			return;

		sent += ( size_t )wrote;
	}
}

// Reads until the blank line that ends the request headers. A single recv was
// enough for a request that arrived in one segment and wrong for one that did
// not: the leftover bytes stayed unread, and closing a socket with unread data
// in its receive buffer makes Windows send RST instead of FIN, which throws
// away whatever of the clip was still queued. That is what cut clips off part
// way through, and it depended on segment timing, so it looked random.
static int ReadRequest( SOCKET client, char *request, int capacity )
{
	int received = 0;

	while ( received < capacity - 1 )
	{
		int got = recv( client, request + received, capacity - 1 - received, 0 );
		if ( got <= 0 )
			break;

		received += got;
		request[received] = '\0';

		if ( strstr( request, "\r\n\r\n" ) != nullptr )
			break;
	}

	return received;
}

// Announces end of stream, then drains whatever the client still had in flight
// so the close is graceful and the body is delivered whole.
static void CloseGracefully( SOCKET client )
{
#if defined( _WIN32 )
	shutdown( client, SD_SEND );
#else
	shutdown( client, SHUT_WR );
#endif

	char drain[512];
	while ( recv( client, drain, sizeof( drain ), 0 ) > 0 )
		;

	closesocket( client );
}

// One connection at a time, served and closed. Clips are a few hundred KB and
// only a handful of reviewers ever pull them, so a thread pool would be noise.
static void ServeLoop( )
{
	while ( s_running.load( ) )
	{
		SOCKET client = accept( s_listen, nullptr, nullptr );
		if ( client == INVALID_SOCKET )
			continue;

		// A send timeout matters as much as a receive one here: the loop serves
		// connections in turn, so one stalled reader would otherwise park every
		// other clip behind it for as long as it felt like.
#if defined( _WIN32 )
		DWORD timeout = 3000;
		setsockopt( client, SOL_SOCKET, SO_RCVTIMEO, ( const char * )&timeout, sizeof( timeout ) );
		setsockopt( client, SOL_SOCKET, SO_SNDTIMEO, ( const char * )&timeout, sizeof( timeout ) );
#else
		struct timeval timeout;
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		setsockopt( client, SOL_SOCKET, SO_RCVTIMEO, ( const char * )&timeout, sizeof( timeout ) );
		setsockopt( client, SOL_SOCKET, SO_SNDTIMEO, ( const char * )&timeout, sizeof( timeout ) );
#endif

		char request[2048];
		int received = ReadRequest( client, request, sizeof( request ) );
		if ( received > 0 )
			SendClip( client, ParseClipId( request, ( size_t )received ) );

		CloseGracefully( client );
	}
}

void StartServeThread( )
{
	s_thread = std::thread( ServeLoop );
}

}
