#ifndef SNESTISTICS_SNESOPS
#define SNESTISTICS_SNESOPS

#define STATUS_EMULATION_FLAG (256)
#define STATUS_MEMORY_FLAG (0x10)
#define STATUS_ACCUMULATOR_FLAG (0x20)

typedef uint32_t Pointer;

uint8_t bank(const Pointer p) { return p>>16; }
uint16_t adr(const Pointer p) { return p&0xffff; }

// Unpacked relative offset for branch operations
int unpackSigned(const uint8_t packed) {
	if ((packed >= 0x80) != 0) {
		return packed - 256;
	} else {
		return packed;
	}
}

// TODO: This function can probably be removed for 8mbit LoROM
static uint32_t map_mirror (uint32_t size, uint32_t pos)
{
	// from bsnes
	if (size == 0)
		return (0);
	if (pos < size)
		return (pos);

	uint32_t mask = 1 << 31;
	while (!(pos & mask))
		mask >>= 1;

	if (size <= (pos & mask))
		return (map_mirror(size, pos - mask));
	else
		return (mask + map_mirror(size - mask, pos - mask));
}

size_t getRomOffset(const Pointer &pointer, const size_t calculatedSize) {
	const uint32_t a = (bank(pointer) & 0x7f) * 0x8000;
	const uint32_t mirrorAddr = map_mirror(calculatedSize, a);
	return mirrorAddr - (adr(pointer) & 0x8000) + adr(pointer);
}

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

// From snes9x
static int	AddrModes[256] =
{
  // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	 3, 10,  3, 19,  6,  6,  6, 12,  0,  1, 24,  0, 14, 14, 14, 17, // 0
	 4, 11,  9, 20,  6,  7,  7, 13,  0, 16, 24,  0, 14, 15, 15, 18, // 1
	14, 10, 17, 19,  6,  6,  6, 12,  0,  1, 24,  0, 14, 14, 14, 17, // 2
	 4, 11,  9, 20,  7,  7,  7, 13,  0, 16, 24,  0, 15, 15, 15, 18, // 3
	 0, 10,  3, 19, 25,  6,  6, 12,  0,  1, 24,  0, 14, 14, 14, 17, // 4
	 4, 11,  9, 20, 25,  7,  7, 13,  0, 16,  0,  0, 17, 15, 15, 18, // 5
	 0, 10,  5, 19,  6,  6,  6, 12,  0,  1, 24,  0, 21, 14, 14, 17, // 6
	 4, 11,  9, 20,  7,  7,  7, 13,  0, 16,  0,  0, 23, 15, 15, 18, // 7
	 4, 10,  5, 19,  6,  6,  6, 12,  0,  1,  0,  0, 14, 14, 14, 17, // 8
	 4, 11,  9, 20,  7,  7,  8, 13,  0, 16,  0,  0, 14, 15, 15, 18, // 9
	 2, 10,  2, 19,  6,  6,  6, 12,  0,  1,  0,  0, 14, 14, 14, 17, // A
	 4, 11,  9, 20,  7,  7,  8, 13,  0, 16,  0,  0, 15, 15, 16, 18, // B
	 2, 10,  3, 19,  6,  6,  6, 12,  0,  1,  0,  0, 14, 14, 14, 17, // C
	 4, 11,  9, 20 /* fixed from snes9x, was 9 */ , 27,  7,  7, 13,  0, 16,  0,  0, 22, 15, 15, 18, // D
	 2, 10,  3, 19,  6,  6,  6, 12,  0,  1,  0,  0, 14, 14, 14, 17, // E
	 4, 11,  9, 20, 26,  7,  7, 13,  0, 16,  0,  0, 23, 15, 15, 18  // F
};

bool branches[256];
bool jumps[256];
bool endOfPrediction[256];

 void initLookupTables() {
	for (int ih=0; ih<256; ih++) {
		endOfPrediction[ih]=false;
    
		if (strcmp(S9xMnemonics[ih], "RTS")==0) { endOfPrediction[ih]=true; }
        if (strcmp(S9xMnemonics[ih], "RTL")==0) { endOfPrediction[ih]=true; }
        if (strcmp(S9xMnemonics[ih], "RTI")==0) { endOfPrediction[ih]=true; }
		if (strcmp(S9xMnemonics[ih], "JMP")==0) { endOfPrediction[ih]=true; }
		if (strcmp(S9xMnemonics[ih], "BRA")==0) { endOfPrediction[ih]=true; }

		jumps[ih]=false;
        branches[ih] = false;
		const char * const i = S9xMnemonics[ih];
		if (i[0]=='J') {
			jumps[ih]=true;
		} else if (i[0]=='B') {
			if (strcmp(i, "BRK")==0) continue;
			if (strcmp(i, "BIT")==0) continue;
			jumps[ih]=true;
            branches[ih] = true;
		}
	}
}

int processArg(const Pointer pc, const uint8_t ih, const uint8_t *ops, char *pretty, char *labelPretty, Pointer *dest, const uint16_t registerP, int *needBits) {

	if (needBits) { *needBits = 8; }
	
	// TODO: Quite a few .B and .W can be removed. Only needed for ops that are ambigious

	const int am = AddrModes[ih];

	const bool acc16   = (registerP & STATUS_ACCUMULATOR_FLAG) == 0;
	const bool index16 = (registerP & STATUS_MEMORY_FLAG) == 0;
	
	if (am==0) {
		sprintf(pretty,  "");
		return 1;
	} else if (am==1) {
		if (acc16) {
			sprintf(pretty, "#$%02X%02X.W", ops[2], ops[1]);
			return 3;
		} else {
			sprintf(pretty, "#$%02X.B", ops[1]);
			return 2;
		}
	} else if (am==2) {

		if (index16) {
			sprintf(pretty, "#$%02X%02X.W", ops[2], ops[1]);
			return 3;
		} else {
			sprintf(pretty, "#$%02X.B", ops[1]);
			return 2;
		}
	} else if (am==3) {

		assert(!branches[ih]);

		if (strcmp(S9xMnemonics[ih],"COP")==0) {
			sprintf(pretty, "$%02X", ops[1]);
		} else if (strcmp(S9xMnemonics[ih],"BRK")==0) {
			sprintf(pretty, "$%02X", ops[1]);
		} else {
			sprintf(pretty, "#$%02X", ops[1]);
		}
		
		return 2;

	} else if (am==4) {
        
        if (branches[ih]) {

			const int signed_offset = unpackSigned(ops[1]);
			*dest = pc + signed_offset + 2;
            
            if(signed_offset>=0) {
                sprintf(pretty, "$%02X", abs(signed_offset));
            } else {
                sprintf(pretty, "-$%02X", abs(signed_offset));
            }
        } else {
            sprintf(pretty, "#$%02X", ops[1]);
        }
        
		sprintf(labelPretty, "%%s");
		return 2;
	} else if (am==5) {
		const Pointer relative(ops[2]*256+ops[1]);
		*dest = pc + relative + 3;
		sprintf(pretty, "$%02X%02X", ops[2], ops[1]);
		sprintf(labelPretty, "%%s");
		return 3;
	} else if (am==6) {
		sprintf(pretty, "$%02X", ops[1]);
		return 2;
	} else if (am==7) {
		sprintf(pretty, "$%02X,x", ops[1]);
		return 2;
	} else if (am==8) {
		sprintf(pretty, "$%02X,y", ops[1]);
		return 2;
	} else if (am==9) {
		sprintf(pretty, "($%02X.B)", ops[1]);
		return 2;
	} else if (am==10) {
		sprintf(pretty, "($%02X.B,x)", ops[1]);
		return 2;
	} else if (am==11) {
		sprintf(pretty, "($%02X.B),y", ops[1]);
		return 2;
	} else if (am==12) {
		sprintf(pretty, "[$%02X.B]", ops[1]);
		return 2;
	} else if (am==13) {
		sprintf(pretty, "[$%02X.B],y", ops[1]);
		return 2;
	} else if (am==14) {
		sprintf(pretty, "$%02X%02X.W", ops[2],ops[1]); // depend DB
		return 3;
	} else if (am==15) {
		sprintf(pretty, "$%02X%02X.W,x", ops[2],ops[1]); // depend x
		return 3;
	} else if (am==16) {
		sprintf(pretty, "$%02X%02X.W,y", ops[2], ops[1]); // depend y
        return 3;
	} else if (am==17) {
		*dest = (ops[3]<<16)|(ops[2]<<8)|(ops[1]),
		sprintf(pretty, "$%06X", *dest);
		sprintf(labelPretty, "%%s");
		if (needBits) { *needBits = 24; }
		return 4;
	} else if (am==18) {
		sprintf(pretty, "$%02X%02X%02X,x", ops[3], ops[2], ops[1]);
		if (needBits) { *needBits = 24; }
		return 4;
	} else if (am==19) {
		sprintf(pretty, "$%02X,s", ops[1]);
		return 2;
	} else if (am==20) {
		sprintf(pretty, "($%02X,s),y", ops[1]);
		return 2;
	} else if (am==21) {
		sprintf(pretty, "($%02X%02X.W)", ops[2], ops[1]);
		sprintf(labelPretty, "%%s");
        *dest = (pc & 0xff0000) | (ops[2]<<8) | (ops[1]); // TODO: Is PB what I think it is?
		return 3;		
	} else if (am==22) {
		sprintf(pretty, "[$%02X%02X.W]", ops[2], ops[1]);
		return 3;
	} else if (am==23) {
		sprintf(pretty, "($%02X%02X.W,x)", ops[2], ops[1]);
		return 3;
	} else if (am==24) {
		sprintf(pretty, "A");
		return 1;
	} else if (am==25) {
		sprintf(pretty, "$%02X, $%02X", ops[1], ops[2]);
		return 3;		 
	} else if (am==26) {
		sprintf(pretty, "$%02X%02X.W", ops[2], ops[1]);
		return 3;		 
	} else if (am==27) {
		sprintf(pretty, "($%02X)", ops[1]);
		return 2;		
	} else {
		assert(false);
		exit(42);
		return false;
	}
	return true;
}


#endif // SNESTISTICS_SNESOPS