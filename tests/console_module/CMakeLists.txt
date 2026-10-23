cmake_minimum_required(VERSION 4.1)

project(TestItemsService LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

add_executable(TestConsoleModule
    test_command.h
    test_command.cpp
		test_command_context.cpp
    test_command_parsers.cpp
)

target_include_directories(TestConsoleModule PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(TestConsoleModule PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui

    RedComplexConsoleModule
		RedComplexResources
		RedComplexGraphics
)

add_test(NAME TestConsoleModule COMMAND TestConsoleModule )
