message(STATUS "> Build CharactersModule")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CharactersModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/CharactersModule
)

file(GLOB_RECURSE SRCS ${CharactersModule-Sources}/*.cpp)

qt_add_library(RedComplexCharactersModule STATIC
	${SRCS}
)

target_link_libraries(RedComplexCharactersModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexResources
)
