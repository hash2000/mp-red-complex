message(STATUS "> Build DatabaseModule")

set(DatabaseModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/DatabaseModule
)

file(GLOB_RECURSE SRCS ${DatabaseModule-Sources}/*.cpp)

qt_add_library(RedComplexDatabaseModule STATIC
	${SRCS}
)

set_target_properties(RedComplexDatabaseModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexDatabaseModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexDatabaseModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexDatabaseModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexResources
)
