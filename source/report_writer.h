#pragma once

#include <cstdio>

/*
	The idea of the report writer is to be a convenient object to do small report from script or C++.
	One such report is the assembler output and the trace log.
*/

struct ReportWriter {
	FILE *report;
	int indentation=0;
};