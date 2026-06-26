message(STATUS "> Build Graphics")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(Graphics-Sources
	${RedComplex_SOURCE_DIR}/src/Libs/Graphics
)

file(GLOB_RECURSE SRCS ${Graphics-Sources}/*.cpp)

qt_add_library(RedComplexGraphics STATIC
	${SRCS}
)

get_target_property(QT_GUI_INC Qt6::Gui INTERFACE_INCLUDE_DIRECTORIES)

list(GET QT_GUI_INC 0 QT_GUI_BASE)
set(QT_VERSIONED_PATH "${QT_GUI_BASE}/${Qt6_VERSION}/QtGui")


target_include_directories(RedComplexGraphics PRIVATE
    ${QT_GUI_INC},
		${QT_VERSIONED_PATH}
)

target_link_libraries(RedComplexGraphics
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::OpenGL
		Qt6::OpenGLWidgets
)

