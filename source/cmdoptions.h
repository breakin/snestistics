#ifndef SNESTISTICS_CMDOPTIONS_H
#define SNESTISTICS_CMDOPTIONS_H

#include <string>
#include <stdint.h>

struct Options {
	Options() {
		allowPrediction = false;
		printHexOpcode = false;
		printProgramCounter = false;
		printFileAdress = false;
		printDataJumpsAsComments = true;
		romOffset = 512;
		outFile = "result.asm";
		calculatedSize = 8*0x20000; // 8mbit LoROM
	}

	int romOffset; // 512 for SMC, 0 for SFC
	int calculatedSize; // Size of ROM
	bool printDataJumpsAsComments;
	bool printHexOpcode;
	bool printProgramCounter;
	bool printFileAdress;
	bool allowPrediction;

	std::string romFile;
	std::string labelsFile;
	std::string outFile;
	std::string traceFile;
	std::string asmHeaderFile;
};

bool parseBool(const char * const s) {
	return (strcmp(s, "true")==0) || (strcmp(s, "on")==0);
}
int parseInt(const char * const s) {
	return atoi(s);
}

bool parseOptions(const int argc, const char * const argv[], Options &options) {

	for (int k=1; k<argc; k++) {
		
		// Make sure we have a pair of strings
		const std::string cmd(argv[k]);
		const char * opt = "";
			
		if (k != argc-1) {
			opt = argv[k+1];
		}

		// All the doubles
		if (cmd=="-romfile") {
			options.romFile = opt;
			// TODO: Predict ROM-offset from extension (or possible filesize)
			k++;
		} else if (cmd == "-asmheaderfile") {
			options.asmHeaderFile = opt;
			k++;
		} else if (cmd == "-out") {
			options.outFile = opt;
			k++;
		} else if (cmd == "-tracefile") {
			options.traceFile = opt;
			k++;
		} else if (cmd == "-labelsfile") {
			options.labelsFile = opt;
			k++;
		} else if (cmd == "-sfc") {
			options.romOffset = 0;
		} else if (cmd == "-smc") {
			options.romOffset = 512;
		} else if (cmd == "-predict") {
			options.allowPrediction = parseBool(opt);
			k++;
		} else if (cmd == "-cs") {
			options.calculatedSize = parseInt(opt);
			k++;
		}
	}

	bool error = false;

	if (options.romFile.length()==0) {
		printf("No romfile specified with -romfile\n");
		error = true;
	}
	if (options.traceFile.length()==0) {
		printf("No traceFile specified with -traceFile\n");
		error = true;
	}

	if (error) {
		// TODO: Print syntax
		return false;
	}

	return true;
}

#endif
