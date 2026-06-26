message(STATUS "> Build ShadersModule")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(ShadersModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/ShadersModule
)

file(GLOB_RECURSE SRCS ${ShadersModule-Sources}/*.cpp)

qt_add_library(RedComplexShadersModule STATIC
	${SRCS}
)

target_link_libraries(RedComplexShadersModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::OpenGL
		Qt6::OpenGLWidgets

		RedComplexBase
		RedComplexDataStream
		RedComplexResources
		RedComplexGraphics
)
