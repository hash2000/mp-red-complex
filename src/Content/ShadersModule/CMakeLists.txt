message(STATUS "> Build ShadersModule")

set(ShadersModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/ShadersModule
)

file(GLOB_RECURSE SRCS ${ShadersModule-Sources}/*.cpp)

add_library(RedComplexShadersModule
	${SRCS}
)

target_link_libraries(RedComplexShadersModule
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
