#pragma once

class RomAccessor;
class AnnotationResolver;
struct Trace;

#include <string>

void guess_range(const Trace &trace, const RomAccessor &rom, const AnnotationResolver &annotations, std::string &output_file);
