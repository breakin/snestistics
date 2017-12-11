#include "replay.h"
#include <algorithm>
#include "trace_format.h"
#include "utils.h"
#include "options.h"
#include "trace.h"

using namespace snestistics;

namespace  {

	#ifdef VERIFY_OPS
		void debug_P(uint16_t P) {
			printf("    <");
			if (P & (uint16_t)ProcessorStatusFlag::Carry) printf("Carry ");
			if (P & (uint16_t)ProcessorStatusFlag::Zero) printf("Zero ");
			if (P & (uint16_t)ProcessorStatusFlag::IRQ) printf("IRQ ");
			if (P & (uint16_t)ProcessorStatusFlag::Decimal) printf("Decimal ");
			if (P & (uint16_t)ProcessorStatusFlag::IndexFlag) printf("IndexFlag ");
			if (P & (uint16_t)ProcessorStatusFlag::MemoryFlag) printf("MemoryFlag ");
			if (P & (uint16_t)ProcessorStatusFlag::Overflow) printf("Overflow ");
			if (P & (uint16_t)ProcessorStatusFlag::Negative) printf("Negative ");
			if (P & (uint16_t)ProcessorStatusFlag::Emulation) printf("Emulation ");
			printf(">\n");
		}
	#endif

	void check_diff(uint64_t current_op, const snestistics::TraceRegisters &test, const snestistics::TraceRegisters &b, const snestistics::TraceRegisters &a, const EmulateRegisters &regs, uint16_t P_before_op, const char * const str) {
		// b=before, a=after
		bool has_diff = 
			test.pc_bank != (regs._PC>>16) ||
			test.pc_address != (regs._PC & 0xFFFF) ||
			test.X  != regs._X  ||
			test.Y  != regs._Y  ||
			test.A  != regs._A  ||
			test.S  != regs._S  ||
			test.P  != regs._P  ||
			test.DB != regs._DB ||
			test.wram_bank != (regs._WRAM >> 16) ||
			test.wram_address != (regs._WRAM & 0xFFFF) ||
			test.DP != regs._DP;

		if (has_diff) {
			printf("Found diff %s at op %d\n", str, (uint32_t)current_op); // TODO: How to print 64-bit cross platform?
			printf("P  %04X %04X %04X\n", b.P,  a.P, regs._P);
			debug_P(b.P);
			debug_P(a.P);
			debug_P(P_before_op);
			debug_P(regs._P);
			printf("A    %04X %04X %04X\n", b.A,  a.A,  regs._A);
			printf("X    %04X %04X %04X\n", b.X,  a.X,  regs._X);
			printf("Y    %04X %04X %04X\n", b.Y,  a.Y,  regs._Y);
			printf("S    %04X %04X %04X\n", b.S,  a.S,  regs._S);
			printf("DB   %02X %02X %02X\n", b.DB, a.DB, regs._DB);
			printf("DP   %04X %04X %04X\n", b.DP, a.DP, regs._DP);
			printf("PC   %02X%04X %02X%04X %06X\n", b.pc_bank, b.pc_address, a.pc_bank, a.pc_address, regs._PC);
			printf("WRAM %02X%04X %02X%04X %06X\n", b.wram_bank, b.wram_address, a.wram_bank, a.wram_address, regs._WRAM);
			exit(1);
		}
	}
}
/*
	Replay a session recorded in an emulator.
	It supports all CPU ops and cpu->cpu DMA. The stream from the emulator says when NMI, RESET and IRQ happens.
	Reads from SRAM/MMIO is backed from the recorded stream such that only the CPU needs to be emulated.
	Thus timing and cycles is something this emulator replayer does not care about.

	TODO: Skipping should either live 100% in trace.cpp or 100% here. Figure out which!
*/

// Given a "recording" of a emulation session, iterate op for op

bool Replay::skip_until_nmi(const char * const skip_cache, const uint32_t target_skip_nmi) {

	EmulateRegisters &regs = this->regs;

	// TODO: If we are running without skip-cache we can't go back! Fix, or require skip cache...

	// Use skip file if it exists!
	// Note: Currently we can only skip to the point right AFTER an NMI so we do one nmi too little here
	// Using do/while here is a bit bananas but helps with indentation :)
	do {
		FILE *f = fopen(skip_cache, "rb"); // The 0 here must be wrong if we do many
		if (!f)
			break;

		snestistics::TraceSkipHeader header;
		fread(&header, sizeof(header), 1, f);

		if (header.version != CURRENT_CACHE_VERSION) {
			printf("Skip cache has wrong version, not using\n");
			fclose(f);
			break;
		}

		const uint32_t nmi_per_skip = header.nmi_per_skip;
		const uint32_t size_per_skip = sizeof(snestistics::TraceSkip)+128*1024;

		// Since skip information is to get AFTER an nmi just happened, we skip to the frame before
		// We use emulation to get to the before NMI case
		uint32_t skip = (target_skip_nmi) / nmi_per_skip;
		const fpos_t j = skip * size_per_skip + sizeof(snestistics::TraceSkipHeader);
		fsetpos(f, &j);

		snestistics::TraceSkip msg;
		fread(&msg, sizeof(msg), 1, f);
		assert(msg.nmi <= target_skip_nmi);

		// TODO: Don't leak STATE implementation like this
		regs._A  = msg.regs.A;
		regs._X  = msg.regs.X;
		regs._Y  = msg.regs.Y;
		regs._S  = msg.regs.S;
		regs._DB = msg.regs.DB;
		regs._DP = msg.regs.DP;
		regs._PC = (msg.regs.pc_bank<<16)|msg.regs.pc_address;
		regs._P  = msg.regs.P;
		regs._WRAM = (msg.regs.wram_bank<<16)|msg.regs.wram_address;

		for (int bank=0; bank<256; bank++)
			memset(&regs._memory[bank*64*1024], 0, 0x8000);

		fread(&regs._memory[0x7E0000], 1024*64, 1, f);
		fread(&regs._memory[0x7F0000], 1024*64, 1, f);

		assert(msg.nmi == skip * nmi_per_skip);

		_current_nmi = msg.nmi;
		fpos_t pos;
		pos = (int64_t)msg.seek_offset_trace_file;
		fsetpos(_trace_file, &pos);

		_current_op = msg.current_op;
		_accumulated_op_counter = msg.current_op-1;

		read_next_event();

		#ifdef VERIFY_OPS
			if (_trace_helper) {
				fpos_t helper_pos = _current_op * (sizeof(snestistics::HelperType)+sizeof(snestistics::HelperOp));
				fsetpos(_trace_helper, &helper_pos);
			}
		#endif
		fclose(f);
	} while (false);

	CUSTOM_ASSERT(_current_nmi <= target_skip_nmi);

	if (_current_nmi == target_skip_nmi) // We had a skip for the exact right nmi
		return true;

	// TODO: We might have to re-emulate from start if there was no skip and we want to go to same frame or less
	// Skips are not for every nmi so make sure we reach the right one
	while (true) {
		if (target_skip_nmi == _current_nmi && _current_op == _next_event_op && _next_event.type == TraceEventType::EVENT_NMI)
			return true;
		if (!next())
			return false;
	}
}

bool Replay::next() {
	
	EmulateRegisters &regs = this->regs;

	regs.clear_event();

	Events do_event = Events::NONE;

	// Are we at the next event yet?
	while (_current_op == _next_event_op) {
		// Now perform the next_op event!
		if (_next_event.type == TraceEventType::EVENT_READ_BYTE) {
			TraceEventReadByte rb;
			size_t num_read = fread(&rb, sizeof(rb), 1, _trace_file);
			CUSTOM_ASSERT(num_read == 1);
			uint32_t r = regs.remap(rb.adress);
			if (regs._debug)
				printf("External write %06X %02X op %d\n", r, rb.value, (int32_t)_current_op);
			regs._memory[r] = rb.value; // Use function to this become traceable from regs
			read_next_event();
		} else if (_next_event.type == TraceEventType::EVENT_READ_WORD) {
			TraceEventReadWord rb;
			size_t num_read = fread(&rb, sizeof(rb), 1, _trace_file);
			CUSTOM_ASSERT(num_read == 1);
			uint32_t shifted_bank = rb.adress & 0x00FF0000;
			uint16_t l0 = rb.adress&0xFFFF;
			uint16_t l1 = l0+1;
			uint32_t r0 = regs.remap(shifted_bank|l0);
			uint32_t r1 = regs.remap(shifted_bank|l1);
			if (regs._debug)
				printf("External write %06X,%06X=%04X op %d\n", r0, r1, rb.value, (int32_t)_current_op);
			regs._memory[r0] = rb.value&0xFF; // Use function to this become traceable from regs
			regs._memory[r1] = rb.value>>8; // Use function to this become traceable from regs
			read_next_event();
		} else if (_next_event.type == TraceEventType::EVENT_RESET) {
			do_event = Events::RESET;
			break;
		} else if (_next_event.type == TraceEventType::EVENT_NMI) {
			do_event = Events::NMI;
			break;
		} else if (_next_event.type == TraceEventType::EVENT_IRQ) {
			do_event = Events::IRQ;
			break;
		} else if (_next_event.type == TraceEventType::EVENT_FINISHED) {
			return false;
		}
		// We've consumed the current next event so pull in a new one (if there is one)
	}

	// Note that NMI, RESET and IRQ are treated as ops so we can validate regs before/after in debug mode
	#ifdef VERIFY_OPS
		HelperOp h;
		if (_trace_helper) {
			HelperType type;
			fread(&type, sizeof(type), 1, _trace_helper);
			CUSTOM_ASSERT(type == HelperType::HELPER_OP);
			fread(&h, sizeof(h), 1, _trace_helper);
			CUSTOM_ASSERT(h.current_op == _current_op);
		}
	#endif

	const uint16_t P_before_op = regs._P;
	const uint32_t PC_before_op = regs._PC;

	#ifdef VERIFY_OPS
		if (_trace_helper && do_event != Events::RESET) {
			check_diff(_current_op, h.registers_before, h.registers_before, h.registers_after, regs, P_before_op, "BEFORE");
		}
	#endif

	regs.event = do_event;
	regs._debug_number = (uint32_t)_current_op;
	regs._debug = false;
	regs._PC_before = PC_before_op;

	if (do_event == Events::RESET) {
		TraceEventReset e;
		fread(&e, sizeof(e), 1, _trace_file);

		// Clear out everything but ROM
		for (int bank=0; bank<256; bank++)
			memset(&regs._memory[bank*64*1024], 0, 0x8000);

		regs.set_PC((e.regs_after.pc_bank<<16)|e.regs_after.pc_address);
		regs.set_P (e.regs_after.P);
		regs.set_A (e.regs_after.A); 
		regs.set_X (e.regs_after.X);
		regs.set_Y (e.regs_after.Y);
		regs.set_S (e.regs_after.S);
		regs.set_DB(e.regs_after.DB);
		regs.set_DP(e.regs_after.DP);
		regs._WRAM = (e.regs_after.wram_bank<<16)|e.regs_after.wram_address;

		// Read RAM to support save games (NOTE: reads another 128k)
		fread(&regs._memory[0x7E0000], 64*1024, 1, _trace_file);
		fread(&regs._memory[0x7F0000], 64*1024, 1, _trace_file);

		// We treat the RESET as a NMI (since it starts the _first_ frame, before first nmi)
		// But we don't increase current_nmi here since we really wanted it to start at -1
		{
			fpos_t pos = 0;
			int res = fgetpos(_trace_file, &pos);
			assert(res == 0);
			_last_after_nmi_offset = pos;
		}
		read_next_event();
	} else if (do_event == Events::NMI) {
		execute_nmi(regs);
		_current_nmi++;
		{
			fpos_t pos = 0;
			int res = fgetpos(_trace_file, &pos);
			assert(res == 0);
			_last_after_nmi_offset = pos;
		}
		read_next_event();
	} else if (do_event == Events::IRQ) {
		execute_irq(regs);
		read_next_event();
	} else {
		execute_op(regs);
	}

	#ifdef VERIFY_OPS
		if (_trace_helper) {
			check_diff(_current_op, h.registers_after, h.registers_before, h.registers_after, regs, P_before_op, "AFTER");
		}
	#endif
	_current_op++;
	return true;
}

void Replay::read_next_event() {
	// NOTE: We only read header here, actual event is delayed until consumed
	size_t num_read = fread(&_next_event, sizeof(_next_event), 1, _trace_file);
	CUSTOM_ASSERT(num_read == 1);

	_accumulated_op_counter += _next_event.op_counter_delta;
	_next_event_op = _accumulated_op_counter;
}

Replay::Replay(const RomAccessor &rom, const char *const capture_file) : regs(rom), breakpoints(1024*64*256) {
	_trace_file = fopen(capture_file, "rb");
	#ifdef VERIFY_OPS
	{
		StringBuilder sb;
		sb.add(capture_file);
		sb.add("_helper");
		_trace_helper = fopen(sb.c_str(), "rb"); // NOTE: If this fails that is OK
	}
	#endif
	read_next_event();
}

Replay::~Replay() {
	fclose(_trace_file);
	#ifdef VERIFY_OPS
		if (_trace_helper) {
			fclose(_trace_helper);
			_trace_helper = nullptr;
		}
	#endif
}

void replay_add_breakpoint(Replay* replay, uint32_t pc) {
	if (pc < 1024*64*256)
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
