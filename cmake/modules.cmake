find_package(Qt6
	REQUIRED COMPONENTS
		Core
		Gui
		Widgets
		OpenGLWidgets)

qt_standard_project_setup()

find_package(ZLIB REQUIRED)
