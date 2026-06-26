message(STATUS "> Build Base")

set(Base-Sources
	${RedComplex_SOURCE_DIR}/src/Libs/Base
)

file(GLOB_RECURSE SRCS ${Base-Sources}/*.cpp)

add_library(RedComplexBase
	${SRCS}
)

target_link_libraries(RedComplexBase
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
)
