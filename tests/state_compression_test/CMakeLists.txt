set(SOURCES
	state_compression_test.cpp
)

SET(EXTERNAL
	../../source/state_compression.cpp
	../../source/state_compression.h
	../../deps/lz4/lib/lz4.c
	../../deps/lz4/lib/lz4.h
)

add_executable(state_compression_test ${SOURCES} ${EXTERNAL}) 
target_compile_definitions(state_compression_test PRIVATE _CRT_SECURE_NO_WARNINGS)

include_directories("../../deps/lz4/lib")
include_directories("../../source")

source_group("source" FILES ${SOURCES})
source_group("external" FILES ${EXTERNAL})

set_target_properties(state_compression_test PROPERTIES FOLDER tests)