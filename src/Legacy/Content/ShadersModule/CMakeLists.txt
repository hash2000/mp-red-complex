message(STATUS "> Build ShadersModule")

set(ShadersModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/ShadersModule
)

file(GLOB_RECURSE SRCS ${ShadersModule-Sources}/*.cpp)

qt_add_library(RedComplexShadersModule STATIC
	${SRCS}
)

set_target_properties(RedComplexShadersModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexShadersModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexShadersModule PROPERTIES CMAKE_AUTOUIC ON)

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
