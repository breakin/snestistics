function trace_log_parameter_printer(emu)
{
	local pc = emu_pc(emu);
	if (pc == 0x8789EC) {
		print(format("0x%02X 0x%04X\n", emu_byte(emu, 0x008030), emu_a(emu)));
	}
}

function trace_log_init(e) {
	print("Knark");
	e.add_breakpoint(0x8789EC);
	e.add_breakpoint(0x879E01);
	e.add_breakpoint(0x879A0C);
	e.add_breakpoint(0x879A0E, 0x879A0F); // Just test the range version

	print(e.a());
	print("Bark");
}
