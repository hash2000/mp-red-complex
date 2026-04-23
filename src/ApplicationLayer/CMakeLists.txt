message(STATUS "> Build ApplicationLayer")

set(ApplicationLayer-Sources
	${RedComplex_SOURCE_DIR}/src/ApplicationLayer
)

set(CMAKE_AUTOMOC ON)

file(GLOB_RECURSE SRCS ${ApplicationLayer-Sources}/*.cpp)
add_library(ApplicationLayer
	${SRCS}
)

target_link_libraries(ApplicationLayer
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		Base
		DataStream
		DataLayer
		Resources
		Graphics
)
