#include "engine.h"

Engine::Engine(int &argc, char **argv)
	: QApplication(argc, argv) {
}

void Engine::configure(const Config &config) {
	styleHints()->setColorScheme(config.color_scheme);
}

bool Engine::nativeEventFilter(
	const QByteArray &eventType, void *message,
  qintptr *result) {
  return false;
}
