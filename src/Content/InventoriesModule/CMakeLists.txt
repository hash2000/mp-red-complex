message(STATUS "> Build InventoriesModule")

set(InventoriesModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/InventoriesModule
)

file(GLOB_RECURSE SRCS ${InventoriesModule-Sources}/*.cpp)

add_library(RedComplexInventoriesModule
	${SRCS}
)

target_link_libraries(RedComplexInventoriesModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexDataStream
		RedComplexDataLayer
		RedComplexResources
)
