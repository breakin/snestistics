#pragma once

#include <vector>
#include <string>
#include "utils.h"
#include "options.h"

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
	RomAccessor(const Options::RomHeaderEnum rom_mode, const uint32_t calcSize) : m_romOffset(-1), m_calcSize(calcSize) {
		if (rom_mode == Options::RH_AUTO) m_romOffset = -1;
		if (rom_mode == Options::RH_NONE) m_romOffset = 0;
		if (rom_mode == Options::RH_COPIER) m_romOffset = 512;
	}

	void load(const std::string &filename) {
		readFile(filename, m_romdata);

		if (m_romOffset == -1) {
			// Auto-detect assumes nothing is appended at end of ROM
			uint32_t header_size = m_romdata.size() % (32*1024);
			m_romOffset = header_size;
		}
		if (m_calcSize == 0) {
			m_calcSize = (((uint32_t)m_romdata.size() - m_romOffset)/0x2000)*0x2000;
		}
		printf("calculated size = 0x%06X, rom_offset = 0x%06X\n", m_calcSize, m_romOffset);
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

	Pointer lorom_bank_remap(const Pointer resolve_address) const {
		uint16_t adr = resolve_address & 0xffff;
		if (adr >= 0x8000) return resolve_address;

		uint8_t bank = resolve_address >> 16;
		if ((bank >= 0x00 && bank <= 0x3F) || (bank >= 0x80 && bank <= 0xBF)) {
			if (adr < 0x2000) {
				return 0x7E0000|adr;
			} else  {
				// A bit of a hack. Make all memory mapped adresses go to bank 0 so we can annotate them once
				return adr; // Strip bank from resolve_adress
			}
		} else {
			return resolve_address;
		}
	}

private:
	static uint8_t bank(const Pointer p) { return p >> 16; }
	static uint16_t adr(const Pointer p) { return p & 0xffff; }

	static size_t getRomOffset(const Pointer &pointer, const uint32_t calculatedSize) {
		const uint32_t a = (bank(pointer) & 0x7f) * 0x8000;
		const uint32_t mirrorAddr = map_mirror(calculatedSize, a);
		return mirrorAddr - (adr(pointer) & 0x8000) + adr(pointer);
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

	inline bool is_cartridge_sram(uint8_t bank, uint16_t adr) const {
		if (adr >= 0x8000) return false;
		if (bank >= 0x70 && bank <= 0x7D) return true;
		if (bank >= 0xF0 && bank <= 0xFf) return true;
		return false;
	}

	inline bool is_ram(uint8_t bank, uint16_t adr) const {
		if (bank >= 0x7E && bank <= 0x7F) return true;
		if (adr >= 0x2000) return false;
		if (bank >= 0x00 && bank <= 0x3f) return true;
		if (bank >= 0x80 && bank <= 0xBF) return true;
		return false;
	}

	inline bool is_rom(uint8_t bank, uint16_t adr) const {
		if (adr < 0x8000) return false;
		if (bank == 0x7E || bank == 0x7F) return false;
		return true;
	}
};

