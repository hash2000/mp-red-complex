message(STATUS "> Build CodeEditorWidget")

set(CodeEditorWidget-Sources
	${RedComplex_SOURCE_DIR}/src/CodeEditorWidget
)

file(GLOB_RECURSE SRCS ${CodeEditorWidget-Sources}/*.cpp)
add_library(CodeEditorWidget
	${SRCS}
)

target_link_libraries(CodeEditorWidget
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
)
