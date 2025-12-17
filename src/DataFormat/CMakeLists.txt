message(STATUS "> Build DataFormat")

set(DataFormat-Sources
	${RedComplex_SOURCE_DIR}/src/DataFormat
)

file(GLOB_RECURSE SRCS ${DataFormat-Sources}/*.cpp)
add_library(DataFormat
	${SRCS}
)

target_link_libraries(DataFormat
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		Base
		DataStream
)
