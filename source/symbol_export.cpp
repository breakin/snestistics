#include "symbol_export.h"
#include <cstdio>
#include "annotations.h"

namespace snestistics {

int mesen_pc(Pointer pc) {
	// ROM address, not something else
	// TODO: Difference LoROM/HiROM?
	return (pc>>16)*0x8000|((pc&0xFFFF)-0x8000);
}

void mesen_write_comment(FILE *f, const std::string &s) {
	if (s.empty()) return;
	fprintf(f, ":");
	for (int i = 0; i < s.size(); ++i) {
		const char c = s[i];
		if (c == '\n') {
			fprintf(f, "\\n");
		} else {
			fprintf(f, "%c", c);
		}
	}
}

void symbol_export_mesen_s(const AnnotationResolver &annotations, const std::string &filename) {
	FILE *f = fopen(filename.c_str(), "wb");

	for (size_t i=0; i<annotations._annotations.size(); i++) {
		const bool last_annotation = i+1 == annotations._annotations.size();
		const Annotation &a = annotations._annotations[i];
		const Annotation *a2 = last_annotation ? nullptr : &annotations._annotations[i+1];

		if (a.type == ANNOTATION_FUNCTION) {
			Pointer stop = a.endOfRange+1;
			if (a2 && a2->startOfRange <= stop) {
				stop = a2->startOfRange;
			}
			fprintf(f, "PRG:%X-%X:%s", mesen_pc(a.startOfRange), mesen_pc(stop), a.name.c_str());
			mesen_write_comment(f, a.comment);
			fprintf(f, "\n");
		} else if (a.type == ANNOTATION_LINE && !a.name.empty()) {
			std::string label, comment;
			annotations.line_info(a.startOfRange, &label, &comment, nullptr, true);
			fprintf(f, "PRG:%X-%X:%s", mesen_pc(a.startOfRange), mesen_pc(a.startOfRange+1), label.c_str());
			mesen_write_comment(f, comment);
			fprintf(f, "\n");
		} else if (a.type == ANNOTATION_DATA && !a.name.empty()) {
			if (a.startOfRange >= 0x002100 && a.endOfRange <= 0x00437A) // Remove MMIO
				continue;
			Pointer stop = a.endOfRange;
			if (a2 && a2->startOfRange <= stop) {
				stop = a2->endOfRange-1;
			}
			fprintf(f, "WORK:%X-%X:%s:%s\n", mesen_pc(a.startOfRange), mesen_pc(stop), a.name.c_str(), a.comment.c_str());
		}
	}

	fclose(f);
}

void symbol_export_fma(const AnnotationResolver &annotations, const std::string &filename, const bool allow_multiline_comments) {

	// https://github.com/BenjaminSchulte/fma-snes65816/blob/master/docs/symbols.adoc
	FILE *f = fopen(filename.c_str(), "wb");
	fprintf(f, "#SNES65816\n");

	fprintf(f, "\n[SYMBOL]\n");
	for (const Annotation &a : annotations._annotations) {
		if (a.type == ANNOTATION_FUNCTION) {
			fprintf(f, "%02X:%04X %s FUNC %d\n", a.startOfRange >> 16, a.startOfRange & 0xFFFF, a.name.c_str(), a.endOfRange - a.startOfRange + 1);
		}
		else if (a.type == ANNOTATION_DATA) {
			if (a.startOfRange >= 0x002100 && a.endOfRange <= 0x00437A) // Remove MMIO
				continue;
			fprintf(f, "%02X:%04X %s DATA %d\n", a.startOfRange >> 16, a.startOfRange & 0xFFFF, a.name.c_str(), a.endOfRange - a.startOfRange + 1);
		}
		else if (a.type == ANNOTATION_LINE && !a.name.empty()) {
			fprintf(f, "%02X:%04X %s FUNC %d\n", a.startOfRange >> 16, a.startOfRange & 0xFFFF, a.name.c_str(), 1);
		}
	}

	fprintf(f, "\n[COMMENT]\n");
	for (const Annotation &a : annotations._annotations) {
		if (a.startOfRange >= 0x002100 && a.endOfRange <= 0x00437A) // Remove MMIO
			continue;
		if (a.comment.empty() && a.useComment.empty())
			continue;

		const std::string &write_comment = a.comment.empty() ? a.useComment : a.comment;

		int comment_line_start = 0;
		while (comment_line_start < write_comment.length()) {
			for (int i = comment_line_start; i<write_comment.length(); ++i) {
				if (write_comment[i] == '\n' || i + 1 == write_comment.length()) {
					int write_length = i - comment_line_start;
					if (i + 1 == write_comment.length())
						write_length++;
					fprintf(f, "%02X:%04X \"", a.startOfRange >> 16, a.startOfRange & 0xFFFF);
					fwrite(&write_comment[comment_line_start], 1, write_length, f);
					comment_line_start = i + 1;

					if (!allow_multiline_comments && comment_line_start < write_comment.length()) {
						fprintf(f, " (first comment)");
					}
					fprintf(f, "\"\n");
					break;
				}
			}
			if (!allow_multiline_comments)
				break;
		}
	}
	fclose(f);
}
}
