#include "utils.h"

namespace snestistics {

void LargeBitfield::write_file(FILE * f) const {
	fwrite(&_num_elements, sizeof(uint32_t), 1, f);
	fwrite(_state, sizeof(uint32_t), _num_elements, f);
}

void LargeBitfield::write_file(BigFile & file) const {
	file.write(_num_elements);
	file.write(_state, sizeof(uint32_t)*_num_elements);
}

void LargeBitfield::read_file(FILE * f) {
	uint32_t new_size = 0;
	fread(&new_size, sizeof(uint32_t), 1, f);

	if (new_size != _num_elements) {
		delete[] _state;
		_state = new uint32_t[new_size];
		_num_elements = new_size;
	}

	fread(_state, sizeof(uint32_t), new_size, f);
}

void LargeBitfield::read_file(BigFile &f) {
	uint32_t new_size = 0;
	f.read(new_size);

	if (new_size != _num_elements) {
		delete[] _state;
		_state = new uint32_t[new_size];
		_num_elements = new_size;
	}

	f.read(_state, sizeof(uint32_t)*new_size);
}
void read_file(const std::string & filename, Array<uint8_t>& result) {
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
	result.init(fileSize);
	fread(&result[0], 1, fileSize, f);
	fclose(f);
}
}
