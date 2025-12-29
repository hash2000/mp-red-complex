#include "Engine/engine.h"
#include "Resources/resources/model/assets_model.h"
#include "DataFormat/proto/procedure.h"

Engine::Engine(int &argc, char **argv)
	: QApplication(argc, argv) {
}

void Engine::configure(const std::shared_ptr<Config> &config) {
	setApplicationName(config->application_path_name);
	setOrganizationName(config->organization_name);
	styleHints()->setColorScheme(config->color_scheme);
}

void Engine::setup(std::shared_ptr<Config> &config, std::shared_ptr<Resources> &resources) {
	config->setupPath();
	config->loadSettings();
	resources->configure(config);
	resources->load();

	registerTypes();
}

void Engine::setupMainFrame(std::unique_ptr<MainFrame> &&mainFrame) {
	_main_frame = std::move(mainFrame);
	_main_frame->resize(800, 600);
	_main_frame->show();
}

void Engine::registerTypes() {
	qRegisterMetaType<AssetsViewItemType>("AssetsViewItemType");
	qRegisterMetaType<ProgramOpCode>("OpCode");
}

bool Engine::nativeEventFilter(
	const QByteArray &eventType, void *message,
  qintptr *result) {
  return false;
}
