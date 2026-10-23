message(STATUS "> Build Resources")

set(Resources-Sources
	${RedComplex_SOURCE_DIR}/src/Libs/Resources
)

file(GLOB_RECURSE SRCS ${Resources-Sources}/*.cpp)

qt_add_library(RedComplexResources STATIC
	${SRCS}
)

set_target_properties(RedComplexResources PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexResources PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexResources PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexResources
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		sqlcipher::sqlcipher

		RedComplexBase
		RedComplexDataStream
)
