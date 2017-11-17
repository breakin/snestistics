#ifndef SNESTISTICS_ASMWRITE_WLADX
#define SNESTISTICS_ASMWRITE_WLADX

#include "utils.h"
#include "options.h"
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

	inline int adjusted_column(const int s) {
		int target = 72;
		while (s > target) target += 4;
		return target;
	}

	inline int indent_column(FILE *f, int target, int pos, bool comment = false) {
		static const char spaces[160]="                                                                                                                                                               ";
		assert(target >= pos);
		int wanted_spaces = target-pos;
		assert(wanted_spaces <= 159);
		fwrite(spaces, 1, wanted_spaces, f);
		if (comment) {
			wanted_spaces += fprintf(f, "; ");
		}
		return pos + wanted_spaces;
	}

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
		int nw = fprintf(m_outFile, ".EQU %s %s", thing.c_str(), value.c_str());
		writeCommentHelper(nw, adjusted_column(nw), description);
	}

	void writeCommentHelper(const int start_pos, const int target_column, const std::string &comment) {

		if (comment.empty()) {
			fputc('\n', m_outFile);
			return;
		}

		int nw = start_pos;

		bool line_start = true;
		for (size_t i=0; i<comment.length(); ++i) {
			if (line_start) {
				nw = indent_column(m_outFile, target_column, nw, true);
				line_start = false;
			}
			char c = comment[i];
			fputc(c, m_outFile);
			if (c == '\n') {
				line_start = true;
				nw = 0;
			}
		}
		fputc('\n', m_outFile);
	}

	void writeComment(const Pointer pc, const std::string &comment, const int start_pos = 0, const int target_column = 0) {
		prepareWrite(pc, false);
		writeCommentHelper(start_pos, target_column, comment);
	}

	void writeComment(const std::string &comment, const int start_pos = 0, const int target_column = 0) {
		prepareOpenFile();
		writeCommentHelper(start_pos, target_column, comment);
	}

	void writeComment(StringBuilder &sb) {
		writeComment(sb.c_str());
		sb.clear();
	}

	void writeSeperator(const char * const text) {
		prepareOpenFile();
		fprintf(m_outFile, "\n");
		fprintf(m_outFile, "; =====================================================================================================\n");
		fprintf(m_outFile, "; %s\n", text);
		fprintf(m_outFile, "; =====================================================================================================\n");
	}

	void writeLabel(const Pointer pc, const std::string &labelName, const std::string &description, const std::string &comment) {
		prepareWrite(pc);
		int nw = 0;
		fprintf(m_outFile, "\n");
		if (!description.empty())
			writeCommentHelper(0, 0, description);
		nw = fprintf(m_outFile, "%s:", labelName.c_str());
		if (!comment.empty()) {
			nw = indent_column(m_outFile, adjusted_column(nw), nw);
			fprintf(m_outFile, "; %s", comment.c_str());
			// TODO: Support multiline
		}			
		fprintf(m_outFile, "\n");
	}

	void writeInstruction(const Pointer pc, const int numBits, const int numBytesUsed, const std::string &param, const std::string &line_comment, CombinationBool accumulator_wide, CombinationBool index_wide, Combination8 data_bank, Combination16 direct_page, bool is_predicted) {
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

		nw += fprintf(m_outFile, "    ");
		if (emitCommentPC) {
			nw += fprintf(m_outFile, "/*%c", is_predicted ? 'p':' ');
		}

		if (m_options.printRegisterSizes) {
			if(!accumulator_wide.has_value) {
				fputc('?', m_outFile);
			} else if (accumulator_wide.single_value) {
				if (accumulator_wide.value) {
					fputc('M', m_outFile);
				} else {
					fputc('m', m_outFile);
				}
			} else {
				fputc('*', m_outFile);
			}
			if(!index_wide.has_value) {
				fputc('?', m_outFile);
			} else if (index_wide.single_value) {
				if (index_wide.value) {
					fputc('I', m_outFile);
				} else {
					fputc('i', m_outFile);
				}
			} else {
				fputc('*', m_outFile);
			}
			fputc(' ', m_outFile);
			nw += 3;
		}

		if (m_options.printDB) {
			assert(data_bank.has_value);
			if (data_bank.single_value) {
				nw += fprintf(m_outFile, "%02X ", data_bank.value);
			} else {
				nw += fprintf(m_outFile, "** ");
			}
		}

		if (m_options.printDP) {
			assert(direct_page.has_value);
			if (direct_page.single_value) {
				nw += fprintf(m_outFile, "%04X ", direct_page.value);
			} else {
				nw += fprintf(m_outFile, "**** ");
			}
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

		writeCommentHelper(nw, adjusted_column(nw), line_comment);

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
			m_outFile = fopen(m_options.asm_file.c_str(), "wb");
			if (m_outFile == nullptr) {
				std::stringstream ss;
				ss << "Could not open output file '" << m_options.asm_file << "'";
				throw std::runtime_error(ss.str());
			}

			if (!m_options.asm_header_file.empty()) {
				std::vector<unsigned char> header;
				readFile(m_options.asm_header_file, header);
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

			const uint32_t holeSize = pc - m_nextPC;

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
