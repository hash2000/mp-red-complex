cmake_minimum_required(VERSION 4.1)

project(RedComplexTests LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

# Моковый ItemsDataProvider для тестов (общий для всех тестов)
set(Test-Mocks
    mock_items_data_provider.h
    mock_items_data_provider.cpp
)

# Автоматическое включение MOC для Qt-классов
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Поддиректории с тестами
add_subdirectory(items_service)
add_subdirectory(inventories_service)

# Включаем тесты в CTest
enable_testing()

# Цель для запуска всех тестов
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS TestItemsService TestInventoriesService
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
