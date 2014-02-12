#ifndef SNESTISTICS_CPUTABLE
#define SNESTISTICS_CPUTABLE

#include <stdint.h>

#define STATUS_EMULATION_FLAG (256)
#define STATUS_MEMORY_FLAG (0x10)
#define STATUS_ACCUMULATOR_FLAG (0x20)

struct OpCodeInfo {
	int id;
	const char * const mnemonics;
	int adressMode;
};

static OpCodeInfo opCodeInfo[256]=
{
	{ 0x00, "BRK", 3 },
	{ 0x01, "ORA", 10 },
	{ 0x02, "COP", 3 },
	{ 0x03, "ORA", 19 },
	{ 0x04, "TSB", 6 },
	{ 0x05, "ORA", 6 },
	{ 0x06, "ASL", 6 },
	{ 0x07, "ORA", 12 },
	{ 0x08, "PHP", 0 },
	{ 0x09, "ORA", 1 },
	{ 0x0A, "ASL", 24 },
	{ 0x0B, "PHD", 0 },
	{ 0x0C, "TSB", 14 },
	{ 0x0D, "ORA", 14 },
	{ 0x0E, "ASL", 14 },
	{ 0x0F, "ORA", 17 },
	{ 0x10, "BPL", 4 },
	{ 0x11, "ORA", 11 },
	{ 0x12, "ORA", 9 },
	{ 0x13, "ORA", 20 },
	{ 0x14, "TRB", 6 },
	{ 0x15, "ORA", 7 },
	{ 0x16, "ASL", 7 },
	{ 0x17, "ORA", 13 },
	{ 0x18, "CLC", 0 },
	{ 0x19, "ORA", 16 },
	{ 0x1A, "INC", 24 },
	{ 0x1B, "TCS", 0 },
	{ 0x1C, "TRB", 14 },
	{ 0x1D, "ORA", 15 },
	{ 0x1E, "ASL", 15 },
	{ 0x1F, "ORA", 18 },
	{ 0x20, "JSR", 14 },
	{ 0x21, "AND", 10 },
	{ 0x22, "JSL", 17 },
	{ 0x23, "AND", 19 },
	{ 0x24, "BIT", 6 },
	{ 0x25, "AND", 6 },
	{ 0x26, "ROL", 6 },
	{ 0x27, "AND", 12 },
	{ 0x28, "PLP", 0 },
	{ 0x29, "AND", 1 },
	{ 0x2A, "ROL", 24 },
	{ 0x2B, "PLD", 0 },
	{ 0x2C, "BIT", 14 },
	{ 0x2D, "AND", 14 },
	{ 0x2E, "ROL", 14 },
	{ 0x2F, "AND", 17 },
	{ 0x30, "BMI", 4 },
	{ 0x31, "AND", 11 },
	{ 0x32, "AND", 9 },
	{ 0x33, "AND", 20 },
	{ 0x34, "BIT", 7 },
	{ 0x35, "AND", 7 },
	{ 0x36, "ROL", 7 },
	{ 0x37, "AND", 13 },
	{ 0x38, "SEC", 0 },
	{ 0x39, "AND", 16 },
	{ 0x3A, "DEC", 24 },
	{ 0x3B, "TSC", 0 },
	{ 0x3C, "BIT", 15 },
	{ 0x3D, "AND", 15 },
	{ 0x3E, "ROL", 15 },
	{ 0x3F, "AND", 18 },
	{ 0x40, "RTI", 0 },
	{ 0x41, "EOR", 10 },
	{ 0x42, "WDM", 3 },
	{ 0x43, "EOR", 19 },
	{ 0x44, "MVP", 25 },
	{ 0x45, "EOR", 6 },
	{ 0x46, "LSR", 6 },
	{ 0x47, "EOR", 12 },
	{ 0x48, "PHA", 0 },
	{ 0x49, "EOR", 1 },
	{ 0x4A, "LSR", 24 },
	{ 0x4B, "PHK", 0 },
	{ 0x4C, "JMP", 14 },
	{ 0x4D, "EOR", 14 },
	{ 0x4E, "LSR", 14 },
	{ 0x4F, "EOR", 17 },
	{ 0x50, "BVC", 4 },
	{ 0x51, "EOR", 11 },
	{ 0x52, "EOR", 9 },
	{ 0x53, "EOR", 20 },
	{ 0x54, "MVN", 25 },
	{ 0x55, "EOR", 7 },
	{ 0x56, "LSR", 7 },
	{ 0x57, "EOR", 13 },
	{ 0x58, "CLI", 0 },
	{ 0x59, "EOR", 16 },
	{ 0x5A, "PHY", 0 },
	{ 0x5B, "TCD", 0 },
	{ 0x5C, "JMP", 17 },
	{ 0x5D, "EOR", 15 },
	{ 0x5E, "LSR", 15 },
	{ 0x5F, "EOR", 18 },
	{ 0x60, "RTS", 0 },
	{ 0x61, "ADC", 10 },
	{ 0x62, "PER", 5 },
	{ 0x63, "ADC", 19 },
	{ 0x64, "STZ", 6 },
	{ 0x65, "ADC", 6 },
	{ 0x66, "ROR", 6 },
	{ 0x67, "ADC", 12 },
	{ 0x68, "PLA", 0 },
	{ 0x69, "ADC", 1 },
	{ 0x6A, "ROR", 24 },
	{ 0x6B, "RTL", 0 },
	{ 0x6C, "JMP", 21 },
	{ 0x6D, "ADC", 14 },
	{ 0x6E, "ROR", 14 },
	{ 0x6F, "ADC", 17 },
	{ 0x70, "BVS", 4 },
	{ 0x71, "ADC", 11 },
	{ 0x72, "ADC", 9 },
	{ 0x73, "ADC", 20 },
	{ 0x74, "STZ", 7 },
	{ 0x75, "ADC", 7 },
	{ 0x76, "ROR", 7 },
	{ 0x77, "ADC", 13 },
	{ 0x78, "SEI", 0 },
	{ 0x79, "ADC", 16 },
	{ 0x7A, "PLY", 0 },
	{ 0x7B, "TDC", 0 },
	{ 0x7C, "JMP", 23 },
	{ 0x7D, "ADC", 15 },
	{ 0x7E, "ROR", 15 },
	{ 0x7F, "ADC", 18 },
	{ 0x80, "BRA", 4 },
	{ 0x81, "STA", 10 },
	{ 0x82, "BRL", 5 },
	{ 0x83, "STA", 19 },
	{ 0x84, "STY", 6 },
	{ 0x85, "STA", 6 },
	{ 0x86, "STX", 6 },
	{ 0x87, "STA", 12 },
	{ 0x88, "DEY", 0 },
	{ 0x89, "BIT", 1 },
	{ 0x8A, "TXA", 0 },
	{ 0x8B, "PHB", 0 },
	{ 0x8C, "STY", 14 },
	{ 0x8D, "STA", 14 },
	{ 0x8E, "STX", 14 },
	{ 0x8F, "STA", 17 },
	{ 0x90, "BCC", 4 },
	{ 0x91, "STA", 11 },
	{ 0x92, "STA", 9 },
	{ 0x93, "STA", 20 },
	{ 0x94, "STY", 7 },
	{ 0x95, "STA", 7 },
	{ 0x96, "STX", 8 },
	{ 0x97, "STA", 13 },
	{ 0x98, "TYA", 0 },
	{ 0x99, "STA", 16 },
	{ 0x9A, "TXS", 0 },
	{ 0x9B, "TXY", 0 },
	{ 0x9C, "STZ", 14 },
	{ 0x9D, "STA", 15 },
	{ 0x9E, "STZ", 15 },
	{ 0x9F, "STA", 18 },
	{ 0xA0, "LDY", 2 },
	{ 0xA1, "LDA", 10 },
	{ 0xA2, "LDX", 2 },
	{ 0xA3, "LDA", 19 },
	{ 0xA4, "LDY", 6 },
	{ 0xA5, "LDA", 6 },
	{ 0xA6, "LDX", 6 },
	{ 0xA7, "LDA", 12 },
	{ 0xA8, "TAY", 0 },
	{ 0xA9, "LDA", 1 },
	{ 0xAA, "TAX", 0 },
	{ 0xAB, "PLB", 0 },
	{ 0xAC, "LDY", 14 },
	{ 0xAD, "LDA", 14 },
	{ 0xAE, "LDX", 14 },
	{ 0xAF, "LDA", 17 },
	{ 0xB0, "BCS", 4 },
	{ 0xB1, "LDA", 11 },
	{ 0xB2, "LDA", 9 },
	{ 0xB3, "LDA", 20 },
	{ 0xB4, "LDY", 7 },
	{ 0xB5, "LDA", 7 },
	{ 0xB6, "LDX", 8 },
	{ 0xB7, "LDA", 13 },
	{ 0xB8, "CLV", 0 },
	{ 0xB9, "LDA", 16 },
	{ 0xBA, "TSX", 0 },
	{ 0xBB, "TYX", 0 },
	{ 0xBC, "LDY", 15 },
	{ 0xBD, "LDA", 15 },
	{ 0xBE, "LDX", 16 },
	{ 0xBF, "LDA", 18 },
	{ 0xC0, "CPY", 2 },
	{ 0xC1, "CMP", 10 },
	{ 0xC2, "REP", 3 },
	{ 0xC3, "CMP", 19 },
	{ 0xC4, "CPY", 6 },
	{ 0xC5, "CMP", 6 },
	{ 0xC6, "DEC", 6 },
	{ 0xC7, "CMP", 12 },
	{ 0xC8, "INY", 0 },
	{ 0xC9, "CMP", 1 },
	{ 0xCA, "DEX", 0 },
	{ 0xCB, "WAI", 0 },
	{ 0xCC, "CPY", 14 },
	{ 0xCD, "CMP", 14 },
	{ 0xCE, "DEC", 14 },
	{ 0xCF, "CMP", 17 },
	{ 0xD0, "BNE", 4 },
	{ 0xD1, "CMP", 11 },
	{ 0xD2, "CMP", 9 },
	{ 0xD3, "CMP", 20 },
	{ 0xD4, "PEI", 27 },
	{ 0xD5, "CMP", 7 },
	{ 0xD6, "DEC", 7 },
	{ 0xD7, "CMP", 13 },
	{ 0xD8, "CLD", 0 },
	{ 0xD9, "CMP", 16 },
	{ 0xDA, "PHX", 0 },
	{ 0xDB, "STP", 0 },
	{ 0xDC, "JML", 22 },
	{ 0xDD, "CMP", 15 },
	{ 0xDE, "DEC", 15 },
	{ 0xDF, "CMP", 18 },
	{ 0xE0, "CPX", 2 },
	{ 0xE1, "SBC", 10 },
	{ 0xE2, "SEP", 3 },
	{ 0xE3, "SBC", 19 },
	{ 0xE4, "CPX", 6 },
	{ 0xE5, "SBC", 6 },
	{ 0xE6, "INC", 6 },
	{ 0xE7, "SBC", 12 },
	{ 0xE8, "INX", 0 },
	{ 0xE9, "SBC", 1 },
	{ 0xEA, "NOP", 0 },
	{ 0xEB, "XBA", 0 },
	{ 0xEC, "CPX", 14 },
	{ 0xED, "SBC", 14 },
	{ 0xEE, "INC", 14 },
	{ 0xEF, "SBC", 17 },
	{ 0xF0, "BEQ", 4 },
	{ 0xF1, "SBC", 11 },
	{ 0xF2, "SBC", 9 },
	{ 0xF3, "SBC", 20 },
	{ 0xF4, "PEA", 26 },
	{ 0xF5, "SBC", 7 },
	{ 0xF6, "INC", 7 },
	{ 0xF7, "SBC", 13 },
	{ 0xF8, "SED", 0 },
	{ 0xF9, "SBC", 16 },
	{ 0xFA, "PLX", 0 },
	{ 0xFB, "XCE", 0 },
	{ 0xFC, "JSR", 23 },
	{ 0xFD, "SBC", 15 },
	{ 0xFE, "INC", 15 },
	{ 0xFF, "SBC", 18 }
};

bool branches[256];
bool jumps[256];

void initLookupTables() {
	for (int ih = 0; ih<256; ih++) {
		jumps[ih] = false;
		branches[ih] = false;
		const char * const i = opCodeInfo[ih].mnemonics;
		if (i[0] == 'J') {
			jumps[ih] = true;
		}
		else if (i[0] == 'B') {
			if (strcmp(i, "BRK") == 0) continue;
			if (strcmp(i, "BIT") == 0) continue;
			jumps[ih] = true;
			branches[ih] = true;
		}
	}
}

struct AdressModeInfo {
	int adressMode; // Just for readability, not used
	int numBytes;
	int numBitsForOpcode;
	const char * const formattingString;
	const char * const formattingWithLabelString;
};

static const AdressModeInfo g_oplut[] = {
	{ 0, 1, 0, "",					""},								
	{ 1, 3, 16, "#$%02X%02X",		""},				// special case for acc=8bit
	{ 2, 3, 16, "#$%02X%02X",		"" },				// special case for index=8bit
	{ 3, 2, 8, "#$%02X",			"" },				// special case for COP and BRK
	{ 4, 2, 8, "#$%02X", "%%s" },						// special case for branches (jumps or branch)
	{ 5, 3, 8, "$%02X%02X", "%%s" },
	{ 6, 2, 8, "$%02X", "" },
	{ 7, 2, 8, "$%02X, x", "" },
	{ 8, 2, 8, "$%02X,y", "" },
	{ 9, 2, 8, "($%02X)", "" },
	{ 10, 2, 8, "($%02X,x)", "" },
	{ 11, 2, 8, "($%02X),y", "" },
	{ 12, 2, 8, "[$%02X]", "" },
	{ 13, 2, 8, "[$%02X],y", "" },
	{ 14, 3, 16, "$%02X%02X", "" },
	{ 15, 3, 16, "$%02X%02X,x", "" },
	{ 16, 3, 16, "$%02X%02X,y", "" },
	{ 17, 4, 24, "$%02X%02X%02X", "%%s" },
	{ 18, 4, 24, "$%02X%02X%02X,x", "" },
	{ 19, 2, 8, "$%02X,s", "" },
	{ 20, 2, 8, "($%02X,s),y", "" },
	{ 21, 3, 16, "($%02X%02X)", "%%s" },
	{ 22, 3, 16, "[$%02X%02X]", "" },
	{ 23, 3, 16, "($%02X%02X,x)", "" },
	{ 24, 1, 0, "A", "" },
	{ 25, 3, 8, "$%02X, $%02X", "" },					// (reversed order?)
	{ 26, 3, 16, "$%02X%02X", "" },
	{ 27, 2, 8, "($%02X)", "" }
};

// Unpacked relative offset for branch operations
static int unpackSigned(const uint8_t packed) {
	if ((packed >= 0x80) != 0) {
		return packed - 256;
	}
	else {
		return packed;
	}
}

static int calculateFormattingandSize(const uint8_t *data, const bool acc16, const bool ind16, const bool emulationFlag, char *target, char *targetLabel, int *bitmodeNeeded) {
	const uint8_t opcode = data[0];
	const int am = opCodeInfo[opcode].adressMode;
	const AdressModeInfo &ami = g_oplut[am];
	*bitmodeNeeded = 8;
	
	// We have a few special cases that doesn't work with our simple table
	if (am == 1 && !acc16) {
		sprintf(target, "#$%02X", data[1]);
		return 2;
	} else if (am == 2 && !ind16) {
		sprintf(target, "#$%02X", data[1]);
		return 2;
	} else if (am == 3 && (opcode == 0 || opcode == 2)) {
		sprintf(target, "$%02X", data[1]);
		return 2;
	} else if (am == 4 && branches[opcode]) {
		const int signed_offset = unpackSigned(data[1]);
		if (signed_offset >= 0) {
			sprintf(target, "$%02X", abs(signed_offset));
		} else {
			sprintf(target, "-$%02X", abs(signed_offset));
		}
		sprintf(targetLabel, ami.formattingWithLabelString);
		return 2;
	} else {
		const int nb = ami.numBytes;
		const char * result = ami.formattingString;
		sprintf(target, result, data[nb-1], data[nb - 2], data[nb - 3]);
		sprintf(targetLabel, ami.formattingWithLabelString);
		*bitmodeNeeded = ami.numBitsForOpcode;
		return nb;
	}
}

// Flags are "sticky"... true=bit unknown
template<typename T>
struct MagicT {
	MagicT(const T _value) : value(_value), flags(0) {}
	MagicT(const T _value, const T _flags) : value(_value), flags(_flags) {}

	T value;
	T flags; // Track if bits are known or unknown. 0=

	void operator=(const T &other) {
		value = other;
		flags = 0; // assume uint type, all are known
	}

	// Operation by sure numbers
	MagicT operator+(const uint32_t knownValue) const { return MagicT(value + knownValue, flags); }
	MagicT operator&(const size_t v) const { return MagicT(value & v, flags & v); }
	MagicT operator|(const size_t v) const { return MagicT(value & v, flags & v); }

	// Operation with unsure members
	MagicT operator|(const MagicT &other) const {
		return (value | other.value, flags | other.flags);
	}
	MagicT operator&(const MagicT &other) const {
		return (value & other.value, flags | other.flags);
	}

	MagicT operator<<(const size_t steps) const {
		MagicT result;
		result.value = value << steps;
		result.flags = flags << steps;
		return result;
	}
	MagicT operator>>(const size_t steps) const {
		MagicT result;
		result.value = value >> steps;
		result.flags = flags >> steps;
		return result;
	}

	bool isKnown() const {
		return flags == 0;
	}
};

typedef MagicT<uint8_t> MagicByte;
typedef MagicT<uint16_t> MagicWord;

struct Registers {
	Registers() : P(0x1FF), pb(-1), pc(-1), db(-1), dp(-1), reg_A(-1), reg_X(-1), reg_Y(-1) {}
	MagicWord P; // processor status
	MagicByte pb;
	MagicWord pc;
	MagicByte db;
	MagicWord dp;
	MagicWord reg_A;
	MagicWord reg_X;
	MagicWord reg_Y;
};

enum ResultType {
	SA_IMMEDIATE,
	SA_ADRESS,
	SA_ACCUMULATOR,
	SA_NOT_IMPLEMENTED
};

ResultType evaluateOp(const uint8_t* ops, const Registers &reg, MagicByte *resultBnk, MagicWord *resultAdr) {

	// Introduce all registers with known flags.. then calculate adress with values... and check flags to see if it was "determined"

	// TODO: Some of these are based on the fact that we want to do an indirection via memory...
	//       but alot of memory is known (such as all jump tables). Involve ROM.
	//       For this to work we must differentiate between source adress and the pointer read...

	const int am = opCodeInfo[ops[0]].adressMode;
	if (am >= 0 && am <= 3) {
		return SA_IMMEDIATE;
	} else if (am == 4) {
		const int signed_offset = unpackSigned(ops[1]);
		*resultBnk = reg.pb;
		*resultAdr = reg.pc + (signed_offset + 2);
		return SA_ADRESS;
	}
	else if (am == 5) {
		const Pointer relative(ops[2] * 256 + ops[1]);
		*resultBnk = reg.pb;
		*resultAdr = reg.pc + relative + 3;
		return SA_ADRESS;
	}
	else if (am == 6) {
		*resultBnk = 0x00;
		*resultAdr = reg.dp + ops[1];
		return SA_ADRESS;
	}
	else if (am == 13) {
		// TODO: Would be cool to not only support this, but also give the indirection pointer... it probably points out a jump table or so
		return SA_NOT_IMPLEMENTED;
	}
	else if (am == 14) {
		*resultBnk = reg.db;
		*resultAdr = (ops[2] << 8) | ops[1];
		return SA_ADRESS;
	}
	else if (am == 15) {
		*resultBnk = reg.db;
		*resultAdr = reg.reg_X + ((ops[2] << 8) | ops[1]); // TODO: Must X be 16-bit for this op?
		return SA_ADRESS;
	} else if (am == 16) {
		*resultBnk = reg.db;
		*resultAdr = reg.reg_Y + ((ops[2] << 8) | ops[1]); // TODO: Must Y be 16-bit for this op?
		return SA_ADRESS;
	}
	else if (am == 17) {
		*resultBnk = ops[3];
		*resultAdr = (ops[2] << 8) | ops[1];
		return SA_ADRESS;
	}
	else if (am == 18) {
		*resultBnk = ops[3];
		*resultAdr = reg.reg_X + (ops[2] << 8) | ops[1];
		return SA_ADRESS;
	}
	else if (am == 22) {
		return SA_NOT_IMPLEMENTED;
	} 
	else if (am == 24) {
		return SA_ACCUMULATOR;
 	} else {

		static size_t bangBuck[27];
		static bool init = true;
		static size_t mmm = 0;
		static size_t oldWinner = 0;
		if (init) {
			memset(bangBuck, 0, sizeof(size_t)* 27);
		}
		bangBuck[am]++;
		if (bangBuck[am] > mmm) {
			mmm = bangBuck[am];

			if (oldWinner != am) {

				printf("am %d leading with %d\n", am, bangBuck[am]);
			}
			oldWinner = am;
		}

		return SA_NOT_IMPLEMENTED;
	}
}



#endif //  SNESTISTICS_CPUTABLE