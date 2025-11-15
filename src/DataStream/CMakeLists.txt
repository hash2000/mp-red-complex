message(STATUS "> Build DataStream")

set(DataStream-Sources
	${RedComplex_SOURCE_DIR}/src/DataStream
)

file(GLOB_RECURSE SRCS ${DataStream-Sources}/*.cpp)
add_library(DataStream
	${SRCS}
)

target_link_libraries(DataStream
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		Base
)
