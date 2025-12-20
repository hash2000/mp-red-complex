message(STATUS "> Build Launcher")

set(Launcher-Sources
	${RedComplex_SOURCE_DIR}/src/Launcher
)

file(GLOB_RECURSE SRCS ${Launcher-Sources}/*.cpp)
add_executable(Launcher
	${SRCS}
)

target_link_libraries(Launcher
	PRIVATE
		Qt6::Core
		Qt6::Gui
		Qt6::Widgets
		Qt6::OpenGL
		Qt6::OpenGLWidgets

		${ZLIB_LIBRARIES}

		Base
		Resources
		DataFormat
)

# target_include_directories(Launcher
# 	PUBLIC
# 		${ZLIB_INCLUDE_DIRS}
# )
