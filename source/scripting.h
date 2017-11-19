#pragma once

#include <vector>
#include "api.h" // TODO: Only types needed

namespace scripting_interface {

	/*
		The reason scripting is exposed like this is that I don't want details
		of the scripting language to leak out to the application in general.
	*/

	struct Scripting;
	struct EmulateRegisters;
	class LargeBitfield;

	Scripting *create_scripting(const char * const script_filename);
	void destroy_scripting(Scripting *scripting);

	typedef void* ScriptingHandle;

	ScriptingHandle create_tracelog(Scripting *s, TraceLog *t);
	ScriptingHandle create_replay(Scripting *s, Replay *t);
	void destroy_handle(Scripting *s, ScriptingHandle h);

	void scripting_trace_log_init(Scripting *scripting, ScriptingHandle replay);
	void scripting_trace_log_parameter_printer(Scripting *scripting, ScriptingHandle replay);

}