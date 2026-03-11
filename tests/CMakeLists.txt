cmake_minimum_required(VERSION 4.1)

project(RedComplexTests LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

# Моковый ItemsDataProvider для тестов
set(Test-Sources
    test_items_service.cpp
    mock_items_data_provider.h
    mock_items_data_provider.cpp
)

# Автоматическое включение MOC для Qt-классов
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(RunTests ${Test-Sources})

target_include_directories(RunTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(RunTests PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
    Qt6::Test
    
    Resources
    DataLayer
    ApplicationLayer
)

# Включаем тесты в CTest
enable_testing()
add_test(NAME RunTests COMMAND RunTests)

# Вывод результатов в формате XML (для CI)
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS RunTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
