message(STATUS "> Build TextEditorModule")

set(TextEditorModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/TextEditorModule
)

file(GLOB_RECURSE SRCS ${TextEditorModule-Sources}/*.cpp)

# Заголовок с Q_OBJECT добавляем явно: AUTOMOC собирает moc по списку
# исходников цели, а GLOB выше перечисляет только .cpp.
set(TextEditorModule-Headers
	${TextEditorModule-Sources}/documents/paged_text_document.h
	${TextEditorModule-Sources}/widgets/search_bar.h
)

qt_add_library(RedComplexTextEditorModule STATIC
	${SRCS}
	${TextEditorModule-Headers}
)

set_target_properties(RedComplexTextEditorModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexTextEditorModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexTextEditorModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexTextEditorModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::Concurrent
		Qt6::Core5Compat

		yaml-cpp::yaml-cpp

		RedComplexEngine
)
