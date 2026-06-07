message(STATUS "> Build DataStream")

set(DataStream-Sources
	${RedComplex_SOURCE_DIR}/src/Libs/DataStream
)

file(GLOB_RECURSE SRCS ${DataStream-Sources}/*.cpp)


add_library(RedComplexDataStream
	${SRCS}
)

target_include_directories(RedComplexDataStream PRIVATE
	${ZLIB_INCLUDE_DIRS})

target_link_libraries(RedComplexDataStream
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		ZLIB::ZLIB

		RedComplexBase
)

