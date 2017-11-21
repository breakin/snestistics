#pragma once

#include <stdint.h>
#include "report_writer.h"

void report_writer_print(ReportWriter *report_writer, const char * const str, uint32_t len) {
	int indent = report_writer->indentation;
	FILE *f = report_writer->report;
	for (int i=0; i<indent; ++i) {
		fputc(' ', f);
	}
	fwrite(str, 1, len, f);
	fputc('\n', f);
}
