#include "replay.h"

// Replay
void add_breakpoint(Replay* e, uint32_t pc) {
	e->breakpoints.setBit(pc);
}
void add_breakpoint_range(Replay* e, uint32_t pc0, uint32_t p1) {
	CUSTOM_ASSERT(false && "Not implemented yet");
}

uint16_t register_a(Replay *e) {
	return e->regs._A;
}

uint32_t register_pc(Replay *e) {
	return e->regs._PC;
}
