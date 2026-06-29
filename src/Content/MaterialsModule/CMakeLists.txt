message(STATUS "> Build MaterialsModule")

set(MaterialsModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/MaterialsModule
)

file(GLOB_RECURSE SRCS ${MaterialsModule-Sources}/*.cpp)

qt_add_library(RedComplexMaterialsModule STATIC
	${SRCS}
)

set_target_properties(RedComplexMaterialsModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexMaterialsModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexMaterialsModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexMaterialsModule
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
