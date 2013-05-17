#include <stdint.h>
#include <set>
#include <vector>
#include <string>
#include <cassert>
#include <map>
#include "snesops.h"

void fillLabels(std::map<Pointer, std::string> &labels) {
	
	// Fill in our nice labels
	labels[Pointer(0x008000)] = "entry";
	labels[Pointer(0x00E766)] = "decompress_tableA_dest7f4000_indexY";
	labels[Pointer(0x00E75C)] = "decompress_tableA_dest7f4600_indexY";
    labels[Pointer(0x00E76E)] =	"decompress_tableA_nodest_indexY",
	labels[Pointer(0x00E783)] = "decompressTableB_indexY";
	labels[Pointer(0x00E33B)] = "decompressTableA_AndCopy2048_2118";
	labels[Pointer(0x00E33E)] = "copy2048_to_vram_2118";
	labels[Pointer(0x00E783)] = "decompressTableB_indexY";
	labels[Pointer(0x00E79E)] = "decompress";
	labels[Pointer(0x00E7A3)] = "decompress_main";
	labels[Pointer(0x00E7AD)] = "decompress_getcodelen";
	labels[Pointer(0x00E7BF)] = "decompress_getlongcount";
	labels[Pointer(0x00E7D1)] = "decompress_notlongcount";
	labels[Pointer(0x00E7E1)] = "repeat_single_incrementing";
	labels[Pointer(0x00E7EF)] = "decompress_transfer_source";
	labels[Pointer(0x00E7FE)] = "decompress_repeat_single";
	labels[Pointer(0x00E80B)] = "decompress_repeat_alt";
	labels[Pointer(0x00E825)] = "decompress_transfer_dest";
	labels[Pointer(0x00E843)] = "decompress_readsourcebyte";
	labels[Pointer(0x00D54E)] = "decompress_and_unpack_image_tableA";
	labels[Pointer(0x00D553)] = "unpack_image_source7f4000";
	labels[Pointer(0x00D561)] = "unpack_image";
	labels[Pointer(0x00D5CE)] = "unpack_image2_inner";
	labels[Pointer(0x00D61C)] = "unpack_image_inner";
    
    // Things under investigation
    labels[Pointer(0x00D231)] = "load_stuff_1";
}

void emitFileStart(FILE *fout) {
	fprintf(fout, ".MEMORYMAP\n");
	fprintf(fout, "  SLOTSIZE $8000\n");
	fprintf(fout, "  DEFAULTSLOT 0\n");
	fprintf(fout, "  SLOT 0 $8000\n");
	fprintf(fout, ".ENDME\n");
	fprintf(fout, "\n");
	fprintf(fout, ".ROMBANKSIZE $8000\n");
	fprintf(fout, ".ROMBANKS 32\n");
	fprintf(fout, ".BACKGROUND \"zelda3org.sfc\"\n");
	fprintf(fout, ".EMPTYFILL $00\n");
	fprintf(fout, "\n");
}
void emitSectionStart(FILE *fout, const Pointer pc, int *sectionCounter) {
	fprintf(fout, ".BANK $%02X SLOT 0\n", pc>>16);
	fprintf(fout, ".ORG $%04X-$8000\n", pc&0xffff);
	fprintf(fout, ".SECTION RandomSection%d OVERWRITE\n", *sectionCounter);
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

void visitOp(const Pointer pc, const uint16_t ps, const bool predict, const std::set<Pointer> &knownOps, std::set<Pointer> &predictOps, std::map<Pointer, std::string> &labels, std::vector<uint16_t> &statusRegArray, const std::vector<uint8_t> &romdata, const size_t calculatedSize) {
    
    const uint32_t romAdr = 512 + getRomOffset(pc, calculatedSize); // +512 due to SMB-header
    const uint8_t ih = romdata[romAdr];
        
    char pretty[16];
    char labelPretty[16];
    Pointer jumpDest(0);
    const int nb = processArg(pc, ih, &romdata[romAdr], pretty, labelPretty, &jumpDest, ps, 0);

	// Make sure this op is not "inside" a valid op
	for (int k=1; k<nb; k++) {
		if (knownOps.find(pc+k) != knownOps.end()) {
			return;
		}
	}
        
    if (jumps[ih]) {
        if (jumpDest != 0) {
            labels[jumpDest] = getLabelName(jumpDest);
        }
    }

    if (predict) {
        predictOps.insert(pc);
        statusRegArray[pc]=ps;
    }
    
    const char * const op = S9xMnemonics[ih];
    
    // If this happens, we should not predict the next instruction. Could be data!
    if (strcmp(op, "JMP")==0) { return; }
    if (strcmp(op, "BRA")==0) { return; }
    if (strcmp(op, "RTS")==0) { return; }
    if (strcmp(op, "RTI")==0) { return; }
    if (strcmp(op, "RTL")==0) { return; }
    
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
        visitOp(next, next_ps, true, knownOps, predictOps, labels, statusRegArray, romdata, calculatedSize);
    }
 }

int main() {

	initLookupTables();

	const std::string romFile("zelda3org.smc");
	FILE *f = fopen(romFile.c_str(), "rb");
	if (f==0) {
		printf("Could not open ROM-file '%s'\n", romFile.c_str());
		return 1;
	}
	fseek(f, 0, SEEK_END);
	const int fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	std::vector<uint8_t> romdata(fileSize);
	fread(&romdata[0], 1, fileSize, f);
	fclose(f);

	const int calculatedSize = 8*0x20000;

	// We can assume that all PC are saved in order
	FILE *f2 = fopen("zelda3.snestrace", "rb");

	std::set<Pointer> ops;
	std::map<Pointer, std::string> labels;
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
			
			labels[t]=getLabelName(t);

		} else {
			assert(false);
		}
	}

	fclose(f2);
    
    std::set<Pointer> predictOps;
    
	// Determine all labels that we need for the determinisic jumps
	for (auto it = begin(ops); it != end(ops); ++it) {
        visitOp(*it, opStatus[*it], false, ops, predictOps, labels, opStatus, romdata, calculatedSize);
	}

    // Make all predicted ops real ops
    for (auto it = begin(predictOps); it != end(predictOps); ++it) {
        ops.insert(*it);
    }
    
	// TODO: Add in non-deterministic jumps that we could put in comments.. paths taken

	// Overwrite some of the predetermined labels with user-defined friendly names
	// TODO: Make sure they are unique?
	fillLabels(labels);

	FILE *fout = fopen("zelda3.gen.asm", "wt");
	assert(fout);

	emitFileStart(fout);

	Pointer next(0);

	int sectionCounter=0;

	int numBits = 8;

	bool firstSection = true;

	for (auto it = begin(ops); it != end(ops); ++it) {
		const Pointer pc = *it;
		const uint32_t romAdr = 512 + getRomOffset(pc, calculatedSize); // +512 due to SMB-header
		const uint8_t ih = romdata[romAdr];
        
		if (pc != next) {

			const int gap = pc - next;

			// TODO: Show larger chunks of data, 64x64 matrix

			if (gap>64) {

				if (!firstSection) {
					emitSectionEnd(fout);
				}
				firstSection = false;
                
				// If this is violated, then we most likely predicted wrongly...
				// Perhaps it wasn't code after all?
				//assert(pc>next);
				

				fprintf(fout, "\n; %d bytes gap\n\n", gap);

				emitSectionStart(fout,pc,&sectionCounter);
			} else {

				fprintf(fout, "\n");
				fprintf(fout, ".DB ");
				for (Pointer p = next; p<pc; p++) {
					const uint32_t pr = 512 + getRomOffset(p, calculatedSize); // +512 due to SMB-header
					if (p !=next) {
						fprintf(fout, ",");
					}
					fprintf(fout, "$%02X", romdata[pr]);
				}
				fprintf(fout, "\n");
				next = pc;
			}
		}

		auto labelIt = labels.find(pc);
		if (labelIt != labels.end()) {
			fprintf(fout, "\n%s:\n\n", labelIt->second.c_str());
		}

		Pointer jumpDest(0);
		char pretty[16]="\0";
		char labelPretty[16]="\0";

		// TODO: Hand in processor status register
		int needBits = numBits; // Unless processArg cares, let it stay!
		const int numBytes = processArg(pc, ih, &romdata[romAdr], pretty, labelPretty, &jumpDest, opStatus[pc], &needBits);

		if (needBits != numBits) {
			if (needBits== 8) { fprintf(fout, ".8bit\n"); }
			if (needBits==16) { fprintf(fout, ".16bit\n"); }
			if (needBits==24) { fprintf(fout, ".24bit\n"); }
			numBits = needBits;
		}

		fprintf(fout, "\t/* %06X " , pc);
        
		for (int k=0; k<numBytes; k++) {
			fprintf(fout, "%02X ", romdata[romAdr+k]);
		}
		for (int k=0; k<4-numBytes; k++) {
			fprintf(fout, "   ");
		}

		fprintf(fout, "*/ ");

		if (false && jumps[ih] && jumpDest != 0 && ops.find(jumpDest) != ops.end()) {
			fprintf(fout, "%s ", S9xMnemonics[ih]);
			fprintf(fout, labelPretty, labels[jumpDest].c_str());
			fprintf(fout, "");
		} else {
			fprintf(fout, "%s %s", S9xMnemonics[ih], pretty);
		}
        
		const bool predicted = predictOps.find(pc) != predictOps.end();
		bool emitPred = false;
		bool emitN = false;

		if (jumps[ih] && !jumpDest) {
			auto recordedJumps = takenJumps.find(pc);
			if (recordedJumps != takenJumps.end()) {
				for (auto pit = recordedJumps->second.begin(); pit != recordedJumps->second.end(); ++pit) {
					const Pointer p = *pit;
					if (false && ops.find(p) != ops.end() && labels.find(p) != labels.end()) {
						fprintf(fout, " ; %s [%06X]", labels[p].c_str(), p);
					} else {
						fprintf(fout, " ; %06X", p);
					}					
					if (!emitPred && predicted) {
						emitPred = true;
						fprintf(fout, " [predicted]");
					}
					emitN = true;
					fprintf(fout, "\n");
				}
			}
		}
		if (predicted && !emitPred) {
			fprintf(fout, " ; predicted");
		}

		if (!emitN) {
			fprintf(fout, "\n");
		}

        // TODO: Looks weird if a label comes directly after
        if (strcmp(S9xMnemonics[ih], "RTS")==0) { fprintf(fout, "\n"); }
        if (strcmp(S9xMnemonics[ih], "RTL")==0) { fprintf(fout, "\n");}
        if (strcmp(S9xMnemonics[ih], "RTI")==0) { fprintf(fout, "\n"); }
		fflush(fout);

		next = pc + numBytes;
	}

	emitSectionEnd(fout);
	fclose(fout);

	return 0;
}