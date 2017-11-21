#include "replay.h"
#include <algorithm>

void replay_add_breakpoint(Replay* replay, uint32_t pc) {
	if (pc >= 0 && pc < 1024*64*256)
		replay->breakpoints.setBit(pc);
}
void replay_add_breakpoint_range(Replay* replay, uint32_t p0, uint32_t p1) {
	if (p1 < p0)
		return;

	p1 = std::min(p1, 1024*64*256U-1);

	// TODO: Add a function in breakpoints to set a range, could be much faster
	for (uint32_t p = p0; p <= p1; p++) {
		replay->breakpoints.setBit(p);
	}	
}

Registers* replay_registers(Replay *replay) {
	replay->temp_registers.pc = replay->regs._PC;
	replay->temp_registers.a = replay->regs._A;
	replay->temp_registers.x = replay->regs._X;
	replay->temp_registers.y = replay->regs._Y;
	replay->temp_registers.p = replay->regs._P;
	replay->temp_registers.s = replay->regs._S;
	replay->temp_registers.dp = replay->regs._DP;
	replay->temp_registers.db = replay->regs._DB;
	return &replay->temp_registers;
}

uint8_t replay_read_byte(Replay *replay, uint32_t address) {
	uint8_t* memory = replay->regs._memory;
	return memory[address+0];
}
uint16_t replay_read_word(Replay *replay, uint32_t address) {
	uint8_t* memory = replay->regs._memory;
	return memory[address+0]+(memory[address+1]<<8);
}
uint32_t replay_read_long(Replay *replay, uint32_t address) {
	uint8_t* memory = replay->regs._memory;
	return memory[address+0]+(memory[address+1]<<8)+(memory[address+2]<<16);
}
