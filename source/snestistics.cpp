#include <stdint.h>
#include <set>
#include <vector>
#include <string>
#include <cassert>
#include <fstream>
#include <map>
#include "cmdoptions.h"
#include "utils.h"
#include "romaccessor.h"
#include "asmwrite_wladx.h"
#include "annotationresolver.h"
#include "cputable.h"
#include <iostream>
#include <iomanip>

/*
	IDEA for better prediction.
		Start prediction from all ops recorded in the emulation. [processor status, databank, direct page known]
		Start emulation from RESET and VBLANK.					 [in reset, processor status is known]
		Start emulation from all annotated labels
			First time a label is predicted, we might not know much.
			Then we might fall into it from a previous label and more is known.
			If someone calls it we can guess at acc16, ind16. We don't need perfect, we just need to interpret the correct ops.

		Track all registers and entire memory (!) while running through a function.
			Emulate all ops.
			Read from ROM are deterministic.
			Read from RAM that was previously written in same label is known.
			etc

		If program writes jump adress to RAM and then performs indirect jumps, it is deterministic!

		Multiple passes when new information is available.

		The idea is that the need for .trace-file diminshes as user annotations increases.
		So while some work is redundant at first, it will later enable generation without tracefile [only annotations!].

		Remember. If deterministic code READ a byte from ROM it probably means that it is not code but rather DATA.
		That can be used to discredit predicted code.

		So the real hard part is stuff such as

			LDX RAM_SOMETHING
			JSL [TABLE, X]

		We just don't know the range for X without actually running the game, and that requires user interaction.
	
*/

/*
	QUESTION: Add support for 6502-emulation-flag just to not mess that up?
*/

struct OpInfo {
	uint16_t P; // status
	uint8_t DB; // data bank
	uint16_t D; // direct page
	const bool operator<(const OpInfo &other) const {
		if (P != other.P) return P < other.P;
		if (DB != other.DB) return DB < other.DB;
		return D < other.D;
	}
};

/*
	This functions currently performs some deterministic jumps to make sure we record the destination.
	More prediction ("emulation") is of course possible.
*/
void predictOps(const RomAccessor &rom, std::map<Pointer, std::set<OpInfo>> &ops, LargeBitfield &labels, std::map<Pointer, std::set<Pointer>> &takenJumps) {
	for (auto opsIt = ops.begin(); opsIt != ops.end(); ++opsIt) {
		const Pointer pc = opsIt->first;
		const uint8_t* data = rom.evalPtr(pc);

		const uint8_t opcode = data[0];

		if ((branches[opcode]||jumps[opcode]) && takenJumps.find(pc) == takenJumps.end()) {
			Registers reg;
			reg.P = opsIt->second.begin()->P; // TODO: Just set bits acc16 and mem16
			reg.pc = pc & 0xFFFF;
			reg.pb = pc >> 16;
			MagicByte rb(0);
			MagicWord ra(0);
			ResultType r = evaluateOp(data, reg, &rb, &ra);

			if (r == SA_ADRESS && rb.isKnown() && ra.isKnown()) {
				const Pointer target((rb.value << 16 ) | (ra.value));
				labels.setBit(target);
				takenJumps[pc].insert(target);
			}
		}
	}
}

void parseTraceFile(const std::string &filename, std::map<Pointer, std::set<OpInfo>> &ops, LargeBitfield &labels, std::map<Pointer, std::set<Pointer>> &takenJumps) {
	// We can assume that all PC are saved in order
	FILE *f2 = fopen(filename.c_str(), "rb");
	if (!f2) {
		std::stringstream ss;
		ss << "Could not open trace-file '" << filename << "'";
		throw std::runtime_error(ss.str());
	}

	const uint16_t expectedVersionNumber = 0x0001;
	uint16_t versionNumber;
	fread(&versionNumber, sizeof(versionNumber), 1, f2);

	if (versionNumber != expectedVersionNumber) {
		std::stringstream ss;
		ss << "Wrong version number " << versionNumber << ", expected " << expectedVersionNumber;
		throw std::runtime_error(ss.str());
	}

	// First pass: Parse file, find all opcode starts and their status.
	while (!feof(f2)) {
		uint8_t code;

		fread(&code, 1, 1, f2);
		if (code == 0) {
			uint32_t pc;
			OpInfo o;
			fread(&pc, sizeof(Pointer), 1, f2);
			fread(&o.P, 2, 1, f2);
			fread(&o.DB, 1, 1, f2);
			fread(&o.D, 2, 1, f2);
			ops[Pointer(pc)].insert(o);
		}
		else if (code == 1) {
			Pointer p, t;
			fread(&p, sizeof(Pointer), 1, f2);
			fread(&t, sizeof(Pointer), 1, f2);
			labels.setBit(t);
			takenJumps[p].insert(t);
		}
		else {
			assert(false);
		}
	}

	fclose(f2);
}

int main(const int argc, const char * const argv[]) {
	try {

		initLookupTables();

		Options options;
		parseOptions(argc, argv, options);

		RomAccessor romData(options.romOffset, options.calculatedSize);
		romData.load(options.romFile);

		AnnotationResolver hardwareAnnotations;
		hardwareAnnotations.load(options.hardwareAnnotationsFile);

		AnnotationResolver annotations(&hardwareAnnotations);
		annotations.load(options.labelsFile);
		annotations.save(options.labelsFile + ".resave");
		
		std::map<Pointer, std::set<Pointer>> takenJumps;
		std::map<Pointer, std::set<OpInfo>> ops;
		LargeBitfield labels;
		labels.setBit(0x008000); // TODO: Get from ROM-header? Also the others like NMI...

		parseTraceFile(options.traceFile, ops, labels, takenJumps);

		predictOps(romData, ops, labels, takenJumps);

		AsmWriteWLADX writer(options, romData);


		// Generate EQU for each label
		// TODO: Only include USED labels
		for (Pointer p = 0; p < 128 * 65536; p++) {
			const Annotation *a = annotations.resolve(p, p, true, false);
			if (a != nullptr && a->type == ANNOTATION_DATA) {
				const size_t numBytes = a->endOfRange - a->startOfRange + 1;

				// NOTE: We only use 16-bit here... the high byte is almost never used in an op
				// TODO: Validate so this doesn't mess thing up when we use it
				char target[512];
				sprintf(target, "$%04X", a->startOfRange&0xFFFF);

				writer.writeDefine(a->name, target, a->description.empty() ? a->useComment : a->description);
			}
		}

		Pointer nextOp(INVALID_POINTER);

		// Now iterate the ops
		for (auto opsit = ops.begin(); opsit != ops.end(); ++opsit) {
			const Pointer pc = opsit->first;

			if (nextOp == INVALID_POINTER) {
				nextOp = pc;
			}

			// TODO: Emit any labels that we might have skipped as well as label for current line
			//       This is mostly if the static jump predictor is predicting a jump into non-ops.
			for (Pointer p(nextOp); p <= pc; ++p) {
				if (labels[p]) {
					// Emit label?
					std::string description;
					std::string labelName = annotations.getLabelName(p, &description, nullptr);
					writer.writeLabel(p, labelName, description);
				}
			}

			const std::set<OpInfo> &variants = opsit->second;
			assert(!variants.empty());


			const uint8_t *data = romData.evalPtr(pc);

			// Determine number of bytes needed for this instruction
			// Here I've assumed that either these flags agree for ALL variants, or that they are irrelevant for this opcode
			const bool acc16 = (variants.begin()->P & STATUS_ACCUMULATOR_FLAG) == 0;
			const bool ind16 = (variants.begin()->P & STATUS_MEMORY_FLAG) == 0;
			const bool emu   = (variants.begin()->P & STATUS_EMULATION_FLAG) != 0;
			
			char target[128] = "\0";
			char targetLabel[128] = "\0";
			int numBitsNeeded = 8;
			const size_t numBytesNeeded = calculateFormattingandSize(data, acc16, ind16, emu, target, targetLabel, &numBitsNeeded);

			nextOp = pc + numBytesNeeded;

			std::map<Pointer, std::pair<std::string, std::string>> proposals;

			auto tji = takenJumps.find(pc);
			if (tji != takenJumps.end()) {

				for (auto jit = tji->second.begin(); jit != tji->second.end(); ++jit) {
					std::string useComment;
					const std::string labelname = annotations.getLabelName(*jit, nullptr, &useComment);
					// TODO: Add use comment
					proposals[*jit] = std::make_pair(labelname, useComment);
				}
			}
			
			for (auto vit = variants.begin(); vit != variants.end(); ++vit) {
				Registers reg;
				reg.P = vit->P;
				reg.dp = vit->D;
				reg.db = vit->DB;
				reg.pc = pc & 0xFFFF;
				reg.pb = pc >> 16;
				// TODO: Make this more robust, listing options by X and Y etc...
				reg.reg_X = 0; // Pretend that these are known and zero... that way we can get label/data namn for base adr
				reg.reg_Y = 0; // Pretend that these are known and zero
				MagicByte rb(0);
				MagicWord ra(0);
				const ResultType r = evaluateOp(data, reg, &rb, &ra);

				if (r == SA_ADRESS && rb.isKnown() && ra.isKnown()) {
					const Pointer p((rb.value << 16) | ra.value);
					const Annotation *ann = annotations.resolve(pc, p);

					if (ann && proposals.find(p) == proposals.end()) {
						proposals[p] = std::make_pair(ann->name, ann->useComment);
					}
				}
			}

			if (proposals.size() == 1) {

				// All variant agree that this is the only choice
				// It might not be possible to codeify in instruction though since it might generate the wrong opcode

				const bool nameInCode = strcmp(targetLabel, "") != 0;

				std::stringstream ss;

				if (!nameInCode) {
					ss << proposals.begin()->second.first; // Name
				}

				const std::string lineComment = annotations.resolveLineComment(pc);
				const std::string useComment = proposals.begin()->second.second;

				if (lineComment.empty() && !useComment.empty()) {
					if (!nameInCode) {
						ss << " - ";
					}
					ss << useComment;
				}				
				if (!lineComment.empty()) {
					if (!nameInCode) {
						ss << " - ";
					}
					ss << lineComment;
				}

				//const Pointer targetAdr = proposals.begin()->first;
				//ss << " [" << std::setfill('0') << std::setw(6) << std::hex << targetAdr << "]";

				if (!nameInCode) {
					writer.writeInstruction(pc, numBitsNeeded, numBytesNeeded, target, ss.str());
				} else {
					char wow[128];
					sprintf(wow, targetLabel, proposals.begin()->second.first.c_str());
					writer.writeInstruction(pc, numBitsNeeded, numBytesNeeded, wow, ss.str());
				}
			} else {

				writer.writeInstruction(pc, numBitsNeeded, numBytesNeeded, target, annotations.resolveLineComment(pc));
				for (auto pit = proposals.begin(); pit != proposals.end(); ++pit) {
					std::stringstream ss;
					ss << "\t\t;" << pit->second.first << "\t\t" << pit->second.second;
					writer.writeComment(pc, ss.str());
				}

			}
		}
	} catch (const std::runtime_error &e) {
		printf("error: %s\n", e.what());
		return 1;
	}

	return 0;
}