message(STATUS "> Build Content.Maps")

set(ContentMaps-Sources
	${RedComplex_SOURCE_DIR}/src/Content/Maps
)

file(GLOB_RECURSE SRCS ${ContentMaps-Sources}/*.cpp)


add_library(Content.Maps
	${SRCS}
)

target_include_directories(Content.Maps PRIVATE
	${ZLIB_INCLUDE_DIRS})

target_link_libraries(Content.Maps
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		Base
		DataFormat
		DataStream
		Resources
)

