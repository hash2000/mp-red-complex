message(STATUS "> Build BaseWidgets")

set(BaseWidgets-Sources
	${RedComplex_SOURCE_DIR}/src/BaseWidgets
)

file(GLOB_RECURSE SRCS ${BaseWidgets-Sources}/*.cpp)
add_library(BaseWidgets
	${SRCS}
)

target_link_libraries(BaseWidgets
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
)
