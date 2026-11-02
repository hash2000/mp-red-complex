cmake_minimum_required(VERSION 4.1)

project(TestItemsService LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

add_executable(TestEngine
    test_command.h
    test_command.cpp
		test_command_context.cpp
    test_command_parsers.cpp
    test_command_parsers_palette.cpp
)

target_include_directories(TestEngine PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(TestEngine PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui

    RedComplexConsoleModule
		RedComplexResources
		RedComplexGraphics
)

add_test(NAME TestEngine COMMAND TestEngine)
