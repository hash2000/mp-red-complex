

find_package(Qt6
	REQUIRED COMPONENTS
		Core
		Gui
		Widgets
		OpenGLWidgets
		Network
		Xml
		Concurrent
)

qt_standard_project_setup()
message(STATUS "Using Qt version: ${Qt6_VERSION}")

find_package(ZLIB REQUIRED)

find_package(EnTT CONFIG REQUIRED)

find_package(sqlcipher CONFIG REQUIRED)

find_package(OpenSSL REQUIRED)

find_package(unofficial-sodium CONFIG REQUIRED)

