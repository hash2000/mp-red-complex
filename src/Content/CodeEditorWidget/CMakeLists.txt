message(STATUS "> Build CodeEditorWidget")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CodeEditorWidget-Sources
	${RedComplex_SOURCE_DIR}/src/Content/CodeEditorWidget
)

file(GLOB_RECURSE SRCS ${CodeEditorWidget-Sources}/*.cpp)

qt_add_library(RedComplexCodeEditorWidget STATIC
	${SRCS}
)

target_link_libraries(RedComplexCodeEditorWidget
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
)
