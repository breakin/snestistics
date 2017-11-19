#pragma once

/*
	This API only encompasses exactly as much as I want to access from scripting.

	TODO: What prefixes do we want for types and functions?
	TODO: What about strings? UTF8?

	Rom
	Replay
		add_breakpoint
		add_breakpoint_range
		Registers
		Memory
		PPU
			WRAM (rename)
	Annotations
	TraceLog
		print_line (indentation from outside)


	Trace (output from emulator).. Rename capture?

	Rewind
*/

// TODO: Are we allowed to use stdint.h in a C-api? Probably not!
#include <stdint.h>

struct Replay;
struct TraceLog;

// Replay
void add_breakpoint(Replay* e, uint32_t pc);
void add_breakpoint_range(Replay* e, uint32_t pc0, uint32_t p1);
uint16_t register_a(Replay *e);
uint32_t register_pc(Replay *e);

// TraceLog
void print_line          (TraceLog *t, const char * const str, int len);