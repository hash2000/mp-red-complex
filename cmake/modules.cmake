
find_package(Poco
  CONFIG REQUIRED COMPONENTS
    Foundation
    XML
    JSON
    Util
    Net
		NetSSL
    Crypto
    JWT
    Encodings)


find_package(Qt6
	REQUIRED COMPONENTS
		Core
		Gui
		Widgets)

qt_standard_project_setup()
