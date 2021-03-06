#include "trace_log.h"
#include "emulate.h"
#include "rom_accessor.h"
#include "annotations.h"
#include "options.h"
#include "scripting.h"
#include <stack>
#include <unordered_set>
#include <memory>
#include "replay.h"
#include "report_writer.h"
#include "cputable.h"

// Rewrite trace_log_filter to be more like breakpoints so don't need to invoke script
bool trace_log_filter(Pointer pc, Pointer function_start, Pointer function_end, const char * const function_name) { return true; }

namespace {

static const int TRACE_START_INDENTATION = 4;

struct TraceState {
	std::stack<int> current_depths;
	FILE *report;
};

void trace_separator(TraceState &state, const char * const text) {
	static const char lines[]="--------------------------------------------------------------------------------------------------------------------------------------------------------------------------";

	assert(!state.current_depths.empty());
	int width = state.current_depths.top() * 2 - 1;

	if (width <= 0) width = 0;

	int s = 0;
	
	fputc('\n', state.report);
	s += (int)fwrite(lines, 1, width, state.report);
	s += (int)fprintf(state.report, " %s ", text);

	int remain_width = 130 - s;

	if (remain_width>0)
		s += (int)fwrite(lines, 1, remain_width, state.report);

	fputc('\n', state.report);
	fputc('\n', state.report);
}

int trace_indent_line(TraceState &state) {
	static const char spaces[]="                                                                                                                                                                                                                                                      ";

	assert(!state.current_depths.empty());
	int width = state.current_depths.top() * 2;

	if (width <= 0) return 0;

	assert(width >=0 && width< 160);
	return (int)fwrite(spaces, 1, width, state.report);
}

void trace_fix_depth(TraceState &state) {
	// Failsafe
	if (state.current_depths.empty() || state.current_depths.top() > 50 || state.current_depths.top() < 0 || state.current_depths.size() > 10) {
		while (!state.current_depths.empty()) state.current_depths.pop();
		state.current_depths.push(TRACE_START_INDENTATION);
		trace_separator(state, "RESETTING INDENTATION");
	}
}
}

namespace snestistics {

void write_trace_log(const Options &options, const RomAccessor &rom, const AnnotationResolver &annotations, scripting_interface::Scripting *scripting) {
	
	CUSTOM_ASSERT(options.trace_files.size() == 1);

	Replay replay(rom, options.trace_files[0].c_str());

	ReportWriter rw(options.trace_log_out_file.c_str());

	scripting_interface::ScriptingHandle scripting_replay = scripting ? scripting_interface::create_replay(scripting, &replay) : nullptr;
	scripting_interface::ScriptingHandle scripting_report_writer = scripting ? scripting_interface::create_report_writer(scripting, &rw) : nullptr;

	if (scripting) {
		scripting_interface::scripting_trace_log_init(scripting, scripting_replay);
	}

	TraceState ts;
	ts.report = rw.report;
	ts.current_depths.push(TRACE_START_INDENTATION);

	trace_separator(ts, "START");

	const Annotation* current_function = nullptr;

	std::unordered_set<uint32_t> missing_annotation;

	const uint32_t capture_nmi_first = options.nmi_first, capture_nmi_last = options.nmi_last;

	printf("Skipping to nmi %d\n", capture_nmi_first);
	EmulateRegisters &regs = replay.regs;
	bool success = replay.skip_until_nmi(capture_nmi_first);
	assert(success);

	uint32_t current_nmi = capture_nmi_first;

	bool do_logging_for_current_function = true;

	while (true) {

		const uint32_t pc = regs._PC;

		if (do_logging_for_current_function && scripting && replay.breakpoints[pc]) {
			rw.indentation = ts.current_depths.top() * 2 + 1; // Set indentation
			scripting_trace_log_parameter_printer(scripting, scripting_replay, scripting_report_writer);
		}

		bool more = replay.next();
		const uint32_t jump_pc  = regs._PC;

		if (!more)
			break;

		if (current_nmi > capture_nmi_last)
			break;

		bool pc_change = true;
		bool is_jump_with_return = false;
		bool is_return = false;

		if (regs.event == Events::NMI) {
			trace_indent_line(ts); fprintf(rw.report, "  # NMI %d\n", current_nmi);
			ts.current_depths.push(TRACE_START_INDENTATION);
			trace_separator(ts, "NMI");
			//printf("Tracelog for nmi %d (%.1f%%)\n", current_nmi, (1+current_nmi-capture_nmi_first)*100.0f/(capture_nmi_last-capture_nmi_first+1));
			current_nmi++;
		} else if (regs.event == Events::RESET) {
			// This have no impact. If we skip frames it will not happen.
			continue;
		} else if (regs.event == Events::IRQ) {
			trace_indent_line(ts); fprintf(rw.report, "  # IRQ\n");
			ts.current_depths.push(TRACE_START_INDENTATION);
			trace_separator(ts, "IRQ");
			continue;
		} else if (regs.event == Events::RTI) {
			if (do_logging_for_current_function)
				fprintf(rw.report, "\n --- RETURN FROM INTERRUPT --- \n\n");
			ts.current_depths.pop();
			trace_fix_depth(ts);
		} else if (regs.event == Events::JMP_OR_JML) {
		} else if (regs.event == Events::JSR_OR_JSL) {
			is_jump_with_return = true;
		} else if (regs.event == Events::RTS_OR_RTL) {
			is_return = true;
		} else if (regs.event == Events::BRANCH) {
		} else if (regs.event == Events::NONE) {
			// Just a plain boring regular op
			pc_change = false;
		} else {
			assert(false);
		}

		int spacing = 0;
		bool print_missing_annotation = false;

		// If there was no jump but we strayed outside our function, print a warning
		// This is about function annotations being off
		if (!pc_change && current_function && (pc < current_function->startOfRange || pc > current_function->endOfRange)) {

			const Annotation *target_function = nullptr;
			annotations.resolve_annotation(pc, &target_function );

			if (target_function) {
				trace_indent_line(ts); fprintf(rw.report, "WARNING: Was in function %s but now running in %s %06X\n", current_function->name.c_str(), target_function->name.c_str(), pc);
			} else {
				trace_indent_line(ts); fprintf(rw.report, "WARNING: Was in function %s but now running outside at %06X\n", current_function->name.c_str(), pc);
			}
			current_function = target_function;

			// We are in a new function (or in no function no, make sure we print its name)
			pc_change = true;
		}

		if (pc_change) {
			// Avoid updating depth if we are ignoring the function
			if (!current_function) {
				const Hint *ta = annotations.hint(pc);
				const bool jump_has_faked_return = ta && ta->has_hint(Hint::JUMP_IS_JSR);

				if (is_jump_with_return || jump_has_faked_return) {
					ts.current_depths.top()++;
					trace_fix_depth(ts);
				} else if (is_return) {
					ts.current_depths.top()--;
					trace_fix_depth(ts);
				}
			}

			const Annotation *target_function = nullptr;
			annotations.resolve_annotation(jump_pc, &target_function );

			if (target_function) {
				do_logging_for_current_function = trace_log_filter(jump_pc, target_function->startOfRange, target_function->endOfRange, target_function->name.c_str());
			} else {
				do_logging_for_current_function = true;
			}

			if (do_logging_for_current_function && current_function != target_function) {
				if (target_function) {
					spacing += trace_indent_line(ts);
					spacing += fprintf(rw.report, "%s", target_function->name.c_str());
				} else {
					spacing += trace_indent_line(ts);
					spacing += fprintf(rw.report, "Jumped from %06X to %06X (not annotated)", pc, jump_pc);

					if (missing_annotation.find(jump_pc) == missing_annotation.end()) {
						missing_annotation.insert(jump_pc);
						print_missing_annotation = true;
					}
				}
			}
			current_function = target_function;
		}

		// If we wrote a function name, output regs
		if (spacing > 0) {
			int move = 76-spacing;
			while (move<0) move+=8;
			for (int k=0; k<move; k++) fputc(' ', rw.report);
			fprintf(rw.report, "X=%04X Y=%04X A=%04X DB=%02X DP=%04X S=%04X P=%04X PC=%06X [%06X-%06X] NMI=%d\n", regs._X, regs._Y, regs._A, regs._DB, regs._DP, regs._S, regs._P, regs._PC, current_function ? current_function->startOfRange : 0, current_function ? current_function->endOfRange : 0, current_nmi);
		}

		if (print_missing_annotation) {
			trace_indent_line(ts);
			fprintf(rw.report, "MISSING ANNOTATION FOR %06X (only reporting once)\n", jump_pc);
			printf("Missing annotation at pc %06X\n", jump_pc);
		}
	}

	if (scripting) {
		scripting_interface::destroy_handle(scripting, scripting_replay);
		scripting_interface::destroy_handle(scripting, scripting_report_writer);
	}
}
}
