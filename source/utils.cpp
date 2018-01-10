#include "utils.h"

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
