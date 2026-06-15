message(STATUS "> Build InventroiesModule")

set(InventroiesModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/InventroiesModule
)

file(GLOB_RECURSE SRCS ${InventroiesModule-Sources}/*.cpp)

add_library(RedComplexInventroiesModule
	${SRCS}
)

target_link_libraries(RedComplexInventroiesModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexDataStream
		RedComplexDataLayer
		RedComplexResources
)
