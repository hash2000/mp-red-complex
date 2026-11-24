message(STATUS "> Build CodeEditorWidget")

set(CodeEditorWidget-Sources
	${RedComplex_SOURCE_DIR}/src/Content/CodeEditorWidget
)

file(GLOB_RECURSE SRCS ${CodeEditorWidget-Sources}/*.cpp)

# Заголовок с Q_OBJECT добавляем явно: AUTOMOC собирает moc по списку
# исходников цели, а GLOB выше перечисляет только .cpp.
set(CodeEditorWidget-Headers
	${CodeEditorWidget-Sources}/documents/paged_text_document.h
)

qt_add_library(RedComplexCodeEditorWidget STATIC
	${SRCS}
	${CodeEditorWidget-Headers}
)

set_target_properties(RedComplexCodeEditorWidget PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexCodeEditorWidget PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexCodeEditorWidget PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexCodeEditorWidget
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::Concurrent
		Qt6::Core5Compat

		yaml-cpp::yaml-cpp

		RedComplexEngine
)
