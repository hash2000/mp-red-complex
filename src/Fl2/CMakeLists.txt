message(STATUS "> Build Fl2")

set(Fl2-Sources
	${RedComplex_SOURCE_DIR}/src/Fl2
)

file(GLOB_RECURSE SRCS ${Fl2-Sources}/*.cpp)
add_library(Fl2
	${SRCS}
)

target_link_libraries(Fl2
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		Base
)
