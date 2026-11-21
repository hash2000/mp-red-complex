cmake_minimum_required(VERSION 4.1)

project(RedComplexTests LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

# Моковый IItemsDataProvider для тестов (общий для всех тестов)
set(Test-Mocks
    mock_items_data_provider.h
    mock_items_data_provider.cpp
)

# Поддиректории с тестами
add_subdirectory(items_service)
add_subdirectory(inventories_service)
add_subdirectory(engine)
add_subdirectory(ui)
add_subdirectory(basewidgets)

# Включаем тесты в CTest
enable_testing()
add_test(NAME TestDatabasesService COMMAND TestDatabasesService)
add_test(NAME TestLazyNodesDataProviderDb COMMAND TestLazyNodesDataProviderDb)
add_test(NAME TestSystemRouter COMMAND TestSystemRouter)
add_test(NAME TestSystemRouterCommandsDb COMMAND TestSystemRouterCommandsDb)
add_test(NAME TestFetchApi COMMAND TestFetchApi)

# Цель для запуска всех тестов
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS TestItemsService TestInventoriesService TestEngine TestDatabasesService TestUiZones TestDocumentTabs TestPropertiesPanel TestClassManager TestHighlighters TestCodeEditorSearch TestLazyTreeView TestLazyTreeWidget TestLazyNodesDataProviderDb TestSystemRouter TestSystemRouterCommandsDb TestFetchApi
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
