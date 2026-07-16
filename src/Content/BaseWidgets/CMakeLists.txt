message(STATUS "> Build BaseWidgets")

set(BaseWidgets-Sources
	${RedComplex_SOURCE_DIR}/src/Content/BaseWidgets
)

file(GLOB_RECURSE SRCS ${BaseWidgets-Sources}/*.cpp)

qt_add_library(RedComplexBaseWidgets STATIC
	${SRCS}
)

set_target_properties(RedComplexBaseWidgets PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexBaseWidgets PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexBaseWidgets PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexBaseWidgets
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
)
