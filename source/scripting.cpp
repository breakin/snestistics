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
#include <algorithm>
#include "api.h"

namespace scripting_interface {

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
			HSQMEMBERHANDLE _user_pointer_handle;
			HSQOBJECT _class_object;
		public:
			ScriptClass(HSQUIRRELVM &v, const char * const name) : _v(v) {
				{
					ScopedValidateTop top(_v);
					sq_pushroottable(_v);
					sq_pushstring(_v, name, strlen(name));
					check_return(sq_newclass(_v, false));

					// Remember the class object
					sq_resetobject(&_class_object);
					sq_getstackobj(_v, -1, &_class_object);
					sq_addref(_v, &_class_object);

					check_return(sq_newslot(_v, -3, SQFalse));
					sq_pop(_v, 1); // Remove class object
				}

				{
					ScopedValidateTop top(_v);
					sq_pushobject(_v, _class_object);
					sq_pushstring(_v, "__user_pointer", -1);
					sq_pushuserpointer(_v, nullptr);
					sq_pushnull(_v);
					check_return(sq_newmember(_v, -4, SQFalse));
					sq_pop(_v, 1);
				}
				{
					ScopedValidateTop top(_v);
					sq_pushobject(_v, _class_object);
					sq_pushstring(_v, "__user_pointer", -1);
					check_return(sq_getmemberhandle(_v, -2, &_user_pointer_handle));
					sq_pop(_v, 1);
				}
				add_function("constructor", disable_constructing);
				add_function("_cloned", disable_cloning);
			}
			~ScriptClass() {
				sq_release(_v, &_class_object);
			}

			void push_class_object(HSQUIRRELVM &v) {
				sq_pushobject(_v, _class_object);
			}

			void add_function(const char * const name, SQFUNCTION function) {
				ScopedValidateTop top(_v);
				sq_pushobject(_v, _class_object);
				sq_pushstring(_v, name, strlen(name));
				sq_newclosure(_v, function, 0);
				check_return(sq_newslot(_v, -3, SQFalse));
				sq_pop(_v, 1);
			}

			template<typename T>
			T* native_ptr(HSQUIRRELVM &v) {
				// The first argument is the instance
				SQObjectType instance_type = sq_gettype(v, 1);
				CUSTOM_ASSERT(instance_type == OT_INSTANCE);

				check_return(sq_getbyhandle(v, 1, &_user_pointer_handle));

				SQUserPointer ptr = nullptr;
				sq_getuserpointer(v, -1, &ptr);
				return static_cast<T*>(ptr);
			}

			HSQOBJECT create_instance(void *native_ptr) {
				ScopedValidateTop top(_v);
				sq_pushobject(_v, _class_object);
				sq_createinstance(_v, -1);

				HSQOBJECT instance;
				sq_resetobject(&instance);
				sq_getstackobj(_v, -1, &instance);

				sq_pushuserpointer(_v, native_ptr);
				check_return(sq_setbyhandle(_v, -2, &_user_pointer_handle));

				sq_addref(_v, &instance);

				sq_pop(_v, 2);
				return instance;
			}

			ScriptingHandle create_instance_handle(void *native_ptr) {
				ScopedValidateTop top(_v);
				sq_pushobject(_v, _class_object);
				sq_createinstance(_v, -1);

				HSQOBJECT *instance = new HSQOBJECT;
				sq_resetobject(instance);
				sq_getstackobj(_v, -1, instance);
				sq_pushuserpointer(_v, native_ptr);
				check_return(sq_setbyhandle(_v, -2, &_user_pointer_handle));

				sq_addref(_v, instance);
				sq_pop(_v, 2);

				return (ScriptingHandle)instance;
			}

			void destroy_instance(HSQOBJECT &instance) {
				sq_release(_v, &instance);
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
			delete class_replay;
			delete class_report_writer;
			sq_close(v);
		}
		ScriptClass* class_replay = nullptr;
		ScriptClass* class_report_writer = nullptr;
		HSQUIRRELVM v;
	};

	namespace {
		Registers *register_func(HSQUIRRELVM v) {
			Scripting *scripting = static_cast<Scripting*>(sq_getforeignptr(v));
			Replay *t = scripting->class_replay->native_ptr<Replay>(v);
			return replay_registers(t);
		}

		SQInteger api_replay_pc(HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->pc); return 1; }
		SQInteger api_replay_a (HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->a ); return 1; }
		SQInteger api_replay_al(HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->a&0xFF ); return 1; }
		SQInteger api_replay_ah(HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->a>>8   ); return 1; }
		SQInteger api_replay_x (HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->x ); return 1; }
		SQInteger api_replay_xl(HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->x&0xFF ); return 1; }
		SQInteger api_replay_xh(HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->x>>8   ); return 1; }
		SQInteger api_replay_y (HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->y ); return 1; }
		SQInteger api_replay_yl(HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->y&0xFF ); return 1; }
		SQInteger api_replay_yh(HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->y>>8   ); return 1; }
		SQInteger api_replay_p (HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->p ); return 1; }
		SQInteger api_replay_s (HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->s ); return 1; }
		SQInteger api_replay_dp(HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->dp); return 1; }
		SQInteger api_replay_db(HSQUIRRELVM v) { sq_pushinteger(v, register_func(v)->db); return 1; }

		SQInteger api_replay_read_byte(HSQUIRRELVM v) {
			Scripting *scripting = static_cast<Scripting*>(sq_getforeignptr(v));
			Replay *t = scripting->class_replay->native_ptr<Replay>(v);
			SQInteger address;
			sq_getinteger(v, 2, &address);
			sq_pushinteger(v, replay_read_byte(t, (uint32_t)address));
			return 1;
		}
		SQInteger api_replay_read_word(HSQUIRRELVM v) {
			Scripting *scripting = static_cast<Scripting*>(sq_getforeignptr(v));
			Replay *t = scripting->class_replay->native_ptr<Replay>(v);
			SQInteger address;
			sq_getinteger(v, 2, &address);
			sq_pushinteger(v, replay_read_word(t, (uint32_t)address));
			return 1;
		}
		SQInteger api_replay_read_long(HSQUIRRELVM v) {
			Scripting *scripting = static_cast<Scripting*>(sq_getforeignptr(v));
			Replay *t = scripting->class_replay->native_ptr<Replay>(v);
			SQInteger address;
			sq_getinteger(v, 2, &address);
			sq_pushinteger(v, replay_read_long(t, (uint32_t)address));
			return 1;
		}

		SQInteger api_replay_add_breakpoint(HSQUIRRELVM v) {
			Scripting *scripting = static_cast<Scripting*>(sq_getforeignptr(v));
			Replay *t = scripting->class_replay->native_ptr<Replay>(v);
			SQInteger pc;
			sq_getinteger(v, 2, &pc);
			replay_add_breakpoint(t, (uint32_t)pc);
			return 0;
		}
		SQInteger api_replay_add_breakpoint_range(HSQUIRRELVM v) {
			Scripting *scripting = static_cast<Scripting*>(sq_getforeignptr(v));
			Replay *t = scripting->class_replay->native_ptr<Replay>(v);
			SQInteger pc0, pc1;
			sq_getinteger(v, 2, &pc0);
			sq_getinteger(v, 3, &pc1);
			replay_add_breakpoint_range(t, (uint32_t)pc0, (uint32_t)pc1);
			return 0;
		}
		SQInteger api_report_writer_print(HSQUIRRELVM v) {
			Scripting *scripting = static_cast<Scripting*>(sq_getforeignptr(v));
			ReportWriter *t = scripting->class_report_writer->native_ptr<ReportWriter>(v);
			const SQChar* str;
			sq_getstring(v, 2, &str);
			report_writer_print(t, str, (uint32_t)strlen(str));
			return 0;
		}

		void register_api(Scripting *scripting) {
			HSQUIRRELVM &v = scripting->v;
			{
				ScopedValidateTop top(v);
				scripting->class_replay = new ScriptClass(v, "Replay");
				auto &e = *scripting->class_replay;
				e.add_function("pc", api_replay_pc);
				e.add_function("a",  api_replay_a);
				e.add_function("al",  api_replay_al);
				e.add_function("ah",  api_replay_ah);
				e.add_function("x",  api_replay_x);
				e.add_function("xl",  api_replay_xl);
				e.add_function("xh",  api_replay_xh);
				e.add_function("y",  api_replay_y);
				e.add_function("yl",  api_replay_yl);
				e.add_function("yh",  api_replay_yh);
				e.add_function("p",  api_replay_p);
				e.add_function("s",  api_replay_s);
				e.add_function("dp", api_replay_dp);
				e.add_function("db", api_replay_db);
				e.add_function("read_byte", api_replay_read_byte);
				e.add_function("read_word", api_replay_read_word);
				e.add_function("read_long", api_replay_read_long);
				e.add_function("add_breakpoint", api_replay_add_breakpoint);
				e.add_function("add_breakpoint_range", api_replay_add_breakpoint_range);
			}
			{
				ScopedValidateTop top(v);
				scripting->class_report_writer = new ScriptClass(v, "ReportWriter");
				auto &e = *scripting->class_report_writer;
				e.add_function("print", api_report_writer_print);
			}
		}

		void printfunc(HSQUIRRELVM v, const SQChar *s, ...) {
			va_list vl;
			va_start(vl, s);
			scvprintf(stdout, s, vl);
			va_end(vl);
		}

		void errorfunc(HSQUIRRELVM v, const SQChar *s, ...) {
			va_list vl;
			va_start(vl, s);
			scvprintf(stderr, s, vl);
			va_end(vl);
		}
	}

	Scripting* create_scripting(const char *const script_filename) {

		Scripting *scripting = new Scripting;
		scripting->v = sq_open(1024);

		HSQUIRRELVM &v = scripting->v;

		sq_setforeignptr(v, scripting); // So we can access global state for this VM

		sq_pushroottable(v);
		sqstd_register_stringlib(v);
		sq_pop(v, 1);

		sqstd_seterrorhandlers(v);

		sq_setprintfunc(v, printfunc, errorfunc);

		sq_pushroottable(v);
		if (!SQ_SUCCEEDED(sqstd_dofile(v, _SC(script_filename), SQFalse, SQTrue))) {
			printf("Failed to open %s\n", script_filename);
			delete scripting;
			throw std::runtime_error("Could not open script file!");
		}
		sq_pop(v, 1);
		register_api(scripting);
		return scripting;
	}

	void destroy_scripting(Scripting *scripting) {
		delete scripting;
	}

	ScriptingHandle create_report_writer(Scripting *s, ReportWriter *t) {
		return s->class_report_writer->create_instance_handle(t);
	}

	ScriptingHandle create_replay(Scripting *s, Replay *t) {
		return s->class_replay->create_instance_handle(t);
	}

	void destroy_handle(Scripting *s, ScriptingHandle h) {
		HSQOBJECT *instance = (HSQOBJECT*)h;
		sq_release(s->v, instance);
		delete instance;
	}

	void scripting_trace_log_init(Scripting *scripting, ScriptingHandle replay) {
		HSQUIRRELVM &v = scripting->v;

		ScopedValidateTop scoped_top(v);

		SQInteger top = sq_gettop(v);

		sq_pushroottable(v);
		sq_pushstring(v, _SC("trace_log_init"), -1);
		bool success = SQ_SUCCEEDED(sq_get(v, -2));
		if (success) {
			sq_pushroottable(v);
			sq_pushobject(v, *(HSQOBJECT*)replay);
			sq_call(v, 2, SQTrue, SQTrue);
			sq_pop(v, 1);
		}
		else {
			printf("trace_log_init not found in script!");
		}
		sq_settop(v, top);
	}

	void scripting_trace_log_parameter_printer(Scripting *scripting, ScriptingHandle replay, ScriptingHandle report_writer) {
		HSQUIRRELVM &v = scripting->v;
		SQInteger top = sq_gettop(v);
		sq_pushroottable(v);
		sq_pushstring(v, _SC("trace_log_parameter_printer"), -1);
		bool success = SQ_SUCCEEDED(sq_get(v, -2));
		if (success) {
			sq_pushroottable(v);
			sq_pushobject(v, *(HSQOBJECT*)replay);
			sq_pushobject(v, *(HSQOBJECT*)report_writer);
			sq_call(v, 3, SQTrue, SQTrue);
			sq_pop(v, 1);
		}
		else {
			printf("trace_log_parameter_printer not found in script!");
		}
		sq_settop(v, top);
	}
}