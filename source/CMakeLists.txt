set(SOURCES
	annotations.cpp
	annotations.h
	asm_writer.cpp
	asm_writer.h
	cputable.cpp
	cputable.h
	emulate.cpp
	emulate.h
	options.cpp
	options.h
	romaccessor.h
	snestistics.cpp
	scripting.cpp
	scripting.h
	trace.cpp
	trace.h
	trace_format.h
	trace_log.cpp
	trace_log.h
	replay.cpp
	replay.h
	api.h
	report_writer.cpp
	report_writer.h
	rewind.cpp
	rewind.h
	utils.h
)

# Actually trigger re-generation from here
SET(GENERATED_SOURCE
	instruction_tables.json
	instruction_tables_generate.py
	instruction_tables.h
	instruction_tables.cpp
)

SET(DATA
	../data/hardware_reg.labels
)

SET(TESTDATA
	../testdata/trace_log_test.nut
)

add_custom_command(
	PRE_BUILD
	COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/instruction_tables_generate.py
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/instruction_tables_generate.py ${CMAKE_CURRENT_SOURCE_DIR}/instruction_tables.json
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/instruction_tables.h
	COMMENT "Generating instruction_tables.h"
)

add_executable(snestistics ${SOURCES} ${GENERATED_SOURCE} ${DATA} ${TESTDATA})
target_compile_definitions(snestistics PRIVATE _CRT_SECURE_NO_WARNINGS)

target_link_libraries(snestistics squirrel_static)
target_link_libraries(snestistics sqstdlib_static)
include_directories("../deps/squirrel/include")

source_group("source" FILES ${SOURCES})
source_group("generate" FILES ${GENERATED_SOURCE})
source_group("data" FILES ${DATA})
source_group("testdata" FILES ${TESTDATA})