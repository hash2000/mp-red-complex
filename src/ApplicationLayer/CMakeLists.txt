message(STATUS "> Build ApplicationLayer")

set(ApplicationLayer-Sources
	${RedComplex_SOURCE_DIR}/src/ApplicationLayer
)

set(CMAKE_AUTOMOC ON)

file(GLOB_RECURSE SRCS ${ApplicationLayer-Sources}/*.cpp)

add_library(RedComplexApplicationLayer
	${SRCS}
)

target_link_libraries(RedComplexApplicationLayer
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		RedComplexBase
		RedComplexDataStream
		RedComplexDataLayer
		RedComplexResources
		RedComplexGraphics
)
