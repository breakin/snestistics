#pragma once

#include "api.h"
#include "emulate.h"
#include "emulate_replay.h"
#include "romaccessor.h"
#include "utils.h"

struct Replay {
	Replay(const RomAccessor &rom, const char *const capture_file) : regs(rom), replay(capture_file), breakpoints(1024*64*256) {}
	EmulateRegisters regs;
	snestistics::EmulateReplay replay;
	LargeBitfield breakpoints;
	Registers temp_registers;
};
