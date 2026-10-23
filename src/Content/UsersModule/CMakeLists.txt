message(STATUS "> Build UsersModule")

set(UsersModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/UsersModule
)

file(GLOB_RECURSE SRCS ${UsersModule-Sources}/*.cpp)

qt_add_library(RedComplexUsersModule STATIC
	${SRCS}
)

target_include_directories(RedComplexUsersModule PRIVATE
	${SODIUM_INCLUDE_DIRS}
)

set_target_properties(RedComplexUsersModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexUsersModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexUsersModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexUsersModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		unofficial-sodium::sodium

		RedComplexBase
		RedComplexDataStream
		RedComplexResources

		RedComplexTexturesModule
)
