#pragma once
#include <QString>
#include <QtGlobal>
#include <list>

namespace BootstrapConfig {

struct Application {
	QString name;
	QString viewport;
};

struct Migration {
	qint64 version = 0;
	QString description;
	QString up;
	QString down;
};

struct Store {
	QString name;
	QString path;
	bool encrypted = false;
	std::list<Migration> migrations;

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
