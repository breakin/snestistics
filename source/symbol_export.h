#pragma once

#include <string>

namespace snestistics {
	class AnnotationResolver;
	void symbol_export_fma(const AnnotationResolver &annotations, const std::string &filename, const bool allow_multiline_comments);
}
