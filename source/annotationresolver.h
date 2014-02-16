#ifndef ANNOTATERESOLVER_H
#define ANNOTATERESOLVER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <cassert>
#include <stdint.h>

typedef uint32_t Pointer;

enum AnnotationType {
	ANNOTATION_NONE,
	ANNOTATION_DATA,
	ANNOTATION_FUNCTION,
	ANNOTATION_POINTERTABLE_16,
	ANNOTATION_POINTERTABLE_24, // Bankless, assume same bank as table belongs in
};

struct Annotation {
	Annotation() : type(ANNOTATION_NONE) {}
	AnnotationType type;
	std::string name;
	std::string description, useComment;
	std::string credit;

	// For ANNOTATION_FUNCTION, ANNOTATION_POINTERTABLE_16, ANNOTATION_POINTERTABLE_24
	Pointer startOfRange, endOfRange;
};

class AnnotationResolver {
public:

	AnnotationResolver(const AnnotationResolver *parentResolver=0) : m_parentResolver(parentResolver) {}
	void load(std::istream &input);

	// It is important to support save if we ever wish to update the format, or need sorting based on something
	void save(std::ostream &output);

	std::string resolveLineComment(const Pointer atAddress) const {
		auto it = m_comments.find(atAddress);
		if (it == m_comments.end()) {
			if (m_parentResolver) {
				return m_parentResolver->resolveLineComment(atAddress);
			}
			else {
				return std::string();
			}
		}
		return it->second;
	}

	const Annotation *resolve(const Pointer atAddress, const Pointer resolveAddress, bool mustBeAtStart = true, const bool performRemap = true) const {

		uint8_t bank = resolveAddress >> 16;
		uint16_t adr = resolveAddress & 0xffff;

		if (performRemap) {

			// Do bank magic... depending on adress, map to 7E or hardware register
			if (adr < 0x2000) {
				bank = 0x7E;
			}
			else if (adr >= 0x2000 && adr < 0x4000 && bank != 0x7E) {
				// Just a hack, this way we only need to keep hardware registers annotated in one bank (the first)
				bank = 0x00;
			}
		}

		const Pointer rm((bank << 16) | adr);

		if (rm < m_annotationPerStuff.size()) {
			const int b = m_annotationPerStuff[rm];
			if (b != -1) {
				if (!mustBeAtStart || m_annotations[b].startOfRange == rm) {
					return &m_annotations[b];
				}
			}
			else if (m_parentResolver) {
				return m_parentResolver->resolve(atAddress, resolveAddress, mustBeAtStart, performRemap);
			}
		}
		return nullptr;
	}

	// Works for any line but supports annotations when available
	std::string getLabelName(const Pointer address, std::string *description = 0, std::string *useComment = 0) const {

		// NOTE: This annotation might be from a parent as well from us
		const Annotation *ann = resolve(address, address, false);

 		if (ann && ann->type == ANNOTATION_FUNCTION) {

			if (ann->startOfRange == address) {
				if (description) {
					*description = ann->description;
				}
				if (useComment) {
					*useComment = ann->useComment;
				}
				return ann->name;
			}
			else {
				if (description) { *description = ""; }
				if (useComment) { *useComment = ""; }
				char hej[64];
				sprintf(hej, "_%s_%06X", ann->name.c_str(), address);
				return std::string(hej);
			}
		}
		if (description) { *description = ""; }
		if (useComment) { *useComment = ""; }
		char hej[64];
		sprintf(hej, "label_%06X", address);
		return std::string(hej);
	}

	void load(const std::string &filename) {
		if (filename.empty()) {
			return;
		}
		std::ifstream loader;
		loader.open(filename);
		assert(loader.is_open());
		load(loader);
	}
	void save(const std::string &filename) {
		std::ofstream saver;
		saver.open(filename);
		assert(saver.is_open());
		save(saver);
	}

private:
	std::vector<Annotation> m_annotations;
	std::unordered_map<Pointer, std::string> m_comments;
	std::vector<int> m_annotationPerStuff;
	const AnnotationResolver *m_parentResolver;
};

#endif // ANNOTATERESOLVER_H

