message(STATUS "> Build MaterialsModule")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(MaterialsModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/MaterialsModule
)

file(GLOB_RECURSE SRCS ${MaterialsModule-Sources}/*.cpp)

qt_add_library(RedComplexMaterialsModule STATIC
	${SRCS}
)

target_link_libraries(RedComplexMaterialsModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::OpenGL
		Qt6::OpenGLWidgets

		RedComplexBase
		RedComplexDataStream
		RedComplexDataLayer
		RedComplexResources
		RedComplexGraphics
)
