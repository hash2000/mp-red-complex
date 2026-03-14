cmake_minimum_required(VERSION 4.1)
cmake_policy(SET CMP0146 NEW)

project(RedComplex LANGUAGES CXX)

include("cmake/compile.cmake")
include("cmake/modules.cmake")

include_directories(
	${RedComplex_SOURCE_DIR}/src
	${RedComplex_SOURCE_DIR}/tests
)

message(STATUS ${RedComplex_SOURCE_DIR})

add_subdirectory(src)
add_subdirectory(tests)

