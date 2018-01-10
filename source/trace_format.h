#pragma once

#include <cstdint>

static const uint32_t TRACE_VERSION_NUMBER = 1;

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

	// Note: Helper are only for validating that emulation works, they are optional and go in a seperate file
	enum class HelperType : uint8_t { HELPER_OP };

	#pragma pack(push, 1)
	struct TraceHeader {
		uint64_t magic = 0x534e535452414345; // TODO: Reverse?
		uint32_t version = 0;
		uint8_t content_guid[8];
		enum RomMode : uint8_t {
			ROMMODE_UNKNOWN,
			ROMMODE_LOROM,
			ROMMODE_HIROM,
		};
		uint32_t rom_size = 0;
		RomMode rom_mode = ROMMODE_UNKNOWN;
		uint32_t rom_checksum = 0;
	};

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

	struct HelperOp {
		// One way to make this one smaller is to exclude PC from before and to have a mask saying what changes... And only state after for changed
		uint64_t current_op;
		TraceRegisters registers_before;
		TraceRegisters registers_after;
	};
	#pragma pack(pop)
}
