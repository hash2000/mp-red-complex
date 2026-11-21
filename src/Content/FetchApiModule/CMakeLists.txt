message(STATUS "> Build FetchApiModule")

set(FetchApiModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/FetchApiModule
)

file(GLOB_RECURSE SRCS ${FetchApiModule-Sources}/*.cpp)
list(FILTER SRCS EXCLUDE REGEX "/tests/")

qt_add_library(RedComplexFetchApiModule STATIC
	${SRCS}
)

set_target_properties(RedComplexFetchApiModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexFetchApiModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexFetchApiModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexFetchApiModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::Network

		RedComplexEngine
		RedComplexBase
		RedComplexBaseWidgets
		RedComplexResources
		RedComplexCodeEditorWidget
)

qt_import_plugins(RedComplexFetchApiModule
    INCLUDE
			Qt6::QTlsBackendOpenSSLPlugin
			Qt6::QSchannelBackendPlugin
			Qt6::QTlsBackendCertOnlyPlugin
)

find_package(Qt6 REQUIRED COMPONENTS Test)
qt_add_executable(TestFetchApi tests/test_fetch_api.cpp)
target_include_directories(TestFetchApi PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/tests
)
target_link_libraries(TestFetchApi PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
	Qt6::Network
	Qt6::Test

	RedComplexFetchApiModule
	RedComplexEngine
	RedComplexBase
	RedComplexBaseWidgets
	RedComplexResources
)
set_target_properties(TestFetchApi PROPERTIES CMAKE_AUTOMOC ON)
