#include "Launcher/main_frame.h"
#include "Engine/application.h"

class LauncherApplication : public Application {
protected:
	virtual std::unique_ptr<MainFrame> createMainFrame() override {
		return std::make_unique<LauncherMainFrame>(resources());
	}
};


int main(int argc, char *argv[]) {
	const auto application = std::make_unique<LauncherApplication>();
	return application->run(argc, argv);
}
