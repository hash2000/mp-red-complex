message(STATUS "> Build Content.Shared")

set(ContentShared-Sources
	${RedComplex_SOURCE_DIR}/src/Content/Shared
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
		Qt6::OpenGL
		Qt6::OpenGLWidgets

		Base
		DataFormat
		DataStream
		Resources
)

