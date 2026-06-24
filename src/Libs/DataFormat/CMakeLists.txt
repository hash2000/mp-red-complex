message(STATUS "> Build DataFormat")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(DataFormat-Sources
	${RedComplex_SOURCE_DIR}/src/Libs/DataFormat
)

file(GLOB_RECURSE SRCS ${DataFormat-Sources}/*.cpp)

qt_add_library(RedComplexDataFormat STATIC
	${SRCS}
)

target_link_libraries(RedComplexDataFormat
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexDataStream
)
