#include "emulate.h"
#include "utils.h"
#include <cstdio>
#include "trace.h"

/*
	NOTE: In snesx9 all indexed mode index by X.W or Y.W, but that is because XH/YH are zero when in 8-bit-mode.
	      Thus indexing is by 8 or 16-bit X/Y based on index mode.
*/

namespace {

#include "emulate_generate/optables.h"

inline void jump(EmulateRegisters &regs, uint32_t target, uint32_t mask, Events ev) {
	CUSTOM_ASSERT(regs.event == Events::NONE);
	regs.event = ev;
	regs.set_PC(target, mask);
}

inline void write_value(EmulateRegisters &regs, uint32_t pointer, uint16_t value, bool wide, MemoryAccessType reason) {
	if (wide) {
		regs.write_word(pointer, value, reason);
	} else {
		regs.write_byte(pointer, (uint8_t)value, reason);
	}
}

inline void push_byte_stack(EmulateRegisters &regs, uint8_t v) {
	uint16_t s = regs.S();
	uint32_t stack_ptr = 0x7E0000 | s;
	regs.write_byte(stack_ptr, v, MemoryAccessType::STACK_RELATIVE);
	regs.set_S(s-1);
}

inline void push_word_stack(EmulateRegisters &regs, uint16_t v) {
	uint16_t s = regs.S();
	uint32_t stack_ptr = 0x7E0000 | (s-1);
	regs.write_word(stack_ptr, v, MemoryAccessType::STACK_RELATIVE);
	regs.set_S(s-2);
}

inline void push_long_stack(EmulateRegisters &regs, uint32_t v) {
	uint16_t s = regs.S();
	uint32_t stack_ptr = 0x7E0000 | (s-2);
	regs.write_long(stack_ptr, v, MemoryAccessType::STACK_RELATIVE);
	regs.set_S(s-3);
}

inline void push_dword_stack(EmulateRegisters &regs, uint32_t v) {
	uint16_t s = regs.S();
	uint32_t stack_ptr = 0x7E0000 | (s-3);
	regs.write_dword(stack_ptr, v, MemoryAccessType::STACK_RELATIVE);
	regs.set_S(s-4);
}

inline uint8_t pop_byte_stack(EmulateRegisters &regs) {
	uint16_t s = regs.S() + 1;
	regs.set_S(s);
	uint32_t stack_ptr = 0x7E0000 | s;
	uint8_t v= regs.read_byte(stack_ptr, MemoryAccessType::STACK_RELATIVE);
	return v;
}

inline uint16_t pop_word_stack(EmulateRegisters &regs) {
	uint16_t s = regs.S() + 2;
	regs.set_S(s);
	uint32_t stack_ptr = 0x7E0000 | (s-1);
	uint16_t v= regs.read_word(stack_ptr, MemoryAccessType::STACK_RELATIVE);
	return v;
}

inline uint32_t pop_long_stack(EmulateRegisters &regs) {
	uint16_t s = regs.S() + 3;
	regs.set_S(s);
	uint32_t stack_ptr = 0x7E0000 | (s-2);
	uint32_t v= regs.read_long(stack_ptr, MemoryAccessType::STACK_RELATIVE);
	return v;
}

inline uint32_t pop_dword_stack(EmulateRegisters &regs) {
	uint16_t s = regs.S() + 4;
	regs.set_S(s);
	uint32_t stack_ptr = 0x7E0000 | (s-3);
	uint32_t v= regs.read_dword(stack_ptr, MemoryAccessType::STACK_RELATIVE);
	return v;
}

inline uint16_t set_nz_flags(EmulateRegisters &regs, const uint16_t value, const bool wide) {
	uint16_t mask = (uint16_t)ProcessorStatusFlag::Negative|(uint16_t)ProcessorStatusFlag::Zero;
	uint16_t set_value = value == 0 ? (uint16_t)ProcessorStatusFlag::Zero : 0;
	if (wide) {
		if (value >= 0x8000) set_value |= (uint16_t)ProcessorStatusFlag::Negative;
	} else {
		if (value >= 0x80) set_value |= (uint16_t)ProcessorStatusFlag::Negative;
	}
	regs.set_P(set_value, mask);
	return value;
}

inline void op_adc_16(EmulateRegisters &regs, const uint16_t value) {
	const uint16_t flags_affected = 
		(uint16_t)ProcessorStatusFlag::Carry|
		(uint16_t)ProcessorStatusFlag::Negative|
		(uint16_t)ProcessorStatusFlag::Zero|
		(uint16_t)ProcessorStatusFlag::Overflow;
	uint16_t flags_value = 0;

	const uint16_t a16 = regs.A();

	const uint32_t carry = regs.P_flag(ProcessorStatusFlag::Carry) == 0 ? 0 : 1;
	const uint32_t sum = a16 + value + carry;

	if ((sum&0xFFFF)==0) flags_value |= (uint16_t)ProcessorStatusFlag::Zero;
	if (sum >= 0x10000) flags_value |= (uint16_t)ProcessorStatusFlag::Carry;
	if (sum & 0x8000 ) flags_value |= (uint16_t)ProcessorStatusFlag::Negative;

	// NOTE: For a signed overflow to happen both A and value must have the same sign
	if (((~(a16 ^ value)) & (value ^ (uint16_t)sum)) & 0x8000)
		flags_value |= (uint16_t)ProcessorStatusFlag::Overflow;

	regs.set_A((uint16_t)sum);
	regs.set_P(flags_value, flags_affected);
}

inline void op_adc_8(EmulateRegisters &regs, const uint8_t value) {
	const uint16_t flags_affected = 
		(uint16_t)ProcessorStatusFlag::Carry|
		(uint16_t)ProcessorStatusFlag::Negative|
		(uint16_t)ProcessorStatusFlag::Zero|
		(uint16_t)ProcessorStatusFlag::Overflow;

	uint16_t flags_value = 0;

	const uint8_t a8 = (uint8_t)regs.A(0xFF);

	const uint16_t carry = regs.P_flag(ProcessorStatusFlag::Carry) == 0 ? 0 : 1;
	const uint16_t sum = a8 + value + carry;

	if ((sum&0xFF) == 0) flags_value |= (uint16_t)ProcessorStatusFlag::Zero;
	if (sum >= 0x100) flags_value |= (uint16_t)ProcessorStatusFlag::Carry;
	if (sum & 0x80 ) flags_value |= (uint16_t)ProcessorStatusFlag::Negative;

	if (((~(a8 ^ value)) & (value ^ (uint8_t)sum)) & 0x80)
		flags_value |= (uint16_t)ProcessorStatusFlag::Overflow;

	regs.set_A((uint8_t)sum, 0x00FF);
	regs.set_P(flags_value, flags_affected);
}

uint32_t bcd_to_uint(uint16_t value) {
	uint32_t v0 = (value       ) & 0xF;
	uint32_t v1 = (value>>(4*1)) & 0xF;
	uint32_t v2 = (value>>(4*2)) & 0xF;
	uint32_t v3 = (value>>(4*3)) & 0xF;
	return v0+v1*10+v2*100+v3*1000;
}

uint16_t uint_to_bcd(uint16_t value) {
	uint16_t result = 0;
	for (int k=0; k<4; k++) {
		result |= (value % 10)<<(k*4);
		value /= 10;
	}
	return result;
}

// For now assumes ADC in decimal mode is rare
inline void op_adc_decimal_16(EmulateRegisters &regs, const uint16_t value) {
	const uint16_t a16 = regs.A();

	uint16_t v_uint = bcd_to_uint(value), a_uint=bcd_to_uint(a16);
	const uint32_t carry = regs.P_flag(ProcessorStatusFlag::Carry) == 0 ? 0 : 1;
	uint32_t sum = a_uint + v_uint + carry;

	uint16_t bcd_result = uint_to_bcd(sum);

	const uint16_t flags_affected = 
		(uint16_t)ProcessorStatusFlag::Carry|
		(uint16_t)ProcessorStatusFlag::Negative|
		(uint16_t)ProcessorStatusFlag::Zero|
		(uint16_t)ProcessorStatusFlag::Overflow;

	uint16_t flags_value = 0;

	if ((bcd_result&0xFFFF) == 0) flags_value |= (uint16_t)ProcessorStatusFlag::Zero;
	if (bcd_result & 0x8000) flags_value |= (uint16_t)ProcessorStatusFlag::Negative;
	if (sum >= 0x10000) flags_value |= (uint16_t)ProcessorStatusFlag::Carry; // Not quite right

	// TODO: Set overflow

	regs.set_A(bcd_result);
	regs.set_P(flags_value, flags_affected);
}

inline void op_adc_decimal_8(EmulateRegisters &regs, const uint8_t value) {
	const uint16_t a8 = regs.A(0x00FF);

	uint16_t v_uint = bcd_to_uint(value), a_uint=bcd_to_uint(a8);
	const uint32_t carry = regs.P_flag(ProcessorStatusFlag::Carry) == 0 ? 0 : 1;
	uint32_t sum = a_uint + v_uint + carry;

	uint16_t bcd_result = uint_to_bcd(sum);

	const uint16_t flags_affected = 
		(uint16_t)ProcessorStatusFlag::Carry|
		(uint16_t)ProcessorStatusFlag::Negative|
		(uint16_t)ProcessorStatusFlag::Zero|
		(uint16_t)ProcessorStatusFlag::Overflow;

	uint16_t flags_value = 0;

	if ((bcd_result&0xFF) == 0) flags_value |= (uint16_t)ProcessorStatusFlag::Zero;
	if (bcd_result & 0x80) flags_value |= (uint16_t)ProcessorStatusFlag::Negative;
	if (sum >= 0x100) flags_value |= (uint16_t)ProcessorStatusFlag::Carry; // Not quite right

	// TODO: Set overflow

	regs.set_A(bcd_result, 0x00FF);
	regs.set_P(flags_value, flags_affected);
}

inline uint16_t op_inc(EmulateRegisters &regs, const uint16_t value, const bool wide) {
	// TODO-IMPROVE: Do not take regs, return mask instead
	uint16_t result = 0;
	if (wide) {
		result = value + 1;
	} else {
		uint8_t v = (uint8_t)value;
		v++;
		result = v;
	}
	set_nz_flags(regs, result, wide);
	return result;
}

inline uint16_t op_dec(EmulateRegisters &regs, const uint16_t value, const bool wide) {
	// TODO-IMPROVE: Do not take regs, return mask instead
	uint16_t result = 0;
	if (wide) {
		result = value - 1;
	} else {
		uint8_t v = (uint8_t)value;
		v--;
		result = v;
	}
	set_nz_flags(regs, result, wide);
	return result;
}

inline bool index16(EmulateRegisters &r) { return r.P((uint16_t)ProcessorStatusFlag::IndexFlag) == 0; }
inline bool memory16(EmulateRegisters &r) { return r.P((uint16_t)ProcessorStatusFlag::MemoryFlag) == 0; }

inline bool transfer(EmulateRegisters &regs, const Operation op) {
	if (op == Operation::TAX) {
		if (index16(regs)) {
			regs.set_X(set_nz_flags(regs, regs.A(), true));
		} else {
			regs.set_X(set_nz_flags(regs, regs.A(0xFF), false), 0xFF);
		}		
	} else if (op == Operation::TAY) {
		if (index16(regs)) {
			regs.set_Y(set_nz_flags(regs, regs.A(), true));
		} else {
			regs.set_Y(set_nz_flags(regs, regs.A(0xFF), false), 0xFF);
		}
	} else if (op == Operation::TCD) {
		regs.set_DP(set_nz_flags(regs, regs.A(), true));
	} else if (op == Operation::TDC) {
		regs.set_A(set_nz_flags(regs, regs.DP(), true));
	} else if (op == Operation::TCS) {
			regs.set_S(set_nz_flags(regs, regs.A(), true));
	} else if (op == Operation::TSC) {
		regs.set_A(set_nz_flags(regs, regs.S(), true));
	} else if (op == Operation::TXS) {
		regs.set_S(set_nz_flags(regs, regs.X(), true));
	} else if (op == Operation::TXY) {
		bool wide = index16(regs);
		regs.set_Y(set_nz_flags(regs, regs.X(wide ? 0xFFFF : 0xFF), wide), wide ? 0xFFFF : 0xFF);
	} else if (op == Operation::TYX) {
		bool wide = index16(regs);
		regs.set_X(set_nz_flags(regs, regs.Y(wide ? 0xFFFF : 0xFF), wide), wide ? 0xFFFF : 0xFF);
	} else if (op == Operation::TSX) {
		if (index16(regs)) {
			regs.set_X(regs.S());
			set_nz_flags(regs, regs.S(), true);
		} else {
			regs.set_X(regs.S(0xFF), 0xFF);
			set_nz_flags(regs, regs.S(0xFF), false);
		}
	} else if (op == Operation::TXA) {
		if (memory16(regs)) {
			regs.set_A(set_nz_flags(regs, regs.X(), true));
		} else {
			regs.set_A(set_nz_flags(regs, regs.X(0xFF), false), 0xFF);
		}		
	} else if (op == Operation::TYA) {
		if (memory16(regs)) {
			regs.set_A(set_nz_flags(regs, regs.Y(), true));
		} else {
			regs.set_A(set_nz_flags(regs, regs.Y(0xFF), false), 0xFF);
		}		
	} else {
		return false;
	}
	return true;
}

inline bool push_pull(EmulateRegisters &regs, const Operation op) {
	// Pushes does not set any flags
	// Most pulls set nz

	if (op == Operation::PHA) {
		if (memory16(regs)) {
			push_word_stack(regs, regs.A());
		} else {
			push_byte_stack(regs, (uint8_t)regs.A(0xFF));
		}
	} else if (op == Operation::PHB) {
		push_byte_stack(regs, regs.DB());
	} else if (op == Operation::PHD) {
		push_word_stack(regs, regs.DP());
	} else if (op == Operation::PHK) {
		push_byte_stack(regs, regs.PC(0xFF0000)>>16);
	} else if (op == Operation::PHP) {
		push_byte_stack(regs, (uint8_t)regs.P(0xFF));	
	} else if (op == Operation::PHX) {
		if (index16(regs)) {
			push_word_stack(regs, regs.X());
		} else {
			push_byte_stack(regs, (uint8_t)regs.X(0xFF));
		}
	} else if (op == Operation::PHY) {
		if (index16(regs)) {
			push_word_stack(regs, regs.Y());
		} else {
			push_byte_stack(regs, (uint8_t)regs.Y(0xFF));
		}
	} else if (op == Operation::PLA) {
		bool wide = memory16(regs);
		uint16_t value = wide ? pop_word_stack(regs) : pop_byte_stack(regs);
		regs.set_A(set_nz_flags(regs, value, wide), wide ? 0xFFFF : 0xFF);
	} else if (op == Operation::PLB) {
		uint8_t value = pop_byte_stack(regs);
		regs.set_DB((uint8_t)set_nz_flags(regs, value, false));
	} else if (op == Operation::PLD) {
		uint16_t value = pop_word_stack(regs);
		regs.set_DP(set_nz_flags(regs, value, true));
	} else if (op == Operation::PLP) {
		uint8_t value = pop_byte_stack(regs);
		regs.set_P(set_nz_flags(regs, value, false), 0xFF);
		// TODO: Can PLP affect emulation bit?
		if (!index16(regs)) {
			regs.set_X(0, 0xFF00);
			regs.set_Y(0, 0xFF00);
		}
	} else if (op == Operation::PLX) {
		if (index16(regs)) {
			uint16_t value = pop_word_stack(regs);
			regs.set_X(set_nz_flags(regs, value, true));
		} else {
			uint8_t value = pop_byte_stack(regs);
			regs.set_X(set_nz_flags(regs, value, false), 0xFF);
		}
	} else if (op == Operation::PLY) {
		if (index16(regs)) {
			uint16_t value = pop_word_stack(regs);
			regs.set_Y(set_nz_flags(regs, value, true));
		} else {
			uint8_t value = pop_byte_stack(regs);
			regs.set_Y(set_nz_flags(regs, value, false), 0xFF);
		}
	} else {
		// Not push-pull
		return false;
	}
	// We handled it!
	return true;
}
}

void execute_op(EmulateRegisters &regs) {

	const uint32_t pc_before = regs._PC;
	const uint8_t opcode_ = regs.read_byte_PC();

	const OpCode info = op_codes[opcode_];

	// Load operand ============================================================================================
	uint16_t value = 0;
	bool value_resolved = false;

	// TODO: Move use_db into an info-flag just like load_operand?
	const bool use_db = info.op != Operation::JMP && info.op != Operation::JSR && info.op != Operation::JSL && info.op != Operation::JML;

	// Is the instruction in wide mode? Table knows which flag (if any to query)
	// TODO: Some ops might ALWAYS be wide and some might always be narrow... need to set wide for them here?
	const bool wide = info.size == InstructionSize::WIDE || (info.size == InstructionSize::INDEX && index16(regs)) || (info.size == InstructionSize::MEMORY && memory16(regs));
	const uint16_t wide_mask = wide ? 0xFFFF : 0xFF;

	if (regs._debug) {
		printf("Op %s (%s) %02X at %06X i%d m%d %s %s %s%s %d\n", Operation_names[(int)info.op], mnemonic_names[opcode_], opcode_, pc_before, regs.P((uint16_t)ProcessorStatusFlag::IndexFlag)==0?16:8, regs.P((uint16_t)ProcessorStatusFlag::MemoryFlag)==0?16:8, Operand_names[(int)info.mode], InstructionSize_names[(int)info.size], wide?"wide":"small", info.load_operand?" load":"", regs._debug_number);
	}

	MemoryAccessType memory_mode = MemoryAccessType::RANDOM;

	uint32_t pointer = INVALID_POINTER;
	if (info.mode == Operand::MANUAL) {
		// This is both IMPLIED as well as one-off modes that I don't want to treat generally
	} else if (info.mode == Operand::ACCUMULATOR) {
		value = regs.A(wide_mask);
		value_resolved = true;
	//} else if (info.mode == Operand::IMMEDIATE_8) {
	//	value = regs.read_byte_PC();
	//	value_resolved = true;
	} else if (info.mode == Operand::IMMEDIATE_MEMORY) {
		value = memory16(regs) ? regs.read_word_PC() : regs.read_byte_PC();
		value_resolved = true;
	} else if (info.mode == Operand::IMMEDIATE_INDEX) {	
		value = index16(regs) ? regs.read_word_PC() : regs.read_byte_PC();
		value_resolved = true;
	} else if (info.mode == Operand::ABSOLUTE) {
		pointer = regs.read_word_PC();
		if (use_db) pointer |= regs.DB() << 16;
	} else if (info.mode == Operand::ABSOLUTE_INDEXED_X) {
		uint16_t index_mask = index16(regs) ? 0xFFFF : 0xFF;
		pointer = regs.read_word_PC() + regs.X(index_mask);
		if (use_db) pointer |= regs.DB() << 16;			
	} else if (info.mode == Operand::ABSOLUTE_INDEXED_Y) {
		uint16_t index_mask = index16(regs) ? 0xFFFF : 0xFF;
		pointer = regs.read_word_PC() + regs.Y(index_mask);
		if (use_db) pointer |= regs.DB() << 16;		
	} else if (info.mode == Operand::ABSOLUTE_LONG) {
		pointer = regs.read_long_PC();
	} else if (info.mode == Operand::ABSOLUTE_LONG_INDEXED_X) {
		pointer = regs.read_long_PC() + regs.X(index16(regs) ? 0xFFFF : 0xFF);
	} else if (info.mode == Operand::ABSOLUTE_INDIRECT) {
		uint32_t indirection_pointer_at = regs.read_word_PC();
		if (use_db) indirection_pointer_at |= regs.DB() << 16;
		else indirection_pointer_at |= regs.PC(0xFF0000);
		pointer = regs.read_long(indirection_pointer_at, MemoryAccessType::FETCH_INDIRECT);
		regs.indirection_pointer = indirection_pointer_at;
	} else if (info.mode == Operand::ABSOLUTE_INDIRECT_LONG) {
		uint16_t indirection_pointer_at = regs.read_word_PC();
		pointer = regs.read_long(indirection_pointer_at, MemoryAccessType::FETCH_INDIRECT);
	} else if (info.mode == Operand::ABSOLUTE_INDEXED_X_INDIRECT) {
		uint32_t indirection_pointer_at = regs.read_word_PC() + regs.X(index16(regs)?0xFFFF:0xFF);
		if (use_db) indirection_pointer_at |= regs.DB() << 16;
		else indirection_pointer_at |= regs.PC(0xFF0000);
		pointer = regs.read_long(indirection_pointer_at, MemoryAccessType::FETCH_INDIRECT);
		regs.indirection_pointer = indirection_pointer_at;
	} else if (info.mode == Operand::DIRECT_PAGE) {
		pointer = regs.DP() + (uint16_t)regs.read_byte_PC();
	} else if (info.mode == Operand::DIRECT_PAGE_INDEXED_X) {
		pointer = regs.DP() + (uint16_t)regs.read_byte_PC() + regs.X();
		//if (use_db) pointer |= regs.DB() << 16;
		//else pointer |= regs.PC(0xFF0000);
	} else if (info.mode == Operand::DIRECT_PAGE_INDEXED_Y) {
		pointer = regs.DP() + (uint16_t)regs.read_byte_PC() + regs.Y();
		//if (use_db) pointer |= regs.DB() << 16;
		//else pointer |= regs.PC(0xFF0000);
	} else if (info.mode == Operand::DIRECT_PAGE_INDIRECT) {
		uint16_t indirection_pointer_at = regs.read_byte_PC() + regs.DP(); // Always bank 0
		pointer = regs. read_word(indirection_pointer_at, MemoryAccessType::FETCH_INDIRECT);
		if (use_db) pointer |= regs.DB() << 16;
		else pointer |= regs.PC(0xFF0000);
		regs.indirection_pointer = indirection_pointer_at;
	} else if (info.mode == Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y) {
		// TODO: There are confusing wrapping rules here in 8-bit mode
		uint16_t indirection_pointer_at = regs.read_byte_PC() + regs.DP(); // Always bank 0
		uint16_t index_mask = index16(regs) ? 0xFFFF : 0xFF;
		pointer = regs.read_long(indirection_pointer_at, MemoryAccessType::FETCH_INDIRECT) + regs.Y(index_mask);
		regs.indirection_pointer = indirection_pointer_at;
	} else if (info.mode == Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y) {
		// TODO: There are confusing wrapping rules here in 8-bit mode
		uint16_t indirection_pointer_at = regs.read_byte_PC() + regs.DP(); // Always bank 0
		uint16_t index_mask = index16(regs) ? 0xFFFF : 0xFF;
		pointer = regs. read_word(indirection_pointer_at, MemoryAccessType::FETCH_INDIRECT) + regs.Y(index_mask);
		if (use_db) pointer |= regs.DB() << 16;
		else pointer |= regs.PC(0xFF0000);
		regs.indirection_pointer = indirection_pointer_at;
	} else if (info.mode == Operand::BRANCH_8) {
		int relative_offset = regs.read_byte_PC();
		// Unpacked relative offset for branch operations
		uint32_t branch_destination = regs.PC() + relative_offset;
		if (relative_offset >= 0x80) 
			branch_destination -= 0x100;
		pointer = branch_destination;
	} else if (info.mode == Operand::BRANCH_16) {
		int relative_offset = regs.read_word_PC();
		// Unpacked relative offset for branch operations
		uint32_t branch_destination = regs.PC() + relative_offset;
		if (relative_offset >= 0x8000) 
			branch_destination -= 0x10000;
		pointer = branch_destination;
	} else if (info.mode == Operand::DIRECT_PAGE_INDIRECT_LONG) {
		// TODO: There are confusing wrapping rules here in 8-bit mode
		uint32_t indirection_pointer_at = regs.read_byte_PC() + regs.DP(); // Always bank 0
		pointer = regs.read_long(indirection_pointer_at, MemoryAccessType::FETCH_INDIRECT);
		regs.indirection_pointer = indirection_pointer_at;
	} else if (info.mode == Operand::STACK_RELATIVE) {
		uint16_t t = regs.read_byte_PC() + regs.S(); 
		memory_mode = MemoryAccessType::STACK_RELATIVE;
		pointer = t;
	} else if (info.mode == Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y) {
		uint32_t indirection_pointer_at = regs.read_byte_PC() + regs.S(); 
		uint16_t index_mask = index16(regs) ? 0xFFFF : 0xFF;
		pointer = regs.read_long(indirection_pointer_at, MemoryAccessType::FETCH_INDIRECT) + regs.Y(index_mask);
		regs.indirection_pointer = indirection_pointer_at;
		memory_mode = MemoryAccessType::STACK_RELATIVE;
	} else {
		printf("** Adressing mode not implemented! %d\n", (int)info.mode);
		exit(1);
	}

	if (!value_resolved && info.load_operand) {
		value = wide ? regs. read_word(pointer, memory_mode) : regs.read_byte(pointer, memory_mode);
	}

	if (regs._debug && pointer != INVALID_POINTER) {
		if (info.load_operand) {
			if (wide)
				printf("  Loading value %04X from %06X\n", value, pointer);
			else
				printf("  Loading value %02X from %06X\n", value, pointer);
		} else {
			printf("  Address pointer is %06X\n", pointer);
		}
	}

	// Perform the requested operation
	// For many operations the operand has already been loaded into operand
	// For jumps, branches and stores the pointer points to where the result should go
	if (info.op == Operation::BCC) {
		if (!regs.P_flag(ProcessorStatusFlag::Carry))
			jump(regs, pointer, 0x00FFFF, Events::BRANCH);
	} else if (info.op == Operation::BCS) {
		if (regs.P_flag(ProcessorStatusFlag::Carry))
			jump(regs, pointer, 0x00FFFF, Events::BRANCH);
	} else if (info.op == Operation::BEQ) {
		if (regs.P((uint16_t)ProcessorStatusFlag::Zero))
			jump(regs, pointer, 0x00FFFF, Events::BRANCH);
	} else if (info.op == Operation::BMI) {
		if (regs.P((uint16_t)ProcessorStatusFlag::Negative))
			jump(regs, pointer, 0x00FFFF, Events::BRANCH);
	} else if (info.op == Operation::BNE) {
		if (!regs.P((uint16_t)ProcessorStatusFlag::Zero))
			jump(regs, pointer, 0x00FFFF, Events::BRANCH);
	} else if (info.op == Operation::BPL) {
		if (!regs.P((uint16_t)ProcessorStatusFlag::Negative))
			jump(regs, pointer, 0x00FFFF, Events::BRANCH);
	} else if (info.op == Operation::BRA) {
		jump(regs, pointer, 0x00FFFF, Events::BRANCH);
	} else if (info.op == Operation::BRL) {
		jump(regs, pointer, 0x00FFFF, Events::BRANCH);
	} else if (info.op == Operation::BVC) {
		if (!regs.P((uint16_t)ProcessorStatusFlag::Overflow))
			jump(regs, pointer, 0x00FFFF, Events::BRANCH);
	} else if (info.op == Operation::BVS) {
		if (regs.P((uint16_t)ProcessorStatusFlag::Overflow))
			jump(regs, pointer, 0x00FFFF, Events::BRANCH);
	} else if (info.op == Operation::JMP) {
		// JMP never changes PB
		jump(regs, pointer, 0x00FFFF, Events::JMP_OR_JML);
	} else if (info.op == Operation::JML) {
		jump(regs, pointer, 0xFFFFFF, Events::JMP_OR_JML);
	} else if (info.op == Operation::JSR) {
		// JSR never changes PB
		uint16_t pc = regs.PC(0x00FFFF);
		pc--;
		push_word_stack(regs, pc);
		jump(regs, pointer, 0x00FFFF, Events::JSR_OR_JSL);
	} else if (info.op == Operation::JSL) {
		uint32_t pc = regs.PC(0xFFFFFF);
		pc--; // TODO: Shouldn't wrap but think through
		push_long_stack(regs, pc);
		jump(regs, pointer, 0xFFFFFF, Events::JSR_OR_JSL);
	} else if (info.op == Operation::STA) {
		if (wide) regs.write_word(pointer, regs.A(), memory_mode); else regs.write_byte(pointer, (uint8_t)regs.A(0xFF), memory_mode);
	} else if (info.op == Operation::STX) {
		if (wide) regs.write_word(pointer, regs.X(), memory_mode); else regs.write_byte(pointer, (uint8_t)regs.X(0xFF), memory_mode);
	} else if (info.op == Operation::STY) {
		if (wide) regs.write_word(pointer, regs.Y(), memory_mode); else regs.write_byte(pointer, (uint8_t)regs.Y(0xFF), memory_mode);
	} else if (info.op == Operation::STZ) {
		if (memory16(regs)) {
			regs.write_word(pointer, 0, memory_mode);
		} else {
			regs.write_byte(pointer, 0, memory_mode);
		}
	} else if (info.op == Operation::LDX) {
		regs.set_X(set_nz_flags(regs, value, wide), wide_mask);
	} else if (info.op == Operation::LDY) {
		regs.set_Y(set_nz_flags(regs, value, wide), wide_mask);
	} else if (info.op == Operation::LDA) {
		regs.set_A(set_nz_flags(regs, value, wide), wide_mask);	
	} else if (info.op == Operation::CPX || info.op == Operation::CPY || info.op == Operation::CMP) {
		// Fetch register (can be A,X,Y)
		uint16_t compare = 0;
		if (info.op == Operation::CPX) compare = regs.X(wide_mask);
		else if (info.op == Operation::CPY) compare = regs.Y(wide_mask);
		else compare = regs.A(wide_mask);

		// Set nzc based on comparision result
		uint16_t flags_value = 0;
		const uint16_t flags_affected = (uint16_t)ProcessorStatusFlag::Carry|(uint16_t)ProcessorStatusFlag::Negative|(uint16_t)ProcessorStatusFlag::Zero;

		if (compare == value) flags_value |= (uint16_t)ProcessorStatusFlag::Zero; // Equal
		if (compare >= value) flags_value |= (uint16_t)ProcessorStatusFlag::Carry; // Register smaller

		if (wide) {
			if ((compare-value) & 0x8000) flags_value |= (uint16_t)ProcessorStatusFlag::Negative; // Highest bit set in result
		} else {
			uint8_t result = ((uint8_t)compare)-((uint8_t)value);
			if (result & 0x80) flags_value |= (uint16_t)ProcessorStatusFlag::Negative; // Highest bit set in result
		}

		regs.set_P(flags_value, flags_affected);

	} else if (info.op == Operation::BIT) {

		uint16_t flags_affected = (uint16_t)ProcessorStatusFlag::Zero;

		if (info.mode != Operand::IMMEDIATE_MEMORY)
			flags_affected |= (uint16_t)ProcessorStatusFlag::Negative|(uint16_t)ProcessorStatusFlag::Overflow;

		uint16_t flags = 0;

		if (wide) {
			if (value & 0x8000) flags |=(uint16_t)ProcessorStatusFlag::Negative;
			if (value & 0x4000) flags |=(uint16_t)ProcessorStatusFlag::Overflow;
		} else {
			if (value & 0x80) flags |=(uint16_t)ProcessorStatusFlag::Negative;
			if (value & 0x40) flags |=(uint16_t)ProcessorStatusFlag::Overflow;
		}

		uint16_t acc = regs.A(wide_mask);
		uint16_t result = acc & value;

		if (result == 0)
			flags |=(uint16_t)ProcessorStatusFlag::Zero;

		regs.set_P(flags, flags_affected);
	} else if (info.op == Operation::AND) {
		uint16_t result = regs.A(wide_mask) & value;
		regs.set_A(set_nz_flags(regs, result, wide), wide_mask);
	} else if (info.op == Operation::ORA) {
		uint16_t result = regs.A(wide_mask) | value;
		regs.set_A(set_nz_flags(regs, result, wide), wide_mask);
	} else if (info.op == Operation::EOR) {
		uint16_t result = regs.A(wide_mask) ^ value;
		regs.set_A(set_nz_flags(regs, result, wide), wide_mask);
	} else if (info.op == Operation::ADC) {

		bool decimal = regs.P_flag(ProcessorStatusFlag::Decimal);

		if (!decimal) {
			if (wide) op_adc_16(regs, value);
			else op_adc_8(regs, (uint8_t)value);
		} else {
			if (wide) op_adc_decimal_16(regs, value);
			else op_adc_decimal_8(regs, (uint8_t)value);
		}

	} else if (info.op == Operation::SBC) {

		CUSTOM_ASSERT(!regs.P_flag(ProcessorStatusFlag::Decimal));

		const uint16_t affected_flags =
			(uint16_t)ProcessorStatusFlag::Carry|
			//(uint16_t)ProcessorStatusFlag::Negative|
			//(uint16_t)ProcessorStatusFlag::Zero|
			(uint16_t)ProcessorStatusFlag::Overflow;

		uint16_t flags = 0;

		const uint16_t a = regs.A(wide_mask);

		const uint16_t old_borrow = regs.P_flag(ProcessorStatusFlag::Carry) !=0 ? 0 : 1;
		int32_t signed_result = (int32_t)a - (int32_t)value - (int32_t)old_borrow;

		if (signed_result >= 0)
			flags |= (uint16_t)ProcessorStatusFlag::Carry;
		
		uint16_t result = 0;
		if (wide) {
			result = uint16_t(signed_result);
			if ((a^value) & (a^result) & 0x8000)
				flags |= (uint16_t)ProcessorStatusFlag::Overflow;
		} else {
			result = uint8_t(signed_result);
			if ((a^value) & (a^result) & 0x80)
				flags |= (uint16_t)ProcessorStatusFlag::Overflow;
		}

		set_nz_flags(regs, result, wide);
		regs.set_A(result, wide_mask);
		regs.set_P(flags, affected_flags);

	} else if (info.op == Operation::INC) {
		const uint16_t result = op_inc(regs, value, wide);
		if (value_resolved) {
			CUSTOM_ASSERT(info.mode == Operand::ACCUMULATOR);
			regs.set_A(result, wide_mask);
		} else {
			write_value(regs, pointer, result, wide, memory_mode);
		}

	} else if (info.op == Operation::ROL || info.op == Operation::LSR || info.op == Operation::ROR || info.op == Operation::ASL) {
		// ROL and ROR shift in carry, LSR and ASL does not
		const uint16_t shift_in = (info.op == Operation::ROL || info.op == Operation::ROR) ? (regs.P_flag(ProcessorStatusFlag::Carry) ? 1:0) : 0;
		const bool shift_left = info.op == Operation::ROL || info.op == Operation::ASL;

		bool new_carry = false;

		// Number to shift in value
		if (shift_left) {
			new_carry = (value & (wide ? 0x8000 : 0x80))!=0;
			value <<= 1;
			value |= shift_in;
		} else {
			new_carry = value & 1;
			value >>= 1;
			value |= shift_in<<(wide ? 15 : 7);
		}

		regs.set_P(new_carry ? (uint16_t)ProcessorStatusFlag::Carry : 0, (uint16_t)ProcessorStatusFlag::Carry);

		value &= wide_mask;

		set_nz_flags(regs, value, wide);

		if (value_resolved) {
			CUSTOM_ASSERT(info.mode == Operand::ACCUMULATOR);
			regs.set_A(value, wide_mask);
		} else {
			write_value(regs, pointer, value, wide, memory_mode);
		}

	} else if (info.op == Operation::CLV) {
		regs.set_P(0, (uint16_t)ProcessorStatusFlag::Overflow);
	} else if (info.op == Operation::CLD) {
		regs.set_P(0, (uint16_t)ProcessorStatusFlag::Decimal);
	} else if (info.op == Operation::CLC) {
		regs.set_P(0, (uint16_t)ProcessorStatusFlag::Carry);
	} else if (info.op == Operation::SEC) {
		regs.set_P(0xFFFF, (uint16_t)ProcessorStatusFlag::Carry);
	} else if (info.op == Operation::SED) {
		regs.set_P(0xFFFF, (uint16_t)ProcessorStatusFlag::Decimal);
	} else if (info.op == Operation::CLI) {
		regs.set_P(0, (uint16_t)ProcessorStatusFlag::IRQ);
	} else if (info.op == Operation::SEI) {
		regs.set_P((uint16_t)ProcessorStatusFlag::IRQ, (uint16_t)ProcessorStatusFlag::IRQ);

	} else if (push_pull(regs, info.op)) {
	} else if (transfer(regs, info.op)) {

	} else if (info.op == Operation::INX) {
		const bool wide = index16(regs);
		const uint16_t wide_mask = wide ? 0xFFFF : 0xFF;
		const uint16_t result = op_inc(regs, regs.X(wide_mask), wide);
		regs.set_X(result, wide_mask);
	} else if (info.op == Operation::INY) {
		const bool wide = index16(regs);
		const uint16_t wide_mask = wide ? 0xFFFF : 0xFF;
		const uint16_t result = op_inc(regs, regs.Y(wide_mask), wide);
		regs.set_Y(result, wide_mask);
	} else if (info.op == Operation::DEC) {
		const uint16_t result = op_dec(regs, value, wide);
		if (value_resolved) {
			CUSTOM_ASSERT(info.mode == Operand::ACCUMULATOR);
			regs.set_A(result, wide_mask);
		} else {
			write_value(regs, pointer, result, wide, memory_mode);
		}
	} else if (info.op == Operation::DEX) {
		// TODO: Use op_dec
		bool wide = index16(regs);
		if (wide) {
			uint16_t value = regs.X()-1;
			set_nz_flags(regs, value, true);
			regs.set_X(value);
		} else {
			uint8_t value = ((uint8_t)regs.X(0xFF))-(uint8_t)1;
			set_nz_flags(regs, value, false);
			regs.set_X(value, 0xFF);
		}
	} else if (info.op == Operation::DEY) {
		// TODO: Use op_dec
		bool wide = index16(regs);
		if (wide) {
			uint16_t value = regs.Y()-1;
			set_nz_flags(regs, value, true);
			regs.set_Y(value);
		} else {
			uint8_t value = ((uint8_t)regs.Y(0xFF))-(uint8_t)1;
			set_nz_flags(regs, value, false);
			regs.set_Y(value, 0xFF);
		}
	} else if (info.op == Operation::SEP || info.op == Operation::REP) {
		uint16_t v = regs.read_byte_PC(); 
		if (info.op == Operation::SEP) {
			regs.set_P(0xFFFF, v); // Set relevant bits to 1
		} else {
			regs.set_P(0, v);  // Set relevant bits to 0
		}

		if (regs.P((uint16_t)ProcessorStatusFlag::Emulation)) {
			regs.set_P(0xFFFF, (uint16_t)ProcessorStatusFlag::IndexFlag|(uint16_t)ProcessorStatusFlag::MemoryFlag);
		}

		// This is mostly to mimic snes9x... Does not seem to match
		// https://wiki.superfamicom.org/snes/files/assembly-programming-manual-for-w65c816.pdf

		if (!index16(regs)) {
			// https://wiki.nesdev.com/w/images/7/76/Programmanual.pdf p79
			regs.set_X(0, 0xFF00);
			regs.set_Y(0, 0xFF00);
		}

	} else if (info.op == Operation::PEA) {
		uint16_t value = regs.read_word_PC();
		push_word_stack(regs, value);

	} else if (info.op == Operation::PEI) {
		// TODO: This one both reads and pushes to stack.. so maybe not compatible with rewind (yet)
		uint32_t value_address = regs.DP() + regs.read_byte_PC();
		value_address |= regs.DB() << 16;
		uint16_t value = regs.read_word(value_address, MemoryAccessType::RANDOM);
		push_word_stack(regs, value);

	} else if (info.op == Operation::XCE) {
		bool carry = regs.P_flag(ProcessorStatusFlag::Carry) != 0;
		bool emulation = regs.P((uint16_t)ProcessorStatusFlag::Emulation) != 0;
		uint16_t s = 0;
		if (carry) s|=(uint16_t)ProcessorStatusFlag::Emulation;
		if (emulation) s|=(uint16_t)ProcessorStatusFlag::Carry;

		regs.set_P(s, (uint16_t)ProcessorStatusFlag::Emulation|(uint16_t)ProcessorStatusFlag::Carry);
	} else if (info.op == Operation::XBA) {
		// NOTE: Ignores memory flag
		uint16_t value = regs.A();
		uint8_t new_b = value & 0xFF;
		uint8_t new_a = value >> 8;

		const uint16_t flags_affected  = (uint16_t)ProcessorStatusFlag::Negative|(uint16_t)ProcessorStatusFlag::Zero;
		uint16_t flags_set = 0;
		if (new_a == 0) flags_set |= (uint16_t)ProcessorStatusFlag::Zero;
		if (new_a & 0x80) flags_set |= (uint16_t)ProcessorStatusFlag::Negative;
		regs.set_P(flags_set, flags_affected);
		regs.set_A(new_a|(new_b<<8));
	} else if (info.op == Operation::RTI) {
		const bool emulation = regs.P((uint16_t)ProcessorStatusFlag::Emulation) != 0;

		if (!emulation) {

			uint32_t all = pop_dword_stack(regs);
			uint8_t p = all & 0xFF;
			uint32_t return_adress = all >> 8;

			regs.set_P(p, 0xFF); // TODO: Can it change emulation?
			jump(regs, return_adress, 0xFFFFFF, Events::RTI);

		} else {
			uint32_t all = pop_long_stack(regs);
			uint8_t p = all & 0xFF;
			uint16_t return_adress = all >> 8;

			p |= (uint16_t)ProcessorStatusFlag::MemoryFlag;
			p |= (uint16_t)ProcessorStatusFlag::IndexFlag;

			regs.set_P(p, 0xFF); // TODO: Can it change emulation?
			jump(regs, return_adress, 0x00FFFF, Events::RTI);
		}

		if (!index16(regs)) {
			regs.set_X(0, 0xFF00);
			regs.set_Y(0, 0xFF00);
		}

	} else if (info.op == Operation::RTS) {
		uint16_t return_adress = pop_word_stack(regs);
		return_adress++;
		jump(regs, return_adress, 0x00FFFF, Events::RTS_OR_RTL);
	} else if (info.op == Operation::RTL) {
		uint32_t return_adress = pop_long_stack(regs);
		return_adress++;
		jump(regs, return_adress, 0xFFFFFF, Events::RTS_OR_RTL);
	} else if (info.op == Operation::TSB || info.op == Operation::TRB) {
		CUSTOM_ASSERT(!value_resolved);

		uint16_t result = regs.A(wide_mask) & value;
		regs.set_P(result == 0 ? (uint16_t)ProcessorStatusFlag::Zero : 0, (uint16_t)ProcessorStatusFlag::Zero);

		uint16_t write_back = 0;
		if (info.op == Operation::TSB) {
			write_back = value | regs.A(wide_mask);
		} else if (info.op == Operation::TRB) {
			write_back = value & ~regs.A(wide_mask);
		}
		
		write_value(regs, pointer, write_back, wide, memory_mode);
	} else if (info.op == Operation::NOP) {
	} else if (info.op == Operation::WAI) {
		// For our situation we can just eat up the WAI. Perhaps report to regs as an event?
	} else if (info.op == Operation::COP || info.op == Operation::BRK) {

		uint16_t target_adress = 0;

		uint16_t pcp = ((uint16_t)regs.PC(0xFFFF))+1;

		const uint16_t v  = info.op == Operation::BRK ? 0xFFE6 : 0xFFE4;
		const uint16_t ve = info.op == Operation::BRK ? 0xFFFE : 0xFFF4;

		if (!regs.P_flag(ProcessorStatusFlag::Emulation))	{
			uint32_t value = ((regs.PC(0xFF0000)|pcp)<<8)|(regs.P(0xFF));
			push_dword_stack(regs, value);
			target_adress = regs.read_word(v, MemoryAccessType::FETCH_IRQ_VECTOR);
		} else {
			uint32_t value = (pcp<<8)|(regs.P(0xFF));
			push_long_stack(regs, value);
			target_adress = regs.read_word(ve, MemoryAccessType::FETCH_IRQ_VECTOR);
		}

		regs.set_P((uint16_t)ProcessorStatusFlag::IRQ, (uint16_t)ProcessorStatusFlag::Decimal|(uint16_t)ProcessorStatusFlag::IRQ);

		// All IRQs go to bank 0
		jump(regs, target_adress, 0xFFFFFF, Events::IRQ); // TODO: Is ::IRQ correct here?

	} else if (info.op == Operation::MVN || info.op == Operation::MVP) { // Block Move Next

		// snes9x emulates MVN/MVP using one operation per copied byte
		// We match this so that we can use the oer-op verification coming from snes9x

		uint8_t dest_bank = regs.read_byte_PC();
		uint8_t source_bank = regs.read_byte_PC();

		bool index_wide = index16(regs);
		uint16_t source_adr = regs.X(index_wide?0xFFFF:0xFF);
		uint16_t dest_adr = regs.Y(index_wide?0xFFFF:0xFF);

		uint16_t num_bytes_minus_one = regs.A();

		uint32_t d = (dest_bank<<16)|dest_adr;
		uint32_t s = (source_bank<<16)|source_adr;
		uint8_t v = regs.read_byte(s, MemoryAccessType::FETCH_MVN_MVP);
		regs.write_byte(d, v, MemoryAccessType::WRITE_MVN_MVP);

		if (info.op == Operation::MVN) {
			if (index_wide) {
				source_adr++;
				dest_adr++;
			} else {
				uint8_t sa = (uint8_t)source_adr; sa++; source_adr = sa;
				uint8_t da = (uint8_t)dest_adr; da++; dest_adr = da;
			}
		} else if (info.op == Operation::MVP) {
			if (index_wide) {
				source_adr--;
				dest_adr--;
			} else {
				uint8_t sa = (uint8_t)source_adr; sa--; source_adr = sa;
				uint8_t da = (uint8_t)dest_adr; da--; dest_adr = da;
			}
		}
		num_bytes_minus_one--;

		regs.set_DB(dest_bank);
		regs.set_A(num_bytes_minus_one);
		regs.set_X(source_adr);
		regs.set_Y(dest_adr);

		if (num_bytes_minus_one != 0xFFFF) {
			// We are not done yet, do op once again
			regs.set_PC(regs.PC()-3);
		}

	} else {
		if (!regs._debug)
			printf("Op %s (%s) %02X at %06X i%d m%d %s %s %s%s %d\n", Operation_names[(int)info.op], mnemonic_names[opcode_], opcode_, pc_before, regs.P((uint16_t)ProcessorStatusFlag::IndexFlag)==0?16:8, regs.P((uint16_t)ProcessorStatusFlag::MemoryFlag)==0?16:8, Operand_names[(int)info.mode], InstructionSize_names[(int)info.size], wide?"wide":"small", info.load_operand?" load":"", regs._debug_number);
		printf("* Unsupported op %02X\n", opcode_);
		exit(1);
	}
}

// TODO: Merge nmi and irq, they are mostly the same except for vector
void execute_nmi(EmulateRegisters &regs) {
	// TODO: Find better way to disable reads/writes
	EmulateRegisters::memoryAccessFunc read_func = regs._read_function;
	EmulateRegisters::memoryAccessFunc write_func = regs._write_function;
	regs._read_function = nullptr;
	regs._write_function = nullptr;

	const bool emulation = regs.P((uint16_t)ProcessorStatusFlag::Emulation) != 0;

	if (!emulation)
		push_byte_stack(regs, regs.PC(0xFF0000)>>16);

	push_word_stack(regs, regs.PC(0x00FFFF)); // TODO: Is this a long push?
	push_byte_stack(regs, (uint8_t)regs.P(0xFF)); // TODO: Wrapping should go in low byte of S only when emulation

	uint16_t addr = regs. read_word(emulation ? 0xFFFA : 0xFFEA, MemoryAccessType::FETCH_NMI_VECTOR);
	regs.set_PC(addr, 0xFFFFFF);

	regs.set_P((uint16_t)ProcessorStatusFlag::IRQ, (uint16_t)ProcessorStatusFlag::IRQ|(uint16_t)ProcessorStatusFlag::Decimal);

	regs._read_function = read_func;
	regs._write_function = write_func;
}

void execute_irq(EmulateRegisters &regs) {
	const bool emulation = regs.P_flag(ProcessorStatusFlag::Emulation);

	if (!emulation) {
		push_long_stack(regs, regs.PC());
		push_byte_stack(regs, (uint8_t)regs.P(0xFF));
		uint16_t addr = regs.read_word(0xFFEE, MemoryAccessType::FETCH_IRQ_VECTOR);
		regs.set_PC(addr);
	} else {
		push_word_stack(regs, regs.PC(0xFFFF));
		push_byte_stack(regs, (uint8_t)regs.P(0xFF));
		uint16_t addr = regs.read_word(0xFFFE, MemoryAccessType::FETCH_IRQ_VECTOR);
		regs.set_PC(addr);
	}

	// Set irq, clear decimal
	regs.set_P((uint16_t)ProcessorStatusFlag::IRQ, 
		(uint16_t)ProcessorStatusFlag::IRQ|(uint16_t)ProcessorStatusFlag::Decimal);
}

template<int IDX, typename T>
bool bit(const T t) { return (t >> IDX)&1; }

void execute_dma(EmulateRegisters & regs, uint8_t channels) {
	for (int channel=0; channel<8; channel++) {
		if ((channels & (1<<channel))==0)
			continue;

		const uint32_t bc = channel<<4;
		const uint32_t b = 0x4300|bc;

		const uint8_t params = regs._memory[b|0];
		const bool reverse_transfer = bit<7>(params);
		const bool type = bit<6>(params);
		const bool decrement = bit<4>(params);
		const uint8_t transfer_mode = params & 7;
		const bool fixed = bit<3>(params);

		const uint16_t a_address = (regs._memory[b|3]<<8)|regs._memory[b|2];
		const uint8_t b_address = regs._memory[b|1];
		const uint8_t a_bank = regs._memory[b|4];
		const uint32_t transfer_bytes = (regs._memory[b|6]<<8)|regs._memory[b|5];
		
		if (regs._dma_function) {
			DmaTransfer d;

			d.b_address = b_address;
			d.a_address = a_address;
			d.a_bank = a_bank;
			d.transfer_bytes = transfer_bytes;
			d.transfer_mode = transfer_mode;
			d.channel = channel;
			d.pc = regs._PC_before;
			d.flags = 0;
			
			if (decrement) d.flags |= DmaTransfer::A_ADDRESS_DECREMENT;
			if (fixed) d.flags |= DmaTransfer::A_ADDRESS_FIXED;
			if (reverse_transfer) d.flags |= DmaTransfer::REVERSE_TRANSFER;

			regs._dma_function(regs._callback_context, d);
		}

		// Ignore everything but CPU->$2180 DMA for now
		// Also filter invalid source for 2180 target (7E or 7F)
		if (a_bank == 0x7E || a_bank == 0x7F || b_address != 0x80 || reverse_transfer)
			continue;

		if (reverse_transfer) {
			printf("Error! Emulator does not support DMA from PPU->CPU!\n");
			exit(1);
		}

		const uint32_t num_transfer = transfer_bytes == 0 ? 0x10000 : transfer_bytes;

		uint32_t source = (a_bank<<16)|a_address;
		uint32_t wram = regs._WRAM;

		const int delta = fixed ? 0 : (decrement ? -1 : 1);

		if (transfer_mode != 0) {
			printf("Error! Emulator does not support DMA from CPU->WRAM through $2180 with transfer mode = %d\n", transfer_mode);
			exit(1);
		}

		for (uint32_t k=0; k<num_transfer; k++) {
			uint32_t s = regs.remap(source);
			source += delta;
			uint32_t d = regs.remap(0x7E0000+wram);
			wram=(wram+1)&0x1ffff;
			uint8_t db = d>>16;
			regs._memory[d] = regs._memory[s];
		}
		regs._WRAM = wram;
	}
}
