message(STATUS "> Build DatabaseModule")

set(DatabaseModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/DatabaseModule
)

file(GLOB_RECURSE SRCS ${DatabaseModule-Sources}/*.cpp)

add_library(RedComplexDatabaseModule
	${SRCS}
)

target_link_libraries(RedComplexDatabaseModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::Sql

		RedComplexBase
		RedComplexResources
)
