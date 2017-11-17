#pragma once

#include <vector>

/*
	The reason scripting is exposed like this is that I don't want details
	of the scripting language to leak out to the application in general.
*/

struct Scripting;
struct EmulateRegisters;
class LargeBitfield;

Scripting *create_scripting(const char * const script_filename);
void destroy_scripting(Scripting *scripting);

void scripting_trace_log_init(Scripting *scripting, LargeBitfield &result);
void scripting_trace_log_parameter_printer(Scripting *scripting, EmulateRegisters *emulator_state);
