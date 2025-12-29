message(STATUS "> Build Engine")

set(Engine-Sources
	${RedComplex_SOURCE_DIR}/src/Engine
)

file(GLOB_RECURSE SRCS ${Engine-Sources}/*.cpp)


add_library(Engine
	${SRCS}
)

target_include_directories(Engine PRIVATE
	${ZLIB_INCLUDE_DIRS})

target_link_libraries(Engine
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		Base
)

