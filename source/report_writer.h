#pragma once

#include <cstdio>

/*
	The idea of the report writer is to be a convenient object to do small report from script or C++.
	One such report is the assembler output and the trace log.
*/

namespace snestistics {
	struct StringBuilder;
}

struct ReportWriter {
	ReportWriter(const char * const filename);
	~ReportWriter();

	FILE *report;
	int indentation=0;

	void writeComment(const char * const str);
	void writeComment(snestistics::StringBuilder &sb);
	void writeSeperator(const char * const text);
};