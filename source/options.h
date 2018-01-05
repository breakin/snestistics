#pragma once

#include <string>
#include <cstdlib>
#include <stdint.h>
#include <vector>

struct Options {
	Options() {
	}

	enum PredictMode {
		PREDICT_NOTHING,
		PREDICT_FUNCTIONS,
		PREDICT_EVERYTHING
	};

	int rom_offset = -1; // Auto-detect
	int calculated_size = -1; // Auto-detect
	bool printDataJumpsAsComments = true;
	bool printHexOpcode = true;
	bool printProgramCounter = true;
	bool lowerCaseOpCode = true;
	bool printCorrectWLA = false;
	bool printRegisterSizes = true;
	bool printDB = true;
	bool printDP = true;
	bool regenerate_emulation = false;
	bool generate_auto_annotations = false;

	PredictMode predict_mode = PREDICT_FUNCTIONS;

	std::string rom_file;
	std::vector<std::string> labels_files;
	std::string auto_label_file;
	std::string asm_file;
	std::string asm_report_file;
	std::vector<std::string> trace_files;
	std::string rewind_file; // Very experimental, might be folded into something.. uses as a on/off switch for tracking prototype for now
	std::string asm_header_file;
	std::string symbol_fma_out_file;

	std::string trace_file_skip_cache(int k) const { return trace_files[k] + ".skip.temp"; }
	std::string trace_file_op_cache(int k) const { return trace_files[k] + ".op.temp"; }

	// Trace log
	std::string trace_log, trace_log_script;
	uint32_t trace_log_nmi_first=5000, trace_log_nmi_last=10000;
};

void parseOptions(const int argc, const char * const argv[], Options &options);
