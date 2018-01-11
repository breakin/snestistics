#pragma once

#include "options.h" // TODO: For the enum, can we forward declare enums?

struct ReportWriter;

namespace snestistics {
	struct Trace;
	class RomAccessor;
	class AnnotationResolver;
	void predict(Options::PredictEnum mode, ReportWriter *writer, const RomAccessor &rom, Trace &trace, const AnnotationResolver &annotations);
}
