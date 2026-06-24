message(STATUS "> Build Engine")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(Engine-Sources
	${RedComplex_SOURCE_DIR}/src/Libs/Engine
)

file(GLOB_RECURSE SRCS ${Engine-Sources}/*.cpp)

qt_add_library(RedComplexEngine STATIC
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

