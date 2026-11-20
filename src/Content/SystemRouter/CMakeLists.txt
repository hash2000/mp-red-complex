message(STATUS "> Build SystemRouter")

set(SystemRouter-Sources
	${RedComplex_SOURCE_DIR}/src/Content/SystemRouter
)

file(GLOB_RECURSE SRCS ${SystemRouter-Sources}/*.cpp)
list(FILTER SRCS EXCLUDE REGEX "/tests/")

qt_add_library(RedComplexSystemRouterModule STATIC
	${SRCS}
)

set_target_properties(RedComplexSystemRouterModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexSystemRouterModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexSystemRouterModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexSystemRouterModule
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
qt_add_executable(TestSystemRouter tests/test_system_router.cpp)
target_include_directories(TestSystemRouter PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/tests
)
target_link_libraries(TestSystemRouter PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	Qt6::Test

	RedComplexSystemRouterModule
	RedComplexEngine
	RedComplexBase
	RedComplexBaseWidgets
	RedComplexResources
	RedComplexDatabaseModule
)
set_target_properties(TestSystemRouter PROPERTIES CMAKE_AUTOMOC ON)

qt_add_executable(TestSystemRouterCommandsDb tests/test_system_router_commands_db.cpp)
target_include_directories(TestSystemRouterCommandsDb PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/tests
)
target_link_libraries(TestSystemRouterCommandsDb PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	Qt6::Test

	RedComplexSystemRouterModule
	RedComplexEngine
	RedComplexBase
	RedComplexBaseWidgets
	RedComplexResources
	RedComplexDatabaseModule
)
set_target_properties(TestSystemRouterCommandsDb PROPERTIES CMAKE_AUTOMOC ON)
