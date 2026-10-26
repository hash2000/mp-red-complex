message(STATUS "> Build TexturesModule")

set(TexturesModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/TexturesModule
)

file(GLOB_RECURSE SRCS ${TexturesModule-Sources}/*.cpp)

qt_add_library(RedComplexTexturesModule STATIC
	${SRCS}
)

set_target_properties(RedComplexTexturesModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexTexturesModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexTexturesModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexTexturesModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexDataStream
		RedComplexResources
)
