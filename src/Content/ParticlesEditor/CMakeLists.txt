message(STATUS "> Build Content.ParticlesEditor")

set(ContentParticlesEditor-Sources
	${RedComplex_SOURCE_DIR}/src/Content/ParticlesEditor
)

file(GLOB_RECURSE SRCS ${ContentParticlesEditor-Sources}/*.cpp)


add_library(Content.ParticlesEditor
	${SRCS}
)

target_include_directories(Content.ParticlesEditor PRIVATE
	${ZLIB_INCLUDE_DIRS})

target_link_libraries(Content.ParticlesEditor
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets

		Base
		DataFormat
		DataStream
		Resources
)

