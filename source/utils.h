#pragma once

#include <cstring> // memset
#include <stdexcept> // std::runtime_error
#include <string>
#include <vector>
#include <stdlib.h>
#include <sstream>
#include <stdio.h>
#include <stdarg.h>
#include <ctime>
#include <cassert>

#define CUSTOM_ASSERT(COND) assert(COND);
//#define CUSTOM_ASSERT(COND)

typedef uint32_t Pointer;
struct BigFile;

static const Pointer INVALID_POINTER(-1);

struct Profile {
	const char * const _msg;
	clock_t   _start;
	bool _hide_if_fast;
	Profile(const char *msg, const bool hide_if_fast = false) : _msg(msg), _start(clock()), _hide_if_fast(hide_if_fast) {
		if (!_hide_if_fast)
			printf("%s...\n", msg);
	}
	~Profile() {
		time_t end;
		time(&end);
		double elapsed = (clock() - _start)/(float)CLOCKS_PER_SEC;
		if (elapsed>0.01f) {
			printf(" %s%s: %.2f seconds\n", _hide_if_fast ? ">":"",_msg, elapsed);
		}
	}
};

class LargeBitfield {
private:
	uint32_t* _state = nullptr;
	uint32_t _num_elements = 0;
public:
	LargeBitfield(const uint32_t size) {
		int k = (size + 31) / 32;
		_state = new uint32_t[k];
		memset(_state, 0, k*sizeof(uint32_t));
		_num_elements = k;
	}
	bool operator[](const uint32_t p) const {
		uint32_t bucket = p / 32;
		uint32_t mask = 1 << (p & 31);

		bool result = (_state[bucket] & mask) != 0;
		return result;
	}
	void setBit(const uint32_t p, const bool newStat = true) {
		uint32_t bucket = p / 32;
		uint32_t mask = 1 << (p & 31);
		uint32_t current = _state[bucket] & ~mask;
		if (newStat)
			current |= mask;
		_state[bucket] = current;
		assert(this->operator[](p) == newStat);
	}

	void write_file(FILE *f) const;
	void write_file(BigFile &file) const;
	void read_file(BigFile &file);
	void read_file(FILE *f);

	void set_union(const LargeBitfield &other) {
		CUSTOM_ASSERT(_num_elements == other._num_elements);
		int N = _num_elements;
		for (int k=0; k<N; ++k) {
			_state[k]|=other._state[k];
		}

	}
};

inline void readFile(const std::string &filename, std::vector<uint8_t> &result) {
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

struct StringBuilder {
private:
	char _backing[4096] = "";
	int _length = 0;
public:
	void column(int w, int step=4) {
		while (w<_length) {
			w+=step;
		}
		int start = _length;
		_length = w;
		for (int i=start; i <_length; i++) _backing[i]=' ';
		_backing[_length]='\0';
	}
	void clear() { _length = 0; _backing[0]='\0'; }
	bool empty() const { return _length == 0; }
	int length() const { return _length; }
	void add(const char * const str) { 
		int s = (int)strlen(str);
		memcpy(_backing + _length, str, s+1);
		_length += s;
	}
	void add(const std::string &str) { 
		int s = (int)str.length();
		memcpy(_backing + _length, str.c_str(), s+1);
		_length += s;
	}
	void format(const char *fmt, ...) {
		va_list args;
		va_start(args, fmt);
		int val = vsprintf (_backing + _length, fmt, args);
		assert(val>=0);
		_length += val;
		assert(_length< 4096);
		va_end(args);
	}

	const char *c_str() const { return _backing; }

	// Convenience
	template<typename A, typename B>
	void add(const A &a, const  B&b) { add(a); add(b); }
};

struct Range {
	int first, N, step;

	Range() { reset(); }

	void reset() { N = 0; first = -1; step = 0; }

	bool fits(int v) const {
		if (N==0||N==1) return true;
		if (first+N*step==v) return true; // accept next
		if (first+(N-1)*step==v) return true; // accept same as last as well
		return false;
	}
	void add(int v) {
		assert(fits(v));
		if (N==0) first = v;
		if (N==1) { step = v - first; }
		if (first+step*N == v) N++; // Don't increase if we got same
	}
	void format(StringBuilder &s) {
		if (N==0) {
		} else if (N<4) {
			for (int k=0; k<N-1; k++) {
				s.format("%X, ", first+step*k);
			}
			s.format("%X", first+step*(N-1));
		} else {
			int last = first + step * N;
			s.format("<%X, %X, ..., %X>", first, first + step, last);
		}
	}
};

/*
	A block based vector. Only support push_back and [], not erase etc.
	Helps keeping address space from getting fragmented when a vector grows, especially in 32-bit programs.
	Currently uses 2MB blocks.
*/
template<typename T, int ELEMENTS_PER_BLOCK=2*1024*1024/sizeof(T)>
struct BlockVector {
public:
	~BlockVector() {
		for (T* t: _blocks) {
			delete[] t;
		}
	}
	void push_back(const T &t) {
		if (_current_block == nullptr || _num_entries_last_block == ELEMENTS_PER_BLOCK) {
			_current_block = new T[ELEMENTS_PER_BLOCK];
			_blocks.push_back(_current_block);
			_num_entries_last_block = 0;
		}
		_current_block[_num_entries_last_block++] = t;
		_total_entries++;
	}
	T& push_back() {
		T t;
		if (_current_block == nullptr || _num_entries_last_block == ELEMENTS_PER_BLOCK) {
			_current_block = new T[ELEMENTS_PER_BLOCK];
			_blocks.push_back(_current_block);
			_num_entries_last_block = 0;
		}
		_current_block[_num_entries_last_block++] = t;
		_total_entries++;
		return last();
	}
	const T &operator[](const uint32_t idx) const {
		CUSTOM_ASSERT(idx < _total_entries);
		uint32_t block_index = idx / ELEMENTS_PER_BLOCK;
		return _blocks[block_index][idx - block_index*ELEMENTS_PER_BLOCK];	
	}
	T &operator[](const uint32_t idx) {
		CUSTOM_ASSERT(idx < _total_entries);
		uint32_t block_index = idx / ELEMENTS_PER_BLOCK;
		return _blocks[block_index][idx - block_index*ELEMENTS_PER_BLOCK];	
	}
	const T& last() const { CUSTOM_ASSERT(!empty()); return this->operator[](_total_entries-1); }
	T& last() { CUSTOM_ASSERT(!empty()); return this->operator[](_total_entries-1); }

	bool empty() const { return _total_entries == 0; }
	uint32_t size() const { return _total_entries; }
private:
	uint32_t _total_entries = 0;
	uint32_t _num_entries_last_block = 0;
	T* _current_block = nullptr;
	std::vector<T*> _blocks;
};

/*
	fgetpos/fsetpos are annoying so I made this workaround
	See: https://stackoverflow.com/questions/8875304/accessing-large-files-in-c
	I opted for this stupid solution since it fit my need perfectly.
	In most cases I will never need to do more than one seek.

	Note that set_offset during writing is not thought through
*/
struct BigFile {
	uint64_t _offset = 0;
	FILE *_file = nullptr;
	void set_offset(uint64_t offset) {
		validate();
		if (offset == _offset)
			return;
		if (offset < _offset) {
			// We can't move back from SEEK_CUR so lets rewind and start over
			rewind(_file);
			_offset = 0;
		}
		// TODO: To catch us from doing a too large jump because of broken file content we should check for EOF
		uint64_t delta = offset - _offset;
		while (delta>0) {
			uint64_t step = delta <= 0x80000000ULL ? delta : 0x80000000ULL;
			fseek(_file, (int32_t)step, SEEK_CUR);
			delta -= step;
		}
		_offset = offset;
		validate();
	}
	inline uint64_t read(void *buffer, uint64_t len) {
		uint64_t r = fread(buffer, 1, len, _file);
		_offset += r;
		validate();
		return r;
	}
	inline uint64_t write(const void * const buffer, uint64_t len) {
		uint64_t written = fwrite(buffer, 1, len, _file);
		_offset += written;
		validate();
		return written;
	}
	template<typename T>
	inline uint64_t write(const T &t) {
		return write(&t, sizeof(T));
	}
	template<typename T>
	inline uint64_t read(T &t) {
		return read(&t, sizeof(T));
	}
private:
	void validate() {
		CUSTOM_ASSERT((_offset > (1<<30)) || _offset == ftell(_file));
	}
};

namespace snestistics {
	uint32_t hash_insecure(const uint8_t* key, size_t len, uint32_t seed);
}
