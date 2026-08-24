#include "http/httpserve.h"
#include "lua/luaapi.h"

namespace Chronos
{

LUA_FUNCTION( Lua_StartClipServer )
{
	int port = ( int )LUA->CheckNumber( 1 );
	if ( port < 1024 || port > 65535 )
		LUA->ThrowError( "chronos: clip server port must be between 1024 and 65535" );

	LUA->PushBool( StartClipServer( port ) );
	return 1;
}

LUA_FUNCTION( Lua_StopClipServer )
{
	StopClipServer( );
	return 0;
}

// Takes the WAV bytes Auris produced and returns the id the URL is built from.
LUA_FUNCTION( Lua_AddClip )
{
	unsigned int length = 0;
	const char *data = LUA->GetString( 1, &length );
	if ( data == nullptr || length == 0 )
		return 0;

	LUA->PushNumber( AddClip( reinterpret_cast<const uint8_t *>( data ), length ) );
	return 1;
}

LUA_FUNCTION( Lua_ClearClips )
{
	ClearClips( );
	return 0;
}

LUA_FUNCTION( Lua_SetClipCap )
{
	SetClipCap( ( size_t )( LUA->CheckNumber( 1 ) * 1024.0 * 1024.0 ) );
	return 0;
}

LUA_FUNCTION( Lua_ClipStats )
{
	LUA->CreateTable( );
	LUA->PushBool( ClipServerRunning( ) );
	LUA->SetField( -2, "running" );
	LUA->PushNumber( ClipServerPort( ) );
	LUA->SetField( -2, "port" );
	LUA->PushNumber( ( double )ClipCount( ) );
	LUA->SetField( -2, "clips" );
	LUA->PushNumber( ( double )ClipBytes( ) );
	LUA->SetField( -2, "bytes" );
	LUA->PushNumber( ( double )ClipCap( ) );
	LUA->SetField( -2, "cap" );
	return 1;
}

}
