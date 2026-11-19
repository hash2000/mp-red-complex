cmake_minimum_required(VERSION 4.1)

project(TestLazyTree LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

add_executable(TestLazyTreeView
	test_lazy_tree_view.cpp
	mock_lazy_tree_provider.h
)

target_include_directories(TestLazyTreeView PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(TestLazyTreeView PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	Qt6::Test

	RedComplexBaseWidgets
)

set_target_properties(TestLazyTreeView PROPERTIES CMAKE_AUTOMOC ON)
add_test(NAME TestLazyTreeView COMMAND TestLazyTreeView)

add_executable(TestLazyTreeWidget
	test_lazy_tree_widget.cpp
	mock_lazy_tree_provider.h
)

target_include_directories(TestLazyTreeWidget PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(TestLazyTreeWidget PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	Qt6::Test

	RedComplexBaseWidgets
)

set_target_properties(TestLazyTreeWidget PROPERTIES CMAKE_AUTOMOC ON)
add_test(NAME TestLazyTreeWidget COMMAND TestLazyTreeWidget)
