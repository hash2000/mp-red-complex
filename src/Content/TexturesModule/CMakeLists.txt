message(STATUS "> Build TexturesModule")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(TexturesModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/TexturesModule
)

file(GLOB_RECURSE SRCS ${TexturesModule-Sources}/*.cpp)

qt_add_library(RedComplexTexturesModule STATIC
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
