#include "predict.h"
#include "trace.h"
#include "rom_accessor.h"
#include "annotations.h"
#include "report_writer.h"
#include "cputable.h" // decode_static_jump

namespace {
	inline uint32_t bank_add(Pointer p, uint16_t delta) {
		uint16_t a = p & 0xFFFF;
		a += delta;
		return (p&0xFF0000)|a;
	}
}
namespace snestistics {

void predict(Options::PredictEnum mode, ReportWriter *writer, const RomAccessor &rom, Trace &trace, const AnnotationResolver &annotations) {

	if (mode == Options::PRD_NEVER)
		return;

	bool limit_to_functions = mode == Options::PRD_FUNCTIONS;

	Profile profile("Predict", true);

	struct PredictBranch {
		const Annotation *annotation;
		Pointer from_pc;
		Pointer pc;
		uint16_t DP, P;
		uint8_t DB;
	};

	std::vector<PredictBranch> predict_brances;
	LargeBitfield has_op(256*64*1024);
	LargeBitfield inside_op(256 * 64 * 1024);

	for (auto opsit : trace.ops) {
		const Pointer pc = opsit.first;

		const Trace::OpVariantLookup &vl = opsit.second;
		const OpInfo &example = trace.variant(vl, 0);

		const uint8_t* data = rom.evalPtr(pc);
		const uint8_t opcode = data[0];

		uint32_t op_size = instruction_size(opcode, is_memory_accumulator_wide(example.P), is_index_wide(example.P));

		for (uint32_t i = 0; i < op_size; ++i) {
			has_op.set_bit(bank_add(pc, i));
			if (i!=0) inside_op.set_bit(bank_add(pc, i));
		}

		StringBuilder sb;

		Pointer target_jump, target_no_jump;
		bool branch_or_jump = decode_static_jump(opcode, rom, pc, &target_jump, &target_no_jump);

		const Hint *hint = annotations.hint(pc);
		if (hint && hint->has_hint(Hint::BRANCH_ALWAYS)) {
			target_no_jump = INVALID_POINTER;
		}
		if (hint && hint->has_hint(Hint::BRANCH_NEVER)) {
			target_jump = INVALID_POINTER;
		}

		if (!branch_or_jump)
			continue;

		const Annotation *source_annotation = nullptr, *target_annotation = nullptr;
		annotations.resolve_annotation(pc,          &source_annotation);

		if (target_jump != INVALID_POINTER) {
			trace.labels.set_bit(target_jump);
			annotations.resolve_annotation(target_jump, &target_annotation);
		}

		if (source_annotation || !limit_to_functions) {
			PredictBranch p;
			p.annotation = source_annotation;
			p.from_pc = pc;
			p.DB = example.DB;
			p.DP = example.DP;
			p.P  = example.P;
			if (target_jump != INVALID_POINTER && (target_annotation == source_annotation || !limit_to_functions)) {
				p.pc = target_jump;
				CUSTOM_ASSERT(target_jump != INVALID_POINTER);
				predict_brances.push_back(p);
			}
			if (target_no_jump != INVALID_POINTER && (!limit_to_functions || (target_no_jump >= source_annotation->startOfRange && target_no_jump <= source_annotation->endOfRange))) {
				// BRA,BRL and the jumps always branches/jumps
				p.pc = target_no_jump;
				CUSTOM_ASSERT(target_no_jump != INVALID_POINTER);
				predict_brances.push_back(p);
			}
		}
	}

	StringBuilder sb;
	sb.clear();

	if (writer)
		writer->writeSeperator("Prediction diagnostics");

	for (int pbi=0; pbi<(int)predict_brances.size(); ++pbi) {
		const PredictBranch pb = predict_brances[pbi];

		Pointer pc = pb.pc;
		Pointer r0 = limit_to_functions ? pb.annotation->startOfRange : 0, r1 = limit_to_functions ? pb.annotation->endOfRange : 0xFFFFFF;
		uint16_t P = pb.P, DP = pb.DP;
		uint8_t DB = pb.DB;

		bool P_unknown = false;

		if (inside_op[pc]) {
			if (writer) {
				sb.clear();
				sb.format("Predicted jump at %06X jumped inside instruction at %06X. Consider adding a \"hint branch_always/branch_never %06X\" annotation.", pc, pb.from_pc, pb.from_pc);
				writer->writeComment(sb);
			}
			printf("Warning; predicted jump went inside instruction at %06X (from %06X)\n", pc, pb.from_pc);
		}

		while (!has_op[pc] && pc >= r0 && pc <= r1) {

			// Make sure we don't run into a data scope
			const Annotation *data_scope = nullptr, *function_scope = nullptr;
			annotations.resolve_annotation(pc, &function_scope, &data_scope);
			if (data_scope)
				continue;
			if (function_scope && function_scope != pb.annotation)
				continue;

			uint8_t opcode = rom.evalByte(pc);

			bool abort_unknown_P = P_unknown;

			if (P_unknown) {
				switch(opcode) {
				case 0x02: // COP const
				case 0x40: // RTI
				case 0x6B: // RTL
				case 0x60: // RTS
				case 0x3B: // TSC
				case 0xBA: // TSX
				case 0x8A: // TXA
				case 0x9A: // TXS
				case 0x9B: // TXY
				case 0x98: // TYA
				case 0xBB: // TYX
				case 0xCB: // WAI
				case 0xEB: // XBA
				case 0xFB: // XCE
				case 0xAA: // TAX
				case 0xA8: // TAY
				case 0x5B: // TCD
				case 0x1B: // TCS
				case 0x7B: // TDC
				case 0xDB: // STP
				case 0x38: // SEC
				case 0xF8: // SED
				case 0x78: // SEI
				case 0xE2: // SEP
				case 0xC2: // REP
				case 0x18: // CLC
				case 0xD8: // CLD
				case 0x58: // CLI
				case 0xB8: // CLV
				case 0xCA: // DEX
				case 0x88: // DEY
				case 0xE8: // INX
				case 0xC8: // INY
				case 0xEA: // NOP
				case 0x8B: // PHB
				case 0x0B: // PHD
				case 0x4B: // PHK
				case 0x08: // PHP
				case 0xDA: // PHX
				case 0x5A: // PHY
				case 0x68: // PLA
				case 0xAB: // PLB
				case 0x2B: // PLD
				case 0x28: // PLP
				case 0xFA: // PLX
				case 0x7A: // PLY
					abort_unknown_P = false;
				}
			}

			if (abort_unknown_P) {
				if (writer) {
					sb.clear();
					sb.format("Aborting trace at %06X due to unknown processor status", pc);
					if (pb.annotation)
						sb.format("(in %s)", pb.annotation->name.c_str());
					writer->writeComment(sb);
				}
				break;
			}

			uint8_t operand = rom.evalByte(pc + 1);

			Pointer target_jump, target_no_jump;
			bool is_jump_or_branch = decode_static_jump(opcode, rom, pc, &target_jump, &target_no_jump);

			{
				Trace::OpVariantLookup l;
				l.count = 1;
				l.offset = (uint32_t)trace.ops_variants.size();
				trace.ops[pc] = l;
				OpInfo info;
				info.P = P_unknown ? 0 : P;
				info.DB = DB;
				info.DP = DP;
				info.X = info.Y = 0;
				info.jump_target = target_jump;
				info.indirect_base_pointer = INVALID_POINTER; // TODO: We should be able to set this one sometimes
				trace.ops_variants.push_back(info);

				// Note that DB and DP here represent a lie :)
			}

			const int op_size = instruction_size(rom.evalByte(pc), is_memory_accumulator_wide(P), is_index_wide(P));

			for (int i=0; i<op_size; ++i) {
				// TODO: We should do overlap test for entire range we are "using" now
				//       Also first might not always be best!
				trace.is_predicted.set_bit(bank_add(pc, i));
				has_op.set_bit(bank_add(pc, i));
				if (i != 0) inside_op.set_bit(bank_add(pc, i));
			}

			const Hint *hint = annotations.hint(pc);
			if (hint && hint->has_hint(Hint::BRANCH_NEVER)) {
				target_jump = INVALID_POINTER;
			}

			bool is_jsr = opcode == 0x20||opcode==0x22||opcode==0xFC;

			if (hint && hint->has_hint(Hint::JUMP_IS_JSR)) {
				 is_jump_or_branch = false;
				 is_jsr = true;
			}

			if (is_jump_or_branch) {
				const Annotation *function = nullptr;
				if (target_jump != INVALID_POINTER) {
					annotations.resolve_annotation(target_jump, &function);
					trace.labels.set_bit(target_jump);
				}

				if (limit_to_functions && writer && (!function || function != pb.annotation)) {
					sb.clear();
					sb.format("Branch going out of %s to ", pb.annotation->name.c_str());
					if (function) {
						sb.format("%s [%06X]", function->name.c_str(), target_jump);
					} else {
						sb.format("%06X", target_jump);
					}
					sb.format(". Not following due to range restriction.");
					writer->writeComment(sb);
				}
				
				if (target_jump != INVALID_POINTER) {
					PredictBranch npb = pb;
					npb.from_pc = pc;
					npb.pc = target_jump;
					predict_brances.push_back(npb);
				}

				if (hint && hint->has_hint(Hint::BRANCH_ALWAYS)) {
					// Never continoue after this op since it always diverges control flow
					continue;
				}

			} else if (opcode == 0xE2) {
				//	TODO:
				//	* When we get a REP or SEP parts or P become known again. We could track unknown per the three flags and update.
				//	* Updating DB/DP might be interesting
				if (operand & 0x10) P |= 0x0010;
				if (operand & 0x20) P |= 0x0020;
			} else if (opcode == 0xC2) {
				if (operand & 0x10) P &= ~0x0010;
				if (operand & 0x20) P &= ~0x0020;
			} else if (opcode == 0x28 || opcode == 0xFB) {
				P_unknown = true; // PLP or XCE
				// A jump or BRA, stop execution flow (not JSR or non-BRA-branch)
			} else if (opcode == 0x4C||opcode==0x5C||opcode==0x6C||opcode==0x7C||opcode==0x80) {
				if (writer && opcode != 0x80) {
					sb.clear();
					sb.format("Not following jump (opcode %02X) at %06X in %s. Only absolute jumps supported.", opcode, pc, pb.annotation->name.c_str());
					writer->writeComment(sb);
				}
				// TODO: if there is a trace annotation about jmp being jsr we could go on
				continue;
			} else if (is_jsr) {
				if (writer) {
					sb.clear();
					sb.format("Not following jump with subroutine (opcode %02X) at %06X", opcode, pc);
					if (pb.annotation)
						sb.format(" in %s.", pb.annotation->name.c_str());
					writer->writeComment(sb);
				}
			} else if (opcode == 0x40 || opcode == 0x6B || opcode == 0x60) {
				// some sort of return, stop execution flow
				continue;
			}
			pc += op_size;
		}
	}
}
}
