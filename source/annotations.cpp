#include "annotations.h"
#include <map>
#include <algorithm>
#include <sstream>
#include "utils.h"
#include <fstream>
#include "rom_accessor.h"

struct DataMMIO {
	uint32_t address;
	const char * const name;
	const char * const use_comment;
};

// These are not gotten from https://en.wikibooks.org/wiki/Super_NES_Programming/SNES_Hardware_Registers
// TODO: Update these description. Some are a bit too vague. Add LO/HI to all word-sized.
static DataMMIO mmio_annotations[] = {
	{ 0x002100, "REG_INIDISP", "Screen Display Register" },
	{ 0x002101, "REG_OBJSEL", "Object Size and Character Size Register" },
	{ 0x002102, "REG_OAMADDL", "OAM Address Registers (Low)" },
	{ 0x002103, "REG_OAMADDH", "OAM Address Registers (High)" },
	{ 0x002104, "REG_OAMDATA", "OAM Data Write Register" },
	{ 0x002105, "REG_BGMODE", "BG Mode and Character Size Register" },
	{ 0x002106, "REG_MOSAIC", "Mosaic Register" },
	{ 0x002107, "REG_BG1SC", "BG Tilemap Address Registers (BG1)" },
	{ 0x002108, "REG_BG2SC", "BG Tilemap Address Registers (BG2)" },
	{ 0x002109, "REG_BG3SC", "BG Tilemap Address Registers (BG3)" },
	{ 0x00210A, "REG_BG4SC", "BG Tilemap Address Registers (BG4)" },
	{ 0x00210B, "REG_BG12NBA", "BG Character Address Registers (BG1&2)" },
	{ 0x00210C, "REG_BG34NBA", "BG Character Address Registers (BG3&4)" },
	{ 0x00210D, "REG_BG1HOFS", "BG Scroll Registers (BG1 vertical)" },
	{ 0x00210E, "REG_BG1VOFS", "BG Scroll Registers (BG1 horizontal)" },
	{ 0x00210F, "REG_BG2HOFS", "BG Scroll Registers (BG2 vertical)" },
	{ 0x002110, "REG_BG2VOFS", "BG Scroll Registers (BG2 horizontal)" },
	{ 0x002111, "REG_BG3HOFS", "BG Scroll Registers (BG3 vertical)" },
	{ 0x002112, "REG_BG3VOFS", "BG Scroll Registers (BG3 horizontal)" },
	{ 0x002113, "REG_BG4HOFS", "BG Scroll Registers (BG4 vertical)" },
	{ 0x002114, "REG_BG4VOFS", "BG Scroll Registers (BG4 horizontal)" },
	{ 0x002115, "REG_VMAIN", "Video Port Control Register" },
	{ 0x002116, "REG_VMADDL", "VRAM Address Registers (Low)" },
	{ 0x002117, "REG_VMADDH", "VRAM Address Registers (High)" },
	{ 0x002118, "REG_VMDATAL", "VRAM Data Write Registers (Low)" },
	{ 0x002119, "REG_VMDATAH", "VRAM Data Write Registers (High)" },
	{ 0x00211A, "REG_M7SEL", "Mode 7 Settings Register" },
	{ 0x00211B, "REG_M7A", "Mode 7 Settings Register" },
	{ 0x00211C, "REG_M7B", "Mode 7 Settings Register" },
	{ 0x00211D, "REG_M7C", "Mode 7 Settings Register" },
	{ 0x00211E, "REG_M7D", "Mode 7 Settings Register" },
	{ 0x00211F, "REG_M7X", "Mode 7 Settings Register" },
	{ 0x002120, "REG_M7Y", "Mode 7 Settings Register" },
	{ 0x002121, "REG_CGADD", "CGRAM Address Register" },
	{ 0x002122, "REG_CGDATA", "CGRAM Data Write Register" },
	{ 0x002123, "REG_W12SEL", "Window Mask Settings Registers" },
	{ 0x002124, "REG_W34SEL", "Window Mask Settings Registers" },
	{ 0x002125, "REG_WOBJSEL", "Window Mask Settings Registers" },
	{ 0x002126, "REG_WH0", "Window Position Registers" },
	{ 0x002127, "REG_WH1", "Window Position Registers" },
	{ 0x002128, "REG_WH2", "Window Position Registers" },
	{ 0x002129, "REG_WH3", "Window Position Registers" },
	{ 0x00212A, "REG_WBGLOG", "Window Mask Logic registers (BG)" },
	{ 0x00212B, "REG_WOBJLOG", "Window Mask Logic registers (OBJ)" },
	{ 0x00212C, "REG_TM", "Screen Destination Registers" },
	{ 0x00212D, "REG_TS", "Screen Destination Registers" },
	{ 0x00212E, "REG_TMW", "Window Mask Destination Registers" },
	{ 0x00212F, "REG_TSW", "Window Mask Destination Registers" },
	{ 0x002130, "REG_CGWSEL", "Color Math Registers" },
	{ 0x002131, "REG_CGADSUB", "Color Math Registers" },
	{ 0x002132, "REG_COLDATA", "Color Math Registers" },
	{ 0x002133, "REG_SETINI", "Screen Mode Select Register" },
	{ 0x002134, "REG_MPYL", "Multiplication Result Registers" },
	{ 0x002135, "REG_MPYM", "Multiplication Result Registers" },
	{ 0x002136, "REG_MPYH", "Multiplication Result Registers" },
	{ 0x002137, "REG_SLHV", "Software Latch Register" },
	{ 0x002138, "REG_OAMDATAREAD", "OAM Data Read Register" },
	{ 0x002139, "REG_VMDATALREADL", "VRAM Data Read Register (Low)" },
	{ 0x00213A, "REG_VMDATAHREADH", "VRAM Data Read Register (High)" },
	{ 0x00213B, "REG_CGDATAREAD", "CGRAM Data Read Register" },
	{ 0x00213C, "REG_OPHCT", "Scanline Location Registers (Horizontal)" },
	{ 0x00213D, "REG_OPVCT", "Scanline Location Registers (Vertical)" },
	{ 0x00213E, "REG_STAT77", "PPU Status register" },
	{ 0x00213F, "REG_STAT78", "PPU Status register" },
	{ 0x002140, "REG_APUIO0", "APU IO Registers" },
	{ 0x002141, "REG_APUIO1", "APU IO Registers" },
	{ 0x002142, "REG_APUIO2", "APU IO Registers" },
	{ 0x002143, "REG_APUIO3", "APU IO Registers" },
	{ 0x002180, "REG_WMDATA", "WRAM Data Register" },
	{ 0x002181, "REG_WMADDL", "WRAM Address Registers (Low)" },
	{ 0x002182, "REG_WMADDM", "WRAM Address Registers (High)" },
	{ 0x002183, "REG_WMADDH", "WRAM Address Registers (Bank)" },
	{ 0x004016, "REG_JOYSER0", "Old Style Joypad Registers 0" },
	{ 0x004017, "REG_JOYSER1", "Old Style Joypad Registers 1" },
	{ 0x004200, "REG_NMITIMEN", "Interrupt Enable Register" },
	{ 0x004201, "REG_WRIO", "IO Port Write Register" },
	{ 0x004202, "REG_WRMPYA", "Multiplicand Registers" },
	{ 0x004203, "REG_WRMPYB", "Multiplicand Registers" },
	{ 0x004204, "REG_WRDIVL", "Divisor & Dividend Registers" },
	{ 0x004205, "REG_WRDIVH", "Divisor & Dividend Registers" },
	{ 0x004206, "REG_WRDIVB", "Divisor & Dividend Registers" },
	{ 0x004207, "REG_HTIMEL", "IRQ Timer Registers Horizontal (Low)" },
	{ 0x004208, "REG_HTIMEH", "IRQ Timer Registers Horizontal (High)" },
	{ 0x004209, "REG_VTIMEL", "IRQ Timer Registers Vertical (Low)" },
	{ 0x00420A, "REG_VTIMEH", "IRQ Timer Registers Vertical (High)" },
	{ 0x00420B, "REG_MDMAEN", "DMA Enable Register" },
	{ 0x00420C, "REG_HDMAEN", "HDMA Enable Register" },
	{ 0x00420D, "REG_MEMSEL", "ROM Speed Register" },
	{ 0x004210, "REG_RDNMI", "NMI Enable" },
	{ 0x004211, "REG_TIMEUP", "IRQ Flag By H/V Count Timer" },
	{ 0x004212, "REG_FLAGS", "H/V Blank Flags and Joypad Status" },
	{ 0x004213, "REG_RDIO", "IO Port Read Register" },
	{ 0x004214, "REG_RDDIVL", "Multiplication Or Divide Result Registers (Low)" },
	{ 0x004215, "REG_RDDIVH", "Multiplication Or Divide Result Registers (High)" },
	{ 0x004216, "REG_RDMPYL", "Multiplication Or Divide Result Registers (Low)" },
	{ 0x004217, "REG_RDMPYH", "Multiplication Or Divide Result Registers (High)" },
	{ 0x004218, "REG_JOY1L", "Controller Port Data Registers (Pad 1 - Low)" },
	{ 0x004219, "REG_JOY1H", "Controller Port Data Registers (Pad 1 - High)" },
	{ 0x00421A, "REG_JOY2L", "Controller Port Data Registers (Pad 2 - Low)" },
	{ 0x00421B, "REG_JOY2H", "Controller Port Data Registers (Pad 2 - High)" },
	{ 0x00421C, "REG_JOY3L", "Controller Port Data Registers (Pad 3 - Low)" },
	{ 0x00421D, "REG_JOY3H", "Controller Port Data Registers (Pad 3 - High)" },
	{ 0x00421E, "REG_JOY4L", "Controller Port Data Registers (Pad 4 - Low)" },
	{ 0x00421F, "REG_JOY4H", "Controller Port Data Registers (Pad 4 - High)" },
	{ 0x004300, "REG_DMAP0", "(H)DMA Control Register 0" },
	{ 0x004301, "REG_BBAD0", "(H)DMA Destination Register 0" },
	{ 0x004302, "REG_A1T0L", "(H)DMA Source Address Registers 0" },
	{ 0x004303, "REG_A1T0H", "(H)DMA Source Address Registers 0" },
	{ 0x004304, "REG_A1B0", "(H)DMA Source Address Registers 0" },
	{ 0x004305, "REG_DAS0L", "DMA Size Registers / HDMA Indirect Address Registers 0" },
	{ 0x004306, "REG_DAS0H", "DMA Size Registers / HDMA Indirect Address Registers 0" },
	{ 0x004307, "REG_DASB0", "HDMA Indirect Address Registers (Bank) 0" },
	{ 0x004308, "REG_A2A0L", "HDMA Mid Frame Table Address Registers 0" },
	{ 0x004309, "REG_A2A0H", "HDMA Mid Frame Table Address Registers0" },
	{ 0x00430A, "REG_NTLR0", "HDMA Line Counter Register 0" },
	{ 0x004310, "REG_DMAP1", "(H)DMA Control Register 1" },
	{ 0x004311, "REG_BBAD1", "(H)DMA Destination Register 1" },
	{ 0x004312, "REG_A1T1L", "(H)DMA Source Address Registers 1" },
	{ 0x004313, "REG_A1T1H", "(H)DMA Source Address Registers 1" },
	{ 0x004314, "REG_A1B1", "(H)DMA Source Address Registers 1" },
	{ 0x004315, "REG_DAS1L", "DMA Size Registers / HDMA Indirect Address Registers 1" },
	{ 0x004316, "REG_DAS1H", "DMA Size Registers / HDMA Indirect Address Registers 1" },
	{ 0x004317, "REG_DASB1", "HDMA Indirect Address Registers (Bank) 1" },
	{ 0x004318, "REG_A2A1L", "HDMA Mid Frame Table Address Registers 1" },
	{ 0x004319, "REG_A2A1H", "HDMA Mid Frame Table Address Registers 1" },
	{ 0x00431A, "REG_NTLR1", "HDMA Line Counter Register 1" },
	{ 0x004320, "REG_DMAP2", "(H)DMA Control Register 2" },
	{ 0x004321, "REG_BBAD2", "(H)DMA Destination Register 2" },
	{ 0x004322, "REG_A1T2L", "(H)DMA Source Address Registers 2" },
	{ 0x004323, "REG_A1T2H", "(H)DMA Source Address Registers 2" },
	{ 0x004324, "REG_A1B2", "(H)DMA Source Address Registers 2" },
	{ 0x004325, "REG_DAS2L", "DMA Size Registers / HDMA Indirect Address Registers 2" },
	{ 0x004326, "REG_DAS2H", "DMA Size Registers / HDMA Indirect Address Registers 2" },
	{ 0x004327, "REG_DASB2", "HDMA Indirect Address Registers (Bank) 2" },
	{ 0x004328, "REG_A2A2L", "HDMA Mid Frame Table Address Registers 2" },
	{ 0x004329, "REG_A2A2H", "HDMA Mid Frame Table Address Registers 2" },
	{ 0x00432A, "REG_NTLR2", "HDMA Line Counter Register 2" },
	{ 0x004330, "REG_DMAP3", "(H)DMA Control Register 3" },
	{ 0x004331, "REG_BBAD3", "(H)DMA Destination Register 3" },
	{ 0x004332, "REG_A1T3L", "(H)DMA Source Address Registers 3" },
	{ 0x004333, "REG_A1T3H", "(H)DMA Source Address Registers 3" },
	{ 0x004334, "REG_A1B3", "(H)DMA Source Address Registers 3" },
	{ 0x004335, "REG_DAS3L", "DMA Size Registers / HDMA Indirect Address Registers 3" },
	{ 0x004336, "REG_DAS3H", "DMA Size Registers / HDMA Indirect Address Registers 3" },
	{ 0x004337, "REG_DASB3", "HDMA Indirect Address Registers (Bank) 3" },
	{ 0x004338, "REG_A2A3L", "HDMA Mid Frame Table Address Registers 3" },
	{ 0x004339, "REG_A2A3H", "HDMA Mid Frame Table Address Registers 3" },
	{ 0x00433A, "REG_NTLR3", "HDMA Line Counter Register 3" },
	{ 0x004340, "REG_DMAP4", "(H)DMA Control Register 4" },
	{ 0x004341, "REG_BBAD4", "(H)DMA Destination Register 4" },
	{ 0x004342, "REG_A1T4L", "(H)DMA Source Address Registers 4" },
	{ 0x004343, "REG_A1T4H", "(H)DMA Source Address Registers 4" },
	{ 0x004344, "REG_A1B4", "(H)DMA Source Address Registers 4" },
	{ 0x004345, "REG_DAS4L", "DMA Size Registers / HDMA Indirect Address Registers 4" },
	{ 0x004346, "REG_DAS4H", "DMA Size Registers / HDMA Indirect Address Registers 4" },
	{ 0x004347, "REG_DASB4", "HDMA Indirect Address Registers (Bank) 4" },
	{ 0x004348, "REG_A2A4L", "HDMA Mid Frame Table Address Registers 4" },
	{ 0x004349, "REG_A2A4H", "HDMA Mid Frame Table Address Registers 4" },
	{ 0x00434A, "REG_NTLR4", "HDMA Line Counter Register 4" },
	{ 0x004350, "REG_DMAP5", "(H)DMA Control Register 5" },
	{ 0x004351, "REG_BBAD5", "(H)DMA Destination Register 5" },
	{ 0x004352, "REG_A1T5L", "(H)DMA Source Address Registers 5" },
	{ 0x004353, "REG_A1T5H", "(H)DMA Source Address Registers 5" },
	{ 0x004354, "REG_A1B5", "(H)DMA Source Address Registers 5" },
	{ 0x004355, "REG_DAS5L", "DMA Size Registers / HDMA Indirect Address Registers 5" },
	{ 0x004356, "REG_DAS5H", "DMA Size Registers / HDMA Indirect Address Registers 5" },
	{ 0x004357, "REG_DASB5", "HDMA Indirect Address Registers (Bank) 5" },
	{ 0x004358, "REG_A2A5L", "HDMA Mid Frame Table Address Registers 5" },
	{ 0x004359, "REG_A2A5H", "HDMA Mid Frame Table Address Registers 5" },
	{ 0x00435A, "REG_NTLR5", "HDMA Line Counter Register 5" },
	{ 0x004360, "REG_DMAP6", "(H)DMA Control Register 6" },
	{ 0x004361, "REG_BBAD6", "(H)DMA Destination Register 6" },
	{ 0x004362, "REG_A1T6L", "(H)DMA Source Address Registers 6" },
	{ 0x004363, "REG_A1T6H", "(H)DMA Source Address Registers 6" },
	{ 0x004364, "REG_A1B6", "(H)DMA Source Address Registers 6" },
	{ 0x004365, "REG_DAS6L", "DMA Size Registers / HDMA Indirect Address Registers 6" },
	{ 0x004366, "REG_DAS6H", "DMA Size Registers / HDMA Indirect Address Registers 6" },
	{ 0x004367, "REG_DASB6", "HDMA Indirect Address Registers (Bank) 6" },
	{ 0x004368, "REG_A2A6L", "HDMA Mid Frame Table Address Registers 6" },
	{ 0x004369, "REG_A2A6H", "HDMA Mid Frame Table Address Registers 6" },
	{ 0x00436A, "REG_NTLR6", "HDMA Line Counter Register 6" },
	{ 0x004370, "REG_DMAP7", "(H)DMA Control Register 7" },
	{ 0x004371, "REG_BBAD7", "(H)DMA Destination Register 7" },
	{ 0x004372, "REG_A1T7L", "(H)DMA Source Address Registers 7" },
	{ 0x004373, "REG_A1T7H", "(H)DMA Source Address Registers 7" },
	{ 0x004374, "REG_A1B7", "(H)DMA Source Address Registers 7" },
	{ 0x004375, "REG_DAS7L", "DMA Size Registers / HDMA Indirect Address Registers 7" },
	{ 0x004376, "REG_DAS7H", "DMA Size Registers / HDMA Indirect Address Registers 7" },
	{ 0x004377, "REG_DASB7", "HDMA Indirect Address Registers (Bank) 7" },
	{ 0x004378, "REG_A2A7L", "HDMA Mid Frame Table Address Registers 7" },
	{ 0x004379, "REG_A2A7H", "HDMA Mid Frame Table Address Registers 7" },
	{ 0x00437A, "REG_NTLR7", "HDMA Line Counter Register 7" }
};

void AnnotationResolver::add_mmio_annotations() {
	uint32_t num_mmio_annotations = sizeof(mmio_annotations)/sizeof(DataMMIO);
	uint32_t first = (uint32_t)_annotations.size();
	_annotations.resize(first + num_mmio_annotations);
	for (uint32_t i = 0, offset = first; i<num_mmio_annotations; ++i, ++offset) {
		const DataMMIO &d = mmio_annotations[i];
		Annotation a;
		a.type = AnnotationType::ANNOTATION_DATA;
		a.name = d.name;
		a.comment_is_multiline = false;
		a.startOfRange = a.endOfRange = d.address;
		a.useComment = d.use_comment;
		_annotations[offset] = a;
	}
}

void AnnotationResolver::add_vector_comment(const RomAccessor &rom, const char * const comment, const uint16_t target_at) {
	uint16_t target = *(uint16_t*)rom.evalPtr(target_at);
	Annotation a;
	a.type = ANNOTATION_LINE;
	a.startOfRange = a.endOfRange = target;
	a.comment = comment;
	a.comment_is_multiline = false;
	_annotations.push_back(a);
}

void AnnotationResolver::add_vector_annotations(const RomAccessor &rom) {
	add_vector_comment(rom, "Vector: Native COP",       0xFFE4);
	add_vector_comment(rom, "Vector: Native BRK",       0xFFE6);
	add_vector_comment(rom, "Vector: Native ABORT",     0xFFE8);
	add_vector_comment(rom, "Vector: Native NMI",       0xFFEA);
	add_vector_comment(rom, "Vector: Native IRQ",       0xFFEE);
	add_vector_comment(rom, "Vector: Emulation COP",    0xFFF4);
	add_vector_comment(rom, "Vector: Emulation ABORT",  0xFFF8);
	add_vector_comment(rom, "Vector: Emulation NMI",    0xFFFA);
	add_vector_comment(rom, "Vector: Emulation RESET",  0xFFFC);
	add_vector_comment(rom, "Vector: Emulation IRQBRK", 0xFFFE);
}

void AnnotationResolver::load(std::istream &input, const std::string &error_file) {
	std::string comment;
	std::string useComment;
	Annotation::TraceType trace_type = Annotation::TRACETYPE_DEFAULT;
	bool comment_is_multiline = false;

	char name[1024]="";
	char mycomment[1024];

	Pointer start;

	int size;

	std::istream &f = input;

	int line_number = 0;
	
	while (!f.eof()) {
		line_number++;
		char buf[4096];
		f.getline(buf, 4096);

		size = 0;

		if (strlen(buf) == 0) {
			continue;
		}
		if (buf[0] == '@') {
			// meta-comment
			continue;
		} else if (buf[0] == ';') {
			const char *stripped = &buf[2];
			if (buf[1]=='\0') {
				stripped = "";
			} else if (buf[1] != ' ') {
				stripped = &buf[1];
			}
			if (!comment.empty()) {
				comment = comment + "\n" + stripped;
				comment_is_multiline = true;
			}
			else {
				comment = stripped;
			}
		}
		else if (buf[0] == '#') {
			useComment = &buf[2];
		} else if (sscanf(buf, "trace %06X %s %d", &start, (char*)&name, &size) > 0) {
			TraceAnnotation ta;
			if (strcmp(name, "push_return")==0) {
				ta.type = TraceAnnotation::PUSH_RETURN;
			} else if (strcmp(name, "pop_return")==0) {
				ta.type = TraceAnnotation::POP_RETURN;
			} else if (strcmp(name, "jmp_is_jsr")==0) {
				ta.type = TraceAnnotation::JMP_IS_JSR;
			}
			ta.location = start;
			ta.optional_parameter = size;
			_trace_annotations.push_back(ta);
			continue;
		} else if (sscanf(buf, "trace %s", (char*)&name) > 0) {
			if (strcmp(name, "ignore")==0) trace_type = Annotation::TRACETYPE_IGNORE;
		} else {

			Annotation a;

			if (sscanf(buf, "function %06X %06X %s", &a.startOfRange, &a.endOfRange, name) > 0) {
				a.type = ANNOTATION_FUNCTION;
				const bool convert_single_byte_functions_to_labels = false;
				if (convert_single_byte_functions_to_labels && a.startOfRange == a.endOfRange) {
					a.type = ANNOTATION_LINE;
					printf("Function '%s' is only one byte long, treating as 'label' instead!\n", name);
					a.endOfRange = a.startOfRange;
				}
			} else if (sscanf(buf, "line %06X", &a.startOfRange) > 0) { // TODO: Does this makes sense?
				a.type = ANNOTATION_LINE;
				a.endOfRange = a.startOfRange;
			} else if (sscanf(buf, "comment %06X \"%[^\"]", &a.startOfRange, mycomment) > 0) {
				a.type = ANNOTATION_LINE;
				a.comment = mycomment;
				a.comment_is_multiline = false;
				a.useComment = "";
				a.endOfRange = a.startOfRange;
			} else if (sscanf(buf, "label %06X %s", &a.startOfRange, name) > 0) {
				a.type = ANNOTATION_LINE;
				a.endOfRange = a.startOfRange;
			} else if (sscanf(buf, "data %06X %06X %s", &a.startOfRange, &a.endOfRange, name) >0 ) {
				a.type = ANNOTATION_DATA;
			} else {
				printf("%s:%d: Not supported format!\n\t'%s'\n", error_file.c_str(), line_number, buf);
				exit(99);
			}

			a.useComment = useComment;
			a.comment = comment;
			a.comment_is_multiline = comment_is_multiline;
			a.trace_type = trace_type;
			a.name = name;
			_annotations.push_back(a);

			// Reset state
			comment.clear();
			useComment.clear();
			comment_is_multiline = false;
			name[0]='\0';
			trace_type = Annotation::TRACETYPE_DEFAULT;
		}
	}
}

void AnnotationResolver::finalize() {

	Profile profile("finalize annotations", true);

	Pointer largest_adress = 0;
	for (auto a : _annotations) {
		largest_adress = std::max(largest_adress, a.endOfRange);
	}

	uint32_t k = largest_adress + 1;
	_annotation_for_adress_size = k;
	_annotation_for_adress = new int[k];
	for (uint32_t i = 0; i < k; i++) _annotation_for_adress[i] = -1;

	const auto annotation_sort = [](const Annotation &a, const Annotation &c) {
		if (a.startOfRange != c.startOfRange) return a.startOfRange < c.startOfRange;
		return false;
	};

	// Sort on start address only. For the rest rely on the sort being stable so we get annotations in the order they were given
	std::stable_sort(_annotations.begin(), _annotations.end(), annotation_sort);

	// First pass. Merge line annotations, possibly into functions/labels. Remove stale annotations.
	// After this pass startOfRange is unique
	// TODO: We could do this during parsing IF we build _annotation_for_adress then. Need to make it suffiently big before knowing usage, that's all.
	Pointer merge_pc = INVALID_POINTER;
	int last_written = 0;
	for (int i=0; i<(int)_annotations.size(); ++i) {
		Annotation &a = _annotations[i];
		CUSTOM_ASSERT(a.type != ANNOTATION_NONE);
		Pointer start = a.startOfRange;
		if (start != merge_pc) {
			_annotations[last_written++] = a;
			merge_pc = start;
			continue;
		}
		CUSTOM_ASSERT(last_written!=0);
		Annotation &b = _annotations[last_written-1];

		// Is a mergeable with b (the merge for this startOfRange)?
		bool a_has_label = a.type == ANNOTATION_FUNCTION || a.type == ANNOTATION_DATA || (a.type == ANNOTATION_LINE && !a.name.empty());
		bool b_has_label = b.type == ANNOTATION_FUNCTION || b.type == ANNOTATION_DATA || (b.type == ANNOTATION_LINE && !b.name.empty());

		if (!a.useComment.empty() && !b.useComment.empty()) {
			printf("Annotation collision at %06X! Only one annotation can have a label for one address!\n", start);
			throw std::runtime_error("Annotation collision!");
		}
		if (a_has_label && b_has_label) {
			printf("Annotation collision at %06X! Only one annotation can have a use comment (# line) for one address!\n", start);
			throw std::runtime_error("Annotation collision!");
		}
		if (!a.comment.empty()) {
			if (b.comment.empty())
				b.comment = a.comment;
			else
				b.comment = b.comment + "\n" + a.comment;
		}
		b.comment_is_multiline = a.comment_is_multiline || b.comment_is_multiline || (!a.comment.empty() && !b.comment.empty());
		if (a_has_label)
			b.name = a.name;

		if (!a.useComment.empty())
			b.useComment = a.useComment;

		// This means to choose the one from the function/data
		b.endOfRange = std::max(a.endOfRange, b.endOfRange);
		a.type = ANNOTATION_NONE;
	}

	// Remove unused annotations at the end
	_annotations.resize(last_written);

	// TODO: This code was a bit lazy!
	for (int pass = 0; pass < 3; pass++) {
		AnnotationType pass_type = ANNOTATION_NONE;
		if (pass == 0) pass_type = ANNOTATION_LINE;
		if (pass == 1) pass_type = ANNOTATION_DATA;
		if (pass == 2) pass_type = ANNOTATION_FUNCTION;

		for (int i=0; i<(int)_annotations.size(); ++i) {
			Annotation &a = _annotations[i];
			if (a.type != pass_type)
				continue;

			Pointer start = a.startOfRange;
			for (uint32_t k=a.startOfRange; k <= a.endOfRange; ++k) {
				if (_annotation_for_adress[k] == -1) {
					// No previous annotation
					_annotation_for_adress[k] = i;
				} else if (_annotations[_annotation_for_adress[k]].type != pass_type) {
					// There was a previous annotation here with lower power so we can overwrite it
					_annotation_for_adress[k] = i;
				} else {
					const Annotation &b = _annotations[_annotation_for_adress[k]];
					printf("Annotation collision at %06X! Both %s [%06X-%06X] and %s [%06X-%06X]!\n", k, a.name.c_str(), a.startOfRange, a.endOfRange, b.name.c_str(), b.startOfRange, b.endOfRange);
					throw std::runtime_error("Annotation collision!");
				}
			}
		}
	}
	
	_trace_annotation_for_adress = new int[k];
	for (uint32_t i = 0; i < k; i++) _trace_annotation_for_adress[i] = -1;

	std::sort(_trace_annotations.begin(), _trace_annotations.end());
	for (uint32_t i = 0; i < _trace_annotations.size(); ++i) {
		const TraceAnnotation &ta = _trace_annotations[i];
		_trace_annotation_for_adress[ta.location] = i;
	}
}

static void write_comment(FILE *output, const std::string &comment) {
	bool dang = true;
	for (size_t i=0; i<comment.length(); i++) {
		if (dang) {
			fprintf(output, "; ");
			dang = false;
		}
		fprintf(output, "%c", comment[i]);
		if (comment[i]=='\n') {
			dang = true;
		}
	}
	fprintf(output, "\n");
}

void AnnotationResolver::line_info(const Pointer p, std::string * label_out, std::string * comment_out, std::string * use_comment_out, bool force_label) const {

	const Annotation *function, *data;
	const Annotation *line_annotation = resolve_annotation(p, &function, &data);

	if (line_annotation && line_annotation->startOfRange == p) {
		if (comment_out) *comment_out = line_annotation->comment;
		if (use_comment_out) *use_comment_out = line_annotation->useComment;
	}

	if (data && data->startOfRange == p) {
		if (label_out) *label_out = data->name;
		return;
	}
	else 	if (function && function->startOfRange == p) {
		if (label_out) *label_out = function->name;
		return;
	}

	std::string label;
	if (line_annotation) {
		label = line_annotation->name;
	}

	if (label.empty() && !force_label)
		return;

	// We are in global scope or inside a (function|data)-scope and user has requsted a label
	StringBuilder sb;
	bool has_scope = true;
	if (data) {
		sb.format("_%s_", data->name.c_str());
	}
	else if (function) {
		sb.format("_%s_", function->name.c_str());
	}
	else {
		has_scope = false;
	}

	if (label.empty() && !has_scope) {
		sb.format("label_%06X", p);
	}
	else if (label.empty() && has_scope) {
		sb.format("%06X", p);
	}
	else {
		sb.add(label);
	}

	if (label_out) *label_out = sb.c_str();
}

std::string AnnotationResolver::label(const Pointer p, std::string * use_comment, bool force) const {
	std::string l;
	line_info(p, &l, nullptr, use_comment, force);
	return l;
}

std::string AnnotationResolver::data_label(const Pointer p, std::string * use_comment, const bool force) const {
	assert(!use_comment || use_comment->empty());
	std::string l;
	line_info(p, &l, nullptr, use_comment, false);
	if (!l.empty() || !force)
		return l;
	char t[7];
	sprintf(t, "data_%0X", p);
	return std::string(t);
}

const Annotation * AnnotationResolver::resolve_annotation(Pointer resolve_adress, const Annotation ** function_scope, const Annotation ** data_scope) const {
	if (function_scope) *function_scope = nullptr;
	if (data_scope) *data_scope = nullptr;

	int start = -1;
	if (resolve_adress < _annotation_for_adress_size) {
		start = _annotation_for_adress[resolve_adress];
	}
	// This is based on init of function and data filling all addresses as belonging to them when there is no line info. So -1 means nothing, no scope...
	if (start == -1) {
		return nullptr;
	}

	const Annotation *line_annotation = &_annotations[start];

	const Annotation *found_data = nullptr;

	if (line_annotation && line_annotation->type == ANNOTATION_FUNCTION && line_annotation->startOfRange == resolve_adress) {
		if (function_scope)	*function_scope = line_annotation;
		return line_annotation;
	}
	else if (line_annotation && line_annotation->type == ANNOTATION_DATA) {
		found_data = line_annotation;
	}

	if (line_annotation->startOfRange != resolve_adress)
		line_annotation = nullptr;

	for (int k = start; k >= 0; --k) {
		const Annotation &a = _annotations[k];
		if (a.type == ANNOTATION_FUNCTION && a.endOfRange < resolve_adress)
			break;
		if (!found_data && a.type == ANNOTATION_DATA && a.startOfRange >= resolve_adress && a.endOfRange <= resolve_adress) found_data = &a;
		if (a.type == ANNOTATION_FUNCTION) {
			if (function_scope) *function_scope = &a;
			break;
		}
	}
	if (found_data && data_scope) {
		*data_scope = found_data;
	}
	return line_annotation;
}

void AnnotationResolver::load(const std::vector<std::string> & filenames) {
	for (auto f : filenames) {
		printf("Loading annotations from %s...\n", f.c_str());
		std::ifstream loader;
		loader.open(f.c_str(), std::ios::in);
		if (!loader.is_open()) {
			printf("Failed to open '%s'\n", f.c_str());
			throw std::runtime_error("Failed to open file!");
		}
		load(loader, f);
	}
	finalize();
}

const TraceAnnotation * AnnotationResolver::trace_annotation(const Pointer pc) const {
	if (pc >= _annotation_for_adress_size)
		return nullptr;
	int tai = _trace_annotation_for_adress[pc];
	if (tai == -1) return nullptr;
	return &_trace_annotations[tai];
}

