find_package(Qt6
	QUIET CONFIG OPTIONAL_COMPONENTS
		Core)

qt_standard_project_setup()

find_package(Qt6
	REQUIRED COMPONENTS
		Core
		Gui
		Widgets
		OpenGLWidgets)

find_package(ZLIB REQUIRED)

find_package(EnTT CONFIG REQUIRED)
