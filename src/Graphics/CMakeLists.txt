message(STATUS "> Build Graphics")

set(Graphics-Sources
	${RedComplex_SOURCE_DIR}/src/Graphics
)

# Включаем MOC для Qt-классов с Q_OBJECT
set(CMAKE_AUTOMOC ON)

file(GLOB_RECURSE SRCS ${Graphics-Sources}/*.cpp)
add_library(Graphics
	${SRCS}
)

target_link_libraries(Graphics
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::OpenGL
		Qt6::OpenGLWidgets
)

