#include "auto_annotate.h"
#include "utils.h"
#include "cputable.h"
#include "annotations.h"
#include "trace.h"
#include "rom_accessor.h"

namespace snestistics {

inline bool same_bank(Pointer pc, Pointer target) {
	return ((pc >> 16) == (target >> 16));
}

void guess_range(const Trace &trace, const RomAccessor &rom, const AnnotationResolver &annotations, std::string &output_file) {

	FILE *output = fopen(output_file.c_str(), "wt");

	int unknownCounter = 0;

	struct FoundRange {
		Pointer start = INVALID_POINTER, stop = INVALID_POINTER;
		bool valid = true;

		bool inside(Pointer p) const { return p >= start && p <= stop; }

		int num_long_jumps = 0;

		std::set<Pointer> branches_out;
	};

	// TODO: Support trace annotation jump is jsr
	std::vector<FoundRange> found_ranges;

	for (auto it = trace.ops.begin(); it != trace.ops.end(); ++it) {
		FoundRange found;
		while (it != trace.ops.end()) {
			Pointer pc = it->first;

			const Annotation *function = nullptr;
			annotations.resolve_annotation(pc, &function);

			// We only want unknown but executed stuff!
			if (function)
				break;

			uint8_t opcode = rom.evalByte(pc);

			if (found.start == INVALID_POINTER)
				found.start = pc;

			if (jump_or_branch[opcode]) {
				const Hint *hint = annotations.hint(pc);
				bool merge_long_jumps = hint && hint->has_hint(Hint::ANNOTATE_MERGE);
				bool relevant_jump = merge_long_jumps || branches8[opcode];

				Pointer jump_target, jump_secondary_target;
				bool op_is_jump_or_branch = decode_static_jump(opcode, rom, pc, &jump_target, &jump_secondary_target);

				bool jump_is_jsr = false;
				if (hint && hint->has_hint(Hint::JUMP_IS_JSR))
					jump_is_jsr = true;

				if (relevant_jump && op_is_jump_or_branch && jump_target != INVALID_POINTER && same_bank(pc, jump_target)) {
					found.branches_out.insert(jump_target);
				}

				// Stop if we find a branch or a jump
				if (op_is_jump_or_branch && jump_secondary_target == INVALID_POINTER) {
					// If there is a secondary jump target (as in a branch) we don't need to stop
					found.stop = pc;
					found_ranges.push_back(found);
					break;
				}
			} else if (opcode == 0x40 || opcode == 0x6B || opcode == 0x60) {
				// Some sort of return (RTS, RTI, RTL)
				found.stop = pc;
				found_ranges.push_back(found);
				break;
			}
			it++;
		}
	}

	std::vector<uint32_t> merged_with(found_ranges.size(), -1);

	for (uint32_t j = 0; j <= found_ranges.size(); ++j) { // TODO: Make <= be just <

		bool debug = false;

		if (debug) {
			printf("Step %d\n", j);

			for (uint32_t ai = 0; ai < found_ranges.size(); ai++) {
				const FoundRange &f = found_ranges[ai];
				printf(" %d: %06X-%06X%s", ai, f.start, f.stop, f.valid ? "" : " (invalid)");
				if (merged_with[ai] != -1)
					printf(" (merged with %d)", merged_with[ai]);
				printf("\n");

				if (ai >= j)
					for (auto b : f.branches_out) {
						if (f.inside(b))
							continue;
						printf("   -> %06X", b);
						for (uint32_t bi = 0; bi < found_ranges.size(); ++bi) {
							const FoundRange &fb = found_ranges[bi];
							if (fb.start <= b && b <= fb.stop)
								printf(" %d", bi);
						}
						printf("\n");
					}
			}
		}

		// We went one extra round for debugging!
		if (j == found_ranges.size())
			break;

		uint32_t merge_target = j;
		while (merged_with[merge_target] != -1)
			merge_target = merged_with[merge_target];

		auto &branches_out = found_ranges[j].branches_out;

		FoundRange &fj = found_ranges[j];

		//printf("Doing %d, saving into %d\n", j, merge_target);
		for (Pointer target : branches_out) {
			// We have three cases; forward, backward or internal branches. We treat them separately for clarity
			if (fj.inside(target)) // Ignore internal branches/jumps
				continue;

			// TODO: This is not enough.. We must stop at any function in the range...
			//       Basically cut target at any function def...
			const Annotation *target_function = nullptr;
			annotations.resolve_annotation(target, &target_function);
			if (target_function)
				continue;

			int len = 0;
			if (target < fj.start) {
				len = fj.start - target;
			}
			else {
				len = target - fj.stop;
			}
			if (len > 1000) {
				fj.num_long_jumps++;
			}

			Pointer t1 = target;

			if (target < fj.start) {
				target = annotations.find_last_free_before_or_at(fj.start, target);
				if (target != t1) {
					int A = 9;
				}

				if (target == INVALID_POINTER)
					continue;
				// Branch going out to smaller addresses (before j)

				int lowest_merge = j;

				for (int32_t t = j - 1; t >= 0; --t) {
					FoundRange &merger = found_ranges[t];
					if (target > merger.stop) // Since ranges are sorted on start and non-overlapping we can stop here
						break;
					lowest_merge = t;
				}

				if (lowest_merge == j)
					continue;

				while (merged_with[lowest_merge] != -1)
					lowest_merge = merged_with[lowest_merge];

				// We want to merge the range (lowest_range, ..., j) into lowest_range
				for (uint32_t t = lowest_merge + 1; t <= j; t++) {
					FoundRange &f = found_ranges[t];
					if (f.valid) {
						f.valid = false;
						merged_with[t] = lowest_merge;
					}
				}

				found_ranges[lowest_merge].stop = fj.stop;

			} else if (target > fj.stop) {

				target = annotations.find_last_free_after_or_at(fj.stop, target);
				if (target != t1) {
					int A = 9;
				}

				if (target == INVALID_POINTER)
					continue;

				// Branch going out to larger addresses (after j)
				for (uint32_t t = j + 1; t<found_ranges.size(); ++t) {
					FoundRange &merger = found_ranges[t];
					if (target < merger.start) // Since ranges are sorted on start and non-overlapping we can stop here
						break;
					if (merger.valid) {
						// Merge t into j (the easy case)
						if (merger.stop > found_ranges[merge_target].stop)
							found_ranges[merge_target].stop = merger.stop;
						merger.valid = false; // Since we make it invalid it can only be merged once...
						merged_with[t] = merge_target;
					}
				}
			}
		}
	}

	int num_found = 0;

	int longest = -1, longest_range = -1;
	uint32_t longest_length = 0;

	for (uint32_t fi = 0; fi < found_ranges.size(); ++fi) {
		const FoundRange &f = found_ranges[fi];
		if (f.num_long_jumps > 4) {
			printf("Long jumps to %06X-%06X (%d)\n", f.start, f.stop, f.num_long_jumps);
		}

		if (f.valid) {
			bool collision = false;
			for (auto k : annotations._annotations) {
				if (k.type == AnnotationType::ANNOTATION_FUNCTION) {
					if (f.stop < k.startOfRange || f.stop > k.endOfRange) {
						continue;
					}
					else {
						collision = true;
						break;
					}
				}
			}
			if (collision) {
				printf("Collision!\n");
			}
			else {
				// TODO: Validate that there is no function annotation inside a range
				if (f.stop - f.start > longest_length) {
					longest = num_found;
					longest_length = f.stop - f.start + 1;
					longest_range = fi;
				}
				num_found++;
				fprintf(output, "function %06X %06X Auto%04d\n", f.start, f.stop, unknownCounter++);
			}
		}
	}

	printf("Found %d functions (longest Auto%04d, len=%d, range=%d)\n", num_found, longest, longest_length, longest_range);

	fclose(output);
}
}
