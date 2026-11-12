#pragma once
#include <QString>

namespace BootstrapConfig {

struct Application {
	QString name;
	QString viewport;
};

struct Config {
	QString themeName;
	Application application;
};

Config loadBootstrap(const QString& path);
}

