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
		const char * const i = snestistics::mnemonic_names[ih];
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
	{ 4, 2, 8, "#$%02X", "%s" },						// special case for branches (jumps or branch)
	{ 5, 3, 8, "$%02X%02X", "%s" },
	{ 6, 2, 8, "$%02X", "<%s" },	// TODO:FIX; <%%s is a hack since it assumes direct page = 0, at least validate that <label == the value...
	{ 7, 2, 8, "$%02X,x", "" },
	{ 8, 2, 8, "$%02X,y", "" },
	{ 9, 2, 8, "($%02X)", "" },
	{ 10, 2, 8, "($%02X,x)", "" },
	{ 11, 2, 8, "($%02X),y", "" },
	{ 12, 2, 8, "[$%02X]", "" },
	{ 13, 2, 8, "[$%02X],y", "" },
	{ 14, 3, 16, "$%02X%02X", "%s" },	// TODO:FIX; <%%s is a hack since it assumes data bank = :label.
	{ 15, 3, 16, "$%02X%02X,x", "" },
	{ 16, 3, 16, "$%02X%02X,y", "" },
	{ 17, 4, 24, "$%02X%02X%02X", "%s" },
	{ 18, 4, 24, "$%02X%02X%02X,x", "" },
	{ 19, 2, 8, "$%02X,s", "" },
	{ 20, 2, 8, "($%02X,s),y", "" },
	{ 21, 3, 16, "($%02X%02X)", ""},
	{ 22, 3, 16, "[$%02X%02X]", "" },
	{ 23, 3, 16, "($%02X%02X,x)", "" },
	{ 24, 1, 0, "A", "" },
	{ 25, 3, 8, "$%02X, $%02X", "" },					// WLA DX wants byte order reversed for this one...
	{ 26, 3, 16, "$%02X%02X", "" },
	{ 27, 2, 8, "($%02X)", "" },
	{ 28, 3, 16, "$%02X%02X", "%s" }, // same as 14 but for jumps (pb instead of db)
	{ 29, 3, 16, "$%02X%02X,x", "" }, // same as 15 but for jumps (pb instead of db)
};

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
