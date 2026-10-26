#include "Launcher/main_frame.h"
#include "Libs/Engine/application.h"
#include <QCoreApplication>

class LauncherApplication : public Application {
protected:
	std::unique_ptr<MainFrame> createMainFrame(Resources* resources) override {
		return std::make_unique<LauncherMainFrame>(resources);
	}
	QString organizationName() const {
		return "Hash2000";
	}
	QString organizationDomain() const {
		return "ru.hash2000";
	}
	QString applicationName() const {
		return "red-complex";
	}
};


int main(int argc, char *argv[]) {
	const auto application = std::make_unique<LauncherApplication>();
	return application->run(argc, argv);
}
