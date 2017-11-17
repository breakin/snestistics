set(SOURCES
	annotations.cpp
	annotations.h
	asmwrite_wladx.h
	cputable.cpp
	cputable.h
	emulate.cpp
	emulate.h
	emulate_replay.h
	options.cpp
	options.h
	romaccessor.h
	snestistics.cpp
	trace.cpp
	trace.h
	trace_format.h
	trace_log.cpp
	trace_log.h
	rewind.cpp
	rewind.h
	utils.h
)

# Actually trigger re-generation from here
SET(GENERATED_SOURCE
	emulate_generate/optables.h
)

SET(DATA
	../data/hardware_reg.labels
)

add_executable(snestistics ${SOURCES} ${GENERATED_SOURCE} ${DATA})
target_compile_definitions(snestistics PRIVATE _CRT_SECURE_NO_WARNINGS)

source_group("source" FILES ${SOURCES})
source_group("generated" FILES ${GENERATED_SOURCE})
source_group("data" FILES ${DATA})