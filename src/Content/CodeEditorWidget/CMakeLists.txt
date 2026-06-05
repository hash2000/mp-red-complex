message(STATUS "> Build CodeEditorWidget")

set(CodeEditorWidget-Sources
	${RedComplex_SOURCE_DIR}/src/Content/CodeEditorWidget
)

file(GLOB_RECURSE SRCS ${CodeEditorWidget-Sources}/*.cpp)

add_library(RedComplexCodeEditorWidget
	${SRCS}
)

target_link_libraries(RedComplexCodeEditorWidget
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
)
