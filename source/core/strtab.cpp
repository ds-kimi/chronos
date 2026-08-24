#include "props/propplan.h"

namespace Chronos
{

static std::vector<std::string> s_strings;
static std::unordered_map<std::string, uint16_t> s_stringIds;

// Class names and model paths repeat across every keyframe, so they are stored
// once and referenced by index for the lifetime of the recording session.
uint16_t InternString( const char *str )
{
	std::string key = str != nullptr ? str : "";

	std::unordered_map<std::string, uint16_t>::iterator it = s_stringIds.find( key );
	if ( it != s_stringIds.end( ) )
		return it->second;

	if ( s_strings.size( ) >= 0xFFFF )
		return 0;

	uint16_t id = ( uint16_t )s_strings.size( );
	s_strings.push_back( key );
	s_stringIds[key] = id;
	return id;
}

const char *StringById( uint16_t id )
{
	return id < s_strings.size( ) ? s_strings[id].c_str( ) : "";
}

void ResetStrings( )
{
	s_strings.clear( );
	s_stringIds.clear( );
	InternString( "" );
}

}
