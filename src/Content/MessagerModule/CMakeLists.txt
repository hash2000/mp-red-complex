message(STATUS "> Build MessagerModule")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(MessagerModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/MessagerModule
)

file(GLOB_RECURSE SRCS ${MessagerModule-Sources}/*.cpp)

qt_add_library(RedComplexMessagerModule STATIC
	${SRCS}
)

target_link_libraries(RedComplexMessagerModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::OpenGL
		Qt6::OpenGLWidgets

		RedComplexBase
		RedComplexDataStream
		RedComplexResources
)
