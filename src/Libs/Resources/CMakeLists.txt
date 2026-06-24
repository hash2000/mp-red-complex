message(STATUS "> Build Resources")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(Resources-Sources
	${RedComplex_SOURCE_DIR}/src/Libs/Resources
)

file(GLOB_RECURSE SRCS ${Resources-Sources}/*.cpp)

qt_add_library(RedComplexResources STATIC
	${SRCS}
)

target_link_libraries(RedComplexResources
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		SQLite3::SQLite3

		RedComplexBase
		RedComplexDataStream
)
