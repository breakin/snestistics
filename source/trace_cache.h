#pragma once

#include "trace_format.h"

namespace snestistics {

	#pragma pack(push, 1)
	struct TraceCacheHeader {
		uint64_t magic = 0x534e535443414348; // TODO: Reverse?
		uint32_t version;
		uint8_t trace_file_content_guid[8]; // Content ID from the trace file so we can refresh cache if it differs
		uint32_t nmi_per_skip; // How many NMIs do we have for each skip?
		uint32_t num_nmis; // For safety
		uint64_t trace_summary_seek_offset;
		uint64_t replay_cache_seek_offset;
	};

	struct TraceSkip {
		/*
			TODO: Content of TraceSkip should be determined by Replay and Replay alone
			It should be a dump of Replay state. Move into replay and hide!
		*/
		uint32_t nmi; // Nmi for this skip (for validation)
		uint64_t seek_offset_trace_file; // Where in .trace should we stand
		uint64_t current_op;
		snestistics::TraceRegisters regs;
	};
	#pragma pack(pop)

	static const int trace_skip_extra_data = 64*1024*2; // RAM content
	static const int trace_skip_total_size = sizeof(TraceSkip) + trace_skip_extra_data; // Size on disk
}
