message(STATUS "> Build CharactersModule")

set(CharactersModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/CharactersModule
)

file(GLOB_RECURSE SRCS ${CharactersModule-Sources}/*.cpp)

qt_add_library(RedComplexCharactersModule STATIC
	${SRCS}
)

set_target_properties(RedComplexCharactersModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexCharactersModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexCharactersModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexCharactersModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexResources
)
