message(STATUS "> Build FetchApiModule")

set(FetchApiModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/FetchApiModule
)

file(GLOB_RECURSE SRCS ${FetchApiModule-Sources}/*.cpp)

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

		RedComplexBase
		RedComplexDataStream
		RedComplexResources
)

qt_import_plugins(RedComplexFetchApiModule
    INCLUDE
			Qt6::QTlsBackendOpenSSLPlugin
			Qt6::QSchannelBackendPlugin
			Qt6::QTlsBackendCertOnlyPlugin
)
