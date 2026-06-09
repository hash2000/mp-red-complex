cmake_minimum_required(VERSION 4.1)

project(TestInventoriesService LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

add_executable(TestInventoriesService
    test_inventories_service.cpp
)

target_include_directories(TestInventoriesService PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(TestInventoriesService PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui

    RedComplexResources
    RedComplexDataLayer
    RedComplexApplicationLayer
)

add_test(NAME TestInventoriesService COMMAND TestInventoriesService)
