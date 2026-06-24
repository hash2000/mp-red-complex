message(STATUS "> Build MaterialsModule")

set(MaterialsModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/MaterialsModule
)

file(GLOB_RECURSE SRCS ${MaterialsModule-Sources}/*.cpp)

add_library(RedComplexMaterialsModule
	${SRCS}
)

target_link_libraries(RedComplexMaterialsModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexDataStream
		RedComplexDataLayer
		RedComplexResources
		RedComplexGraphics
)
