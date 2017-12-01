local settings = {
	ux_text      = true
	ux_sys       = true
	sys_dma      = true
	sys_unpack   = true
	sys_util     = true
	sys_lib      = true
	verbose      = true
	very_verbose = true
}

local lzn_source_start = 0;
local lzn_dest_start = 0;

function PB(replay) { return replay.pc()>>16; }
function PC(replay) { return replay.pc(); }
function PC_16(replay) { return replay.pc() & 0xFFFF; }

// a weird workaround for the lack of acall without seperate this
function acall_no_this(fn,a) {
	a.insert(0, "dummy"); // Not so nice to mutate a here
	return fn.acall(a);
}

function trace_log_parameter_printer(replay, report)
{
	local pc = replay.pc();
	local verbose = settings.verbose || settings.very_verbose;
	local very_verbose = settings.very_verbose;

	local p = @(...) print(acall_no_this(format, vargv));

	local function p_boxdef_struct(struct_ptr) {
		local struct_addr = replay.read_long(struct_ptr);
		p("boxdef_struct   = %06X",           struct_addr);
		p("::style         = %02X",           replay.read_byte(struct_addr+9));
		p("::box_pos       = %d,%d",          replay.read_byte(struct_addr+10), replay.read_byte(struct_addr+11));
		p("::boxdim_struct = %06X",           replay.read_long(struct_addr+6));
		if (verbose) {
			struct_addr = replay.read_long(struct_addr+6);
			p("  ::dim         = %d,%d",          replay.read_byte(struct_addr+0), replay.read_byte(struct_addr+1));
			p("  ::map_ptr     = %06X",           replay.read_long(struct_addr+2));
			p("  ::buf_ptr     = %06X",           replay.read_long(struct_addr+6));
		}
	};

	local function p_boxdim_struct(struct_ptr) {
		local struct_addr = replay.read_long(struct_ptr);
		p("boxdim_struct   = %06X",           struct_addr);
		p("::dim           = %d,%d",          replay.read_byte(struct_addr+0), replay.read_byte(struct_addr+1));
		if (verbose) {
			p("::map_ptr       = %06X",           replay.read_long(struct_addr+2));
			p("::buf_ptr       = %06X",           replay.read_long(struct_addr+6));
		}
	};

	local function p_data(src_ptr, length) {
		local max_length = 64;
		local len = length;
		if (len > max_length) len = max_length;
		local data_str = "";
		for (local i = 0; i < len; ++i) {
			local n = format("%02X", replay.read_byte(src_ptr + i));
			data_str = data_str + n; // TODO: Not good performance/memory wise
		}
		if (length == len) {
			p("data            = %s\n", data_str);
		} else {
			p("data            = %s...\n", data_str);
		}
	};

	//
	// loglevels::ux_text
	//
	if (settings.ux_text) {
		if (pc == 0x8789EC) {
			p("string_ptr      = %06X",           replay.read_long(0x7E002F));
			p_boxdef_struct(0x7E0F51);
		}
		if (pc == 0x879E01) {
			p("string_ptr      = %06X",           replay.read_long(0x7E002F));
			p("cursor_pos      = %d,%d",          replay.xl(), replay.xh());
			if (verbose) {
				p("tile_attr       = %04X",           replay.read_word(0x7E0F5D));
				p("empty_char      = %04X",           replay.read_word(0x7E0F5F));
			}
			p_boxdim_struct(0x7E0F54);
		}
		if (pc == 0x879A0C) {
			p("value           = %d",             replay.a());
			p("cursor_pos      = %d,%d",          replay.xl(), replay.xh());
			if (verbose) {
				p("tile_attr       = %04X",           replay.read_word(0x7E0F5D));
			}
			p_boxdim_struct(0x7E0F54);
		}
		if (pc == 0x8797D7) {
			p("tile_index      = %04X",           replay.a());
			p("cursor_pos      = %d,%d",          replay.xl(), replay.xh());
			if (verbose) {
				p("tile_attr       = %04X",           replay.read_word(0x7E0F5D));
			}
			p_boxdim_struct(0x7E0F54);
		}
		if (pc == 0x879C54) {
			p("value           = %d",             replay.a());
			p("cursor_pos      = %d,%d",          replay.xl(), replay.xh());
			if (verbose) {
				p("tile_attr       = %04X",           replay.read_word(0x7E0F5D));
			}
			p_boxdim_struct(0x7E0F54);
		}
		if (pc == 0x87C779) {
			p("value           = %d",             replay.read_long(0x7E002F));
			p("cursor_pos      = %d,%d",          replay.xl(), replay.xh());
			if (verbose) {
				p("tile_attr       = %04X",           replay.read_word(0x7E0F5D));
			}
			p_boxdim_struct(0x7E0F54);
		}
		/*
		if (pc == 0x8796E9) {
			p("source_ptr      = %06X",           replay.read_long(0x7E002F));
			p("tile#offs       = %04X",           replay.a());
			p("coord           = %d,%d",          replay.xl(), replay.xh());
			p("tile_attr       = %04X",           replay.read_word(0x7E0F5D));
			p_boxdim_struct(0x7E0F54);
		}
		*/

		if (pc == 0x87983C) {
			p("cursor_pos      = %d,%d",          replay.xl(), replay.xh());
			p_boxdim_struct(0x7E0F54);
		}
		if (pc == 0x87964A) {
			p("source_ptr      = %06X",           replay.read_long(0x7E002F));
			p("cursor_pos      = %d,%d",          replay.xl(), replay.xh());
			p("tile_attr       = %04X",           replay.read_word(0x7E0F5D));
			p_boxdim_struct(0x7E0F54);
		}

		if (pc == 0x83A7E9) {
			p("value           = %d",             replay.read_long(0x7E002F));
			p("dest            = %06X",           replay.read_long(0x7E003B));
		}

		// TXT_string_len
		if (pc == 0x879F4C) {
			p("string_ptr      = %06X",           replay.read_long(0x7E002F));
		}
		if (pc == 0x879F6F) {
			// result
			p("result          > %04X",           replay.a());
		}
	}

	//
	// loglevels::ux_sys
	//
	if (settings.ux_sys) {
		if (pc == 0x82EF64) {
			p("src_ptr         = %04X",     replay.x());
			local src_ptr = replay.read_long(0x7E0000 + replay.x());
			p("source          = %06X",     src_ptr);
			p("dest            = %06X",     replay.read_long(0x7E0044));
			local length = replay.a();
			p("length          = %04X",     length);
			p_data(src_ptr, length);
		}
		if (pc == 0x878BD2) {
			p("dest_offset     = %04X",     replay.x());
			p("skip_value      = %04X",     replay.read_word(0x7E0F61));
			p_boxdim_struct(0x7E0F54);
		}

		if (very_verbose && pc == 0x87885C) {
			p_boxdef_struct(0x7E0F51);
		}

		if (pc == 0x878D8A) {
			p("clear_char      = %04X",     replay.read_word(0x7E0F61));
			if (very_verbose) {
				p_boxdim_struct(0x7E0F54);
			}
		}
		if (pc == 0x878D92) {
			p("clear_char      = %04X",     replay.read_word(0x7E0F5F));
			if (very_verbose) {
				p_boxdim_struct(0x7E0F54);
			}
		}
		if (pc == 0x878D95) {
			p("clear_char      = %04X",     replay.a());
			p_boxdim_struct(0x7E0F54);
		}
		if (very_verbose && pc == 0x87883D) {
			p("new_size        = %d,%d",          replay.al(), replay.ah());
		}
		if (very_verbose && pc == 0x87BB7A) {
			p("new_size        = %d,%d",          replay.al(), replay.ah());
		}

		/*
		// MovB_Ptr0F51+9_to_A
		if (pc == 0x8786CB) {
			local param_prt = replay.read_long(0x7E0F51);
			p("param_ptr = %06X",      param_ptr);
			p("%02X -> A",             replay.read_byte(param_ptr+9));
		}
		// MovW_A_to_Ptr0F51
		if (pc == 0x87872F) {
			p("#%04X -> %06X",         replay.a(), replay.read_long(0x7E0F51));
		}
		// MovW_Ptr0F51+A_to_A
		if (pc == 0x878716) {
			p("#%04X -> A",            replay.read_word(replay.read_long(0x7E0F51)+0xA));
		}
		// MovL_0625+X_to_0F51
		if (pc == 0x87B0E1) {
			p("#%06X -> 7E0F51",       replay.read_long(0x7E0625+replay.x()));
		}
		// MovB_#40_to_PtrDB+Y__MovL_0F31_to_PtrDB+Y+3
		if (pc == 0x82F13D) {
			local dest_ptr = (replay.db() << 16) + replay.y();
			p("#40 -> %06X",           dest_ptr);
			p("#%06X -> %06X",         replay.read_long(0x7E0F31), dest_ptr+3);
		}
		*/
	}

	//
	// loglevels::sys_dma
	//
	if (settings.sys_dma) {
		if (verbose && pc == 0x80B4B6) {
			p("source          = %06X",          replay.read_long(0x7E002F));
			p("dest            = %04X",          replay.read_word(0x7E000D) << 1);
			p("length          = %04X",          replay.read_word(0x7E000B));
			p("queue_index     > %d",            replay.read_byte(0x7E0343) / 9);
		}
		if (verbose && pc == 0x80B503) {
			p("source          = %06X",          replay.read_long(0x7E002F));
			p("dest            = %04X",          replay.read_word(0x7E000D) << 1);
			p("length          = %04X",          replay.read_word(0x7E000B));
			p("queue_index     > %d",            replay.read_byte(0x7E0343) / 9);
		}
		if (verbose && pc == 0x80B3EB) {
			local inline_ptr = replay.read_long(0x7E0000 + replay.s() + 1) + 1;
			local command = replay.read_byte(inline_ptr);
			if (command) {
				p("command         = %02X",              command);
				p("source          = %06X",              replay.read_long(inline_ptr + 1));
				p("dest            = %04X",              replay.read_word(inline_ptr + 7) << 1);
				p("length          = %04X",              replay.read_word(inline_ptr + 4));
				p("inc_mode        = %02X",              replay.read_byte(inline_ptr + 6));
				p("queue_index     > %d",                replay.read_byte(0x7E0343) / 9);
			}
		}
		if (pc == 0x80B2FF) {
			local y = replay.y();
			p("queue_index     = %02X",          y / 9);
			p("source          = %06X",          replay.read_long(0x7E0347 + y));
			p("dest            = %04X",          replay.read_word(0x7E034D + y) << 1);
			p("length          = %04X",          replay.read_word(0x7E034A + y));
			p("inc_mode        = %02X",          replay.read_byte(0x7E034C + y));
		}
	}

	//
	// loglevels::sys_unpack
	//
	if (settings.sys_unpack) {
		if (verbose && pc == 0x80B5C7) {
			p("source          = %06X",          replay.read_long(0x7E002F));
			p("dest            = %06X",          replay.read_long(0x7E0032));
			p("queue_index     > %d",            replay.read_word(0x7E0446) / 6);
		}
		if (verbose && pc == 0x80B579) {
			if (replay.read_byte(0x7E0448)) {
				local lzn_index = 0;
				while (replay.read_long(0x7E0449 + (lzn_index * 6))) {
					p("source[%d]      = %06X",       lzn_index, replay.read_long(0x7E0449 + (lzn_index * 6) + 0));
					p("dest[%d]        = %06X",       lzn_index, replay.read_long(0x7E0449 + (lzn_index * 6) + 3));
					++lzn_index;
				}
			}
		}
		// SYS_UNPACK_decode
		if (pc == 0x80BAF4) {
			lzn_source_start = replay.read_long(0x7E0000);
			lzn_dest_start = replay.read_long(0x7E0003);
		}
		if (pc == 0x80BE0C) {
			if (lzn_source_start) {
				p("source          = %06X (length: %04X)",  lzn_source_start, (replay.read_byte(0x7E0002) << 16) + replay.y() - lzn_source_start);
				p("dest            = %06X (length: %04X)",  lzn_dest_start,   replay.x() - (lzn_dest_start & 0xFFFF));
			}
		}
	}

	//
	// loglevels::sys_util
	//
	if (settings.sys_util) {
		if (pc == 0x80AE41) {
			p("bg1_tile_base   = %04X",          replay.read_word(0x7E000D) << 1);
			p("bg2_tile_base   = %04X",          replay.read_word(0x7E000F) << 1);
			p("bg3_tile_base   = %04X",          replay.read_word(0x7E0011) << 1);
			p("bg4_tile_base   = %04X",          replay.read_word(0x7E0013) << 1);
			p("obj_tile_base   = %04X",          replay.read_word(0x7E0015) << 1);
			p("bg1_screen      = %04X",          replay.read_word(0x7E0017) << 1);
			p("bg2_screen      = %04X",          replay.read_word(0x7E0019) << 1);
			p("bg3_screen      = %04X",          replay.read_word(0x7E001B) << 1);
		}
	}

	//
	// loglevels::sys_lib
	//
	if (settings.sys_lib) {
		if (pc == 0x80B8FD) {
			local src_ptr = replay.read_long(0x7E002F);
			p("source          = %06X",     src_ptr);
			p("dest            = %06X",     replay.read_long(0x7E0032));
			local length = replay.read_word(0x7E0035);
			p("length          = %04X",     length);
			p_data(src_ptr, length);
		}
		if (pc == 0x80B929) {
			p("address         = %06X",     replay.read_long(0x7E002F));
			p("length          = %04X",     replay.read_word(0x7E0032));
			p("value           = %04X",     replay.a());
		}
		if (pc == 0x80B954) {
			p("address         = %06X",     replay.read_long(0x7E002F));
			p("length          = %04X",     replay.read_word(0x7E0032));
			p("and_value       = %04X",     replay.a());
		}
		if (pc == 0x80B98E) {
			p("address         = %06X",     replay.read_long(0x7E002F));
			p("length          = %04X",     replay.read_word(0x7E0032));
			p("or_value        = %04X",     replay.a());
		}
	}

}

function trace_log_init(replay) {
	replay.add_breakpoint(0x8789EC);
	replay.add_breakpoint(0x879E01);
	replay.add_breakpoint(0x879A0C);
	replay.add_breakpoint(0x8797D7);
	replay.add_breakpoint(0x879C54);
	replay.add_breakpoint(0x87C779);
	replay.add_breakpoint(0x8796E9);
	replay.add_breakpoint(0x87983C);
	replay.add_breakpoint(0x87964A);
	replay.add_breakpoint(0x83A7E9);
	replay.add_breakpoint(0x879F4C);
	replay.add_breakpoint(0x879F6F);
	replay.add_breakpoint(0x82EF64);
	replay.add_breakpoint(0x878BD2);
	replay.add_breakpoint(0x87885C);
	replay.add_breakpoint(0x878D8A);
	replay.add_breakpoint(0x878D92);
	replay.add_breakpoint(0x878D95);
	replay.add_breakpoint(0x87883D);
	replay.add_breakpoint(0x87BB7A);
	replay.add_breakpoint(0x8786CB);
	replay.add_breakpoint(0x87872F);
	replay.add_breakpoint(0x878716);
	replay.add_breakpoint(0x87B0E1);
	replay.add_breakpoint(0x82F13D);
	replay.add_breakpoint(0x80B4B6);
	replay.add_breakpoint(0x80B503);
	replay.add_breakpoint(0x80B3EB);
	replay.add_breakpoint(0x80B2FF);
	replay.add_breakpoint(0x80B5C7);
	replay.add_breakpoint(0x80B579);
	replay.add_breakpoint(0x80BAF4);
	replay.add_breakpoint(0x80BE0C);
	replay.add_breakpoint(0x80AE41);
	replay.add_breakpoint(0x80B8FD);
	replay.add_breakpoint(0x80B929);
	replay.add_breakpoint(0x80B954);
	replay.add_breakpoint(0x80B98E);
}
