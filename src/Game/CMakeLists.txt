message(STATUS "> Build Game")

set(Game-Sources
	${RedComplex_SOURCE_DIR}/src/Game
)

file(GLOB_RECURSE SRCS ${Game-Sources}/*.cpp)
add_executable(Game
	${SRCS}
)

target_link_libraries(Game
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::OpenGL
		Qt6::OpenGLWidgets

		Base
		Resources
		DataLayer
		Engine
		Content.Shared
)

