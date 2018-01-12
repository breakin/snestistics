#pragma once

#include <vector>
#include <cassert>
#include <string>
#include <stdint.h>
#include "utils.h"

typedef uint32_t Pointer;
namespace snestistics {

class RomAccessor;

enum AnnotationType {
	ANNOTATION_NONE=0,
	ANNOTATION_FUNCTION,
	ANNOTATION_DATA,
	ANNOTATION_LINE,
};

struct AnnotationPos {
	AnnotationType type;
	Pointer address;
};

// Annotations to help the trace log
struct Hint {
	enum Type {
		JUMP_IS_JSR=1,
		JUMP_IS_JSR_ISH=2,
		JSR_IS_JMP=4,
		BRANCH_ALWAYS=8,
		BRANCH_NEVER=16,
		ANNOTATE_MERGE=32,
	};
	uint8_t hints;
	Pointer location;

	bool has_hint(Type t) const { return hints & t; }

	bool operator<(const Hint &o) const {
		if (location != o.location) return location < o.location;
		if (hints != o.hints) return hints < o.hints;
		return false;
	}
};
   
struct Annotation {
	AnnotationType type = ANNOTATION_NONE;
	std::string name;
	std::string comment;
	bool comment_is_multiline = false;
	std::string useComment;

	// label and comment does not have a endOfRange
	Pointer startOfRange, endOfRange;

	bool operator<(const Pointer c) const {
		if (startOfRange != c) return startOfRange < c;
		return false;
	}
};

class AnnotationResolver {
public:

	AnnotationResolver() {}
	~AnnotationResolver() {}

	void add_mmio_annotations();
	void add_vector_annotations(const RomAccessor &rom);

	void line_info(const Pointer p, std::string *label_out, std::string *comment_out, std::string *use_comment_out, bool force_label) const;

	std::string label(const Pointer p, std::string *use_comment, bool force) const;
	std::string data_label(const Pointer p, std::string *use_comment, const bool force = false) const;
	const Annotation* resolve_annotation(Pointer resolve_adress, const Annotation **function_scope = nullptr, const Annotation **data_scope = nullptr) const;
	
	void load(const std::vector<std::string> & filenames);
	std::vector<Annotation> _annotations;
	std::vector<Hint> _hints;

	const Hint* hint(const Pointer pc) const;

	Pointer find_last_free_before_or_at(const Pointer p, const Pointer stop) const {
		// TODO: We could binary search annotations for stop instead to avoid linear search

		// Assume in same bank
		for (int i = p; i >= (int32_t)stop; i--) {
			if ((uint32_t)i >= _annotation_for_adress.size())
				continue;
			int ai = _annotation_for_adress[i];
			if (ai == -1)
				continue;
			const Annotation &a = _annotations[ai];
			if (a.type == ANNOTATION_FUNCTION || a.type == ANNOTATION_DATA) {
				if ((uint32_t)i == p)
					return INVALID_POINTER; // No valid
				return a.endOfRange+1;
			}
		}
		return stop;
	}

	Pointer find_last_free_after_or_at(Pointer p, Pointer stop) const {

		// Assume in same bank
		for (uint32_t i = p; i <= stop; i++) {
			if (i >= _annotation_for_adress.size())
				continue;
			int ai = _annotation_for_adress[i];
			if (ai == -1)
				continue;
			const Annotation &a = _annotations[ai];
			if (a.type == ANNOTATION_FUNCTION || a.type == ANNOTATION_DATA) {
				if (i==p)
					return INVALID_POINTER; // No valid
				return a.startOfRange-1; // Min here if we stared in a function
			}
		}
		return stop;
	}

private:

	void add_vector_comment(const RomAccessor &rom, const char * const comment, const uint16_t target_at);

	// Prefer load(std::vector<string>)
	void load(std::istream &input, const std::string &error_file); // Can be called many times, end with finalize
	void finalize();

	Array<int> _annotation_for_adress, _hint_for_adress;
};
}
