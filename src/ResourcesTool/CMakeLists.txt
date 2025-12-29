message(STATUS "> Build ResourcesTool")

set(ResourcesTool-Sources
	${RedComplex_SOURCE_DIR}/src/ResourcesTool
)

file(GLOB_RECURSE SRCS ${ResourcesTool-Sources}/*.cpp)
add_executable(ResourcesTool
	${SRCS}
)

target_link_libraries(ResourcesTool
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::OpenGL
		Qt6::OpenGLWidgets

		${ZLIB_LIBRARIES}

		Base
		Resources
		DataFormat
		Engine
)

