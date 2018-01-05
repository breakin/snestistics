#include <stdint.h>
#include <memory>
#include <set>
#include <vector>
#include <string>
#include <cassert>
#include <fstream>
#include <map>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include "trace_format.h"
#include "options.h"
#include "utils.h"
#include "romaccessor.h"
#include "asm_writer.h"
#include "annotations.h"
#include "cputable.h"
#include "trace_log.h"
#include "trace.h"
#include "rewind.h"
#include "scripting.h"
#include "report_writer.h"

using namespace snestistics;

// Jumps, branches but not JSLs
// If secondary target is set, it is always set to the next op after this op
bool decode_static_jump(uint8_t opcode, const RomAccessor &rom, const Pointer pc, Pointer *target, Pointer *secondary_target) {
	*target = INVALID_POINTER;
	*secondary_target = INVALID_POINTER;
	if (opcode == 0x82) { // BRL
		uint16_t v = *(uint16_t*)rom.evalPtr(pc+1);
		Pointer t = pc;
		t += v;
		if (v >= 0x8000)
			t -= 0x10000;
		*target = t;
	} else if (branches[opcode]) {
		const Pointer target1 = pc + (unpackSigned(rom.evalByte(pc+1)) + 2);
		*target = pc + (unpackSigned(rom.evalByte(pc+1)) + 2);
		if (opcode != 0x80) {
			*secondary_target = pc + 2;
		}
	} else if (opcode == 0x4C) { // Absolute jump
		Pointer p = *(uint16_t*)rom.evalPtr(pc+1);
		p|=pc&0xFF0000;
		*target = p;
	} else if (opcode == 0x5C) { // Absolute long jump
		Pointer p = 0;
		p |= rom.evalByte(pc+1);
		p |= rom.evalByte(pc+2)<<8;
		p |= rom.evalByte(pc+3)<<16;
		*target = p;
	} else if (opcode == 0x6C || opcode == 0x7C || opcode == 0xDC) {
		// All of these are indeterminate, leave as INVALID_POINTER
	} else {
		return false;
	}
	return true;
}

bool guess_valid_jump(Pointer pc, Pointer target, int max_distance = 1000) {
	if ((pc>>16)!=(target>>16))
		return false;
	return true;
	int distance = abs((int32_t)(target&0xFFFF)-(int32_t)(pc&0xFFFF));
	return distance < max_distance;
}

void guess_range(const Trace &trace, const RomAccessor &rom, const AnnotationResolver &annotations, std::string &output_file) {

	FILE *output = fopen(output_file.c_str(), "wt");

	int unknownCounter = 0;

	struct FoundRange {
		Pointer start = INVALID_POINTER, stop = INVALID_POINTER;
		bool ends_with_rti = false;
		bool valid = true;

		bool inside(Pointer p) const { return p>=start && p<=stop; }

		int num_long_jumps = 0;

		std::set<Pointer> branches_out;
	};

	// TODO: Support trace annotation jump is jsr
	std::vector<FoundRange> found_ranges;

	for (auto it = trace.ops.begin(); it != trace.ops.end(); ++it) {
		FoundRange found;
		while (it != trace.ops.end()) {
			Pointer pc = it->first;

			//if (pc < 0x80801C || pc > 0x80E582)
			//	break;

			//if (pc > 0x00A0F6) // TODO: Remove
			//	break;

			const Annotation *function = nullptr;
			annotations.resolve_annotation(pc, &function);

			// We only want unknown but executed stuff!
			if (function)
				break;

			uint8_t opcode = rom.evalByte(pc);

			Pointer jump_target, jump_secondary_target;
			bool op_is_jump_or_branch = decode_static_jump(opcode, rom, pc, &jump_target, &jump_secondary_target);

			const TraceAnnotation *ta = annotations.trace_annotation(pc);
			bool jump_is_jsr = false;
			if (ta && ta->type == TraceAnnotation::JMP_IS_JSR)
				jump_is_jsr = true;

			if (op_is_jump_or_branch && jump_target != INVALID_POINTER && guess_valid_jump(pc, jump_target)) {
				found.branches_out.insert(jump_target);
			}
			
			// If this was a jump, deterministic or not, loop over all variants
			if (op_is_jump_or_branch && !jump_is_jsr) {
				auto lit = trace.ops.find(pc);
				if (lit != trace.ops.end()) {
					const Trace::OpVariantLookup &lut = lit->second;
					for (uint32_t i=0; i<lut.count; i++) {
						const OpInfo &o = trace.variant(lut, i);
						if (o.jump_target != INVALID_POINTER && guess_valid_jump(pc, o.jump_target)) {
							found.branches_out.insert(o.jump_target);
						}
					}
				}
			}

			if (found.start == INVALID_POINTER)
				found.start = pc;

			// Stop if we find a branch or a jump
			if (op_is_jump_or_branch && jump_secondary_target == INVALID_POINTER) {
				// If there is a secondary jump target (as in a branch) we don't need to stop
				found.stop = pc;
				found_ranges.push_back(found);
				break;
			}

			bool is_return = opcode == 0x40 || opcode == 0x6B || opcode == 0x60;

			if (is_return) {
				found.stop = pc;
				found.ends_with_rti = true;
				found_ranges.push_back(found);
				break;
			}

			it++;
		}
	}

	std::vector<uint32_t> merged_with(found_ranges.size(), -1);

	for (uint32_t j=0; j<=found_ranges.size(); ++j) { // TODO: Make <= be just <

		bool debug = false;

		if (debug) {
			printf("Step %d\n", j);

			for (uint32_t ai = 0; ai < found_ranges.size(); ai++) {
				const FoundRange &f = found_ranges[ai];
				printf(" %d: %06X-%06X%s", ai, f.start, f.stop, f.valid?"":" (invalid)");
				if (merged_with[ai]!=-1)
					printf(" (merged with %d)", merged_with[ai]);
				printf("\n");

				if (ai >=j)
				for (auto b : f.branches_out) {
					if (f.inside(b))
						continue;
					printf("   -> %06X", b);
					for (uint32_t bi = 0; bi < found_ranges.size(); ++bi) {
						const FoundRange &fb = found_ranges[bi];
						if (fb.start <= b && b <= fb.stop)
							printf(" %d", bi);
					}
					printf("\n");
				}
			}
		}

		// We went one extra round for debugging!
		if (j==found_ranges.size())
			break;

		uint32_t merge_target = j;
		while (merged_with[merge_target] != -1)
			merge_target = merged_with[merge_target];

		auto &branches_out = found_ranges[j].branches_out;

		FoundRange &fj = found_ranges[j];
		
		//printf("Doing %d, saving into %d\n", j, merge_target);
		for (Pointer target : branches_out) {
			// We have three cases; forward, backward or internal branches. We treat them separately for clarity
			if (fj.inside(target)) // Ignore internal branches/jumps
				continue;

			// TODO: This is not enough.. We must stop at any function in the range...
			//       Basically cut target at any function def...
			const Annotation *target_function = nullptr;
			annotations.resolve_annotation(target, &target_function);
			if (target_function)
				continue;

			int len = 0;
			if (target < fj.start) {
				len = fj.start - target;
			} else {
				len = target - fj.stop;
			}
			if (len > 1000) {
				fj.num_long_jumps++;
			}

			Pointer t1 = target;

			if (target < fj.start) {
				target = annotations.find_last_free_before_or_at(fj.start, target);
				if (target != t1) {
					int A=9;
				}

				if (target == INVALID_POINTER)
					continue;
				// Branch going out to smaller addresses (before j)

				int lowest_merge = j;

				for (int32_t t=j-1; t>=0; --t) {
					FoundRange &merger = found_ranges[t];
					if (target > merger.stop) // Since ranges are sorted on start and non-overlapping we can stop here
						break;
					lowest_merge = t;
				}

				if (lowest_merge == j)
					continue;

				while (merged_with[lowest_merge] != -1)
					lowest_merge = merged_with[lowest_merge];

				// We want to merge the range (lowest_range, ..., j) into lowest_range
				for (uint32_t t=lowest_merge+1; t <= j; t++) {
					FoundRange &f = found_ranges[t];
					if (f.valid) {
						f.valid = false;
						merged_with[t] = lowest_merge;
					}
				}

				found_ranges[lowest_merge].stop = fj.stop;

			} else if (target > fj.stop) {

				target = annotations.find_last_free_after_or_at(fj.stop, target);
				if (target != t1) {
					int A=9;
				}

				if (target == INVALID_POINTER)
					continue;

				// Branch going out to larger addresses (after j)
				for (uint32_t t=j+1; t<found_ranges.size(); ++t) {
					FoundRange &merger = found_ranges[t];
					if (target < merger.start) // Since ranges are sorted on start and non-overlapping we can stop here
						break;
					if (merger.valid) {
						// Merge t into j (the easy case)
						fj.stop = merger.stop;
						merger.valid = false; // Since we make it invalid it can only be merged once...
						merged_with[t] = merge_target;
					}
				}
			}
		}
	}

	int num_found = 0;

	int longest = -1, longest_range = -1;
	uint32_t longest_length = 0;

	for (uint32_t fi = 0; fi < found_ranges.size(); ++fi) {
		const FoundRange &f = found_ranges[fi];
		if (f.num_long_jumps > 4) {
			printf("Long jumps to %06X-%06X (%d)\n", f.start, f.stop, f.num_long_jumps);
		}

		if (f.valid) {
			bool collision = false;
			for (auto k : annotations._annotations) {
				if (k.type == AnnotationType::ANNOTATION_FUNCTION) {
					if (f.stop < k.startOfRange || f.stop > k.endOfRange) {
						continue;
					} else {
						collision = true;
						break;
					}
				}
			}
			if (collision) {
				printf("Collision!\n");
			} else {
				// TODO: Validate that there is no function annotation inside a range
				if (f.stop - f.start > longest_length) {
					longest = num_found;
					longest_length = f.stop - f.start + 1;
					longest_range = fi;
				}
				num_found++;
				fprintf(output, "function %06X %06X Auto%04d\n", f.start, f.stop, unknownCounter++);
			}
		}
	}

	printf("Found %d functions (longest Auto%04d, len=%d, range=%d)\n", num_found, longest, longest_length, longest_range);

	fclose(output);
}

void dma_report(ReportWriter &writer, const Trace &trace, const AnnotationResolver &annotations) {
	Profile profile("DMA report", true);
	writer.writeSeperator("DMA analysis");
	writer.writeComment("");
	writer.writeComment("Currently not sure if we can deduce step or not!");
	writer.writeComment("Currently not doing anything with data annotations! Let me know what I could do!");
	writer.writeComment("HDMA is not supported. Can be added in.");
	writer.writeComment("");

	StringBuilder sb;

	const Annotation* reported_function = nullptr;
	Pointer reported_pc = -1;

	for (const DmaTransfer &d : trace.dma_transfers) {

		bool print_pc = false;

		if (reported_pc != d.pc) {
			const Annotation *from;
			annotations.resolve_annotation(d.pc, &from);

			if (reported_function != from) {
				reported_function = from;

				if (from) {
					sb.format("%s", from->name.c_str());
				} else {
					sb.format("(unannotated)", from->name.c_str());
				}
			}

			print_pc = true;
			reported_pc = d.pc;
		}

		sb.column(20); sb.format("%d", d.channel);
		sb.column(24); sb.format("$%05X $%02X:$%04X %s $21%02X (mode=$%02X)%s%s", d.transfer_bytes == 0 ? 0x10000 : d.transfer_bytes, d.a_bank, d.a_address, d.flags & DmaTransfer::REVERSE_TRANSFER ? "<-":"->", d.b_address, d.transfer_mode, d.flags & DmaTransfer::A_ADDRESS_DECREMENT ?" (dec)":" (inc)", d.flags & DmaTransfer::A_ADDRESS_FIXED ? " (fixed)":"");

		if (print_pc) {
			sb.column(86);  sb.format("at pc=$%06X", d.pc);
		}
		writer.writeComment(sb);
	}
}

void data_report(ReportWriter &writer, const Trace &trace, const AnnotationResolver &annotations) {
	StringBuilder sb;

	Profile profile("Data report", true);
	writer.writeSeperator("Data analysis");

	sb.clear();
	sb.format("DATA RANGE");
	sb.column(20);
	sb.format("READERS");
	sb.column(60);
	sb.format("WRITERS");
	sb.column(90);
	sb.format("ANNOTATIONS");
	writer.writeSeperator(sb.c_str());

	Pointer first_global = INVALID_POINTER, last_global = INVALID_POINTER;

	std::set<const Annotation*> global_readers;
	std::set<uint32_t> global_unknown_readers;
	std::set<const Annotation*> global_writers;
	std::set<uint32_t> global_unknown_writers;
	const Annotation *global_data = nullptr;

	for (uint32_t p = 0; p < trace.memory_accesses.size(); ) {
		const Trace::MemoryAccess ra = trace.memory_accesses[p];
		Pointer adress = ra.adress;

		const Annotation *data = nullptr;

		// Don't care about accesses to code
		// TODO: Check so we don't write code?
		{
			const Annotation *accessed_function = nullptr;
			annotations.resolve_annotation(adress, &accessed_function, &data);
			if (accessed_function != nullptr) {
				for (uint32_t j=p; j<trace.memory_accesses.size(); j++, p++) {
					const Trace::MemoryAccess rb = trace.memory_accesses[j];
					if (adress != rb.adress) break;
				}
				continue;
			}
		}

		bool need_flush = false;
		std::set<const Annotation*> new_readers;
		std::set<uint32_t> new_unknown_readers;
		std::set<const Annotation*> new_writers;
		std::set<uint32_t> new_unknown_writers;

		for (uint32_t j=p; j<trace.memory_accesses.size(); j++, p++) {
			const Trace::MemoryAccess rb = trace.memory_accesses[j];
			if (adress != rb.adress) break;

			const bool is_write = (rb.pc & 0x80000000)!=0;
			const Pointer pc = is_write ? rb.pc & ~0x80000000 : rb.pc;

			const Annotation *our_function = nullptr;
			annotations.resolve_annotation(pc, &our_function);

			if (our_function) {
				if (is_write) {
					new_writers.insert(our_function);
					if (!need_flush && global_writers.find(our_function) == global_writers.end()) need_flush = true;
				} else {
					new_readers.insert(our_function);
					if (!need_flush && global_readers.find(our_function) == global_readers.end()) need_flush = true;
				}
			} else {
				if (is_write) {
					new_unknown_writers.insert(pc);
					if (!need_flush && new_unknown_writers.find(pc) == new_unknown_writers.end()) need_flush = true;
				} else {
					new_unknown_readers.insert(pc);
					if (!need_flush && global_unknown_readers.find(pc) == global_unknown_readers.end()) need_flush = true;
				}
			}
		}

		if (data != global_data)
			need_flush = true;

		// We know that we aren't adding any new readers/writers, but did we produce the same full list?
		if (first_global != INVALID_POINTER) {
			if ((new_readers.size() != global_readers.size()) || (global_unknown_readers.size() != new_unknown_readers.size())) {
				need_flush = true;
			}
			if ((new_writers.size() != global_writers.size()) || (global_unknown_writers.size() != new_unknown_writers.size())) {
				need_flush = true;
			}
		}

		bool update_globals = false;
		if (need_flush || first_global == INVALID_POINTER) {
			update_globals = true;
		}

		struct AccessIterator {
			AccessIterator(const std::set<const Annotation*> &known, const std::set<uint32_t> &unknown) : _known(known), _unknown(unknown) {}
			const std::set<const Annotation*> &_known;
			const std::set<uint32_t> &_unknown;

			uint32_t _current = 0;
			bool has() {
				const uint32_t total = (uint32_t)_known.size() + (uint32_t)_unknown.size();
				return _current < total;
			}
			void advance() {
				_current++;
			}
			void emit(StringBuilder &sb) {
				if (_current < _known.size()) {
					auto it = _known.begin();
					for (uint32_t i=0; i<_current; i++, it++);
					const Annotation *a = *it;
					sb.format("%s", a->name.c_str());
				} else {
					uint32_t lc = _current - (uint32_t)_known.size();
					auto it = _unknown.begin();
					for (uint32_t i=0; i<lc; i++, it++);
					const uint32_t pc = *it;
					sb.format("%06X", pc);
				}
			}
		};

		AccessIterator readers(global_readers, global_unknown_readers);
		AccessIterator writers(global_writers, global_unknown_writers);

		// If last range was finished, print it
		if (need_flush) {
			bool first_line = true;
			while (readers.has() || writers.has()) {
				sb.clear();
				if (first_line) {
					if (first_global != last_global)
						sb.format("%06X-%06X", first_global, last_global);
					else
						sb.format("%06X", first_global);
				}
				if (readers.has()) {
					sb.column(20);
					readers.emit(sb);
				}
				if (writers.has()) {
					sb.column(60);
					writers.emit(sb);
				}
				if (first_line) {
					if (global_data) {
						sb.column(90);
						sb.format("%s [%06X-%06X]", global_data->name.c_str(), global_data->startOfRange, global_data->endOfRange);
					}
				}
				writer.writeComment(sb);
				first_line = false;
				readers.advance();
				writers.advance();
			}
			first_global = last_global = adress;
		} else {
			if (first_global == INVALID_POINTER) first_global = adress;
			last_global = adress;
		}
		
		if (update_globals) {
			std::swap(global_readers, new_readers);
			std::swap(global_writers, new_writers);
			std::swap(global_unknown_readers, new_unknown_readers);
			std::swap(global_unknown_writers, new_unknown_writers);
			global_data = data;
		}
	}

	// TODO: Print last range
}

void predict(Options::PredictMode mode, ReportWriter *writer, const RomAccessor &rom, Trace &trace, const AnnotationResolver &annotations) {

	if (mode == Options::PREDICT_NOTHING)
		return;

	bool limit_to_functions = mode == Options::PREDICT_FUNCTIONS;

	Profile profile("Predict", true);

	struct PredictBranch {
		const Annotation *annotation;
		Pointer pc;
		uint16_t DP, P;
		uint8_t DB;
	};

	std::vector<PredictBranch> predict_brances;
	LargeBitfield has_op(256*64*1024);

	for (auto opsit : trace.ops) {
		const Pointer pc = opsit.first;
		has_op.setBit(pc);

		const Trace::OpVariantLookup &vl = opsit.second;
		const uint8_t* data = rom.evalPtr(pc);

		const uint8_t opcode = data[0];

		StringBuilder sb;

		Pointer target_jump, target_no_jump;
		bool branch_or_jump = decode_static_jump(opcode, rom, pc, &target_jump, &target_no_jump);

		if (!branch_or_jump || target_jump == INVALID_POINTER)
			continue;

		trace.labels.setBit(target_jump);

		const Annotation *source_annotation = nullptr, *target_annotation = nullptr;
		annotations.resolve_annotation(pc,          &source_annotation);
		annotations.resolve_annotation(target_jump, &target_annotation);

		if (source_annotation || !limit_to_functions) {
			// We only predict branches going within an annotated function
			PredictBranch p;
			p.annotation = source_annotation;
			p.DB = trace.variant(vl, 0).DB;
			p.DP = trace.variant(vl, 0).DP;
			p.P  = trace.variant(vl, 0).P;
			if (target_annotation == source_annotation || !limit_to_functions) {
				p.pc = target_jump;
				CUSTOM_ASSERT(target_jump != INVALID_POINTER);
				predict_brances.push_back(p);
			}
			if (target_no_jump != INVALID_POINTER && (!limit_to_functions || (target_no_jump >= source_annotation->startOfRange && target_no_jump <= source_annotation->endOfRange))) {
				// BRA,BRL and the jumps always branches/jumps
				p.pc = target_no_jump;
				CUSTOM_ASSERT(target_no_jump != INVALID_POINTER);
				predict_brances.push_back(p);
			}
		}
	}

	StringBuilder sb;
	sb.clear();

	if (writer)
		writer->writeSeperator("Prediction diagnostics");

	for (int pbi=0; pbi<(int)predict_brances.size(); ++pbi) {
		const PredictBranch pb = predict_brances[pbi];

		Pointer pc = pb.pc;
		Pointer r0 = limit_to_functions ? pb.annotation->startOfRange : 0, r1 = limit_to_functions ? pb.annotation->endOfRange : 0xFFFFFF;
		uint16_t P = pb.P, DP = pb.DP;
		uint8_t DB = pb.DB;

		bool P_unknown = false;

		while (!has_op[pc] && pc >= r0 && pc <= r1) {

			// Make sure we don't run into a data scope
			const Annotation *data_scope = nullptr;
			annotations.resolve_annotation(pc, nullptr, &data_scope);
			if (data_scope)
				continue;

			uint8_t opcode = rom.evalByte(pc);

			bool abort_unknown_P = P_unknown;

			if (P_unknown) {
				switch(opcode) {
				case 0x02: // COP const
				case 0x40: // RTI
				case 0x6B: // RTL
				case 0x60: // RTS
				case 0x3B: // TSC
				case 0xBA: // TSX
				case 0x8A: // TXA
				case 0x9A: // TXS
				case 0x9B: // TXY
				case 0x98: // TYA
				case 0xBB: // TYX
				case 0xCB: // WAI
				case 0xEB: // XBA
				case 0xFB: // XCE
				case 0xAA: // TAX
				case 0xA8: // TAY
				case 0x5B: // TCD
				case 0x1B: // TCS
				case 0x7B: // TDC
				case 0xDB: // STP
				case 0x38: // SEC
				case 0xF8: // SED
				case 0x78: // SEI
				case 0xE2: // SEP
				case 0xC2: // REP
				case 0x18: // CLC
				case 0xD8: // CLD
				case 0x58: // CLI
				case 0xB8: // CLV
				case 0xCA: // DEX
				case 0x88: // DEY
				case 0xE8: // INX
				case 0xC8: // INY
				case 0xEA: // NOP
				case 0x8B: // PHB
				case 0x0B: // PHD
				case 0x4B: // PHK
				case 0x08: // PHP
				case 0xDA: // PHX
				case 0x5A: // PHY
				case 0x68: // PLA
				case 0xAB: // PLB
				case 0x2B: // PLD
				case 0x28: // PLP
				case 0xFA: // PLX
				case 0x7A: // PLY
					abort_unknown_P = false;
				}
			}

			if (abort_unknown_P) {
				if (writer) {
					sb.clear();
					sb.format("Aborting trace at %06X in %s due to unknown processor status", pc, pb.annotation->name.c_str());
					writer->writeComment(sb);
				}
				break;
			}

			{
				Trace::OpVariantLookup l;
				l.count = 1;
				l.offset = (uint32_t)trace.ops_variants.size();
				trace.ops[pc] = l;
				OpInfo info;
				info.P = P_unknown ? 0 : P;
				info.DB = DB;
				info.DP = DP;
				info.X = info.Y = 0;
				info.jump_target = INVALID_POINTER;
				info.indirect_base_pointer = INVALID_POINTER;
				trace.ops_variants.push_back(info);

				// Note that DB and DP here represent a lie :)
			}

			trace.is_predicted.setBit(pc);
			has_op.setBit(pc);
			int op_size = calculateFormattingandSize(rom.evalPtr(pc), is_memory_accumulator_wide(P), is_index_wide(P), nullptr, nullptr, nullptr);

			uint8_t operand = rom.evalByte(pc+1);

			Pointer target_jump, target_no_jump;
			bool is_jump_or_branch = decode_static_jump(opcode, rom, pc, &target_jump, &target_no_jump);

			bool is_jsr = opcode == 0x20||opcode==0x22||opcode==0xFC;

			const TraceAnnotation *ta = annotations.trace_annotation(pc);
			if (ta && ta->type == TraceAnnotation::JMP_IS_JSR) {
				 is_jump_or_branch = false;
				 is_jsr = true;
			}

			if (is_jump_or_branch) {
				const Annotation *function = nullptr;
				if (target_jump != INVALID_POINTER) {
					annotations.resolve_annotation(target_jump, &function);
					trace.labels.setBit(target_jump);
				}

				if (writer && (!function || function != pb.annotation)) {
					sb.clear();
					sb.format("Branch going out of %s to ", pb.annotation->name.c_str());
					if (function) {
						sb.format("%s [%06X]", function->name.c_str(), target_jump);
					} else {
						sb.format("%06X", target_jump);
					}
					sb.format(". Not following due to range restriction.");
					writer->writeComment(sb);
				}
				
				if (target_jump != INVALID_POINTER) {
					PredictBranch npb = pb;
					npb.pc = target_jump;
					predict_brances.push_back(npb);
				}
			} else if (opcode == 0xE2) {
				//	TODO:
				//	* When we get a REP or SEP parts or P become known again. We could track unknown per the three flags and update.
				//	* Updating DB/DP might be interesting
				if (operand & 0x10) P |= 0x0010;
				if (operand & 0x20) P |= 0x0020;
			} else if (opcode == 0xC2) {
				if (operand & 0x10) P &= ~0x0010;
				if (operand & 0x20) P &= ~0x0020;
			} else if (opcode == 0x28 || opcode == 0xFB) {
				P_unknown = true; // PLP or XCE
				// A jump or BRA, stop execution flow (not JSR or non-BRA-branch)
			} else if (opcode == 0x4C||opcode==0x5C||opcode==0x6C||opcode==0x7C||opcode==0x80) {
				if (writer && opcode != 0x80) {
					sb.clear();
					sb.format("Not following jump (opcode %02X) at %06X in %s. Only absolute jumps supported.", opcode, pc, pb.annotation->name.c_str());
					writer->writeComment(sb);
				}
				// TODO: if there is a trace annotation about jmp being jsr we could go on
				continue;
			} else if (is_jsr) {
				if (writer) {
					sb.clear();
					sb.format("Not following jump with subroutine (opcode %02X) at %06X in %s.", opcode, pc, pb.annotation->name.c_str());
					writer->writeComment(sb);
				}
			} else if (opcode == 0x40 || opcode == 0x6B || opcode == 0x60) {
				// some sort of return, stop execution flow
				continue;
			}
			pc += op_size;
		}
	}
}

void branch_report(ReportWriter &writer, const RomAccessor &rom, const Trace &op_trace, const AnnotationResolver &annotations) {
	StringBuilder sb;
	writer.writeSeperator("Branch analysis");
	writer.writeComment("NOTE: Branches between two functions could mean that they really are one function.");
	writer.writeComment("      Branches out from a function could mean that the function is larger than tagged.");
	writer.writeComment("      This report was generated AFTER extra branches were predicted.");
	writer.writeComment("");

	sb.clear();

	for (const auto &it : op_trace.ops) {
		const Pointer pc = it.first;
		uint8_t opcode = rom.evalByte(pc);
		if (!branches[opcode])
			continue;

		for (int myloop = 0; myloop < 2; myloop++) {
			Pointer target = myloop == 0 ? pc + (unpackSigned(rom.evalByte(pc+1)) + 2) : pc + 2;
			if (myloop == 1 && opcode == 0x80)
				break;

			const Annotation *source_annotation = nullptr, *target_annotation = nullptr;
			annotations.resolve_annotation(pc,     &source_annotation);
			annotations.resolve_annotation(target, &target_annotation);

			if (!source_annotation || source_annotation == target_annotation)
				continue;

			sb.clear();
			sb.format("Branch going from %s", source_annotation->name.c_str());
						
			if (target_annotation) {
				sb.format(" to %s", target_annotation->name.c_str());
			}
			sb.format(" [%06X->%06X]", pc, target);
			writer.writeComment(sb);
		}
	}
}

void entry_point_report(ReportWriter &writer, const Trace &trace, const AnnotationResolver &annotations) {
	writer.writeSeperator("Entry point report");
	writer.writeComment("Disabled");
/*
	struct FunctionInfo {
		int jumps_within_function = 0; // Could be interesting if we limit was sorts of jumps we count (jsr vs jmp)
		std::set<Pointer> targets_inside; // Lazy coding here
	};

	std::vector<FunctionInfo> stats(annotations._annotations.size());

	// We assume this is done when no branches are part of jumps
	for (JumpTrace::const_iterator it = jumps.begin(); it != jumps.end(); ++it) {

		const Pointer from = it->first;

		const Annotation *source_function = nullptr;
		annotations.resolve_annotation(from, &source_function);
		
		for (const JumpInfo &ji : it->second) {
			const Pointer target = ji.target;

			const Annotation *function = nullptr;
			annotations.resolve_annotation(target, &function);

			if (!function)
				continue;

			int function_index = (((uint8_t*)function)-((uint8_t*)&annotations._annotations[0]))/sizeof(Annotation);
			assert(&annotations._annotations[function_index] == function);

			FunctionInfo &fi = stats[function_index];

			if (source_function == function) {
				fi.jumps_within_function++;
			} else if (target == function->startOfRange) {
				fi.targets_inside.insert(target);
			} else {
				fi.targets_inside.insert(target);
			}
		}		
	}

	StringBuilder sb;

	writer.writeSeperator("Entry point analysis");

	for (uint32_t i = 0; i < stats.size(); i++) {
		const FunctionInfo &f = stats[i];
		int entries = f.targets_inside.size();

		const Annotation &a = annotations._annotations[i];
		if (entries>1) {
			sb.clear();
			sb.format("%s has %d entry points (function start %06X)", a.name.c_str(), entries, a.startOfRange);
			writer.writeComment(sb);
			for (Pointer t : f.targets_inside) {
				std::string label = annotations.label(t, nullptr, true);
				sb.clear();
				sb.format("    %06X %s    %s", t, t == a.startOfRange ? "(*)":"   ",label.c_str());
				writer.writeComment(sb);
			}
		} else if (entries == 1) {
			const Pointer t = *f.targets_inside.begin();
			if (t != a.startOfRange) {			
				sb.clear();
				sb.format("%s has an entry point that is not at start (%06X vs %06X)", a.name.c_str(), t, a.startOfRange);
				writer.writeComment(sb);
			}
		}
	}
*/
}

int main(const int argc, const char * const argv[]) {
	try {
		initLookupTables();

		Options options;
		parseOptions(argc, argv, options);

		printf("Loading ROM '%s'\n", options.rom_file.c_str());

		RomAccessor rom_accessor(options.rom_offset, options.calculated_size);
		{
			Profile profile("Load ROM data");
			rom_accessor.load(options.rom_file);
		}

		Trace trace;

		for (uint32_t k=0; k<options.trace_files.size(); ++k) {
			bool generate = true;

			Trace backing_trace;
			Trace &local_trace = k==0 ? trace : backing_trace;

			if (!options.regenerate_emulation && load_trace(options.trace_file_op_cache(k).c_str(), local_trace))
				generate = false;

			if (generate) {
				Profile profile("Create trace using emulation");
				create_trace(options, k, rom_accessor, local_trace);
				save_trace(local_trace, options.trace_file_op_cache(k).c_str());
			}

			if (k != 0) {
				merge_trace(trace, local_trace);
			}
		}

		if (!options.auto_label_file.empty()) {
			FILE *test_file = fopen(options.auto_label_file.c_str(), "rb");
			bool auto_file_exists = test_file != NULL;
			if (test_file != NULL) {
				fclose(test_file);
			}
			
			if (options.generate_auto_annotations || !auto_file_exists) {
				Profile profile("Regenerating auto-labels");
				// We load annotations here since we want to avoid the annotation file
				AnnotationResolver annotations;
				annotations.load(options.labels_files);
				guess_range(trace, rom_accessor, annotations, options.auto_label_file);
			}
		}

		AnnotationResolver annotations;
		{
			Profile profile("Loading game annotations");
			annotations.add_mmio_annotations();
			annotations.add_vector_annotations(rom_accessor);
			if (options.auto_label_file.empty()) {
				// Don't load the auto-labels file if we are re-generating it
				annotations.load(options.labels_files);
			} else {
				std::vector<std::string> copy(options.labels_files);
				copy.push_back(options.auto_label_file);
				annotations.load(copy);
			}
		}

		if (!options.symbol_out_file.empty()) {
			FILE *f = fopen(options.symbol_out_file.c_str(), "wb");
			fprintf(f, "#SNES65816\n");

			fprintf(f, "\n[SYMBOL]\n");
			for (const Annotation &a : annotations._annotations) {
				if (a.type == ANNOTATION_FUNCTION) {
					fprintf(f, "%02X:%04X %s FUNC %d\n", a.startOfRange>>16, a.startOfRange&0xFFFF, a.name.c_str(), a.endOfRange-a.startOfRange+1);
				} else if (a.type == ANNOTATION_LINE && !a.name.empty()) {
					fprintf(f, "%02X:%04X %s FUNC %d\n", a.startOfRange >> 16, a.startOfRange & 0xFFFF, a.name.c_str(), 1);
				}
			}

			fprintf(f, "\n[COMMENT]\n");
			for (const Annotation &a : annotations._annotations) {
				if (a.type == ANNOTATION_LINE) {
					if (!a.comment.empty() && !a.comment_is_multiline) {
						fprintf(f, "%02X:%04X \"%s\"\n", a.startOfRange >> 16, a.startOfRange&0xFFFF, a.comment.c_str());
					}
				}
			}
			fclose(f);
		}

		// Make sure this happens after create_trace so skip file is fresh
		if (!options.rewind_file.empty()) {
			rewind_report(options, rom_accessor, annotations);
		}

		// Write trace log if requested
		if (!options.trace_log.empty()) {
			Profile profile("Create trace log");

			bool has_scripting = !options.trace_log_script.empty();

			scripting_interface::Scripting *scripting = nullptr;
			
			if (!options.trace_log_script.empty())
				scripting = scripting_interface::create_scripting(options.trace_log_script.c_str());
			
			write_trace_log(options, rom_accessor, annotations, scripting);

			if (scripting)
				scripting_interface::destroy_scripting(scripting);
		}

		std::unique_ptr<ReportWriter> report_writer;
		if (!options.asm_report_file.empty())
			report_writer.reset(new ReportWriter(options.asm_report_file.c_str()));

		// TODO: Maybe run once before guess_range as well to find longer ranges
		predict(options.predict_mode, report_writer.get(), rom_accessor, trace, annotations);

		if (!options.asm_file.empty()) {
			Profile profile("Writing asm");
			ReportWriter asm_output(options.asm_file.c_str());
			asm_writer(asm_output, options, trace, annotations, rom_accessor);
		}

		if (report_writer) {
			data_report(*report_writer, trace, annotations);
			dma_report(*report_writer, trace, annotations);
			branch_report(*report_writer, rom_accessor, trace, annotations);
			entry_point_report(*report_writer, trace, annotations);
		}

	} catch (const std::runtime_error &e) {
		printf("error: %s\n", e.what());
		return 1;
	} catch (const std::exception &e) {
		printf("error: %s\n", e.what());
		return 1;
	}

	return 0;
}
