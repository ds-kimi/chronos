#pragma once

#include <string>

namespace Chronos
{

void Fmt( std::string &out, const char *format, ... );
void AppendPhases( std::string &out, double frameTotal );
void AppendBudget( std::string &out, double tickIntervalMs );
void AppendTickrate( std::string &out );
void AppendMemory( std::string &out );
void AppendCpu( std::string &out );
void AppendCapture( std::string &out );

}
