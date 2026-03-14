cmake_minimum_required(VERSION 4.1)

project(TestItemsService LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(TestItemsService
    test_items_service.cpp
    ../mock_items_data_provider.h
    ../mock_items_data_provider.cpp
)

target_include_directories(TestItemsService PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(TestItemsService PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Resources
    DataLayer
    ApplicationLayer
)

add_test(NAME TestItemsService COMMAND TestItemsService)
