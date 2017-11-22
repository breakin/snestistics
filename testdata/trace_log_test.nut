function trace_log_parameter_printer(replay, report)
{
	local pc = replay.pc();
	//report.print(format("trace_log_parameter_printer %06X", pc));
	if (pc == 0x8789EC) {
		report.print(format("Hit %06X %04X %04X %04X", replay.pc(), replay.a(), replay.x(), replay.y()));
	}
}

function trace_log_init(replay) {
	replay.add_breakpoint(0x8789EC);
	replay.add_breakpoint(0x879E01);
	replay.add_breakpoint(0x879A0C);
	replay.add_breakpoint_range(0x879A0E, 0x879A0F);
	replay.add_breakpoint_range(0, 0xFFFFFF);
}
