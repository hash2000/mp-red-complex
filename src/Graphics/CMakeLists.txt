message(STATUS "> Build Graphics")

set(Graphics-Sources
	${RedComplex_SOURCE_DIR}/src/Graphics
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

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

