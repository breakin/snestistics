#pragma once

#include <cstdint>

namespace snestistics {

	enum class TraceEventType : uint8_t {
		EVENT_NMI=0,
		EVENT_RESET=1,
		EVENT_IRQ=2,
		EVENT_READ_BYTE=3,
		EVENT_READ_WORD=4,
		EVENT_FINISHED=5,
		EVENT_DMA=6,
	};

	#pragma pack(push, 1)
	struct TraceEvent {
		TraceEventType type; // TODO: These two can probably be folded into one uint32. Perhaps we need a dummy event to reset the op counter
		uint32_t op_counter_delta;
	};

	struct TraceEventReadByte {
		uint32_t adress;
		uint8_t value;
	};

	struct TraceEventReadWord {
		uint32_t adress;
		uint16_t value;
	};

	struct TraceRegisters {
		uint16_t pc_address;
		uint16_t wram_address;
		uint16_t P, A, DP, S, X, Y;
		uint8_t pc_bank;
		uint8_t wram_bank;
		uint8_t DB;
	};

	struct TraceEventReset {
		TraceRegisters regs_after;
		// Followed by 128kb of SRAM
	};

	struct TraceSkipHeader {
		uint32_t version;
		uint32_t nmi_per_skip;
	};

	struct TraceSkip {
		uint32_t nmi;
		uint64_t seek_offset_trace_file;
		uint64_t current_op;
		TraceRegisters regs;
	};

	#pragma pack(pop)

	// Note: Helper are only for validating that emulation works, they are optional and go in a seperate file
	enum class HelperType : uint8_t { HELPER_OP };

	#pragma pack(push, 1)
	struct HelperOp {
		// One way to make this one smaller is to exclude PC from before and to have a mask saying what changes... And only state after for changed
		uint64_t current_op;
		TraceRegisters registers_before;
		TraceRegisters registers_after;
	};
	#pragma pack(pop)
}
