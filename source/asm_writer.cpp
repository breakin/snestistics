#include "utils.h"
#include "options.h"
#include <sstream>
#include "rom_accessor.h"
#include "cputable.h"
#include "asm_writer.h"
#include "report_writer.h"
#include <algorithm>
#include "annotations.h"
#include "rom_accessor.h"
#include "trace.h"

/*
	TODO: This look of this file is partially to blame for me wanting multiple asm output languages.
	      It needs to be cleaned up a lot! Move functions over to report_writer and remove the AsmWriteWLADX class.
*/

namespace {

	using namespace snestistics;

	// Flags are "sticky"... true=bit unknown
template<typename T>
struct MagicT {
	MagicT(const T _value) : value(_value), flags(0) {}
	MagicT(const T _value, const T _flags) : value(_value), flags(_flags) {}

	T value;
	T flags; // Track if bits are known or unknown. 0=

	void operator=(const T &other) {
		value = other;
		flags = 0; // assume uint type, all are known
	}

	// Operation by sure numbers
	MagicT operator+(const uint32_t knownValue) const { return MagicT(value + knownValue, flags); }
	MagicT operator&(const size_t v) const { return MagicT(value & v, flags & v); }
	MagicT operator|(const size_t v) const { return MagicT(value & v, flags & v); }

	// Operation with unsure members
	MagicT operator|(const MagicT &other) const {
		return (value | other.value, flags | other.flags);
	}
	MagicT operator&(const MagicT &other) const {
		return (value & other.value, flags | other.flags);
	}

	MagicT operator<<(const size_t steps) const {
		MagicT result;
		result.value = value << steps;
		result.flags = flags << steps;
		return result;
	}
	MagicT operator>>(const size_t steps) const {
		MagicT result;
		result.value = value >> steps;
		result.flags = flags >> steps;
		return result;
	}

	void set_unknown() {
		flags = (T)-1;
	}

	bool isKnown() const {
		return flags == 0;
	}
};

typedef MagicT<uint8_t> MagicByte;
typedef MagicT<uint16_t> MagicWord;

struct CPURegisters {
	CPURegisters() : P(0x1FF), pb(-1), pc(-1), db(-1), dp(-1), reg_A(-1), reg_X(-1), reg_Y(-1) {}
	MagicWord P; // processor status
	MagicByte pb;
	MagicWord pc;
	MagicByte db;
	MagicWord dp;
	MagicWord reg_A;
	MagicWord reg_X;
	MagicWord reg_Y;
};

enum ResultType {
	SA_IMMEDIATE,
	SA_ADRESS,
	SA_ACCUMULATOR,
	SA_NOT_IMPLEMENTED
};

ResultType evaluateOp(const snestistics::RomAccessor &rom_accessor, const uint8_t * ops, const CPURegisters & reg, MagicByte * result_bank, MagicWord * result_addr, bool *depend_DB, bool *depend_DP, bool *depend_X, bool *depend_Y) {

	assert(!depend_DB || !(*depend_DB));
	assert(!depend_DP || !(*depend_DP));
	assert(!depend_X  || !(*depend_X ));
	assert(!depend_Y  || !(*depend_Y ));

	// Introduce all registers with known flags.. then calculate adress with values... and check flags to see if it was "determined"

	// TODO: Some of these are based on the fact that we want to do an indirection via memory...
	//       but alot of memory is known (such as all jump tables). Involve ROM.
	//       For this to work we must differentiate between source adress and the pointer read...

	const uint8_t opcode = *ops;
	const int am = opCodeInfo[opcode].adressMode;

	if (am >= 0 && am <= 3) {
		return SA_IMMEDIATE;
	}
	else if (am == 4) {
		*result_bank = reg.pb;
		*result_addr = branch8(reg.pc.value, ops[1]);
		return SA_ADRESS;
	}
	else if (am == 5) {
		// BRL, PER
		*result_bank = reg.pb;
		*result_addr = branch16(reg.pc.value, ops[1] * 256 + ops[2]);
		return SA_ADRESS;
	}
	else if (am == 6) {
		*result_bank = 0x00;
		*result_addr = reg.dp + ops[1];
		if (depend_DP) *depend_DP = true;
		return SA_ADRESS;
	}
	else if (am == 13) {
		// TODO: Would be cool to not only support this, but also give the indirection pointer... it probably points out a jump table or so
		return SA_NOT_IMPLEMENTED;
	}
	else if (am == 14 || am == 28) {
		*result_bank = am == 14 ? reg.db : reg.pb;
		*result_addr = ((ops[2] << 8) | ops[1]);
		if (am == 14 && depend_DB) *depend_DB = true;
		return SA_ADRESS;
	}
	else if (am == 15) {
		*result_bank = am == 15 ? reg.db : reg.pb;
		*result_addr = reg.reg_X + ((ops[2] << 8) | ops[1]); // TODO: Must X be 16-bit for this op?
		if (depend_X) *depend_X = true;
		if (am == 15 && depend_DB) *depend_DB = true;
		return SA_ADRESS;
	}
	else if (am == 16) {
		*result_bank = reg.db;
		*result_addr = reg.reg_Y + ((ops[2] << 8) | ops[1]); // TODO: Must Y be 16-bit for this op?
		if (depend_Y) *depend_Y = true;
		if (depend_DB) *depend_DB = true;
		return SA_ADRESS;
	}
	else if (am == 17) {
		*result_bank = ops[3];
		*result_addr = ((ops[2] << 8) | ops[1]);
		return SA_ADRESS;
	}
	else if (am == 18) {
		*result_bank = ops[3];
		*result_addr = reg.reg_X + ((ops[2] << 8) | ops[1]);
		if (depend_X) *depend_X = true;
		return SA_ADRESS;
	}
	else if (am == 22) {
		uint16_t addr = (ops[2] << 8) | ops[1];
		bool is_jump = jumps[opcode]; // TODO: Consider making other am?
		uint8_t bank = is_jump ? reg.pb.value : reg.db.value; // NOTE: Bypasses unknown system for now, TODO
		Pointer indirection_pointer = (bank << 16)|addr;
		if (!rom_accessor.is_rom(indirection_pointer))
			return SA_ADRESS;
		uint8_t l = rom_accessor.evalByte(indirection_pointer  );
		uint8_t h = rom_accessor.evalByte(indirection_pointer+1);
		*result_bank = bank;
		*result_addr = (h<<8)|l;
		return SA_ADRESS;
	}
	else if (am == 23) {
		MagicWord addr = reg.reg_X + (ops[2] << 8) | ops[1];
		bool is_jump = jumps[opcode];
		assert(is_jump);
		uint8_t bank = reg.pb.value; // NOTE: Bypasses unknown system for now, TODO
		Pointer indirection_pointer = (bank << 16)|addr.value;
		if (!rom_accessor.is_rom(indirection_pointer))
			return SA_ADRESS;
									  // Address load wraps within the bank
		uint8_t l = rom_accessor.evalByte(indirection_pointer  );
		uint8_t h = rom_accessor.evalByte(indirection_pointer+1);
		*result_bank = bank;
		*result_addr = (h<<8)|l;
		return SA_ADRESS;
	}
	else if (am == 24) {
		return SA_ACCUMULATOR;
	}
	else {
		return SA_NOT_IMPLEMENTED;
	}
}


template<typename T>
struct CombinationT {
	T value = (T)0;
	bool single_value = true;
	bool has_value = false;

	void operator=(const T t) {
		if (has_value) {
			if (t != value) {
				single_value = false;
				value = (T)0;
			}
		} else {
			has_value = true;
			single_value = true;
			value = t;
		}
	}
};

typedef CombinationT<bool> CombinationBool;
typedef CombinationT<uint8_t> Combination8;
typedef CombinationT<uint16_t> Combination16;

class AsmWriteWLADX {
private:
	const Options &m_options;
	const snestistics::RomAccessor &m_romData;
	Pointer m_nextPC;
	bool m_bankOpen = false;
	int m_sectionCounter = 0;
	FILE * const m_outFile;

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
	AsmWriteWLADX(ReportWriter &report, const Options &options, const snestistics::RomAccessor &romData) : m_options(options), m_report(report), m_romData(romData), m_nextPC(INVALID_POINTER), m_outFile(report.report) {
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
		writeCommentHelper(start_pos, target_column, comment);
	}

	void writeComment(StringBuilder &sb) {
		m_report.writeComment(sb);
	}

	void writeSeperator(const char * const text) {
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

		const bool emitCommentPC = m_options.asm_print_pc || m_options.asm_print_bytes || overrideInstructionWithDB;

		nw += fprintf(m_outFile, "    ");
		if (emitCommentPC) {
			nw += fprintf(m_outFile, "/*%c", is_predicted ? 'p':' ');
		}

		if (m_options.asm_print_register_sizes) {
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

		if (m_options.asm_print_db) {
			assert(data_bank.has_value);
			if (data_bank.single_value) {
				nw += fprintf(m_outFile, "%02X ", data_bank.value);
			} else {
				nw += fprintf(m_outFile, "   ");
			}
		}

		if (m_options.asm_print_dp) {
			assert(direct_page.has_value);
			if (direct_page.single_value) {
				nw += fprintf(m_outFile, "%04X ", direct_page.value);
			} else {
				nw += fprintf(m_outFile, "     ");
			}
		}

		if (m_options.asm_print_pc) {
			nw += fprintf(m_outFile, "%06X ", pc);
		}
		if (m_options.asm_print_bytes) {
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
		if (m_options.asm_lower_case_op) {
			nw += fprintf(m_outFile, "%c%c%c", tolower(op[0]), tolower(op[1]), tolower(op[2]));
		} else {
			nw += fprintf(m_outFile, "%s", op);
		}

		if (strcmp(opCodeInfo[data[0]].mnemonics, "BRL") == 0) {

		} else if (numBits == 0) {
		} else if (numBits == 8) {
			nw += fprintf(m_outFile, ".b");
		} else if (numBits == 16) {
			nw += fprintf(m_outFile, ".w");
		} else if (numBits == 24) {
			nw += fprintf(m_outFile, ".l");
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

	void write_vectors(const snestistics::AnnotationResolver &annotations, const LargeBitfield &trace) {
		// "Programming the 65816", page 55
		fprintf(m_outFile, ".SNESNATIVEVECTOR      ; Define Native Mode interrupt vector table\n");
		write_vector_single("COP",   annotations, trace, 0xFFE4);
		write_vector_single("BRK",   annotations, trace, 0xFFE6);
		write_vector_single("ABORT", annotations, trace, 0xFFE8);
		write_vector_single("NMI",   annotations, trace, 0xFFEA);
		write_vector_single("IRQ",   annotations, trace, 0xFFEE);
		fprintf(m_outFile, ".ENDNATIVEVECTOR\n");

		fprintf(m_outFile, "\n.SNESEMUVECTOR         ; Define Emulation Mode interrupt vector table\n");
		write_vector_single("COP",    annotations, trace, 0xFFF4);
		write_vector_single("ABORT",  annotations, trace, 0xFFF8);
		write_vector_single("NMI",    annotations, trace, 0xFFFA);
		write_vector_single("RESET",  annotations, trace, 0xFFFC);
		write_vector_single("IRQBRK", annotations, trace, 0xFFFE);
		fprintf(m_outFile, ".ENDEMUVECTOR\n");
	}

private:

	inline void write_vector_single(const char * const name, const snestistics::AnnotationResolver &annotations, const LargeBitfield &labels, const uint16_t target_at) {
		uint16_t target = *(uint16_t*)m_romData.evalPtr(target_at);
		// NOTE: We don't require an annotation for the label, but it must have be part of trace such that
		//       a label is emitted in the source. Otherwise revert to hex
		if (labels[target] && (target & 0xFF0000) == 0) {
			std::string label_name = annotations.label(target, nullptr, true);
			if (!label_name.empty()) {
				fprintf(m_outFile, "  %-6s %s\n", name, label_name.c_str());
				return;
			}
		}
		fprintf(m_outFile, "  %-6s $%04X\n", name, target);
	}

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

	void prepareWrite(const Pointer pc, const bool advancePC=true) {
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

namespace snestistics {

void asm_writer(ReportWriter &report, const Options &options, Trace &trace, const AnnotationResolver &annotations, const RomAccessor &rom_accessor) {
	AsmWriteWLADX writer(report, options, rom_accessor);

	int next_dma_event = 0;

	for (const Annotation &a : annotations._annotations) {
		if (a.type == ANNOTATION_FUNCTION || (a.type == ANNOTATION_LINE && !a.name.empty()))
			trace.labels.set_bit(a.startOfRange);
	}

	writer.writeSeperator("Header");

	if (!options.asm_header_file.empty()) {
		Array<unsigned char> header;
		read_file(options.asm_header_file, header);
		fwrite(&header[0], header.size(), 1, report.report);
		char newline = '\n';
		fwrite(&newline, 1, 1, report.report);
	}

	writer.write_vectors(annotations, trace.labels);

	writer.writeSeperator("Data");

	// Generate EQU for each label
	// TODO: Only include USED labels?
	for (const Annotation &a : annotations._annotations) {
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
			// NOTE-TODO: While an op might depend on say DB it might not use it if adress is in $0000-$1999

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

		if (opcode == 0x82) {
			// BRL
			uint16_t operand16 = data[1]|(data[2]<<8);
			const Pointer branch_target = branch16(pc, operand16);
			proposals[branch_target] = "";
		} else if (branches[opcode]) {
			const Pointer branch_target = branch8(pc, data[1]);
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
						const Pointer p = rom_accessor.lorom_bank_remap((rb.value << 16) | ra.value);

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

		while (next_dma_event < trace.dma_transfers.size() && trace.dma_transfers[next_dma_event].pc == pc) {
			const DmaTransfer &d = trace.dma_transfers[next_dma_event++];
			StringBuilder ss;
			bool reverse = d.flags & DmaTransfer::REVERSE_TRANSFER;
			int num_bytes = d.transfer_bytes == 0 ? 0x10000 : d.transfer_bytes;
			assert(d.b_address == 0x80); // So we can detect that we've enabled more and can improve
			ss.format("dma-event $%02X:%04X %s $%02X:%04X (via $2180), $%X/%d bytes%s%s", d.a_bank, d.a_address, reverse ? "<-" : "->", d.wram>>16, d.wram&0xFFFF, num_bytes, num_bytes, d.flags & DmaTransfer::A_ADDRESS_DECREMENT ?" (decrease A)":"", d.flags & DmaTransfer::A_ADDRESS_FIXED ? " (A fixed)":"");
			writer.writeComment(pc, ss.c_str(), 0, 10);
		}
	}
}
}
