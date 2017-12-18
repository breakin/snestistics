#include "instruction_tables.h"

namespace snestistics {

const char* const mnemonic_names[256] = {
	"BRK",	"ORA",	"COP",	"ORA",	"TSB",	"ORA",	"ASL",	"ORA",	"PHP",	"ORA",	"ASL",	"PHD",	"TSB",	"ORA",	"ASL",	"ORA",
	"BPL",	"ORA",	"ORA",	"ORA",	"TRB",	"ORA",	"ASL",	"ORA",	"CLC",	"ORA",	"INC",	"TCS",	"TRB",	"ORA",	"ASL",	"ORA",
	"JSR",	"AND",	"JSR",	"AND",	"BIT",	"AND",	"ROL",	"AND",	"PLP",	"AND",	"ROL",	"PLD",	"BIT",	"AND",	"ROL",	"AND",
	"BMI",	"AND",	"AND",	"AND",	"BIT",	"AND",	"ROL",	"AND",	"SEC",	"AND",	"DEC",	"TSC",	"BIT",	"AND",	"ROL",	"AND",
	"RTI",	"EOR",	"WDM",	"EOR",	"MVP",	"EOR",	"LSR",	"EOR",	"PHA",	"EOR",	"LSR",	"PHK",	"JMP",	"EOR",	"LSR",	"EOR",
	"BVC",	"EOR",	"EOR",	"EOR",	"MVN",	"EOR",	"LSR",	"EOR",	"CLI",	"EOR",	"PHY",	"TCD",	"JMP",	"EOR",	"LSR",	"EOR",
	"RTS",	"ADC",	"PER",	"ADC",	"STZ",	"ADC",	"ROR",	"ADC",	"PLA",	"ADC",	"ROR",	"RTL",	"JMP",	"ADC",	"ROR",	"ADC",
	"BVS",	"ADC",	"ADC",	"ADC",	"STZ",	"ADC",	"ROR",	"ADC",	"SEI",	"ADC",	"PLY",	"TDC",	"JMP",	"ADC",	"ROR",	"ADC",
	"BRA",	"STA",	"BRL",	"STA",	"STY",	"STA",	"STX",	"STA",	"DEY",	"BIT",	"TXA",	"PHB",	"STY",	"STA",	"STX",	"STA",
	"BCC",	"STA",	"STA",	"STA",	"STY",	"STA",	"STX",	"STA",	"TYA",	"STA",	"TXS",	"TXY",	"STZ",	"STA",	"STZ",	"STA",
	"LDY",	"LDA",	"LDX",	"LDA",	"LDY",	"LDA",	"LDX",	"LDA",	"TAY",	"LDA",	"TAX",	"PLB",	"LDY",	"LDA",	"LDX",	"LDA",
	"BCS",	"LDA",	"LDA",	"LDA",	"LDY",	"LDA",	"LDX",	"LDA",	"CLV",	"LDA",	"TSX",	"TYX",	"LDY",	"LDA",	"LDX",	"LDA",
	"CPY",	"CMP",	"REP",	"CMP",	"CPY",	"CMP",	"DEC",	"CMP",	"INY",	"CMP",	"DEX",	"WAI",	"CPY",	"CMP",	"DEC",	"CMP",
	"BNE",	"CMP",	"CMP",	"CMP",	"PEI",	"CMP",	"DEC",	"CMP",	"CLD",	"CMP",	"PHX",	"STP",	"JMP",	"CMP",	"DEC",	"CMP",
	"CPX",	"SBC",	"SEP",	"SBC",	"CPX",	"SBC",	"INC",	"SBC",	"INX",	"SBC",	"NOP",	"XBA",	"CPX",	"SBC",	"INC",	"SBC",
	"BEQ",	"SBC",	"SBC",	"SBC",	"PEA",	"SBC",	"INC",	"SBC",	"SED",	"SBC",	"PLX",	"XCE",	"JSR",	"SBC",	"INC",	"SBC",
};

const OpCode op_codes[256] = {
	{Operation::BRK, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x00 BRK
	{Operation::ORA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,        true }, // 0x01 ORA
	{Operation::COP, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x02 COP
	{Operation::ORA, InstructionSize::MEMORY, Operand::STACK_RELATIVE,                        true }, // 0x03 ORA
	{Operation::TSB, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0x04 TSB
	{Operation::ORA, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0x05 ORA
	{Operation::ASL, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0x06 ASL
	{Operation::ORA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG,             true }, // 0x07 ORA
	{Operation::PHP, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x08 PHP
	{Operation::ORA, InstructionSize::MEMORY, Operand::IMMEDIATE_MEMORY,                      false}, // 0x09 ORA
	{Operation::ASL, InstructionSize::MEMORY, Operand::ACCUMULATOR,                           false}, // 0x0A ASL
	{Operation::PHD, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x0B PHD
	{Operation::TSB, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0x0C TSB
	{Operation::ORA, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0x0D ORA
	{Operation::ASL, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0x0E ASL
	{Operation::ORA, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG,                         true }, // 0x0F ORA
	{Operation::BPL, InstructionSize::SMALL,  Operand::BRANCH_8,                              false}, // 0x10 BPL
	{Operation::ORA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,        true }, // 0x11 ORA
	{Operation::ORA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT,                  true }, // 0x12 ORA
	{Operation::ORA, InstructionSize::MEMORY, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,     true }, // 0x13 ORA
	{Operation::TRB, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0x14 TRB
	{Operation::ORA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0x15 ORA
	{Operation::ASL, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0x16 ASL
	{Operation::ORA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,   true }, // 0x17 ORA
	{Operation::CLC, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x18 CLC
	{Operation::ORA, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_Y,                    true }, // 0x19 ORA
	{Operation::INC, InstructionSize::MEMORY, Operand::ACCUMULATOR,                           false}, // 0x1A INC
	{Operation::TCS, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x1B TCS
	{Operation::TRB, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0x1C TRB
	{Operation::ORA, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0x1D ORA
	{Operation::ASL, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0x1E ASL
	{Operation::ORA, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG_INDEXED_X,               true }, // 0x1F ORA
	{Operation::JSR, InstructionSize::SMALL,  Operand::ABSOLUTE,                              false}, // 0x20 JSR
	{Operation::AND, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,        true }, // 0x21 AND
	{Operation::JSL, InstructionSize::SMALL,  Operand::ABSOLUTE_LONG,                         false}, // 0x22 JSR
	{Operation::AND, InstructionSize::MEMORY, Operand::STACK_RELATIVE,                        true }, // 0x23 AND
	{Operation::BIT, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0x24 BIT
	{Operation::AND, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0x25 AND
	{Operation::ROL, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0x26 ROL
	{Operation::AND, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG,             true }, // 0x27 AND
	{Operation::PLP, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x28 PLP
	{Operation::AND, InstructionSize::MEMORY, Operand::IMMEDIATE_MEMORY,                      false}, // 0x29 AND
	{Operation::ROL, InstructionSize::MEMORY, Operand::ACCUMULATOR,                           false}, // 0x2A ROL
	{Operation::PLD, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x2B PLD
	{Operation::BIT, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0x2C BIT
	{Operation::AND, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0x2D AND
	{Operation::ROL, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0x2E ROL
	{Operation::AND, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG,                         true }, // 0x2F AND
	{Operation::BMI, InstructionSize::SMALL,  Operand::BRANCH_8,                              false}, // 0x30 BMI
	{Operation::AND, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,        true }, // 0x31 AND
	{Operation::AND, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT,                  true }, // 0x32 AND
	{Operation::AND, InstructionSize::MEMORY, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,     true }, // 0x33 AND
	{Operation::BIT, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0x34 BIT
	{Operation::AND, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0x35 AND
	{Operation::ROL, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0x36 ROL
	{Operation::AND, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,   true }, // 0x37 AND
	{Operation::SEC, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x38 SEC
	{Operation::AND, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_Y,                    true }, // 0x39 AND
	{Operation::DEC, InstructionSize::MEMORY, Operand::ACCUMULATOR,                           false}, // 0x3A DEC
	{Operation::TSC, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x3B TSC
	{Operation::BIT, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0x3C BIT
	{Operation::AND, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0x3D AND
	{Operation::ROL, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0x3E ROL
	{Operation::AND, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG_INDEXED_X,               true }, // 0x3F AND
	{Operation::RTI, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x40 RTI
	{Operation::EOR, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,        true }, // 0x41 EOR
	{Operation::WDM, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x42 WDM
	{Operation::EOR, InstructionSize::MEMORY, Operand::STACK_RELATIVE,                        true }, // 0x43 EOR
	{Operation::MVP, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x44 MVP
	{Operation::EOR, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0x45 EOR
	{Operation::LSR, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0x46 LSR
	{Operation::EOR, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG,             true }, // 0x47 EOR
	{Operation::PHA, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x48 PHA
	{Operation::EOR, InstructionSize::MEMORY, Operand::IMMEDIATE_MEMORY,                      false}, // 0x49 EOR
	{Operation::LSR, InstructionSize::MEMORY, Operand::ACCUMULATOR,                           false}, // 0x4A LSR
	{Operation::PHK, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x4B PHK
	{Operation::JMP, InstructionSize::SMALL,  Operand::ABSOLUTE,                              false}, // 0x4C JMP
	{Operation::EOR, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0x4D EOR
	{Operation::LSR, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0x4E LSR
	{Operation::EOR, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG,                         true }, // 0x4F EOR
	{Operation::BVC, InstructionSize::SMALL,  Operand::BRANCH_8,                              false}, // 0x50 BVC
	{Operation::EOR, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,        true }, // 0x51 EOR
	{Operation::EOR, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT,                  true }, // 0x52 EOR
	{Operation::EOR, InstructionSize::MEMORY, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,     true }, // 0x53 EOR
	{Operation::MVN, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x54 MVN
	{Operation::EOR, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0x55 EOR
	{Operation::LSR, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0x56 LSR
	{Operation::EOR, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,   true }, // 0x57 EOR
	{Operation::CLI, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x58 CLI
	{Operation::EOR, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_Y,                    true }, // 0x59 EOR
	{Operation::PHY, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x5A PHY
	{Operation::TCD, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x5B TCD
	{Operation::JML, InstructionSize::SMALL,  Operand::ABSOLUTE_LONG,                         false}, // 0x5C JMP
	{Operation::EOR, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0x5D EOR
	{Operation::LSR, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0x5E LSR
	{Operation::EOR, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG_INDEXED_X,               true }, // 0x5F EOR
	{Operation::RTS, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x60 RTS
	{Operation::ADC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,        true }, // 0x61 ADC
	{Operation::PER, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x62 PER
	{Operation::ADC, InstructionSize::MEMORY, Operand::STACK_RELATIVE,                        true }, // 0x63 ADC
	{Operation::STZ, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           false}, // 0x64 STZ
	{Operation::ADC, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0x65 ADC
	{Operation::ROR, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0x66 ROR
	{Operation::ADC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG,             true }, // 0x67 ADC
	{Operation::PLA, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x68 PLA
	{Operation::ADC, InstructionSize::MEMORY, Operand::IMMEDIATE_MEMORY,                      false}, // 0x69 ADC
	{Operation::ROR, InstructionSize::MEMORY, Operand::ACCUMULATOR,                           false}, // 0x6A ROR
	{Operation::RTL, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x6B RTL
	{Operation::JMP, InstructionSize::SMALL,  Operand::ABSOLUTE_INDIRECT,                     false}, // 0x6C JMP
	{Operation::ADC, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0x6D ADC
	{Operation::ROR, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0x6E ROR
	{Operation::ADC, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG,                         true }, // 0x6F ADC
	{Operation::BVS, InstructionSize::SMALL,  Operand::BRANCH_8,                              false}, // 0x70 BVS
	{Operation::ADC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,        true }, // 0x71 ADC
	{Operation::ADC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT,                  true }, // 0x72 ADC
	{Operation::ADC, InstructionSize::MEMORY, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,     true }, // 0x73 ADC
	{Operation::STZ, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 false}, // 0x74 STZ
	{Operation::ADC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0x75 ADC
	{Operation::ROR, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0x76 ROR
	{Operation::ADC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,   true }, // 0x77 ADC
	{Operation::SEI, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x78 SEI
	{Operation::ADC, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_Y,                    true }, // 0x79 ADC
	{Operation::PLY, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x7A PLY
	{Operation::TDC, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x7B TDC
	{Operation::JMP, InstructionSize::SMALL,  Operand::ABSOLUTE_INDEXED_X_INDIRECT,           false}, // 0x7C JMP
	{Operation::ADC, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0x7D ADC
	{Operation::ROR, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0x7E ROR
	{Operation::ADC, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG_INDEXED_X,               true }, // 0x7F ADC
	{Operation::BRA, InstructionSize::SMALL,  Operand::BRANCH_8,                              false}, // 0x80 BRA
	{Operation::STA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,        false}, // 0x81 STA
	{Operation::BRL, InstructionSize::SMALL,  Operand::BRANCH_16,                             false}, // 0x82 BRL
	{Operation::STA, InstructionSize::MEMORY, Operand::STACK_RELATIVE,                        false}, // 0x83 STA
	{Operation::STY, InstructionSize::INDEX,  Operand::DIRECT_PAGE,                           false}, // 0x84 STY
	{Operation::STA, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           false}, // 0x85 STA
	{Operation::STX, InstructionSize::INDEX,  Operand::DIRECT_PAGE,                           false}, // 0x86 STX
	{Operation::STA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG,             false}, // 0x87 STA
	{Operation::DEY, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x88 DEY
	{Operation::BIT, InstructionSize::MEMORY, Operand::IMMEDIATE_MEMORY,                      false}, // 0x89 BIT
	{Operation::TXA, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x8A TXA
	{Operation::PHB, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x8B PHB
	{Operation::STY, InstructionSize::INDEX,  Operand::ABSOLUTE,                              false}, // 0x8C STY
	{Operation::STA, InstructionSize::MEMORY, Operand::ABSOLUTE,                              false}, // 0x8D STA
	{Operation::STX, InstructionSize::INDEX,  Operand::ABSOLUTE,                              false}, // 0x8E STX
	{Operation::STA, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG,                         false}, // 0x8F STA
	{Operation::BCC, InstructionSize::SMALL,  Operand::BRANCH_8,                              false}, // 0x90 BCC
	{Operation::STA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,        false}, // 0x91 STA
	{Operation::STA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT,                  false}, // 0x92 STA
	{Operation::STA, InstructionSize::MEMORY, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,     false}, // 0x93 STA
	{Operation::STY, InstructionSize::INDEX,  Operand::DIRECT_PAGE_INDEXED_X,                 false}, // 0x94 STY
	{Operation::STA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 false}, // 0x95 STA
	{Operation::STX, InstructionSize::INDEX,  Operand::DIRECT_PAGE_INDEXED_Y,                 false}, // 0x96 STX
	{Operation::STA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,   false}, // 0x97 STA
	{Operation::TYA, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x98 TYA
	{Operation::STA, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_Y,                    false}, // 0x99 STA
	{Operation::TXS, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x9A TXS
	{Operation::TXY, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0x9B TXY
	{Operation::STZ, InstructionSize::MEMORY, Operand::ABSOLUTE,                              false}, // 0x9C STZ
	{Operation::STA, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    false}, // 0x9D STA
	{Operation::STZ, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    false}, // 0x9E STZ
	{Operation::STA, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG_INDEXED_X,               false}, // 0x9F STA
	{Operation::LDY, InstructionSize::INDEX,  Operand::IMMEDIATE_INDEX,                       false}, // 0xA0 LDY
	{Operation::LDA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,        true }, // 0xA1 LDA
	{Operation::LDX, InstructionSize::INDEX,  Operand::IMMEDIATE_INDEX,                       false}, // 0xA2 LDX
	{Operation::LDA, InstructionSize::MEMORY, Operand::STACK_RELATIVE,                        true }, // 0xA3 LDA
	{Operation::LDY, InstructionSize::INDEX,  Operand::DIRECT_PAGE,                           true }, // 0xA4 LDY
	{Operation::LDA, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0xA5 LDA
	{Operation::LDX, InstructionSize::INDEX,  Operand::DIRECT_PAGE,                           true }, // 0xA6 LDX
	{Operation::LDA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG,             true }, // 0xA7 LDA
	{Operation::TAY, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xA8 TAY
	{Operation::LDA, InstructionSize::MEMORY, Operand::IMMEDIATE_MEMORY,                      false}, // 0xA9 LDA
	{Operation::TAX, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xAA TAX
	{Operation::PLB, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xAB PLB
	{Operation::LDY, InstructionSize::INDEX,  Operand::ABSOLUTE,                              true }, // 0xAC LDY
	{Operation::LDA, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0xAD LDA
	{Operation::LDX, InstructionSize::INDEX,  Operand::ABSOLUTE,                              true }, // 0xAE LDX
	{Operation::LDA, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG,                         true }, // 0xAF LDA
	{Operation::BCS, InstructionSize::SMALL,  Operand::BRANCH_8,                              false}, // 0xB0 BCS
	{Operation::LDA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,        true }, // 0xB1 LDA
	{Operation::LDA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT,                  true }, // 0xB2 LDA
	{Operation::LDA, InstructionSize::MEMORY, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,     true }, // 0xB3 LDA
	{Operation::LDY, InstructionSize::INDEX,  Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0xB4 LDY
	{Operation::LDA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0xB5 LDA
	{Operation::LDX, InstructionSize::INDEX,  Operand::DIRECT_PAGE_INDEXED_Y,                 true }, // 0xB6 LDX
	{Operation::LDA, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,   true }, // 0xB7 LDA
	{Operation::CLV, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xB8 CLV
	{Operation::LDA, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_Y,                    true }, // 0xB9 LDA
	{Operation::TSX, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xBA TSX
	{Operation::TYX, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xBB TYX
	{Operation::LDY, InstructionSize::INDEX,  Operand::ABSOLUTE_INDEXED_X,                    true }, // 0xBC LDY
	{Operation::LDA, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0xBD LDA
	{Operation::LDX, InstructionSize::INDEX,  Operand::ABSOLUTE_INDEXED_Y,                    true }, // 0xBE LDX
	{Operation::LDA, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG_INDEXED_X,               true }, // 0xBF LDA
	{Operation::CPY, InstructionSize::INDEX,  Operand::IMMEDIATE_INDEX,                       false}, // 0xC0 CPY
	{Operation::CMP, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,        true }, // 0xC1 CMP
	{Operation::REP, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xC2 REP
	{Operation::CMP, InstructionSize::MEMORY, Operand::STACK_RELATIVE,                        true }, // 0xC3 CMP
	{Operation::CPY, InstructionSize::INDEX,  Operand::DIRECT_PAGE,                           true }, // 0xC4 CPY
	{Operation::CMP, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0xC5 CMP
	{Operation::DEC, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0xC6 DEC
	{Operation::CMP, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG,             true }, // 0xC7 CMP
	{Operation::INY, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xC8 INY
	{Operation::CMP, InstructionSize::MEMORY, Operand::IMMEDIATE_MEMORY,                      false}, // 0xC9 CMP
	{Operation::DEX, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xCA DEX
	{Operation::WAI, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xCB WAI
	{Operation::CPY, InstructionSize::INDEX,  Operand::ABSOLUTE,                              true }, // 0xCC CPY
	{Operation::CMP, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0xCD CMP
	{Operation::DEC, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0xCE DEC
	{Operation::CMP, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG,                         true }, // 0xCF CMP
	{Operation::BNE, InstructionSize::SMALL,  Operand::BRANCH_8,                              false}, // 0xD0 BNE
	{Operation::CMP, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,        true }, // 0xD1 CMP
	{Operation::CMP, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT,                  true }, // 0xD2 CMP
	{Operation::CMP, InstructionSize::MEMORY, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,     true }, // 0xD3 CMP
	{Operation::PEI, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xD4 PEI
	{Operation::CMP, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0xD5 CMP
	{Operation::DEC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0xD6 DEC
	{Operation::CMP, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,   true }, // 0xD7 CMP
	{Operation::CLD, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xD8 CLD
	{Operation::CMP, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_Y,                    true }, // 0xD9 CMP
	{Operation::PHX, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xDA PHX
	{Operation::STP, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xDB STP
	{Operation::JML, InstructionSize::SMALL,  Operand::ABSOLUTE_INDIRECT_LONG,                false}, // 0xDC JMP
	{Operation::CMP, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0xDD CMP
	{Operation::DEC, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0xDE DEC
	{Operation::CMP, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG_INDEXED_X,               true }, // 0xDF CMP
	{Operation::CPX, InstructionSize::INDEX,  Operand::IMMEDIATE_INDEX,                       false}, // 0xE0 CPX
	{Operation::SBC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X_INDIRECT,        true }, // 0xE1 SBC
	{Operation::SEP, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xE2 SEP
	{Operation::SBC, InstructionSize::MEMORY, Operand::STACK_RELATIVE,                        true }, // 0xE3 SBC
	{Operation::CPX, InstructionSize::INDEX,  Operand::DIRECT_PAGE,                           true }, // 0xE4 CPX
	{Operation::SBC, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0xE5 SBC
	{Operation::INC, InstructionSize::MEMORY, Operand::DIRECT_PAGE,                           true }, // 0xE6 INC
	{Operation::SBC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG,             true }, // 0xE7 SBC
	{Operation::INX, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xE8 INX
	{Operation::SBC, InstructionSize::MEMORY, Operand::IMMEDIATE_MEMORY,                      false}, // 0xE9 SBC
	{Operation::NOP, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xEA NOP
	{Operation::XBA, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xEB XBA
	{Operation::CPX, InstructionSize::INDEX,  Operand::ABSOLUTE,                              true }, // 0xEC CPX
	{Operation::SBC, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0xED SBC
	{Operation::INC, InstructionSize::MEMORY, Operand::ABSOLUTE,                              true }, // 0xEE INC
	{Operation::SBC, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG,                         true }, // 0xEF SBC
	{Operation::BEQ, InstructionSize::SMALL,  Operand::BRANCH_8,                              false}, // 0xF0 BEQ
	{Operation::SBC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y,        true }, // 0xF1 SBC
	{Operation::SBC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT,                  true }, // 0xF2 SBC
	{Operation::SBC, InstructionSize::MEMORY, Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y,     true }, // 0xF3 SBC
	{Operation::PEA, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xF4 PEA
	{Operation::SBC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0xF5 SBC
	{Operation::INC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDEXED_X,                 true }, // 0xF6 INC
	{Operation::SBC, InstructionSize::MEMORY, Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y,   true }, // 0xF7 SBC
	{Operation::SED, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xF8 SED
	{Operation::SBC, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_Y,                    true }, // 0xF9 SBC
	{Operation::PLX, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xFA PLX
	{Operation::XCE, InstructionSize::SMALL,  Operand::MANUAL,                                false}, // 0xFB XCE
	{Operation::JSR, InstructionSize::SMALL,  Operand::ABSOLUTE_INDEXED_X_INDIRECT,           false}, // 0xFC JSR
	{Operation::SBC, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0xFD SBC
	{Operation::INC, InstructionSize::MEMORY, Operand::ABSOLUTE_INDEXED_X,                    true }, // 0xFE INC
	{Operation::SBC, InstructionSize::MEMORY, Operand::ABSOLUTE_LONG_INDEXED_X,               true }, // 0xFF SBC
};

}
