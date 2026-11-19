message(STATUS "> Build InventoriesModule")

set(InventoriesModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/InventoriesModule
)

file(GLOB_RECURSE SRCS ${InventoriesModule-Sources}/*.cpp)

qt_add_library(RedComplexInventoriesModule STATIC
	${SRCS}
)

set_target_properties(RedComplexInventoriesModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexInventoriesModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexInventoriesModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexInventoriesModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexDataStream
		RedComplexResources

		RedComplexTexturesModule
)
