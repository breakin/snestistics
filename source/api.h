#pragma once

/*
	This API only encompasses exactly as much as I want to access from scripting.

	TODO: What prefixes do we want for types and functions?
	TODO: What about strings? UTF8?
	QUESTION:
		What about namespaces? Currently Replay and ReportWriter is in global namespace but most other symbols in snestistics ns

	DESIGN:
		I would love If Registers was actually part of Replay.
		We could pimpl away stuff instead of having it completely opaque.
*/

// TODO: Are we allowed to use stdint.h in a C-api?
#include <stdint.h>

struct Replay;
struct ReportWriter;

struct Registers {
	uint32_t pc;
	uint16_t a, x, y, p, s, dp;
	uint8_t db;
};

// Replay
void replay_set_breakpoint(Replay* replay, uint32_t pc);
void replay_set_breakpoint_range(Replay* replay, uint32_t pc0, uint32_t p1);

Registers *replay_registers(Replay *replay);

/*
	NOTE: All of these will eventually read with bank-wrapping
*/
uint8_t  replay_read_byte(Replay *replay, uint32_t address);
uint16_t replay_read_word(Replay *replay, uint32_t address);
uint32_t replay_read_long(Replay *replay, uint32_t address);

// Report
void report_writer_print(ReportWriter *report_writer, const char * const str, uint32_t len);