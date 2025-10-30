#include "application.h"
#include "engine.h"

std::unique_ptr<Application> Application::create() {
  return std::make_unique<Application>();
}

int Application::run(int &argc, char **argv) {
	init();
	Engine engine(argc, argv);
	engine.configure(*_config);
	return engine.exec();
}

void Application::init() {
	QApplication::setApplicationName(QString::fromUtf8("Red Complex"));

	_config = std::make_unique<Config>(Config::getDefult());

}
