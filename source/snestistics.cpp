#include <stdint.h>
#include <set>
#include <vector>
#include <string>
#include <cassert>
#include <fstream>
#include <map>
#include "snesops.h"
#include "cmdoptions.h"

/*
	Currently most of this code assumes that we are dealing with a LoROM SNES-game.
	It also assumes that all code is executed from ROM, ie no self-modifying code and no changing code.

	NOTE: WLA DX seems most comfortable with at most 256 sections
	      Short breaks between sections is filled using .DB instead of starting new sections.
*/

struct RangeInfo {
	RangeInfo() {}
	RangeInfo(const Pointer pc) : firstPC(pc), lastPC(pc) {}
	RangeInfo(const Pointer pc, const Pointer pc2) : firstPC(pc), lastPC(pc2) {}

	Pointer firstPC,lastPC;
	std::string labelName, comment;
};

typedef std::map<Pointer, RangeInfo> RangeInfoMap;

void insertRangeInfo(const RangeInfo &ri, RangeInfoMap &labels) {
	labels[ri.firstPC]=ri;
}

bool fillLabels(const std::string &labelsFile, RangeInfoMap &labels) {

	if (labelsFile.length()==0) {
		return true;
	}

	std::ifstream f;
	f.open(labelsFile);

	std::string comment;

	int numLocalLabels = 0;

	while (!f.eof()) {
		char buf[4096];
		f.getline(buf, 4096);

		if (strlen(buf)==0) {
			continue;
		}
		if (buf[0]==';') {
			if (comment.length() !=0) {
				comment = comment + "\n" + buf;
			} else {
				comment = buf;
			}
		} else {
			char labelName[4096];
			memset(labelName, 0, 4096);
			Pointer pc,pc2;

			numLocalLabels = 0;

			if (buf[6]==':') {
				sscanf(buf, "%06X:%06X \"%[^\"]", &pc, &pc2, labelName);

				for (Pointer p = pc+1; p<=pc2; ++p) {
					if (labels.find(p) != labels.end()) {

						char a[128];
						sprintf(a, "_%s_%d", labelName, numLocalLabels);

						RangeInfo le(p);
						le.labelName = a;
						insertRangeInfo(le, labels);

						numLocalLabels++;
					}
				}


			} else {
				sscanf(buf, "%06X \"%[^\"]", &pc, labelName);
				pc2=pc;
			}

			// TODO: Validate labelName

			RangeInfo le(pc,pc2);
			le.labelName = labelName;
			le.comment = comment;
			insertRangeInfo(le, labels);
			comment.clear();
		}
	}

	return true;
}

void emitSectionStart(FILE *fout, const Pointer pc, int *sectionCounter) {
	fprintf(fout, ".BANK $%02X SLOT 0\n", pc>>16);
	fprintf(fout, ".ORG $%04X-$8000\n", pc&0xffff);
	fprintf(fout, ".SECTION SnestisticsSection%d OVERWRITE\n", *sectionCounter);
	(*sectionCounter)++;
}

void emitSectionEnd(FILE *fout) {
	fprintf(fout, "\n.ENDS\n\n");
}

std::string getLabelName(const Pointer p) {
    char hej[64];
    sprintf(hej, "label_%06X", p);
    return hej;
}

void visitOp(const Options &options, const Pointer pc, const uint16_t ps, const bool predict, const std::set<Pointer> &knownOps, std::set<Pointer> &predictOps, RangeInfoMap &labels, std::vector<uint16_t> &statusRegArray, const std::vector<uint8_t> &romdata) {
    
    const uint32_t romAdr = options.romOffset + getRomOffset(pc, options.calculatedSize);
    const uint8_t ih = romdata[romAdr];
        
    char pretty[16];
    char labelPretty[16];
    Pointer jumpDest(0);
    const int nb = processArg(pc, ih, &romdata[romAdr], pretty, labelPretty, &jumpDest, ps, 0);

	// Make sure this op is not "inside" a valid op
	if (predict) {
		for (int k=1; k<nb; k++) {
			if (knownOps.find(pc+k) != knownOps.end()) {
				return;
			}
		}
	}
        
    if (jumps[ih]) {
        if (jumpDest != 0) {
			RangeInfo le(jumpDest);
			le.labelName = getLabelName(jumpDest);
			insertRangeInfo(le, labels);
        }
    }

	if (!options.allowPrediction) {
		return;
	}

    if (predict) {
        predictOps.insert(pc);
        statusRegArray[pc]=ps;
    }
    
    const char * const op = S9xMnemonics[ih];
    
    // If this happens, we should not predict the next instruction. Could be data!
	if (endOfPrediction[ih]) {
		return;
	}
    
    // Update process status register so we can interpret the byte stream correctly
    uint16_t next_ps = ps;
    if (strcmp(op, "SEP")==0) { next_ps |=  romdata[romAdr+1]; } // set
	if (strcmp(op, "REP")==0) { next_ps &= ~romdata[romAdr+1]; } // reset
    
    // TODO; Ok to just say that processor status is unknown, only matters for a few ops... make processArg say what it depends on so it can be discarded...
    if (strcmp(op, "PLP")==0) { return; }

    // TODO: Do we care about emulation bit? We could trace XCE, SEC, CLC and track weather carry bit is known or not.
	// Snes9x lists what is different (but not in debug.cpp)
    
    const Pointer next = pc + nb;

    if (knownOps.find(next) == knownOps.end() && predictOps.find(next)==predictOps.end()) {
        // Not visited yet
        visitOp(options, next, next_ps, true, knownOps, predictOps, labels, statusRegArray, romdata);
    }
 }

bool readFile(const std::string &filename, std::vector<uint8_t> &result) {
	if (filename.length()==0) {
		return true;
	}

	FILE *f = fopen(filename.c_str(), "rb");
	if (f==0) {
		printf("Could not open the file '%s' for reading\n", filename.c_str());
		return false;
	}
	fseek(f, 0, SEEK_END);
	const int fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	result.resize(fileSize);
	fread(&result[0], 1, fileSize, f);
	fclose(f);
	return true;
}

int main(const int argc, const char * const argv[]) {

	Options options;
	if (!parseOptions(argc, argv, options)) {
		return 1;
	}

	initLookupTables();

	std::vector<uint8_t> romdata;
	if (!readFile(options.romFile, romdata)) {
		return 2;
	}
	std::vector<uint8_t> asmHeader;
	if (!readFile(options.asmHeaderFile, asmHeader)) {
		return 2;
	}

	// We can assume that all PC are saved in order
	FILE *f2 = fopen(options.traceFile.c_str(), "rb");
	if (!f2) {
		printf("Could not open trace-file '%s'\n", options.romFile.c_str());
		return 2;
	}

	const uint16_t expectedVersionNumber = 0x0001;
	uint16_t versionNumber;
	fread(&versionNumber, sizeof(versionNumber), 1, f2);

	if(versionNumber != expectedVersionNumber) {
		printf("Wrong version number %04X, expected %04X\n", versionNumber, expectedVersionNumber);
		return 3;
	}

	std::set<Pointer> ops;
	RangeInfoMap labels;
	std::vector<uint16_t> opStatus;

	opStatus.resize(256*65536, 0xffff);

	std::map<Pointer,std::set<Pointer>> takenJumps;
    
	// First pass: Parse file, find all opcode starts and their status.
	while (!feof(f2)) {
		uint8_t code;
		
		fread(&code, 1, 1, f2);
		if (code==0) {
			Pointer p;
			uint16_t status;

			fread(&p, sizeof(Pointer), 1, f2);
			fread(&status, 2, 1, f2);

			ops.insert(p);
			opStatus[p]=status;
		} else if (code==1) {

			Pointer p,t;
			fread(&p, sizeof(Pointer), 1, f2);
			fread(&t, sizeof(Pointer), 1, f2);
			takenJumps[p].insert(t);
		
			RangeInfo le(t);
			le.labelName = getLabelName(t);
			insertRangeInfo(le, labels);

		} else {
			assert(false);
		}
	}

	fclose(f2);
    
    std::set<Pointer> predictOps;
    
	// Determine all labels that we need for the determinisic jumps
	for (auto it = begin(ops); it != end(ops); ++it) {
        visitOp(options, *it, opStatus[*it], false, ops, predictOps, labels, opStatus, romdata);
	}

    // Make all predicted ops real ops
    for (auto it = begin(predictOps); it != end(predictOps); ++it) {
        ops.insert(*it);
    }
    
	// Overwrite some of the predetermined labels with user-defined friendly names
	if (!fillLabels(options.labelsFile, labels)) {
		return 3;
	}

	FILE *fout = fopen(options.outFile.c_str(), "wb");
	if (!fout) {
		printf("Could not open %s for writing result\n", options.outFile.c_str());
		return 3;
	}

	fwrite(&asmHeader[0], asmHeader.size(), 1, fout);

	Pointer next(0);

	int sectionCounter=0;

	int numBits = 8;

	bool firstSection = true;

	for (auto it = begin(ops); it != end(ops); ++it) {
		const Pointer pc = *it;

		const uint32_t romAdr = options.romOffset + getRomOffset(pc, options.calculatedSize);
		const uint8_t ih = romdata[romAdr];

		if (pc < next && predictOps.find(pc) != predictOps.end()) {
			continue;
		}
        
		if (pc != next) {

			const int gap = pc - next;

			if (gap>16*64 || (bank(pc)!=bank(next))) {
				if (!firstSection) {
					emitSectionEnd(fout);
				}
				if (!firstSection) {
					fprintf(fout, "; %d bytes gap (0x%X)\n\n", gap, gap);
				}
				firstSection = false;
                
				emitSectionStart(fout,pc,&sectionCounter);
			} else {

				fprintf(fout, "\n");
				

				Pointer p = next;
				for (int y=0; y<64 && p<pc; y++) {
					fprintf(fout, ".DB ");
					for (int x=0; x<16 && p<pc; x++, ++p) {
						const uint32_t pr = options.romOffset + getRomOffset(p, options.calculatedSize);
						fprintf(fout, "$%02X", romdata[pr]);

						if (x!=32-1 && p != pc-1) {
							fprintf(fout, ",");
						}
					}
					fprintf(fout, "\n");
				}
				next = pc;
			}
		}

		auto labelIt = labels.find(pc);
		if (labelIt != labels.end()) {
			if (labelIt->second.labelName=="__jumpTableWord__") {

				Pointer dataStart = pc;
				Pointer dataEnd = labelIt->second.lastPC;

				const int guessBank = pc >> 16;

				for (Pointer a = dataStart; a <= dataEnd; a += 2) {
					const uint32_t pr = options.romOffset + getRomOffset(a, options.calculatedSize);
					const Pointer full = (guessBank << 16) | (romdata[pr + 1] << 8) | (romdata[pr + 0]);

					// TODO: Change to DL?
					fprintf(fout, "\t.DB $%02X,$%02X", romdata[pr + 0], romdata[pr + 1]);
					auto targetLabelIt = labels.find(full);
					if (targetLabelIt != labels.end()) {
						fprintf(fout, "\t\t; %s", targetLabelIt->second.labelName.c_str());
					}
					else {
						fprintf(fout, "\t\t; %06X", full);
					}
					fprintf(fout, "\n");
					next += 2;
				}
				continue;

			} else if (labelIt->second.labelName=="__jumpTableLong__") {

				Pointer dataStart = pc;
				Pointer dataEnd = labelIt->second.lastPC;

				for (Pointer a = dataStart; a <= dataEnd; a+=3) {
					const uint32_t pr = options.romOffset + getRomOffset(a, options.calculatedSize);
					const Pointer full = (romdata[pr+2]<<16)|(romdata[pr+1]<<8)|(romdata[pr+0]);

					// TODO: Change to DL?
					fprintf(fout, "\t.DB $%02X,$%02X,$%02X", romdata[pr + 0], romdata[pr + 1], romdata[pr + 2]);

					auto targetLabelIt = labels.find(full);
					if (targetLabelIt != labels.end()) {
						fprintf(fout, "\t\t; %s", targetLabelIt->second.labelName.c_str());
					} else {
						fprintf(fout, "\t\t; %06X", full);
					}					
					fprintf(fout, "\n");
					next+=3;
				}
				continue;

			} else {
				fprintf(fout, "\n%s\n", labelIt->second.comment.c_str());
				fprintf(fout, "%s:\n\n", labelIt->second.labelName.c_str());
			}
		}

		Pointer jumpDest(0);
		char pretty[16]="\0";
		char labelPretty[16]="\0";

		// TODO: Hand in processor status register
		int needBits = numBits; // Unless processArg cares, let it stay!
		const HardwareAdressEntry *adressContext = 0;
		const int numBytes = processArg(pc, ih, &romdata[romAdr], pretty, labelPretty, &jumpDest, opStatus[pc], &needBits, &adressContext);

		if (needBits != numBits) {
			if (options.printCorrectWLA) {
				if (needBits== 8) { fprintf(fout, ".8bit\n"); }
				if (needBits==16) { fprintf(fout, ".16bit\n"); }
				if (needBits==24) { fprintf(fout, ".24bit\n"); }
			}
			numBits = needBits;
		}

		fprintf(fout, "\t");
		if (options.printHexOpcode || options.printProgramCounter || options.printFileAdress) {
			fprintf(fout, "/* ");

			if (options.printProgramCounter) {
				fprintf(fout, "%06X " , pc);
			}
			if (options.printFileAdress) {
				fprintf(fout, "%06X " , romAdr);
			}
			if (options.printHexOpcode) {
				for (int k=0; k<numBytes; k++) {
					fprintf(fout, "%02X ", romdata[romAdr+k]);
				}
				for (int k=0; k<4-numBytes; k++) {
					fprintf(fout, "   ");
				}
			}

			fprintf(fout, "*/ ");
		}

		if (options.lowerCaseOpCode) {
			fprintf(fout, "%c%c%c ", tolower(S9xMnemonics[ih][0]), tolower(S9xMnemonics[ih][1]), tolower(S9xMnemonics[ih][2]));
		} else {
			fprintf(fout, "%s ", S9xMnemonics[ih]);		
		}

		if (jumps[ih] && jumpDest != 0 && ops.find(jumpDest) != ops.end()) {
			fprintf(fout, labelPretty, labels[jumpDest].labelName.c_str());
		} else {
			fprintf(fout, "%s", pretty);
		}
        
		const bool predicted = predictOps.find(pc) != predictOps.end();
		bool emitPred = false;
		bool emitN = false;

		if (jumps[ih] && !jumpDest) {
			auto recordedJumps = takenJumps.find(pc);
			if (recordedJumps != takenJumps.end()) {
				for (auto pit = recordedJumps->second.begin(); pit != recordedJumps->second.end(); ++pit) {
					const Pointer p = *pit;
					if (ops.find(p) != ops.end() && labels.find(p) != labels.end()) {
						fprintf(fout, "\t\t; \"%s\" [%06X]", labels[p].labelName.c_str(), p);
					} else {
						fprintf(fout, "\t\t; %06X", p);
					}					
					if (!emitPred && predicted) {
						emitPred = true;
						fprintf(fout, " [P]");
					}
					emitN = true;
					fprintf(fout, "\n");
				}
			}
		}

		bool beginComment = false;

		if (adressContext) {
			if (!beginComment) {
				fprintf(fout, "\t\t;");
				beginComment = true;
			}
			fprintf(fout, " %s", adressContext->desc.c_str());
		}

		if (predicted && !emitPred) {
			if (!beginComment) {
				fprintf(fout, "\t\t;");
				beginComment = true;
			}
			fprintf(fout, " [P]");
		}

		if (!emitN) {
			fprintf(fout, "\n");
		}

		if (endOfPrediction[ih]) {
			// TODO: Looks weird if a label comes directly after, double \n
			fprintf(fout, "\n");
		}
		fflush(fout);

		next = pc + numBytes;
	}

	emitSectionEnd(fout);
	fclose(fout);

	return 0;
}