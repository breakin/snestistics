#pragma once

#include "api.h"
#include "emulate.h"
#include "utils.h"
#include "trace_format.h"
#include "emulate.h" // TODO: Remove, only for EmulateRegisters

struct Options;
class RomAccessor;

#define VERIFY_OPS

struct Replay {
	Replay(const RomAccessor &rom, const char *const capture_file);
	~Replay();
	EmulateRegisters regs;
	LargeBitfield breakpoints;
	Registers temp_registers;
	bool skip_until_nmi(const char * const skip_cache, const uint32_t target_skip_nmi);
	bool next();

private:
	snestistics::TraceEvent _next_event;
	FILE *_trace_file = nullptr;
	#ifdef VERIFY_OPS
		FILE *_trace_helper = nullptr;
	#endif
	uint64_t _accumulated_op_counter = 0;
	uint64_t _next_event_op = 0;
	uint32_t _current_nmi = 0;
public:
	uint64_t _current_op = 0;
	uint64_t _last_after_nmi_offset = 0; // TODO: Make private, only visible since skip cache creation is entangled with trace.cpp
private:

	void read_next_event();
};
