message(STATUS "> Build SystemMenu")

set(SystemMenu-Sources
	${RedComplex_SOURCE_DIR}/src/Content/SystemMenu
)

file(GLOB_RECURSE SRCS ${SystemMenu-Sources}/*.cpp)

qt_add_library(RedComplexSystemMenuModule STATIC
	${SRCS}
)

set_target_properties(RedComplexSystemMenuModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexSystemMenuModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexSystemMenuModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexSystemMenuModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexEngine
		RedComplexBase
		RedComplexBaseWidgets
		RedComplexResources
		RedComplexDatabaseModule
)

find_package(Qt6 REQUIRED COMPONENTS Test)
qt_add_executable(TestSystemMenu tests/test_system_menu.cpp)
target_include_directories(TestSystemMenu PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/tests
)
target_link_libraries(TestSystemMenu PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	Qt6::Test

	RedComplexSystemMenuModule
	RedComplexEngine
	RedComplexBase
	RedComplexBaseWidgets
	RedComplexResources
	RedComplexDatabaseModule
)
set_target_properties(TestSystemMenu PROPERTIES CMAKE_AUTOMOC ON)
