message(STATUS "> Build InventoriesModule")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(InventoriesModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/InventoriesModule
)

file(GLOB_RECURSE SRCS ${InventoriesModule-Sources}/*.cpp)

qt_add_library(RedComplexInventoriesModule STATIC
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
