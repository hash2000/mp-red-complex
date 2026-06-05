message(STATUS "> Build BaseWidgets")

set(BaseWidgets-Sources
	${RedComplex_SOURCE_DIR}/src/Content/BaseWidgets
)

file(GLOB_RECURSE SRCS ${BaseWidgets-Sources}/*.cpp)

add_library(RedComplexBaseWidgets
	${SRCS}
)

target_link_libraries(RedComplexBaseWidgets
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
)
