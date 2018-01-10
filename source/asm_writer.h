#pragma once

struct ReportWriter;
namespace snestistics {
	struct Trace;
	class RomAccessor;
	class AnnotationResolver;
	void asm_writer(ReportWriter &report, const Options &options, Trace &trace, const AnnotationResolver &annotations, const RomAccessor &rom_accessor);
}

