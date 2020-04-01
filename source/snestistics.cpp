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
#include "rom_accessor.h"
#include "asm_writer.h"
#include "annotations.h"
#include "cputable.h"
#include "trace_log.h"
#include "trace.h"
#include "rewind.h"
#include "scripting.h"
#include "report_writer.h"
#include "symbol_export.h"
#include "auto_annotate.h"
#include "predict.h"

using namespace snestistics;

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
					sb.format("(unannotated)");
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
			Pointer target = myloop == 0 ? branch8(pc, rom.evalByte(pc+1)) : pc + 2;
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
		parse_options(argc, argv, options);

		printf("Loading ROM '%s'\n", options.rom_file.c_str());

		// TODO: TraceHeader has rom_size and rom_mode (lorom/hirom) so lets read them!
		RomAccessor rom_accessor(options.rom_size);
		{
			Profile profile("Load ROM data");
			rom_accessor.load(options.rom_file);
		}

		Trace trace;

		for (uint32_t k=0; k<options.trace_files.size(); ++k) {
			bool generate = true;

			Trace backing_trace;
			Trace &local_trace = k==0 ? trace : backing_trace;

			if (load_trace_cache(options.trace_files[k], local_trace))
				generate = false;

			if (generate) {
				Profile profile("Create trace using emulation");
				create_trace(options, k, rom_accessor, local_trace); // Will automatically save new cache
			}

			if (k != 0) {
				merge_trace(trace, local_trace);
			}
		}

		if (!options.auto_labels_file.empty()) {
			FILE *test_file = fopen(options.auto_labels_file.c_str(), "rb");
			bool auto_file_exists = test_file != NULL;
			if (test_file != NULL) {
				fclose(test_file);
			}
			
			if (options.auto_annotate || !auto_file_exists) {
				Profile profile("Regenerating auto-labels");
				// We load annotations here since we want to avoid the annotation file
				AnnotationResolver annotations;
				annotations.load(options.labels_files);
				guess_range(trace, rom_accessor, annotations, options.auto_labels_file);
			}
		}

		AnnotationResolver annotations;
		{
			Profile profile("Loading game annotations");
			annotations.add_mmio_annotations();
			annotations.add_vector_annotations(rom_accessor);
			if (options.auto_labels_file.empty()) {
				// Don't load the auto-labels file if we are re-generating it
				annotations.load(options.labels_files);
			} else {
				std::vector<std::string> copy(options.labels_files);
				copy.push_back(options.auto_labels_file);
				annotations.load(copy);
			}
		}

		if (!options.symbol_fma_out_file.empty()) {
			symbol_export_fma(annotations, options.symbol_fma_out_file, false);
		}

		if (!options.symbol_mesen_s_out_file.empty()) {
			// false here is to disable multiline comments since bsnes-plus does not support them
			symbol_export_mesen_s(annotations, options.symbol_mesen_s_out_file);
		}

		// Make sure this happens after create_trace so skip file is fresh
		if (!options.rewind_out_file.empty()) {
			rewind_report(options, rom_accessor, annotations);
		}

		// Write trace log if requested
		if (!options.trace_log_out_file.empty()) {
			Profile profile("Create trace log");

			bool has_scripting = !options.script_file.empty();

			scripting_interface::Scripting *scripting = nullptr;
			
			if (!options.script_file.empty())
				scripting = scripting_interface::create_scripting(options.script_file.c_str());
			
			write_trace_log(options, rom_accessor, annotations, scripting);

			if (scripting)
				scripting_interface::destroy_scripting(scripting);
		}

		std::unique_ptr<ReportWriter> report_writer;
		if (!options.report_out_file.empty())
			report_writer.reset(new ReportWriter(options.report_out_file.c_str()));

		// TODO: Maybe run once before guess_range as well to find longer ranges
		predict(options.predict, report_writer.get(), rom_accessor, trace, annotations);

		if (!options.asm_out_file.empty()) {
			Profile profile("Writing asm");
			ReportWriter asm_output(options.asm_out_file.c_str());
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
