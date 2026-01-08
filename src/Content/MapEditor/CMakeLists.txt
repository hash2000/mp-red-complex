message(STATUS "> Build Content.MapEditor")

set(ContentMapEditor-Sources
	${RedComplex_SOURCE_DIR}/src/Content/MapEditor
)

file(GLOB_RECURSE SRCS ${ContentMapEditor-Sources}/*.cpp)


add_library(Content.MapEditor
	${SRCS}
)

target_include_directories(Content.MapEditor PRIVATE
	${ZLIB_INCLUDE_DIRS})

target_link_libraries(Content.MapEditor
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		Base
		DataFormat
		DataStream
		Resources
)

