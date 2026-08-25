#pragma once

#include "core/chronos.h"

class IServerUnknown;

namespace Chronos
{

// Every entity is force-keyed once per this many ticks, staggered by edict
// index so the cost spreads evenly and any window of this length is seekable.
static const int kDefaultKeyInterval = 64;

// How often the capture scan walks every edict slot instead of stopping at the
// high-water mark. Cheap at this rate, and it is what lets the bound discover
// indices that opened above it.
static const int kSweepInterval = 8;

struct Recorder
{
	std::deque<Frame> frames;
	std::vector<std::vector<uint8_t> > pool;
	std::vector<EntitySlot> slots;
	std::vector<uint8_t> ignore;
	std::vector<uint8_t> skip;
	std::vector<uint16_t> proxy;
	size_t bytes;
	size_t byteCap;

	// Size of the last frame emitted. Frames are near enough the same size tick
	// to tick that this is what the next buffer should start at: the ring keeps
	// every frame it takes, so nothing can be recycled until eviction, and a
	// buffer starting empty pays a dozen reallocations on the way back up.
	size_t lastFrameBytes;
	int keyInterval;

	// keyInterval - 1 when the interval is a power of two, so the stagger test
	// is a mask instead of a division for every live entity every tick. Zero
	// means it is not, and the division stands.
	int keyMask;
	int32_t lastTick;

	// Highest edict index ever seen live. The scan stops above this and the
	// engine's own count, so an empty server does not walk all 8192 slots.
	int highWater;
	bool recording;

	Recorder( ) : slots( kMaxEdicts ), ignore( kMaxEdicts, 0 ), skip( kMaxEdicts, 0 ), proxy( kMaxEdicts, 0xFFFF ), bytes( 0 ),
		byteCap( 512u * 1024u * 1024u ), lastFrameBytes( 0 ), keyInterval( kDefaultKeyInterval ),
		keyMask( kDefaultKeyInterval - 1 ), lastTick( -1 ), highWater( 0 ),
		recording( false ) { }
};

// Reconstructed state of one edict while seeking; separate from the recorder's
// shadow slots so seeking never corrupts an in-progress recording.
struct WorkSlot
{
	std::vector<uint8_t> blob;
	uint16_t classId;
	uint16_t classNameId;
	uint16_t modelNameId;
	bool valid;

	// Birth tick carried by the keyframe: which occupant of this index the
	// rebuilt state describes. See EntitySlot::born.
	int32_t born;

	WorkSlot( ) : classId( 0xFFFF ), classNameId( 0 ), modelNameId( 0 ), valid( false ),
		born( kBornUnknown ) { }
};

inline bool KeyDue( const Recorder &rec, int32_t tick, int index )
{
	int mixed = tick + index;
	return rec.keyMask != 0 ? ( mixed & rec.keyMask ) == 0 : ( mixed % rec.keyInterval ) == 0;
}

// A power-of-two interval is what lets KeyDue use a mask, so the mask is
// derived here rather than tested on every entity.
inline void SetKeyInterval( Recorder &rec, int interval )
{
	rec.keyInterval = interval;
	rec.keyMask = ( interval & ( interval - 1 ) ) == 0 ? interval - 1 : 0;
}

Recorder &Rec( );
std::vector<uint8_t> AcquireBuffer( );
void PushFrame( int32_t tick, float curTime, std::vector<uint8_t> &data );
int FrameIndexAtTick( int32_t tick );

void StartRecording( );
void StopRecording( );
void ClearRecording( );
const Frame *FrameAtTick( int32_t tick );

void ScrapeEntity( CBaseEntity *ent, const ClassPlan *plan, std::vector<uint8_t> &dst );
void EmitHeader( std::vector<uint8_t> &out, uint16_t index, uint16_t classId, uint8_t type );
void EmitGone( std::vector<uint8_t> &out, uint16_t index, EntitySlot &slot );
void RefreshIdentity( edict_t *edict, EntitySlot &slot );

void EmitKey( std::vector<uint8_t> &out, uint16_t index, EntitySlot &slot,
	const ClassPlan *plan, CBaseEntity *ent );
void EmitDelta( std::vector<uint8_t> &out, uint16_t index, EntitySlot &slot,
	const ClassPlan *plan, const uint8_t *base );

const std::vector<uint8_t> &ProxyMask( const ClassPlan *plan );
const ClassPlan *PushPlan( int target, IServerUnknown *unknown, CBaseEntity *ent );
void ResetPushPlans( );

void CaptureTick( float curTime );
bool RestoreTick( int32_t tick, bool proxyOnly );
bool BuildStateAtTick( int32_t tick );
void InvalidateRebuild( );
const WorkSlot *WorkAt( int index );
int WorkHighWater( );

}
