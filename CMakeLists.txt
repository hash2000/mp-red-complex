cmake_minimum_required(VERSION 4.1)

project(RedComplex LANGUAGES CXX)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include("cmake/modules.cmake")

include_directories(
	${RedComplex_SOURCE_DIR}
)

add_subdirectory(src)
