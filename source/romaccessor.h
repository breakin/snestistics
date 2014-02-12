#ifndef SNESTISTICS_ROMACCESSOR
#define SNESTISTICS_ROMACCESSOR

#include "utils.h"

class RomAccessor {
private:
	const uint32_t m_romOffset, m_calcSize;
	std::vector<uint8_t> m_romdata;
public:
	RomAccessor(const uint32_t romOffset, const uint32_t calcSize) : m_romOffset(romOffset), m_calcSize(calcSize) {}
	void load(const std::string &filename) {
		readFile(filename, m_romdata);
	}

	uint8_t evalByte(const Pointer p) const {
		return m_romdata[m_romOffset + getRomOffset(p, m_calcSize)];
	}

	// NOTE: Assumes it stays within bank etc
	const uint8_t* evalPtr(const Pointer p) const {
		return &m_romdata[m_romOffset + getRomOffset(p, m_calcSize)];
	}

private:
	static uint8_t bank(const Pointer p) { return p >> 16; }
	static uint16_t adr(const Pointer p) { return p & 0xffff; }

	static size_t getRomOffset(const Pointer &pointer, const size_t calculatedSize) {
		const uint32_t a = (bank(pointer) & 0x7f) * 0x8000;
		const uint32_t mirrorAddr = map_mirror(calculatedSize, a);
		return mirrorAddr - (adr(pointer) & 0x8000) + adr(pointer);
	}

	// Unpacked relative offset for branch operations
	static int unpackSigned(const uint8_t packed) {
		if ((packed >= 0x80) != 0) {
			return packed - 256;
		}
		else {
			return packed;
		}
	}

	// TODO: This function can probably be removed for 8mbit LoROM
	static uint32_t map_mirror(uint32_t size, uint32_t pos)
	{
		// from bsnes
		if (size == 0)
			return (0);
		if (pos < size)
			return (pos);

		uint32_t mask = 1 << 31;
		while (!(pos & mask))
			mask >>= 1;

		if (size <= (pos & mask))
			return (map_mirror(size, pos - mask));
		else
			return (mask + map_mirror(size - mask, pos - mask));
	}
};

#endif
