#include "Launcher/engine.h"
#include "engine.h"

Engine::Engine(int &argc, char **argv)
	: QApplication(argc, argv) {
}

void Engine::configure(const Config &config) {
	setApplicationName(config.application_path_name);
	setOrganizationName(config.organization_name);
	styleHints()->setColorScheme(config.color_scheme);
}

void Engine::setup(Config &config, Resources& resources) {
	config.setupPath();
	config.loadSettings();
	resources.configure(config);
	resources.loadDatFile("master.dat");

}

void Engine::setupMainFrame(std::unique_ptr<MainFrame> &&mainFrame) {
	_main_frame = std::move(mainFrame);
	_main_frame->resize(800, 600);
	_main_frame->show();
}

bool Engine::nativeEventFilter(
	const QByteArray &eventType, void *message,
  qintptr *result) {
  return false;
}
