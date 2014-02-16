#ifndef SNESTISTICS_UTILS
#define SNESTISTICS_UTILS

#include <string>
#include <vector>
#include <stdlib.h>
#include <sstream>

typedef uint32_t Pointer;

static const Pointer INVALID_POINTER(-1);

class LargeBitfield {
private:
	std::vector<bool> m_state;
public:
	bool operator[](const size_t p) const {
		if (p >= m_state.size()) {
			return false;
		}
		return m_state[p];
	}
	void setBit(const size_t p, const bool newStat = true) {
		if (p+1 >= m_state.size()) {
			if (newStat == false) {
				// Always false outside!
				return;
			}
			m_state.resize(p + 1, false);
		}
		m_state[p] = newStat;
	}
};

static void readFile(const std::string &filename, std::vector<uint8_t> &result) {
	assert(!filename.empty());
	if (filename.empty()) {
		std::stringstream ss;
		ss << "Internal error: Filename not specifed!";
		throw std::runtime_error(ss.str());
	}

	FILE *f = fopen(filename.c_str(), "rb");
	if (f == 0) {
		std::stringstream ss;
		ss << "Could not open file " << filename << " for reading";
		throw std::runtime_error(ss.str());
	}
	fseek(f, 0, SEEK_END);
	const int fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	result.resize(fileSize);
	fread(&result[0], 1, fileSize, f);
	fclose(f);
}

#endif
