#include "report_writer.h"
#include "utils.h"

void report_writer_print(ReportWriter *report_writer, const char * const str, uint32_t len) {
	int indent = report_writer->indentation;
	FILE *f = report_writer->report;
	for (int i=0; i<indent; ++i) {
		fputc(' ', f);
	}
	fwrite(str, 1, len, f);
	fputc('\n', f);
}

ReportWriter::ReportWriter(const char * const filename) : report(fopen(filename, "wb")) {
	if (!report) {
		printf("Could not open the file '%s'\n", filename);
		exit(1);
	}
}

ReportWriter::~ReportWriter() {
	fclose(report);
}

void ReportWriter::writeComment(const char * const str) {
	fprintf(report, "%s\n", str);
}

void ReportWriter::writeComment(StringBuilder & sb) {
	writeComment(sb.c_str());
	sb.clear();
}

void ReportWriter::writeSeperator(const char * const text) {
	fprintf(report, "\n");
	fprintf(report, "; =====================================================================================================\n");
	fprintf(report, "; %s\n", text);
	fprintf(report, "; =====================================================================================================\n");
}
