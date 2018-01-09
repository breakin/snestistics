#pragma once

#include <string>

struct Options;

namespace snestistics {

class RomAccessor;
class AnnotationResolver;

namespace tracking {

struct Event {
	uint32_t pc; // TODO: To support self-modifying code we must store opcode as well

	bool memory_flag, index_flag, emulation_flag;
	uint32_t nmi;
	bool wide = false;

	uint32_t data_pointer = 0xFFFFFFFF;
	bool had_read = false, had_write = false;

	uint64_t opcount; // How many ops since the nmi we skipped to is this event
};

struct Value {

	enum Type {
		A=0, AL=1, AH=2,
		X=3, XL=4, XH=5,
		Y=6, YL=7, YH=8,
		FLAG_CARRY=9,
		FLAG_EMULATION=10,
		FLAG_OVERFLOW=11,
		FLAG_NEGATIVE=12,
		FLAG_ZERO=13,
		S=14,
		DB=15,
		PB=16, // Needed?
		MEM=17,
		DP=18,
		LAST=19
	};

	Type type;
	uint16_t value;
	uint32_t adress;
	bool wide = false;

	uint32_t event_consumer, event_producer;

	bool operator<(const Value &o) const {
		if (event_consumer != o.event_consumer) return event_consumer < o.event_consumer;
		if (event_producer != o.event_producer) return event_producer < o.event_producer;
		if (type != o.type) return type < o.type;
		if (adress != o.adress) return adress < o.adress;
		// Tie-breaker
		if (wide != o.wide) return wide;
		if (value != o.value) return value < o.value;
		return false;
	}

	bool operator!=(const Value &o) const {
		if (event_consumer != o.event_consumer) return true;
		if (event_producer != o.event_producer) return true;
		if (type != o.type) return true;
		if (adress != o.adress) return true;
		// Tie-breaker
		if (wide != o.wide) return true;
		if (value != o.value) return true;
		return false;
	}
};
}

void rewind_report(const Options &options, const RomAccessor &rom, const AnnotationResolver &annotations);
}
