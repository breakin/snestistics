#include "options.h"
#include <cstdlib>
#include <cstring> // strcmp
#include <stdexcept> // std::runtime_error

namespace {
	bool parseBool(const char * const s) {
		return (strcmp(s, "true")==0) || (strcmp(s, "on")==0);
	}
	int parseInt(const char * const s) {
		return atoi(s);
	}
}

void parseOptions(const int argc, const char * const argv[], Options &options) {
	bool error = false;

	for (int k=1; k<argc; k++) {
		// Make sure we have a pair of strings
		const std::string cmd(argv[k]);
		const char * opt = "";
			
		if (k != argc-1) {
			opt = argv[k+1];
		}

		// All the doubles
		if (cmd=="-romfile") {
			options.rom_file = opt;
			k++;
		}
		else if (cmd == "-asmheaderfile") {
			options.asm_header_file = opt;
			k++;
		} else if (cmd == "-asmfile") {
			options.asm_file = opt;
			k++;
		} else if (cmd == "-reportfile") {
			options.asm_report_file = opt;
			k++;
		} else if (cmd == "-tracefile") {
			options.trace_files.push_back(opt);
			k++;
		} else if (cmd == "-labelsfile") {
			options.labels_files.push_back(opt);
			k++;
		} else if (cmd == "-rewindfile") {
			options.rewind_file = opt;
			k++;
		} else if (cmd == "-sfc") {
			options.rom_offset = 0;
		} else if (cmd == "-smc") {
			options.rom_offset = 512;
		} else if (cmd == "-regenerate") {
			options.regenerate_emulation = true;
		} else if (cmd == "-predict") {
			if (strcmp(opt, "nothing")==0) options.predict_mode = Options::PREDICT_NOTHING;
			else if (strcmp(opt, "functions")==0) options.predict_mode = Options::PREDICT_FUNCTIONS;
			else if (strcmp(opt, "everything")==0) options.predict_mode = Options::PREDICT_EVERYTHING;
			else {
				printf("-predict can take nothing, functions or everything as value.");
			}
			k++;
		} else if (cmd == "-cs") {
			options.calculated_size = parseInt(opt);
			k++;
		} else if (cmd == "-lowercaseopcode") {
			options.lowerCaseOpCode = parseBool(opt);
			k++;
		} else if (cmd == "-autoannotate") {
			options.generate_auto_annotations = parseBool(opt);
			k++;
		} else if (cmd == "-autolabels") {
			options.auto_label_file = opt;
			k++;
		} else if (cmd == "-asm-print-pc") {
			options.printProgramCounter = parseBool(opt);
			k++;
		} else if (cmd == "-asm-print-opcode") {
			options.printHexOpcode = parseBool(opt);
			k++;
		} else if (cmd == "-tracelog") {
			options.trace_log = opt;
			k++;
		} else if (cmd == "-tracelogscript") {
			options.trace_log_script = opt;
			k++;			
		} else if (cmd == "-nmi_first") {
			options.trace_log_nmi_first = parseInt(opt);
			k++;
		} else if (cmd == "-nmi_last") {
			options.trace_log_nmi_last = parseInt(opt);
			k++;
		} else {
			printf("Unknown switch %s\n", cmd.c_str());
			error = true;
			break;
		}
	}

	if (options.generate_auto_annotations && options.auto_label_file.empty()) {
		printf("Can't regenerate auto labels when no labels file is specified with -autolabels");
		error = true;
	}

	if (options.trace_files.empty()) {
		printf("No trace file(s) specified with -tracefile\n");
		error = true;
	}

	if (options.trace_files.size() > 1) {
		if (!options.rewind_file.empty() || !options.trace_log.empty()) {
			printf("Can't specify trace log or rewind file when using multiple trace files!");
			error = true;
		}
	}

	if (options.rom_file.empty()) {
		printf("No romfile specified with -romfile\n");
		error = true;
	}

	if (error) {
		// TODO: Print syntax
		throw std::runtime_error("Error parsing command line!");
	}
}
