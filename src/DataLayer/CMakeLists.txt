message(STATUS "> Build DataLayer")

set(DataLayer-Sources
	${RedComplex_SOURCE_DIR}/src/DataLayer
)

file(GLOB_RECURSE SRCS ${DataLayer-Sources}/*.cpp)

add_library(RedComplexDataLayer
	${SRCS}
)

target_link_libraries(RedComplexDataLayer
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexDataStream
		RedComplexResources
)
