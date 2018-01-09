#pragma once

#include <string>

namespace snestistics {
	struct Trace;
	class RomAccessor;
	class AnnotationResolver;
	void guess_range(const Trace &trace, const RomAccessor &rom, const AnnotationResolver &annotations, std::string &output_file);
}
