message(STATUS "> Build DatabaseModule")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(DatabaseModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/DatabaseModule
)

file(GLOB_RECURSE SRCS ${DatabaseModule-Sources}/*.cpp)

qt_add_library(RedComplexDatabaseModule STATIC
	${SRCS}
)

target_link_libraries(RedComplexDatabaseModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexResources
)
