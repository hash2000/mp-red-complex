message(STATUS "> Build Engine")

set(Engine-Sources
	${RedComplex_SOURCE_DIR}/src/Libs/Engine
)

set(CMAKE_AUTOMOC ON)

file(GLOB_RECURSE SRCS ${Engine-Sources}/*.cpp)


add_library(RedComplexEngine
	${SRCS}
)

target_include_directories(RedComplexEngine PRIVATE
	${ZLIB_INCLUDE_DIRS})

target_link_libraries(RedComplexEngine
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
)

