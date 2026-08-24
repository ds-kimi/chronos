#pragma once

#include <cstdint>
#include <cstddef>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

class CBaseEntity;
class IServerGameDLL;
class IVEngineServer;
class SendProp;
class SendTable;
class ServerClass;
struct edict_t;

namespace Chronos
{

// Source caps networked entities at 8192; anything above has no edict and is
// therefore invisible to clients, so it is irrelevant for replay.
static const int kMaxEdicts = 8192;

// Sentinel written after the last entity record of a frame.
static const uint16_t kEndOfFrame = 0xFFFF;

enum RecordType : uint8_t
{
	REC_DELTA = 0,
	REC_KEY = 1,
	REC_GONE = 2
};

// One capturable member of a networked class, resolved from its SendTable.
struct PlanEntry
{
	uint16_t offset;
	uint16_t size;
	uint8_t type;
};

// Flattened capture plan shared by every entity of one ServerClass.
struct ClassPlan
{
	std::vector<PlanEntry> entries;
	std::vector<std::string> names;
	std::vector<uint32_t> prefix;
	uint32_t blobSize;
	uint16_t id;
	const char *netName;

	// Resolved on first use by name; -2 means "not looked up yet", -1 "absent".
	mutable int originSlot;
	mutable int anglesSlot;

	// One byte per entry, built on the first restore into a stand-in: which
	// props must not be written onto an entity that is standing in for another.
	mutable std::vector<uint8_t> proxySkip;

	ClassPlan( ) : blobSize( 0 ), id( 0 ), netName( nullptr ),
		originSlot( -2 ), anglesSlot( -2 ) { }
};

// Last captured full state of one edict, used to emit deltas.
struct EntitySlot
{
	std::vector<uint8_t> blob;
	uint16_t classId;
	uint16_t classNameId;
	uint16_t modelNameId;
	bool live;
	bool needKey;

	EntitySlot() : classId( 0xFFFF ), classNameId( 0 ), modelNameId( 0 ),
		live( false ), needKey( true ) { }
};

struct Frame
{
	int32_t tick;
	float curTime;
	std::vector<uint8_t> data;
};

bool InitInterfaces( );

extern IVEngineServer *g_engine;
extern IServerGameDLL *g_serverDLL;

}
