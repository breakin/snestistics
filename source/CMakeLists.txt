set(SOURCES
	annotationresolver.cpp
	annotationresolver.h
	asmwrite_wladx.h
	cputable.h
	cmdoptions.h
	romaccessor.h
	snestistics.cpp
	utils.h
)

set(GENERATED_SOURCE
)

SET(DATA
	../examples/hardware_reg.labels
)

add_executable(snestistics ${SOURCES} ${GENERATED_SOURCE} ${DATA})
target_compile_definitions(snestistics PRIVATE _CRT_SECURE_NO_WARNINGS)

source_group("source" FILES ${SOURCES})
source_group("generated" FILES ${GENERATED_SOURCE})
source_group("data" FILES ${DATA})
