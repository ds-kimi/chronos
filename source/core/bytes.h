#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

namespace Chronos
{

template <typename T>
inline void Put( std::vector<uint8_t> &out, T value )
{
	const uint8_t *src = reinterpret_cast<const uint8_t *>( &value );
	out.insert( out.end( ), src, src + sizeof( T ) );
}

inline void PutBytes( std::vector<uint8_t> &out, const void *src, size_t count )
{
	if ( count == 0 )
		return;

	const uint8_t *bytes = static_cast<const uint8_t *>( src );
	out.insert( out.end( ), bytes, bytes + count );
}

template <typename T>
inline T Get( const uint8_t *&cursor )
{
	T value;
	memcpy( &value, cursor, sizeof( T ) );
	cursor += sizeof( T );
	return value;
}

}
