#pragma once

#include "core/chronos.h"

namespace Chronos
{

// Clips are handed out by opaque id rather than filename: the URL is the only
// thing a client ever sees, and nothing it sends is used to reach the disk.
bool StartClipServer( int port );
void StopClipServer( );
bool ClipServerRunning( );
int ClipServerPort( );

uint32_t AddClip( const uint8_t *data, size_t size );
void SetClipCap( size_t bytes );
void ClearClips( );
size_t ClipCount( );
size_t ClipBytes( );
size_t ClipCap( );

}
