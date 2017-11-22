#include "utils.h"
#include "options.h"
#include <sstream>
#include "romaccessor.h"
#include "cputable.h"
#include "asm_writer.h"
#include "report_writer.h"
#include <algorithm>
#include "annotations.h"
#include "romaccessor.h"
#include "trace.h"

/*
	TODO: This look of this file is partially to blame for me wanting multiple asm output languages.
	      It needs to be cleaned up a lot! Move functions over to report_writer and remove the AsmWriteWLADX class.
*/

namespace {

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

	ReportWriter &m_report;

public:
	AsmWriteWLADX(ReportWriter &report, const Options &options, const RomAccessor &romData) : m_options(options), m_report(report), m_romData(romData), m_nextPC(INVALID_POINTER), m_outFile(nullptr) {
	}
	~AsmWriteWLADX() {
		// TODO: Write footer I guess
		if (m_outFile) {
			if (m_bankOpen) {
				emitBankEnd();
			}
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
		m_report.writeComment(sb);
	}

	void writeSeperator(const char * const text) {
		prepareOpenFile();
		m_report.writeSeperator(text);
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

	bool m_first_prepare = true;

	void prepareOpenFile() {
		if (m_first_prepare) {
			m_first_prepare = false;
			m_bankOpen = false;
			m_sectionCounter = 0;
			m_outFile = m_report.report;
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
}

void asm_writer(ReportWriter &report, const Options &options, Trace &trace, const AnnotationResolver &annotations, const RomAccessor &rom_accessor) {
	AsmWriteWLADX writer(report, options, rom_accessor);

	writer.writeSeperator("Data");

	// Generate EQU for each label
	// TODO: Only include USED labels?
	for (const Annotation &a : annotations._annotations) {
		if (a.type == ANNOTATION_FUNCTION || (a.type == ANNOTATION_LINE && !a.name.empty()))
			trace.labels.setBit(a.startOfRange);
		if (a.type != ANNOTATION_DATA)
			continue;

		// NOTE: We only use 16-bit here... the high byte is almost never used in an op
		// TODO: Validate so this doesn't mess thing up when we use it
		char target[512];
		sprintf(target, "$%04X", a.startOfRange&0xFFFF);

		writer.writeDefine(a.name, target, a.comment.empty() ? a.useComment : a.comment);
	}

	writer.writeSeperator("Code");

	Pointer nextOp(INVALID_POINTER);

	std::vector<OpInfo> variants;
	variants.reserve(1024*64);

	// Now iterate the ops
	for (auto opsit = trace.ops.begin(); opsit != trace.ops.end(); ++opsit) {
		const Pointer pc = opsit->first;

		if (nextOp == INVALID_POINTER) {
			nextOp = pc;
		}

		bool emitted_label = false;

		// TODO: Emit any labels that we might have skipped as well as label for current line
		//       This is mostly if the static jump predictor is predicting a jump into non-ops.
		for (Pointer p(nextOp); p <= pc; ++p) {
			if (trace.labels[p]) {

				// Emit label?
				std::string label, line_comment, line_use_comment;

				annotations.line_info(p, &label, &line_comment, &line_use_comment, true);

				std::string description = line_comment;
				if (line_comment.empty()) {
					description = line_use_comment;
				}

				bool shortmode = false;//!multiline && description.size() < 20;

				writer.writeLabel(p, label, shortmode ? "" : description, shortmode ? description : "");

				if (p == pc)
					emitted_label = true;
			}
		}

		const Trace::OpVariantLookup variant_lookup = opsit->second;
		assert(variant_lookup.count != 0);

		const uint8_t *data = rom_accessor.evalPtr(pc);
		const uint8_t opcode = *data;

		// Determine number of bytes needed for this instruction
		// Here I've assumed that either these flags agree for ALL variants, or that they are irrelevant for this opcode
		const OpInfo &first_variant = trace.variant(variant_lookup, 0);
		const bool acc16 = is_memory_accumulator_wide(first_variant.P);
		const bool ind16 = is_index_wide(first_variant.P);
		const bool emu   = is_emulation_mode(first_variant.P);
		const bool is_predicted = trace.is_predicted[pc];
		if (emu) printf("emu mode at pc %06X\n", pc);

		char target[128] = "\0";
		char target_label[128] = "\0";
		int numBitsNeeded = 8;

		const uint32_t numBytesNeeded = calculateFormattingandSize(data, acc16, ind16, target, target_label, &numBitsNeeded);

		nextOp = pc + numBytesNeeded;

		typedef std::string Proposal;
		typedef std::map<Pointer, Proposal> Proposals;
		Proposals proposals; // Given destionation/source give comment on how we got there

		const bool is_jump = jumps[opcode]; // TODO: Cover all jumps we want? And not too many?

		// Does adress depend on ....?
		bool is_indirect = cputable::is_indirect(opcode);
		const bool depend_X = cputable::address_depend_x(opcode);
		const bool depend_Y = cputable::address_depend_y(opcode);
		const bool depend_DB = cputable::address_depend_db(opcode);
		const bool depend_DP = is_jump ? false : cputable::address_depend_dp(opcode);

		// First copy all variants into our local vector where we can modify
		variants.resize(variant_lookup.count);
		memcpy(&variants[0], &trace.ops_variants[variant_lookup.offset], variant_lookup.count * sizeof(OpInfo));

		// These know if they have been assigned no value, one value (maybe multiple times) or multiple different values
		Combination8 shared_db;
		Combination16 shared_dp;
		CombinationBool accumulator_wide;
		CombinationBool index_wide;

		{
			// NOTE: While an op might depend on say DB it might not use it if adress is in $0000-$1999

			// Zero out unused registers so our sort get items in correct order
			for (OpInfo &o : variants) {
				accumulator_wide = is_memory_accumulator_wide(o.P);
				index_wide = is_index_wide(o.P);
				shared_dp = o.DP;
				shared_db = o.DB;
				if (!depend_X) o.X = 0;
				if (!depend_Y) o.Y = 0;
				if (!depend_DB) o.DB = 0;
				if (!depend_DP) o.DP = 0;
				if (!is_indirect) o.indirect_base_pointer = 0;
			}
		}

		if (branches[opcode]) {
			const Pointer branch_target = pc + (unpackSigned(data[1]) + 2);
			proposals[branch_target] = "";
		} else {
			// Sort and make variants unique
			std::sort(variants.begin(), variants.end());

			uint32_t num_unique_variants = 0;

			// Remove duplicates
			for (uint32_t source_idx = 0; source_idx < variants.size(); source_idx++) {
				if (num_unique_variants == 0 || variants[source_idx] != variants[num_unique_variants - 1]) {
					variants[num_unique_variants++] = variants[source_idx];
				}
			}
			variants.resize(num_unique_variants);

			if (is_jump) {
				for (uint32_t variant_idx = 0; variant_idx < variants.size(); ++variant_idx) {
					const OpInfo &vit = variants[variant_idx];
					if (vit.jump_target == INVALID_POINTER)
						continue;
					StringBuilder sb;
					sb.clear();
					if (depend_X)
						sb.format("[X=%04X]", vit.X);
					proposals[vit.jump_target] = sb.c_str();
				}

			} else if (!is_predicted) {

				for (uint32_t variant_idx = 0; variant_idx < variants.size(); ++variant_idx) {

					const OpInfo &vit = variants[variant_idx];

					StringBuilder ranges;
					if (depend_X||depend_Y) {
						if (depend_X) ranges.add("X=");
						if (depend_Y) ranges.add("Y=");

						int max_size = 80;
						int skipped = 0;

						bool first = true;

						Range r;
						r.add(depend_X?vit.X:vit.Y);
						for (uint32_t v = variant_idx+1; v < variants.size(); ++v) {
							const OpInfo &vi = variants[v];
							if (vi.DB != vit.DB || vi.DP != vit.DP)
								break;
							int value = depend_X ? variants[v].X : variants[v].Y;
							if (r.fits(value)) {
								r.add(value);
							} else {
								bool skip = ranges.length() >= max_size;
								if (!skip) {
									if (!first) ranges.add(", "); first = false;
									r.format(ranges);
								} else {
									skipped++;
								}
								r.reset();
								r.add(value);
							}
							variant_idx++;
						}
						if (skipped == 0) {
							if (!first) ranges.add(", "); first = false;
							r.format(ranges);
						} else {
							ranges.format(" and %d more", skipped);
						}
					}

					CPURegisters reg;
					reg.P = vit.P;
					reg.dp = vit.DP;
					reg.db = vit.DB;
					reg.pc = pc & 0xFFFF;
					reg.pb = pc >> 16;
					reg.reg_X = 0; // Start at 0 to get base pointer
					reg.reg_Y = 0; // Start at 0 to get base pointer
					MagicByte rb(0);
					MagicWord ra(0);
					ra.set_unknown();
					rb.set_unknown();
					assert(!ra.isKnown() && !rb.isKnown());
					const ResultType r = evaluateOp(rom_accessor, data, reg, &rb, &ra, nullptr, nullptr, nullptr, nullptr);

					if (r == SA_ADRESS && rb.isKnown() && ra.isKnown()) {
						const Pointer p = lorom_bank_remap((rb.value << 16) | ra.value);

						//if((p>>16)!=rb.value) // data bank was removed due to adressing being shuffled into 7E
						// depend_DB = false;

						if (proposals.find(p) != proposals.end())
							continue;

						std::string use_comment;

						bool has_remark = depend_DB || depend_DP || !ranges.empty();

						StringBuilder sb;
						if (!use_comment.empty()) {
							sb.add(use_comment);
							if (has_remark)
								sb.add(" ");
						}
						bool first = true;
						if (has_remark)
							sb.add("[");
						if (depend_DB) {
							if (!first) sb.add(" "); first = false;
							sb.format("DB=%X", vit.DB);
						}
						if (depend_DP) {
							if (!first) sb.add(" "); first = false;
							sb.format("DP=%X", vit.DP);
						}
						if (!ranges.empty()) {
							if (!first) sb.add(" "); first = false;
							sb.add(ranges.c_str());
						}
						if (has_remark)
							sb.add("]");

						proposals[p] = sb.c_str();
					}
				}
			}
		}

		std::string line_comment;
		if (!emitted_label) {
			annotations.line_info(pc, nullptr, &line_comment, nullptr, false);
		}

		auto variant_writer = [&annotations, &line_comment, is_jump](StringBuilder &ss, const Pointer target_pointer, const Proposal &p, bool name_in_code) {
			ss.clear();
			std::string use_comment, pointer_comment = p;
			const std::string name = annotations.label(target_pointer, &use_comment, is_jump);

			if (!name_in_code || name.empty()) {
				if (!name.empty()) {
					ss.format("%s ", name.c_str()); // Name
				} else {
					ss.format("%06X ", target_pointer);
				}
			}

			bool first = true;
			if (!line_comment.empty()) {
				if (!first) ss.add(" - "); first = false;
				ss.add(line_comment);
			}
			if (!use_comment.empty()) {
				if (!first) ss.add(" - "); first = false;
				ss.add(use_comment);
			}
			if (!pointer_comment.empty()) {
				if (!first) ss.add(" - "); first = false;
				ss.add(pointer_comment);
			}
			return name;
		};

		StringBuilder ss;

		if (proposals.size() == 1) {

			// All variant agree that this is the only choice
			// If we have a valid label we might be able to use that in the op-code
			// It might not be possible to codify it though since it might generate the wrong opcode
			// If target_label is non-empty we can codify using a label

			const bool name_in_code = strcmp(target_label, "") != 0;
			std::string name = variant_writer(ss, proposals.begin()->first, proposals.begin()->second, name_in_code);

			if (!name_in_code || name.empty()) {
				writer.writeInstruction(pc, numBitsNeeded, numBytesNeeded, target, ss.c_str(), accumulator_wide, index_wide, shared_db, shared_dp, is_predicted);
			} else {
				char wow[256];
				sprintf(wow, target_label, name.c_str());
				writer.writeInstruction(pc, numBitsNeeded, numBytesNeeded, wow, ss.c_str(), accumulator_wide, index_wide, shared_db, shared_dp, is_predicted);
			}
		} else {
			writer.writeInstruction(pc, numBitsNeeded, numBytesNeeded, target, line_comment, accumulator_wide, index_wide, shared_db, shared_dp, is_predicted);
			// List all possible targets in comments!
			for (auto pit = proposals.begin(); pit != proposals.end(); ++pit) {
				std::string name = variant_writer(ss, pit->first, pit->second, false);
				writer.writeComment(pc, ss.c_str(), 0, 10);
			}
		}

		if (is_indirect && !is_jump && !is_predicted) {
			for (uint32_t variant_idx = 0; variant_idx < variants.size(); ++variant_idx) {

				const OpInfo &v = variants[variant_idx];

				StringBuilder ranges;
					
				if (depend_X||depend_Y) {
					if (depend_X) ranges.add(" X=");
					if (depend_Y) ranges.add(" Y=");
					Range r;
					bool first = true;
					int skipped = 0;

					r.add(depend_X ? v.X : v.Y);

					for (uint32_t vi2 = variant_idx+1; vi2 < variants.size(); ++vi2) {
						const OpInfo &v2 = variants[vi2];
						if (v.DB != v2.DB) break;
						if (v.DP != v2.DP) break;
						if (v.indirect_base_pointer != v2.indirect_base_pointer) break;

						int value = depend_X ? v2.X : v2.Y;

						if (r.fits(value)) {
							r.add(value);
								
						} else {
							if (ranges.length() > 128) {
								skipped++;
							} else {
								if (!first)	ranges.add(", "); first = false;
								r.format(ranges);
							}
							r.reset();
						}

						variant_idx++;
					}
					if (skipped==0 && r.N != 0) {
						if (!first)	ranges.add(", "); first = false;
						r.format(ranges);
					} else if (skipped !=0) {
						ranges.format(" (and %d more)", skipped);
					}
				}

				StringBuilder ss;

				std::string use_comment;
				const std::string name = annotations.label(v.indirect_base_pointer, &use_comment, is_jump);

				ss.format("indirect base=%06X", v.indirect_base_pointer);
				if (depend_DP) ss.format(", DP=%X", v.DP);
				if (depend_DB) ss.format(", DB=%X", v.DB);

				if (!ranges.empty()) {
					ss.add(ranges.c_str());
				}

				if (!name.empty()) {
					ss.add(" - ", name);
				}

				if (!use_comment.empty()) {
					ss.add(" - ", use_comment);
				}

				writer.writeComment(pc, ss.c_str(), 0, 10);
			}
		}
	}
}