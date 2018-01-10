#pragma once

#include <string>

struct Options;
namespace scripting_interface {
	struct Scripting;
}

namespace snestistics {

class AnnotationResolver;
class RomAccessor;
void write_trace_log(const Options &options, const RomAccessor &rom, const AnnotationResolver &annotations, scripting_interface::Scripting *scripting);

}

struct TraceLog {
	TraceLog() {}
};
