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


message(STATUS "Qt6_DIR = ${Qt6_DIR}")
message(STATUS "Qt6Core_DIR = ${Qt6Core_DIR}")
get_target_property(_qt6_core_location Qt6::Core LOCATION)
message(STATUS "Qt6::Core LOCATION = ${_qt6_core_location}")
message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")


find_package(ZLIB REQUIRED)
