message(STATUS "> Build Content.ShaderEditor")

set(ContentShaderEditor-Sources
	${RedComplex_SOURCE_DIR}/src/Content/ShaderEditor
)

file(GLOB_RECURSE SRCS ${ContentShaderEditor-Sources}/*.cpp)


add_library(Content.ShaderEditor
	${SRCS}
)

target_include_directories(Content.ShaderEditor PRIVATE
	${ZLIB_INCLUDE_DIRS})

target_link_libraries(Content.ShaderEditor
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

