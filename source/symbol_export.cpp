#include "symbol_export.h"
#include <cstdio>
#include "annotations.h"

void symbol_export_fma(const AnnotationResolver & annotations, const std::string & filename) {

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
		if (a.type == ANNOTATION_NONE)
			continue;
		if (a.startOfRange >= 0x002100 && a.endOfRange <= 0x00437A) // Remove MMIO
			continue;
		if (a.comment.empty() && a.useComment.empty())
			continue;

		const std::string &write_comment = a.comment.empty() ? a.useComment : a.comment;

		static const bool allow_multiline = false; // bsnes-plus currently only supports one line of comment

												   // The actual comment
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

					if (!allow_multiline && comment_line_start < write_comment.length()) {
						fprintf(f, " (first comment)");
					}
					fprintf(f, "\"\n");
					break;
				}
			}
			if (!allow_multiline)
				break;
		}
	}
	fclose(f);

}
