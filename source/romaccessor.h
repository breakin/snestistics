#pragma once

#include "utils.h"

/*
	This class is responsible for resolving a pointer/adress to a byte in the ROM.
	It obviously can't resolve RAM.

	Currently it only supports LoROM and no FastROM.
*/

class RomAccessor {
private:
	uint32_t m_romOffset, m_calcSize;
	std::vector<uint8_t> m_romdata;
public:
	RomAccessor(const uint32_t romOffset, const uint32_t calcSize) : m_romOffset(romOffset), m_calcSize(calcSize) {}
	void load(const std::string &filename) {
		readFile(filename, m_romdata);

		if (m_romOffset == -1) {
			// Auto-detect assumes nothing is appended at end of ROM
			uint32_t header_size = m_romdata.size() % (32*1024);
			m_romOffset = header_size;
		}
		if (m_calcSize == -1) {
			m_calcSize = (((uint32_t)m_romdata.size() - m_romOffset)/0x2000)*0x2000;
		}
		printf("calculated size = %06X, rom_offset = %06X\n", m_calcSize, m_romOffset);
	}

	uint8_t evalByte(const Pointer p) const {
		return m_romdata[m_romOffset + getRomOffset(p, m_calcSize)];
	}

	// NOTE: Assumes it stays within bank etc
	const uint8_t* evalPtr(const Pointer p) const {
		return &m_romdata[m_romOffset + getRomOffset(p, m_calcSize)];
	}

	static bool is_rom(const Pointer p) {
		uint8_t b = bank(p);
		if (b==0x7E || b== 0x7F) return false;
		uint16_t a = adr(p);
		if (a >= 0x8000) return true;
		return false;
	}

private:
	static uint8_t bank(const Pointer p) { return p >> 16; }
	static uint16_t adr(const Pointer p) { return p & 0xffff; }

	static size_t getRomOffset(const Pointer &pointer, const uint32_t calculatedSize) {
		const uint32_t a = (bank(pointer) & 0x7f) * 0x8000;
		const uint32_t mirrorAddr = map_mirror(calculatedSize, a);
		return mirrorAddr - (adr(pointer) & 0x8000) + adr(pointer);
	}

	// Unpacked relative offset for branch operations
	// TODO: Move to shared space
	static int unpackSigned(const uint8_t packed) {
		if ((packed >= 0x80) != 0) {
			return packed - 256;
		}
		else {
			return packed;
		}
	}

	// This function describes how ROM is repeated when ROM is smaller than adress space
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

