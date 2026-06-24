message(STATUS "> Build UsersModule")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(UsersModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/UsersModule
)

file(GLOB_RECURSE SRCS ${UsersModule-Sources}/*.cpp)

qt_add_library(RedComplexUsersModule STATIC
	${SRCS}
)

target_link_libraries(RedComplexUsersModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexDataStream
		RedComplexDataLayer
		RedComplexResources
)
