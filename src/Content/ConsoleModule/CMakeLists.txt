message(STATUS "> Build ConsoleModule")

set(ConsoleModule-Sources
	${RedComplex_SOURCE_DIR}/src/Content/ConsoleModule
)

file(GLOB_RECURSE SRCS ${ConsoleModule-Sources}/*.cpp)

qt_add_library(RedComplexConsoleModule STATIC
	${SRCS}
)

set_target_properties(RedComplexConsoleModule PROPERTIES CMAKE_AUTOMOC ON)
set_target_properties(RedComplexConsoleModule PROPERTIES CMAKE_AUTORCC ON)
set_target_properties(RedComplexConsoleModule PROPERTIES CMAKE_AUTOUIC ON)

target_link_libraries(RedComplexConsoleModule
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::Xml

		RedComplexBase
		RedComplexEngine
		RedComplexDataStream
		RedComplexResources
)
