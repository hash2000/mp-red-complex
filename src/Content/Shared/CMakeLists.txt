message(STATUS "> Build Content.Shared")

set(ContentShared-Sources
	${RedComplex_SOURCE_DIR}/src/Shared
)

file(GLOB_RECURSE SRCS ${ContentShared-Sources}/*.cpp)


add_library(Content.Shared
	${SRCS}
)

target_include_directories(Content.Shared PRIVATE
	${ZLIB_INCLUDE_DIRS})

target_link_libraries(Content.Shared
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		Base
		DataFormat
		DataStream
		Resources
)

