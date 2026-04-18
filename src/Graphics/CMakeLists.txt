message(STATUS "> Build Graphics")

set(Graphics-Sources
	${RedComplex_SOURCE_DIR}/src/Graphics
)

# Включаем MOC для Qt-классов с Q_OBJECT
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON) # Для ресурсов шейдеров

file(GLOB_RECURSE SRCS ${Graphics-Sources}/*.cpp)

# Шейдеры как ресурсы
set(Graphics-Resources
	${RedComplex_SOURCE_DIR}/src/graphics/graphics_resources.qrc
	${RedComplex_SOURCE_DIR}/src/Graphics/shaders/tile.vert
	${RedComplex_SOURCE_DIR}/src/Graphics/shaders/tile.frag
)

add_library(Graphics
	${SRCS}
	${Graphics-Resources}
)

target_link_libraries(Graphics
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::OpenGL
		Qt6::OpenGLWidgets
)

