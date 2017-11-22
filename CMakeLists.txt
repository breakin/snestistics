cmake_minimum_required (VERSION 2.8.11)
project(snestistics)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

if(MSVC)
	add_compile_options(/MP) # Allow multi-processor building
endif(MSVC)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory(deps)
add_subdirectory(source)
