#pragma once

#include <cstdint>
#include "utils.h"
#include <map>
#include <vector>
#include <set>

struct Options;
namespace snestistics {

class RomAccessor;

static const uint32_t TRACE_CACHE_VERSION = 4;

/*
	The Trace is where information about the entire run is captured from an emulation replay.
	It can be used to generate the assembly file for the code in the ROM.
*/

struct OpInfo {
	Pointer indirect_base_pointer;
	Pointer jump_target; // If this was a deliberate jump/branch that was taken
	uint16_t P; // status
	uint16_t DP; // direct page
	uint16_t X, Y;
	uint8_t DB; // data bank
	inline bool operator<(const OpInfo &other) const {
		if (DB != other.DB) return DB < other.DB;
		if (DP != other.DP) return DP < other.DP;
		if (indirect_base_pointer != other.indirect_base_pointer) return indirect_base_pointer < other.indirect_base_pointer;
		if (X != other.X) return X < other.X;
		if (Y != other.Y) return Y < other.Y;
		if (P != other.P) return P < other.P;
		if (jump_target != other.jump_target) return jump_target < other.jump_target;
		return false;
	}
	inline bool operator==(const OpInfo &other) const {
		if (DB != other.DB) return false;
		if (DP != other.DP) return false;
		if (indirect_base_pointer != other.indirect_base_pointer) return false;
		if (X != other.X) return false;
		if (Y != other.Y) return false;
		if (P != other.P) return false;
		if (jump_target != other.jump_target) return false;
		return true;
	}
	inline bool operator!=(const OpInfo &other) const {
		return !(*this == other);
	}
};

// TODO: Consider moving
struct DmaTransfer {
	uint32_t pc;
	uint16_t a_address;
	uint16_t transfer_bytes;
	uint8_t	transfer_mode;
	uint8_t	b_address;
	uint8_t	a_bank;
	uint8_t channel;
	enum Flags {
		REVERSE_TRANSFER=1,
		A_ADDRESS_FIXED=2,
		A_ADDRESS_DECREMENT=4,
	};
	uint8_t flags;

	bool operator<(const DmaTransfer &o) const {
		if (pc != o.pc) return pc<o.pc;
		if (channel != o.channel) return channel<o.channel;
		if (a_address != o.a_address) return a_address<o.a_address;
		if (transfer_bytes != o.transfer_bytes) return transfer_bytes<o.transfer_bytes;
		if (transfer_mode != o.transfer_mode) return transfer_mode<o.transfer_mode;
		if (a_bank != o.a_bank) return a_bank<o.a_bank;
		if (flags != o.flags) return flags < o.flags;
		return false;
	}
};

struct Trace {
	Trace() : labels(256*64*1024), is_predicted(256*64*1024) {} 

	struct OpVariantLookup {
		uint32_t offset, count;
	};

	// TODO: Remake this map into two vectors, one with PC (for binary search) and one for offset/count
	std::map<Pointer, OpVariantLookup> ops;
	const OpInfo &variant(const OpVariantLookup &vl, const int idx) const {
		return ops_variants[vl.offset+idx];
	}
	std::vector<OpInfo> ops_variants;
	LargeBitfield labels;

	struct MemoryAccess { 
		Pointer adress, pc; 
		bool operator<(const MemoryAccess &o) const {
			if (adress != o.adress) return adress < o.adress;
			return pc < o.pc;
		}
		bool operator==(const MemoryAccess &o) const {
			if (pc != o.pc) return false;
			return adress == o.adress;
		}
	};
	std::vector<MemoryAccess> memory_accesses;
	std::vector<DmaTransfer> dma_transfers;	

	// Not serialized
	LargeBitfield is_predicted;
};

void create_trace(const Options &options, const int trace_file_index, const RomAccessor &rom_accessor, Trace &trace);
void merge_trace(Trace &dest, const Trace &add);

// Since emulation takes time we can save/load traces (caching)
bool load_trace_cache(const std::string &trace_file, Trace &trace);
}
