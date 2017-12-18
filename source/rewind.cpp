#include "rewind.h"
#include "emulate.h"
#include "replay.h"
#include "instruction_tables.h"
#include "options.h"
#include <map>
#include <algorithm>

using namespace snestistics;

// NOTE: Instructions like BRK and NMI (not an instruction but still) both write (stack) and reads (IRQ vector).. Does not work well with current method since we have data_pointer
//       Works out since we ignore IRQ_VECTOR and NMI_VECTOR-reads though..

// TODO: NMI is not an op but it should get an opcount.. it does modify stack so should treat it like any other op

namespace thing {
	using namespace snestistics;
	using namespace tracking;
	static const uint32_t NONE=0;
	static const uint32_t A  = 1<<((uint32_t)Value::Type::A);
	static const uint32_t AL = 1<<((uint32_t)Value::Type::AL);
	static const uint32_t AH = 1<<((uint32_t)Value::Type::AH);
	static const uint32_t X  = 1<<((uint32_t)Value::Type::X);
	static const uint32_t XL = 1<<((uint32_t)Value::Type::XL);
	static const uint32_t XH = 1<<((uint32_t)Value::Type::XH);
	static const uint32_t Y  = 1<<((uint32_t)Value::Type::Y);
	static const uint32_t YL = 1<<((uint32_t)Value::Type::YL);
	static const uint32_t YH = 1<<((uint32_t)Value::Type::YH);
	static const uint32_t FLAG_CARRY=  1<<((uint32_t)Value::Type::FLAG_CARRY);
	static const uint32_t FLAG_EMULATION=1<<((uint32_t)Value::Type::FLAG_EMULATION);
	static const uint32_t FLAG_OVERFLOW=1<<((uint32_t)Value::Type::FLAG_OVERFLOW);
	static const uint32_t FLAG_NEGATIVE=1<<((uint32_t)Value::Type::FLAG_NEGATIVE);
	static const uint32_t FLAG_ZERO=1<<((uint32_t)Value::Type::FLAG_ZERO);
	static const uint32_t S=1<<((uint32_t)Value::Type::S);
	static const uint32_t DB=1<<((uint32_t)Value::Type::DB);
	static const uint32_t PB=1<<((uint32_t)Value::Type::PB);
	static const uint32_t MEM=1<<((uint32_t)Value::Type::MEM);
	static const uint32_t DP=1<<((uint32_t)Value::Type::DP);
	static const uint32_t NZ = FLAG_ZERO|FLAG_NEGATIVE;
	static const uint32_t NZC = FLAG_ZERO|FLAG_NEGATIVE|FLAG_CARRY;
	static const uint32_t FLAGS_LOWER = NZC|FLAG_OVERFLOW|FLAG_NEGATIVE;

	static const uint32_t Y_SOME = YL|YH|Y;
	static const uint32_t X_SOME = XL|XH|X;
	static const uint32_t A_SOME = AL|AH|A;
};

const char *thing_flag_to_str(const uint32_t flag) {
	if(flag== 0) return "A";
	if(flag== 1) return "AL";
	if(flag== 2) return "AH";
	if(flag== 3) return "X";
	if(flag== 4) return "XL";
	if(flag== 5) return "XH";
	if(flag== 6) return "Y";
	if(flag== 7) return "YL";
	if(flag== 8) return "YH";
	if(flag== 9) return "CARRY";
	if(flag==10) return "EMU";
	if(flag==11) return "OVERFLOW";
	if(flag==12) return "NEG";
	if(flag==13) return "ZERO";
	if(flag==14) return "S";
	if(flag==15) return "DB";
	if(flag==16) return "PB";
	if(flag==17) return "MEM";
	if(flag==18) return "DP";
	return "oops";
}

int print_thing(uint32_t thing) {
	int d=0;
	if (thing==0) {
		d+=printf("NONE");
	}
	
	bool first = true;
	for (int k=0; k<=18; k++) {
		if ((thing & (1<<k))!=0) {
			if (!first) { d+=printf("|"); }
			first=false;
			d+=printf("%s", thing_flag_to_str(k));
		}
	}
	return d;
}
int fprint_thing(FILE *f, uint32_t thing) {
	int d=0;
	if (thing==0) {
		d+=fprintf(f, "NONE");
	}
	
	bool first = true;
	for (int k=0; k<=18; k++) {
		if ((thing & (1<<k))!=0) {
			if (!first) { d+=fprintf(f, "|"); }
			first=false;
			d+=fprintf(f, "%s", thing_flag_to_str(k));
		}
	}
	return d;
}

void fprint_thing_hej(FILE *f, uint32_t thing) {
	int d=fprint_thing(f, thing);
	while (d<20) {
		fprintf(f, " ");
		d++;
	}
}

struct OpEffect {
	uint32_t source, dest, side_source, side_dest;
	enum class Size { SMALL, WIDE, LONG, INDEX, MEMORY, NONE, AUTOASSIGN} size;
	bool mixing_low_high = false;

	int actual_size(const bool memory_flag, const bool index_flag) const {
		uint32_t opsize = 0;
		if (size == OpEffect::Size::SMALL) opsize = 1;
		if (size == OpEffect::Size::LONG) opsize = 3;
		if (size == OpEffect::Size::WIDE) opsize = 2;
		if (size == OpEffect::Size::INDEX) opsize =  index_flag ? 1 : 2;
		if (size == OpEffect::Size::MEMORY) opsize =  memory_flag ? 1 : 2;
		return opsize;
	}
};

// NOTE: If something is stack-realtive we will not track the S-register.
//       It will fall out anyway from whomever pushed to that memory location

// ROL and ROR depends on carry
static OpEffect op_effects[256];

void init_table_entry(const uint8_t opcode, OpEffect &e) {
	const OpCode& oc = op_codes[opcode];
	memset(&e, 0, sizeof(e));

	e.mixing_low_high = false;
	e.size = OpEffect::Size::AUTOASSIGN;

	// TODO: Jumps and branches don't quite load... but they also dont move data... fix in op

	// Fill in source based on operand mode
	// TODO: Fill in DB for the modes that read it (only for non-jumps of course)
	switch(oc.mode) {
	case Operand::ABSOLUTE:
	case Operand::ABSOLUTE_INDIRECT:
	case Operand::ABSOLUTE_INDIRECT_LONG:
	case Operand::ABSOLUTE_LONG:
	case Operand::DIRECT_PAGE:
	case Operand::DIRECT_PAGE_INDIRECT:
	case Operand::DIRECT_PAGE_INDIRECT_LONG:
	case Operand::STACK_RELATIVE:
		e.source |= thing::MEM;
		break;
	case Operand::ABSOLUTE_INDEXED_X:
	case Operand::ABSOLUTE_INDEXED_X_INDIRECT:
	case Operand::ABSOLUTE_LONG_INDEXED_X:
	case Operand::DIRECT_PAGE_INDEXED_X:
	case Operand::DIRECT_PAGE_INDEXED_X_INDIRECT:
		e.source |= thing::MEM;
		e.side_source |= thing::X;
		break;
	case Operand::ABSOLUTE_INDEXED_Y:
	case Operand::DIRECT_PAGE_INDEXED_Y:
	case Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y:
	case Operand::DIRECT_PAGE_INDIRECT_LONG_INDEXED_Y:
	case Operand::STACK_RELATIVE_INDIRECT_INDEXED_Y:
		e.source |= thing::MEM;
		e.side_source |= thing::Y;
		break;
	case Operand::ACCUMULATOR:
		e.source |= thing::A;
		break;
	case Operand::IMMEDIATE_INDEX:
	case Operand::IMMEDIATE_MEMORY:
	case Operand::MANUAL:
	case Operand::BRANCH_8:
	case Operand::BRANCH_16:
		break;
	};

	switch(oc.mode) {
	case Operand::ABSOLUTE:
	case Operand::ABSOLUTE_INDEXED_X:
	case Operand::ABSOLUTE_INDEXED_Y:
	case Operand::ABSOLUTE_INDIRECT:
	case Operand::ABSOLUTE_INDEXED_X_INDIRECT:
	case Operand::DIRECT_PAGE_INDEXED_X:
	case Operand::DIRECT_PAGE_INDIRECT:
	case Operand::DIRECT_PAGE_INDIRECT_INDEXED_Y:
		e.side_source |= thing::DB;
		break;
	default:
		break;
	};

	switch(oc.op) {
	case Operation::AND:
	case Operation::EOR:
	case Operation::ORA:
	{
		e.source |= thing::A;
		e.dest |= thing::A;
		e.side_dest |= thing::NZ;
		break;
	}
	case Operation::ASL:
	case Operation::LSR:
	{
		e.size = OpEffect::Size::MEMORY;
		e.dest |= e.source;
		e.side_dest |= thing::NZC;
		e.mixing_low_high = true;
		break;
	}
	case Operation::ROL:
	case Operation::ROR:
	{
		e.size = OpEffect::Size::MEMORY;
		e.dest |= e.source;
		e.side_source |= thing::FLAG_CARRY;
		e.side_dest |= thing::NZC;
		e.mixing_low_high = true;
		break;
	}
	case Operation::ADC:
	case Operation::SBC:
	{
		e.source |= thing::A; // TODO: Perhaps this is a side source?
		e.dest |= thing::A;
		e.side_dest |= thing::NZC|thing::FLAG_OVERFLOW;
		e.mixing_low_high = true;
		break;
	}
	case Operation::PHP: {
		e.source |= thing::FLAGS_LOWER;
		e.dest |= thing::MEM;
		e.size = OpEffect::Size::SMALL;
		break;
	}
	case Operation::PLA: {
		e.source |= thing::MEM;
		e.dest |= thing::A;
		e.side_dest |= thing::NZ;
		break;
	}
	case Operation::PLB: {
		e.source |= thing::MEM;
		e.dest |= thing::DB;
		e.side_dest |= thing::NZ;
		e.size = OpEffect::Size::SMALL;
		break;
	}
	case Operation::PLY: {
		e.source |= thing::MEM;
		e.dest |= thing::Y;
		e.side_dest |= thing::NZ;
		break;
	}
	case Operation::PLX: {
		e.source |= thing::MEM;
		e.dest |= thing::X;
		e.side_dest |= thing::NZ;
		break;
	}
	case Operation::PLP: {
		e.source |= thing::MEM;
		e.dest |= thing::FLAGS_LOWER;
		e.size = OpEffect::Size::SMALL;
		// Will write upper byte of XY as well if we went to 8-bit index TODO
		break;
	}
	case Operation::PHD: {
		e.source |= thing::DP;
		e.dest |= thing::MEM;
		e.size = OpEffect::Size::WIDE;
		break;
	}
	case Operation::PHY: {
		e.source |= thing::Y;
		e.dest |= thing::MEM;
		e.size = OpEffect::Size::INDEX;
		break;
	}
	case Operation::PHX: {
		e.source |= thing::X;
		e.dest |= thing::MEM;
		e.size = OpEffect::Size::INDEX;
		break;
	}
	case Operation::PHA: {
		e.source |= thing::A;
		e.dest |= thing::MEM;
		e.size = OpEffect::Size::MEMORY;
		break;
	}
	case Operation::PHB: {
		e.source |= thing::DB;
		e.dest |= thing::MEM;
		e.size = OpEffect::Size::SMALL;
		break;
	}
	case Operation::PHK: {
		e.source |= thing::PB;
		e.dest |= thing::MEM;
		e.size = OpEffect::Size::SMALL;
		break;
	}
	case Operation::PLD: {
		e.source |= thing::MEM;
		e.dest |= thing::DP;
		e.side_dest |= thing::NZ;
		e.size = OpEffect::Size::WIDE;
		break;
	}
	case Operation::CLV:
	{
		e.dest |= thing::FLAG_OVERFLOW;
		e.size = OpEffect::Size::NONE;
		break;
	}
	case Operation::SEC:
	case Operation::CLC:
	{
		e.dest |= thing::FLAG_CARRY;
		e.size = OpEffect::Size::NONE;
		break;
	}
	case Operation::BIT: {
		e.size = OpEffect::Size::MEMORY;
		e.dest |= thing::FLAG_ZERO;
		if (oc.mode != Operand::IMMEDIATE_MEMORY)
			e.dest |= thing::FLAG_NEGATIVE|thing::FLAG_OVERFLOW;
		break;
	}
	case Operation::INY:
	{
		e.source |= thing::Y;
		e.dest |= thing::Y;
		e.side_dest |= thing::NZ;
		e.mixing_low_high = true;
		break;
	}
	case Operation::INX:
	{
		e.source |= thing::X;
		e.dest |= thing::X;
		e.side_dest |= thing::NZ;
		e.mixing_low_high = true;
		break;
	}
	case Operation::INC:
	case Operation::DEC:
	{
		e.dest |= e.source;
		e.side_dest |= thing::NZ;
		e.mixing_low_high = true;
		e.size = OpEffect::Size::MEMORY;
		break;
	}
	case Operation::DEY:
	{
		e.dest  |= thing::Y;
		e.source |= thing::Y;
		e.side_dest |= thing::NZ;
		e.mixing_low_high = true;
		break;
	}
	case Operation::DEX:
	{
		e.dest  |= thing::X;
		e.source |= thing::X;
		e.side_dest |= thing::NZ;
		e.mixing_low_high = true;
		break;
	}
	case Operation::TCS: {
		e.source |= thing::AL|thing::AH; // TODO: Is this a good way to indicate always 16-bit A?
		e.dest |= thing::S;
		e.side_dest |= thing::NZ;
		e.size = OpEffect::Size::WIDE;
		break;
	}
	case Operation::TSC:
	{
		e.dest  |= thing::AL|thing::AH; // TODO: Is this a good way to indicate always 16-bit A?
		e.source |= thing::S;
		e.side_dest |= thing::NZ;
		e.size = OpEffect::Size::WIDE;
		break;
	}
	case Operation::TCD:
	{
		e.dest  |= thing::DP;
		e.source |= thing::AL|thing::AH;
		e.side_dest |= thing::NZ;
		e.size = OpEffect::Size::WIDE;
		break;
	}
	case Operation::TDC:
	{
		e.dest |= thing::AL|thing::AH;
		e.source |= thing::DP;
		e.side_dest |= thing::NZ;
		e.size = OpEffect::Size::WIDE;
		break;
	}
	case Operation::TXA:
	{
		e.dest |= thing::A;
		e.source |= thing::X;
		e.side_dest |= thing::NZ;
		break;
	}
	case Operation::TAX:
	{
		e.dest |= thing::X;
		e.source |= thing::A;
		e.side_dest |= thing::NZ;
		break;
	}
	case Operation::TYA:
	{
		e.dest |= thing::A;
		e.source |= thing::Y;
		e.side_dest |= thing::NZ;
		break;
	}
	case Operation::TAY:
	{
		e.dest |= thing::Y;
		e.source |= thing::A;
		e.side_dest |= thing::NZ;
		break;
	}
	case Operation::TXS:
	{
		e.dest |= thing::S;
		e.source |= thing::XL|thing::XH;
		e.side_dest |= thing::NZ;
		e.size = OpEffect::Size::WIDE;
		break;
	}
	case Operation::TSX:
	{
		e.source |= thing::S;
		e.dest |= thing::XL|thing::XH;
		e.side_dest |= thing::NZ;
		e.size = OpEffect::Size::WIDE;
		break;
	}
	case Operation::TXY:
	{
		e.dest |= thing::Y;
		e.source |= thing::X;
		e.side_dest |= thing::NZ;
		break;
	}
	case Operation::TYX:
	{
		e.dest |= thing::X;
		e.source |= thing::Y;
		e.side_dest |= thing::NZ;
		break;
	}
	case Operation::PER:
	{
		e.dest |= thing::MEM;
		e.size = OpEffect::Size::WIDE; // We don't have this one afaik
		break;
	}
	case Operation::PEA:
	{
		e.dest |= thing::MEM;
		e.size = OpEffect::Size::WIDE;
		break;
	}
	case Operation::CPY:
	{
		e.source |= thing::Y;
		e.dest |= thing::NZC;
		break;
	}
	case Operation::CPX:
	{
		e.source |= thing::X;
		e.dest |= thing::NZC;
		break;
	}
	case Operation::CMP:
	{
		e.source |= thing::A;
		e.dest |= thing::NZC;
		break;
	}
	case Operation::STA:
	{
		e.dest = e.source;
		e.source = thing::A;
		break;
	}
	case Operation::STX:
	{
		e.dest = e.source;
		e.source = thing::X;
		break;
	}
	case Operation::STY:
	{
		e.dest = e.source;
		e.source = thing::Y;
		break;
	}
	case Operation::STZ:
	{
		e.dest = e.source;
		e.source = 0;
		e.size = OpEffect::Size::MEMORY;
		break;
	}
	case Operation::LDA:
	{
		if (oc.mode != Operand::IMMEDIATE_MEMORY)
			e.source |= thing::MEM;
		e.dest |= thing::A;
		break;
	}
	case Operation::LDX:
	{
		if (oc.mode != Operand::IMMEDIATE_INDEX)
			e.source |= thing::MEM;
		e.dest |= thing::X;
		break;
	}
	case Operation::LDY:
	{
		if (oc.mode != Operand::IMMEDIATE_INDEX)
			e.source |= thing::MEM;
		e.dest |= thing::Y;
		break;
	}
	case Operation::TSB:
	case Operation::TRB:
	{
		e.dest |= e.source;
		e.source |= thing::A;
		e.side_dest |= thing::FLAG_ZERO;
		e.size = OpEffect::Size::MEMORY;
		break;
	}
	case Operation::SEP:
	case Operation::REP:
	{
		e.size = OpEffect::Size::NONE;
		// Special case since the bits they affect depends on the operand
		// Tracking will special case these
		break;
	}
	case Operation::XBA:
	{
		// Here the model falls apart...  If we tracked AL we shall track AH and vice versa
		e.source |= thing::AL|thing::AH;
		e.dest |= thing::AL|thing::AH;
		e.side_dest |= thing::NZ;
		e.size = OpEffect::Size::WIDE;
		e.mixing_low_high = true;
		break;
	}
	case Operation::XCE:
	{
		// Here the model falls apart...  If we tracked carry we shall track emu and vice versa
		e.source |= thing::FLAG_CARRY|thing::FLAG_EMULATION;
		e.dest |= thing::FLAG_CARRY|thing::FLAG_EMULATION;
		e.size = OpEffect::Size::NONE;
		break;
	}
	case Operation::JSR: // Modifies stack
	{
		e.dest |= thing::MEM; // Pushes to stack
		e.size = OpEffect::Size::NONE;
		break;
	}
	case Operation::JSL: // Modifies stack
	{
		e.dest |= thing::MEM; // Pushes to stack
		e.source |= thing::PB;
		e.size = OpEffect::Size::NONE;
		break;
	}
	case Operation::RTI: // Modifies stack
	case Operation::RTS: // Modifies stack
	{
		e.source |= thing::MEM; // Pulls from stack
		e.size = OpEffect::Size::NONE;
		break;
	}
	case Operation::RTL: // Modifies stack
	{
		e.source |= thing::MEM; // Pulls from stack
		e.dest |= thing::PB;
		e.size = OpEffect::Size::NONE;
		break;
	}
	case Operation::BRK:
	case Operation::COP:
	case Operation::STP:
	case Operation::NOP:
	case Operation::BPL:
	case Operation::BCC:
	case Operation::BCS:
	case Operation::BEQ:
	case Operation::BMI:
	case Operation::BNE:
	case Operation::BRA:
	case Operation::BVC:
	case Operation::BVS:
	case Operation::JMP:
	case Operation::JML:
	case Operation::WDM:
	case Operation::BRL:
	case Operation::MVP:
	case Operation::MVN:
	case Operation::WAI:
	case Operation::PEI:
	case Operation::SED: // We don't track decimal flag
	case Operation::CLD: // We don't track decimal flag
	case Operation::CLI: // We don't track IRQ flag
	case Operation::SEI: // We don't track IRQ flag
	{
		e.size = OpEffect::Size::NONE;
		// TODO
		break;
	}
	default:
		printf("%s\n", Operation_names[(int)oc.op]);
	};

	if (e.size == OpEffect::Size::AUTOASSIGN) {
		if (e.dest & thing::A) e.size = OpEffect::Size::MEMORY;
		if (e.dest & thing::X) e.size = OpEffect::Size::INDEX;
		if (e.dest & thing::Y) e.size = OpEffect::Size::INDEX;
	}
	if (e.size == OpEffect::Size::AUTOASSIGN) {
		if (e.source & thing::A) e.size = OpEffect::Size::MEMORY;
		if (e.source & thing::X) e.size = OpEffect::Size::INDEX;
		if (e.source & thing::Y) e.size = OpEffect::Size::INDEX;
	}

	if (e.size == OpEffect::Size::AUTOASSIGN) {
		printf("%s %s\n", Operation_names[(int)oc.op], Operand_names[(int)oc.mode]);
		exit(1);
	}
}
void init_table() {
	for (int k=0; k<256; k++) {
		init_table_entry(k, op_effects[k]);
		CUSTOM_ASSERT((op_effects[k].side_source & thing::A)==0);
		/*
		fprintf(report, "{");
		fprint_thing_hej(report, op_effects[k].source);
		fprintf(report, ", ");
		fprint_thing_hej(report, op_effects[k].dest);
		fprintf(report, ", ");
		fprint_thing_hej(report, op_effects[k].side_source);
		fprintf(report, ", ");
		fprint_thing_hej(report, op_effects[k].side_dest);
		fprintf(report, "}, %s 0x%02X\n", Operation_names[(int)op_codes[k].op], k);
		*/
	}
}


/*
	Lets look at how a value can propagate forward:
	* Read from ROM into register
	* Produced by arithmetic into registers
	* Write from register into memory
	* Read from memory into register
	* And at some point we have the breakpoint with value in reg or mem

	Some operation like LSR can read mem, modify and write back. That does nothing to suspicion.
*/

namespace {

using namespace snestistics;

struct Event {
	uint32_t _pc_bitset = 0;
	uint32_t data_pointer = INVALID_POINTER;
	uint32_t _nmi_event = 0;

	uint32_t nmi() const { return _nmi_event & 0xFFFFFF; }
	Events event() const { return (Events)(_nmi_event>>24); }

	Pointer pc() const { return _pc_bitset & 0x00FFFFFF; }
	void set_pc(Pointer p) { _pc_bitset = (_pc_bitset & 0xFF000000)|p; }

	enum Bits {
		BIT_MEMORY_FLAG=1<<24,
		BIT_INDEX_FLAG=2<<24,
		BIT_HAD_READ_FLAG=4<<24,
		BIT_HAD_WRITE_FLAG=8<<24,
		SIZE_1_FLAG=16<<24,
		SIZE_2_FLAG=32<<24,
		SIZE_3_FLAG=64<<24,
		SIZE_4_FLAG=128<<24
	};

	void set_bit(const Bits b) {
		_pc_bitset |= (uint32_t)b;
	}

	int data_size() const {
		uint32_t b = _pc_bitset;
		if (b & SIZE_1_FLAG) return 1;
		if (b & SIZE_2_FLAG) return 2;
		if (b & SIZE_3_FLAG) return 3;
		if (b & SIZE_4_FLAG) return 4;
		return 0;
	}
	bool memory_flag() const { return (_pc_bitset & BIT_MEMORY_FLAG) != 0; }
	bool index_flag() const  { return (_pc_bitset & BIT_INDEX_FLAG) != 0; }	
	bool had_read() const    { return (_pc_bitset & BIT_HAD_READ_FLAG) != 0; }	
	bool had_write() const   { return (_pc_bitset & BIT_HAD_WRITE_FLAG) != 0; }	

	void set_data_size(int v) {
		if (v == 1) _pc_bitset |= SIZE_1_FLAG;
		else if (v == 2) _pc_bitset |= SIZE_2_FLAG;
		else if (v == 3) _pc_bitset |= SIZE_3_FLAG;
		else if (v == 4) _pc_bitset |= SIZE_4_FLAG;
		else {
			CUSTOM_ASSERT(false);
		}
	}
};

struct State {
	BlockVector<Event> events;
};

void read_function(void* context, Pointer location, Pointer remapped_location, uint32_t value, int num_bytes, MemoryAccessType reason) {
	Event &event = ((State*)context)->events.last();
	if (reason == MemoryAccessType::RANDOM || reason == MemoryAccessType::STACK_RELATIVE) {
		if (event.data_pointer != INVALID_POINTER) {
			CUSTOM_ASSERT(event.data_pointer == remapped_location);
			CUSTOM_ASSERT(event.data_size() == num_bytes);
		} else {
			event.data_pointer = remapped_location;
			event.set_data_size(num_bytes);
		}
		event.set_bit(Event::BIT_HAD_READ_FLAG);
	}
}

void write_function(void* context, Pointer location, Pointer remapped_location, uint32_t value, int num_bytes, MemoryAccessType reason) {
	Event &event = ((State*)context)->events.last();
	if (reason == MemoryAccessType::RANDOM || reason == MemoryAccessType::STACK_RELATIVE) {
		if (event.data_pointer != INVALID_POINTER) {
			CUSTOM_ASSERT(event.data_pointer == remapped_location);
			CUSTOM_ASSERT(event.data_size() == num_bytes);
		} else {
			event.data_pointer = remapped_location;
			event.set_data_size(num_bytes);
		}
		event.set_bit(Event::BIT_HAD_WRITE_FLAG);
	}
}

struct Suspect {
	Suspect() {}
	Suspect(uint32_t rm, uint32_t p) { reg_mask = rm; prev_events.push_back(p); }
	uint32_t reg_mask = thing::NONE;
	Pointer mem_ptr = 0;
	bool dead = false;
	std::vector<uint32_t> prev_events;
	bool operator<(const Suspect &o) const {
		if (reg_mask != o.reg_mask) return reg_mask < o.reg_mask;
		if (mem_ptr != o.mem_ptr) return mem_ptr < o.mem_ptr;
		return this < &o; // Tie-breaker
	}
};

void update_suspect(Suspect &s, const uint32_t new_op, std::vector<snestistics::tracking::Value> &out_values, const Suspect &prev_suspect) {
	for (uint32_t prev_event : prev_suspect.prev_events) {
		snestistics::tracking::Value v;
		for (int k=0; k<19; k++) {
			if (s.reg_mask == 1<<k) {
				v.type = (snestistics::tracking::Value::Type)k;
				break;
			}
		}
		v.adress = s.mem_ptr;
		v.event_consumer = prev_event;
		v.event_producer = new_op;
		v.value = 0; // TBD by re-emulating
		v.wide = false; // Always small here until a later merge
		out_values.push_back(v);
	}

	s.prev_events.clear();
	s.prev_events.push_back(new_op);
}

void merge_and_trim_suspects(std::vector<Suspect> &suspects, uint32_t *all_suspects_mask) {
	uint32_t full_mask = 0;

	// Remove dead suspects
	for (auto it =  suspects.begin(); it != suspects.end();) {
		if (it->dead) {
			it = suspects.erase(it);
		} else {
			full_mask |= it->reg_mask;
			++it;
		}
	}

	// We can write out the full mask here since merging below will not change the suspects, only remove duplicates
	*all_suspects_mask = full_mask;

	if (suspects.empty())
		return;

	// Given that we have few suspects a vector works fine, we could use a set otherwise
	std::sort(suspects.begin(), suspects.end());

	bool did_you_ever_kill_someone = false;
	int last_alive = 0;
	for (int k=last_alive+1; k<(int)suspects.size(); k++) {
		Suspect &a = suspects[last_alive];
		Suspect &b = suspects[k];
		if (a.reg_mask == b.reg_mask && a.mem_ptr == b.mem_ptr) {
			for (auto j : b.prev_events) a.prev_events.push_back(j);
			b.prev_events.clear();
			std::sort(a.prev_events.begin(), a.prev_events.end()); // TODO: This is stupid but short list
			auto it = std::unique(a.prev_events.begin(), a.prev_events.end());
			a.prev_events.resize(std::distance(a.prev_events.begin(), it));

			// Merge back and mark as dead
			b.dead = true;
			did_you_ever_kill_someone = true;
		} else {
			last_alive = k;
			// we've got a new merge point
		}
	}
	if (did_you_ever_kill_someone) {
		for (auto it = suspects.begin(); it != suspects.end();) {
			if (it->dead) {
				it = suspects.erase(it);
			} else {
				++it;
			}
		}
	}
}

void write_event_label(FILE *report_dot, const snestistics::tracking::Event &e, const RomAccessor &rom, bool write_nmi = true) {
	uint8_t opcode = rom.evalByte(e.pc);
	fprintf(report_dot, "%s[%02X] at %06X", Operation_names[(int)op_codes[opcode].op], opcode, e.pc);

	const Operand mode = op_codes[opcode].mode;
	if (mode == Operand::MANUAL) {
	} else if (mode == Operand::IMMEDIATE_INDEX || mode == Operand::IMMEDIATE_MEMORY) {
		bool wide = mode == Operand::IMMEDIATE_INDEX ? !e.index_flag : !e.memory_flag;
		CUSTOM_ASSERT(wide == e.wide);
		// Note all operands are 1 byte and if they have an immediate that is all they have so it is at pc+1
		if (wide) {
			uint16_t value = *(uint16_t*)rom.evalPtr(e.pc+1);
			fprintf(report_dot, "\nimmediate %04X", value);
		} else {
			uint8_t value = rom.evalByte(e.pc+1);
			fprintf(report_dot, "\nimmediate %02X", value);
		}
	} else {
		fprintf(report_dot, "\n%s", Operand_names[(int)mode]);
	}
	if (e.data_pointer != INVALID_POINTER) {
		// Should always exist as an input unless we are doing a STA or so
		fprintf(report_dot, "\n%06X", e.data_pointer);
	}
	if (write_nmi)
		fprintf(report_dot, "\nnmi=%d", e.nmi);

	if (!e.wide)
		fprintf(report_dot, "\n8-bit");
	fprintf(report_dot, "\n");
}

void write_value_label(FILE *report_dot, const snestistics::tracking::Value &v, const bool inline_mode = false) {
	if (inline_mode) {
		fprintf(report_dot, "=> ");
	}
	if (v.type == tracking::Value::MEM) {
		fprintf(report_dot, "mem %06X", v.adress);
	} else {
		fprint_thing(report_dot, 1<<(uint32_t)v.type);
	}
	if (inline_mode) {
		fprintf(report_dot, "=");
	} else {
		fprintf(report_dot, "\n");
	}
	if (v.wide) {
		fprintf(report_dot, "%04X", v.value);
	} else {
		fprintf(report_dot, "%02X", v.value);
	}
	fprintf(report_dot, "\n");
}

void write_graph1(const std::string &filename, const RomAccessor &rom, std::vector<snestistics::tracking::Event> &out_events, const std::vector<snestistics::tracking::Value> &out_values) {
	const uint32_t target_nmi = out_events[0].nmi;

	printf("Write graph..\n");
	FILE *report_dot = fopen(filename.c_str(), "wt");
	fprintf(report_dot, "digraph track {\n");

	// The events are ordered... first one should come last

	// Events
	for (uint32_t i=0; i<out_events.size(); ++i) {
		const tracking::Event &e = out_events[i];
		fprintf(report_dot, "\tEvent_%d [label=\"", i);
		write_event_label(report_dot, e, rom, e.nmi != target_nmi);

		if (i==0)
			fprintf(report_dot, "\nMURDER INVESTIGATION!");

		fprintf(report_dot, "\"");

		if (e.data_pointer != INVALID_POINTER) {
			uint8_t bank = e.data_pointer >> 16;
			if (bank != 0x7E && bank != 0x7F)
				fprintf(report_dot, " shape=diamond");
		}

		if(i==0)
			fprintf(report_dot, " color=red");

		fprintf(report_dot, " style = filled");
			
		fprintf(report_dot, "];\n");
	}

	// Since we draw the values using boxes we don't want to have duplicated boxes for the same value
	// So we do a pass over them to remove some
	std::map<tracking::Value, uint32_t> event_value_taken;

	// Connections
	for (uint32_t i=0; i<out_values.size(); ++i) {
		tracking::Value v = out_values[i]; // Make copy here
		const uint32_t event_from = v.event_consumer;
		v.event_consumer = 0;

		uint32_t value_box_to_use = i;

		auto vit = event_value_taken.find(v);

		if (vit != event_value_taken.end()) {
			value_box_to_use = vit->second;
		} else {
			event_value_taken[v] = i; // Let our index i be the go-to index for this value

			fprintf(report_dot, "\tValue_%d [ label = \"", i);
			write_value_label(report_dot, v);
			fprintf(report_dot, "\"");
			fprintf(report_dot, " shape=box");
			fprintf(report_dot, "];\n");

			fprintf(report_dot, "\tEvent_%d->Value_%d", v.event_producer, value_box_to_use);
		}

		fprintf(report_dot, "\tValue_%d->Event_%d", value_box_to_use, event_from);
	}

	fprintf(report_dot, "}\n");
	fclose(report_dot);
}

/*
	This is an attempt to group nodes so the graph becomes more readable.
	As attempts go it feels quite successful.
*/
void write_graph2(const std::string &filename, const RomAccessor &rom, std::vector<snestistics::tracking::Event> &out_events, const std::vector<snestistics::tracking::Value> &out_values) {
	const uint32_t target_nmi = out_events[0].nmi;

	printf("Write graph..\n");
	FILE *report_dot = fopen(filename.c_str(), "wt");
	fprintf(report_dot, "digraph track {\n");

	struct EventDependency {
		uint32_t event_consume = INVALID_POINTER;
		uint32_t event_produce = INVALID_POINTER;
		bool multiple_producers = false, multiple_consumers = false;
		tracking::Value value_candidate;
		bool produces_multiple_values = false;
		bool has_value_candidate = false;
	};

	std::vector<EventDependency> dependency(out_events.size());
	for (const auto &v : out_values) {
		const uint32_t consumer = v.event_consumer, producer = v.event_producer;
		{
			// Update stats for the event producing values that we consume... add our consuming event
			EventDependency&r = dependency[producer];
			if (!r.multiple_consumers && (r.event_consume == INVALID_POINTER || r.event_consume == consumer)) {
				r.event_consume = consumer;
			} else {
				r.event_consume = INVALID_POINTER;
				r.multiple_consumers = true;
			}

			// Value tracking of producer to know if the producing event only produces one value
			tracking::Value vc = v;
			vc.event_consumer = 0; // Zero out the consumer since we want to see if we have same value, not same link
			if (!r.has_value_candidate) {
				r.value_candidate = vc;
			} else if (r.value_candidate != vc) {
				r.produces_multiple_values = true;
			}
			r.has_value_candidate = true;
		}
		{
			// Update stats for the event consuming this values... add our producing event
			EventDependency&r = dependency[consumer];
			if (!r.multiple_producers && (r.event_produce == INVALID_POINTER || r.event_produce == producer)) {
				r.event_produce = producer;
			} else {
				r.event_produce = INVALID_POINTER;
				r.multiple_producers = true;
			}
		}
	}

	std::vector<int> group(out_events.size(), -1);

	// Traverse event from the first producer... due to construction the events are correctly ordered already topologically if iterated in reverse
	// Here we take an event and figures out if it should be merged with the next one
	for (int i=(int)out_events.size()-1; i>=0; --i) {
		// We are not beginning/continuing a chain
		const EventDependency&r = dependency[i];

		const bool starting_group = group[i]==-1;

		// We can continue a group if we have multiple consumers (or none)
		if (r.multiple_consumers || r.event_consume == INVALID_POINTER)
			continue;
		if (!starting_group && r.multiple_producers)
			continue;

		// Keep the murder suspect output alone
		if (r.event_consume == 0)
			continue;

		const EventDependency& consumer_dependency = dependency[r.event_consume];

		// The start of a chain can consume values from multiple events, but an event inside the chain must only have one producing event
		if (consumer_dependency.multiple_producers)
			continue;

		// Don't merge if different nmi
		if (out_events[i].nmi != out_events[r.event_consume].nmi)
			continue;

		// NOTE: We might already be part of a group, if so just propagate number forward
		if (starting_group)
			group[i] = i;

		group[r.event_consume] = group[i]; // Let our consumer inherit our group
	}

	std::vector<bool> group_done(out_events.size(), false);

	// Events
	for (int i=(int)out_events.size()-1; i>=0; --i) {
		if (group[i] == -1) {
			const tracking::Event &e = out_events[i];
			fprintf(report_dot, "\tEvent_%d [label=\"", i);
			write_event_label(report_dot, e, rom, e.nmi != target_nmi);

			if (i==0)
				fprintf(report_dot, "\nMURDER INVESTIGATION!");

			// Inline the value if we only produce one
			if (!dependency[i].produces_multiple_values && dependency[i].has_value_candidate) {
				fprintf(report_dot, "\n");
				write_value_label(report_dot, dependency[i].value_candidate, true);
			}

			fprintf(report_dot, "\"");

			if (e.data_pointer != INVALID_POINTER) {
				uint8_t bank = e.data_pointer >> 16;
				if (bank != 0x7E && bank != 0x7F)
					fprintf(report_dot, " shape=diamond");
			}

			if(i==0)
				fprintf(report_dot, " color=red");

			fprintf(report_dot, " style=filled");
			
			fprintf(report_dot, "];\n");
		} else {
			int g = group[i];
			if (group_done[g])
				continue;
			group_done[g] = true;

			fprintf(report_dot, "\tEvent_%d [label=\"", g);

			bool first = true;

			// Only report NMI once for groups
			if (out_events[i].nmi != target_nmi)
				fprintf(report_dot, "group nmi=%d\n\n", out_events[i].nmi);

			// TODO: Very bad performance here... Optimize for larger graphs!
			for (int j=(int)out_events.size()-1; j>=0; --j) {
				if (group[j]!=g) continue;
				if (!first) fprintf(report_dot, "\n");
				write_event_label(report_dot, out_events[j], rom, false);
				first = false;

				// Find the value this event is producing (IF the next event is also in chain)
				uint32_t producer = j;

				const bool event_produces_multiple_values = dependency[producer].produces_multiple_values;

				if (!event_produces_multiple_values) {
					// always inline our value since we are in the group and we only produce one value

					// This is the usual case and it is fast
					fprintf(report_dot, "\n");
					if (dependency[producer].has_value_candidate)
						write_value_label(report_dot, dependency[producer].value_candidate, true);

				} else {
					// We inline values from an event even if it produces many values if the consumer is in the group
					uint32_t consumer = dependency[producer].event_consume;
					if (consumer == INVALID_POINTER || group[consumer] != g)
						continue;

					bool first_value = true;

					// TODO: Even worse performance here... Optimize for larger graphs!
					for (const auto &v : out_values) {
						if (v.event_producer == producer) {
							if (!first_value) fprintf(report_dot, "\n");
							first_value = false;
							write_value_label(report_dot, v, true);
						}
					}
				}
			}

			fprintf(report_dot, "\"");
			fprintf(report_dot, " shape=box");
			fprintf(report_dot, " style=filled");
			fprintf(report_dot, "];\n");
		}
	}

	// Since we draw the values using boxes we don't want to have duplicated boxes for the same value
	// So we keep track of boxes we've omitted and redirect accordingly
	std::map<tracking::Value, uint32_t> event_value_taken;

	// Connections
	for (uint32_t i=0; i<out_values.size(); ++i) {
		tracking::Value v = out_values[i]; // Make copy here

		// If this value is internal in a group then just omit it
		int32_t g1 = group[v.event_consumer], g2 = group[v.event_producer];
		if(g1 != -1 && g1==g2)
			continue;

		// If this value is to sole value produced by an event, omit it
		if (!dependency[v.event_producer].produces_multiple_values) {
			// Value has been inlined in a event or group box so we just need to connect it
			uint32_t event_producer = group[v.event_producer]==-1 ? v.event_producer : group[v.event_producer];
			uint32_t event_consumer = group[v.event_consumer]==-1 ? v.event_consumer : group[v.event_consumer];
			fprintf(report_dot, "\tEvent_%d->Event_%d", event_producer, event_consumer);
			continue;
		}

		const uint32_t event_consumer = v.event_consumer;
		v.event_consumer = 0;

		uint32_t value_box_to_use = i;

		auto vit = event_value_taken.find(v);

		if (vit != event_value_taken.end()) {
			value_box_to_use = vit->second;
		} else {
			event_value_taken[v] = i; // Let our index i be the go-to index for this value

			fprintf(report_dot, "\tValue_%d [ label = \"", i);
			write_value_label(report_dot, v);
			fprintf(report_dot, "\"");
			fprintf(report_dot, " shape=box");
			fprintf(report_dot, "];\n");

			uint32_t event_producer = group[v.event_producer]==-1 ? v.event_producer : group[v.event_producer];

			fprintf(report_dot, "\tEvent_%d->Value_%d", event_producer, value_box_to_use);
		}

		uint32_t group_event_consumer = group[event_consumer]==-1 ? event_consumer : group[event_consumer];
		fprintf(report_dot, "\tValue_%d->Event_%d", value_box_to_use, group_event_consumer);
	}

	fprintf(report_dot, "}\n");
	fclose(report_dot);
}

}

namespace snestistics {

void rewind_report(const Options &options, const RomAccessor &rom, const AnnotationResolver &annotations) {

	CUSTOM_ASSERT(options.trace_files.size() == 1);

	printf("Tracking...\n");

	// NOTE: Always track AL or AL|AH at track_pc (even it doesn't use AL|AH), start at track_nmi.
	//       If the op at track_pc has 16-bit memory flag then AH is tracked, otherwise just AL
	//uint32_t track_pc = 0x808930, track_nmi = 0;
	uint32_t track_pc = 0x879500, track_nmi = 4500;
	//uint32_t track_pc = 0x80895F, track_nmi = 0;

	const uint32_t original_nmi = track_nmi;
	int nmi = original_nmi;
	Replay replay(rom, options.trace_files[0].c_str());
	EmulateRegisters &regs = replay.regs;
	replay.skip_until_nmi(options.trace_file_skip_cache(0).c_str(), nmi);

	std::vector<tracking::Event> out_events;
	std::vector<tracking::Value> out_values;

	init_table();

	State state;

	// These are reads to memory other than ROM/SRAM (outside what we emulate)

	std::vector<Suspect> suspects;

	// Set after skip
	regs._read_function = read_function;
	regs._write_function = write_function;
	regs._callback_context = &state;

	// First find first suspect
	while (true) {
		Pointer pc = regs._PC;

		uint64_t op = state.events.size();

		Event &event = state.events.push_back();
		event.data_pointer = INVALID_POINTER;
		event.set_pc(pc);
		const bool old_memory_flag = regs.P_flag(ProcessorStatusFlag::MemoryFlag);
		const bool old_index_flag = regs.P_flag(ProcessorStatusFlag::IndexFlag);

		if (old_memory_flag) event.set_bit(Event::BIT_MEMORY_FLAG);
		if (old_index_flag ) event.set_bit(Event::BIT_INDEX_FLAG);

		uint8_t opcode = regs._memory[regs._PC];

		if (!replay.next())
			break;

		if (regs.event == Events::NMI) nmi++;
		if (regs.event == Events::NMI) opcode = 0;
		if (regs.event == Events::RESET) opcode = 0;
		if (regs.event == Events::IRQ) opcode = 0;

		event._nmi_event = ((uint32_t)regs.event<<24)|nmi;
		if (regs.event != Events::NMI)
		{
			const OpEffect &effect = op_effects[opcode];
			int opsize = effect.actual_size(event.memory_flag(), event.index_flag());
			static const char* size_names[]={"SMALL", "WIDE", "LONG", "INDEX", "MEMORY", "NONE", "AUTOASSIGN"};

			//printf("0x%02X size=%d %s %s effect_size=%s data_size=%d\n", opcode, opsize, Operation_names[(int)op_codes[opcode].op], Operand_names[(int)op_codes[opcode].mode], size_names[(int)effect.size], event.data_size());

			CUSTOM_ASSERT(event.data_pointer == INVALID_POINTER || opsize == event.data_size() || effect.size == OpEffect::Size::NONE);
		}

		bool found = false;

		// Hard-wirded questions for now
		if (pc == track_pc) {
			found = true;
			{
				Suspect suspect;
				suspect.reg_mask = thing::AL;
				suspect.prev_events.push_back(0);
				suspects.push_back(suspect);
			}
			if (!regs.P_flag(ProcessorStatusFlag::MemoryFlag))
			{
				Suspect suspect;
				suspect.reg_mask = thing::AH;
				suspect.prev_events.push_back(0);
				suspects.push_back(suspect);
			}
			printf("Tracking PC %06X when A=%04X\n", pc, regs._A);
			tracking::Event start_event;
			start_event.opcount = op+1; // Wrong but that is ok
			start_event.pc = track_pc;
			start_event.nmi = nmi;
			start_event.memory_flag = regs.P_flag(ProcessorStatusFlag::MemoryFlag);
			start_event.index_flag = regs.P_flag(ProcessorStatusFlag::MemoryFlag);
			start_event.emulation_flag = regs.P_flag(ProcessorStatusFlag::Emulation);
			out_events.push_back(start_event);
			break;
		}
	}

	printf("Event size %.2f MB (%d bytes per event)\n", state.events.size()*sizeof(Event)/1024.0f/1024.0f, (int)sizeof(Event));

	uint32_t all_suspects_mask = 0;
	merge_and_trim_suspects(suspects, &all_suspects_mask); // In case we were sloppy, merge

	std::vector<Suspect> updated_suspects;

	for (int32_t op = (int32_t)state.events.size()-1; op>=0; --op) {
		if (suspects.empty())
			break;

		const Event &e = state.events[op];
		if (e.event() == Events::NMI || e.event() == Events::RESET || e.event() == Events::IRQ)
			continue;

		const uint8_t event_opcode = regs._memory[e.pc()];
		const OpEffect &effect = op_effects[event_opcode];

		// Do sloppy culling based on combined mask of all suspets (all_suspects_mask)
		{
			uint32_t dest_mask = effect.dest|effect.side_dest;
			if (dest_mask & thing::A) dest_mask |= thing::AL|thing::AH;
			if (dest_mask & thing::X) dest_mask |= thing::XL|thing::XH;
			if (dest_mask & thing::Y) dest_mask |= thing::YL|thing::YH;
			if ((dest_mask & all_suspects_mask)==0)
				continue;
		}

		if ((op%10000)==0) {
			printf("%.1f%% %d suspects\n", 100.0f*(state.events.size()-op)/(float)(state.events.size()), (int)suspects.size());
		}

		// This op does not produce anything that we suspect... Memory makes this less efficient than it could be
		const int opsize = effect.actual_size(e.memory_flag(), e.index_flag());

		//printf("0x%02X size=%d %s %s effect_size=%d\n", event_opcode, opsize, Operation_names[(int)op_codes[event_opcode].op], Operand_names[(int)op_codes[event_opcode].mode], effect.size);

		CUSTOM_ASSERT(e.data_pointer == INVALID_POINTER || opsize == e.data_size() || effect.size == OpEffect::Size::NONE);

		updated_suspects.clear();
		bool has_kill = false;

		bool event_logged_dot = false;

		uint32_t current_out_event = (uint32_t)out_events.size() - 1;

		for (uint32_t sidx = 0; sidx < suspects.size(); ++sidx) {
			Suspect &s = suspects[sidx];
			if (s.dead)
				continue;

			const Suspect original_s = s;

			bool want_high = false;

			uint32_t event_write_mask = effect.dest|effect.side_dest;
			if (((event_write_mask & thing::A)!=0) && (s.reg_mask & (thing::AL|thing::AH))!=0) {
				if (s.reg_mask == thing::AH) want_high = true;
			} else if (((event_write_mask & thing::X)!=0) && (s.reg_mask & (thing::XL|thing::XH))!=0) {
				if (s.reg_mask == thing::XH) want_high = true;
			} else if (((event_write_mask & thing::Y)!=0) && (s.reg_mask & (thing::YL|thing::YH))!=0) { 
				if (s.reg_mask == thing::YH) want_high = true;
			} else if ((event_write_mask & s.reg_mask)==0) { // No overlap
				continue;
			}
			if (s.reg_mask == thing::MEM) {
				// Ok we suspect a mem write and we have a mem write, but it the adress correct?
				// Same bank?
				if ((s.mem_ptr>>16) != (e.data_pointer>>16)) {
					// We always works with remaps for now so this is enough
					continue;
				}
				// TODO: Here write_size can be taken from op_effects[e.opcode].size
				if (s.mem_ptr < e.data_pointer || s.mem_ptr >= e.data_pointer + e.data_size()) {
					continue;
				}

				bool has_high = e.data_size() == 2;
				if (want_high && !has_high && !effect.mixing_low_high) continue;
				if (has_high && e.data_pointer+1 == s.mem_ptr) {
					want_high = true;
				}
			}

			if (!event_logged_dot) {
				tracking::Event se;
				se.wide = opsize == 2;
				se.pc = e.pc();
				se.data_pointer = e.data_pointer;
				se.nmi = e.nmi();
				se.index_flag = e.index_flag();
				se.memory_flag = e.memory_flag();
				se.opcount = op;
				se.had_write = e.had_write();
				se.had_read = e.had_read();
				out_events.push_back(se);
				event_logged_dot = true;
				current_out_event++;
			}

			s.dead = true;

			has_kill = true;

			// TODO: Add the special cases where P due to PLP and XH/YH are set to 0

			// Ok this op produces that which we suspect. Switch suspicion to all inputs
			if (effect.source == thing::NONE && effect.side_source == thing::NONE) {
				update_suspect(s, current_out_event, out_values, original_s);
			} else if (event_opcode == 0xEB) { // XBA
				s.dead = false;
				CUSTOM_ASSERT(s.reg_mask == thing::AL || s.reg_mask == thing::AH || s.reg_mask == thing::A);
				if (s.reg_mask == thing::A) {
					s.reg_mask = thing::A;
				} else if (s.reg_mask == thing::AL) {
					s.reg_mask = thing::AH;
				} else if (s.reg_mask == thing::AH) {
					s.reg_mask = thing::AL;
				}
				update_suspect(s, current_out_event, out_values, original_s);
	
			} else if (event_opcode == 0xFB) { // XCE

				if (s.reg_mask == thing::FLAG_EMULATION) { s.reg_mask = thing::FLAG_CARRY; s.dead = false; }
				else if (s.reg_mask == thing::FLAG_CARRY) { s.reg_mask = thing::FLAG_EMULATION; s.dead = false; }

				update_suspect(s, current_out_event, out_values, original_s);
	
			} else if (event_opcode == 0xC2 || event_opcode == 0xE2) { // REP or SEP
				const uint8_t param = regs._memory[e.pc()-1];
				bool found = false;
				if (s.reg_mask == thing::FLAG_ZERO      && (param & (uint8_t)ProcessorStatusFlag::Zero     )) found = true;
				if (s.reg_mask == thing::FLAG_NEGATIVE  && (param & (uint8_t)ProcessorStatusFlag::Negative )) found = true;
				if (s.reg_mask == thing::FLAG_CARRY     && (param & (uint8_t)ProcessorStatusFlag::Carry    )) found = true;
				if (s.reg_mask == thing::FLAG_EMULATION && (param & (uint8_t)ProcessorStatusFlag::Emulation)) found = true;
				if (s.reg_mask == thing::FLAG_OVERFLOW  && (param & (uint8_t)ProcessorStatusFlag::Overflow )) found = true;

				if (event_opcode == 0xE2) { // SEP, settings bit to one
					if (param & (uint8_t)ProcessorStatusFlag::IndexFlag) {
						if (s.reg_mask == thing::X || s.reg_mask == thing::Y) {
							s.reg_mask = s.reg_mask == thing::X ? thing::XH : thing::YH; // We only satisfied H here so restrict
							Suspect side_s;
							side_s.dead = false;
							side_s.mem_ptr = INVALID_POINTER;
							side_s.prev_events = s.prev_events;
							side_s.reg_mask = s.reg_mask == thing::X ? thing::XL : thing::YL;
							updated_suspects.push_back(side_s); // Keep tracking XL
							// TODO: We could reuse s here, but update_suspects is a bit clunky
							found = true;
						} else if (s.reg_mask == thing::XH) {
							found = true;
						} else if (s.reg_mask == thing::YH) {
							found = true;
						}
					}
				}

				if (found) {
					// We saistied this one
					update_suspect(s, current_out_event, out_values, original_s);
				} else {
					s.dead = false; // Keep looking for this one
				}
	
			} else {

				/*
					Make sure index tracking is 8-bit OR 16-bit depending on index flag
				*/
				uint32_t taken = 0;

				// We are only allowed to suspect one thing per suspicion
				for (int k=0; k<=18; ++k) {
					uint32_t mask = 1<<k;
					if (taken & mask) continue; // We've already done this one

					bool primary = (effect.source & mask) != 0;
					bool side = (effect.side_source & mask) != 0;

					if (!primary && !side)
						continue;

					bool overcourse = false;

					// The following flags mostly affect branches and jumps and they are not the focus of the tracking view so turn them off for now
					if (mask == thing::PB) overcourse = true;
					if (mask == thing::FLAG_ZERO) overcourse = true;
					if (mask == thing::FLAG_NEGATIVE) overcourse = true;
					if (mask == thing::FLAG_OVERFLOW) overcourse = true;
					if (mask == thing::FLAG_EMULATION) overcourse = true;
					if (!primary && mask == thing::DB && ((effect.source & mask) == 0)) // DB side source, not primary
						overcourse = true;

					// For now disable overcourse
					if (overcourse)
						continue;

					if (mask == thing::A) {
						CUSTOM_ASSERT(!side);
						if (opsize == 2 && effect.mixing_low_high) {
							updated_suspects.push_back(Suspect(thing::AL, current_out_event));
							updated_suspects.push_back(Suspect(thing::AH, current_out_event));
							taken |= thing::AL|thing::AH;
						} else if (opsize == 2 && want_high) {
							updated_suspects.push_back(Suspect(thing::AH, current_out_event));
							taken |= thing::AH;
						} else if (!want_high) {
							updated_suspects.push_back(Suspect(thing::AL, current_out_event));
							taken |= thing::AL;
						}
						continue;
					} else if (mask == thing::A || mask == thing::X || mask == thing::Y) {
						bool mixing = effect.mixing_low_high;
						const bool is_index = mask == thing::X || mask == thing::Y;
						if (side && is_index && !e.index_flag()) {
							// There are two cases where we need to suspect both XL and XH even though we only need say XL
							// One is if the op is "mixing" (such as roll/shift/add) were bits are moved between low/high)
							// This happens when X is primary.
							// The other is if it used for indexing (side==true) and index flag is false (16-bit index)
							// A can never be used as index...
							mixing = true;
						}

						uint32_t lo = thing::AL, hi = thing::AH;
						if (is_index) {
							lo = mask == thing::X ? thing::XL : thing::YL;
							hi = mask == thing::X ? thing::XH : thing::YH;
						}
						if (opsize == 2 && mixing) {
							updated_suspects.push_back(Suspect(lo, current_out_event));
							updated_suspects.push_back(Suspect(hi, current_out_event));
							taken |= lo|hi;
						} else if (opsize == 2 && want_high) {
							updated_suspects.push_back(Suspect(hi, current_out_event));
							taken |= lo;
						} else if (!want_high) {
							updated_suspects.push_back(Suspect(lo, current_out_event));
							taken |= hi;
						}
						continue;
					}

					Suspect ns;
					ns.dead = false;
					ns.reg_mask = mask;
					ns.prev_events.clear();
					ns.prev_events.push_back(current_out_event);
					taken |= mask;

					if (mask == thing::MEM) {
						ns.mem_ptr = e.data_pointer; // Sometimes +1
						if (want_high) ns.mem_ptr++;
						// NOTE: If the pointer is outside operand it should be filtered above
					} else {
						ns.mem_ptr = INVALID_POINTER;
					}
					updated_suspects.push_back(ns);
				}

				update_suspect(s, current_out_event, out_values, original_s);
			}
		}

		if (event_logged_dot) {
			for (auto s : updated_suspects) {
				suspects.push_back(s);
			}
			updated_suspects.clear();
			merge_and_trim_suspects(suspects, &all_suspects_mask);

		} else {
			assert(updated_suspects.empty());
		}
	}

	if (!suspects.empty()) {
		printf("Still got %d suspect at end of emulation\n", (int)suspects.size());
	}
	
	if (true) {
		// TODO: We can use the skip cache to jump ahead if we knew at what nmi each event happens at (and we do!)
		printf("Re-emuluate to find values for all connections...\n");

		// TODO: We should be able to use regs now instead of creating a new one!
		EmulateRegisters regs2 = replay.regs;
		regs2._read_function = nullptr;
		regs2._write_function = nullptr;
		regs2._callback_context = nullptr;
		replay.skip_until_nmi(options.trace_file_skip_cache(0).c_str(), original_nmi);

		uint32_t next_event = (uint32_t)out_events.size()-1;
		uint64_t next_op = out_events[next_event].opcount;

		bool work_to_do = false;

		for (uint64_t opcount = 0; next_event != 0; opcount++) {
			
			replay.next(); // Is this correct to run op first and then see registers? I think so

			if (opcount == next_op) {
				const tracking::Event &e = out_events[next_event];

				// See what values we have that get their value from this one (TODO: Stupid way to do this!)
				for (auto &v : out_values) {
					if (v.event_producer == next_event) {
						     if (v.type == tracking::Value::AL) v.value = regs2.A(0xFF);
						else if (v.type == tracking::Value::AH) v.value = regs2.A(0xFF00)>>8;
						else if (v.type == tracking::Value::XL) v.value = regs2.X(0xFF);
						else if (v.type == tracking::Value::XH) v.value = regs2.X(0xFF00)>>8;
						else if (v.type == tracking::Value::YL) v.value = regs2.Y(0xFF);
						else if (v.type == tracking::Value::YH) v.value = regs2.Y(0xFF00)>>8;
						else if (v.type == tracking::Value::MEM) v.value = regs2._memory[v.adress];
						else if (v.type == tracking::Value::DB) { v.value = regs2.DB(); CUSTOM_ASSERT(!v.wide); }
						else if (v.type == tracking::Value::DP) { v.value = regs2.DP(); v.wide = true; }
						else if (v.type == tracking::Value::S) { v.value = regs2.S(); v.wide = true; }
						else if (v.type == tracking::Value::PB) { v.value = regs2.PC(0xFF0000)>>16; CUSTOM_ASSERT(!v.wide); }
						else if (v.type == tracking::Value::FLAG_CARRY) v.value = regs2.P_flag(ProcessorStatusFlag::Carry) ? 1:0;
						else if (v.type == tracking::Value::FLAG_EMULATION) v.value = regs2.P_flag(ProcessorStatusFlag::Emulation) ? 1:0;
						else if (v.type == tracking::Value::FLAG_ZERO) v.value = regs2.P_flag(ProcessorStatusFlag::Zero) ? 1:0;
						else if (v.type == tracking::Value::FLAG_NEGATIVE) v.value = regs2.P_flag(ProcessorStatusFlag::Negative) ? 1:0;
						else if (v.type == tracking::Value::FLAG_OVERFLOW) v.value = regs2.P_flag(ProcessorStatusFlag::Overflow) ? 1:0;
					}
				}
				next_event--;
				next_op = out_events[next_event].opcount;
			}
		}
	}

	// Merge AL|AH into A (same for X and Y) and adjecent memory accesses if they share from/to (same arrow in graph)
	if (true) {
		printf("Merge arrows..\n");
		std::sort(out_values.begin(), out_values.end());
		for (uint32_t i=1; i<out_values.size(); i++) {
			tracking::Value &a = out_values[i-1];
			tracking::Value &b = out_values[i];
			if (a.event_consumer != b.event_consumer || a.event_producer != b.event_producer)
				continue;
			bool merge = false;
			// Mostly safe-guard against producing stuff that is more than two bytes...
			if (a.wide || b.wide)
				continue;
			if (a.type == tracking::Value::MEM && b.type == tracking::Value::MEM) {
				if (a.adress+1 == b.adress)
					merge = true;
			} else if (a.type == tracking::Value::AL && b.type == tracking::Value::AH) {
				a.type = tracking::Value::A;
				merge = true;
			} else if (a.type == tracking::Value::XL && b.type == tracking::Value::XH) {
				a.type = tracking::Value::X;
				merge = true;
			} else if (a.type == tracking::Value::YL && b.type == tracking::Value::YH) {
				a.type = tracking::Value::Y;
				merge = true;
			}
			if (merge) {
				a.wide = true;
				a.value = a.value|(b.value<<8); // TODO: Correct?
				out_values.erase(out_values.begin() + i);
				--i;
			}
		}
	}

	//write_graph1(options.track_file, rom, out_events, out_values);
	write_graph2(options.rewind_file, rom, out_events, out_values);
}

}