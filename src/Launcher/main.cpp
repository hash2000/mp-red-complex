#include "Launcher/app_controller.h"
#include "Libs/Engine/application.h"

#include <QCoreApplication>

class LauncherApplication : public Application {
protected:
	QString organizationName() const override {
		return "Hash2000";
	}
	QString organizationDomain() const override {
		return "ru.hash2000";
	}
	QString applicationName() const override {
		return "red-complex";
	}

	CommandController* buildCommandController(Resources* resources) override {
		appController = std::make_unique<ApplicationController>(resources);
		return appController.get();
	}

private:
	std::unique_ptr<ApplicationController> appController;
};


int main(int argc, char *argv[]) {
	const auto application = std::make_unique<LauncherApplication>();
	return application->run(argc, argv);
}
