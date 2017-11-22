#include "trace.h"
#include "options.h"
#include "emulate.h"
#include "replay.h"
#include "romaccessor.h"
#include <algorithm>
#include <memory>

namespace {

	template <class T>
	inline void hash_combine(std::size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	struct OpRecord {
		uint32_t PC;
		OpInfo op_info;

		inline bool operator<(const OpRecord &other) const {
			if (PC != other.PC) return PC < other.PC;
			return op_info < other.op_info;
		}
		inline bool operator==(const OpRecord &other) const {
			if (PC != other.PC) return false;
			return op_info == other.op_info;
		}
	};
}

namespace std {
	template <> struct hash<OpRecord>
	{
		size_t operator()(const OpRecord & x) const
		{
			size_t h = 0;
			hash_combine(h, x.PC);
			hash_combine(h, x.op_info.DB);
			hash_combine(h, x.op_info.DP);
			hash_combine(h, x.op_info.indirect_base_pointer);
			hash_combine(h, x.op_info.jump_target);
			hash_combine(h, x.op_info.P);
			hash_combine(h, x.op_info.X);
			hash_combine(h, x.op_info.Y);
			return h;
		}
	};
	template <> struct hash<Trace::MemoryAccess>
	{
		size_t operator()(const Trace::MemoryAccess & x) const
		{
			size_t h = 0;
			hash_combine(h, x.adress);
			hash_combine(h, x.pc);
			return h;
		}
	};
}

namespace {
struct CallbackContext {
	Pointer current_pc = 0;
	std::set<Trace::MemoryAccess> accesses;
	std::set<DmaTransfer> dma_transfers;
};

void read_function(void* context, Pointer location, Pointer remapped_location, uint32_t value, int num_bytes, MemoryAccessType reason) {
	if (reason != MemoryAccessType::RANDOM && reason != MemoryAccessType::FETCH_INDIRECT)
		return;
	CallbackContext &m = *(CallbackContext*)context;
	Trace::MemoryAccess a;
	for (int k = 0; k < num_bytes; ++k) {
		a.adress = remapped_location + k;
		a.pc = m.current_pc;
		m.accesses.insert(a);
	}
}

void write_function(void* context, Pointer location, Pointer remapped_location, uint32_t value, int num_bytes, MemoryAccessType reason) {
	if (reason != MemoryAccessType::RANDOM && reason != MemoryAccessType::FETCH_INDIRECT)
		return;
	CallbackContext &m = *(CallbackContext*)context;
	Trace::MemoryAccess a;
	for (int k = 0; k < num_bytes; ++k) {
		a.adress = remapped_location + k;
		a.pc = m.current_pc | 0x80000000; // Indicates write
		m.accesses.insert(a);
	}
}

void dma_function(void *context, const DmaTransfer &dma) {
	CallbackContext &m = *(CallbackContext*)context;
	m.dma_transfers.insert(dma);
}

// Now count variants for each PC and put them all in a big vector with an index
// Now we iterate op_trace in order sorted by PC
void pack_ops(Trace &trace, std::set<OpRecord> &op_trace) {
	trace.ops.clear();

	const uint32_t number_of_variants = (uint32_t)op_trace.size();
	trace.ops_variants.resize(number_of_variants);

	Profile profile("Sorting trace entries", true);
	Pointer current_pc = op_trace.begin()->PC; // Set current_pc to the first one
	int count = 0;
	int offset = 0;

	for (auto it : op_trace) {
		if (it.PC != current_pc) {
			// Time to make a record
			Trace::OpVariantLookup lookup;
			lookup.count = count;
			lookup.offset = offset - count;
			trace.ops[current_pc] = lookup;
			// Get read for next PC
			count = 0;
			current_pc = it.PC;
		}
		trace.ops_variants[offset++] = it.op_info;
		count++;
	}
	{
		Trace::OpVariantLookup lookup;
		lookup.count = count;
		lookup.offset = offset - count;
		trace.ops[current_pc] = lookup;
	}
}
}

void create_trace(const Options &options, const int trace_file_index, const RomAccessor &rom_accessor, Trace &trace) {
	FILE *skip_cache = nullptr;
	const uint32_t nmi_per_skip = 10;

	skip_cache = fopen(options.trace_file_skip_cache(trace_file_index).c_str(), "wb");
	snestistics::TraceSkipHeader skip_header;
	skip_header.version = CURRENT_CACHE_VERSION;
	skip_header.nmi_per_skip = nmi_per_skip;
	fwrite(&skip_header, sizeof(skip_header), 1, skip_cache);

	// Fast structure used during emulation
	std::set<OpRecord> op_trace;
	CallbackContext memory_accesses;

	Replay replay(rom_accessor, options.trace_files[trace_file_index].c_str());

	EmulateRegisters &regs = replay.regs;
	regs._read_function = read_function;
	regs._write_function = write_function;
	regs._dma_function = dma_function;
	regs._callback_context = &memory_accesses;

	uint32_t last_reported_nmi = 0;

	{
		Profile profile("Emulation", true);
		int nmi = 0;

		while (true) {
			const uint32_t pc_before = regs._PC;
			memory_accesses.current_pc = pc_before;

			// We arrived at this op with some registers... Remember the ones we care about
			const uint16_t X_before = regs._X, Y_before = regs._Y, DP_before = regs._DP, P_before = regs._P;
			const uint8_t DB_before = regs._DB;

			if (!replay.next())
				break;

			if (nmi != last_reported_nmi && (nmi%100)==0) {
				printf("%d nmi emulated\n", nmi);
				last_reported_nmi = nmi; 
			}

			if (skip_cache && (regs.event == Events::RESET || regs.event == Events::NMI) && (nmi % nmi_per_skip)==0) {
				snestistics::TraceSkip msg;
				msg.nmi = nmi;
				msg.regs.A = regs._A;
				msg.regs.X = regs._X;
				msg.regs.Y = regs._Y;
				msg.regs.S = regs._S;
				msg.regs.DB = regs._DB;
				msg.regs.DP = regs._DP;
				msg.regs.pc_bank = regs._PC >> 16;
				msg.regs.pc_address = regs._PC & 0xFFFF;
				msg.regs.P = regs._P;
				msg.regs.wram_bank = regs._WRAM >> 16;
				msg.regs.wram_address = regs._WRAM & 0xFFFF;
				msg.seek_offset_trace_file = replay._last_after_nmi_offset;
				msg.current_op = replay._current_op;
				fwrite(&msg, sizeof(msg), 1, skip_cache);
				fwrite(&regs._memory[0x7E0000], 64*1024, 1, skip_cache);
				fwrite(&regs._memory[0x7F0000], 64*1024, 1, skip_cache);
			}

			const uint32_t jump_pc = regs._PC;

			bool is_jump = false; // Did the event/op cause a discontinous program counter?
			bool is_return = false;
			bool op = false; // Did we execute an op here? Anything but reset, nmi, irq or

			switch(regs.event) {
			case Events::RESET:
			case Events::NMI:
			case Events::IRQ:
				// Do not register this as a jump; nobody cares where we jumped from to an nmi/irq/reset
				trace.labels.setBit(jump_pc);
				break;
			case Events::RTI:
			case Events::RTS_OR_RTL:
				is_return = true;
				op = true;
				break;
			case Events::JMP_OR_JML:
			case Events::JSR_OR_JSL:
			case Events::BRANCH:
				// This means that the op _took_ a jump, not that it was a jump instruction
				trace.labels.setBit(jump_pc);
				is_jump = true;
			case Events::NONE:
				op = true;
			};

			if(regs.event == Events::NMI || regs.event == Events::RESET) {
				nmi++;
			}

			if (op) {
				// TODO: Set all to zero if not touched by next()
				OpRecord o;
				memset(&o, 0, sizeof(OpRecord)); // Make sure padding is zero since we serialize cache
				o.PC = pc_before;
				o.op_info.DB = regs.used_DB ? DB_before : 0;
				o.op_info.DP = regs.used_DP ? DP_before : 0;
				o.op_info.P = P_before & (0x10|0x20|0x100); // Index, memory, emulation
				o.op_info.X = X_before & regs.used_X_mask;
				o.op_info.Y = Y_before & regs.used_Y_mask;
				o.op_info.jump_target = (is_jump||is_return) ? jump_pc : INVALID_POINTER;
				o.op_info.indirect_base_pointer = regs.indirection_pointer;
				op_trace.insert(o);
			}
		}
	}

	pack_ops(trace, op_trace);

	// Put all DMA transfers in order
	trace.dma_transfers.reserve(memory_accesses.dma_transfers.size());
	for (auto it : memory_accesses.dma_transfers) {
		trace.dma_transfers.push_back(it);
	}

	// Put all memory accesses in order
	trace.memory_accesses.reserve(memory_accesses.accesses.size());
	for (auto it : memory_accesses.accesses) {
		trace.memory_accesses.push_back(it);
	}

	if (skip_cache)
		fclose(skip_cache);
}

void save_trace(const Trace &trace, const std::string &filename) {
	Profile profile("Saving trace cache", true);

	FILE *dest = fopen(filename.c_str(), "wb");

	const uint32_t version = CURRENT_CACHE_VERSION;
	fwrite(&version, sizeof(uint32_t), 1, dest);

	const uint32_t num_ops = (uint32_t)trace.ops.size();
	const uint32_t num_variants = (uint32_t)trace.ops_variants.size();

	// Ops lookups (one for each PC)
	fwrite(&num_ops, sizeof(uint32_t), 1, dest);
	for (auto k : trace.ops) {
		fwrite(&k.first, sizeof(uint32_t), 1, dest); // PC
		fwrite(&k.second.offset, sizeof(uint32_t), 1, dest); // offset
		fwrite(&k.second.count, sizeof(uint32_t), 1, dest); // count
	}

	// Ops variants (multiple for each PC)
	fwrite(&num_variants, sizeof(uint32_t), 1, dest);
	fwrite(&trace.ops_variants[0], sizeof(OpInfo), num_variants, dest);

	// Write labels
	trace.labels.write_file(dest);

	// Write memory accesses
	const uint32_t num_memory_accesses = (uint32_t)trace.memory_accesses.size();
	fwrite(&num_memory_accesses, sizeof(uint32_t), 1, dest);
	if (num_memory_accesses!=0) {
		fwrite(&trace.memory_accesses[0], sizeof(Trace::MemoryAccess), num_memory_accesses, dest);
	}

	// Write DMA-transfers
	const uint32_t num_dma_transfers = (uint32_t)trace.dma_transfers.size();
	fwrite(&num_dma_transfers, sizeof(uint32_t), 1, dest);
	if (num_dma_transfers!=0) {
		fwrite(&trace.dma_transfers[0], sizeof(DmaTransfer), num_dma_transfers, dest);
	}

	fclose(dest);
}

bool load_trace(const std::string &filename, Trace &trace) {
	Profile profile("Loading trace cache", true);

	FILE *source = fopen(filename.c_str(), "rb");
	if (!source)
		return false;
	
	uint32_t version = 0;
	fread(&version, sizeof(uint32_t), 1, source);
	if (version != CURRENT_CACHE_VERSION)
		return false;

	// Ops
	uint32_t num_ops = 0;
	fread(&num_ops, sizeof(uint32_t), 1, source);
	for (uint32_t i = 0; i < num_ops; ++i) {
		uint32_t pc = 0;
		fread(&pc, sizeof(uint32_t), 1, source);
		Trace::OpVariantLookup a;
		fread(&a.offset, sizeof(uint32_t), 1, source);
		fread(&a.count, sizeof(uint32_t), 1, source);
		trace.ops[pc] = a;
	}

	uint32_t num_variants = 0;
	fread(&num_variants, sizeof(uint32_t), 1, source);

	trace.ops_variants.resize(num_variants);
	if (num_variants!=0) {
		fread(&trace.ops_variants[0], sizeof(OpInfo), num_variants, source);
	}

	// Labels
	trace.labels.read_file(source);

	// Read memory accesses
	uint32_t num_memory_accesses = 0;
	fread(&num_memory_accesses, sizeof(uint32_t), 1, source);
	trace.memory_accesses.resize(num_memory_accesses);
	if (num_memory_accesses!=0) {
		fread(&trace.memory_accesses[0], sizeof(Trace::MemoryAccess), num_memory_accesses, source);
	}

	// Read DMA-transfers
	uint32_t num_dma_transfers = 0;
	fread(&num_dma_transfers, sizeof(uint32_t), 1, source);
	trace.dma_transfers.resize(num_dma_transfers);
	if (num_dma_transfers!=0) {
		fread(&trace.dma_transfers[0], sizeof(DmaTransfer), num_dma_transfers, source);
	}

	fclose(source);
	return true;
}

// This function is stupid!!!
template<typename T>
void merge_unique(std::vector<T> &dest, const std::vector<T> &add) {
	std::vector<T> temp;
	temp.reserve(dest.size() + add.size());
	std::set_union(dest.begin(), dest.end(), add.begin(), add.end(), std::back_inserter(temp));
	std::swap(dest, temp);	
}

void merge_trace(Trace &dest, const Trace &add) {
	merge_unique(dest.dma_transfers, add.dma_transfers);
	merge_unique(dest.memory_accesses, add.memory_accesses);
	dest.labels.set_union(add.labels);

	// OK the hard one! We cheat by doing it slowly
	std::set<OpRecord> op_trace;
	for (auto op : dest.ops) {
		OpRecord r;
		r.PC = op.first;
		for (uint32_t k=0; k<op.second.count; k++) {
			r.op_info = dest.variant(op.second, k);
			op_trace.insert(r);
		}
	}
	for (auto op : add.ops) {
		OpRecord r;
		r.PC = op.first;
		for (uint32_t k=0; k<op.second.count; k++) {
			r.op_info = add.variant(op.second, k);
			op_trace.insert(r);
		}
	}
	pack_ops(dest, op_trace);
}
