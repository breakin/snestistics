#ifndef SNESTISTICS_CPUTABLE
#define SNESTISTICS_CPUTABLE

#include <stdint.h>

#define STATUS_EMULATION_FLAG (256)
#define STATUS_MEMORY_FLAG (0x10)
#define STATUS_ACCUMULATOR_FLAG (0x20)

bool branches[256];
bool jumps[256];


// From snes9x (one should be INA)
static const char	*S9xMnemonics[256] =
{
	"BRK", "ORA", "COP", "ORA", "TSB", "ORA", "ASL", "ORA",
	"PHP", "ORA", "ASL", "PHD", "TSB", "ORA", "ASL", "ORA",
	"BPL", "ORA", "ORA", "ORA", "TRB", "ORA", "ASL", "ORA",
	"CLC", "ORA", "INC", "TCS", "TRB", "ORA", "ASL", "ORA",
	"JSR", "AND", "JSL", "AND", "BIT", "AND", "ROL", "AND",
	"PLP", "AND", "ROL", "PLD", "BIT", "AND", "ROL", "AND",
	"BMI", "AND", "AND", "AND", "BIT", "AND", "ROL", "AND",
	"SEC", "AND", "DEC", "TSC", "BIT", "AND", "ROL", "AND",
	"RTI", "EOR", "WDM", "EOR", "MVP", "EOR", "LSR", "EOR",
	"PHA", "EOR", "LSR", "PHK", "JMP", "EOR", "LSR", "EOR",
	"BVC", "EOR", "EOR", "EOR", "MVN", "EOR", "LSR", "EOR",
	"CLI", "EOR", "PHY", "TCD", "JMP", "EOR", "LSR", "EOR",
	"RTS", "ADC", "PER", "ADC", "STZ", "ADC", "ROR", "ADC",
	"PLA", "ADC", "ROR", "RTL", "JMP", "ADC", "ROR", "ADC",
	"BVS", "ADC", "ADC", "ADC", "STZ", "ADC", "ROR", "ADC",
	"SEI", "ADC", "PLY", "TDC", "JMP", "ADC", "ROR", "ADC",
	"BRA", "STA", "BRL", "STA", "STY", "STA", "STX", "STA",
	"DEY", "BIT", "TXA", "PHB", "STY", "STA", "STX", "STA",
	"BCC", "STA", "STA", "STA", "STY", "STA", "STX", "STA",
	"TYA", "STA", "TXS", "TXY", "STZ", "STA", "STZ", "STA",
	"LDY", "LDA", "LDX", "LDA", "LDY", "LDA", "LDX", "LDA",
	"TAY", "LDA", "TAX", "PLB", "LDY", "LDA", "LDX", "LDA",
	"BCS", "LDA", "LDA", "LDA", "LDY", "LDA", "LDX", "LDA",
	"CLV", "LDA", "TSX", "TYX", "LDY", "LDA", "LDX", "LDA",
	"CPY", "CMP", "REP", "CMP", "CPY", "CMP", "DEC", "CMP",
	"INY", "CMP", "DEX", "WAI", "CPY", "CMP", "DEC", "CMP",
	"BNE", "CMP", "CMP", "CMP", "PEI", "CMP", "DEC", "CMP",
	"CLD", "CMP", "PHX", "STP", "JML", "CMP", "DEC", "CMP",
	"CPX", "SBC", "SEP", "SBC", "CPX", "SBC", "INC", "SBC",
	"INX", "SBC", "NOP", "XBA", "CPX", "SBC", "INC", "SBC",
	"BEQ", "SBC", "SBC", "SBC", "PEA", "SBC", "INC", "SBC",
	"SED", "SBC", "PLX", "XCE", "JSR", "SBC", "INC", "SBC"
};

void initLookupTables() {
	for (int ih = 0; ih<256; ih++) {
		jumps[ih] = false;
		branches[ih] = false;
		const char * const i = S9xMnemonics[ih];
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


static int adressModes[256] =
{
	3,	// 0x00 [0]
	10,	// 0x01 [1]
	3,	// 0x02 [2]
	19,	// 0x03 [3]
	6,	// 0x04 [4]
	6,	// 0x05 [5]
	6,	// 0x06 [6]
	12,	// 0x07 [7]
	0,	// 0x08 [8]
	1,	// 0x09 [9]
	24,	// 0x0A [10]
	0,	// 0x0B [11]
	14,	// 0x0C [12]
	14,	// 0x0D [13]
	14,	// 0x0E [14]
	17,	// 0x0F [15]
	4,	// 0x10 [16]
	11,	// 0x11 [17]
	9,	// 0x12 [18]
	20,	// 0x13 [19]
	6,	// 0x14 [20]
	7,	// 0x15 [21]
	7,	// 0x16 [22]
	13,	// 0x17 [23]
	0,	// 0x18 [24]
	16,	// 0x19 [25]
	24,	// 0x1A [26]
	0,	// 0x1B [27]
	14,	// 0x1C [28]
	15,	// 0x1D [29]
	15,	// 0x1E [30]
	18,	// 0x1F [31]
	14,	// 0x20 [32]
	10,	// 0x21 [33]
	17,	// 0x22 [34]
	19,	// 0x23 [35]
	6,	// 0x24 [36]
	6,	// 0x25 [37]
	6,	// 0x26 [38]
	12,	// 0x27 [39]
	0,	// 0x28 [40]
	1,	// 0x29 [41]
	24,	// 0x2A [42]
	0,	// 0x2B [43]
	14,	// 0x2C [44]
	14,	// 0x2D [45]
	14,	// 0x2E [46]
	17,	// 0x2F [47]
	4,	// 0x30 [48]
	11,	// 0x31 [49]
	9,	// 0x32 [50]
	20,	// 0x33 [51]
	7,	// 0x34 [52]
	7,	// 0x35 [53]
	7,	// 0x36 [54]
	13,	// 0x37 [55]
	0,	// 0x38 [56]
	16,	// 0x39 [57]
	24,	// 0x3A [58]
	0,	// 0x3B [59]
	15,	// 0x3C [60]
	15,	// 0x3D [61]
	15,	// 0x3E [62]
	18,	// 0x3F [63]
	0,	// 0x40 [64]
	10,	// 0x41 [65]
	3,	// 0x42 [66]
	19,	// 0x43 [67]
	25,	// 0x44 [68]
	6,	// 0x45 [69]
	6,	// 0x46 [70]
	12,	// 0x47 [71]
	0,	// 0x48 [72]
	1,	// 0x49 [73]
	24,	// 0x4A [74]
	0,	// 0x4B [75]
	14,	// 0x4C [76]
	14,	// 0x4D [77]
	14,	// 0x4E [78]
	17,	// 0x4F [79]
	4,	// 0x50 [80]
	11,	// 0x51 [81]
	9,	// 0x52 [82]
	20,	// 0x53 [83]
	25,	// 0x54 [84]
	7,	// 0x55 [85]
	7,	// 0x56 [86]
	13,	// 0x57 [87]
	0,	// 0x58 [88]
	16,	// 0x59 [89]
	0,	// 0x5A [90]
	0,	// 0x5B [91]
	17,	// 0x5C [92]
	15,	// 0x5D [93]
	15,	// 0x5E [94]
	18,	// 0x5F [95]
	0,	// 0x60 [96]
	10,	// 0x61 [97]
	5,	// 0x62 [98]
	19,	// 0x63 [99]
	6,	// 0x64 [100]
	6,	// 0x65 [101]
	6,	// 0x66 [102]
	12,	// 0x67 [103]
	0,	// 0x68 [104]
	1,	// 0x69 [105]
	24,	// 0x6A [106]
	0,	// 0x6B [107]
	21,	// 0x6C [108]
	14,	// 0x6D [109]
	14,	// 0x6E [110]
	17,	// 0x6F [111]
	4,	// 0x70 [112]
	11,	// 0x71 [113]
	9,	// 0x72 [114]
	20,	// 0x73 [115]
	7,	// 0x74 [116]
	7,	// 0x75 [117]
	7,	// 0x76 [118]
	13,	// 0x77 [119]
	0,	// 0x78 [120]
	16,	// 0x79 [121]
	0,	// 0x7A [122]
	0,	// 0x7B [123]
	23,	// 0x7C [124]
	15,	// 0x7D [125]
	15,	// 0x7E [126]
	18,	// 0x7F [127]
	4,	// 0x80 [128]
	10,	// 0x81 [129]
	5,	// 0x82 [130]
	19,	// 0x83 [131]
	6,	// 0x84 [132]
	6,	// 0x85 [133]
	6,	// 0x86 [134]
	12,	// 0x87 [135]
	0,	// 0x88 [136]
	1,	// 0x89 [137]
	0,	// 0x8A [138]
	0,	// 0x8B [139]
	14,	// 0x8C [140]
	14,	// 0x8D [141]
	14,	// 0x8E [142]
	17,	// 0x8F [143]
	4,	// 0x90 [144]
	11,	// 0x91 [145]
	9,	// 0x92 [146]
	20,	// 0x93 [147]
	7,	// 0x94 [148]
	7,	// 0x95 [149]
	8,	// 0x96 [150]
	13,	// 0x97 [151]
	0,	// 0x98 [152]
	16,	// 0x99 [153]
	0,	// 0x9A [154]
	0,	// 0x9B [155]
	14,	// 0x9C [156]
	15,	// 0x9D [157]
	15,	// 0x9E [158]
	18,	// 0x9F [159]
	2,	// 0xA0 [160]
	10,	// 0xA1 [161]
	2,	// 0xA2 [162]
	19,	// 0xA3 [163]
	6,	// 0xA4 [164]
	6,	// 0xA5 [165]
	6,	// 0xA6 [166]
	12,	// 0xA7 [167]
	0,	// 0xA8 [168]
	1,	// 0xA9 [169]
	0,	// 0xAA [170]
	0,	// 0xAB [171]
	14,	// 0xAC [172]
	14,	// 0xAD [173]
	14,	// 0xAE [174]
	17,	// 0xAF [175]
	4,	// 0xB0 [176]
	11,	// 0xB1 [177]
	9,	// 0xB2 [178]
	20,	// 0xB3 [179]
	7,	// 0xB4 [180]
	7,	// 0xB5 [181]
	8,	// 0xB6 [182]
	13,	// 0xB7 [183]
	0,	// 0xB8 [184]
	16,	// 0xB9 [185]
	0,	// 0xBA [186]
	0,	// 0xBB [187]
	15,	// 0xBC [188]
	15,	// 0xBD [189]
	16,	// 0xBE [190]
	18,	// 0xBF [191]
	2,	// 0xC0 [192]
	10,	// 0xC1 [193]
	3,	// 0xC2 [194]
	19,	// 0xC3 [195]
	6,	// 0xC4 [196]
	6,	// 0xC5 [197]
	6,	// 0xC6 [198]
	12,	// 0xC7 [199]
	0,	// 0xC8 [200]
	1,	// 0xC9 [201]
	0,	// 0xCA [202]
	0,	// 0xCB [203]
	14,	// 0xCC [204]
	14,	// 0xCD [205]
	14,	// 0xCE [206]
	17,	// 0xCF [207]
	4,	// 0xD0 [208]
	11,	// 0xD1 [209]
	9,	// 0xD2 [210]
	20,	// 0xD3 [211]
	27,	// 0xD4 [212]
	7,	// 0xD5 [213]
	7,	// 0xD6 [214]
	13,	// 0xD7 [215]
	0,	// 0xD8 [216]
	16,	// 0xD9 [217]
	0,	// 0xDA [218]
	0,	// 0xDB [219]
	22,	// 0xDC [220]
	15,	// 0xDD [221]
	15,	// 0xDE [222]
	18,	// 0xDF [223]
	2,	// 0xE0 [224]
	10,	// 0xE1 [225]
	3,	// 0xE2 [226]
	19,	// 0xE3 [227]
	6,	// 0xE4 [228]
	6,	// 0xE5 [229]
	6,	// 0xE6 [230]
	12,	// 0xE7 [231]
	0,	// 0xE8 [232]
	1,	// 0xE9 [233]
	0,	// 0xEA [234]
	0,	// 0xEB [235]
	14,	// 0xEC [236]
	14,	// 0xED [237]
	14,	// 0xEE [238]
	17,	// 0xEF [239]
	4,	// 0xF0 [240]
	11,	// 0xF1 [241]
	9,	// 0xF2 [242]
	20,	// 0xF3 [243]
	26,	// 0xF4 [244]
	7,	// 0xF5 [245]
	7,	// 0xF6 [246]
	13,	// 0xF7 [247]
	0,	// 0xF8 [248]
	16,	// 0xF9 [249]
	0,	// 0xFA [250]
	0,	// 0xFB [251]
	23,	// 0xFC [252]
	15,	// 0xFD [253]
	15,	// 0xFE [254]
	18	// 0xFF [255]
};

struct AdressModeInfo {
	int adressMode; // Just for readability, not used
	int numBytes;
	int numBitsForOpcode;
	const char * const formattingString;
	const char * const formattingWithLabelString;
};

static const AdressModeInfo g_oplut[] = {
	{ 0, 1, 8, "",					""},								
	{ 1, 3, 8, "#$%02X%02X.W",		""},					// special case for acc=8bit
	{ 2, 3, 8, "#$%02X%02X.W",		"" },					// special case for index=8bit
	{ 3, 2, 8, "#$%02X",			"" },							// special case for COP and BRK
	{ 4, 2, 8, "#$%02X", "%%s" },							// special case for branches (jumps or branch)
	{ 5, 3, 8, "$%02X%02X", "%%s" },
	{ 6, 2, 8, "$%02X", "" },
	{ 7, 2, 8, "$%02X, x", "" },
	{ 8, 2, 8, "$%02X,y", "" },
	{ 9, 2, 8, "($%02X.B)", "" },
	{ 10, 2, 8, "($%02X.B,x)", "" },
	{ 11, 2, 8, "($%02X.B),y", "" },
	{ 12, 2, 8, "[$%02X.B]", "" },
	{ 13, 2, 8, "[$%02X.B],y", "" },
	{ 14, 3, 8, "$%02X%02X.W", "" },
	{ 15, 3, 8, "$%02X%02X.W,x", "" },
	{ 16, 3, 8, "$%02X%02X.W,y", "" },
	{ 17, 4, 8, "$%02X%02X%02X", "%%s" },
	{ 18, 4, 8, "$%02X%02X%02X,x", "" },
	{ 19, 2, 8, "$%02X,s", "" },
	{ 20, 2, 8, "($%02X,s),y", "" },
	{ 21, 3, 8, "($%02X%02X.W)", "%%s" },
	{ 22, 3, 8, "[$%02X%02X.W]", "" },
	{ 23, 3, 8, "($%02X%02X.W,x)", "" },
	{ 24, 1, 8, "A", "" },
	{ 25, 3, 8, "$%02X, $%02X", "" },					// (reversed order?)
	{ 26, 3, 8, "$%02X%02X.W", "" },
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

static int calculateFormattingandSize(const uint8_t *data, const bool acc16, const bool ind16, const bool emulationFlag, char *target, char *targetLabel) {
	const uint8_t opcode = data[0];
	const int am = adressModes[opcode];
	
	// We have a few special cases that doesn't work with our simple table
	if (am == 1 && !acc16) {
		sprintf(target, "#$%02X.B", data[1]);
		return 2;
	} else if (am == 2 && !ind16) {
		sprintf(target, "#$%02X.B", data[1]);
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
		sprintf(targetLabel, g_oplut[am].formattingWithLabelString);
		return 2;
	} else {
		const int nb = g_oplut[am].numBytes;
		const char * result = g_oplut[am].formattingString;
		sprintf(target, result, data[nb-1], data[nb - 2], data[nb - 3]);
		sprintf(targetLabel, g_oplut[am].formattingWithLabelString);
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

ResultType evaluateOp(const uint8_t* ops, 
	const Registers &reg,
	MagicByte *resultBnk,
	MagicWord *resultAdr) {
	// Introduce all registers with known flags.. then calculate adress with values... and check flags to see if it was "determined"

	const int am = adressModes[ops[0]];
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