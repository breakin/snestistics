#pragma once

#include <vector>
#include <cassert>
#include <string>
#include <stdint.h>
#include "utils.h"

typedef uint32_t Pointer;
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
struct TraceAnnotation {
	enum Type {
		PUSH_RETURN,
		POP_RETURN,
		JMP_IS_JSR,
	} type;
	Pointer location;
	int optional_parameter = 0;

	bool operator<(const TraceAnnotation &o) const {
		if (location != o.location) return location < o.location;
		if (type != o.type) return type < o.type;
		if (optional_parameter != o.optional_parameter) return optional_parameter < o.optional_parameter;
		return false;
	}
};

struct Annotation {
	AnnotationType type = ANNOTATION_NONE;
	std::string name;
	std::string comment;
	bool comment_is_multiline = false;
	std::string useComment;

	enum TraceType { TRACETYPE_DEFAULT, TRACETYPE_IGNORE } trace_type = TRACETYPE_DEFAULT;

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
	~AnnotationResolver() {
		delete[] _annotation_for_adress;
		delete[] _trace_annotation_for_adress;
	}

	void add_mmio_annotations();
	void add_vector_annotations(const RomAccessor &rom);

	void line_info(const Pointer p, std::string *label_out, std::string *comment_out, std::string *use_comment_out, bool force_label) const;

	std::string label(const Pointer p, std::string *use_comment, bool force) const;
	std::string data_label(const Pointer p, std::string *use_comment, const bool force = false) const;
	const Annotation* resolve_annotation(Pointer resolve_adress, const Annotation **function_scope = nullptr, const Annotation **data_scope = nullptr) const;
	
	void load(const std::vector<std::string> & filenames);
	std::vector<Annotation> _annotations;
	std::vector<TraceAnnotation> _trace_annotations;

	const TraceAnnotation* trace_annotation(const Pointer pc) const;

	Pointer find_last_free_before_or_at(const Pointer p, const Pointer stop) const {
		// TODO: We could binary search annotations for stop instead to avoid linear search

		// Assume in same bank
		for (int i = p; i >= (int32_t)stop; i--) {
			if ((uint32_t)i >= _annotation_for_adress_size)
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
			if (i >= _annotation_for_adress_size)
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

	// TODO: Make some sort of abstraction around these that bounds check
	// We don't use std::vector due to debug performance when we resize it and set it to zero
	int* _annotation_for_adress = nullptr;
	int* _trace_annotation_for_adress = nullptr;
	uint32_t _annotation_for_adress_size = 0;
};
