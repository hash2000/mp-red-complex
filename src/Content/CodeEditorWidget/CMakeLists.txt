message(STATUS "> Build CodeEditorWidget")

set(CodeEditorWidget-Sources
	${RedComplex_SOURCE_DIR}/src/Content/CodeEditorWidget
)

file(GLOB_RECURSE SRCS ${CodeEditorWidget-Sources}/*.cpp)

qt_add_library(RedComplexCodeEditorWidget STATIC
	${SRCS}
)

set_target_properties(RedComplexCodeEditorWidget PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexCodeEditorWidget PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexCodeEditorWidget PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexCodeEditorWidget
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
)
