#pragma once

#include <string>

class RomAccessor;
class AnnotationResolver;
struct Options;
namespace scripting_interface {
	struct Scripting;
}

namespace snestistics {
	extern void write_trace_log(const Options &options, const RomAccessor &rom, const AnnotationResolver &annotations, scripting_interface::Scripting *scripting);
}

struct TraceLog {
	TraceLog() {}
};
