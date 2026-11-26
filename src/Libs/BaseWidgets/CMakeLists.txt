message(STATUS "> Build BaseWidgets")

set(BaseWidgets-Sources
	${RedComplex_SOURCE_DIR}/src/Libs/BaseWidgets
)

file(GLOB_RECURSE SRCS ${BaseWidgets-Sources}/*.cpp)

# Заголовки с Q_OBJECT перечисляем явно: AUTOMOC не видит их через GLOB .cpp.
set(BaseWidgets-Headers
	${BaseWidgets-Sources}/editor/widgets/code_editor_widget.h
	${BaseWidgets-Sources}/editor/widgets/code_editor_search_panel.h
	${BaseWidgets-Sources}/editor/widgets/paged_text_editor.h
	${BaseWidgets-Sources}/editor/widgets/search_bar.h
	${BaseWidgets-Sources}/properties/property_descriptor_dialog.h
	${BaseWidgets-Sources}/widgets/background_tasks_indicator.h
)

qt_add_library(RedComplexBaseWidgets STATIC
	${SRCS}
	${BaseWidgets-Headers}
)

set_target_properties(RedComplexBaseWidgets PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexBaseWidgets PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexBaseWidgets PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexBaseWidgets
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::Concurrent
		Qt6::Core5Compat

		yaml-cpp::yaml-cpp

		RedComplexBase
		RedComplexDataStream
		RedComplexResources
)
