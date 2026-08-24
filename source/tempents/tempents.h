#pragma once

#include "core/chronos.h"

class SendTable;

namespace Chronos
{

// A temp entity carries no state between frames, so it is stored as the bytes
// of the sending object plus the table needed to write them back.
struct TempEntRecord
{
	int32_t tick;
	int32_t classID;
	const SendTable *table;
	void *sender;
	std::vector<uint8_t> blob;
};

const ClassPlan *PlanForTable( const SendTable *table, const void *sender );

bool InstallTempEntHook( );
void RemoveTempEntHook( );

void ClearTempEnts( );
void PruneTempEnts( int32_t firstTick );
extern bool g_replayingTempEnts;

void PlayTempEnts( int32_t from, int32_t to, int only = 0 );

extern std::deque<TempEntRecord> g_tempEnts;
extern size_t g_tempEntCap;

}
