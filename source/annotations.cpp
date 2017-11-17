#include "annotations.h"
#include <map>
#include <algorithm>
#include <sstream>
#include "utils.h"
#include <fstream>

void AnnotationResolver::load(std::istream &input, const std::string &error_file) {
	std::string comment;
	std::string useComment;
	Annotation::TraceType trace_type = Annotation::TRACETYPE_DEFAULT;
	bool comment_is_multiline = false;

	char name[1024]="";
	char mycomment[1024];

	Pointer start;

	int size;

	std::istream &f = input;

	int line_number = 0;
	
	while (!f.eof()) {
		line_number++;
		char buf[4096];
		f.getline(buf, 4096);

		size = 0;

		if (strlen(buf) == 0) {
			continue;
		}
		if (buf[0] == '@') {
			// meta-comment
			continue;
		} else if (buf[0] == ';') {
			const char *stripped = &buf[2];
			if (buf[1]=='\0') {
				stripped = "";
			} else if (buf[1] != ' ') {
				stripped = &buf[1];
			}
			if (!comment.empty()) {
				comment = comment + "\n" + stripped;
				comment_is_multiline = true;
			}
			else {
				comment = stripped;
			}
		}
		else if (buf[0] == '#') {
			useComment = &buf[2];
		} else if (sscanf(buf, "trace %06X %s %d", &start, (char*)&name, &size) > 0) {
			TraceAnnotation ta;
			if (strcmp(name, "push_return")==0) {
				ta.type = TraceAnnotation::PUSH_RETURN;
			} else if (strcmp(name, "pop_return")==0) {
				ta.type = TraceAnnotation::POP_RETURN;
			} else if (strcmp(name, "jmp_is_jsr")==0) {
				ta.type = TraceAnnotation::JMP_IS_JSR;
			}
			ta.location = start;
			ta.optional_parameter = size;
			_trace_annotations.push_back(ta);
			continue;
		} else if (sscanf(buf, "trace %s", (char*)&name) > 0) {
			if (strcmp(name, "ignore")==0) trace_type = Annotation::TRACETYPE_IGNORE;
		} else {

			Annotation a;

			if (sscanf(buf, "function %06X %06X %s", &a.startOfRange, &a.endOfRange, name) > 0) {
				a.type = ANNOTATION_FUNCTION;
				const bool convert_single_byte_functions_to_labels = false;
				if (convert_single_byte_functions_to_labels && a.startOfRange == a.endOfRange) {
					a.type = ANNOTATION_LINE;
					printf("Function '%s' is only one byte long, treating as 'label' instead!\n", name);
					a.endOfRange = a.startOfRange;
				}
			} else if (sscanf(buf, "line %06X", &a.startOfRange) > 0) {
				a.type = ANNOTATION_LINE;
				a.endOfRange = a.startOfRange;
			} else if (sscanf(buf, "comment %06X \"%[^\"]", &a.startOfRange, mycomment) > 0) {
				a.type = ANNOTATION_LINE;
				a.comment = mycomment;
				a.comment_is_multiline = false;
				a.useComment = "";
				a.endOfRange = a.startOfRange;
			} else if (sscanf(buf, "label %06X %s", &a.startOfRange, name) > 0) {
				a.type = ANNOTATION_LINE;
				a.endOfRange = a.startOfRange;
			} else if (sscanf(buf, "data %06X %06X %s", &a.startOfRange, &a.endOfRange, name) >0 ) {
				a.type = ANNOTATION_DATA;
			} else {
				printf("%s:%d: Not supported format!\n\t'%s'\n", error_file.c_str(), line_number, buf);
				exit(99);
			}

			a.useComment = useComment;
			a.comment = comment;
			a.comment_is_multiline = comment_is_multiline;
			a.trace_type = trace_type;
			a.name = name;
			_annotations.push_back(a);

			// Reset state
			comment.clear();
			useComment.clear();
			comment_is_multiline = false;
			name[0]='\0';
			trace_type = Annotation::TRACETYPE_DEFAULT;
		}
	}
}

void AnnotationResolver::finalize() {

	Pointer largest_adress = 0;
	for (auto a : _annotations) {
		largest_adress = std::max(largest_adress, a.endOfRange);
	}

	uint32_t k = largest_adress + 1;
	_annotation_for_adress_size = k;
	_annotation_for_adress = new int[k];
	for (uint32_t i = 0; i < k; i++) _annotation_for_adress[i] = -1;
	
	std::sort(_annotations.begin(), _annotations.end());

	Pointer p = 0xFFFFFFFF;
	for (int i=0; i<(int)_annotations.size(); ++i) {
		const Annotation &a = _annotations[i];
		Pointer start = a.startOfRange;
		if (start == p) {
			printf("Annotation collision at %06X! Only one annotation can start at the same adress!\n", start);
			throw std::runtime_error("Annotation collision!");
		}
		p = start;
		for (uint32_t k=a.startOfRange; k <= a.endOfRange; ++k) {
			if (a.type == ANNOTATION_LINE || _annotation_for_adress[k] == -1) {
				_annotation_for_adress[k] = i;
			} else {
				const Annotation &b = _annotations[_annotation_for_adress[k]];
				printf("Annotation collision at %06X! Both %s [%06X-%06X] and %s [%06X-%06X]!\n", k, a.name.c_str(), a.startOfRange, a.endOfRange, b.name.c_str(), b.startOfRange, b.endOfRange);
				throw std::runtime_error("Annotation collision!");
			}
		}
	}
	
	_trace_annotation_for_adress = new int[k];
	for (uint32_t i = 0; i < k; i++) _trace_annotation_for_adress[i] = -1;

	std::sort(_trace_annotations.begin(), _trace_annotations.end());
	for (uint32_t i = 0; i < _trace_annotations.size(); ++i) {
		const TraceAnnotation &ta = _trace_annotations[i];
		_trace_annotation_for_adress[ta.location] = i;
	}
}

static void write_comment(FILE *output, const std::string &comment) {
	bool dang = true;
	for (size_t i=0; i<comment.length(); i++) {
		if (dang) {
			fprintf(output, "; ");
			dang = false;
		}
		fprintf(output, "%c", comment[i]);
		if (comment[i]=='\n') {
			dang = true;
		}
	}
	fprintf(output, "\n");
}

void AnnotationResolver::line_info(const Pointer p, std::string * label_out, std::string * comment_out, std::string * use_comment_out, bool force_label) const {

	const Annotation *function, *data;
	const Annotation *line_annotation = resolve_annotation(p, &function, &data);

	if (line_annotation && line_annotation->startOfRange == p) {
		if (comment_out) *comment_out = line_annotation->comment;
		if (use_comment_out) *use_comment_out = line_annotation->useComment;
	}

	if (data && data->startOfRange == p) {
		if (label_out) *label_out = data->name;
		return;
	}
	else 	if (function && function->startOfRange == p) {
		if (label_out) *label_out = function->name;
		return;
	}

	std::string label;
	if (line_annotation) {
		label = line_annotation->name;
	}

	if (label.empty() && !force_label)
		return;

	// We are in global scope or inside a (function|data)-scope and user has requsted a label
	StringBuilder sb;
	bool has_scope = true;
	if (data) {
		sb.format("_%s_", data->name.c_str());
	}
	else if (function) {
		sb.format("_%s_", function->name.c_str());
	}
	else {
		has_scope = false;
	}

	if (label.empty() && !has_scope) {
		sb.format("label_%06X", p);
	}
	else if (label.empty() && has_scope) {
		sb.format("%06X", p);
	}
	else {
		sb.add(label);
	}

	if (label_out) *label_out = sb.c_str();
}

std::string AnnotationResolver::label(const Pointer p, std::string * use_comment, bool force) const {
	std::string l;
	line_info(p, &l, nullptr, use_comment, force);
	return l;
}

std::string AnnotationResolver::data_label(const Pointer p, std::string * use_comment, const bool force) const {
	assert(!use_comment || use_comment->empty());
	std::string l;
	line_info(p, &l, nullptr, use_comment, false);
	if (!l.empty() || !force)
		return l;
	char t[7];
	sprintf(t, "data_%0X", p);
	return std::string(t);
}

const Annotation * AnnotationResolver::resolve_annotation(Pointer resolve_adress, const Annotation ** function_scope, const Annotation ** data_scope) const {
	if (function_scope) *function_scope = nullptr;
	if (data_scope) *data_scope = nullptr;

	int start = -1;
	if (resolve_adress < _annotation_for_adress_size) {
		start = _annotation_for_adress[resolve_adress];
	}
	// This is based on init of function and data filling all addresses as belonging to them when there is no line info. So -1 means nothing, no scope...
	if (start == -1) {
		return nullptr;
	}

	const Annotation *line_annotation = &_annotations[start];

	const Annotation *found_data = nullptr;

	if (line_annotation && line_annotation->type == ANNOTATION_FUNCTION && line_annotation->startOfRange == resolve_adress) {
		if (function_scope)	*function_scope = line_annotation;
		return line_annotation;
	}
	else if (line_annotation && line_annotation->type == ANNOTATION_DATA) {
		found_data = line_annotation;
	}

	if (line_annotation->startOfRange != resolve_adress)
		line_annotation = nullptr;

	for (int k = start; k >= 0; --k) {
		const Annotation &a = _annotations[k];
		if (a.type == ANNOTATION_FUNCTION && a.endOfRange < resolve_adress)
			break;
		if (a.type == ANNOTATION_DATA && !found_data) found_data = &a;
		if (a.type == ANNOTATION_FUNCTION) {
			if (function_scope) *function_scope = &a;
			break;
		}
	}
	if (found_data && data_scope) {
		*data_scope = found_data;
	}
	return line_annotation;
}

void AnnotationResolver::load(const std::vector<std::string> & filenames) {
	if (filenames.empty()) {
		return;
	}
	for (auto f : filenames) {
		printf("Loading annotations from %s...\n", f.c_str());
		std::ifstream loader;
		loader.open(f.c_str(), std::ios::in);
		if (!loader.is_open()) {
			printf("Failed to open '%s'\n", f.c_str());
			throw std::runtime_error("Failed to open file!");
		}
		load(loader, f);
	}
	finalize();
}

const TraceAnnotation * AnnotationResolver::trace_annotation(const Pointer pc) const {
	if (pc >= _annotation_for_adress_size)
		return nullptr;
	int tai = _trace_annotation_for_adress[pc];
	if (tai == -1) return nullptr;
	return &_trace_annotations[tai];
}
