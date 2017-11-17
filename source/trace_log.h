#pragma once

#include <string>

class RomAccessor;
class AnnotationResolver;
struct Options;

namespace snestistics {
	extern void write_trace_log(const Options &options, const RomAccessor &rom, const AnnotationResolver &annotations);
}
