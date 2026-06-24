message(STATUS "> Build TexturesModule")

set(TexturesModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/TexturesModule
)

file(GLOB_RECURSE SRCS ${TexturesModule-Sources}/*.cpp)

add_library(RedComplexTexturesModule
	${SRCS}
)

target_link_libraries(RedComplexTexturesModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexDataStream
		RedComplexDataLayer
		RedComplexResources
)
