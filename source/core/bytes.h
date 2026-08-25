#pragma once

#include <cstdint>
#include <cstring>
#include <emmintrin.h>
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

// memcmp is an out-of-line call for any length the compiler cannot see, and the
// capture scan's average run is 25 bytes -- at that size the call and its size
// dispatch are most of what a compare costs. Inlining it as SSE2 word loads
// turns each one into a handful of instructions with no call at all. Long runs
// still go to memcmp, where its dispatch is worth paying for.
inline bool SameBytes( const uint8_t *a, const uint8_t *b, uint32_t size )
{
	if ( size > 256 )
		return memcmp( a, b, size ) == 0;

	uint32_t i = 0;
	for ( ; i + 16 <= size; i += 16 )
	{
		__m128i same = _mm_cmpeq_epi8( _mm_loadu_si128( ( const __m128i * )( a + i ) ),
			_mm_loadu_si128( ( const __m128i * )( b + i ) ) );
		if ( _mm_movemask_epi8( same ) != 0xFFFF )
			return false;
	}

	for ( ; i + 4 <= size; i += 4 )
	{
		if ( *( const uint32_t * )( a + i ) != *( const uint32_t * )( b + i ) )
			return false;
	}

	for ( ; i < size; ++i )
	{
		if ( a[i] != b[i] )
			return false;
	}

	return true;
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
