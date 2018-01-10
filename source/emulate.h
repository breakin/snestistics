#pragma once

#include <cstdint>
#include <cstdio>
#include "trace_format.h"
#include "rom_accessor.h"

namespace snestistics {
struct DmaTransfer;

// TODO: Working with this enum is annoying as hell
enum class ProcessorStatusFlag : uint16_t {
	Carry=1,
	Zero = 2,
	IRQ=4,
	Decimal=8,
	IndexFlag=16,
	MemoryFlag=32,
	Overflow=64,
	Negative=128,
	Emulation=256
};

/*
	TODO:
	* Make PC into 8-bit PB and 16-bit adress? Wrapping works that way
	* Scope the flags somehow...?
*/

enum class Events : uint8_t {
	NONE=0,
	NMI=1,
	IRQ=2,
	RESET=4,
	JMP_OR_JML=8,
	JSR_OR_JSL=16,
	RTS_OR_RTL=32,
	BRANCH=64,
	RTI=128,
};

enum class MemoryAccessType {
	PROGRAM_COUNTER_RELATIVE,
	STACK_RELATIVE,
	FETCH_MVN_MVP,
	WRITE_MVN_MVP,
	FETCH_NMI_VECTOR,
	FETCH_IRQ_VECTOR,
	FETCH_INDIRECT,
	RANDOM,
	DMA_READ,
	DMA_WRITE,
};


struct EmulateRegisters;
void execute_dma(EmulateRegisters &regs, uint8_t channels);

struct EmulateRegisters {
	Events event = Events::NONE;

	// Register state
	uint32_t _PC = 0;
	uint16_t _A = 0, _X = 0, _Y = 0, _P = 0, _S = 0, _DP = 0;
	uint8_t _DB = 0;
	uint32_t _WRAM = 0;
	uint32_t _PC_before=0;

	// Register manipulators
	uint32_t PC(uint32_t mask = 0xFFFFFF) const { return _PC & mask; }
	void set_PC(uint32_t value, uint32_t mask = 0xFFFFFF) { _PC = (value & mask) | (_PC & (~mask)); }

	uint16_t A(uint16_t mask = 0xFFFF) const { return _A & mask; }
	void set_A(uint16_t value, uint16_t mask = 0xFFFF) { _A = (value & mask) | (_A & (~mask)); }

	bool P_flag(ProcessorStatusFlag flag) const { return (_P & (uint16_t)flag)!=0; }

	uint16_t P(uint16_t mask = 0xFFFF) const { return _P & mask; }
	void set_P(uint16_t value, uint16_t mask = 0xFFFF) { _P = (value & mask) | (_P & (~mask)); }

	uint16_t X(uint16_t mask = 0xFFFF) { 
		used_X_mask |= mask;
		return _X & mask;
	}
	void set_X(uint16_t value, uint16_t mask = 0xFFFF) { _X = (value & mask) | (_X & (~mask)); }

	uint16_t Y(uint16_t mask = 0xFFFF) { 
		used_Y_mask |= mask;
		return _Y & mask; 
	}
	void set_Y(uint16_t value, uint16_t mask = 0xFFFF) { _Y = (value & mask) | (_Y & (~mask)); }

	uint16_t S(uint16_t mask = 0xFFFF) const { return _S & mask; }
	void set_S(uint16_t value, uint16_t mask = 0xFFFF) { _S = (value & mask) | (_S & (~mask)); }

	uint16_t DP() { used_DP = true; return _DP;	}
	void set_DP(uint16_t value) { _DP = value; }

	uint8_t DB() { used_DB = true; return _DB; }
	void set_DB(uint8_t value) { _DB = value; };

	uint32_t remap(uint32_t address) const {
		uint8_t bank = address >> 16;
		const uint16_t a = address & 0xFFFF;
		if (a < 0x8000) {
			if ((bank >= 0x00 && bank <= 0x3f) || (bank >= 0x80 && bank <= 0xBF)) {
				if (a < 0x2000) {
					bank = 0x7E;
				} else  {
					bank = 0;
				}
			}
		}
		return (bank<<16)|a;
	}

	uint8_t read_byte_PC() {
		uint8_t v = read_byte(_PC, MemoryAccessType::PROGRAM_COUNTER_RELATIVE);
		_PC += 1;
		return v;
	}

	uint16_t read_word_PC() {
		uint16_t v = read_word(_PC, MemoryAccessType::PROGRAM_COUNTER_RELATIVE);
		_PC += 2;
		return v;
	}

	uint32_t read_long_PC() {
		uint32_t v = read_long(_PC, MemoryAccessType::PROGRAM_COUNTER_RELATIVE);
		_PC += 3;
		return v;
	}

	inline uint8_t read_byte(uint32_t address, MemoryAccessType reason) const {
		uint32_t r = remap(address);
		uint8_t result = _memory[r];
		if(_read_function)
			(*_read_function)(_callback_context, address, r, result, 1, reason);
		return result;
	}

	inline uint16_t read_word(uint32_t address, MemoryAccessType reason) const {
		uint32_t r = remap(address);
		uint16_t result = *(uint16_t*)&_memory[r];
		if (_read_function)
			(*_read_function)(_callback_context, address, r, result, 2, reason);
		return result;
	}

	inline uint32_t read_long(uint32_t address, MemoryAccessType reason) const {
		uint32_t r = remap(address);
		uint32_t result = _memory[r];
		result |= _memory[r + 1] << 8;
		result |= _memory[r + 2] << 16;
		if (_read_function)
			(*_read_function)(_callback_context, address, r, result, 3, reason);
		return result;
	}

	inline uint32_t read_dword(uint32_t address, MemoryAccessType reason) const {
		uint32_t r = remap(address);
		uint32_t result = _memory[r];
		result |= _memory[r + 1] << 8;
		result |= _memory[r + 2] << 16;
		result |= _memory[r + 3] << 24;
		if (_read_function)
			(*_read_function)(_callback_context, address, r, result, 4, reason);
		return result;
	}

	inline void write_long(uint32_t address, uint32_t v, MemoryAccessType reason) {
		uint32_t r = remap(address);

		if (r >= 0x002100 && r <= 0x0021FF) {
			special_write_a(r, v&0xFF);
			special_write_a((r+1)&0xFFFF, (v>>8)&0xFF);
			special_write_a((r+2)&0xFFFF, (v>>16)&0xFF);
		}

		if(_write_function)
			(*_write_function)(_callback_context, address, r, v, 3, reason);

		// Don't allow writing to memory mapped registers, SRAM etc...
		// NOTE: Remap remaps to 7E when needed
		uint8_t bank = r>>16;
		if (bank != 0x7E && bank != 0x7F)
			return;

		_memory[r+0] = v &0xFF;
		_memory[r+1] = (v>>8) &0xFF;
		_memory[r+2] = (v>>16) &0xFF;
	}

	inline void write_dword(uint32_t address, uint32_t v, MemoryAccessType reason) {
		uint32_t r = remap(address);

		if(_write_function)
			(*_write_function)(_callback_context, address, r, v, 4, reason);

		// Don't allow writing to memory mapped registers, SRAM etc...
		// NOTE: Remap remaps to 7E when needed
		uint8_t bank = r>>16;
		if (bank != 0x7E && bank != 0x7F)
			return;

		*((uint32_t*)&(_memory[r])) = v;
	}

	inline void write_word(uint32_t address, uint16_t v, MemoryAccessType reason) {
		uint32_t r = remap(address);

		bool sw = false;

		if (r >= 0x002100 && r <= 0x0021FF) {
			special_write_a(r, v&0xFF);
			special_write_a((r+1)&0xFFFF, v>>8);
			sw = true;
		} else if (r >= 0x004200 && r <= 0x0044ff) {
			special_write_b(r, v&0xFF);
			special_write_b((r+1), (v>>8)&0xFF);
			sw = true;
		}

		if(_write_function)
			(*_write_function)(_callback_context, address, r, v, 2, reason);

		// Don't allow writing to memory mapped registers, SRAM etc...
		// NOTE: Remap remaps to 7E when needed
		uint8_t bank = r>>16;
		if (bank != 0x7E && bank != 0x7F && !sw)
			return;

		*((uint16_t*)&(_memory[r])) = v;
	}

	void special_write_a(uint32_t r, uint8_t v) {
		// TODO: This bypasses write_function, making rewind confused... Must make these writes trigger write_function!
		if (r == 0x2180) {
			// TODO: not if written because of dma transfer
			uint32_t target = 0x7E0000+_WRAM;
			CUSTOM_ASSERT((target>>16)==0x7E||(target>>16)==0x7F);
			_memory[target] = v;
			_WRAM++;
			_WRAM &= 0x1ffff;
		} else if (r == 0x2181) {
			// TODO: not if written because of dma transfer
			_WRAM &= 0x1FF00;
			_WRAM |= v;
		} else if (r == 0x2182) {
			// TODO: not if written because of dma transfer
			_WRAM &= 0x100FF;
			_WRAM |= v<<8;
		} else if (r == 0x2183) {
			// TODO: not if written because of dma transfer
			_WRAM &= 0x0FFFF;
			_WRAM |= v<<16;
			_WRAM &= 0x1FFFF;
		}
	}

	void special_write_b(uint32_t r, uint8_t v) {
		if (r == 0x420b) {
			execute_dma(*this, v);
		}
	}

	inline void write_byte(uint32_t address, uint8_t v, MemoryAccessType reason) {
		uint32_t r = remap(address);

		bool sw = false;

		if (r >= 0x002100 && r <= 0x0021FF) {
			special_write_a(r, v);
			sw = true;
		} else if (r >= 0x004200 && r <= 0x0044ff) {
			special_write_b(r, v);
			sw = true;
		}

		if(_write_function)
			(*_write_function)(_callback_context, address, r, v, 1, reason);

		// Don't allow writing to memory mapped registers, SRAM etc...
		// NOTE: Remap remaps to 7E when needed
		uint8_t bank = r>>16;
		if (bank != 0x7E && bank != 0x7F && !sw)
			return;

		_memory[r] = v;
	}

	EmulateRegisters(const snestistics::RomAccessor &rom) {
		// Duplicated bytes for mirrored/shared mem
		// These are reads to memory other than ROM/SRAM (outside what we emulate)
		_memory = new uint8_t[1024*64*256];
		memset(_memory, 0, 1024*64*256);

		// Copy ROM into memory. Everything except ROM/RAM is managed through events
		for (uint32_t b = 0; b <= 0xFF; b++) {
			if (b == 0x7E || b == 0x7F)
				continue;
			for (uint32_t a = 0x8000; a <= 0xFFFF; ++a) {
				uint32_t adr = b * (64*1024) + a;
				_memory[adr] = rom.evalByte(adr);
			}
		}
	}

	~EmulateRegisters() {
		delete[] _memory;
	}

	uint8_t* _memory = nullptr;
	uint32_t _debug_number = 0;
	bool _debug = false;

	typedef void(*dmaFunc)(void* context, const snestistics::DmaTransfer &dma);
	dmaFunc _dma_function = nullptr;

	typedef void(*memoryAccessFunc)(void* context, Pointer location, Pointer remapped_location, uint32_t value, int num_bytes, MemoryAccessType reason);
	void *_callback_context = nullptr;
	memoryAccessFunc _read_function = nullptr;
	memoryAccessFunc _write_function = nullptr;

	void clear_event() {
		_PC_before = INVALID_POINTER;
		event = Events::NONE;
		used_X_mask = 0;
		used_Y_mask = 0;
		used_DB = false;
		used_DP = false;
		indirection_pointer = INVALID_POINTER;
	}

	Pointer indirection_pointer;
	// TODO: Remove these
	uint16_t used_X_mask, used_Y_mask;
	bool used_DB, used_DP;
};

void execute_op(EmulateRegisters &regs);
void execute_nmi(EmulateRegisters &regs);
void execute_irq(EmulateRegisters &regs);
void execute_dma(EmulateRegisters &regs, uint8_t channels);

}