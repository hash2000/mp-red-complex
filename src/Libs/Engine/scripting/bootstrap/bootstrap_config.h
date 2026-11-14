#pragma once
#include <QString>
#include <list>

namespace BootstrapConfig {

struct Application {
	QString name;
	QString viewport;
};

struct Store {
	QString name;

	struct {
		int checkInterval = 60000;
		int maxSizeMb = 20;
	} wal;
};

struct Config {
	QString themeName;
	Application application;
	std::list<Store> stores;
};

Config loadBootstrap(const QString& path);
}
