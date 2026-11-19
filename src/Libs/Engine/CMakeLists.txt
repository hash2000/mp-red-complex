message(STATUS "> Build Engine")

set(Engine-Sources
	${RedComplex_SOURCE_DIR}/src/Libs/Engine
)

file(GLOB_RECURSE SRCS ${Engine-Sources}/*.cpp)

qt_add_library(RedComplexEngine STATIC
	${SRCS}
)

target_include_directories(RedComplexEngine PRIVATE
	${ZLIB_INCLUDE_DIRS})

set_target_properties(RedComplexUsersModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexUsersModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexUsersModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexEngine
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::OpenGLWidgets
		Qt6::Xml

		yaml-cpp::yaml-cpp

		RedComplexBase
		RedComplexBaseWidgets
		RedComplexGraphics
)

