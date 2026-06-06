message(STATUS "> Build UsersModule")

set(UsersModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/UsersModule
)

file(GLOB_RECURSE SRCS ${UsersModule-Sources}/*.cpp)

add_library(RedComplexUsersModule
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
