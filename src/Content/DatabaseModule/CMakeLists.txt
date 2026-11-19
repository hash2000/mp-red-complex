message(STATUS "> Build DatabaseModule")

set(DatabaseModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/DatabaseModule
)

file(GLOB_RECURSE SRCS ${DatabaseModule-Sources}/*.cpp)
list(FILTER SRCS EXCLUDE REGEX "/tests/")

qt_add_library(RedComplexDatabaseModule STATIC
	${SRCS}
)

set_target_properties(RedComplexDatabaseModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexDatabaseModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexDatabaseModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexDatabaseModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexEngine
		RedComplexBase
		RedComplexBaseWidgets
		RedComplexResources
)

find_package(Qt6 REQUIRED COMPONENTS Test)
qt_add_executable(TestDatabasesService tests/test_databases_service.cpp)
target_link_libraries(TestDatabasesService PRIVATE
  Qt6::Core Qt6::Test RedComplexDatabaseModule RedComplexEngine RedComplexResources RedComplexBase
)
set_target_properties(TestDatabasesService PROPERTIES CMAKE_AUTOMOC ON)

qt_add_executable(TestLazyNodesDataProviderDb tests/test_lazy_nodes_data_provider_db.cpp)
target_link_libraries(TestLazyNodesDataProviderDb PRIVATE
  Qt6::Core Qt6::Test RedComplexDatabaseModule RedComplexEngine RedComplexResources RedComplexBase
  RedComplexBaseWidgets
)
set_target_properties(TestLazyNodesDataProviderDb PROPERTIES CMAKE_AUTOMOC ON)
