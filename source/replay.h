#pragma once

#include "api.h"
#include "emulate.h"
#include "utils.h"
#include "trace_format.h"
#include "emulate.h" // TODO: Remove, only for EmulateRegisters

struct Options;
namespace snestistics {
	class RomAccessor;
}

#define VERIFY_OPS

struct Replay {
	Replay(const snestistics::RomAccessor &rom, const char *const trace_file);
	~Replay();
	snestistics::LargeBitfield breakpoints;
	snestistics::EmulateRegisters regs; // TODO: Make replay use temp_registers instead of regs...
	Registers temp_registers;
	bool skip_until_nmi(const uint32_t target_skip_nmi);
	bool next();
private:
	std::string _trace_file_name;
	snestistics::TraceEvent _next_event;
	snestistics::BigFile _trace_file;
	#ifdef VERIFY_OPS
	snestistics::BigFile _trace_helper;
	#endif
	uint64_t _accumulated_op_counter = 0;
	uint64_t _next_event_op = 0;
	uint32_t _current_nmi = 0;
public:
	uint64_t _current_op = 0;
	uint64_t _last_after_nmi_offset = 0; // TODO: Make private, only visible since skip cache creation is entangled with trace.cpp
	uint8_t _trace_content_guid[8];
private:
	void read_next_event();
};
