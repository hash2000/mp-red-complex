#include "Libs/Engine/application.h"
#include <QCoreApplication>

class LauncherApplication : public Application {
protected:
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
