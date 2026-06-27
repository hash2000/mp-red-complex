message(STATUS "> Build FetchApiModule")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(FetchApiModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/FetchApiModule
)

file(GLOB_RECURSE SRCS ${FetchApiModule-Sources}/*.cpp)

qt_add_library(RedComplexFetchApiModule STATIC
	${SRCS}
)

target_link_libraries(RedComplexFetchApiModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::OpenGL
		Qt6::OpenGLWidgets
		Qt6::Network

		RedComplexBase
		RedComplexDataStream
		RedComplexResources
)
