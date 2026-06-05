

find_package(Qt6
	REQUIRED COMPONENTS
		Core
		Gui
		Widgets
		OpenGLWidgets
		Sql)

qt_standard_project_setup()

message(STATUS "Using Qt version: ${Qt6_VERSION}")

find_package(ZLIB REQUIRED)

find_package(EnTT CONFIG REQUIRED)

find_package(SQLite3 REQUIRED)
