#ifndef SNESTISTICS_ASMWRITE_WLADX
#define SNESTISTICS_ASMWRITE_WLADX

#include "utils.h"
#include "cmdoptions.h"
#include <sstream>
#include "romaccessor.h"
#include "cputable.h"

class AsmWriteWLADX {
private:
	const Options &m_options;
	FILE* m_outFile;
	const RomAccessor &m_romData;
	Pointer m_nextPC;
	bool m_bankOpen;
	int m_sectionCounter;
public:
	AsmWriteWLADX(const Options &options, const RomAccessor &romData) : m_options(options), m_outFile(nullptr), m_romData(romData), m_nextPC(INVALID_POINTER) {}
	~AsmWriteWLADX() {
		// TODO: Write footer I guess
		if (m_outFile) {
			if (m_bankOpen) {
				emitBankEnd();
			}
			fclose(m_outFile);
		}
	}

	void writeDefine(const std::string &thing, const std::string &value, const std::string &description) {
		prepareOpenFile();

		if (!description.empty()) {
			fprintf(m_outFile, "; ");
		}
		for (auto it = description.begin(); it != description.end(); ++it) {
			if (*it == '\n') {
				fprintf(m_outFile, "\n; ");
			}
			else if (*it != '\r') {
				fprintf(m_outFile, "%c", *it);
			}
		}
		if (!description.empty()) {
			fprintf(m_outFile, "\n");
		}
		fprintf(m_outFile, ".EQU %s %s\n", thing.c_str(), value.c_str());
	}

	void writeComment(const Pointer pc, const std::string &comment) {
		prepareWrite(pc, false);
		fprintf(m_outFile, "%s\n", comment.c_str());
	}
	
	void writeLabel(const Pointer pc, const std::string &labelName, const std::string &comment="") {
		prepareWrite(pc);
		fprintf(m_outFile, "\n");
		if (!comment.empty()) {
			writeComment(pc, comment);
		}
		fprintf(m_outFile, "%s:\n", labelName.c_str());
	}

	void writeInstruction(const Pointer pc, const int numBits, const int numBytesUsed, const std::string &param, std::string &lineComment) {
		prepareWrite(pc);

		const uint8_t* data = m_romData.evalPtr(pc);

		bool overrideInstructionWithDB = false;

		if (numBytesUsed == 2 && data[0] == 0xF0 && data[1] == 0x80) {
			// WLA DX has a bug here, emit using .DB instad
			overrideInstructionWithDB = true;
		}

		// Keep track of bytes written so we can align comment column
		int nw = 0;

		const bool emitCommentPC = m_options.printProgramCounter || m_options.printHexOpcode || overrideInstructionWithDB;

		nw += fprintf(m_outFile, "\t");
		if (emitCommentPC) {
			nw += fprintf(m_outFile, "/* ");
		}
		if (m_options.printProgramCounter) {
			nw += fprintf(m_outFile, "%06X ", pc);
		}
		if (m_options.printHexOpcode) {
			for (int k = 0; k < numBytesUsed; k++) {
				nw += fprintf(m_outFile, "%02X ", data[k]);
			}
			for (int k = numBytesUsed; k < 4; k++) {
				nw += fprintf(m_outFile, "   ");
			}
		}
		if (emitCommentPC && !overrideInstructionWithDB) {
			nw += fprintf(m_outFile, "*/ ");
		}

		const char * const op = opCodeInfo[data[0]].mnemonics;
		if (m_options.lowerCaseOpCode) {
			nw += fprintf(m_outFile, "%c%c%c", tolower(op[0]), tolower(op[1]), tolower(op[2]));
		} else {
			nw += fprintf(m_outFile, "%s", op);
		}

		if (strcmp(opCodeInfo[data[0]].mnemonics, "BRL") == 0) {

		} else if (numBits == 0) {
		} else if (numBits == 8) {
			nw += fprintf(m_outFile, ".B");
		} else if (numBits == 16) {
			nw += fprintf(m_outFile, ".W");
		} else if (numBits == 24) {
			nw += fprintf(m_outFile, ".L");
		} else {
			assert(false);
		}


		if (!param.empty()) {
			nw += fprintf(m_outFile, " %s", param.c_str());
		}

		if (overrideInstructionWithDB) {
			nw += fprintf(m_outFile, "*/ ");
		}

		int nwtarget = 56;
		while (nw > nwtarget) {
			nwtarget += 8;
		}

		if (!lineComment.empty()) {
			while (nw < nwtarget) {
				nw += fprintf(m_outFile, " ");
			}
			fprintf(m_outFile, "; %s", lineComment.c_str());
		}

		fprintf(m_outFile, "\n");

		if (overrideInstructionWithDB) {
			fprintf(m_outFile, ".DB ");
			for (int k = 0; k < numBytesUsed; k++) {
				fprintf(m_outFile, "$%02X%s", data[k], k != numBytesUsed-1 ? ", ":"");
			}
			fprintf(m_outFile, "\n");
		}


		m_nextPC = pc + numBytesUsed;
	}
private:

	void emitBankStart(const Pointer pc) {
		fprintf(m_outFile, "\n");
		fprintf(m_outFile, ".BANK $%02X SLOT 0\n", pc >> 16);
		fprintf(m_outFile, ".ORG $%04X-$8000\n", pc & 0xffff);
		fprintf(m_outFile, ".SECTION SnestisticsSection%d OVERWRITE\n", m_sectionCounter);
		m_sectionCounter++;
	}

	void emitBankEnd() {
		fprintf(m_outFile, "\n.ENDS\n\n");
	}

	void prepareOpenFile() {
		if (m_outFile == nullptr) {
			m_bankOpen = false;
			m_sectionCounter = 0;
			m_outFile = fopen(m_options.outFile.c_str(), "wb");
			if (m_outFile == nullptr) {
				std::stringstream ss;
				ss << "Could not open output file '" << m_options.outFile << "'";
				throw std::runtime_error(ss.str());
			}

			if (!m_options.asmHeaderFile.empty()) {
				std::vector<unsigned char> header;
				readFile(m_options.asmHeaderFile, header);
				fwrite(&header[0], header.size(), 1, m_outFile);
			}
		}
	}

	void prepareWrite(const Pointer pc, const bool advancePC=true) {
		prepareOpenFile();
		if (m_nextPC == INVALID_POINTER){
			m_nextPC = pc;
		}

		// TODO: Take care of banks and filling "holes" between instructions
		if (!m_bankOpen) {
			emitBankStart(pc);
			m_bankOpen = true;
		} else if (m_nextPC != pc && advancePC) {

			const size_t holeSize = pc - m_nextPC;

			if (holeSize > 1024 || ((m_nextPC >> 16) != (pc >> 16))) {
				emitBankEnd();

				fprintf(m_outFile, "\n; %d bytes gap (0x%X)\n\n", holeSize, holeSize);

				emitBankStart(pc);
			}
			else {
				Pointer p = m_nextPC;
				fprintf(m_outFile, "\n");
				while (p<pc) {
					fprintf(m_outFile, ".DB ");
					for (int x = 0; x<32 && p<pc; x++, ++p) {
						fprintf(m_outFile, "$%02X", m_romData.evalByte(p));

						if (x != 32 - 1 && p != pc - 1) {
							fprintf(m_outFile, ",");
						}
					}
					fprintf(m_outFile, "\n");
				}
				fprintf(m_outFile, "\n");
			}

			m_nextPC = pc;
		}
	}
};

#endif // SNESTISTICS_ASMWRITE_WLADX
