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

# Вкладки документов: правила сортировки/группировки, перетаскивание,
# цвет вкладки и документ системных настроек.
add_executable(TestDocumentTabs
    test_document_tabs.cpp
)

target_include_directories(TestDocumentTabs PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(TestDocumentTabs PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test

    RedComplexEngine
    RedComplexBase
    RedComplexBaseWidgets
    RedComplexResources
)

add_test(NAME TestDocumentTabs COMMAND TestDocumentTabs)

# Тест проверяет эталонную тему: цвета и глифы вкладок заданы в
# assets/themes/{theme}/documents.yaml.
target_compile_definitions(TestDocumentTabs PRIVATE
    REDCOMPLEX_CONFIGURATIONS_DIR="${CMAKE_SOURCE_DIR}/src/Configurations"
)

# Панель свойств: сведения о контексте документа (имя и переменные).
add_executable(TestPropertiesPanel
    test_properties_panel.cpp
)

target_include_directories(TestPropertiesPanel PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(TestPropertiesPanel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test

    RedComplexEngine
    RedComplexBase
    RedComplexBaseWidgets
    RedComplexResources
)

add_test(NAME TestPropertiesPanel COMMAND TestPropertiesPanel)

add_executable(TestClassManager
    test_class_manager.cpp
)

target_include_directories(TestClassManager PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(TestClassManager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test

    RedComplexEngine
    RedComplexBase
    RedComplexResources
)

# Тест читает эталонные yaml-описания прямо из исходников.
target_compile_definitions(TestClassManager PRIVATE
    REDCOMPLEX_CONFIGURATIONS_DIR="${CMAKE_SOURCE_DIR}/src/Configurations"
)

add_test(NAME TestClassManager COMMAND TestClassManager)

# Тест читает правила подсветки прямо из темы в исходниках.
add_executable(TestHighlighters
    test_highlighters.cpp
)

target_include_directories(TestHighlighters PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(TestHighlighters PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test

    RedComplexCodeEditorWidget
    RedComplexEngine
    RedComplexBase
    RedComplexResources
)

target_compile_definitions(TestHighlighters PRIVATE
    REDCOMPLEX_CONFIGURATIONS_DIR="${CMAKE_SOURCE_DIR}/src/Configurations"
)

add_test(NAME TestHighlighters COMMAND TestHighlighters)

# Тест логики поиска в документе редактора кода.
add_executable(TestCodeEditorSearch
    test_code_editor_search.cpp
)

target_include_directories(TestCodeEditorSearch PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(TestCodeEditorSearch PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Core5Compat

    RedComplexCodeEditorWidget
    RedComplexEngine
    RedComplexBase
    RedComplexResources
)

add_test(NAME TestCodeEditorSearch COMMAND TestCodeEditorSearch)

# Прототип виртуального редактора больших файлов.
add_executable(TestPagedTextEditor
    test_paged_text_editor.cpp
)
target_include_directories(TestPagedTextEditor PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)
target_link_libraries(TestPagedTextEditor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Core5Compat

    RedComplexCodeEditorWidget
    RedComplexEngine
    RedComplexBase
    RedComplexResources
)
add_test(NAME TestPagedTextEditor COMMAND TestPagedTextEditor)

# Тест подсветки и форматирования читает правила из эталонной темы.
target_compile_definitions(TestCodeEditorSearch PRIVATE
    REDCOMPLEX_CONFIGURATIONS_DIR="${CMAKE_SOURCE_DIR}/src/Configurations"
)

# Стенд измерений редактора кода (не тест, в CTest не включён).
add_executable(BenchCodeEditor
    bench_code_editor.cpp
)

target_include_directories(BenchCodeEditor PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(BenchCodeEditor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets

    RedComplexCodeEditorWidget
    RedComplexEngine
    RedComplexBase
    RedComplexResources
)

target_compile_definitions(BenchCodeEditor PRIVATE
    REDCOMPLEX_CONFIGURATIONS_DIR="${CMAKE_SOURCE_DIR}/src/Configurations"
)
