message(STATUS "> Build Resources")

set(Resources-Sources
	${RedComplex_SOURCE_DIR}/src/Resources
)

file(GLOB_RECURSE SRCS ${Resources-Sources}/*.cpp)
add_library(Resources
	${SRCS}
)

target_link_libraries(Resources
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		Base
)
