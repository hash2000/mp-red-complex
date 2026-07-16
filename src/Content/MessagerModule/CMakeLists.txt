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

set_target_properties(RedComplexMessagerModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexMessagerModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexMessagerModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexMessagerModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexDataStream
		RedComplexResources
)
