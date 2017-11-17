#include "scripting.h"
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdaux.h>
#include <sqstdstring.h>
#include <stdexcept>
#include <cassert>
#include <stdarg.h>
#include <stdio.h>
#include "utils.h"
#include "emulate.h"
#include <algorithm>
#include <memory>

#ifdef SQUNICODE
	#define scvprintf vfwprintf
#else
	#define scvprintf vfprintf
#endif

namespace {

	struct ScopedTop {
		const SQInteger _start_top;
		HSQUIRRELVM &_v;
		ScopedTop(HSQUIRRELVM &v) : _v(v), _start_top(sq_gettop(v)) {}
		~ScopedTop() {
			sq_settop(_v, _start_top);
		}
	};

	struct ScopedValidateTop {
		const SQInteger _start_top;
		HSQUIRRELVM &_v;
		ScopedValidateTop(HSQUIRRELVM &v) : _v(v), _start_top(sq_gettop(v)) {}
		~ScopedValidateTop() {
			SQInteger end_top = sq_gettop(_v);
			CUSTOM_ASSERT(_start_top == end_top);
		}
	};

	void check_return(SQRESULT result) {
		CUSTOM_ASSERT(result == SQ_OK);
	}

	// A simple class with no base
	// TODO: We can add pointer to this class using free variable
	//       That way we can't get instance data but maybe getbyhandle

	// This is what we need to maintain life instances I guess
	// Do we need to maintain life of class as well?
	// http://www.squirrel-lang.org/squirreldoc/reference/embedding/references_from_c.html
	//	Two errors: sq_pushobject no longer takes &
	//              sq_resetobject(v,&obj) does not take v
	class ScriptClass {
	private:
		HSQUIRRELVM &_v; // Maybe bad to hold this on here. Pass it in to all functions? Only bad thing is destructor but making it explicit makes sense anyway
		HSQOBJECT _class_object;
	public:
		ScriptClass(HSQUIRRELVM &v, const char * const name) : _v(v) {
			{
				ScopedValidateTop top(_v);
				sq_pushroottable(_v);
				sq_pushstring(_v,name,strlen(name));
				check_return(sq_newclass(_v, false));

				// Remember the class object
				sq_resetobject(&_class_object);
				sq_getstackobj(_v,-1,&_class_object);
				sq_addref(_v,&_class_object);

				check_return(sq_newslot(_v,-3,SQFalse));
				sq_pop(_v,1); // Remove class object
			}

			{
				ScopedValidateTop top(_v);
				sq_pushobject(_v,_class_object);
				sq_pushstring(_v,"__user_pointer",-1);
				sq_pushuserpointer(_v, nullptr); // null default value
				sq_pushnull(_v); // null attribute
				check_return(sq_newmember(_v,-4, SQFalse));
				sq_pop(_v,1); // Remove class object
			}
			add_function("constructor", disable_constructing);
			add_function("_cloned", disable_cloning);
		}
		~ScriptClass() {
			sq_release(_v,&_class_object);
		}

		void push_class_object(HSQUIRRELVM &v) {
			sq_pushobject(_v,_class_object);
		}

		void add_function(const char * const name, SQFUNCTION function) {
			ScopedValidateTop top(_v);
			sq_pushobject(_v,_class_object);
			sq_pushstring(_v,name,strlen(name));
			sq_newclosure(_v,function, 0);
			check_return(sq_newslot(_v,-3,SQFalse));
			sq_pop(_v,1); // Remove class object
		}

		template<typename T>
		static T* native_ptr(HSQUIRRELVM &v) {
			SQObjectType emu_type = sq_gettype(v,1);
			sq_pushstring(v, "__user_pointer", -1);
			// TODO: We can use getbyhandle instead
			sq_get(v, -2);
			SQUserPointer ptr = nullptr;
			sq_getuserpointer(v, -1, &ptr);
			return static_cast<T*>(ptr);
		}
		static void set_native_ptr(HSQUIRRELVM &v, void *ptr) {
			// TODO
		}

	private:
		static SQInteger disable_cloning(HSQUIRRELVM v) {
			return sq_throwerror(v, "Cloning for this class is disabled");
		}
		static SQInteger disable_constructing(HSQUIRRELVM v) {
			return sq_throwerror(v, "Creating this class from script directly is prohibited");
		}
	};
}

struct Scripting {
	Scripting() {}
	~Scripting() {
		class_emulator.reset();
		sq_close(v);
	}
	std::unique_ptr<ScriptClass> class_emulator;
	HSQUIRRELVM v;
	
};

struct ScriptEmulator {
	LargeBitfield *breakpoints = nullptr;
	int magic_number = 42;
};

namespace {

	// add_breakpoint(emu, pc0)
	// add_breakpoint(emu, pc0, pc1) - mistake to use one function?
/*	SQInteger api_add_breakpoint(HSQUIRRELVM v) {
		SQInteger nargs = sq_gettop(v);
		SQObjectType emu_type = sq_gettype(v,2);

		SQUserPointer user_ptr;
		sq_getuserpointer(v, 4, &user_ptr);
		Scripting *scripting = (Scripting*)user_ptr;

		if (!scripting->breakpoints)
			return 0;

		SQInteger emu;
		sq_getinteger(v, 2, &emu);

		CUSTOM_ASSERT(sq_gettype(v,3) == OT_INTEGER);
		SQInteger value_a;
		sq_getinteger(v, 3, &value_a);

		const uint32_t last_bit = 1024*256*64-1;

		if (nargs == 4) {
			if (value_a<0 || value_a>last_bit)
				return 0;
			scripting->breakpoints->setBit((uint32_t)value_a);
		} else if (nargs == 5) {
			CUSTOM_ASSERT(sq_gettype(v,4) == OT_INTEGER);
			SQInteger value_b;
			sq_getinteger(v, 4, &value_b);
			CUSTOM_ASSERT(value_a <= value_b);
			CUSTOM_ASSERT(value_a >= 0);
			CUSTOM_ASSERT(value_b <= last_bit);
			if (value_a > last_bit)
				return 0;
			uint32_t va = (uint32_t)std::max(value_a, 0LL);
			uint32_t vb = (uint32_t)std::min(value_b, (SQInteger)last_bit);
			for (uint32_t value = va; value <= vb; ++value) {
				// TODO: Add setBits? This can be made faster on bit-level.
				scripting->breakpoints->setBit(value);
			}
		} else {
			CUSTOM_ASSERT(false);
		}
		return 0;
	}
	*/
	/*
	EmulateRegisters* emu_get(HSQUIRRELVM &v) {
		SQInteger nargs = sq_gettop(v);
		SQObjectType emu_type = sq_gettype(v,2);
		assert(nargs == 2 && emu_type == OT_USERPOINTER);
		SQUserPointer user_ptr;
		sq_getuserpointer(v, 2, &user_ptr);
		return (EmulateRegisters*)user_ptr;
	}

	EmulateRegisters* emu_ptr(HSQUIRRELVM &v, uint32_t *ptr) {
		SQInteger nargs = sq_gettop(v);
		SQObjectType emu_type = sq_gettype(v,2);
		SQObjectType ptr_type = sq_gettype(v,3);
		assert(nargs == 3 && emu_type == OT_USERPOINTER && ptr_type == OT_INTEGER);
		SQUserPointer user_ptr;
		sq_getuserpointer(v, 2, &user_ptr);
		SQInteger sq_ptr;
		sq_getinteger(v, 3, &sq_ptr);
		assert(sq_ptr>=0);
		*ptr = (uint32_t)sq_ptr;
		return (EmulateRegisters*)user_ptr;
	}

	SQInteger api_emu_a (HSQUIRRELVM v) { sq_pushinteger(v, emu_get(v)->_A ); return 1; }
	SQInteger api_emu_pc(HSQUIRRELVM v) { sq_pushinteger(v, emu_get(v)->_PC); return 1; }
	SQInteger api_emu_x (HSQUIRRELVM v) { sq_pushinteger(v, emu_get(v)->_X ); return 1; }
	SQInteger api_emu_y (HSQUIRRELVM v) { sq_pushinteger(v, emu_get(v)->_Y ); return 1; }
	SQInteger api_emu_dp(HSQUIRRELVM v) { sq_pushinteger(v, emu_get(v)->_DP); return 1; }
	SQInteger api_emu_db(HSQUIRRELVM v) { sq_pushinteger(v, emu_get(v)->_DB); return 1; }
	SQInteger api_emu_s (HSQUIRRELVM v) { sq_pushinteger(v, emu_get(v)->_S ); return 1; }
	SQInteger api_emu_p (HSQUIRRELVM v) { sq_pushinteger(v, emu_get(v)->_P ); return 1; }
	SQInteger api_emu_byte(HSQUIRRELVM v) {
		uint32_t ptr = 0;
		EmulateRegisters *e = emu_ptr(v, &ptr);
		uint8_t value = e->_memory[ptr+0];
		sq_pushinteger(v, value);
		return 1;
	}
	SQInteger api_emu_word(HSQUIRRELVM v) {
		uint32_t ptr = 0;
		EmulateRegisters *e = emu_ptr(v, &ptr);
		uint16_t value = e->_memory[ptr+0]|(e->_memory[ptr+1]<<8);
		sq_pushinteger(v, value);
		return 1;
	}
	SQInteger api_emu_long(HSQUIRRELVM v) {
		uint32_t ptr = 0;
		EmulateRegisters *e = emu_ptr(v, &ptr);
		uint16_t value = e->_memory[ptr+0]|(e->_memory[ptr+1]<<8)|(e->_memory[ptr+2]<<16);
		sq_pushinteger(v, value);
		return 1;
	}
	void register_emu_getter(Scripting *scripting, const char * const fname, SQFUNCTION function) {
		HSQUIRRELVM &v = scripting->v;
		sq_pushroottable(v);
		sq_pushstring(v,fname,-1);
		sq_newclosure(v,function, 0);
		sq_newslot(v,-3,SQFalse);
		sq_pop(v,1);
	}
		// TODO-IDEA: We could expose register using a table instead. Not sure what is faster
		register_emu_getter(scripting, "emu_a",  api_emu_a);
		register_emu_getter(scripting, "emu_pc", api_emu_pc);
		register_emu_getter(scripting, "emu_x",  api_emu_x);
		register_emu_getter(scripting, "emu_y",  api_emu_y);
		register_emu_getter(scripting, "emu_dp", api_emu_dp);
		register_emu_getter(scripting, "emu_db", api_emu_db);
		register_emu_getter(scripting, "emu_s",  api_emu_s);
		register_emu_getter(scripting, "emu_p",  api_emu_p);
		register_emu_getter(scripting, "emu_byte",  api_emu_byte);
		register_emu_getter(scripting, "emu_word",  api_emu_word);
		register_emu_getter(scripting, "emu_long",  api_emu_long);
	*/

	SQInteger api_emulator_a(HSQUIRRELVM v) {
		ScriptEmulator *se = ScriptClass::native_ptr<ScriptEmulator>(v);
		printf("Emulator::a, magic = %d!\n", se->magic_number);
		return 0;
	}
	SQInteger api_emulator_add_breakpoint(HSQUIRRELVM v) {
		ScriptEmulator *se = ScriptClass::native_ptr<ScriptEmulator>(v);
		printf("Emulator::add_breakpoint, magic = %d!\n", se->magic_number);
		return 0;
	}

	/*
	void add_function(HSQUIRRELVM &v, const char * const fname, SQFUNCTION func, bool add_free = false, void *free_variable = nullptr) {
		ScopedValidateTop top(v);
		sq_pushroottable(v);
		sq_pushstring(v,fname,-1);
		if (add_free)
			sq_pushuserpointer(v,free_variable);
		sq_newclosure(v,func, add_free ? 1 : 0);
		sq_newslot(v,-3,SQFalse);
		sq_pop(v,1);
	}
	*/

	void register_api(Scripting *scripting) {
		HSQUIRRELVM &v = scripting->v;
		{
			ScopedValidateTop top(v);
			scripting->class_emulator = std::make_unique<ScriptClass>(v, "Emulator");
			auto &e = *scripting->class_emulator.get();
			e.add_function("a", api_emulator_a);
			// Consider add_breakpoint and add_breakpoint_range
			e.add_function("add_breakpoint", api_emulator_add_breakpoint);
		}
	}

	void printfunc(HSQUIRRELVM v,const SQChar *s,...) {
		va_list vl;
		va_start(vl, s);
		scvprintf(stdout, s, vl);
		va_end(vl);
	}

	void errorfunc(HSQUIRRELVM v,const SQChar *s,...) {
		va_list vl;
		va_start(vl, s);
		scvprintf(stderr, s, vl);
		va_end(vl);
	}
}

Scripting* create_scripting(const char *const script_filename) {

	Scripting *scripting = new Scripting;
    scripting->v = sq_open(1024); // creates a VM with initial stack size 1024

	HSQUIRRELVM &v = scripting->v;

	sq_pushroottable(v); //push the root table where the std function will be registered
	sqstd_register_stringlib(v);
	sq_pop(v,1); //pops the root table

	sqstd_seterrorhandlers(v); //registers the default error handlers

	sq_setprintfunc(v, printfunc, errorfunc); //sets the print function

	sq_pushroottable(v); //push the root table(were the globals of the script will be stored)
	if(!SQ_SUCCEEDED(sqstd_dofile(v, _SC(script_filename), SQFalse, SQTrue))) {
		printf("Failed to open %s\n", script_filename);
		delete scripting;
		throw std::runtime_error("Could not open script file!");
	}
	sq_pop(v,1); //pops the root table
	register_api(scripting);
	return scripting;
}

void destroy_scripting(Scripting *scripting) {
	delete scripting;
}

void scripting_trace_log_init(Scripting *scripting, LargeBitfield &result) {
	ScriptEmulator *se = new ScriptEmulator;
	se->breakpoints = &result;

	HSQUIRRELVM &v = scripting->v;

	ScopedValidateTop scoped_top(v);

	SQInteger top = sq_gettop(v);
	scripting->class_emulator->push_class_object(v);
	sq_pushroottable(v);
	sq_pushstring(v,_SC("trace_log_init"),-1);
	bool success = SQ_SUCCEEDED(sq_get(v,-2));
	if(success) {
		sq_pushroottable(v);
		sq_createinstance(v, -4);
		scripting->class_emulator->set_native_ptr(v, se);
		sq_call(v,2,SQTrue,SQTrue);
		sq_pop(v,1);
	} else {
		printf("trace_log_init not found in script!");
	}
	sq_settop(v,top);
	delete se;
}

void scripting_trace_log_parameter_printer(Scripting *scripting, EmulateRegisters *emulator_state) {
	HSQUIRRELVM &v = scripting->v;
	SQInteger top = sq_gettop(v);
	sq_pushroottable(v);
	sq_pushstring(v,_SC("trace_log_parameter_printer"),-1);
	bool success = SQ_SUCCEEDED(sq_get(v,-2));
	if(success) {
		sq_pushroottable(v);
		sq_pushuserpointer(v, emulator_state);
		sq_call(v,2,SQTrue,SQTrue);
		sq_pop(v,1);
	} else {
		printf("trace_log_parameter_printer not found in script!");
	}
	sq_settop(v,top);
}