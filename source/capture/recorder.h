#pragma once

#include "core/chronos.h"

namespace Chronos
{

// Every entity is force-keyed once per this many ticks, staggered by edict
// index so the cost spreads evenly and any window of this length is seekable.
static const int kDefaultKeyInterval = 64;

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
	int keyInterval;
	int32_t lastTick;
	bool recording;

	Recorder( ) : slots( kMaxEdicts ), ignore( kMaxEdicts, 0 ), skip( kMaxEdicts, 0 ), proxy( kMaxEdicts, 0xFFFF ), bytes( 0 ),
		byteCap( 512u * 1024u * 1024u ), keyInterval( kDefaultKeyInterval ),
		lastTick( -1 ), recording( false ) { }
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

	WorkSlot( ) : classId( 0xFFFF ), classNameId( 0 ), modelNameId( 0 ), valid( false ) { }
};

Recorder &Rec( );
std::vector<uint8_t> AcquireBuffer( );
void PushFrame( int32_t tick, float curTime, std::vector<uint8_t> &data );
int FrameIndexAtTick( int32_t tick );

void StartRecording( );
void StopRecording( );
void ClearRecording( );
const Frame *FrameAtTick( int32_t tick );

void ScrapeEntity( CBaseEntity *ent, const ClassPlan *plan, std::vector<uint8_t> &dst );
void EmitRecord( std::vector<uint8_t> &out, uint16_t index, const EntitySlot &slot,
	const ClassPlan *plan, const std::vector<uint8_t> &cur, bool key );

void CaptureTick( float curTime );
bool RestoreTick( int32_t tick, bool proxyOnly );
bool BuildStateAtTick( int32_t tick );
const WorkSlot *WorkAt( int index );

}
