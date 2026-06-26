message(STATUS "> Build BaseWidgets")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(BaseWidgets-Sources
	${RedComplex_SOURCE_DIR}/src/Content/BaseWidgets
)

file(GLOB_RECURSE SRCS ${BaseWidgets-Sources}/*.cpp)

qt_add_library(RedComplexBaseWidgets STATIC
	${SRCS}
)

target_link_libraries(RedComplexBaseWidgets
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
)
