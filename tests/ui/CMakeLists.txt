cmake_minimum_required(VERSION 4.1)

project(TestUiZones LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

add_executable(TestUiZones
    test_ui_zones.cpp
)

target_include_directories(TestUiZones PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(TestUiZones PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test

    RedComplexEngine
    RedComplexBase
    RedComplexResources
)

add_test(NAME TestUiZones COMMAND TestUiZones)
