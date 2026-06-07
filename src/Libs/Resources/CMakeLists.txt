message(STATUS "> Build Resources")

set(Resources-Sources
	${RedComplex_SOURCE_DIR}/src/Libs/Resources
)

set(CMAKE_AUTOMOC ON)

file(GLOB_RECURSE SRCS ${Resources-Sources}/*.cpp)

add_library(RedComplexResources
	${SRCS}
)

target_link_libraries(RedComplexResources
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::Sql

		RedComplexBase
		RedComplexDataStream
)
