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

		RedComplexBase
		RedComplexBaseWidgets
		RedComplexGraphics		
		RedComplexDataLayer
		RedComplexApplicationLayer
		RedComplexEngine

		RedComplexCodeEditorWidget

		RedComplexUsersModule
		RedComplexDatabaseModule
		RedComplexInventoriesModule
		RedComplexTexturesModule
		RedComplexShadersModule
)

