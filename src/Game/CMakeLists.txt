message(STATUS "> Build Game")

set(Game-Sources
	${RedComplex_SOURCE_DIR}/src/Game
)

file(GLOB_RECURSE SRCS ${Game-Sources}/*.cpp)
add_library(Game
	${SRCS}
)

target_link_libraries(Game
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		Base
		DataStream
)
