#include "cputable.h"
#include "romaccessor.h"

bool branches[256];
bool jumps[256];
bool pushpops[256];

void initLookupTables() {
	for (int ih = 0; ih<256; ih++) {
		jumps[ih] = false;
		branches[ih] = false;
		pushpops[ih] = false;
		const char * const i = opCodeInfo[ih].mnemonics;
		if (i[0] == 'J') {
			jumps[ih] = true;
		}
		else if (i[0] == 'B') {
			if (strcmp(i, "BRK") == 0) continue;
			if (strcmp(i, "BIT") == 0) continue;
			jumps[ih] = true;
			branches[ih] = true;
		} else if (i[0]=='P' && i[1]=='H') {
			pushpops[ih] = true;
		} else if (i[0]=='P' && i[1]=='L') {
			pushpops[ih] = true;
		}
	}
}

uint32_t calculateFormattingandSize(const uint8_t * data, const bool acc16, const bool ind16, char * target, char * targetLabel, int * bitmodeNeeded) {
	const uint8_t opcode = data[0];
	const int am = opCodeInfo[opcode].adressMode;
	const AdressModeInfo &ami = g_oplut[am];
	if (bitmodeNeeded)
		*bitmodeNeeded = 8;

	// We have a few special cases that doesn't work with our simple table
	if (am == 1 && !acc16) {
		if (target) sprintf(target, "#$%02X", data[1]);
		return 2;
	}
	else if (am == 2 && !ind16) {
		if (target) sprintf(target, "#$%02X", data[1]);
		return 2;
	}
	else if (am == 3 && (opcode == 0 || opcode == 2)) {
		if (target) sprintf(target, "$%02X", data[1]);
		return 2;
	}
	else if (am == 4 && branches[opcode]) {
		const int signed_offset = unpackSigned(data[1]);
		if (signed_offset >= 0) {
			if (target) sprintf(target, "$%02X", abs(signed_offset));
		}
		else {
			if (target) sprintf(target, "-$%02X", abs(signed_offset));
		}
		if (targetLabel) strcpy(targetLabel, ami.formattingWithLabelString);
		return 2;
	} if (opcode == 0x54) {
		// HACK: WLA DX specific reversal of parameter order, involve asmwrite_wladx so it can customize?
		const int nb = ami.numBytes;
		assert(nb == 3);
		const char * result = ami.formattingString;
		if (target) sprintf(target, result, data[1], data[2]);
		if (targetLabel) strcpy(targetLabel, ami.formattingWithLabelString);
		if (bitmodeNeeded) *bitmodeNeeded = ami.numBitsForOpcode;
		return nb;
	}
	else {
		const int nb = ami.numBytes;
		const char * result = ami.formattingString;
		if (target) sprintf(target, result, data[nb - 1], data[nb - 2], data[nb - 3]);
		if (targetLabel) strcpy(targetLabel, ami.formattingWithLabelString);
		if (bitmodeNeeded) *bitmodeNeeded = ami.numBitsForOpcode;
		return nb;
	}
}

ResultType evaluateOp(const RomAccessor &rom_accessor, const uint8_t * ops, const CPURegisters & reg, MagicByte * result_bank, MagicWord * result_addr, bool *depend_DB, bool *depend_DP, bool *depend_X, bool *depend_Y) {

	assert(!depend_DB || !(*depend_DB));
	assert(!depend_DP || !(*depend_DP));
	assert(!depend_X  || !(*depend_X ));
	assert(!depend_Y  || !(*depend_Y ));

	// Introduce all registers with known flags.. then calculate adress with values... and check flags to see if it was "determined"

	// TODO: Some of these are based on the fact that we want to do an indirection via memory...
	//       but alot of memory is known (such as all jump tables). Involve ROM.
	//       For this to work we must differentiate between source adress and the pointer read...

	const uint8_t opcode = *ops;
	const int am = opCodeInfo[opcode].adressMode;

	if (am >= 0 && am <= 3) {
		return SA_IMMEDIATE;
	}
	else if (am == 4) {
		const int signed_offset = unpackSigned(ops[1]);
		*result_bank = reg.pb;
		*result_addr = reg.pc + (signed_offset + 2);
		return SA_ADRESS;
	}
	else if (am == 5) {
		const Pointer relative(ops[2] * 256 + ops[1]);
		*result_bank = reg.pb;
		*result_addr = reg.pc + relative + 3;
		return SA_ADRESS;
	}
	else if (am == 6) {
		*result_bank = 0x00;
		*result_addr = reg.dp + ops[1];
		if (depend_DP) *depend_DP = true;
		return SA_ADRESS;
	}
	else if (am == 13) {
		// TODO: Would be cool to not only support this, but also give the indirection pointer... it probably points out a jump table or so
		return SA_NOT_IMPLEMENTED;
	}
	else if (am == 14 || am == 28) {
		*result_bank = am == 14 ? reg.db : reg.pb;
		*result_addr = ((ops[2] << 8) | ops[1]);
		if (am == 14 && depend_DB) *depend_DB = true;
		return SA_ADRESS;
	}
	else if (am == 15) {
		*result_bank = am == 15 ? reg.db : reg.pb;
		*result_addr = reg.reg_X + ((ops[2] << 8) | ops[1]); // TODO: Must X be 16-bit for this op?
		if (depend_X) *depend_X = true;
		if (am == 15 && depend_DB) *depend_DB = true;
		return SA_ADRESS;
	}
	else if (am == 16) {
		*result_bank = reg.db;
		*result_addr = reg.reg_Y + ((ops[2] << 8) | ops[1]); // TODO: Must Y be 16-bit for this op?
		if (depend_Y) *depend_Y = true;
		if (depend_DB) *depend_DB = true;
		return SA_ADRESS;
	}
	else if (am == 17) {
		*result_bank = ops[3];
		*result_addr = ((ops[2] << 8) | ops[1]);
		return SA_ADRESS;
	}
	else if (am == 18) {
		*result_bank = ops[3];
		*result_addr = reg.reg_X + ((ops[2] << 8) | ops[1]);
		if (depend_X) *depend_X = true;
		return SA_ADRESS;
	}
	else if (am == 22) {
		uint16_t addr = (ops[2] << 8) | ops[1];
		bool is_jump = jumps[opcode]; // TODO: Consider making other am?
		uint8_t bank = is_jump ? reg.pb.value : reg.db.value; // NOTE: Bypasses unknown system for now, TODO
		Pointer indirection_pointer = (bank << 16)|addr;
		if (!rom_accessor.is_rom(indirection_pointer))
			return SA_ADRESS;
		uint8_t l = rom_accessor.evalByte(indirection_pointer  );
		uint8_t h = rom_accessor.evalByte(indirection_pointer+1);
		*result_bank = bank;
		*result_addr = (h<<8)|l;
		return SA_ADRESS;
	}
	else if (am == 23) {
		MagicWord addr = reg.reg_X + (ops[2] << 8) | ops[1];
		bool is_jump = jumps[opcode];
		assert(is_jump);
		uint8_t bank = reg.pb.value; // NOTE: Bypasses unknown system for now, TODO
		Pointer indirection_pointer = (bank << 16)|addr.value;
		if (!rom_accessor.is_rom(indirection_pointer))
			return SA_ADRESS;
									  // Address load wraps within the bank
		uint8_t l = rom_accessor.evalByte(indirection_pointer  );
		uint8_t h = rom_accessor.evalByte(indirection_pointer+1);
		*result_bank = bank;
		*result_addr = (h<<8)|l;
		return SA_ADRESS;
	}
	else if (am == 24) {
		return SA_ACCUMULATOR;
	}
	else {
		return SA_NOT_IMPLEMENTED;
	}
}

inline bool is_cartridge_sram(uint8_t bank, uint16_t adr) {
	if (adr >= 0x8000) return false;
	if (bank >= 0x70 && bank <= 0x7D) return true;
	if (bank >= 0xF0 && bank <= 0xFf) return true;
	return false;
}

inline bool is_ram(uint8_t bank, uint16_t adr) {
	if (bank >= 0x7E && bank <= 0x7F) return true;
	if (adr >= 0x2000) return false;
	if (bank >= 0x00 && bank <= 0x3f) return true;
	if (bank >= 0x80 && bank <= 0xBF) return true;
	return false;
}

inline bool is_rom(uint8_t bank, uint16_t adr) {
	if (adr < 0x8000) return false;
	if (bank == 0x7E || bank == 0x7F) return false;
	return true;
}

Pointer lorom_bank_remap(const Pointer resolve_address) {
	uint16_t adr = resolve_address & 0xffff;
	if (adr >= 0x8000) return resolve_address;

	uint8_t bank = resolve_address >> 16;
	if ((bank >= 0x00 && bank <= 0x3F) || (bank >= 0x80 && bank <= 0xBF)) {
		if (adr < 0x2000) {
			return 0x7E0000|adr;
		} else  {
			// A bit of a hack. Make all memory mapped adresses go to bank 0 so we can annotate them once
			return adr; // Strip bank from resolve_adress
		}
	} else {
		return resolve_address;
	}
}
