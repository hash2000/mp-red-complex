message(STATUS "> Build Base")

set(Base-Sources
	${RedComplex_SOURCE_DIR}/src/Base
)

file(GLOB_RECURSE SRCS ${Base-Sources}/*.cpp)
add_library(Base
	${SRCS}
)

target_link_libraries(Base
	PRIVATE
		Qt6::Core
)
