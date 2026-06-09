message(STATUS "> Build Resources")

set(Resources-Sources
	${RedComplex_SOURCE_DIR}/src/Libs/Resources
)

file(GLOB_RECURSE SRCS ${Resources-Sources}/*.cpp)

add_library(RedComplexResources
	${SRCS}
)

target_link_libraries(RedComplexResources
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		SQLite::SQLite3

		RedComplexBase
		RedComplexDataStream
)
