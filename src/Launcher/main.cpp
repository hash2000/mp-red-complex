#include "Launcher/application.h"

int main(int argc, char *argv[]) {
	const auto application = Application::create();
	return application->run(argc, argv);
}
