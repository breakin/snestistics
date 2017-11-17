import operands
import json

manual_table="""
	opcodes[0x24] = {Operation::BIT,    Operand::DIRECT_PAGE,            OperandSize::MEMORY};
	opcodes[0x2C] = {Operation::BIT,    Operand::ABSOLUTE,               OperandSize::MEMORY};
	opcodes[0x34] = {Operation::BIT,    Operand::DIRECT_PAGE_INDEXED_X,  OperandSize::MEMORY};
	opcodes[0x3C] = {Operation::BIT,    Operand::ABSOLUTE_INDEXED_X,     OperandSize::MEMORY};
	opcodes[0x89] = {Operation::BIT,    Operand::IMMEDIATE_MEMORY,       OperandSize::MEMORY};
	opcodes[0x1A] = {Operation::INC,    Operand::ACCUMULATOR,            OperandSize::MEMORY};
	opcodes[0xE6] = {Operation::INC,    Operand::DIRECT_PAGE,            OperandSize::MEMORY};
	opcodes[0xEE] = {Operation::INC,    Operand::ABSOLUTE,               OperandSize::MEMORY};
	opcodes[0xF6] = {Operation::INC,    Operand::DIRECT_PAGE_INDEXED_X,  OperandSize::MEMORY};
	opcodes[0xFE] = {Operation::INC,    Operand::ABSOLUTE_INDEXED_X,     OperandSize::MEMORY};
	opcodes[0x3A] = {Operation::DEC,    Operand::ACCUMULATOR,            OperandSize::MEMORY};
	opcodes[0xC6] = {Operation::DEC,    Operand::DIRECT_PAGE,            OperandSize::MEMORY};
	opcodes[0xCE] = {Operation::DEC,    Operand::ABSOLUTE,               OperandSize::MEMORY};
	opcodes[0xD6] = {Operation::DEC,    Operand::DIRECT_PAGE_INDEXED_X,  OperandSize::MEMORY};
	opcodes[0xDE] = {Operation::DEC,    Operand::ABSOLUTE_INDEXED_X,     OperandSize::MEMORY};
	opcodes[0x26] = {Operation::ROL,    Operand::DIRECT_PAGE,            OperandSize::MEMORY};
	opcodes[0x2A] = {Operation::ROL,    Operand::ACCUMULATOR,            OperandSize::MEMORY};
	opcodes[0x2E] = {Operation::ROL,    Operand::ABSOLUTE,               OperandSize::MEMORY};
	opcodes[0x36] = {Operation::ROL,    Operand::DIRECT_PAGE_INDEXED_X,  OperandSize::MEMORY};
	opcodes[0x3E] = {Operation::ROL,    Operand::ABSOLUTE_INDEXED_X,     OperandSize::MEMORY};
	opcodes[0x66] = {Operation::ROR,    Operand::DIRECT_PAGE,            OperandSize::MEMORY};
	opcodes[0x6A] = {Operation::ROR,    Operand::ACCUMULATOR,            OperandSize::MEMORY};
	opcodes[0x6E] = {Operation::ROR,    Operand::ABSOLUTE,               OperandSize::MEMORY};
	opcodes[0x76] = {Operation::ROR,    Operand::DIRECT_PAGE_INDEXED_X,  OperandSize::MEMORY};
	opcodes[0x7E] = {Operation::ROR,    Operand::ABSOLUTE_INDEXED_X,     OperandSize::MEMORY};
	opcodes[0x06] = {Operation::ASL,    Operand::DIRECT_PAGE,            OperandSize::MEMORY};
	opcodes[0x0A] = {Operation::ASL,    Operand::ACCUMULATOR,            OperandSize::MEMORY};
	opcodes[0x0E] = {Operation::ASL,    Operand::ABSOLUTE,               OperandSize::MEMORY};
	opcodes[0x16] = {Operation::ASL,    Operand::DIRECT_PAGE_INDEXED_X,  OperandSize::MEMORY};
	opcodes[0x1E] = {Operation::ASL,    Operand::ABSOLUTE_INDEXED_X,     OperandSize::MEMORY};
	opcodes[0x46] = {Operation::LSR,    Operand::DIRECT_PAGE,            OperandSize::MEMORY};
	opcodes[0x4A] = {Operation::LSR,    Operand::ACCUMULATOR,            OperandSize::MEMORY};
	opcodes[0x4E] = {Operation::LSR,    Operand::ABSOLUTE,               OperandSize::MEMORY};
	opcodes[0x56] = {Operation::LSR,    Operand::DIRECT_PAGE_INDEXED_X,  OperandSize::MEMORY};
	opcodes[0x5E] = {Operation::LSR,    Operand::ABSOLUTE_INDEXED_X,     OperandSize::MEMORY};
	opcodes[0x90] = {Operation::BCC, Operand::BRANCH_8,            OperandSize::NONE};
	opcodes[0xB0] = {Operation::BCS, Operand::BRANCH_8,            OperandSize::NONE};
	opcodes[0xF0] = {Operation::BEQ, Operand::BRANCH_8,            OperandSize::NONE};
	opcodes[0x30] = {Operation::BMI, Operand::BRANCH_8,            OperandSize::NONE};
	opcodes[0xD0] = {Operation::BNE, Operand::BRANCH_8,            OperandSize::NONE};
	opcodes[0x10] = {Operation::BPL, Operand::BRANCH_8,            OperandSize::NONE};
	opcodes[0x80] = {Operation::BRA, Operand::BRANCH_8,            OperandSize::NONE};
	opcodes[0x82] = {Operation::BRL, Operand::BRANCH_16,            OperandSize::NONE};
	opcodes[0x50] = {Operation::BVC, Operand::BRANCH_8,            OperandSize::NONE};
	opcodes[0x70] = {Operation::BVS, Operand::BRANCH_8,            OperandSize::NONE};
	opcodes[0x4C] = {Operation::JMP, Operand::ABSOLUTE,                  OperandSize::NONE};
	opcodes[0x6C] = {Operation::JMP, Operand::ABSOLUTE_INDIRECT,         OperandSize::NONE};
	opcodes[0x7C] = {Operation::JMP, Operand::ABSOLUTE_INDEXED_X_INDIRECT,   OperandSize::NONE};
	opcodes[0x5C] = {Operation::JML, Operand::ABSOLUTE_LONG,             OperandSize::NONE};
	opcodes[0xDC] = {Operation::JML, Operand::ABSOLUTE_INDIRECT_LONG,    OperandSize::NONE};
	opcodes[0x20] = {Operation::JSR, Operand::ABSOLUTE,                  OperandSize::NONE};
	opcodes[0xFC] = {Operation::JSR, Operand::ABSOLUTE_INDEXED_X_INDIRECT,   OperandSize::NONE};
	opcodes[0x22] = {Operation::JSL, Operand::ABSOLUTE_LONG,             OperandSize::NONE};
	opcodes[0x86] = {Operation::STX, Operand::DIRECT_PAGE,           OperandSize::INDEX};
	opcodes[0x8E] = {Operation::STX, Operand::ABSOLUTE,              OperandSize::INDEX};
	opcodes[0x96] = {Operation::STX, Operand::DIRECT_PAGE_INDEXED_Y, OperandSize::INDEX};
	opcodes[0x84] = {Operation::STY, Operand::DIRECT_PAGE,           OperandSize::INDEX};
	opcodes[0x8C] = {Operation::STY, Operand::ABSOLUTE,              OperandSize::INDEX};
	opcodes[0x94] = {Operation::STY, Operand::DIRECT_PAGE_INDEXED_X, OperandSize::INDEX};
	opcodes[0x64] = {Operation::STZ, Operand::DIRECT_PAGE,           OperandSize::MEMORY};
	opcodes[0x74] = {Operation::STZ, Operand::DIRECT_PAGE_INDEXED_X, OperandSize::MEMORY};
	opcodes[0x9C] = {Operation::STZ, Operand::ABSOLUTE,              OperandSize::MEMORY};
	opcodes[0x9E] = {Operation::STZ, Operand::ABSOLUTE_INDEXED_X,    OperandSize::MEMORY};
	opcodes[0xA2] = {Operation::LDX, Operand::IMMEDIATE_INDEX,       OperandSize::INDEX};
	opcodes[0xA6] = {Operation::LDX, Operand::DIRECT_PAGE,           OperandSize::INDEX};
	opcodes[0xAE] = {Operation::LDX, Operand::ABSOLUTE,              OperandSize::INDEX};
	opcodes[0xB6] = {Operation::LDX, Operand::DIRECT_PAGE_INDEXED_Y, OperandSize::INDEX};
	opcodes[0xBE] = {Operation::LDX, Operand::ABSOLUTE_INDEXED_Y,    OperandSize::INDEX};
	opcodes[0xA0] = {Operation::LDY, Operand::IMMEDIATE_INDEX,       OperandSize::INDEX};
	opcodes[0xA4] = {Operation::LDY, Operand::DIRECT_PAGE,           OperandSize::INDEX};
	opcodes[0xAC] = {Operation::LDY, Operand::ABSOLUTE,              OperandSize::INDEX};
	opcodes[0xB4] = {Operation::LDY, Operand::DIRECT_PAGE_INDEXED_X, OperandSize::INDEX};
	opcodes[0xBC] = {Operation::LDY, Operand::ABSOLUTE_INDEXED_X,    OperandSize::INDEX};
	opcodes[0x04] = {Operation::TSB, Operand::DIRECT_PAGE,       OperandSize::MEMORY};
	opcodes[0x0C] = {Operation::TSB, Operand::ABSOLUTE,       OperandSize::MEMORY};
	opcodes[0x14] = {Operation::TRB, Operand::DIRECT_PAGE,       OperandSize::MEMORY};
	opcodes[0x1C] = {Operation::TRB, Operand::ABSOLUTE,       OperandSize::MEMORY};
	opcodes[0xC0] = {Operation::CPY, Operand::IMMEDIATE_INDEX,       OperandSize::INDEX};
	opcodes[0xC4] = {Operation::CPY, Operand::DIRECT_PAGE,           OperandSize::INDEX};
	opcodes[0xCC] = {Operation::CPY, Operand::ABSOLUTE,              OperandSize::INDEX};
	opcodes[0xE0] = {Operation::CPX, Operand::IMMEDIATE_INDEX,       OperandSize::INDEX};
	opcodes[0xE4] = {Operation::CPX, Operand::DIRECT_PAGE,           OperandSize::INDEX};
	opcodes[0xEC] = {Operation::CPX, Operand::ABSOLUTE,              OperandSize::INDEX};
	opcodes[0xA1] = {Operation::LDA, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,      OperandSize::MEMORY};
	opcodes[0xA3] = {Operation::LDA, Operand::STACK_RELATIVE,                      OperandSize::MEMORY};
	opcodes[0xA5] = {Operation::LDA, Operand::DIRECT_PAGE,                         OperandSize::MEMORY};
	opcodes[0xAF] = {Operation::LDA, Operand::ABSOLUTE_LONG,                       OperandSize::MEMORY};
	opcodes[0xA7] = {Operation::LDA, Operand::DIRECT_PAGE_INDIRECT_LONG,           OperandSize::MEMORY};
	opcodes[0xA9] = {Operation::LDA, Operand::IMMEDIATE_MEMORY,                    OperandSize::MEMORY};
	opcodes[0xAD] = {Operation::LDA, Operand::ABSOLUTE,                            OperandSize::MEMORY};
	opcodes[0xAF] = {Operation::LDA, Operand::ABSOLUTE_LONG,                       OperandSize::MEMORY};
	opcodes[0xB1] = {Operation::LDA, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,      OperandSize::MEMORY};
	opcodes[0xB2] = {Operation::LDA, Operand::DIRECT_PAGE_INDIRECT,                OperandSize::MEMORY};
	opcodes[0xB3] = {Operation::LDA, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,   OperandSize::MEMORY};
	opcodes[0xB5] = {Operation::LDA, Operand::DIRECT_PAGE_INDEXED_X,               OperandSize::MEMORY};
	opcodes[0xB7] = {Operation::LDA, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y, OperandSize::MEMORY};
	opcodes[0xB9] = {Operation::LDA, Operand::ABSOLUTE_INDEXED_Y,                  OperandSize::MEMORY};
	opcodes[0xBD] = {Operation::LDA, Operand::ABSOLUTE_INDEXED_X,                  OperandSize::MEMORY};
	opcodes[0xBF] = {Operation::LDA, Operand::ABSOLUTE_LONG_INDEXED_X,             OperandSize::MEMORY};
	opcodes[0x81] = {Operation::STA, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,    OperandSize::MEMORY};
	opcodes[0x83] = {Operation::STA, Operand::STACK_RELATIVE,                    OperandSize::MEMORY};
	opcodes[0x85] = {Operation::STA, Operand::DIRECT_PAGE,                       OperandSize::MEMORY};
	opcodes[0x87] = {Operation::STA, Operand::DIRECT_PAGE_INDIRECT_LONG,         OperandSize::MEMORY};
	opcodes[0x8D] = {Operation::STA, Operand::ABSOLUTE,                          OperandSize::MEMORY};
	opcodes[0x8F] = {Operation::STA, Operand::ABSOLUTE_LONG,                     OperandSize::MEMORY};
	opcodes[0x91] = {Operation::STA, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,    OperandSize::MEMORY};
	opcodes[0x92] = {Operation::STA, Operand::DIRECT_PAGE_INDIRECT,              OperandSize::MEMORY};
	opcodes[0x93] = {Operation::STA, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y, OperandSize::MEMORY};
	opcodes[0x95] = {Operation::STA, Operand::DIRECT_PAGE_INDEXED_X,             OperandSize::MEMORY};
	opcodes[0x97] = {Operation::STA, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,                  OperandSize::MEMORY};
	opcodes[0x99] = {Operation::STA, Operand::ABSOLUTE_INDEXED_Y,                OperandSize::MEMORY};
	opcodes[0x9D] = {Operation::STA, Operand::ABSOLUTE_INDEXED_X,                OperandSize::MEMORY};
	opcodes[0x9F] = {Operation::STA, Operand::ABSOLUTE_LONG_INDEXED_X,           OperandSize::MEMORY};
	opcodes[0x61] = {Operation::ADC, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,              OperandSize::MEMORY};
	opcodes[0x63] = {Operation::ADC, Operand::STACK_RELATIVE,                              OperandSize::MEMORY};
	opcodes[0x65] = {Operation::ADC, Operand::DIRECT_PAGE,                                 OperandSize::MEMORY};
	opcodes[0x67] = {Operation::ADC, Operand::DIRECT_PAGE_INDIRECT_LONG,                   OperandSize::MEMORY};
	opcodes[0x69] = {Operation::ADC, Operand::IMMEDIATE_MEMORY,                            OperandSize::MEMORY};
	opcodes[0x6D] = {Operation::ADC, Operand::ABSOLUTE,                                    OperandSize::MEMORY};
	opcodes[0x6F] = {Operation::ADC, Operand::ABSOLUTE_LONG,                               OperandSize::MEMORY};
	opcodes[0x71] = {Operation::ADC, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,              OperandSize::MEMORY};
	opcodes[0x72] = {Operation::ADC, Operand::DIRECT_PAGE_INDIRECT,                        OperandSize::MEMORY};
	opcodes[0x73] = {Operation::ADC, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,           OperandSize::MEMORY};
	opcodes[0x75] = {Operation::ADC, Operand::DIRECT_PAGE_INDEXED_X,                       OperandSize::MEMORY};
	opcodes[0x77] = {Operation::ADC, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,         OperandSize::MEMORY};
	opcodes[0x79] = {Operation::ADC, Operand::ABSOLUTE_INDEXED_Y,                          OperandSize::MEMORY};
	opcodes[0x7D] = {Operation::ADC, Operand::ABSOLUTE_INDEXED_X,                          OperandSize::MEMORY};
	opcodes[0x7F] = {Operation::ADC, Operand::ABSOLUTE_LONG_INDEXED_X,                     OperandSize::MEMORY};
	opcodes[0xE1] = {Operation::SBC, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,              OperandSize::MEMORY};
	opcodes[0xE3] = {Operation::SBC, Operand::STACK_RELATIVE,                              OperandSize::MEMORY};
	opcodes[0xE5] = {Operation::SBC, Operand::DIRECT_PAGE,                                 OperandSize::MEMORY};
	opcodes[0xE7] = {Operation::SBC, Operand::DIRECT_PAGE_INDIRECT_LONG,                   OperandSize::MEMORY};
	opcodes[0xE9] = {Operation::SBC, Operand::IMMEDIATE_MEMORY,                            OperandSize::MEMORY};
	opcodes[0xED] = {Operation::SBC, Operand::ABSOLUTE,                                    OperandSize::MEMORY};
	opcodes[0xEF] = {Operation::SBC, Operand::ABSOLUTE_LONG,                               OperandSize::MEMORY};
	opcodes[0xF1] = {Operation::SBC, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,              OperandSize::MEMORY};
	opcodes[0xF2] = {Operation::SBC, Operand::DIRECT_PAGE_INDIRECT,                        OperandSize::MEMORY};
	opcodes[0xF3] = {Operation::SBC, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,           OperandSize::MEMORY};
	opcodes[0xF5] = {Operation::SBC, Operand::DIRECT_PAGE_INDEXED_X,                       OperandSize::MEMORY};
	opcodes[0xF7] = {Operation::SBC, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,         OperandSize::MEMORY};
	opcodes[0xF9] = {Operation::SBC, Operand::ABSOLUTE_INDEXED_Y,                          OperandSize::MEMORY};
	opcodes[0xFD] = {Operation::SBC, Operand::ABSOLUTE_INDEXED_X,                          OperandSize::MEMORY};
	opcodes[0xFF] = {Operation::SBC, Operand::ABSOLUTE_LONG_INDEXED_X,                     OperandSize::MEMORY};
	opcodes[0x21] = {Operation::AND, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,              OperandSize::MEMORY};
	opcodes[0x23] = {Operation::AND, Operand::STACK_RELATIVE,                              OperandSize::MEMORY};
	opcodes[0x25] = {Operation::AND, Operand::DIRECT_PAGE,                                 OperandSize::MEMORY};
	opcodes[0x27] = {Operation::AND, Operand::DIRECT_PAGE_INDIRECT_LONG,                   OperandSize::MEMORY};
	opcodes[0x29] = {Operation::AND, Operand::IMMEDIATE_MEMORY,                            OperandSize::MEMORY};
	opcodes[0x2D] = {Operation::AND, Operand::ABSOLUTE,                                    OperandSize::MEMORY};
	opcodes[0x2F] = {Operation::AND, Operand::ABSOLUTE_LONG,                               OperandSize::MEMORY};
	opcodes[0x31] = {Operation::AND, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,              OperandSize::MEMORY};
	opcodes[0x32] = {Operation::AND, Operand::DIRECT_PAGE_INDIRECT,                        OperandSize::MEMORY};
	opcodes[0x33] = {Operation::AND, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,           OperandSize::MEMORY};
	opcodes[0x35] = {Operation::AND, Operand::DIRECT_PAGE_INDEXED_X,                       OperandSize::MEMORY};
	opcodes[0x37] = {Operation::AND, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,         OperandSize::MEMORY};
	opcodes[0x39] = {Operation::AND, Operand::ABSOLUTE_INDEXED_Y,                          OperandSize::MEMORY};
	opcodes[0x3D] = {Operation::AND, Operand::ABSOLUTE_INDEXED_X,                          OperandSize::MEMORY};
	opcodes[0x3F] = {Operation::AND, Operand::ABSOLUTE_LONG_INDEXED_X,                     OperandSize::MEMORY};
	opcodes[0xC1] = {Operation::CMP, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,              OperandSize::MEMORY};
	opcodes[0xC3] = {Operation::CMP, Operand::STACK_RELATIVE,                              OperandSize::MEMORY};
	opcodes[0xC5] = {Operation::CMP, Operand::DIRECT_PAGE,                                 OperandSize::MEMORY};
	opcodes[0xC7] = {Operation::CMP, Operand::DIRECT_PAGE_INDIRECT_LONG,                   OperandSize::MEMORY};
	opcodes[0xC9] = {Operation::CMP, Operand::IMMEDIATE_MEMORY,                            OperandSize::MEMORY};
	opcodes[0xCD] = {Operation::CMP, Operand::ABSOLUTE,                                    OperandSize::MEMORY};
	opcodes[0xCF] = {Operation::CMP, Operand::ABSOLUTE_LONG,                               OperandSize::MEMORY};
	opcodes[0xD1] = {Operation::CMP, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,              OperandSize::MEMORY};
	opcodes[0xD2] = {Operation::CMP, Operand::DIRECT_PAGE_INDIRECT,                        OperandSize::MEMORY};
	opcodes[0xD3] = {Operation::CMP, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,           OperandSize::MEMORY};
	opcodes[0xD5] = {Operation::CMP, Operand::DIRECT_PAGE_INDEXED_X,                       OperandSize::MEMORY};
	opcodes[0xD7] = {Operation::CMP, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,         OperandSize::MEMORY};
	opcodes[0xD9] = {Operation::CMP, Operand::ABSOLUTE_INDEXED_Y,                          OperandSize::MEMORY};
	opcodes[0xDD] = {Operation::CMP, Operand::ABSOLUTE_INDEXED_X,                          OperandSize::MEMORY};
	opcodes[0xDF] = {Operation::CMP, Operand::ABSOLUTE_LONG_INDEXED_X,                     OperandSize::MEMORY};
	opcodes[0x01] = {Operation::ORA, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,              OperandSize::MEMORY};
	opcodes[0x03] = {Operation::ORA, Operand::STACK_RELATIVE,                              OperandSize::MEMORY};
	opcodes[0x05] = {Operation::ORA, Operand::DIRECT_PAGE,                                 OperandSize::MEMORY};
	opcodes[0x07] = {Operation::ORA, Operand::DIRECT_PAGE_INDIRECT_LONG,                   OperandSize::MEMORY};
	opcodes[0x09] = {Operation::ORA, Operand::IMMEDIATE_MEMORY,                            OperandSize::MEMORY};
	opcodes[0x0D] = {Operation::ORA, Operand::ABSOLUTE,                                    OperandSize::MEMORY};
	opcodes[0x0F] = {Operation::ORA, Operand::ABSOLUTE_LONG,                               OperandSize::MEMORY};
	opcodes[0x11] = {Operation::ORA, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,              OperandSize::MEMORY};
	opcodes[0x12] = {Operation::ORA, Operand::DIRECT_PAGE_INDIRECT,                        OperandSize::MEMORY};
	opcodes[0x13] = {Operation::ORA, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,           OperandSize::MEMORY};
	opcodes[0x15] = {Operation::ORA, Operand::DIRECT_PAGE_INDEXED_X,                       OperandSize::MEMORY};
	opcodes[0x17] = {Operation::ORA, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,         OperandSize::MEMORY};
	opcodes[0x19] = {Operation::ORA, Operand::ABSOLUTE_INDEXED_Y,                          OperandSize::MEMORY};
	opcodes[0x1D] = {Operation::ORA, Operand::ABSOLUTE_INDEXED_X,                          OperandSize::MEMORY};
	opcodes[0x1F] = {Operation::ORA, Operand::ABSOLUTE_LONG_INDEXED_X,                     OperandSize::MEMORY};
	opcodes[0x41] = {Operation::EOR, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,              OperandSize::MEMORY};
	opcodes[0x43] = {Operation::EOR, Operand::STACK_RELATIVE,                              OperandSize::MEMORY};
	opcodes[0x45] = {Operation::EOR, Operand::DIRECT_PAGE,                                 OperandSize::MEMORY};
	opcodes[0x47] = {Operation::EOR, Operand::DIRECT_PAGE_INDIRECT_LONG,                   OperandSize::MEMORY};
	opcodes[0x49] = {Operation::EOR, Operand::IMMEDIATE_MEMORY,                            OperandSize::MEMORY};
	opcodes[0x4D] = {Operation::EOR, Operand::ABSOLUTE,                                    OperandSize::MEMORY};
	opcodes[0x4F] = {Operation::EOR, Operand::ABSOLUTE_LONG,                               OperandSize::MEMORY};
	opcodes[0x51] = {Operation::EOR, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,              OperandSize::MEMORY};
	opcodes[0x52] = {Operation::EOR, Operand::DIRECT_PAGE_INDIRECT,                        OperandSize::MEMORY};
	opcodes[0x53] = {Operation::EOR, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,           OperandSize::MEMORY};
	opcodes[0x55] = {Operation::EOR, Operand::DIRECT_PAGE_INDEXED_X,                       OperandSize::MEMORY};
	opcodes[0x57] = {Operation::EOR, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,         OperandSize::MEMORY};
	opcodes[0x59] = {Operation::EOR, Operand::ABSOLUTE_INDEXED_Y,                          OperandSize::MEMORY};
	opcodes[0x5D] = {Operation::EOR, Operand::ABSOLUTE_INDEXED_X,                          OperandSize::MEMORY};
	opcodes[0x5F] = {Operation::EOR, Operand::ABSOLUTE_LONG_INDEXED_X,                     OperandSize::MEMORY};
	"""

adressing_modes=set([])
ops={}

lines = manual_table.split("\n")
for l in lines:
	k = l.split(",")
	if len(k)<3: continue
	opcode = int(k[0][11:13], 16)

	k[0]=k[0][29:]
	k[1] = k[1].strip()
	k[1]=k[1][9:]
	k[2]=k[2][:-2]
	k[2]=k[2].strip()
	k[2]=k[2][13:]

	adressing_modes.add(k[1])
	ops[opcode]=(k[0], k[1], k[2])

ops_array=[]

remapping=[-1]*256

def to_hex(i):
	a = hex(i)[2:]
	if len(a)==1: a = "0" + a
	return "0x"+a.upper()

for i in range(256):
	h = operands.opcodes[i][0][2:]
	n = int(h, 16)
	remapping[n]=i

for i in range(256):
	opcode = to_hex(i)

	full = operands.opcodes[remapping[i]]
	menmon = full[1]
	if i in ops:
		op = ops[i]
		operand_mode = op[1]
		size = op[2]
		if size == "NONE": size = "SMALL"
		ops_array.append((opcode, menmon, op[0], operand_mode, size, ""))
	else:
		am = operands.addressing_modes[full[2]][0]
		alias = full[3]

		ops_array.append((opcode, menmon, menmon, "MANUAL", "SMALL", alias))


with open('optab.json', 'w') as outfile:
    json.dump(ops_array, outfile, indent = 4)
#print(op)